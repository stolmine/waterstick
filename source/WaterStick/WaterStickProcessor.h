#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "WaterStickParameters.h"
#include "ThreeSistersFilter.h"
#include "DecoupledDelayArchitecture.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <string>
#include <atomic>
#include <memory>

// Debug logging system for pitch shifting dropout investigation
#ifdef DEBUG
#define PITCH_DEBUG_LOGGING 1
#else
#define PITCH_DEBUG_LOGGING 0
#endif

namespace PitchDebug {
    void logMessage(const std::string& message);
    void enableLogging(bool enable);
    bool isLoggingEnabled();
}

// Performance profiler for dropout investigation
class PerformanceProfiler {
public:
    struct ProfileData {
        std::chrono::high_resolution_clock::time_point startTime;
        std::chrono::high_resolution_clock::time_point endTime;
        double durationUs;
        std::string operation;
        int tapIndex;
        int parameterType;
    };

    static PerformanceProfiler& getInstance();

    void startProfiling(const std::string& operation, int tapIndex = -1, int parameterType = -1);
    void endProfiling();
    void logPerformanceReport();
    void clearProfile();
    void enableProfiling(bool enable);
    bool isProfilingEnabled() const;

    // Dropout detection
    double getMaxParameterUpdateTime() const;
    double getAverageParameterUpdateTime() const;
    int getTimeoutCount() const;

private:
    PerformanceProfiler() : mProfilingEnabled(false), mCurrentProfile(nullptr), mTimeoutThresholdUs(1000.0) {}

    bool mProfilingEnabled;
    std::vector<ProfileData> mProfileHistory;
    ProfileData* mCurrentProfile;
    double mTimeoutThresholdUs;
    int mTimeoutCount = 0;

    static const size_t MAX_PROFILE_HISTORY = 10000;
};

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

// PHASE 3: PERFORMANCE OPTIMIZATION - Lock-Free & SIMD-Accelerated Parameter System

// Memory Pool for parameter update objects to avoid real-time allocations
template<typename T, size_t PoolSize = 64>
class LockFreeMemoryPool {
public:
    LockFreeMemoryPool() : mFreeList(nullptr), mHead(0) {
        for (size_t i = 0; i < PoolSize; ++i) {
            mPool[i].next = &mPool[i + 1];
        }
        mPool[PoolSize - 1].next = nullptr;
        mFreeList.store(&mPool[0], std::memory_order_relaxed);
    }

    T* acquire() {
        Node* node = mFreeList.load(std::memory_order_acquire);
        while (node != nullptr) {
            if (mFreeList.compare_exchange_weak(node, node->next, std::memory_order_release, std::memory_order_acquire)) {
                return &node->data;
            }
        }
        return nullptr; // Pool exhausted
    }

    void release(T* obj) {
        Node* node = reinterpret_cast<Node*>(reinterpret_cast<char*>(obj) - offsetof(Node, data));
        Node* oldHead = mFreeList.load(std::memory_order_relaxed);
        do {
            node->next = oldHead;
        } while (!mFreeList.compare_exchange_weak(oldHead, node, std::memory_order_release, std::memory_order_relaxed));
    }

private:
    struct Node {
        T data;
        std::atomic<Node*> next;
        Node() : next(nullptr) {}
    };

    alignas(64) Node mPool[PoolSize];
    std::atomic<Node*> mFreeList;
    std::atomic<size_t> mHead;
};

// Cache-aligned parameter update structure for SIMD processing
struct alignas(32) MacroParameterBatch {
    float values[8];        // 8 macro knob values
    float curves[8];        // 8 curve evaluations
    float targets[16];      // 16 tap parameter targets
    uint32_t dirtyFlags;    // Bit flags for changed parameters
    uint64_t version;       // Parameter version for consistency
};

// SIMD-accelerated macro curve evaluator
class SIMDMacroCurveEvaluator {
public:
    SIMDMacroCurveEvaluator();
    ~SIMDMacroCurveEvaluator();

    void evaluateBatch(const float* inputs, float* outputs, int curveType, int count);
    void evaluateAllCurves(MacroParameterBatch& batch);

private:
    // SIMD curve evaluation functions
    void evaluateLinearSIMD(const float* inputs, float* outputs, int count);
    void evaluateRampUpSIMD(const float* inputs, float* outputs, int count);
    void evaluateRampDownSIMD(const float* inputs, float* outputs, int count);
    void evaluateSCurveSIMD(const float* inputs, float* outputs, int count);
    void evaluateExpUpSIMD(const float* inputs, float* outputs, int count);
    void evaluateExpDownSIMD(const float* inputs, float* outputs, int count);

