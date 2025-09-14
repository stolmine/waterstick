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
, combBuffers(nullptr)
, combBufferSize(0)
, combWritePos(0)
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

    // Initialize comb LP filter state
    combLPState[0] = 0.0;
    combLPState[1] = 0.0;
}

WaterStickProcessor::~WaterStickProcessor()
{
    deallocateDelayBuffers();
    deallocateCombBuffers();
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
    deallocateCombBuffers();
    return AudioEffect::terminate();
}

tresult PLUGIN_API WaterStickProcessor::setActive(TBool state)
{
    if (state)
    {
        allocateDelayBuffers();
        allocateCombBuffers();
    }
    else
    {
        deallocateDelayBuffers();
        deallocateCombBuffers();
    }

    return AudioEffect::setActive(state);
}

tresult PLUGIN_API WaterStickProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
{
    // Calculate delay buffer size (max 5 seconds at any sample rate)
    delayBufferSize = static_cast<int32>(5.0 * newSetup.sampleRate);

    // Calculate comb buffer size (max 100ms at any sample rate for comb resonator)
    combBufferSize = static_cast<int32>(0.1 * newSetup.sampleRate);

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
    processAudio(input, output, numChannels, sampleFrames);

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

void WaterStickProcessor::allocateCombBuffers()
{
    deallocateCombBuffers();

    if (combBufferSize > 0)
    {
        combBuffers = new Vst::Sample64*[2]; // Stereo
        for (int32 channel = 0; channel < 2; ++channel)
        {
            combBuffers[channel] = new Vst::Sample64[combBufferSize];
            memset(combBuffers[channel], 0, combBufferSize * sizeof(Vst::Sample64));
        }
        combWritePos = 0;
    }
}

void WaterStickProcessor::deallocateCombBuffers()
{
    if (combBuffers)
    {
        for (int32 channel = 0; channel < 2; ++channel)
        {
            delete[] combBuffers[channel];
        }
        delete[] combBuffers;
        combBuffers = nullptr;
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

void WaterStickProcessor::processComb(Vst::Sample32** inputs, Vst::Sample32** outputs,
                                     int32 numChannels, int32 sampleFrames)
{
    if (!combBuffers || combBufferSize == 0)
        return;

    // Calculate comb delay length (in samples) based on combSize parameter
    // combSize range 0.0-1.0 maps to 2ms-100ms delay
    double combDelayMs = 2.0 + (combSize * 98.0);
    int32 combDelaySamples = static_cast<int32>(combDelayMs * 0.001 * processSetup.sampleRate);
    combDelaySamples = std::max(1, std::min(combDelaySamples, combBufferSize - 1));

    // Calculate damping coefficient (0.0-1.0)
    double dampingCoeff = combDamping * 0.3; // Scale to reasonable range

    for (int32 sample = 0; sample < sampleFrames; ++sample)
    {
        for (int32 channel = 0; channel < std::min(numChannels, 2); ++channel)
        {
            // Get input sample
            Vst::Sample64 input = inputs[channel][sample];

            // Calculate read position for comb delay
            int32 readPos = combWritePos - combDelaySamples;
            if (readPos < 0)
                readPos += combBufferSize;

            // Get delayed sample from comb buffer
            Vst::Sample64 delayedSample = combBuffers[channel][readPos];

            // Apply damping (simple low-pass filter)
            combLPState[channel] = combLPState[channel] + dampingCoeff * (delayedSample - combLPState[channel]);
            Vst::Sample64 dampedSample = combLPState[channel];

            // Write to comb buffer (input + feedback)
            combBuffers[channel][combWritePos] = input + (dampedSample * combFeedback);

            // Output is the delayed/damped sample
            outputs[channel][sample] = static_cast<Vst::Sample32>(dampedSample);
        }

        // Advance write position
        combWritePos = (combWritePos + 1) % combBufferSize;
    }
}

void WaterStickProcessor::processAudio(Vst::Sample32** inputs, Vst::Sample32** outputs,
                                      int32 numChannels, int32 sampleFrames)
{
    // Temporary buffers for routing between delay and comb
    Vst::Sample32* tempBuffers[2];
    tempBuffers[0] = new Vst::Sample32[sampleFrames];
    tempBuffers[1] = new Vst::Sample32[sampleFrames];
    Vst::Sample32** tempOutputs = tempBuffers;

    // Process delay first
    processDelay(inputs, tempOutputs, numChannels, sampleFrames);

    // Then process comb (using delay output as input)
    processComb(tempOutputs, outputs, numChannels, sampleFrames);

    // Mix delay and comb outputs
    for (int32 channel = 0; channel < std::min(numChannels, 2); ++channel)
    {
        for (int32 sample = 0; sample < sampleFrames; ++sample)
        {
            // Blend delay and comb outputs (50/50 mix for now)
            outputs[channel][sample] = (tempOutputs[channel][sample] * 0.5f) + (outputs[channel][sample] * 0.5f);
        }
    }

    // Clean up temporary buffers
    delete[] tempBuffers[0];
    delete[] tempBuffers[1];
}

} // namespace WaterStick