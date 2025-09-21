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

// Routing modes for signal path control
enum RouteMode {
    DelayToComb = 0,    // Delay section feeds into Comb section
    CombToDelay,        // Comb section feeds into Delay section
    DelayPlusComb       // Parallel processing: both sections process input independently
};

class RoutingManager {
public:
    RoutingManager();
    ~RoutingManager();

    void initialize(double sampleRate);
    void setRouteMode(RouteMode mode);
    RouteMode getRouteMode() const { return mRouteMode; }

    // Signal path validation
    bool isValidRouting() const;

    // State management for routing transitions
    void processRouteTransition();
    bool isTransitionInProgress() const { return mTransitionInProgress; }

    // Get text representation for display
    const char* getRouteModeText() const;

    void reset();

private:
    RouteMode mRouteMode;
    RouteMode mPendingRouteMode;
    bool mTransitionInProgress;
    double mSampleRate;

    // Transition state management
    int mTransitionSamples;
    int mTransitionCounter;

    // Text lookup for route modes
    static const char* sRouteModeTexts[3];

    void startTransition(RouteMode newMode);
    void completeTransition();
};

class CombProcessor {
public:
    // Fade mode enumeration for parameter transitions
    enum FadeMode {
        FADE_MODE_AUTO = 0,     // Adaptive system (default)
        FADE_MODE_FIXED,        // User-defined fixed time
        FADE_MODE_INSTANT       // Minimal delay for immediate response
    };

    // Tap position structure for interpolated mapping
    struct TapPosition {
        float currentPos;     // Current interpolated position
        float targetPos;      // Target position for new tap count
        float previousPos;    // Position from old tap count

        TapPosition()
            : currentPos(0.0f)
            , targetPos(0.0f)
            , previousPos(0.0f)
        {
        }
    };

    // Tap fade state structure
    struct TapFadeState {
        enum FadeType {
            FADE_NONE = 0,
            FADE_IN,
            FADE_OUT,
            CROSSFADE
        };

        FadeType fadeType;
        float fadePosition;         // 0.0 to 1.0 fade progress
        float fadeDuration;         // Duration in samples
        float fadeStartTime;        // Sample when fade started
        int targetTapCount;         // Target number of taps
        int previousTapCount;       // Previous number of taps
        bool isActive;              // Whether fade is currently active

        TapFadeState()
            : fadeType(FADE_NONE)
            , fadePosition(0.0f)
            , fadeDuration(0.0f)
            , fadeStartTime(0.0f)
            , targetTapCount(0)
            , previousTapCount(0)
            , isActive(false)
        {
        }
    };

    CombProcessor();
    ~CombProcessor();

    void initialize(double sampleRate, double maxDelaySeconds = 2.0);
    void setSize(float sizeSeconds);  // Comb size (delay time of tap 64)
    void setNumTaps(int numTaps);     // 1-64 active taps
    void setFeedback(float feedback); // 0-1 feedback amount
    void setSyncMode(bool synced);
    void setClockDivision(int division);
    void setPitchCV(float cv);        // 1V/oct control
    void setPattern(int pattern);     // 0-15 tap spacing patterns
    void setSlope(int slope);         // 0-3 envelope slopes
    void setGain(float gain);         // Linear gain multiplier
    void setFadeTime(float fadeTimeMs); // Set user-controlled fade time in milliseconds
    void updateTempo(double hostTempo, bool isValid); // Update tempo for sync calculations

    void processStereo(float inputL, float inputR, float& outputL, float& outputR);
    void reset();

    // Get current size for display
    float getCurrentSize() const { return mCombSize; }
    float getSyncedCombSize() const;  // Get sync-adjusted comb size

    // Tap fade management for smooth transitions
    void startTapCountFade(int newTapCount);
    void updateFadeState();
    float calculateFadeGain(float fadePosition, TapFadeState::FadeType fadeType) const;
    void processFadedOutput(float& outputL, float& outputR);
    float calculateFadeDurationSamples() const;

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
    bool mHostTempoValid;

