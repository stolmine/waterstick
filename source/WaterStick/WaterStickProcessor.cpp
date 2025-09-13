#include "WaterStickProcessor.h"
#include "WaterStickCIDs.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/ustring.h"
#include "base/source/fstreamer.h"
#include <algorithm>
#include <cmath>

using namespace Steinberg;

namespace WaterStick {

WaterStickProcessor::WaterStickProcessor()
: delayBuffers(nullptr)
, delayBufferSize(0)
, delayWritePos(0)
, delayTime(0.25)      // 250ms default
, delayFeedback(0.3)   // 30% feedback
, delayMix(0.5)        // 50% mix
, combSize(0.5)        // Medium comb size
, combFeedback(0.4)    // 40% comb feedback
, combDamping(0.5)     // Medium damping
, inputGain(1.0)       // Unity gain
, outputGain(1.0)      // Unity gain
, bypass(0.0)          // Not bypassed
{
    setControllerClass(kWaterStickControllerUID);
}

WaterStickProcessor::~WaterStickProcessor()
{
    deallocateDelayBuffers();
}

tresult PLUGIN_API WaterStickProcessor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk)
        return result;

    // Add audio input/output buses
    addAudioInput(STR16("Stereo In"), Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::terminate()
{
    deallocateDelayBuffers();
    return AudioEffect::terminate();
}

tresult PLUGIN_API WaterStickProcessor::setActive(TBool state)
{
    if (state)
        allocateDelayBuffers();
    else
        deallocateDelayBuffers();

    return AudioEffect::setActive(state);
}

tresult PLUGIN_API WaterStickProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
{
    // Calculate delay buffer size (max 5 seconds at any sample rate)
    delayBufferSize = static_cast<int32>(5.0 * newSetup.sampleRate);
    return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API WaterStickProcessor::canProcessSampleSize(int32 symbolicSampleSize)
{
    if (symbolicSampleSize == Vst::kSample32)
        return kResultTrue;

    return kResultFalse;
}

tresult PLUGIN_API WaterStickProcessor::process(Vst::ProcessData& data)
{
    // Process parameter changes
    if (data.inputParameterChanges)
    {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
        for (int32 index = 0; index < numParamsChanged; index++)
        {
            if (auto* paramQueue = data.inputParameterChanges->getParameterData(index))
            {
                Vst::ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();

                if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                {
                    switch (paramQueue->getParameterId())
                    {
                        case kDelayTime: delayTime = value; break;
                        case kDelayFeedback: delayFeedback = value; break;
                        case kDelayMix: delayMix = value; break;
                        case kCombSize: combSize = value; break;
                        case kCombFeedback: combFeedback = value; break;
                        case kCombDamping: combDamping = value; break;
                        case kInputGain: inputGain = value; break;
                        case kOutputGain: outputGain = value; break;
                        case kBypass: bypass = value; break;
                    }
                }
            }
        }
    }

    // Check if we have audio input and output
    if (data.numInputs == 0 || data.numOutputs == 0)
        return kResultOk;

    // Get audio buffers
    Vst::Sample32** input = data.inputs[0].channelBuffers32;
    Vst::Sample32** output = data.outputs[0].channelBuffers32;

    int32 numChannels = data.inputs[0].numChannels;
    int32 sampleFrames = data.numSamples;

    // Bypass processing
    if (bypass > 0.5)
    {
        for (int32 channel = 0; channel < numChannels; ++channel)
        {
            if (input != output)
            {
                memcpy(output[channel], input[channel], sampleFrames * sizeof(Vst::Sample32));
            }
        }
        return kResultOk;
    }

    // Process audio
    processDelay(input, output, numChannels, sampleFrames);

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::getState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    // Write parameters to state
    IBStreamer streamer(state, kLittleEndian);

    streamer.writeDouble(delayTime);
    streamer.writeDouble(delayFeedback);
    streamer.writeDouble(delayMix);
    streamer.writeDouble(combSize);
    streamer.writeDouble(combFeedback);
    streamer.writeDouble(combDamping);
    streamer.writeDouble(inputGain);
    streamer.writeDouble(outputGain);
    streamer.writeDouble(bypass);

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::setState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    // Read parameters from state
    IBStreamer streamer(state, kLittleEndian);

    if (!streamer.readDouble(delayTime)) return kResultFalse;
    if (!streamer.readDouble(delayFeedback)) return kResultFalse;
    if (!streamer.readDouble(delayMix)) return kResultFalse;
    if (!streamer.readDouble(combSize)) return kResultFalse;
    if (!streamer.readDouble(combFeedback)) return kResultFalse;
    if (!streamer.readDouble(combDamping)) return kResultFalse;
    if (!streamer.readDouble(inputGain)) return kResultFalse;
    if (!streamer.readDouble(outputGain)) return kResultFalse;
    if (!streamer.readDouble(bypass)) return kResultFalse;

    return kResultOk;
}

void WaterStickProcessor::allocateDelayBuffers()
{
    deallocateDelayBuffers();

    if (delayBufferSize > 0)
    {
        delayBuffers = new Vst::Sample64*[2]; // Stereo
        for (int32 channel = 0; channel < 2; ++channel)
        {
            delayBuffers[channel] = new Vst::Sample64[delayBufferSize];
            memset(delayBuffers[channel], 0, delayBufferSize * sizeof(Vst::Sample64));
        }
        delayWritePos = 0;
    }
}

void WaterStickProcessor::deallocateDelayBuffers()
{
    if (delayBuffers)
    {
        for (int32 channel = 0; channel < 2; ++channel)
        {
            delete[] delayBuffers[channel];
        }
        delete[] delayBuffers;
        delayBuffers = nullptr;
    }
}

void WaterStickProcessor::processDelay(Vst::Sample32** inputs, Vst::Sample32** outputs,
                                      int32 numChannels, int32 sampleFrames)
{
    if (!delayBuffers || delayBufferSize == 0)
        return;

    // Calculate delay samples
    int32 delaySamples = static_cast<int32>(delayTime * processSetup.sampleRate);
    delaySamples = std::min(delaySamples, delayBufferSize - 1);

    for (int32 sample = 0; sample < sampleFrames; ++sample)
    {
        for (int32 channel = 0; channel < std::min(numChannels, 2); ++channel)
        {
            // Input with gain
            Vst::Sample64 input = inputs[channel][sample] * inputGain;

            // Calculate read position
            int32 readPos = delayWritePos - delaySamples;
            if (readPos < 0)
                readPos += delayBufferSize;

            // Get delayed sample
            Vst::Sample64 delayedSample = delayBuffers[channel][readPos];

            // Write to delay buffer (input + feedback)
            delayBuffers[channel][delayWritePos] = input + (delayedSample * delayFeedback);

            // Mix output
            Vst::Sample64 output = input * (1.0 - delayMix) + delayedSample * delayMix;

            // Apply output gain
            outputs[channel][sample] = static_cast<Vst::Sample32>(output * outputGain);
        }

        // Advance write position
        delayWritePos = (delayWritePos + 1) % delayBufferSize;
    }
}

} // namespace WaterStick