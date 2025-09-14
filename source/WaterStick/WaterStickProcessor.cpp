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
, activeTapCount(8)    // Default to 8 active taps
, delayTime(0.25)      // 250ms default
, delayFeedback(0.3)   // 30% feedback
, delayMix(0.5)        // 50% mix
, combSize(0.5)        // Medium comb size
, combFeedback(0.4)    // 40% comb feedback
, combDamping(0.5)     // Medium damping
, combDensity(0.125)   // 8/64 = 0.125 (8 active taps default)
, inputGain(1.0)       // Unity gain
, outputGain(1.0)      // Unity gain
, bypass(0.0)          // Not bypassed
{
    setControllerClass(kWaterStickControllerUID);

    // Initialize multitap comb structures
    for (int32 i = 0; i < kMaxCombTaps; ++i)
    {
        combTaps[i].delaySamples = 0;
        combTaps[i].gain = 0.0;
        combTaps[i].lpState[0] = 0.0;
        combTaps[i].lpState[1] = 0.0;
    }
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

    // Update comb tap configuration with new sample rate
    updateCombTaps();

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
                        case kCombSize: combSize = value; updateCombTaps(); break;
                        case kCombFeedback: combFeedback = value; break;
                        case kCombDamping: combDamping = value; break;
                        case kCombDensity: combDensity = value; updateCombTaps(); break;
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
    streamer.writeDouble(combDensity);
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
    if (!streamer.readDouble(combDensity)) return kResultFalse;
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

    // Calculate damping coefficient (0.0-1.0)
    double dampingCoeff = combDamping * 0.3; // Scale to reasonable range

    for (int32 sample = 0; sample < sampleFrames; ++sample)
    {
        for (int32 channel = 0; channel < std::min(numChannels, 2); ++channel)
        {
            // Get input sample
            Vst::Sample64 input = inputs[channel][sample];
            Vst::Sample64 output = 0.0;
            Vst::Sample64 feedbackSum = 0.0;

            // Process all active taps
            for (int32 tap = 0; tap < activeTapCount; ++tap)
            {
                if (combTaps[tap].delaySamples > 0)
                {
                    // Calculate read position for this tap
                    int32 readPos = combWritePos - combTaps[tap].delaySamples;
                    if (readPos < 0)
                        readPos += combBufferSize;

                    // Get delayed sample from comb buffer
                    Vst::Sample64 delayedSample = combBuffers[channel][readPos];

                    // Apply per-tap damping (low-pass filter)
                    combTaps[tap].lpState[channel] = combTaps[tap].lpState[channel] +
                        dampingCoeff * (delayedSample - combTaps[tap].lpState[channel]);
                    Vst::Sample64 dampedSample = combTaps[tap].lpState[channel];

                    // Add tap contribution to output
                    output += dampedSample * combTaps[tap].gain;

                    // Add to feedback sum
                    feedbackSum += dampedSample * combTaps[tap].gain;
                }
            }

            // Write to comb buffer (input + feedback)
            combBuffers[channel][combWritePos] = input + (feedbackSum * combFeedback);

            // Output is the sum of all active taps
            outputs[channel][sample] = static_cast<Vst::Sample32>(output);
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

void WaterStickProcessor::updateCombTaps()
{
    // Calculate number of active taps based on density (0.0-1.0 -> 1-64 taps)
    activeTapCount = static_cast<int32>(1 + (combDensity * (kMaxCombTaps - 1)));
    activeTapCount = std::max(1, std::min(activeTapCount, kMaxCombTaps));

    // Base delay time in milliseconds (2ms-100ms based on combSize)
    double baseDelayMs = 2.0 + (combSize * 98.0);
    double baseDelaySamples = baseDelayMs * 0.001 * processSetup.sampleRate;

    // Configure tap delays using Fibonacci-like distribution for natural resonance
    double tapSpacing = baseDelaySamples / (activeTapCount + 1);

    for (int32 tap = 0; tap < activeTapCount; ++tap)
    {
        // Distribute taps with slight random variation for more natural sound
        double tapMultiplier = 1.0 + (tap * 0.618); // Golden ratio spacing
        combTaps[tap].delaySamples = static_cast<int32>(tapSpacing * tapMultiplier);
        combTaps[tap].delaySamples = std::max(1, std::min(combTaps[tap].delaySamples, combBufferSize - 1));

        // Set tap gain with slight decay for later taps
        combTaps[tap].gain = 1.0 / sqrt(activeTapCount) * (1.0 - tap * 0.05);

        // Reset filter states
        combTaps[tap].lpState[0] = 0.0;
        combTaps[tap].lpState[1] = 0.0;
    }

    // Clear unused taps
    for (int32 tap = activeTapCount; tap < kMaxCombTaps; ++tap)
    {
        combTaps[tap].delaySamples = 0;
        combTaps[tap].gain = 0.0;
        combTaps[tap].lpState[0] = 0.0;
        combTaps[tap].lpState[1] = 0.0;
    }
}

} // namespace WaterStick