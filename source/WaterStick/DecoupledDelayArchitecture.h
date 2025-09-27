#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <array>

namespace WaterStick {

// ===================================================================
// PHASE 5: FULLY DECOUPLED DELAY + PITCH ARCHITECTURE
// ===================================================================
//
// DESIGN PRINCIPLES:
// 1. Complete separation: Delay processing works independently of pitch
// 2. Optional pitch: Pitch is a post-process that never affects delay
// 3. Unified coordination: Single coordinator manages all pitch resources
// 4. Graceful degradation: Delay always works, pitch fails safely
// 5. Resource isolation: No competition between delay and pitch systems

// ===================================================================
// 1. PURE DELAY SYSTEM (Always works, never affected by pitch)
// ===================================================================

class PureDelayLine {
public:
    PureDelayLine();
    ~PureDelayLine();

    void initialize(double sampleRate, double maxDelaySeconds);
    void setDelayTime(float delayTimeSeconds);
    void processSample(float input, float& output);
    void reset();

    // Pure delay has no pitch coupling whatsoever
    bool isInitialized() const { return mInitialized; }

private:
    // Dual-buffer system for zipper-free delay time changes
    std::vector<float> mBufferA;
    std::vector<float> mBufferB;
    int mBufferSize;
    int mWriteIndexA;
    int mWriteIndexB;
    double mSampleRate;
    bool mInitialized;

    // Active/Standby line management for crossfading
    bool mUsingLineA;
    enum CrossfadeState { STABLE, CROSSFADING };
    CrossfadeState mCrossfadeState;

    // Movement detection for smooth transitions
    float mTargetDelayTime;
    float mCurrentDelayTime;
    int mStabilityCounter;
    int mStabilityThreshold;

    // Crossfade control for zipper-free modulation
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

    // Core processing methods
    void updateDelayState(DelayLineState& state, float delayTime);
    void updateAllpassCoeff(DelayLineState& state);
    float processDelayLine(std::vector<float>& buffer, int& writeIndex, DelayLineState& state, float input);
    float nextOut(DelayLineState& state, const std::vector<float>& buffer);

    // Crossfading methods for smooth delay time changes
    void startCrossfade();
    void updateCrossfade();
    int calculateCrossfadeLength(float delayTime);
};

// ===================================================================
// 2. CENTRALIZED PITCH COORDINATOR (Manages all 16 taps)
// ===================================================================

class PitchCoordinator {
public:
    static constexpr int MAX_TAPS = 16;
    static constexpr int PITCH_BUFFER_SIZE = 8192;  // Dedicated pitch buffer per tap

    struct TapPitchState {
        int semitones = 0;
        float pitchRatio = 1.0f;
        bool enabled = false;
        bool needsReset = false;

        // Dedicated pitch processing buffer (separate from delay)
        std::array<float, PITCH_BUFFER_SIZE> pitchBuffer{};
        int pitchWriteIndex = 0;
        float pitchReadPosition = 0.0f;

        // Pitch-specific state
        float smoothingCoeff = 1.0f;
        float targetPitchRatio = 1.0f;
        float currentPitchRatio = 1.0f;
    };

    PitchCoordinator();
    ~PitchCoordinator();

    void initialize(double sampleRate);

    // Unified tap management - no resource competition
    void enableTap(int tapIndex, bool enable);
    void setPitchShift(int tapIndex, int semitones);

    // Coordinated processing - all taps processed together
    void processAllTaps(const float* delayOutputs, float* pitchOutputs);

    // System health monitoring
    bool isHealthy() const { return mSystemHealthy; }
    void getSystemStats(int& activeTaps, int& failedTaps, double& maxProcessingTime) const;

    void reset();

private:
    double mSampleRate;
    std::array<TapPitchState, MAX_TAPS> mTapStates;
    std::atomic<bool> mSystemHealthy{true};
    std::atomic<int> mActiveTaps{0};
    std::atomic<int> mFailedTaps{0};
    std::atomic<double> mMaxProcessingTime{0.0};

    // Unified resource pool
    static constexpr double PROCESSING_TIMEOUT_US = 100.0;  // 100μs total budget

    void processSingleTap(int tapIndex, float delayOutput, float& pitchOutput);
    void updateTapParameters(int tapIndex);
    void performTapRecovery(int tapIndex);
    bool validateTapState(int tapIndex) const;

    float interpolatePitchBuffer(int tapIndex, float position) const;
    void resetTapBuffer(int tapIndex);
};

// ===================================================================
// 3. DECOUPLED TAP PROCESSOR (Delay + Optional Pitch)
// ===================================================================

class DecoupledTapProcessor {
public:
    DecoupledTapProcessor();
    ~DecoupledTapProcessor();

