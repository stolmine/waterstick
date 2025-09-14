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
        kCombDensity,
        kCombMix,

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

    // Multitap comb structures
    struct CombTap {
        Steinberg::int32 delaySamples;     // Current delay length in samples
        Steinberg::int32 targetDelaySamples; // Target delay length for crossfade
        Steinberg::Vst::Sample64 gain;     // Tap gain
        Steinberg::Vst::Sample64 targetGain; // Target gain for crossfade
        Steinberg::Vst::Sample64 lpState[2]; // Per-tap damping filter state
        Steinberg::Vst::Sample64 crossfade;  // Crossfade amount (0.0-1.0)
    };
    CombTap combTaps[kMaxCombTaps];
    Steinberg::int32 activeTapCount;
    Steinberg::int32 targetActiveTapCount;

    // Crossfade parameters
    bool needsCrossfade;
    Steinberg::int32 crossfadeSamples;
    static const Steinberg::int32 kCrossfadeLength = 512; // 512 samples ~= 10ms at 48kHz

    // Parameters
    Steinberg::Vst::ParamValue delayTime;
    Steinberg::Vst::ParamValue delayFeedback;
    Steinberg::Vst::ParamValue delayMix;
    Steinberg::Vst::ParamValue combSize;
    Steinberg::Vst::ParamValue combFeedback;
    Steinberg::Vst::ParamValue combDamping;
    Steinberg::Vst::ParamValue combDensity;
    Steinberg::Vst::ParamValue combMix;
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
    void updateCombTaps();
    void updateCrossfade();
};

} // namespace WaterStick