#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "WaterStickParameters.h"
#include "ThreeSistersFilter.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace WaterStick {

class TempoSync {
public:
    TempoSync();
    ~TempoSync();

    void initialize(double sampleRate);
    void updateTempo(double hostTempo, bool isValid);
    void setMode(bool isSynced);
    void setSyncDivision(int division);
    void setFreeTime(float timeSeconds);

    float getDelayTime() const;
    const char* getDivisionText() const;
    const char* getModeText() const;

private:
    double mSampleRate;
    double mHostTempo;
    bool mHostTempoValid;
    bool mIsSynced;
    int mSyncDivision;
    float mFreeTime;

    float calculateSyncTime() const;
    static const char* sDivisionTexts[kNumSyncDivisions];
    static const float sDivisionValues[kNumSyncDivisions];
};

class DualDelayLine {
public:
    DualDelayLine();
    ~DualDelayLine();

    void initialize(double sampleRate, double maxDelaySeconds);
    void setDelayTime(float delayTimeSeconds);
    void processSample(float input, float& output);
    void reset();

private:
    // Dual delay lines for crossfading
    std::vector<float> mBufferA;
    std::vector<float> mBufferB;
    int mBufferSize;
    int mWriteIndexA;
    int mWriteIndexB;
    double mSampleRate;

    // Active/Standby line management
    bool mUsingLineA;
    enum CrossfadeState { STABLE, CROSSFADING };
    CrossfadeState mCrossfadeState;

    // Movement detection
    float mTargetDelayTime;
    float mCurrentDelayTime;
    int mStabilityCounter;
    int mStabilityThreshold;

    // Crossfade control
    int mCrossfadeLength;
    int mCrossfadePosition;
    float mCrossfadeGainA;
    float mCrossfadeGainB;

    // Per-line delay state
    struct DelayLineState {
        float delayInSamples;
        int readIndex;
        float allpassCoeff;
        float apInput;
        float lastOutput;
        bool doNextOut;
        float nextOutput;
    };

    DelayLineState mStateA;
    DelayLineState mStateB;

    void updateDelayState(DelayLineState& state, float delayTime);
    void updateAllpassCoeff(DelayLineState& state);
    float processDelayLine(std::vector<float>& buffer, int& writeIndex, DelayLineState& state, float input);
    float nextOut(DelayLineState& state, const std::vector<float>& buffer);
    void startCrossfade();
    void updateCrossfade();
    int calculateCrossfadeLength(float delayTime);
};

class STKDelayLine {
public:
    STKDelayLine();
    ~STKDelayLine();

    void initialize(double sampleRate, double maxDelaySeconds);
    void setDelayTime(float delayTimeSeconds);
    void processSample(float input, float& output);
    void reset();

private:
    std::vector<float> mBuffer;
    int mBufferSize;
    int mWriteIndex;
    int mReadIndex;
    double mSampleRate;

    // Current delay in samples (STK approach - direct, no smoothing)
    float mDelayInSamples;

    // STK allpass interpolation state
    float mAllpassCoeff;
    float mApInput;
    float mLastOutput;
    bool mDoNextOut;
    float mNextOutput;

    void updateAllpassCoeff();
    float nextOut();
};

class CombProcessor {
public:
    CombProcessor();
    ~CombProcessor();

    void initialize(double sampleRate, double maxDelaySeconds = 2.0);
    void setSize(float sizeSeconds);  // Comb size (delay time of tap 64)
    void setNumTaps(int numTaps);     // 1-64 active taps
    void setFeedback(float feedback); // 0-1 feedback amount
    void setSyncMode(bool synced);
    void setClockDivision(int division);
    void setPitchCV(float cv);        // 1V/oct control

    void processStereo(float inputL, float inputR, float& outputL, float& outputR);
    void reset();

    // Get current size for display
    float getCurrentSize() const { return mCombSize; }

private:
    static const int MAX_TAPS = 64;

    // Delay buffers for all taps (stereo)
    std::vector<float> mDelayBufferL;
    std::vector<float> mDelayBufferR;
    int mBufferSize;
    int mWriteIndex;
    double mSampleRate;