    void initialize(double sampleRate, double maxDelaySeconds, int tapIndex);

    // Delay control (always works)
    void setDelayTime(float delayTimeSeconds);
    void setEnabled(bool enabled);

    // Pitch control (optional, never affects delay)
    void setPitchShift(int semitones);

    // Processing: Delay first, then optional pitch
    void processSample(float input, float& output);

    void reset();

    // Status monitoring
    bool isDelayHealthy() const { return mDelayHealthy; }
    bool isPitchHealthy() const { return mPitchHealthy; }
    bool isPitchActive() const { return mPitchEnabled && (mPitchSemitones != 0); }

private:
    int mTapIndex;
    bool mEnabled;
    bool mDelayHealthy;
    bool mPitchHealthy;
    bool mPitchEnabled;
    int mPitchSemitones;

    // Separate systems - no coupling
    std::unique_ptr<PureDelayLine> mDelayLine;  // Always works

    // Pitch processing happens at coordinator level
    float mLastDelayOutput;  // Cache for pitch coordinator

    friend class DecoupledDelaySystem;  // Allow system to access delay output
};

// ===================================================================
// 4. UNIFIED DECOUPLED SYSTEM (Complete architecture)
// ===================================================================

class DecoupledDelaySystem {
public:
    static constexpr int NUM_TAPS = 16;

    DecoupledDelaySystem();
    ~DecoupledDelaySystem();

    void initialize(double sampleRate, double maxDelaySeconds);

    // Per-tap control
    void setTapDelayTime(int tapIndex, float delayTimeSeconds);
    void setTapEnabled(int tapIndex, bool enabled);
    void setTapPitchShift(int tapIndex, int semitones);

    // Batch processing - delay first, then coordinated pitch
    void processAllTaps(float input, float* outputs);

    // System control
    void enablePitchProcessing(bool enable);
    bool isPitchProcessingEnabled() const { return mPitchProcessingEnabled; }

    void reset();

    // Health monitoring
    struct SystemHealth {
        bool delaySystemHealthy = true;
        bool pitchSystemHealthy = true;
        int activeTaps = 0;
        int failedPitchTaps = 0;
        double delayProcessingTime = 0.0;
        double pitchProcessingTime = 0.0;
        double totalProcessingTime = 0.0;
    };

    void getSystemHealth(SystemHealth& health) const;

private:
    double mSampleRate;
    bool mPitchProcessingEnabled;

    // Completely separate systems
    std::array<DecoupledTapProcessor, NUM_TAPS> mTapProcessors;
    std::unique_ptr<PitchCoordinator> mPitchCoordinator;

    // Processing buffers (avoid allocations in audio thread)
    mutable std::array<float, NUM_TAPS> mDelayOutputs{};
    mutable std::array<float, NUM_TAPS> mPitchOutputs{};

    // Performance monitoring
    mutable std::chrono::high_resolution_clock::time_point mProcessingStartTime;
    mutable std::atomic<double> mDelayProcessingTime{0.0};
    mutable std::atomic<double> mPitchProcessingTime{0.0};

    void processDelayStage(float input);
    void processPitchStage();
    void combineOutputs(float* outputs);
    void updatePerformanceMetrics() const;
};

// ===================================================================
// KEY ARCHITECTURAL BENEFITS:
// ===================================================================
//
// 1. COMPLETE DECOUPLING:
//    - Delay processing never waits for or depends on pitch
//    - Pitch processing gets delay outputs, never blocks delay
//    - Each system can fail independently without affecting the other
//
// 2. UNIFIED COORDINATION:
//    - Single PitchCoordinator manages all 16 taps
//    - No resource competition between taps
//    - Centralized recovery and health monitoring
//    - Shared processing budget prevents cascading failures
//
// 3. GRACEFUL DEGRADATION:
//    - Delay always works, even if pitch completely fails
//    - Individual pitch taps can fail without affecting others
//    - System automatically disables failed components
//    - Clean fallback to delay-only operation
//
// 4. PERFORMANCE ISOLATION:
//    - Delay processing cost is constant regardless of pitch
//    - Pitch processing budget is isolated and bounded
//    - No cross-contamination of processing times
//    - Clear performance attribution per subsystem
//
// 5. OPERATIONAL SIMPLICITY:
//    - Simple interface: setDelayTime(), setPitchShift()
//    - Clear separation of concerns
//    - Easy debugging and monitoring
//    - Predictable behavior under all conditions

} // namespace WaterStick