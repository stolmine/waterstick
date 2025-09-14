#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

namespace WaterStick {

class WaterStickProcessor : public Steinberg::Vst::AudioEffect
{
public:
    WaterStickProcessor();
    ~WaterStickProcessor() SMTG_OVERRIDE;

    // Create function
    static Steinberg::FUnknown* createInstance(void* /*context*/)
    {
        return (Steinberg::Vst::IAudioProcessor*)new WaterStickProcessor;
    }

    // IPluginBase
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

    // IAudioProcessor
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

    // IComponent
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;

protected:
    // Parameters
    enum {
        // Delay Section
        kDelayTime,
        kDelayFeedback,
        kDelayMix,

        // Comb Section
        kCombSize,
        kCombFeedback,
        kCombDamping,

        // Global
        kInputGain,
        kOutputGain,
        kBypass,

        kNumParams
    };

    // Audio processing state
    Steinberg::Vst::Sample64** delayBuffers;
    Steinberg::int32 delayBufferSize;
    Steinberg::int32 delayWritePos;

    // Comb resonator state
    static const Steinberg::int32 kMaxCombTaps = 64;
    Steinberg::Vst::Sample64** combBuffers;
    Steinberg::int32 combBufferSize;
    Steinberg::int32 combWritePos;
    Steinberg::Vst::Sample64 combLPState[2];  // Low-pass filter state for damping

    // Parameters
    Steinberg::Vst::ParamValue delayTime;
    Steinberg::Vst::ParamValue delayFeedback;
    Steinberg::Vst::ParamValue delayMix;
    Steinberg::Vst::ParamValue combSize;
    Steinberg::Vst::ParamValue combFeedback;
    Steinberg::Vst::ParamValue combDamping;
    Steinberg::Vst::ParamValue inputGain;
    Steinberg::Vst::ParamValue outputGain;
    Steinberg::Vst::ParamValue bypass;

private:
    void allocateDelayBuffers();
    void deallocateDelayBuffers();
    void allocateCombBuffers();
    void deallocateCombBuffers();
    void processDelay(Steinberg::Vst::Sample32** inputs, Steinberg::Vst::Sample32** outputs,
                     Steinberg::int32 numChannels, Steinberg::int32 sampleFrames);
    void processComb(Steinberg::Vst::Sample32** inputs, Steinberg::Vst::Sample32** outputs,
                    Steinberg::int32 numChannels, Steinberg::int32 sampleFrames);
    void processAudio(Steinberg::Vst::Sample32** inputs, Steinberg::Vst::Sample32** outputs,
                     Steinberg::int32 numChannels, Steinberg::int32 sampleFrames);
};

} // namespace WaterStick