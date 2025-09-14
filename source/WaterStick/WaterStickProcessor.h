#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "WaterStickParameters.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace WaterStick {

class FadeDelayLine {
public:
    FadeDelayLine();
    ~FadeDelayLine();

    void initialize(double sampleRate, double maxDelaySeconds);
    void setDelayTime(float delayTimeSeconds);
    void processSample(float input, float& output);
    void reset();

private:
    std::vector<float> mBuffer;
    int mBufferSize;
    int mWriteIndex;
    float mCurrentDelayTime;
    float mTargetDelayTime;
    double mSampleRate;

    // Fade parameters for smooth transitions (ER301 style)
    std::vector<float> mFadeFrames;
    int mFadeLength;
    int mFadePosition;
    bool mFading;

    // INTEGER read indices only (ER301 approach)
    int mReadIndex0, mReadIndex1;

    void startFade();
    int quantizeToFour(int samples); // ER301: quantize to 4-sample boundaries
    float interpolateLinear(float a, float b, float t);
};

class WaterStickProcessor : public Steinberg::Vst::AudioEffect
{
public:
    WaterStickProcessor();
    ~WaterStickProcessor() SMTG_OVERRIDE;

    // Create function used by factory
    static Steinberg::FUnknown* createInstance(void* /*context*/)
    {
        return (Steinberg::Vst::IAudioProcessor*)new WaterStickProcessor;
    }

    // IPluginBase
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

    // IAudioProcessor
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;

    // IComponent
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;

private:
    // Parameters
    float mInputGain;
    float mOutputGain;
    float mDelayTime;
    float mDryWet;

    // DSP
    FadeDelayLine mDelayLineL;
    FadeDelayLine mDelayLineR;
    double mSampleRate;

    void updateParameters();
};

} // namespace WaterStick