    // Pattern and slope parameters
    int mPattern;           // 0-15 tap spacing pattern
    int mSlope;             // 0-3 envelope slope
    float mGain;            // Linear gain multiplier

    // Fade control parameters
    FadeMode mFadeMode;     // Current fade mode (auto/fixed/instant)
    float mUserFadeTime;    // User-defined fade time in milliseconds (for fixed mode)

    // Tap fade state management
    TapFadeState mFadeState;                    // Current fade state
    float mSampleCounter;                       // Sample counter for fade timing
    std::vector<float> mPreviousOutputL;        // Previous output buffer for crossfade (left)
    std::vector<float> mPreviousOutputR;        // Previous output buffer for crossfade (right)

    // Tap position interpolation
    std::vector<TapPosition> mTapPositions;     // Interpolated tap positions for smooth mapping

    // Calculate tap delays based on comb size
    float getTapDelay(int tapIndex) const;
    float getTapGain(int tapIndex) const;
    float applyCVScaling(float baseDelay) const;
    float tanhLimiter(float input) const;

    // Tap position interpolation methods
    void updateTapPositions();
    float getInterpolatedTapPosition(int tap) const;
    float getTapDelayFromFloat(float tapPosition) const;
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

    // Helper methods
    void checkTapStateChangesAndClearBuffers();
    void checkBypassStateChanges();
    void processDelaySection(float inputL, float inputR, float dryRefL, float dryRefR, float& outputL, float& outputR, RouteMode routeMode);
    void processCombSection(float inputL, float inputR, float dryRefL, float dryRefR, float& outputL, float& outputR, RouteMode routeMode);

    // Serial-aware processing helpers
    enum ProcessingMode { SERIAL_MODE, PARALLEL_MODE };
    ProcessingMode getProcessingMode(RouteMode routeMode) const;
    bool hasAnyTapsEnabled() const;

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
    float mDelayGain;

    // Routing and Wet/Dry controls
    RouteMode mRouteMode;
    float mGlobalDryWet;      // Global Dry/Wet mix (affects final output)
    float mDelayDryWet;       // Delay section Dry/Wet mix (delay section only)
    float mCombDryWet;        // Comb section Dry/Wet mix (comb section only)
    bool mDelayBypass;        // Delay section bypass toggle
    bool mCombBypass;         // Comb section bypass toggle

    // Comb control parameters
    float mCombSize;          // Comb delay size in seconds
    float mCombFeedback;      // Comb feedback amount
    float mCombPitchCV;       // Comb pitch CV in volts
    int mCombTaps;            // Number of active comb taps
    bool mCombSync;           // Comb sync mode
    int mCombDivision;        // Comb sync division
    int mCombPattern;         // Comb tap spacing pattern (0-15)
    int mCombSlope;           // Comb envelope slope (0-3)
    float mCombGain;          // Comb section gain (linear multiplier)

    // Bypass fade system state
    bool mDelayBypassPrevious;     // Track previous state for fade triggering
    bool mCombBypassPrevious;      // Track previous state for fade triggering
    bool mDelayFadingOut;          // True when delay section is fading out
    bool mDelayFadingIn;           // True when delay section is fading in
    bool mCombFadingOut;           // True when comb section is fading out
    bool mCombFadingIn;            // True when comb section is fading in
    int mDelayFadeRemaining;       // Samples remaining in delay fade
    int mDelayFadeTotalLength;     // Total delay fade length for calculation
    int mCombFadeRemaining;        // Samples remaining in comb fade
    int mCombFadeTotalLength;      // Total comb fade length for calculation
    float mDelayFadeGain;          // Current delay fade gain (0.0 to 1.0)
    float mCombFadeGain;           // Current comb fade gain (0.0 to 1.0)

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

    // Routing manager
    RoutingManager mRoutingManager;

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