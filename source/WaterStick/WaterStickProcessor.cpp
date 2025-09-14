#include "WaterStickProcessor.h"
#include "WaterStickCIDs.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"
#include <cmath>

using namespace Steinberg;

namespace WaterStick {

//------------------------------------------------------------------------
// FadeDelayLine Implementation
//------------------------------------------------------------------------

FadeDelayLine::FadeDelayLine()
: mBufferSize(0)
, mWriteIndex(0)
, mCurrentDelayTime(0.0f)
, mTargetDelayTime(0.0f)
, mSampleRate(44100.0)
, mFadeLength(0)
, mFadePosition(0)
, mFading(false)
, mReadIndex0(0)
, mReadIndex1(0)
{
}

FadeDelayLine::~FadeDelayLine()
{
}

void FadeDelayLine::initialize(double sampleRate, double maxDelaySeconds)
{
    mSampleRate = sampleRate;
    mBufferSize = static_cast<int>(maxDelaySeconds * sampleRate) + 1;
    mBuffer.resize(mBufferSize, 0.0f);
    mWriteIndex = 0;
    // Set minimum delay time (4 samples) to avoid reading uninitialized buffer
    float minDelayTime = 4.0f / static_cast<float>(mSampleRate);
    mCurrentDelayTime = minDelayTime;
    mTargetDelayTime = minDelayTime;

    // ER301 uses 25ms fade length
    mFadeLength = static_cast<int>(sampleRate * 0.025f);
    mFadeFrames.resize(mFadeLength);

    // Generate smooth fade curve (could be linear or curved)
    for (int i = 0; i < mFadeLength; i++) {
        mFadeFrames[i] = static_cast<float>(i) / static_cast<float>(mFadeLength - 1);
    }

    mFadePosition = 0;
    mFading = false;
    mReadIndex0 = 0;
    mReadIndex1 = 0;
}

void FadeDelayLine::setDelayTime(float delayTimeSeconds)
{
    // Ensure minimum delay time to avoid reading uninitialized buffer
    float minDelayTime = 4.0f / static_cast<float>(mSampleRate); // 4 samples minimum
    float maxDelayTime = static_cast<float>(mBufferSize) / static_cast<float>(mSampleRate);
    float newDelayTime = std::max(minDelayTime, std::min(delayTimeSeconds, maxDelayTime));

    if (std::abs(newDelayTime - mCurrentDelayTime) > 0.0001f)
    {
        mTargetDelayTime = newDelayTime;
        startFade();
    }
}

int FadeDelayLine::quantizeToFour(int samples)
{
    // ER301: quantize to multiples of 4 samples
    return (samples / 4) * 4;
}

void FadeDelayLine::startFade()
{
    // Calculate delay in samples and quantize to 4-sample boundaries (ER301 style)
    int currentDelaySamples = quantizeToFour(static_cast<int>(mCurrentDelayTime * mSampleRate));
    int targetDelaySamples = quantizeToFour(static_cast<int>(mTargetDelayTime * mSampleRate));

    // Ensure minimum delay to avoid reading uninitialized data
    if (currentDelaySamples < 4) currentDelaySamples = 4;
    if (targetDelaySamples < 4) targetDelaySamples = 4;

    // Calculate INTEGER read indices
    mReadIndex0 = mWriteIndex - currentDelaySamples;
    mReadIndex1 = mWriteIndex - targetDelaySamples;

    // Handle wrap-around with proper modulo
    while (mReadIndex0 < 0) mReadIndex0 += mBufferSize;
    while (mReadIndex1 < 0) mReadIndex1 += mBufferSize;

    mReadIndex0 %= mBufferSize;
    mReadIndex1 %= mBufferSize;

    mFading = true;
    mFadePosition = 0;
}

float FadeDelayLine::interpolateLinear(float a, float b, float t)
{
    return a + t * (b - a);
}

void FadeDelayLine::processSample(float input, float& output)
{
    if (mFading)
    {
        // Read from INTEGER indices BEFORE writing new input
        float y0 = mBuffer[mReadIndex0];
        float y1 = mBuffer[mReadIndex1];

        // Get fade weight from pre-calculated fade curve
        float fadeWeight = (mFadePosition < mFadeLength) ? mFadeFrames[mFadePosition] : 1.0f;
        output = interpolateLinear(y0, y1, fadeWeight);

        // Advance fade position
        mFadePosition++;
        if (mFadePosition >= mFadeLength)
        {
            mFading = false;
            mCurrentDelayTime = mTargetDelayTime;
        }

        // Update INTEGER read indices to follow write index
        mReadIndex0 = (mReadIndex0 + 1) % mBufferSize;
        mReadIndex1 = (mReadIndex1 + 1) % mBufferSize;
    }
    else
    {
        // Normal operation - read from quantized delay time
        int delayInSamples = quantizeToFour(static_cast<int>(mCurrentDelayTime * mSampleRate));

        // Ensure minimum delay to avoid reading uninitialized data
        if (delayInSamples < 4) delayInSamples = 4;

        int readIndex = mWriteIndex - delayInSamples;
        while (readIndex < 0) readIndex += mBufferSize;
        readIndex %= mBufferSize;

        output = mBuffer[readIndex];
    }

    // Write input to buffer AFTER reading output
    mBuffer[mWriteIndex] = input;

    // Advance write index
    mWriteIndex = (mWriteIndex + 1) % mBufferSize;
}

void FadeDelayLine::reset()
{
    std::fill(mBuffer.begin(), mBuffer.end(), 0.0f);
    mWriteIndex = 0;
    mCurrentDelayTime = 0.0f;
    mTargetDelayTime = 0.0f;
    mFadePosition = 0;
    mFading = false;
    mReadIndex0 = 0;
    mReadIndex1 = 0;
}

//------------------------------------------------------------------------
// WaterStickProcessor Implementation
//------------------------------------------------------------------------

WaterStickProcessor::WaterStickProcessor()
: mInputGain(1.0f)
, mOutputGain(1.0f)
, mDelayTime(0.1f)  // 100ms default
, mDryWet(0.5f)     // 50% wet default
, mSampleRate(44100.0)
{
    setControllerClass(kWaterStickControllerUID);
}

WaterStickProcessor::~WaterStickProcessor()
{
}

tresult PLUGIN_API WaterStickProcessor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk)
    {
        return result;
    }

    // Add audio input/output busses
    addAudioInput(STR16("Stereo In"), Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::terminate()
{
    return AudioEffect::terminate();
}

tresult PLUGIN_API WaterStickProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
{
    mSampleRate = newSetup.sampleRate;

    // Initialize delay lines with 2 second max delay
    mDelayLineL.initialize(mSampleRate, 2.0);
    mDelayLineR.initialize(mSampleRate, 2.0);

    return AudioEffect::setupProcessing(newSetup);
}

void WaterStickProcessor::updateParameters()
{
    mDelayLineL.setDelayTime(mDelayTime);
    mDelayLineR.setDelayTime(mDelayTime);
}

tresult PLUGIN_API WaterStickProcessor::process(Vst::ProcessData& data)
{
    // Process parameter changes
    if (data.inputParameterChanges)
    {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < numParamsChanged; i++)
        {
            Vst::IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData(i);
            if (paramQueue)
            {
                Vst::ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();

                if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                {
                    switch (paramQueue->getParameterId())
                    {
                        case kInputGain:
                            mInputGain = static_cast<float>(value);
                            break;
                        case kOutputGain:
                            mOutputGain = static_cast<float>(value);
                            break;
                        case kDelayTime:
                            mDelayTime = static_cast<float>(value * 2.0); // 0-2 seconds
                            break;
                        case kDryWet:
                            mDryWet = static_cast<float>(value); // 0-1 (dry to wet)
                            break;
                    }
                }
            }
        }
    }

    updateParameters();

    // Check for valid input/output
    if (data.numInputs == 0 || data.numOutputs == 0)
    {
        return kResultOk;
    }

    Vst::AudioBusBuffers* input = data.inputs;
    Vst::AudioBusBuffers* output = data.outputs;

    // Ensure we have stereo input/output
    if (input->numChannels < 2 || output->numChannels < 2)
    {
        return kResultOk;
    }

    float* inputL = input->channelBuffers32[0];
    float* inputR = input->channelBuffers32[1];
    float* outputL = output->channelBuffers32[0];
    float* outputR = output->channelBuffers32[1];

    // Process samples
    for (int32 sample = 0; sample < data.numSamples; sample++)
    {
        // Get input samples
        float inL = inputL[sample];
        float inR = inputR[sample];

        // Apply input gain
        float gainedL = inL * mInputGain;
        float gainedR = inR * mInputGain;

        // Process through delay lines
        float delayedL, delayedR;
        mDelayLineL.processSample(gainedL, delayedL);
        mDelayLineR.processSample(gainedR, delayedR);

        // Dry/wet mix
        float dryGain = 1.0f - mDryWet;
        float wetGain = mDryWet;
        float mixedL = (inL * dryGain) + (delayedL * wetGain);
        float mixedR = (inR * dryGain) + (delayedR * wetGain);

        // Apply output gain and write to output
        outputL[sample] = mixedL * mOutputGain;
        outputR[sample] = mixedR * mOutputGain;
    }

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::getState(IBStream* state)
{
    IBStreamer streamer(state, kLittleEndian);

    streamer.writeFloat(mInputGain);
    streamer.writeFloat(mOutputGain);
    streamer.writeFloat(mDelayTime);
    streamer.writeFloat(mDryWet);

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::setState(IBStream* state)
{
    IBStreamer streamer(state, kLittleEndian);

    streamer.readFloat(mInputGain);
    streamer.readFloat(mOutputGain);
    streamer.readFloat(mDelayTime);
    streamer.readFloat(mDryWet);

    return kResultOk;
}

} // namespace WaterStick