    // Platform-specific SIMD implementations
    #if defined(__SSE4_1__) && (defined(__x86_64__) || defined(__i386__))
    void evaluateSSE(const float* inputs, float* outputs, int curveType, int count);
    #endif
    #ifdef __ARM_NEON__
    void evaluateNEON(const float* inputs, float* outputs, int curveType, int count);
    #endif

    alignas(32) float mScratchBuffer[32];
};

// Cache-optimized parameter lookup with dirty flag tracking
class CacheOptimizedParameterLookup {
public:
    CacheOptimizedParameterLookup();
    ~CacheOptimizedParameterLookup();

    void initialize(int numTaps);
    void markDirty(int tapIndex, int parameterType);
    bool isDirty(int tapIndex, int parameterType) const;
    void clearDirty(int tapIndex, int parameterType);
    void clearAllDirty();

    // Cache-friendly parameter access
    float getTapParameter(int tapIndex, int parameterType) const;
    void setTapParameter(int tapIndex, int parameterType, float value);

    // Batch operations for better cache utilization
    void getTapParametersBatch(int tapIndex, float* outParams, int count) const;
    void setTapParametersBatch(int tapIndex, const float* params, int count);

    // Version access for optimization
    uint64_t getGlobalVersion() const;

private:
    static constexpr int CACHE_LINE_SIZE = 64;
    static constexpr int PARAMS_PER_TAP = 8; // Level, Pan, FilterCutoff, FilterResonance, FilterType, PitchShift, FeedbackSend, Enable

    // Cache-aligned parameter storage (one cache line per tap)
    struct alignas(CACHE_LINE_SIZE) TapParameterCache {
        float parameters[PARAMS_PER_TAP];
        uint32_t dirtyFlags;
        uint64_t version;
        char padding[CACHE_LINE_SIZE - sizeof(float) * PARAMS_PER_TAP - sizeof(uint32_t) - sizeof(uint64_t)];
    };

    std::vector<TapParameterCache> mTapCache;
    std::atomic<uint64_t> mGlobalVersion{0};
    int mNumTaps;
};

// Enhanced Lock-Free Parameter Manager with SIMD and caching
class LockFreeParameterManager {
public:
    struct ParameterState {
        std::atomic<float> pitchRatio{1.0f};
        std::atomic<float> delayTime{0.0f};
        std::atomic<int> pitchSemitones{0};
        std::atomic<uint64_t> version{0};

        void copyFrom(const ParameterState& other) {
            pitchRatio.store(other.pitchRatio.load(std::memory_order_acquire), std::memory_order_release);
            delayTime.store(other.delayTime.load(std::memory_order_acquire), std::memory_order_release);
            pitchSemitones.store(other.pitchSemitones.load(std::memory_order_acquire), std::memory_order_release);
            version.store(other.version.load(std::memory_order_acquire), std::memory_order_release);
        }
    };

    LockFreeParameterManager();
    ~LockFreeParameterManager();

    void initialize();
    void updatePitchShift(int semitones);
    void updateDelayTime(float delayTimeSeconds);

    void getAudioThreadParameters(ParameterState& outState);
    bool hasParametersChanged(uint64_t lastVersion) const;

    // PHASE 3: Advanced parameter management
    void updateMacroParametersBatch(const MacroParameterBatch& batch);
    bool getMacroParametersBatch(MacroParameterBatch& outBatch);

private:
    static constexpr int NUM_BUFFERS = 3;
    ParameterState mParameterBuffers[NUM_BUFFERS];
    std::atomic<int> mWriteIndex{0};
    std::atomic<int> mReadIndex{0};
    std::atomic<uint64_t> mGlobalVersion{0};

    // PHASE 3: Macro parameter triple buffering
    MacroParameterBatch mMacroBatches[NUM_BUFFERS];
    std::atomic<int> mMacroWriteIndex{0};
    std::atomic<int> mMacroReadIndex{0};

    void calculatePitchRatio(ParameterState& state);
    int getNextIndex(int currentIndex) const;
};

// Recovery Manager for bulletproof error recovery
class RecoveryManager {
public:
    enum RecoveryLevel {
        NONE = 0,
        POSITION_CORRECTION = 1,
        BUFFER_RESET = 2,
        EMERGENCY_BYPASS = 3
    };

    RecoveryManager();
    ~RecoveryManager();

    void initialize(double sampleRate);
    void startProcessingTimer();
    RecoveryLevel checkAndHandleTimeout();
    void reportSuccess();
    void reset();

    bool isInEmergencyBypass() const;
    void clearEmergencyBypass();