    // Comb parameters
    float mCombSize;        // Base delay time in seconds
    int mNumActiveTaps;     // 1-64
    float mFeedback;        // Feedback amount 0-1
    float mPitchCV;         // 1V/oct control voltage

    // Feedback buffers and limiters
    float mFeedbackBufferL;
    float mFeedbackBufferR;

    // Sync parameters
    bool mIsSynced;
    int mClockDivision;
    float mHostTempo;

    // Calculate tap delays based on comb size
    float getTapDelay(int tapIndex) const;
    float applyCVScaling(float baseDelay) const;
    float tanhLimiter(float input) const;
};

class TapDistribution {
public:
    TapDistribution();
    ~TapDistribution();

    void initialize(double sampleRate);
    void updateTempo(const TempoSync& tempoSync);
    void setGrid(int gridValue);
    void setTapEnable(int tapIndex, bool enabled);
    void setTapLevel(int tapIndex, float level);
    void setTapPan(int tapIndex, float pan);

    // Calculate delay time for each tap based on current settings
    void calculateTapTimes();
    float getTapDelayTime(int tapIndex) const;
    bool isTapEnabled(int tapIndex) const;
    float getTapLevel(int tapIndex) const;
    float getTapPan(int tapIndex) const;

    // Get Grid parameter display text
    const char* getGridText() const;

private:
    static const int NUM_TAPS = 16;
    static const float sGridValues[kNumGridValues];
    static const char* sGridTexts[kNumGridValues];

    double mSampleRate;
    float mBeatTime;          // Current beat time in seconds
    int mGrid;                // Current grid setting (0-7 index)

    // Per-tap settings
    bool mTapEnabled[NUM_TAPS];
    float mTapLevel[NUM_TAPS];
    float mTapPan[NUM_TAPS];

    // Calculated tap delay times
    float mTapDelayTimes[NUM_TAPS];
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
    // Helper methods
    void checkTapStateChangesAndClearBuffers();

    // Parameters
    float mInputGain;
    float mOutputGain;
    float mDelayTime;
    float mDryWet;
    float mFeedback;
    bool mTempoSyncMode;
    int mSyncDivision;
    int mGrid;

    // Per-tap parameters
    bool mTapEnabled[16];
    bool mTapEnabledPrevious[16];  // Track previous state for buffer clearing
    float mTapLevel[16];
    float mTapPan[16];

    // Per-tap filter parameters
    float mTapFilterCutoff[16];
    float mTapFilterResonance[16];
    int mTapFilterType[16];

    // Fade-out state for smooth tap disengagement
    bool mTapFadingOut[16];        // True when tap is fading out
    int mTapFadeOutRemaining[16];  // Samples remaining in fade-out
    int mTapFadeOutTotalLength[16]; // Total fade-out length for calculation

    // Fade-in state for smooth tap engagement
    bool mTapFadingIn[16];         // True when tap is fading in
    int mTapFadeInRemaining[16];   // Samples remaining in fade-in
    int mTapFadeInTotalLength[16]; // Total fade-in length for calculation

    float mTapFadeGain[16];        // Current fade gain (0.0 to 1.0)

    // DSP
    DualDelayLine mDelayLineL;  // Keep original for legacy
    DualDelayLine mDelayLineR;  // Keep original for legacy

    // Multi-tap delay lines (16 taps, stereo)
    static const int NUM_TAPS = 16;
    DualDelayLine mTapDelayLinesL[NUM_TAPS];  // Left channel delay lines
    DualDelayLine mTapDelayLinesR[NUM_TAPS];  // Right channel delay lines

    TempoSync mTempoSync;
    TapDistribution mTapDistribution;
    double mSampleRate;

    // Per-tap filters (16 taps, stereo)
    ThreeSistersFilter mTapFiltersL[NUM_TAPS];  // Left channel filters
    ThreeSistersFilter mTapFiltersR[NUM_TAPS];  // Right channel filters

    // Feedback storage for tanh limiting
    float mFeedbackBufferL;
    float mFeedbackBufferR;

    // Comb processor
    CombProcessor mCombProcessor;

    void updateParameters();
};

} // namespace WaterStick