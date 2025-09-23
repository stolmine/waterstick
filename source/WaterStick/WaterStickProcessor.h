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

protected:
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

class PitchShiftingDelayLine : public DualDelayLine {
public:
    PitchShiftingDelayLine();
    ~PitchShiftingDelayLine();

    void initialize(double sampleRate, double maxDelaySeconds);
    void processSample(float input, float& output);
    void reset();

    void setPitchShift(int semitones);
    bool isPitchShiftActive() const { return mPitchSemitones != 0; }

private:
    struct Grain {
        bool active;
        float position;         // Current read position in grain (floating-point for precision)
        float phase;           // 0.0 to 1.0 for windowing
        float pitchRatio;      // Playback speed ratio for this grain
        float grainStart;      // Start position in delay buffer (floating-point)
        float readPosition;    // Current floating-point read position in buffer
    };

    static const int NUM_GRAINS = 6;    // Increased for better upward pitch shifting
    static const int GRAIN_SIZE = 2048;  // ~46ms at 44.1kHz
    static const int GRAIN_OVERLAP = GRAIN_SIZE * 3 / 4;  // 75% overlap
    static const int LOOKAHEAD_BUFFER = GRAIN_SIZE * 2;  // Extra buffer for upward shifts

    Grain mGrains[NUM_GRAINS];
    int mPitchSemitones;      // -12 to +12
    float mPitchRatio;        // Calculated from semitones
    float mNextGrainSample;   // When to start next grain (floating-point)
    float mSampleCount;       // Running sample counter (floating-point for precision)
    float mAdaptiveSpacing;   // Dynamically calculated grain spacing

    // Pitch shifting buffers (separate from delay buffers)
    std::vector<float> mPitchBufferA;
    std::vector<float> mPitchBufferB;

    void initializeGrains();
    void updateGrains();
    void startNewGrain();
    float calculateHannWindow(float phase);
    float processPitchShifting(float input);
    void updatePitchRatio();
    void calculateAdaptiveSpacing();
    float interpolateBuffer(const std::vector<float>& buffer, float position) const;
    bool isBufferPositionValid(const std::vector<float>& buffer, float position) const;
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

    // Friend declarations for helper classes
    friend struct TapParameterProcessor;

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
    // State version constants
    static constexpr Steinberg::int32 kStateVersionLegacy = 0;  // Legacy unversioned state
    static constexpr Steinberg::int32 kStateVersionCurrent = 1; // First versioned release

    // State signature for freshness detection (magic number)
    static constexpr Steinberg::int32 kStateMagicNumber = 0x57415453; // "WATS" in hex

    void checkTapStateChangesAndClearBuffers();
    void checkBypassStateChanges();
    void processDelaySection(float inputL, float inputR, float& outputL, float& outputR);

    // State versioning methods
    Steinberg::tresult readLegacyProcessorState(Steinberg::IBStream* state);
    Steinberg::tresult readVersionedProcessorState(Steinberg::IBStream* state, Steinberg::int32 version);
    Steinberg::tresult readCurrentVersionProcessorState(Steinberg::IBStream* state);

    // Parameters
    float mInputGain;
    float mOutputGain;
    float mDelayTime;
    float mFeedback;
    bool mTempoSyncMode;
    int mSyncDivision;
    int mGrid;

    float mGlobalDryWet;
    bool mDelayBypass;

    bool mDelayBypassPrevious;
    bool mDelayFadingOut;
    bool mDelayFadingIn;
    int mDelayFadeRemaining;
    int mDelayFadeTotalLength;
    float mDelayFadeGain;

    // Per-tap parameters
    bool mTapEnabled[16];
    bool mTapEnabledPrevious[16];  // Track previous state for buffer clearing
    float mTapLevel[16];
    float mTapPan[16];

    // Per-tap filter parameters
    float mTapFilterCutoff[16];
    float mTapFilterResonance[16];
    int mTapFilterType[16];

    // Per-tap pitch shift parameters
    int mTapPitchShift[16];

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
    PitchShiftingDelayLine mTapDelayLinesL[NUM_TAPS];  // Left channel delay lines
    PitchShiftingDelayLine mTapDelayLinesR[NUM_TAPS];  // Right channel delay lines

    TempoSync mTempoSync;
    TapDistribution mTapDistribution;
    double mSampleRate;

    // Per-tap filters (16 taps, stereo)
    ThreeSistersFilter mTapFiltersL[NUM_TAPS];  // Left channel filters
    ThreeSistersFilter mTapFiltersR[NUM_TAPS];  // Right channel filters

    // Feedback storage for tanh limiting
    float mFeedbackBufferL;
    float mFeedbackBufferR;


    // Parameter change tracking for tempo sync optimization
    float mLastTempoSyncDelayTime;
    bool mTempoSyncParametersChanged;

    // Parameter capture for delay propagation
    struct ParameterSnapshot {
        float level;
        float pan;
        float filterCutoff;
        float filterResonance;
        int filterType;
        int pitchShift;
        bool enabled;
    };

    // Store parameter snapshots for each tap (circular buffer approach)
    static const int PARAM_HISTORY_SIZE = 8192;  // Enough for ~185ms at 44.1kHz
    ParameterSnapshot mTapParameterHistory[16][PARAM_HISTORY_SIZE];
    int mParameterHistoryWriteIndex;

    void captureCurrentParameters();
    ParameterSnapshot getHistoricParameters(int tapIndex, float delayTimeSeconds) const;

    void updateParameters();
    void checkTempoSyncParameterChanges();
};

} // namespace WaterStick