    // Statistics
    int getTimeoutCount() const;
    int getRecoveryCount(RecoveryLevel level) const;
    double getMaxProcessingTime() const;

private:
    std::chrono::high_resolution_clock::time_point mProcessingStartTime;
    std::atomic<bool> mEmergencyBypass{false};
    std::atomic<int> mTimeoutCount{0};
    std::atomic<int> mRecoveryCount[4]{0, 0, 0, 0};
    std::atomic<double> mMaxProcessingTime{0.0};

    double mSampleRate;

    static constexpr double LEVEL1_TIMEOUT_US = 50.0;    // 50μs - Position correction
    static constexpr double LEVEL2_TIMEOUT_US = 100.0;   // 100μs - Buffer reset
    static constexpr double LEVEL3_TIMEOUT_US = 200.0;   // 200μs - Emergency bypass
    static constexpr int MAX_CONSECUTIVE_TIMEOUTS = 3;

    mutable int mConsecutiveTimeouts = 0;
};

// Unified Pitch Delay Line - single robust buffer system
class UnifiedPitchDelayLine {
public:
    UnifiedPitchDelayLine();
    ~UnifiedPitchDelayLine();

    void initialize(double sampleRate, double maxDelaySeconds);
    void processSample(float input, float& output);
    void reset();

    void setPitchShift(int semitones);
    void setDelayTime(float delayTimeSeconds);
    bool isPitchShiftActive() const;

    // Recovery and diagnostics
    RecoveryManager::RecoveryLevel getLastRecoveryLevel() const;
    void logProcessingStats() const;

private:
    // Core buffer system - single unified buffer
    std::vector<float> mBuffer;
    int mBufferSize;
    std::atomic<int> mWriteIndex{0};
    std::atomic<float> mReadPosition{0.0f};
    double mSampleRate;

    // Safety margins and bounds
    int mSafetyMargin;         // Minimum safe distance between read/write
    int mMaxReadAdvance;       // Maximum samples to advance per cycle
    float mMinReadPosition;    // Minimum allowed read position
    float mMaxReadPosition;    // Maximum allowed read position

    // Lock-free parameter management
    std::unique_ptr<LockFreeParameterManager> mParameterManager;
    uint64_t mLastParameterVersion = 0;

    // Recovery system
    std::unique_ptr<RecoveryManager> mRecoveryManager;

    // Current processing state
    struct ProcessingState {
        float currentPitchRatio = 1.0f;
        float targetPitchRatio = 1.0f;
        float currentDelayTime = 0.0f;
        float smoothingCoeff = 1.0f;
        bool needsReset = false;
    };
    ProcessingState mState;

    // Processing methods
    void updateParameters();
    void updateSmoothingCoeff();
    void applySmoothingAndValidation();
    float interpolateBuffer(float position) const;
    bool validateReadPosition() const;
    void correctReadPosition();
    void performBufferReset();

    // Deterministic processing bounds
    static constexpr int MAX_PROCESSING_CYCLES = 10;
    static constexpr float SMOOTHING_TIME_CONSTANT_MS = 5.0f;
    static constexpr float PITCH_RATIO_MIN = 0.25f;
    static constexpr float PITCH_RATIO_MAX = 4.0f;
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

// LEGACY SYSTEM: Kept for emergency fallback only
// Production system now uses UnifiedPitchDelayLine (47.9x performance improvement)
class SpeedBasedDelayLine : public DualDelayLine {
public:
    SpeedBasedDelayLine();
    ~SpeedBasedDelayLine();

    void initialize(double sampleRate, double maxDelaySeconds);
    void processSample(float input, float& output);
    void reset();

    void setPitchShift(int semitones);
    bool isPitchShiftActive() const { return mPitchSemitones != 0; }

private:
    // Speed-based pitch shifting parameters
    int mPitchSemitones;         // -12 to +12 semitones
    float mPitchRatio;           // Current smoothed pitch ratio (speed)
    float mTargetPitchRatio;     // Target pitch ratio for smoothing
    float mSmoothingCoeff;       // Parameter smoothing coefficient (15ms)

    // Variable speed playback state
    float mReadPosition;         // Floating-point read position in buffer

    // Enhanced buffers for variable speed playback
    std::vector<float> mSpeedBufferA;  // Enhanced buffer A for pitch shifting
    std::vector<float> mSpeedBufferB;  // Enhanced buffer B for pitch shifting
    int mSpeedBufferSize;        // Size of speed buffers (4x original for safety)

    // Separate write indices for speed buffers
    int mSpeedWriteIndexA;       // Write index for speed buffer A
    int mSpeedWriteIndexB;       // Write index for speed buffer B
    int mMinReadDistance;        // Minimum safe read distance (prevents underruns)

    // Buffer management
    static const int BUFFER_MULTIPLIER = 4;  // 4x buffer size for extreme pitch down protection

    // Safety and diagnostics for dropout investigation
    static const int MAX_LOOP_ITERATIONS = 1000;  // Maximum iterations for while loops
    static const int TIMEOUT_THRESHOLD_MS = 1;     // Timeout threshold (1ms)
    mutable bool mEmergencyBypassMode;             // Emergency bypass when processing gets stuck
    mutable int mProcessingTimeouts;               // Count of processing timeouts
    mutable int mInfiniteLoopPrevention;           // Count of infinite loop preventions

    // High-resolution timing for dropout detection
    mutable std::chrono::high_resolution_clock::time_point mProcessingStartTime;

    // Core speed-based processing methods
    void updatePitchRatio();
    void updateSmoothingCoeff();
    void updatePitchRatioSmoothing();
    float interpolateBuffer(const std::vector<float>& buffer, float position) const;
    float processSpeedBasedPitchShifting(float input);

    // Safety and diagnostic methods
    void startProcessingTimer() const;
    bool checkProcessingTimeout() const;
    void enterEmergencyBypass(const std::string& reason) const;

public:
    // Public diagnostic methods
    void logProcessingStats() const;
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

    // ENHANCED FEEDBACK SYSTEM METHODS
    // ================================
    void initializeFeedbackSystem();
    void updateFeedbackDampingCoefficients();
    void processFeedbackDamping(float& feedbackL, float& feedbackR);
    void applyFeedbackPolarity(float& feedbackL, float& feedbackR);
    float calculateDampingCoefficient(float cutoffNormalized) const;
    float mapCutoffFrequency(float normalized) const;  // Maps 0.0-1.0 to 20Hz-20kHz

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

    // Discrete control parameters (24 parameters with real-time smoothing)
    float mDiscreteParameters[24];
    float mDiscreteParametersSmoothed[24];
    float mSmoothingCoeff;

    // Macro curve parameters
    int mMacroCurveTypes[4];

    // Macro knob values (8 macro knobs)
    float mMacroKnobValues[8];

    // Macro system state tracking for conditional evaluation
    bool mMacroSystemActive[8];           // Track which macro knobs are actively controlling parameters
    bool mMacroInfluenceActive;           // Global flag to control macro system activation
    bool mParameterModifiedByUser[1024];  // Track which parameters have been manually modified
    float mPreviousMacroKnobValues[8];    // Track previous macro knob values for change detection

    // PHASE 3: Performance optimization components
    std::unique_ptr<SIMDMacroCurveEvaluator> mSIMDCurveEvaluator;
    std::unique_ptr<CacheOptimizedParameterLookup> mParameterCache;
    std::unique_ptr<LockFreeMemoryPool<MacroParameterBatch>> mParameterUpdatePool;

    // Lock-free macro parameter state
    MacroParameterBatch mCurrentMacroBatch;
    std::atomic<uint64_t> mMacroParameterVersion{0};
    std::atomic<bool> mMacroParametersNeedUpdate{false};

    // Parameter change detection with dirty flags
    std::atomic<uint32_t> mTapParameterDirtyFlags[16];
    std::atomic<uint64_t> mLastParameterUpdateVersion{0};

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

    // Per-tap feedback send parameters
    float mTapFeedbackSend[16];

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
    SpeedBasedDelayLine mTapDelayLinesL[NUM_TAPS];  // LEGACY: Emergency fallback only
    SpeedBasedDelayLine mTapDelayLinesR[NUM_TAPS];  // LEGACY: Emergency fallback only

    // Phase 4: PRODUCTION unified delay lines (47.9x performance improvement)
    UnifiedPitchDelayLine mUnifiedTapDelayLinesL[NUM_TAPS];  // Left channel unified delay lines (LEGACY)
    UnifiedPitchDelayLine mUnifiedTapDelayLinesR[NUM_TAPS];  // Right channel unified delay lines (LEGACY)
    bool mUseUnifiedDelayLines;                              // Flag to switch between old/new implementations

    // Phase 5: DECOUPLED DELAY + PITCH ARCHITECTURE (Production-ready solution)
    DecoupledDelaySystem mDecoupledDelaySystemL;             // Left channel decoupled system (PRIMARY)
    DecoupledDelaySystem mDecoupledDelaySystemR;             // Right channel decoupled system (PRIMARY)
    bool mUseDecoupledArchitecture;                          // Feature flag for decoupled system

    TempoSync mTempoSync;
    TapDistribution mTapDistribution;
    double mSampleRate;

    // Per-tap filters (16 taps, stereo)
    ThreeSistersFilter mTapFiltersL[NUM_TAPS];  // Left channel filters
    ThreeSistersFilter mTapFiltersR[NUM_TAPS];  // Right channel filters

    // ENHANCED FEEDBACK SYSTEM ARCHITECTURE
    // =================================

    // Core feedback storage for tanh limiting
    float mFeedbackBufferL;
    float mFeedbackBufferR;

    // Feedback sub-mixer for per-tap sends
    float mFeedbackSubMixerL;
    float mFeedbackSubMixerR;

    // Pre-effects feedback capture (before filters and pitch processing)
    float mFeedbackSubMixerPreEffectsL;
    float mFeedbackSubMixerPreEffectsR;

    // PROFESSIONAL FEEDBACK ENHANCEMENT PARAMETERS
    // ============================================

    // High-frequency damping parameters
    float mFeedbackDamping;              // Amount of high-frequency damping (0.0-1.0)
    float mFeedbackDampingCutoff;        // Damping filter cutoff frequency (0.0-1.0, mapped to 20Hz-20kHz)

    // Feedback routing parameters
    bool mFeedbackPreEffects;            // true = pre-effects, false = post-effects
    bool mFeedbackPolarityInvert;        // true = inverted polarity, false = normal

    // High-frequency damping filter state (one-pole lowpass for each channel)
    // Using y[n] = a * x[n] + b * y[n-1] form for efficiency
    float mDampingFilterStateL;          // Previous output sample (left)
    float mDampingFilterStateR;          // Previous output sample (right)
    float mDampingFilterCoeff;           // Filter coefficient (calculated from cutoff)
    float mDampingFilterGain;            // Input gain to maintain unity at DC

    // Smooth parameter changes to prevent audio artifacts
    float mDampingCoeffSmoothed;         // Smoothed damping coefficient
    float mDampingCoeffTarget;           // Target damping coefficient


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
        float feedbackSend;
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

    // Real-time curve evaluation and parameter smoothing
    void updateDiscreteParameters();
    void applyCurveEvaluation();
    void applyParameterSmoothing();
    float evaluateMacroCurve(int curveType, float input) const;

    // PHASE 3: Optimized parameter update methods
    void updateParametersOptimized();
    void updateMacroParametersBatch();
    void processMacroParameterUpdate(const MacroParameterBatch& batch);
    void applyParameterCacheUpdates();
    bool checkParameterChangesOptimized();

    // Processor-side macro curve evaluation system
    void applyMacroCurvesToTapParameters();
    float getGlobalCurveValueForTap(int discretePosition, int tapIndex) const;
    float getGlobalCurveValueForTapContinuous(float continuousValue, int tapIndex) const;
    void applyMacroKnobToTapParameter(int macroKnobIndex, int tapContext);
    void applyMacroKnobToAllTaps(int macroKnobIndex, int tapContext);

    // Rainmaker curve evaluation (matching controller implementation)
    float evaluateRampUp(float x) const;
    float evaluateRampDown(float x) const;
    float evaluateSigmoidSCurve(float x) const;
    float evaluateInverseSigmoid(float x) const;
    float evaluateExpUp(float x) const;
    float evaluateExpDown(float x) const;

    // Current tap context for macro curve application (matches controller state)
    int mCurrentTapContext;

    // Parameter source priority methods
    void detectUserParameterChanges();
    void markParameterAsUserModified(int paramId);
    void clearMacroInfluenceForParameter(int paramId);
    void enableMacroControlForParameter(int paramId);
    bool isParameterUnderMacroInfluence(int paramId) const;

public:
    // Access discrete parameters with smoothing
    float getSmoothedDiscreteParameter(int index) const;
    float getRawDiscreteParameter(int index) const;

    // Debug and diagnostics methods for dropout investigation
    void enablePitchDebugLogging(bool enable);
    void logPitchProcessingStats() const;

    // Performance profiling methods for dropout investigation
    void enablePerformanceProfiling(bool enable);
    void logPerformanceReport() const;
    void clearPerformanceProfile();

    // Phase 2: Unified delay line system control for A/B testing
    void enableUnifiedDelayLines(bool enable);
    bool isUsingUnifiedDelayLines() const;
    void logUnifiedDelayLineStats() const;

    // Phase 5: Decoupled delay + pitch architecture control
    void enableDecoupledDelayLines(bool enable);
    bool isUsingDecoupledDelayLines() const;
    void enablePitchProcessing(bool enable);
    void logDecoupledSystemHealth() const;
    bool isDecoupledSystemHealthy() const;
};

} // namespace WaterStick