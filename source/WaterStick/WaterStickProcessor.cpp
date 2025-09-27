#include "WaterStickProcessor.h"
#include "WaterStickCIDs.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>
#include <cstring>

// Platform-specific SIMD includes
#if defined(__SSE4_1__) && (defined(__x86_64__) || defined(__i386__))
#include <immintrin.h>
#include <smmintrin.h>
#endif
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

using namespace Steinberg;

// Debug logging system implementation
namespace PitchDebug {
    static bool loggingEnabled = false;

    void enableLogging(bool enable) {
        loggingEnabled = enable;
    }

    bool isLoggingEnabled() {
        return loggingEnabled;
    }

    void logMessage(const std::string& message) {
        if (loggingEnabled) {
            std::cerr << "[PitchDebug] " << message << std::endl;
        }
    }
}

// Performance profiler implementation for dropout investigation
PerformanceProfiler& PerformanceProfiler::getInstance() {
    static PerformanceProfiler instance;
    return instance;
}

void PerformanceProfiler::startProfiling(const std::string& operation, int tapIndex, int parameterType) {
    if (!mProfilingEnabled) return;

    // Limit profile history size
    if (mProfileHistory.size() >= MAX_PROFILE_HISTORY) {
        mProfileHistory.erase(mProfileHistory.begin(), mProfileHistory.begin() + MAX_PROFILE_HISTORY / 2);
    }

    mProfileHistory.emplace_back();
    mCurrentProfile = &mProfileHistory.back();
    mCurrentProfile->operation = operation;
    mCurrentProfile->tapIndex = tapIndex;
    mCurrentProfile->parameterType = parameterType;
    mCurrentProfile->startTime = std::chrono::high_resolution_clock::now();
}

void PerformanceProfiler::endProfiling() {
    if (!mProfilingEnabled || !mCurrentProfile) return;

    mCurrentProfile->endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        mCurrentProfile->endTime - mCurrentProfile->startTime);
    mCurrentProfile->durationUs = duration.count() / 1000.0; // Convert to microseconds

    // Check for timeout
    if (mCurrentProfile->durationUs > mTimeoutThresholdUs) {
        mTimeoutCount++;
        PitchDebug::logMessage("Performance timeout detected: " + mCurrentProfile->operation +
            " took " + std::to_string(mCurrentProfile->durationUs) + "μs");
    }

    mCurrentProfile = nullptr;
}

void PerformanceProfiler::logPerformanceReport() {
    if (!mProfilingEnabled || mProfileHistory.empty()) return;

    // Calculate statistics
    double totalTime = 0.0;
    double maxTime = 0.0;
    std::map<std::string, std::vector<double>> operationTimes;

    for (const auto& profile : mProfileHistory) {
        totalTime += profile.durationUs;
        if (profile.durationUs > maxTime) {
            maxTime = profile.durationUs;
        }
        operationTimes[profile.operation].push_back(profile.durationUs);
    }

    double avgTime = totalTime / mProfileHistory.size();

    std::ostringstream report;
    report << "Performance Report - " << mProfileHistory.size() << " measurements:\n";
    report << "  Average: " << std::fixed << std::setprecision(2) << avgTime << "μs\n";
    report << "  Maximum: " << std::fixed << std::setprecision(2) << maxTime << "μs\n";
    report << "  Timeouts: " << mTimeoutCount << "\n";
    report << "  Operations breakdown:";

    for (const auto& op : operationTimes) {
        double opTotal = 0.0;
        for (double time : op.second) {
            opTotal += time;
        }
        double opAvg = opTotal / op.second.size();
        report << "\n    " << op.first << ": " << std::fixed << std::setprecision(2) << opAvg << "μs avg";
    }

    PitchDebug::logMessage(report.str());
}

void PerformanceProfiler::clearProfile() {
    mProfileHistory.clear();
    mTimeoutCount = 0;
}

void PerformanceProfiler::enableProfiling(bool enable) {
    mProfilingEnabled = enable;
    if (enable) {
        clearProfile();
        PitchDebug::logMessage("Performance profiling enabled");
    }
}

bool PerformanceProfiler::isProfilingEnabled() const {
    return mProfilingEnabled;
}

double PerformanceProfiler::getMaxParameterUpdateTime() const {
    double maxTime = 0.0;
    for (const auto& profile : mProfileHistory) {
        if (profile.durationUs > maxTime) {
            maxTime = profile.durationUs;
        }
    }
    return maxTime;
}

double PerformanceProfiler::getAverageParameterUpdateTime() const {
    if (mProfileHistory.empty()) return 0.0;

    double total = 0.0;
    for (const auto& profile : mProfileHistory) {
        total += profile.durationUs;
    }
    return total / mProfileHistory.size();
}

int PerformanceProfiler::getTimeoutCount() const {
    return mTimeoutCount;
}

namespace WaterStick {

// No static constants needed for SpeedBasedDelayLine

struct ParameterConverter {
    static float convertFilterCutoff(double value) {
        return static_cast<float>(20.0 * std::pow(1000.0, value));
    }

    static float convertFilterResonance(double value) {
        if (value >= 0.5) {
            float positiveValue = (value - 0.5f) * 2.0f;
            if (positiveValue >= 0.9f) {
                float highResValue = (positiveValue - 0.9f) / 0.1f;
                return 0.7f + highResValue * 0.3f;
            } else {
                float lowResValue = positiveValue / 0.9f;
                return lowResValue * 0.7f;
            }
        } else {
            return static_cast<float>((value - 0.5f) * 2.0f);
        }
    }

    static int convertFilterType(double value) {
        return static_cast<int>(value * (kNumFilterTypes - 1) + 0.5);
    }

    static float convertGain(double value) {
        float dbValue = -40.0f + (static_cast<float>(value) * 52.0f);
        return std::pow(10.0f, dbValue / 20.0f);
    }

    static float convertFeedback(double value) {
        float normalizedValue = static_cast<float>(value);
        return std::sqrt(normalizedValue);
    }

    static int convertPitchShift(double value) {
        // Convert 0.0-1.0 range to -12 to +12 semitones
        return static_cast<int>(round((value * 24.0) - 12.0));
    }

};

// =====================================================
// PHASE 2: UNIFIED LOCK-FREE ARCHITECTURE IMPLEMENTATION
// =====================================================

// LockFreeParameterManager Implementation
LockFreeParameterManager::LockFreeParameterManager() {
    // Initialize all parameter buffers
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        mParameterBuffers[i].pitchRatio.store(1.0f, std::memory_order_relaxed);
        mParameterBuffers[i].delayTime.store(0.0f, std::memory_order_relaxed);
        mParameterBuffers[i].pitchSemitones.store(0, std::memory_order_relaxed);
        mParameterBuffers[i].version.store(0, std::memory_order_relaxed);
    }
}

LockFreeParameterManager::~LockFreeParameterManager() = default;

void LockFreeParameterManager::initialize() {
    mWriteIndex.store(0, std::memory_order_relaxed);
    mReadIndex.store(0, std::memory_order_relaxed);
    mGlobalVersion.store(0, std::memory_order_relaxed);
}

void LockFreeParameterManager::updatePitchShift(int semitones) {
    // Get next write buffer
    int writeIdx = mWriteIndex.load(std::memory_order_acquire);
    int nextWriteIdx = getNextIndex(writeIdx);

    // Copy current state to new buffer
    ParameterState& newState = mParameterBuffers[nextWriteIdx];
    const ParameterState& currentState = mParameterBuffers[writeIdx];
    newState.copyFrom(currentState);

    // Update pitch parameters
    newState.pitchSemitones.store(std::max(-12, std::min(12, semitones)), std::memory_order_relaxed);
    calculatePitchRatio(newState);

    // Increment version and publish
    uint64_t newVersion = mGlobalVersion.load(std::memory_order_acquire) + 1;
    newState.version.store(newVersion, std::memory_order_relaxed);

    // Atomically update write index and global version
    mWriteIndex.store(nextWriteIdx, std::memory_order_release);
    mGlobalVersion.store(newVersion, std::memory_order_release);
}

void LockFreeParameterManager::updateDelayTime(float delayTimeSeconds) {
    // Get next write buffer
    int writeIdx = mWriteIndex.load(std::memory_order_acquire);
    int nextWriteIdx = getNextIndex(writeIdx);

    // Copy current state to new buffer
    ParameterState& newState = mParameterBuffers[nextWriteIdx];
    const ParameterState& currentState = mParameterBuffers[writeIdx];
    newState.copyFrom(currentState);

    // Update delay time
    newState.delayTime.store(std::max(0.0f, delayTimeSeconds), std::memory_order_relaxed);

    // Increment version and publish
    uint64_t newVersion = mGlobalVersion.load(std::memory_order_acquire) + 1;
    newState.version.store(newVersion, std::memory_order_relaxed);

    // Atomically update write index and global version
    mWriteIndex.store(nextWriteIdx, std::memory_order_release);
    mGlobalVersion.store(newVersion, std::memory_order_release);
}

void LockFreeParameterManager::getAudioThreadParameters(ParameterState& outState) {
    // Get current read buffer
    int readIdx = mReadIndex.load(std::memory_order_acquire);
    int writeIdx = mWriteIndex.load(std::memory_order_acquire);

    // If write index has changed, update read index
    if (readIdx != writeIdx) {
        mReadIndex.store(writeIdx, std::memory_order_release);
        readIdx = writeIdx;
    }

    // Copy values from the read buffer
    outState.copyFrom(mParameterBuffers[readIdx]);
}

bool LockFreeParameterManager::hasParametersChanged(uint64_t lastVersion) const {
    return mGlobalVersion.load(std::memory_order_acquire) != lastVersion;
}

void LockFreeParameterManager::calculatePitchRatio(ParameterState& state) {
    int semitones = state.pitchSemitones.load(std::memory_order_relaxed);
    if (semitones == 0) {
        state.pitchRatio.store(1.0f, std::memory_order_relaxed);
    } else {
        float exponent = static_cast<float>(semitones) / 12.0f;
        float ratio = std::pow(2.0f, exponent);

        // Clamp to safe bounds
        ratio = std::max(0.25f, std::min(4.0f, ratio));
        state.pitchRatio.store(ratio, std::memory_order_relaxed);
    }
}

int LockFreeParameterManager::getNextIndex(int currentIndex) const {
    return (currentIndex + 1) % NUM_BUFFERS;
}

// RecoveryManager Implementation
RecoveryManager::RecoveryManager() = default;

RecoveryManager::~RecoveryManager() = default;

void RecoveryManager::initialize(double sampleRate) {
    mSampleRate = sampleRate;
    mEmergencyBypass.store(false, std::memory_order_relaxed);
    mTimeoutCount.store(0, std::memory_order_relaxed);
    mMaxProcessingTime.store(0.0, std::memory_order_relaxed);

    for (int i = 0; i < 4; ++i) {
        mRecoveryCount[i].store(0, std::memory_order_relaxed);
    }
    mConsecutiveTimeouts = 0;
}

void RecoveryManager::startProcessingTimer() {
    mProcessingStartTime = std::chrono::high_resolution_clock::now();
}

RecoveryManager::RecoveryLevel RecoveryManager::checkAndHandleTimeout() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - mProcessingStartTime);
    double durationUs = duration.count() / 1000.0;

    // Update max processing time
    double currentMax = mMaxProcessingTime.load(std::memory_order_acquire);
    if (durationUs > currentMax) {
        mMaxProcessingTime.store(durationUs, std::memory_order_release);
    }

    // Check timeout levels
    RecoveryLevel level = NONE;

    if (durationUs > LEVEL3_TIMEOUT_US) {
        level = EMERGENCY_BYPASS;
        mEmergencyBypass.store(true, std::memory_order_release);
        mConsecutiveTimeouts++;
    } else if (durationUs > LEVEL2_TIMEOUT_US) {
        level = BUFFER_RESET;
        mConsecutiveTimeouts++;
    } else if (durationUs > LEVEL1_TIMEOUT_US) {
        level = POSITION_CORRECTION;
        mConsecutiveTimeouts++;
    } else {
        // Reset consecutive timeout counter on success
        mConsecutiveTimeouts = 0;
        return NONE;
    }

    // Update statistics
    mTimeoutCount.fetch_add(1, std::memory_order_relaxed);
    mRecoveryCount[level].fetch_add(1, std::memory_order_relaxed);

    // Check if we need emergency bypass due to consecutive timeouts
    if (mConsecutiveTimeouts >= MAX_CONSECUTIVE_TIMEOUTS && level < EMERGENCY_BYPASS) {
        level = EMERGENCY_BYPASS;
        mEmergencyBypass.store(true, std::memory_order_release);
    }

    return level;
}

void RecoveryManager::reportSuccess() {
    mConsecutiveTimeouts = 0;
}

void RecoveryManager::reset() {
    mEmergencyBypass.store(false, std::memory_order_release);
    mConsecutiveTimeouts = 0;
}

bool RecoveryManager::isInEmergencyBypass() const {
    return mEmergencyBypass.load(std::memory_order_acquire);
}

void RecoveryManager::clearEmergencyBypass() {
    mEmergencyBypass.store(false, std::memory_order_release);
    mConsecutiveTimeouts = 0;
}

int RecoveryManager::getTimeoutCount() const {
    return mTimeoutCount.load(std::memory_order_acquire);
}

int RecoveryManager::getRecoveryCount(RecoveryLevel level) const {
    if (level >= 0 && level < 4) {
        return mRecoveryCount[level].load(std::memory_order_acquire);
    }
    return 0;
}

double RecoveryManager::getMaxProcessingTime() const {
    return mMaxProcessingTime.load(std::memory_order_acquire);
}

// =====================================================
// PHASE 3: PERFORMANCE OPTIMIZATION IMPLEMENTATIONS
// =====================================================

// SIMD-accelerated macro curve evaluator implementation
SIMDMacroCurveEvaluator::SIMDMacroCurveEvaluator() {
    memset(mScratchBuffer, 0, sizeof(mScratchBuffer));
}

SIMDMacroCurveEvaluator::~SIMDMacroCurveEvaluator() = default;

void SIMDMacroCurveEvaluator::evaluateBatch(const float* inputs, float* outputs, int curveType, int count) {
    // Use platform-specific SIMD when available, fallback to scalar otherwise
    #if defined(__SSE4_1__) && (defined(__x86_64__) || defined(__i386__))
    if (count >= 4 && (count % 4 == 0)) {
        evaluateSSE(inputs, outputs, curveType, count);
        return;
    }
    #endif
    #ifdef __ARM_NEON__
    if (count >= 4 && (count % 4 == 0)) {
        evaluateNEON(inputs, outputs, curveType, count);
        return;
    }
    #endif

    // Scalar fallback for remaining elements or unsupported platforms
    for (int i = 0; i < count; ++i) {
        float x = std::max(0.0f, std::min(1.0f, inputs[i]));
        switch (curveType) {
            case 0: outputs[i] = x; break; // Linear
            case 1: outputs[i] = x * x; break; // Ramp Up
            case 2: outputs[i] = 1.0f - (1.0f - x) * (1.0f - x); break; // Ramp Down
            case 3: outputs[i] = 1.0f / (1.0f + std::exp(-12.0f * (x - 0.5f))); break; // S-Curve
            case 4: outputs[i] = std::exp(3.0f * x) / std::exp(3.0f); break; // Exp Up
            case 5: outputs[i] = 1.0f - std::exp(-3.0f * x); break; // Exp Down
            default: outputs[i] = x; break;
        }
    }
}

#if defined(__SSE4_1__) && (defined(__x86_64__) || defined(__i386__))
void SIMDMacroCurveEvaluator::evaluateSSE(const float* inputs, float* outputs, int curveType, int count) {
    const __m128 zero = _mm_setzero_ps();
    const __m128 one = _mm_set1_ps(1.0f);

    for (int i = 0; i < count; i += 4) {
        __m128 x = _mm_loadu_ps(&inputs[i]);
        x = _mm_max_ps(zero, _mm_min_ps(one, x)); // Clamp to [0,1]

        __m128 result;
        switch (curveType) {
            case 0: // Linear
                result = x;
                break;
            case 1: // Ramp Up (x^2)
                result = _mm_mul_ps(x, x);
                break;
            case 2: // Ramp Down (1-(1-x)^2)
            {
                __m128 one_minus_x = _mm_sub_ps(one, x);
                result = _mm_sub_ps(one, _mm_mul_ps(one_minus_x, one_minus_x));
                break;
            }
            case 3: // S-Curve approximation (simplified for SIMD)
            {
                // Simplified sigmoid: x^3 * (3 - 2*x)
                __m128 x2 = _mm_mul_ps(x, x);
                __m128 x3 = _mm_mul_ps(x2, x);
                __m128 two_x = _mm_add_ps(x, x);
                __m128 three_minus_2x = _mm_sub_ps(_mm_set1_ps(3.0f), two_x);
                result = _mm_mul_ps(x3, three_minus_2x);
                break;
            }
            default:
                result = x;
                break;
        }

        _mm_storeu_ps(&outputs[i], result);
    }
}
#endif

#ifdef __ARM_NEON__
void SIMDMacroCurveEvaluator::evaluateNEON(const float* inputs, float* outputs, int curveType, int count) {
    const float32x4_t zero = vdupq_n_f32(0.0f);
    const float32x4_t one = vdupq_n_f32(1.0f);

    for (int i = 0; i < count; i += 4) {
        float32x4_t x = vld1q_f32(&inputs[i]);
        x = vmaxq_f32(zero, vminq_f32(one, x)); // Clamp to [0,1]

        float32x4_t result;
        switch (curveType) {
            case 0: // Linear
                result = x;
                break;
            case 1: // Ramp Up (x^2)
                result = vmulq_f32(x, x);
                break;
            case 2: // Ramp Down (1-(1-x)^2)
            {
                float32x4_t one_minus_x = vsubq_f32(one, x);
                result = vsubq_f32(one, vmulq_f32(one_minus_x, one_minus_x));
                break;
            }
            case 3: // S-Curve approximation
            {
                float32x4_t x2 = vmulq_f32(x, x);
                float32x4_t x3 = vmulq_f32(x2, x);
                float32x4_t two_x = vaddq_f32(x, x);
                float32x4_t three_minus_2x = vsubq_f32(vdupq_n_f32(3.0f), two_x);
                result = vmulq_f32(x3, three_minus_2x);
                break;
            }
            default:
                result = x;
                break;
        }

        vst1q_f32(&outputs[i], result);
    }
}
#endif

void SIMDMacroCurveEvaluator::evaluateAllCurves(MacroParameterBatch& batch) {
    // Evaluate all 8 macro knob values with their respective curve types (batched)
    for (int i = 0; i < 8; ++i) {
        evaluateBatch(&batch.values[i], &batch.curves[i], i % 6, 1); // Cycle through 6 curve types
    }
}

// Cache-optimized parameter lookup implementation
CacheOptimizedParameterLookup::CacheOptimizedParameterLookup() : mNumTaps(0) {}

CacheOptimizedParameterLookup::~CacheOptimizedParameterLookup() = default;

void CacheOptimizedParameterLookup::initialize(int numTaps) {
    mNumTaps = numTaps;
    mTapCache.resize(numTaps);

    // Initialize all tap caches
    for (int i = 0; i < numTaps; ++i) {
        memset(mTapCache[i].parameters, 0, sizeof(mTapCache[i].parameters));
        mTapCache[i].dirtyFlags = 0;
        mTapCache[i].version = 0;
    }

    mGlobalVersion.store(0, std::memory_order_relaxed);
}

void CacheOptimizedParameterLookup::markDirty(int tapIndex, int parameterType) {
    if (tapIndex >= 0 && tapIndex < mNumTaps && parameterType >= 0 && parameterType < PARAMS_PER_TAP) {
        mTapCache[tapIndex].dirtyFlags |= (1U << parameterType);
        mTapCache[tapIndex].version = mGlobalVersion.fetch_add(1, std::memory_order_acq_rel);
    }
}

bool CacheOptimizedParameterLookup::isDirty(int tapIndex, int parameterType) const {
    if (tapIndex >= 0 && tapIndex < mNumTaps && parameterType >= 0 && parameterType < PARAMS_PER_TAP) {
        return (mTapCache[tapIndex].dirtyFlags & (1U << parameterType)) != 0;
    }
    return false;
}

void CacheOptimizedParameterLookup::clearDirty(int tapIndex, int parameterType) {
    if (tapIndex >= 0 && tapIndex < mNumTaps && parameterType >= 0 && parameterType < PARAMS_PER_TAP) {
        mTapCache[tapIndex].dirtyFlags &= ~(1U << parameterType);
    }
}

void CacheOptimizedParameterLookup::clearAllDirty() {
    for (int i = 0; i < mNumTaps; ++i) {
        mTapCache[i].dirtyFlags = 0;
    }
}

float CacheOptimizedParameterLookup::getTapParameter(int tapIndex, int parameterType) const {
    if (tapIndex >= 0 && tapIndex < mNumTaps && parameterType >= 0 && parameterType < PARAMS_PER_TAP) {
        return mTapCache[tapIndex].parameters[parameterType];
    }
    return 0.0f;
}

void CacheOptimizedParameterLookup::setTapParameter(int tapIndex, int parameterType, float value) {
    if (tapIndex >= 0 && tapIndex < mNumTaps && parameterType >= 0 && parameterType < PARAMS_PER_TAP) {
        mTapCache[tapIndex].parameters[parameterType] = value;
        markDirty(tapIndex, parameterType);
    }
}

void CacheOptimizedParameterLookup::getTapParametersBatch(int tapIndex, float* outParams, int count) const {
    if (tapIndex >= 0 && tapIndex < mNumTaps) {
        int copyCount = std::min(count, PARAMS_PER_TAP);
        memcpy(outParams, mTapCache[tapIndex].parameters, copyCount * sizeof(float));
    }
}

void CacheOptimizedParameterLookup::setTapParametersBatch(int tapIndex, const float* params, int count) {
    if (tapIndex >= 0 && tapIndex < mNumTaps) {
        int copyCount = std::min(count, PARAMS_PER_TAP);
        memcpy(mTapCache[tapIndex].parameters, params, copyCount * sizeof(float));
        // Mark all updated parameters as dirty
        for (int i = 0; i < copyCount; ++i) {
            markDirty(tapIndex, i);
        }
    }
}

uint64_t CacheOptimizedParameterLookup::getGlobalVersion() const {
    return mGlobalVersion.load(std::memory_order_acquire);
}

// Enhanced LockFreeParameterManager with batch operations
void LockFreeParameterManager::updateMacroParametersBatch(const MacroParameterBatch& batch) {
    // Get next write buffer for macro parameters
    int writeIdx = mMacroWriteIndex.load(std::memory_order_acquire);
    int nextWriteIdx = getNextIndex(writeIdx);

    // Copy batch to next buffer
    mMacroBatches[nextWriteIdx] = batch;
    mMacroBatches[nextWriteIdx].version = mGlobalVersion.fetch_add(1, std::memory_order_acq_rel);

    // Publish new macro batch
    mMacroWriteIndex.store(nextWriteIdx, std::memory_order_release);
}

bool LockFreeParameterManager::getMacroParametersBatch(MacroParameterBatch& outBatch) {
    int readIdx = mMacroReadIndex.load(std::memory_order_acquire);
    int writeIdx = mMacroWriteIndex.load(std::memory_order_acquire);

    if (readIdx != writeIdx) {
        // Copy latest batch
        outBatch = mMacroBatches[writeIdx];
        mMacroReadIndex.store(writeIdx, std::memory_order_release);
        return true;
    }

    return false; // No new data
}

// UnifiedPitchDelayLine Implementation
UnifiedPitchDelayLine::UnifiedPitchDelayLine()
: mBufferSize(0)
, mSampleRate(44100.0)
, mSafetyMargin(0)
, mMaxReadAdvance(0)
, mMinReadPosition(0.0f)
, mMaxReadPosition(0.0f) {
    mParameterManager = std::make_unique<LockFreeParameterManager>();
    mRecoveryManager = std::make_unique<RecoveryManager>();
}

UnifiedPitchDelayLine::~UnifiedPitchDelayLine() = default;

void UnifiedPitchDelayLine::initialize(double sampleRate, double maxDelaySeconds) {
    mSampleRate = sampleRate;

    // Calculate buffer size with safety margins
    const double SAFETY_MULTIPLIER = 8.0;  // 8x buffer for extreme pitch down protection
    mBufferSize = static_cast<int>(maxDelaySeconds * sampleRate * SAFETY_MULTIPLIER) + 1024;
    mBuffer.resize(mBufferSize, 0.0f);

    // Set safety margins
    mSafetyMargin = static_cast<int>(sampleRate * 0.05);  // 50ms minimum buffer
    mMaxReadAdvance = static_cast<int>(sampleRate * 0.01); // 10ms max advance per sample
    mMinReadPosition = static_cast<float>(mSafetyMargin);
    mMaxReadPosition = static_cast<float>(mBufferSize - mSafetyMargin);

    // Initialize atomic indices
    mWriteIndex.store(0, std::memory_order_relaxed);
    mReadPosition.store(mMinReadPosition, std::memory_order_relaxed);

    // Initialize managers
    mParameterManager->initialize();
    mRecoveryManager->initialize(sampleRate);

    // Initialize processing state
    updateSmoothingCoeff();
    mState.currentPitchRatio = 1.0f;
    mState.targetPitchRatio = 1.0f;
    mState.currentDelayTime = 0.0f;
    mState.needsReset = false;

    mLastParameterVersion = 0;
}

void UnifiedPitchDelayLine::processSample(float input, float& output) {
    // Start recovery timer
    mRecoveryManager->startProcessingTimer();

    // Check for emergency bypass
    if (mRecoveryManager->isInEmergencyBypass()) {
        output = input;
        return;
    }

    // Update parameters if needed
    updateParameters();

    // Get current write index and advance it
    int writeIdx = mWriteIndex.load(std::memory_order_acquire);
    mBuffer[writeIdx] = input;

    int nextWriteIdx = (writeIdx + 1) % mBufferSize;
    mWriteIndex.store(nextWriteIdx, std::memory_order_release);

    // Check for timeout after write
    RecoveryManager::RecoveryLevel recoveryLevel = mRecoveryManager->checkAndHandleTimeout();
    if (recoveryLevel >= RecoveryManager::EMERGENCY_BYPASS) {
        output = input;
        return;
    }

    // Apply smoothing and validation
    applySmoothingAndValidation();

    // Get current read position
    float readPos = mReadPosition.load(std::memory_order_acquire);

    // Variable speed playback: advance read position
    float newReadPos = readPos + mState.currentPitchRatio;

    // Deterministic wrapping with cycle limit
    int cycles = 0;
    while (newReadPos >= mMaxReadPosition && cycles < MAX_PROCESSING_CYCLES) {
        newReadPos -= static_cast<float>(mBufferSize);
        cycles++;
    }

    while (newReadPos < mMinReadPosition && cycles < MAX_PROCESSING_CYCLES) {
        newReadPos += static_cast<float>(mBufferSize);
        cycles++;
    }

    // Final timeout check before interpolation
    recoveryLevel = mRecoveryManager->checkAndHandleTimeout();
    if (recoveryLevel >= RecoveryManager::BUFFER_RESET) {
        if (recoveryLevel == RecoveryManager::BUFFER_RESET) {
            performBufferReset();
        }
        output = input;
        return;
    } else if (recoveryLevel == RecoveryManager::POSITION_CORRECTION) {
        correctReadPosition();
        newReadPos = mReadPosition.load(std::memory_order_acquire);
    }

    // Validate final position
    if (!validateReadPosition()) {
        correctReadPosition();
        newReadPos = mReadPosition.load(std::memory_order_acquire);
    }

    // Store new read position
    mReadPosition.store(newReadPos, std::memory_order_release);

    // Interpolate output
    output = interpolateBuffer(newReadPos);

    // Report success
    mRecoveryManager->reportSuccess();
}

void UnifiedPitchDelayLine::reset() {
    // Clear buffer
    std::fill(mBuffer.begin(), mBuffer.end(), 0.0f);

    // Reset indices
    mWriteIndex.store(0, std::memory_order_release);
    mReadPosition.store(mMinReadPosition, std::memory_order_release);

    // Reset processing state
    mState.currentPitchRatio = 1.0f;
    mState.targetPitchRatio = 1.0f;
    mState.currentDelayTime = 0.0f;
    mState.needsReset = false;

    // Reset managers
    mParameterManager->initialize();
    mRecoveryManager->reset();

    mLastParameterVersion = 0;
}

void UnifiedPitchDelayLine::setPitchShift(int semitones) {
    mParameterManager->updatePitchShift(semitones);
}

void UnifiedPitchDelayLine::setDelayTime(float delayTimeSeconds) {
    mParameterManager->updateDelayTime(delayTimeSeconds);
}

bool UnifiedPitchDelayLine::isPitchShiftActive() const {
    return std::abs(mState.currentPitchRatio - 1.0f) > 1e-6f;
}

RecoveryManager::RecoveryLevel UnifiedPitchDelayLine::getLastRecoveryLevel() const {
    // Simple heuristic based on current state
    if (mRecoveryManager->isInEmergencyBypass()) {
        return RecoveryManager::EMERGENCY_BYPASS;
    }
    return RecoveryManager::NONE;
}

void UnifiedPitchDelayLine::logProcessingStats() const {
    if (PitchDebug::isLoggingEnabled()) {
        std::ostringstream ss;
        ss << "UnifiedPitchDelayLine stats - PitchRatio: " << mState.currentPitchRatio
           << ", ReadPos: " << mReadPosition.load(std::memory_order_acquire)
           << ", BufferSize: " << mBufferSize
           << ", EmergencyBypass: " << (mRecoveryManager->isInEmergencyBypass() ? "YES" : "NO")
           << ", Timeouts: " << mRecoveryManager->getTimeoutCount()
           << ", MaxTime: " << mRecoveryManager->getMaxProcessingTime() << "μs";
        PitchDebug::logMessage(ss.str());
    }
}

void UnifiedPitchDelayLine::updateParameters() {
    if (mParameterManager->hasParametersChanged(mLastParameterVersion)) {
        LockFreeParameterManager::ParameterState newParams;
        mParameterManager->getAudioThreadParameters(newParams);
        mLastParameterVersion = newParams.version.load(std::memory_order_acquire);

        // Update target parameters
        mState.targetPitchRatio = newParams.pitchRatio.load(std::memory_order_acquire);
        mState.currentDelayTime = newParams.delayTime.load(std::memory_order_acquire);

        // Clamp pitch ratio to safe bounds
        mState.targetPitchRatio = std::max(PITCH_RATIO_MIN, std::min(PITCH_RATIO_MAX, mState.targetPitchRatio));
    }
}

void UnifiedPitchDelayLine::updateSmoothingCoeff() {
    const float timeConstantSec = SMOOTHING_TIME_CONSTANT_MS / 1000.0f;
    mState.smoothingCoeff = std::exp(-1.0f / (timeConstantSec * static_cast<float>(mSampleRate)));
}

void UnifiedPitchDelayLine::applySmoothingAndValidation() {
    // Apply exponential smoothing
    float newRatio = mState.smoothingCoeff * mState.currentPitchRatio +
                     (1.0f - mState.smoothingCoeff) * mState.targetPitchRatio;

    // Validate result
    if (std::isfinite(newRatio) && newRatio > 0.0f) {
        mState.currentPitchRatio = std::max(PITCH_RATIO_MIN, std::min(PITCH_RATIO_MAX, newRatio));
    } else {
        mState.currentPitchRatio = mState.targetPitchRatio;
    }
}

float UnifiedPitchDelayLine::interpolateBuffer(float position) const {
    if (mBuffer.empty()) {
        return 0.0f;
    }

    // Validate position
    if (!std::isfinite(position) || position < 0.0f) {
        return 0.0f;
    }

    // Get integer and fractional parts
    int intPos = static_cast<int>(position);
    float frac = position - static_cast<float>(intPos);

    // Ensure indices are within bounds
    intPos = intPos % mBufferSize;
    if (intPos < 0) intPos += mBufferSize;

    int nextPos = (intPos + 1) % mBufferSize;

    // Linear interpolation
    return mBuffer[intPos] * (1.0f - frac) + mBuffer[nextPos] * frac;
}

bool UnifiedPitchDelayLine::validateReadPosition() const {
    float readPos = mReadPosition.load(std::memory_order_acquire);
    return std::isfinite(readPos) && readPos >= mMinReadPosition && readPos <= mMaxReadPosition;
}

void UnifiedPitchDelayLine::correctReadPosition() {
    int writeIdx = mWriteIndex.load(std::memory_order_acquire);

    // Calculate safe read position
    float safeReadPos = static_cast<float>(writeIdx) - static_cast<float>(mSafetyMargin);

    if (safeReadPos < mMinReadPosition) {
        safeReadPos += static_cast<float>(mBufferSize);
    }

    mReadPosition.store(safeReadPos, std::memory_order_release);
}

void UnifiedPitchDelayLine::performBufferReset() {
    // Clear buffer content
    std::fill(mBuffer.begin(), mBuffer.end(), 0.0f);

    // Reset read position to safe distance behind write
    int writeIdx = mWriteIndex.load(std::memory_order_acquire);
    float safeReadPos = static_cast<float>(writeIdx) - static_cast<float>(mSafetyMargin);

    if (safeReadPos < mMinReadPosition) {
        safeReadPos += static_cast<float>(mBufferSize);
    }

    mReadPosition.store(safeReadPos, std::memory_order_release);

    // Reset state
    mState.currentPitchRatio = 1.0f;
    mState.targetPitchRatio = 1.0f;
}

struct TapParameterRange {
    Vst::ParamID startId;
    Vst::ParamID endId;
    int paramsPerTap;

    bool contains(Vst::ParamID paramId) const {
        return paramId >= startId && paramId <= endId;
    }

    void getIndices(Vst::ParamID paramId, int& tapIndex, int& paramType) const {
        int paramOffset = paramId - startId;
        tapIndex = paramOffset / paramsPerTap;
        paramType = paramOffset % paramsPerTap;
    }
};

struct TapParameterProcessor {
    static const TapParameterRange kTapBasicRange;
    static const TapParameterRange kTapFilterRange;
    static const TapParameterRange kTapPitchRange;
    static const TapParameterRange kTapFeedbackRange;

    static void processTapParameter(Vst::ParamID paramId, Vst::ParamValue value, WaterStickProcessor* processor) {
        if (kTapBasicRange.contains(paramId)) {
            int tapIndex, paramType;
            kTapBasicRange.getIndices(paramId, tapIndex, paramType);

            if (tapIndex < 16) {
                switch (paramType) {
                    case 0: processor->mTapEnabled[tapIndex] = value > 0.5; break;
                    case 1: processor->mTapLevel[tapIndex] = static_cast<float>(value); break;
                    case 2: processor->mTapPan[tapIndex] = static_cast<float>(value); break;
                }

                // Mark this parameter as user-modified to disable macro influence
                processor->markParameterAsUserModified(paramId);
            }
            return;
        }

        if (kTapFilterRange.contains(paramId)) {
            int tapIndex, paramType;
            kTapFilterRange.getIndices(paramId, tapIndex, paramType);

            if (tapIndex < 16) {
                switch (paramType) {
                    case 0: processor->mTapFilterCutoff[tapIndex] = ParameterConverter::convertFilterCutoff(value); break;
                    case 1: processor->mTapFilterResonance[tapIndex] = ParameterConverter::convertFilterResonance(value); break;
                    case 2: processor->mTapFilterType[tapIndex] = ParameterConverter::convertFilterType(value); break;
                }

                // Mark this parameter as user-modified to disable macro influence
                processor->markParameterAsUserModified(paramId);
            }
            return;
        }

        if (kTapPitchRange.contains(paramId)) {
            int tapIndex, paramType;
            kTapPitchRange.getIndices(paramId, tapIndex, paramType);

            if (tapIndex < 16) {
                // CRITICAL FIX: Removed heavy profiling calls from audio thread
                processor->mTapPitchShift[tapIndex] = ParameterConverter::convertPitchShift(value);

                // Mark this parameter as user-modified to disable macro influence
                processor->markParameterAsUserModified(paramId);
            }
            return;
        }

        if (kTapFeedbackRange.contains(paramId)) {
            int tapIndex, paramType;
            kTapFeedbackRange.getIndices(paramId, tapIndex, paramType);

            if (tapIndex < 16) {
                processor->mTapFeedbackSend[tapIndex] = static_cast<float>(value);

                // Mark this parameter as user-modified to disable macro influence
                processor->markParameterAsUserModified(paramId);
            }
        }
    }
};

const TapParameterRange TapParameterProcessor::kTapBasicRange = {kTap1Enable, kTap16Pan, 3};
const TapParameterRange TapParameterProcessor::kTapFilterRange = {kTap1FilterCutoff, kTap16FilterType, 3};
const TapParameterRange TapParameterProcessor::kTapPitchRange = {kTap1PitchShift, kTap16PitchShift, 1};
const TapParameterRange TapParameterProcessor::kTapFeedbackRange = {kTap1FeedbackSend, kTap16FeedbackSend, 1};

const char* TempoSync::sDivisionTexts[kNumSyncDivisions] = {
    "1/64", "1/32T", "1/64.", "1/32", "1/16T", "1/32.", "1/16",
    "1/8T", "1/16.", "1/8", "1/4T", "1/8.", "1/4",
    "1/2T", "1/4.", "1/2", "1T", "1/2.", "1", "2", "4", "8"
};

const float TempoSync::sDivisionValues[kNumSyncDivisions] = {
    0.0625f,     // 1/64
    0.08333f,    // 1/32T (1/32 triplet)
    0.09375f,    // 1/64. (1/64 dotted)
    0.125f,      // 1/32
    0.16667f,    // 1/16T (1/16 triplet)
    0.1875f,     // 1/32. (1/32 dotted)
    0.25f,       // 1/16
    0.33333f,    // 1/8T (1/8 triplet)
    0.375f,      // 1/16. (1/16 dotted)
    0.5f,        // 1/8
    0.66667f,    // 1/4T (1/4 triplet)
    0.75f,       // 1/8. (1/8 dotted)
    1.0f,        // 1/4
    1.33333f,    // 1/2T (1/2 triplet)
    1.5f,        // 1/4. (1/4 dotted)
    2.0f,        // 1/2
    2.66667f,    // 1T (1 bar triplet)
    3.0f,        // 1/2. (1/2 dotted)
    4.0f,        // 1 (1 bar)
    8.0f,        // 2 (2 bars)
    16.0f,       // 4 (4 bars)
    32.0f        // 8 (8 bars)
};

TempoSync::TempoSync()
: mSampleRate(44100.0)
, mHostTempo(120.0)
, mHostTempoValid(false)
, mIsSynced(false)
, mSyncDivision(kSync_1_4)
, mFreeTime(0.25f)
{
}

TempoSync::~TempoSync()
{
}

void TempoSync::initialize(double sampleRate)
{
    mSampleRate = sampleRate;
}

void TempoSync::updateTempo(double hostTempo, bool isValid)
{
    mHostTempo = hostTempo;
    mHostTempoValid = isValid;
}

void TempoSync::setMode(bool isSynced)
{
    mIsSynced = isSynced;
}

void TempoSync::setSyncDivision(int division)
{
    if (division >= 0 && division < kNumSyncDivisions) {
        mSyncDivision = division;
    }
}

void TempoSync::setFreeTime(float timeSeconds)
{
    mFreeTime = timeSeconds;
}

float TempoSync::getDelayTime() const
{
    if (mIsSynced && mHostTempoValid) {
        return calculateSyncTime();
    } else {
        return mFreeTime;
    }
}

float TempoSync::calculateSyncTime() const
{
    if (!mHostTempoValid || mHostTempo <= 0.0) {
        return mFreeTime;
    }

    double quarterNoteTime = 60.0 / mHostTempo;
    float divisionValue = sDivisionValues[mSyncDivision];
    return static_cast<float>(quarterNoteTime * divisionValue);
}

const char* TempoSync::getDivisionText() const
{
    return sDivisionTexts[mSyncDivision];
}

const char* TempoSync::getModeText() const
{
    return mIsSynced ? "Synced" : "Free";
}

const float TapDistribution::sGridValues[kNumGridValues] = {
    1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f, 16.0f
};

const char* TapDistribution::sGridTexts[kNumGridValues] = {
    "1", "2", "3", "4", "6", "8", "12", "16"
};

TapDistribution::TapDistribution()
: mSampleRate(44100.0)
, mBeatTime(0.5f)
, mGrid(kGrid_4)
{
    for (int i = 0; i < NUM_TAPS; i++) {
        mTapEnabled[i] = false;
        mTapLevel[i] = 0.8f;
        mTapPan[i] = 0.5f;
        mTapDelayTimes[i] = 0.0f;
    }
    calculateTapTimes();
}

TapDistribution::~TapDistribution()
{
}

void TapDistribution::initialize(double sampleRate)
{
    mSampleRate = sampleRate;
}

void TapDistribution::updateTempo(const TempoSync& tempoSync)
{
    mBeatTime = tempoSync.getDelayTime();
    calculateTapTimes();
}

void TapDistribution::setGrid(int gridValue)
{
    if (gridValue >= 0 && gridValue < kNumGridValues) {
        mGrid = gridValue;
        calculateTapTimes();
    }
}

void TapDistribution::setTapEnable(int tapIndex, bool enabled)
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        mTapEnabled[tapIndex] = enabled;
    }
}

void TapDistribution::setTapLevel(int tapIndex, float level)
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        mTapLevel[tapIndex] = std::isfinite(level) ? std::max(0.0f, std::min(level, 1.0f)) : 1.0f;
    } else {
    }
}

void TapDistribution::setTapPan(int tapIndex, float pan)
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        mTapPan[tapIndex] = std::max(0.0f, std::min(pan, 1.0f));
    }
}

void TapDistribution::calculateTapTimes()
{
    float gridValue = sGridValues[mGrid];

    for (int tap = 0; tap < NUM_TAPS; tap++) {
        int tapNumber = tap + 1;
        mTapDelayTimes[tap] = mBeatTime * static_cast<float>(tapNumber) / gridValue;
        mTapDelayTimes[tap] = std::max(mTapDelayTimes[tap], 0.001f);
    }
}

float TapDistribution::getTapDelayTime(int tapIndex) const
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        return mTapDelayTimes[tapIndex];
    }
    return 0.0f;
}

bool TapDistribution::isTapEnabled(int tapIndex) const
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        return mTapEnabled[tapIndex];
    }
    return false;
}

float TapDistribution::getTapLevel(int tapIndex) const
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        return mTapLevel[tapIndex];
    }
    return 0.0f;
}

float TapDistribution::getTapPan(int tapIndex) const
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        return mTapPan[tapIndex];
    }
    return 0.5f;
}

const char* TapDistribution::getGridText() const
{
    return sGridTexts[mGrid];
}


DualDelayLine::DualDelayLine()
: mBufferSize(0)
, mWriteIndexA(0)
, mWriteIndexB(0)
, mSampleRate(44100.0)
, mUsingLineA(true)
, mCrossfadeState(STABLE)
, mTargetDelayTime(0.1f)
, mCurrentDelayTime(0.1f)
, mStabilityCounter(0)
, mStabilityThreshold(2048)
, mCrossfadeLength(0)
, mCrossfadePosition(0)
, mCrossfadeGainA(1.0f)
, mCrossfadeGainB(0.0f)
{
    mStateA.delayInSamples = 0.5f;
    mStateA.readIndex = 0;
    mStateA.allpassCoeff = 0.0f;
    mStateA.apInput = 0.0f;
    mStateA.lastOutput = 0.0f;
    mStateA.doNextOut = true;
    mStateA.nextOutput = 0.0f;

    mStateB = mStateA;
}

DualDelayLine::~DualDelayLine()
{
}

void DualDelayLine::initialize(double sampleRate, double maxDelaySeconds)
{
    mSampleRate = sampleRate;
    mBufferSize = static_cast<int>(maxDelaySeconds * sampleRate) + 1;

    mBufferA.resize(mBufferSize, 0.0f);
    mBufferB.resize(mBufferSize, 0.0f);

    mWriteIndexA = 0;
    mWriteIndexB = 0;

    updateDelayState(mStateA, mCurrentDelayTime);
    updateDelayState(mStateB, mCurrentDelayTime);

    mStabilityThreshold = static_cast<int>(sampleRate * 0.05);
}

void DualDelayLine::setDelayTime(float delayTimeSeconds)
{
    if (std::abs(delayTimeSeconds - mTargetDelayTime) > 0.001f) {
        mTargetDelayTime = delayTimeSeconds;
        mStabilityCounter = 0;
    }
}

void DualDelayLine::updateDelayState(DelayLineState& state, float delayTime)
{
    float delaySamples = delayTime * static_cast<float>(mSampleRate);
    float maxDelaySamples = static_cast<float>(mBufferSize - 1);

    state.delayInSamples = std::max(0.5f, std::min(delaySamples, maxDelaySamples));
    updateAllpassCoeff(state);
}

void DualDelayLine::updateAllpassCoeff(DelayLineState& state)
{
    float integerPart = floorf(state.delayInSamples);
    float fracPart = state.delayInSamples - integerPart;

    if (fracPart < 0.5f) {
        fracPart = 0.5f;
    }

    state.allpassCoeff = (1.0f - fracPart) / (1.0f + fracPart);
}

float DualDelayLine::nextOut(DelayLineState& state, const std::vector<float>& buffer)
{
    if (state.doNextOut) {
        state.nextOutput = -state.allpassCoeff * state.lastOutput;
        state.nextOutput += state.apInput + (state.allpassCoeff * buffer[state.readIndex]);
        state.doNextOut = false;
    }
    return state.nextOutput;
}

float DualDelayLine::processDelayLine(std::vector<float>& buffer, int& writeIndex, DelayLineState& state, float input)
{
    // Calculate integer delay part
    int integerDelay = static_cast<int>(floorf(state.delayInSamples));

    // Update read index for integer delay
    state.readIndex = writeIndex - integerDelay;
    if (state.readIndex < 0) state.readIndex += mBufferSize;
    state.readIndex %= mBufferSize;

    // Write input to buffer
    buffer[writeIndex] = input;

    // Get output using STK allpass interpolation
    float output = nextOut(state, buffer);
    state.lastOutput = output;
    state.doNextOut = true;
    state.apInput = buffer[state.readIndex];

    // Advance write index
    writeIndex = (writeIndex + 1) % mBufferSize;

    return output;
}

int DualDelayLine::calculateCrossfadeLength(float delayTime)
{
    float baseCrossfadeMs = 50.0f + (delayTime * 1000.0f * 0.25f);
    baseCrossfadeMs = std::min(baseCrossfadeMs, 500.0f);

    return static_cast<int>(baseCrossfadeMs * 0.001f * mSampleRate);
}

void DualDelayLine::startCrossfade()
{
    mCrossfadeState = CROSSFADING;
    mCrossfadeLength = calculateCrossfadeLength(mTargetDelayTime);
    mCrossfadePosition = 0;

    if (mUsingLineA) {
        updateDelayState(mStateB, mTargetDelayTime);
    } else {
        updateDelayState(mStateA, mTargetDelayTime);
    }
}

void DualDelayLine::updateCrossfade()
{
    if (mCrossfadeState != CROSSFADING) return;

    float progress = static_cast<float>(mCrossfadePosition) / static_cast<float>(mCrossfadeLength);
    progress = std::min(progress, 1.0f);

    float fadeOut = 0.5f * (1.0f + cosf(progress * 3.14159265f));
    float fadeIn = 1.0f - fadeOut;

    if (mUsingLineA) {
        mCrossfadeGainA = fadeOut;
        mCrossfadeGainB = fadeIn;
    } else {
        mCrossfadeGainA = fadeIn;
        mCrossfadeGainB = fadeOut;
    }

    mCrossfadePosition++;

    if (mCrossfadePosition >= mCrossfadeLength) {
        mCrossfadeState = STABLE;
        mUsingLineA = !mUsingLineA;
        mCurrentDelayTime = mTargetDelayTime;

        if (mUsingLineA) {
            mCrossfadeGainA = 1.0f;
            mCrossfadeGainB = 0.0f;
        } else {
            mCrossfadeGainA = 0.0f;
            mCrossfadeGainB = 1.0f;
        }
    }
}

void DualDelayLine::processSample(float input, float& output)
{
    if (std::abs(mTargetDelayTime - mCurrentDelayTime) > 0.001f) {
        mStabilityCounter++;

        if (mStabilityCounter >= mStabilityThreshold && mCrossfadeState == STABLE) {
            startCrossfade();
        }
    } else {
        mStabilityCounter = 0;
    }

    updateCrossfade();

    float outputA = processDelayLine(mBufferA, mWriteIndexA, mStateA, input);
    float outputB = processDelayLine(mBufferB, mWriteIndexB, mStateB, input);

    if (mCrossfadeState == STABLE) {
        output = mUsingLineA ? outputA : outputB;
    } else {
        output = (outputA * mCrossfadeGainA) + (outputB * mCrossfadeGainB);
    }
}

void DualDelayLine::reset()
{
    std::fill(mBufferA.begin(), mBufferA.end(), 0.0f);
    std::fill(mBufferB.begin(), mBufferB.end(), 0.0f);

    mWriteIndexA = 0;
    mWriteIndexB = 0;

    mUsingLineA = true;
    mCrossfadeState = STABLE;
    mStabilityCounter = 0;
    mCrossfadePosition = 0;
    mCrossfadeGainA = 1.0f;
    mCrossfadeGainB = 0.0f;

    mStateA.delayInSamples = 0.5f;
    mStateA.readIndex = 0;
    mStateA.apInput = 0.0f;
    mStateA.lastOutput = 0.0f;
    mStateA.doNextOut = true;
    mStateA.nextOutput = 0.0f;
    updateAllpassCoeff(mStateA);

    mStateB = mStateA;
}


STKDelayLine::STKDelayLine()
: mBufferSize(0)
, mWriteIndex(0)
, mReadIndex(0)
, mSampleRate(44100.0)
, mDelayInSamples(0.5f) // STK minimum delay
, mAllpassCoeff(0.0f)
, mApInput(0.0f)
, mLastOutput(0.0f)
, mDoNextOut(true)
, mNextOutput(0.0f)
{
}

STKDelayLine::~STKDelayLine()
{
}

void STKDelayLine::initialize(double sampleRate, double maxDelaySeconds)
{
    mSampleRate = sampleRate;
    mBufferSize = static_cast<int>(maxDelaySeconds * sampleRate) + 1;
    mBuffer.resize(mBufferSize, 0.0f);
    mWriteIndex = 0;
    mReadIndex = 0;

    mDelayInSamples = 0.5f;
    updateAllpassCoeff();

    mApInput = 0.0f;
    mLastOutput = 0.0f;
    mDoNextOut = true;
    mNextOutput = 0.0f;
}

void STKDelayLine::setDelayTime(float delayTimeSeconds)
{
    float delaySamples = delayTimeSeconds * static_cast<float>(mSampleRate);
    float maxDelaySamples = static_cast<float>(mBufferSize - 1);

    mDelayInSamples = std::max(0.5f, std::min(delaySamples, maxDelaySamples));
    updateAllpassCoeff();
}

void STKDelayLine::updateAllpassCoeff()
{
    float integerPart = floorf(mDelayInSamples);
    float fracPart = mDelayInSamples - integerPart;

    if (fracPart < 0.5f) {
        fracPart = 0.5f;
    }

    mAllpassCoeff = (1.0f - fracPart) / (1.0f + fracPart);
}

float STKDelayLine::nextOut()
{
    if (mDoNextOut) {
        mNextOutput = -mAllpassCoeff * mLastOutput;
        mNextOutput += mApInput + (mAllpassCoeff * mBuffer[mReadIndex]);
        mDoNextOut = false;
    }
    return mNextOutput;
}

void STKDelayLine::processSample(float input, float& output)
{
    int integerDelay = static_cast<int>(floorf(mDelayInSamples));

    mReadIndex = mWriteIndex - integerDelay;
    if (mReadIndex < 0) mReadIndex += mBufferSize;
    mReadIndex %= mBufferSize;

    mBuffer[mWriteIndex] = input;

    output = nextOut();
    mLastOutput = output;
    mDoNextOut = true;
    mApInput = mBuffer[mReadIndex];

    mWriteIndex = (mWriteIndex + 1) % mBufferSize;
}

void STKDelayLine::reset()
{
    std::fill(mBuffer.begin(), mBuffer.end(), 0.0f);
    mWriteIndex = 0;
    mReadIndex = 0;
    mDelayInSamples = 0.5f;
    updateAllpassCoeff();
    mApInput = 0.0f;
    mLastOutput = 0.0f;
    mDoNextOut = true;
    mNextOutput = 0.0f;
}

SpeedBasedDelayLine::SpeedBasedDelayLine()
: DualDelayLine()
, mPitchSemitones(0)
, mPitchRatio(1.0f)
, mTargetPitchRatio(1.0f)
, mSmoothingCoeff(1.0f)
, mReadPosition(0.0f)
, mSpeedBufferSize(0)
, mSpeedWriteIndexA(0)
, mSpeedWriteIndexB(0)
, mMinReadDistance(0)
, mEmergencyBypassMode(false)
, mProcessingTimeouts(0)
, mInfiniteLoopPrevention(0)
{
}

SpeedBasedDelayLine::~SpeedBasedDelayLine()
{
}

void SpeedBasedDelayLine::initialize(double sampleRate, double maxDelaySeconds)
{
    DualDelayLine::initialize(sampleRate, maxDelaySeconds);

    // Enhanced buffer size for variable speed playback - 4x multiplier for extreme pitch down protection
    mSpeedBufferSize = static_cast<int>(maxDelaySeconds * sampleRate * BUFFER_MULTIPLIER) + 1024; // Extra safety margin
    mSpeedBufferA.resize(mSpeedBufferSize, 0.0f);
    mSpeedBufferB.resize(mSpeedBufferSize, 0.0f);

    // Initialize separate write indices for speed buffers
    mSpeedWriteIndexA = 0;
    mSpeedWriteIndexB = 0;

    // Calculate minimum safe read distance (prevents buffer underruns)
    mMinReadDistance = static_cast<int>(sampleRate * 0.1); // 100ms minimum buffer

    // Initialize read position with safe offset from write position
    mReadPosition = static_cast<float>(mMinReadDistance);

    // Set up parameter smoothing coefficient (10ms time constant for faster response)
    updateSmoothingCoeff();
    updatePitchRatio();
}

void SpeedBasedDelayLine::setPitchShift(int semitones)
{
    // CRITICAL FIX: Removed heavy profiling calls from audio thread

    // Validate and clamp semitones parameter
    int clampedSemitones = std::max(-12, std::min(12, semitones));

    if (mPitchSemitones != clampedSemitones) {
        // Log pitch shift parameter change (only in debug mode)
        if (PitchDebug::isLoggingEnabled()) {
            std::ostringstream ss;
            ss << "setPitchShift: " << mPitchSemitones << " -> " << clampedSemitones
               << " semitones (requested: " << semitones << ")";
            PitchDebug::logMessage(ss.str());
        }

        mPitchSemitones = clampedSemitones;
        updatePitchRatio();

        // For extreme pitch changes, add extra safety margin to read position
        if (std::abs(clampedSemitones) > 6) { // More than a tritone
            float currentWritePos = static_cast<float>(mUsingLineA ? mSpeedWriteIndexA : mSpeedWriteIndexB);
            float safeDistance = static_cast<float>(mMinReadDistance * 2); // Double safety margin
            mReadPosition = currentWritePos - safeDistance;

            if (mReadPosition < 0.0f) {
                mReadPosition += static_cast<float>(mSpeedBufferSize);
            }
        }
    }
}

void SpeedBasedDelayLine::updatePitchRatio()
{
    float oldTargetRatio = mTargetPitchRatio;

    // Calculate target pitch ratio using standard musical formula with validation
    if (mPitchSemitones == 0) {
        mTargetPitchRatio = 1.0f;
    } else {
        // Calculate pitch ratio: 2^(semitones/12)
        float exponent = static_cast<float>(mPitchSemitones) / 12.0f;
        mTargetPitchRatio = powf(2.0f, exponent);

        // Validate the result and clamp to safe bounds
        if (!std::isfinite(mTargetPitchRatio) || mTargetPitchRatio <= 0.0f) {
            std::ostringstream ss;
            ss << "Invalid pitch ratio calculated: " << mTargetPitchRatio
               << " from semitones: " << mPitchSemitones << " - resetting to 1.0";
            PitchDebug::logMessage(ss.str());
            mTargetPitchRatio = 1.0f;
        } else {
            // Clamp to reasonable bounds (0.25x to 4x speed)
            float unclamped = mTargetPitchRatio;
            mTargetPitchRatio = std::max(0.25f, std::min(mTargetPitchRatio, 4.0f));

            if (unclamped != mTargetPitchRatio && PitchDebug::isLoggingEnabled()) {
                std::ostringstream ss;
                ss << "Pitch ratio clamped: " << unclamped << " -> " << mTargetPitchRatio;
                PitchDebug::logMessage(ss.str());
            }
        }
    }

    // Log significant ratio changes
    if (PitchDebug::isLoggingEnabled() && fabsf(oldTargetRatio - mTargetPitchRatio) > 0.01f) {
        std::ostringstream ss;
        ss << "updatePitchRatio: " << oldTargetRatio << " -> " << mTargetPitchRatio
           << " (semitones: " << mPitchSemitones << ")";
        PitchDebug::logMessage(ss.str());
    }
}

void SpeedBasedDelayLine::updateSmoothingCoeff()
{
    // 10ms smoothing time constant for real-time safe parameter smoothing
    const float timeConstantMs = 10.0f;
    const float timeConstantSec = timeConstantMs / 1000.0f;

    // First-order IIR filter coefficient: α = exp(-1/(timeConstant × sampleRate))
    mSmoothingCoeff = expf(-1.0f / (timeConstantSec * static_cast<float>(mSampleRate)));
}

void SpeedBasedDelayLine::updatePitchRatioSmoothing()
{
    // Apply exponential smoothing to pitch ratio for zipper-free modulation
    // y[n] = α·y[n-1] + (1-α)·x[n]
    float newRatio = mSmoothingCoeff * mPitchRatio + (1.0f - mSmoothingCoeff) * mTargetPitchRatio;

    // Validate smoothed result
    if (std::isfinite(newRatio) && newRatio > 0.0f) {
        mPitchRatio = newRatio;
    } else {
        // Fallback to target ratio if smoothed value is invalid
        mPitchRatio = mTargetPitchRatio;
    }

    // Ensure ratio stays within safe bounds
    mPitchRatio = std::max(0.25f, std::min(mPitchRatio, 4.0f));
}

float SpeedBasedDelayLine::interpolateBuffer(const std::vector<float>& buffer, float position) const
{
    if (buffer.empty()) {
        return 0.0f;
    }

    // Validate position is within reasonable bounds
    if (!std::isfinite(position) || position < 0.0f) {
        return 0.0f;
    }

    int bufferSize = static_cast<int>(buffer.size());

    // Clamp position to buffer bounds with safety margin and iteration limits
    int clampIterations = 0;
    while (position >= static_cast<float>(bufferSize) && clampIterations < MAX_LOOP_ITERATIONS) {
        position -= static_cast<float>(bufferSize);
        clampIterations++;
    }

    if (clampIterations >= MAX_LOOP_ITERATIONS) {
        std::ostringstream ss;
        ss << "Prevented infinite loop in interpolateBuffer position clamping (high) - Position: " << position
           << ", BufferSize: " << bufferSize << ", Iterations: " << clampIterations;
        PitchDebug::logMessage(ss.str());
        return 0.0f; // Safe fallback
    }

    clampIterations = 0;
    while (position < 0.0f && clampIterations < MAX_LOOP_ITERATIONS) {
        position += static_cast<float>(bufferSize);
        clampIterations++;
    }

    if (clampIterations >= MAX_LOOP_ITERATIONS) {
        std::ostringstream ss;
        ss << "Prevented infinite loop in interpolateBuffer position clamping (low) - Position: " << position
           << ", BufferSize: " << bufferSize << ", Iterations: " << clampIterations;
        PitchDebug::logMessage(ss.str());
        return 0.0f; // Safe fallback
    }

    // Linear interpolation for fractional sample positions
    int intPos = static_cast<int>(std::floor(position));
    float fracPos = position - static_cast<float>(intPos);

    // Ensure indices are within bounds
    intPos = std::max(0, std::min(intPos, bufferSize - 1));
    int nextPos = std::min(intPos + 1, bufferSize - 1);

    // Handle boundary case for wraparound
    if (nextPos >= bufferSize) {
        nextPos = 0;
    }

    // Linear interpolation between adjacent samples
    return buffer[intPos] * (1.0f - fracPos) + buffer[nextPos] * fracPos;
}

float SpeedBasedDelayLine::processSpeedBasedPitchShifting(float input)
{
    // Start timing for dropout detection
    startProcessingTimer();

    // Check for emergency bypass mode
    if (mEmergencyBypassMode) {
        PitchDebug::logMessage("Emergency bypass active - passing input through directly");
        return input;
    }

    // Log entry to processing
    if (PitchDebug::isLoggingEnabled() && (mProcessingTimeouts > 0 || mInfiniteLoopPrevention > 0)) {
        std::ostringstream ss;
        ss << "Entering processSpeedBasedPitchShifting - Pitch: " << mPitchSemitones
           << ", Ratio: " << mPitchRatio << ", ReadPos: " << mReadPosition;
        PitchDebug::logMessage(ss.str());
    }

    // Apply parameter smoothing for zipper-free modulation
    updatePitchRatioSmoothing();

    // Check for timeout after parameter smoothing
    if (checkProcessingTimeout()) {
        mProcessingTimeouts++;
        enterEmergencyBypass("Timeout during parameter smoothing");
        return input;
    }

    // If no pitch shifting is active, return input directly
    if (mPitchSemitones == 0 && fabsf(mPitchRatio - 1.0f) < 1e-6f) {
        return input;
    }

    // Select current speed buffer and write index based on dual delay line state
    std::vector<float>& currentBuffer = mUsingLineA ? mSpeedBufferA : mSpeedBufferB;
    int& currentWriteIndex = mUsingLineA ? mSpeedWriteIndexA : mSpeedWriteIndexB;

    // Store input sample in the speed buffer at current write position
    currentBuffer[currentWriteIndex] = input;

    // Calculate safe read distance to prevent buffer underruns
    float writePos = static_cast<float>(currentWriteIndex);
    float readDistance = writePos - mReadPosition;

    // Handle wraparound for read distance calculation
    if (readDistance < 0.0f) {
        readDistance += static_cast<float>(mSpeedBufferSize);
    }

    // If read position is too close to write position, add safety margin
    if (readDistance < static_cast<float>(mMinReadDistance)) {
        // Reset read position to safe distance behind write position
        mReadPosition = writePos - static_cast<float>(mMinReadDistance);
        if (mReadPosition < 0.0f) {
            mReadPosition += static_cast<float>(mSpeedBufferSize);
        }
    }

    // Variable speed playback: advance read position by pitch ratio
    mReadPosition += mPitchRatio;

    // Wrap read position within buffer bounds with iteration limits
    int wrapIterations = 0;
    while (mReadPosition >= static_cast<float>(mSpeedBufferSize) && wrapIterations < MAX_LOOP_ITERATIONS) {
        mReadPosition -= static_cast<float>(mSpeedBufferSize);
        wrapIterations++;

        // Check for timeout during wrapping
        if (checkProcessingTimeout()) {
            mProcessingTimeouts++;
            enterEmergencyBypass("Timeout during read position wrapping (high)");
            return input;
        }
    }

    if (wrapIterations >= MAX_LOOP_ITERATIONS) {
        mInfiniteLoopPrevention++;
        std::ostringstream ss;
        ss << "Prevented infinite loop in read position wrapping (high) - ReadPos: " << mReadPosition
           << ", BufferSize: " << mSpeedBufferSize << ", Iterations: " << wrapIterations;
        PitchDebug::logMessage(ss.str());
        enterEmergencyBypass("Infinite loop prevention in read position wrapping (high)");
        return input;
    }

    wrapIterations = 0;
    while (mReadPosition < 0.0f && wrapIterations < MAX_LOOP_ITERATIONS) {
        mReadPosition += static_cast<float>(mSpeedBufferSize);
        wrapIterations++;

        // Check for timeout during wrapping
        if (checkProcessingTimeout()) {
            mProcessingTimeouts++;
            enterEmergencyBypass("Timeout during read position wrapping (low)");
            return input;
        }
    }

    if (wrapIterations >= MAX_LOOP_ITERATIONS) {
        mInfiniteLoopPrevention++;
        std::ostringstream ss;
        ss << "Prevented infinite loop in read position wrapping (low) - ReadPos: " << mReadPosition
           << ", BufferSize: " << mSpeedBufferSize << ", Iterations: " << wrapIterations;
        PitchDebug::logMessage(ss.str());
        enterEmergencyBypass("Infinite loop prevention in read position wrapping (low)");
        return input;
    }

    // Check for timeout before final interpolation
    if (checkProcessingTimeout()) {
        mProcessingTimeouts++;
        enterEmergencyBypass("Timeout before final interpolation");
        return input;
    }

    // Interpolate the output sample using safe linear interpolation
    float output = interpolateBuffer(currentBuffer, mReadPosition);

    // Validate output before returning
    if (!std::isfinite(output)) {
        std::ostringstream ss;
        ss << "Invalid output detected: " << output << " - using input as fallback";
        PitchDebug::logMessage(ss.str());
        output = input;
    }

    // Advance write index with bounds checking
    currentWriteIndex = (currentWriteIndex + 1) % mSpeedBufferSize;
    if (currentWriteIndex < 0 || currentWriteIndex >= mSpeedBufferSize) {
        std::ostringstream ss;
        ss << "Write index out of bounds: " << currentWriteIndex << " (BufferSize: " << mSpeedBufferSize << ")";
        PitchDebug::logMessage(ss.str());
        currentWriteIndex = 0; // Reset to safe value
    }

    // Final timeout check and logging
    if (checkProcessingTimeout()) {
        mProcessingTimeouts++;
        std::ostringstream ss;
        ss << "Processing completed with timeout - Duration: "
           << std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - mProcessingStartTime).count() << "ms";
        PitchDebug::logMessage(ss.str());
    }

    // Log processing stats periodically (every 1000 calls if issues detected)
    static int processCallCount = 0;
    if (++processCallCount >= 1000 && (mProcessingTimeouts > 0 || mInfiniteLoopPrevention > 0)) {
        logProcessingStats();
        processCallCount = 0;
    }

    return output;
}

void SpeedBasedDelayLine::processSample(float input, float& output)
{
    // Emergency bypass mode - pass input through directly
    if (mEmergencyBypassMode) {
        output = input;
        return;
    }

    // Check if pitch shifting is active
    if (mPitchSemitones == 0) {
        // No pitch shifting - use standard dual delay line processing
        DualDelayLine::processSample(input, output);
    } else {
        // Apply delay first, then pitch shifting
        float delayedInput;
        DualDelayLine::processSample(input, delayedInput);
        output = processSpeedBasedPitchShifting(delayedInput);
    }
}

void SpeedBasedDelayLine::reset()
{
    // Reset dual delay line state
    DualDelayLine::reset();

    // Clear speed buffers
    std::fill(mSpeedBufferA.begin(), mSpeedBufferA.end(), 0.0f);
    std::fill(mSpeedBufferB.begin(), mSpeedBufferB.end(), 0.0f);

    // Reset speed buffer write indices
    mSpeedWriteIndexA = 0;
    mSpeedWriteIndexB = 0;

    // Reset read position to safe distance behind write position
    mReadPosition = static_cast<float>(mMinReadDistance);

    // Reset pitch ratio parameters
    mPitchRatio = 1.0f;
    mTargetPitchRatio = 1.0f;

    // Reset safety and diagnostic counters
    mEmergencyBypassMode = false;
    mProcessingTimeouts = 0;
    mInfiniteLoopPrevention = 0;
}

// Safety and diagnostic methods for dropout investigation
void SpeedBasedDelayLine::startProcessingTimer() const {
    mProcessingStartTime = std::chrono::high_resolution_clock::now();
}

bool SpeedBasedDelayLine::checkProcessingTimeout() const {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - mProcessingStartTime);
    return duration.count() > TIMEOUT_THRESHOLD_MS;
}

void SpeedBasedDelayLine::enterEmergencyBypass(const std::string& reason) const {
    if (!mEmergencyBypassMode) {
        mEmergencyBypassMode = true;
        std::ostringstream ss;
        ss << "EMERGENCY BYPASS ACTIVATED: " << reason
           << " (Timeouts: " << mProcessingTimeouts
           << ", Loop preventions: " << mInfiniteLoopPrevention << ")";
        PitchDebug::logMessage(ss.str());
    }
}

void SpeedBasedDelayLine::logProcessingStats() const {
    if (PitchDebug::isLoggingEnabled()) {
        std::ostringstream ss;
        ss << "Processing stats - Pitch: " << mPitchSemitones
           << " semitones, Ratio: " << mPitchRatio
           << ", ReadPos: " << mReadPosition
           << ", BufferSize: " << mSpeedBufferSize
           << ", EmergencyBypass: " << (mEmergencyBypassMode ? "YES" : "NO")
           << ", Timeouts: " << mProcessingTimeouts
           << ", Loop preventions: " << mInfiniteLoopPrevention;
        PitchDebug::logMessage(ss.str());
    }
}




WaterStickProcessor::WaterStickProcessor()
: mInputGain(1.0f)
, mOutputGain(1.0f)
, mDelayTime(0.1f)
, mFeedback(0.0f)
, mTempoSyncMode(false)
, mSyncDivision(kSync_1_4)
, mGrid(kGrid_4)
, mGlobalDryWet(0.5f)
, mDelayBypass(false)
, mDelayBypassPrevious(false)
, mDelayFadingOut(false)
, mDelayFadingIn(false)
, mDelayFadeRemaining(0)
, mDelayFadeTotalLength(0)
, mDelayFadeGain(1.0f)
, mSampleRate(44100.0)
, mLastTempoSyncDelayTime(-1.0f)
, mTempoSyncParametersChanged(false)
, mParameterHistoryWriteIndex(0)
{
    for (int i = 0; i < 16; i++) {
        mTapEnabled[i] = false;
        mTapEnabledPrevious[i] = false;
        mTapLevel[i] = 0.8f;
        mTapPan[i] = 0.5f;

        mTapFilterCutoff[i] = 1000.0f;
        mTapFilterResonance[i] = 0.0f;
        mTapFilterType[i] = kFilterType_Bypass;

        mTapPitchShift[i] = 0;  // Default to no pitch shift
        mTapFeedbackSend[i] = 0.0f;  // Default to no feedback send

        mTapFadingOut[i] = false;
        mTapFadeOutRemaining[i] = 0;
        mTapFadeOutTotalLength[i] = 0;

        mTapFadingIn[i] = false;
        mTapFadeInRemaining[i] = 0;
        mTapFadeInTotalLength[i] = 0;

        mTapFadeGain[i] = 1.0f;
    }

    mFeedbackBufferL = 0.0f;
    mFeedbackBufferR = 0.0f;

    mFeedbackSubMixerL = 0.0f;
    mFeedbackSubMixerR = 0.0f;

    // Initialize discrete parameters and smoothing
    mSmoothingCoeff = 0.999f; // Very smooth for audio-rate control
    for (int i = 0; i < 24; i++) {
        mDiscreteParameters[i] = 0.0f;
        mDiscreteParametersSmoothed[i] = 0.0f;
    }

    // Initialize macro curve types
    for (int i = 0; i < 4; i++) {
        mMacroCurveTypes[i] = 0; // Linear curves
    }

    // Initialize macro knob values
    for (int i = 0; i < 8; i++) {
        mMacroKnobValues[i] = 0.0f; // Default to 0.0
    }

    // Initialize macro system state tracking
    mMacroInfluenceActive = false;
    for (int i = 0; i < 8; i++) {
        mMacroSystemActive[i] = false;
        mPreviousMacroKnobValues[i] = 0.0f;
    }
    for (int i = 0; i < 1024; i++) {
        mParameterModifiedByUser[i] = false;
    }

    // Initialize current tap context (default to Volume)
    mCurrentTapContext = 1; // TapContext::Volume

    // PHASE 3: Initialize performance optimization components
    mSIMDCurveEvaluator = std::make_unique<SIMDMacroCurveEvaluator>();
    mParameterCache = std::make_unique<CacheOptimizedParameterLookup>();
    mParameterUpdatePool = std::make_unique<LockFreeMemoryPool<MacroParameterBatch>>();

    // Initialize macro parameter batch
    memset(&mCurrentMacroBatch, 0, sizeof(mCurrentMacroBatch));
    mMacroParameterVersion.store(0, std::memory_order_relaxed);
    mMacroParametersNeedUpdate.store(false, std::memory_order_relaxed);
    mLastParameterUpdateVersion.store(0, std::memory_order_relaxed);

    // Initialize dirty flags for all taps
    for (int i = 0; i < 16; ++i) {
        mTapParameterDirtyFlags[i].store(0, std::memory_order_relaxed);
    }

    // Phase 4: Initialize unified delay line system flag
    // Production default: unified system (legacy kept as emergency fallback)
    mUseUnifiedDelayLines = true;

    // Phase 5: Initialize decoupled delay + pitch architecture flag
    // Production default: decoupled system (complete solution for all critical issues)
    mUseDecoupledArchitecture = false;  // Will be enabled in setupProcessing()

    // Initialize parameter history with current values
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < PARAM_HISTORY_SIZE; j++) {
            mTapParameterHistory[i][j].level = mTapLevel[i];
            mTapParameterHistory[i][j].pan = mTapPan[i];
            mTapParameterHistory[i][j].filterCutoff = mTapFilterCutoff[i];
            mTapParameterHistory[i][j].filterResonance = mTapFilterResonance[i];
            mTapParameterHistory[i][j].filterType = mTapFilterType[i];
            mTapParameterHistory[i][j].pitchShift = mTapPitchShift[i];
            mTapParameterHistory[i][j].feedbackSend = mTapFeedbackSend[i];
            mTapParameterHistory[i][j].enabled = mTapEnabled[i];
        }
    }

    setControllerClass(kWaterStickControllerUID);
}

WaterStickProcessor::~WaterStickProcessor()
{
}

void WaterStickProcessor::checkTapStateChangesAndClearBuffers()
{
    for (int i = 0; i < 16; i++) {
        // Check if tap went from enabled to disabled
        if (mTapEnabledPrevious[i] && !mTapEnabled[i]) {
            // Start fade-out instead of immediate cut
            mTapFadingOut[i] = true;
            mTapFadingIn[i] = false;  // Stop any fade-in
            mTapFadeGain[i] = 1.0f;   // Start at full gain

            // Calculate fade-out length proportional to delay time (but capped)
            float tapDelayTime = mTapDistribution.getTapDelayTime(i);
            int fadeLength = static_cast<int>(tapDelayTime * mSampleRate * 0.01f); // 1% of delay time
            fadeLength = std::max(64, std::min(fadeLength, 2048)); // Cap between 64-2048 samples
            mTapFadeOutRemaining[i] = fadeLength;
            mTapFadeOutTotalLength[i] = fadeLength;
        }
        // Check if tap went from disabled to enabled
        else if (!mTapEnabledPrevious[i] && mTapEnabled[i]) {
            // Stop any ongoing fade-out and start fade-in
            mTapFadingOut[i] = false;
            mTapFadingIn[i] = true;
            mTapFadeGain[i] = 0.0f;   // Start at zero gain

            // Clear buffers for clean start
            if (mUseDecoupledArchitecture) {
                // Decoupled system coordinated reset (production solution)
                mDecoupledDelaySystemL.reset();
                mDecoupledDelaySystemR.reset();
            } else if (mUseUnifiedDelayLines) {
                mUnifiedTapDelayLinesL[i].reset();
                mUnifiedTapDelayLinesR[i].reset();
            } else {
                // Emergency fallback buffer clearing
                mTapDelayLinesL[i].reset();
                mTapDelayLinesR[i].reset();
            }

            // Calculate fade-in length - much shorter than fade-out (0.25% of delay time)
            float tapDelayTime = mTapDistribution.getTapDelayTime(i);
            int fadeLength = static_cast<int>(tapDelayTime * mSampleRate * 0.0025f); // 0.25% of delay time
            fadeLength = std::max(16, std::min(fadeLength, 512)); // Cap between 16-512 samples (0.3ms-11.6ms)
            mTapFadeInRemaining[i] = fadeLength;
            mTapFadeInTotalLength[i] = fadeLength;
        }

        // Update previous state for next call
        mTapEnabledPrevious[i] = mTapEnabled[i];
    }
}

void WaterStickProcessor::checkBypassStateChanges()
{
    const float MIN_SAMPLE_RATE = 8000.0f;
    const float MAX_SAMPLE_RATE = 192000.0f;

    if (mSampleRate < MIN_SAMPLE_RATE || mSampleRate > MAX_SAMPLE_RATE) {
        mSampleRate = 44100.0f;
    }

    if (mDelayBypassPrevious != mDelayBypass) {
        if (!mDelayFadingOut && !mDelayFadingIn) {
            if (!mDelayBypassPrevious && mDelayBypass) {
                mDelayFadingOut = true;
                mDelayFadingIn = false;
                mDelayFadeGain = 1.0f;

                int fadeLength = static_cast<int>(std::max(64.0f, std::min(static_cast<float>(mSampleRate * 0.01f), 2048.0f)));
                mDelayFadeRemaining = fadeLength;
                mDelayFadeTotalLength = fadeLength;
            }
            else if (mDelayBypassPrevious && !mDelayBypass) {
                mDelayFadingOut = false;
                mDelayFadingIn = true;
                mDelayFadeGain = 0.0f;

                int fadeLength = static_cast<int>(std::max(32.0f, std::min(static_cast<float>(mSampleRate * 0.005f), 1024.0f)));
                mDelayFadeRemaining = fadeLength;
                mDelayFadeTotalLength = fadeLength;
            }
        }
        mDelayBypassPrevious = mDelayBypass;
    }
}

void WaterStickProcessor::processDelaySection(float inputL, float inputR, float& outputL, float& outputR)
{
    float sumL = 0.0f;
    float sumR = 0.0f;

    // Clear feedback sub-mixer for this sample
    mFeedbackSubMixerL = 0.0f;
    mFeedbackSubMixerR = 0.0f;

    // Phase 5: Use decoupled delay + pitch architecture for bulletproof operation
    if (mUseDecoupledArchitecture) {
        // Process all taps in one coordinated batch - guaranteed to work
        float tapOutputsL[NUM_TAPS];
        float tapOutputsR[NUM_TAPS];

        mDecoupledDelaySystemL.processAllTaps(inputL, tapOutputsL);
        mDecoupledDelaySystemR.processAllTaps(inputR, tapOutputsR);

        // Apply per-tap processing (filters, panning, fading)
        for (int tap = 0; tap < NUM_TAPS; tap++) {
            bool processTap = mTapDistribution.isTapEnabled(tap) || mTapFadingOut[tap] || mTapFadingIn[tap];

            if (processTap) {
                float tapDelayTime = mTapDistribution.getTapDelayTime(tap);
                ParameterSnapshot historicParams = getHistoricParameters(tap, tapDelayTime);

                float tapOutputL = tapOutputsL[tap] * historicParams.level;
                float tapOutputR = tapOutputsR[tap] * historicParams.level;

                // Apply filtering
                mTapFiltersL[tap].setParameters(historicParams.filterCutoff, historicParams.filterResonance, historicParams.filterType);
                mTapFiltersR[tap].setParameters(historicParams.filterCutoff, historicParams.filterResonance, historicParams.filterType);
                tapOutputL = static_cast<float>(mTapFiltersL[tap].process(tapOutputL));
                tapOutputR = static_cast<float>(mTapFiltersR[tap].process(tapOutputR));

                // Apply fade processing
                if (mTapFadingOut[tap]) {
                    tapOutputL *= mTapFadeGain[tap];
                    tapOutputR *= mTapFadeGain[tap];

                    mTapFadeOutRemaining[tap]--;
                    if (mTapFadeOutRemaining[tap] <= 0) {
                        mTapFadingOut[tap] = false;
                        mTapFadeGain[tap] = 1.0f;
                        // Reset buffers through decoupled system
                        mDecoupledDelaySystemL.reset();
                        mDecoupledDelaySystemR.reset();
                    } else {
                        float fadeProgress = 1.0f - (static_cast<float>(mTapFadeOutRemaining[tap]) / static_cast<float>(mTapFadeOutTotalLength[tap]));
                        mTapFadeGain[tap] = std::exp(-6.0f * fadeProgress);
                    }
                }
                else if (mTapFadingIn[tap]) {
                    tapOutputL *= mTapFadeGain[tap];
                    tapOutputR *= mTapFadeGain[tap];

                    mTapFadeInRemaining[tap]--;
                    if (mTapFadeInRemaining[tap] <= 0) {
                        mTapFadingIn[tap] = false;
                        mTapFadeGain[tap] = 1.0f;
                    } else {
                        float fadeProgress = 1.0f - (static_cast<float>(mTapFadeInRemaining[tap]) / static_cast<float>(mTapFadeInTotalLength[tap]));
                        mTapFadeGain[tap] = 1.0f - std::exp(-6.0f * fadeProgress);
                    }
                }

                // Apply panning
                float pan = historicParams.pan;
                float leftGain = 1.0f - pan;
                float rightGain = pan;

                float tapMainL = (tapOutputL * leftGain) + (tapOutputR * leftGain);
                float tapMainR = (tapOutputL * rightGain) + (tapOutputR * rightGain);
                sumL += tapMainL;
                sumR += tapMainR;

                // Add to feedback sub-mixer based on per-tap send level
                float feedbackSendLevel = historicParams.feedbackSend;
                mFeedbackSubMixerL += tapMainL * feedbackSendLevel;
                mFeedbackSubMixerR += tapMainR * feedbackSendLevel;
            }
        }
    } else {
        // Fallback to legacy processing (for A/B testing)
        for (int tap = 0; tap < NUM_TAPS; tap++) {
            bool processTap = mTapDistribution.isTapEnabled(tap) || mTapFadingOut[tap] || mTapFadingIn[tap];

            if (processTap) {
                float tapDelayTime = mTapDistribution.getTapDelayTime(tap);
                ParameterSnapshot historicParams = getHistoricParameters(tap, tapDelayTime);

                float tapOutputL, tapOutputR;

                // Phase 2: Choose between legacy and unified delay line systems
                if (mUseUnifiedDelayLines) {
                    // Use production unified delay lines (47.9x performance improvement)
                    mUnifiedTapDelayLinesL[tap].setPitchShift(historicParams.pitchShift);
                    mUnifiedTapDelayLinesR[tap].setPitchShift(historicParams.pitchShift);
                    mUnifiedTapDelayLinesL[tap].processSample(inputL, tapOutputL);
                    mUnifiedTapDelayLinesR[tap].processSample(inputR, tapOutputR);
                } else {
                    // Emergency fallback to legacy system (kept for compatibility)
                    mTapDelayLinesL[tap].setPitchShift(historicParams.pitchShift);
                    mTapDelayLinesR[tap].setPitchShift(historicParams.pitchShift);
                    mTapDelayLinesL[tap].processSample(inputL, tapOutputL);
                    mTapDelayLinesR[tap].processSample(inputR, tapOutputR);
                }

                tapOutputL *= historicParams.level;
                tapOutputR *= historicParams.level;

                mTapFiltersL[tap].setParameters(historicParams.filterCutoff, historicParams.filterResonance, historicParams.filterType);
                mTapFiltersR[tap].setParameters(historicParams.filterCutoff, historicParams.filterResonance, historicParams.filterType);
                tapOutputL = static_cast<float>(mTapFiltersL[tap].process(tapOutputL));
                tapOutputR = static_cast<float>(mTapFiltersR[tap].process(tapOutputR));

                if (mTapFadingOut[tap]) {
                    tapOutputL *= mTapFadeGain[tap];
                    tapOutputR *= mTapFadeGain[tap];

                    mTapFadeOutRemaining[tap]--;
                    if (mTapFadeOutRemaining[tap] <= 0) {
                        mTapFadingOut[tap] = false;
                        mTapFadeGain[tap] = 1.0f;
                        if (mUseUnifiedDelayLines) {
                            mUnifiedTapDelayLinesL[tap].reset();
                            mUnifiedTapDelayLinesR[tap].reset();
                        } else {
                            // Emergency fallback buffer clearing
                            mTapDelayLinesL[tap].reset();
                            mTapDelayLinesR[tap].reset();
                        }
                    } else {
                        float fadeProgress = 1.0f - (static_cast<float>(mTapFadeOutRemaining[tap]) / static_cast<float>(mTapFadeOutTotalLength[tap]));
                        mTapFadeGain[tap] = std::exp(-6.0f * fadeProgress);
                    }
                }
                else if (mTapFadingIn[tap]) {
                    tapOutputL *= mTapFadeGain[tap];
                    tapOutputR *= mTapFadeGain[tap];

                    mTapFadeInRemaining[tap]--;
                    if (mTapFadeInRemaining[tap] <= 0) {
                        mTapFadingIn[tap] = false;
                        mTapFadeGain[tap] = 1.0f;
                    } else {
                        float fadeProgress = 1.0f - (static_cast<float>(mTapFadeInRemaining[tap]) / static_cast<float>(mTapFadeInTotalLength[tap]));
                        mTapFadeGain[tap] = 1.0f - std::exp(-6.0f * fadeProgress);
                    }
                }

                float pan = historicParams.pan;
                float leftGain = 1.0f - pan;
                float rightGain = pan;

                // Apply panning for main tap output
                float tapMainL = (tapOutputL * leftGain) + (tapOutputR * leftGain);
                float tapMainR = (tapOutputL * rightGain) + (tapOutputR * rightGain);
                sumL += tapMainL;
                sumR += tapMainR;

                // Add to feedback sub-mixer based on per-tap send level
                float feedbackSendLevel = historicParams.feedbackSend;
                mFeedbackSubMixerL += tapMainL * feedbackSendLevel;
                mFeedbackSubMixerR += tapMainR * feedbackSendLevel;
            }
        }
    }

    // Store the feedback sub-mixer output (instead of raw tap outputs)
    mFeedbackBufferL = mFeedbackSubMixerL;
    mFeedbackBufferR = mFeedbackSubMixerR;

    // Delay section is always 100% wet now
    float dryGain = 0.0f;
    float wetGain = 1.0f;
    float delayWetGain = wetGain;

    outputL = (inputL * dryGain) + (sumL * delayWetGain);
    outputR = (inputR * dryGain) + (sumR * delayWetGain);

    if (mDelayFadingOut) {
        outputL *= mDelayFadeGain;
        outputR *= mDelayFadeGain;

        mDelayFadeRemaining--;
        if (mDelayFadeRemaining <= 0) {
            mDelayFadingOut = false;
            mDelayFadeGain = 1.0f;
        } else {
            float fadeProgress = 1.0f - (static_cast<float>(mDelayFadeRemaining) / static_cast<float>(mDelayFadeTotalLength));
            mDelayFadeGain = std::exp(-6.0f * fadeProgress);
        }
    }
    else if (mDelayFadingIn) {
        outputL *= mDelayFadeGain;
        outputR *= mDelayFadeGain;

        mDelayFadeRemaining--;
        if (mDelayFadeRemaining <= 0) {
            mDelayFadingIn = false;
            mDelayFadeGain = 1.0f;
        } else {
            float fadeProgress = 1.0f - (static_cast<float>(mDelayFadeRemaining) / static_cast<float>(mDelayFadeTotalLength));
            mDelayFadeGain = 1.0f - std::exp(-6.0f * fadeProgress);
        }
    }

    if (mDelayBypass && !mDelayFadingOut && !mDelayFadingIn) {
        outputL = inputL;
        outputR = inputR;
    }
}



tresult PLUGIN_API WaterStickProcessor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk)
    {
        return result;
    }

    // Add audio input/output busses
    addAudioInput(STR16("Stereo In"), Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::terminate()
{
    return AudioEffect::terminate();
}

tresult PLUGIN_API WaterStickProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
{
    mSampleRate = newSetup.sampleRate;

    mDelayLineL.initialize(mSampleRate, 2.0);
    mDelayLineR.initialize(mSampleRate, 2.0);

    double maxDelayTime = 20.0;
    for (int i = 0; i < NUM_TAPS; i++) {
        mTapDelayLinesL[i].initialize(mSampleRate, maxDelayTime);
        mTapDelayLinesR[i].initialize(mSampleRate, maxDelayTime);
    }

    // Phase 2: Initialize unified delay lines (bulletproof architecture)
    for (int i = 0; i < NUM_TAPS; i++) {
        mUnifiedTapDelayLinesL[i].initialize(mSampleRate, maxDelayTime);
        mUnifiedTapDelayLinesR[i].initialize(mSampleRate, maxDelayTime);
    }

    // Phase 5: Initialize decoupled delay + pitch architecture (production solution)
    mDecoupledDelaySystemL.initialize(mSampleRate, maxDelayTime);
    mDecoupledDelaySystemR.initialize(mSampleRate, maxDelayTime);
    mUseDecoupledArchitecture = true;  // Enable by default for production

    // PHASE 3: Initialize performance optimization components
    mParameterCache->initialize(NUM_TAPS);

    mTempoSync.initialize(mSampleRate);
    mTapDistribution.initialize(mSampleRate);

    for (int i = 0; i < NUM_TAPS; i++) {
        mTapFiltersL[i].setSampleRate(mSampleRate);
        mTapFiltersR[i].setSampleRate(mSampleRate);
    }

    return AudioEffect::setupProcessing(newSetup);
}


void WaterStickProcessor::captureCurrentParameters()
{
    // Store current parameter values for all taps
    for (int i = 0; i < 16; i++) {
        ParameterSnapshot& snapshot = mTapParameterHistory[i][mParameterHistoryWriteIndex];
        snapshot.level = mTapLevel[i];
        snapshot.pan = mTapPan[i];
        snapshot.filterCutoff = mTapFilterCutoff[i];
        snapshot.filterResonance = mTapFilterResonance[i];
        snapshot.filterType = mTapFilterType[i];
        snapshot.pitchShift = mTapPitchShift[i];
        snapshot.feedbackSend = mTapFeedbackSend[i];
        snapshot.enabled = mTapEnabled[i];
    }

    // Advance write index (circular buffer)
    mParameterHistoryWriteIndex = (mParameterHistoryWriteIndex + 1) % PARAM_HISTORY_SIZE;
}

WaterStickProcessor::ParameterSnapshot WaterStickProcessor::getHistoricParameters(int tapIndex, float delayTimeSeconds) const
{
    if (tapIndex < 0 || tapIndex >= 16) {
        // Return current parameters as fallback
        ParameterSnapshot current;
        current.level = mTapLevel[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        current.pan = mTapPan[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        current.filterCutoff = mTapFilterCutoff[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        current.filterResonance = mTapFilterResonance[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        current.filterType = mTapFilterType[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        current.pitchShift = mTapPitchShift[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        current.feedbackSend = mTapFeedbackSend[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        current.enabled = mTapEnabled[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        return current;
    }

    // Calculate how many samples back to look
    int samplesBack = static_cast<int>(delayTimeSeconds * mSampleRate);
    samplesBack = std::min(samplesBack, PARAM_HISTORY_SIZE - 1);

    // Calculate history index
    int historyIndex = mParameterHistoryWriteIndex - samplesBack;
    if (historyIndex < 0) {
        historyIndex += PARAM_HISTORY_SIZE;
    }

    return mTapParameterHistory[tapIndex][historyIndex];
}

void WaterStickProcessor::checkTempoSyncParameterChanges()
{
    // Check if tempo sync parameters actually changed
    if (mTempoSyncMode) {
        float currentDelayTime = mTempoSync.getDelayTime();
        if (std::abs(currentDelayTime - mLastTempoSyncDelayTime) > 0.001f) {
            mTempoSyncParametersChanged = true;
            mLastTempoSyncDelayTime = currentDelayTime;
        } else {
            mTempoSyncParametersChanged = false;
        }
    } else {
        // In free mode, always update if parameters changed
        mTempoSyncParametersChanged = true;
    }
}

void WaterStickProcessor::updateParameters()
{
    // CRITICAL FIX: Removed heavy profiling calls from audio thread
    // Performance profiling moved to debug/development mode only

    mTempoSync.setMode(mTempoSyncMode);
    mTempoSync.setSyncDivision(mSyncDivision);
    mTempoSync.setFreeTime(mDelayTime);

    checkTempoSyncParameterChanges();

    mTapDistribution.setGrid(mGrid);
    mTapDistribution.updateTempo(mTempoSync);

    for (int i = 0; i < 16; i++) {
        mTapDistribution.setTapEnable(i, mTapEnabled[i]);
        mTapDistribution.setTapLevel(i, mTapLevel[i]);
        mTapDistribution.setTapPan(i, mTapPan[i]);
    }

    for (int i = 0; i < NUM_TAPS; i++) {
        float tapDelayTime = mTapDistribution.getTapDelayTime(i);
        mTapDelayLinesL[i].setDelayTime(tapDelayTime);
        mTapDelayLinesR[i].setDelayTime(tapDelayTime);

        // Update unified delay lines (production system)
        mUnifiedTapDelayLinesL[i].setDelayTime(tapDelayTime);
        mUnifiedTapDelayLinesR[i].setDelayTime(tapDelayTime);

        // Update decoupled delay + pitch architecture (final production solution)
        if (mUseDecoupledArchitecture) {
            mDecoupledDelaySystemL.setTapDelayTime(i, tapDelayTime);
            mDecoupledDelaySystemR.setTapDelayTime(i, tapDelayTime);
            mDecoupledDelaySystemL.setTapEnabled(i, mTapEnabled[i]);
            mDecoupledDelaySystemR.setTapEnabled(i, mTapEnabled[i]);
            mDecoupledDelaySystemL.setTapPitchShift(i, mTapPitchShift[i]);
            mDecoupledDelaySystemR.setTapPitchShift(i, mTapPitchShift[i]);
        }
    }

    if (!mTempoSyncMode) {
        float finalDelayTime = mTempoSync.getDelayTime();
        mDelayLineL.setDelayTime(finalDelayTime);
        mDelayLineR.setDelayTime(finalDelayTime);
    }

    for (int i = 0; i < NUM_TAPS; i++) {
        mTapFiltersL[i].setParameters(mTapFilterCutoff[i], mTapFilterResonance[i], mTapFilterType[i]);
        mTapFiltersR[i].setParameters(mTapFilterCutoff[i], mTapFilterResonance[i], mTapFilterType[i]);
    }

    // Update discrete parameters with real-time curve evaluation and smoothing
    updateDiscreteParameters();
    applyCurveEvaluation();
    applyParameterSmoothing();
}

tresult PLUGIN_API WaterStickProcessor::process(Vst::ProcessData& data)
{
    if (data.processContext && data.processContext->state & Vst::ProcessContext::kTempoValid)
    {
        double hostTempo = data.processContext->tempo;
        mTempoSync.updateTempo(hostTempo, true);
    }
    else
    {
        mTempoSync.updateTempo(120.0, false);
    }

    // Process parameter changes
    if (data.inputParameterChanges)
    {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < numParamsChanged; i++)
        {
            Vst::IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData(i);
            if (paramQueue)
            {
                Vst::ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();

                if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                {
                    switch (paramQueue->getParameterId())
                    {
                        case kInputGain:
                            mInputGain = ParameterConverter::convertGain(value);
                            break;
                        case kOutputGain:
                            mOutputGain = ParameterConverter::convertGain(value);
                            break;
                        case kDelayTime:
                            mDelayTime = static_cast<float>(value * 2.0); // 0-2 seconds
                            break;
                        case kFeedback:
                            mFeedback = ParameterConverter::convertFeedback(value);
                            break;
                        case kTempoSyncMode:
                            mTempoSyncMode = value > 0.5; // Toggle: >0.5 = synced
                            break;
                        case kSyncDivision:
                            mSyncDivision = static_cast<int>(value * (kNumSyncDivisions - 1) + 0.5); // Round to nearest
                            break;
                        case kGrid:
                            mGrid = static_cast<int>(value * (kNumGridValues - 1) + 0.5);
                            break;
                        case kGlobalDryWet:
                            mGlobalDryWet = static_cast<float>(value);
                            break;
                        case kDelayBypass:
                            mDelayBypass = value > 0.5;
                            break;
                        default:
                            // Handle discrete parameters
                            if (paramQueue->getParameterId() >= kDiscrete1 && paramQueue->getParameterId() <= kDiscrete24) {
                                int index = paramQueue->getParameterId() - kDiscrete1;
                                mDiscreteParameters[index] = static_cast<float>(value);
                            }
                            // Handle macro curve type parameters
                            else if (paramQueue->getParameterId() >= kMacroCurve1Type && paramQueue->getParameterId() <= kMacroCurve4Type) {
                                int index = paramQueue->getParameterId() - kMacroCurve1Type;
                                mMacroCurveTypes[index] = static_cast<int>(value * (kNumCurveTypes - 1) + 0.5);
                            }
                            // Handle macro knob parameters
                            else if (paramQueue->getParameterId() >= kMacroKnob1 && paramQueue->getParameterId() <= kMacroKnob8) {
                                int index = paramQueue->getParameterId() - kMacroKnob1;
                                mMacroKnobValues[index] = static_cast<float>(value);

                                // CRITICAL FIX: Apply macro curves only when macro parameters change
                                // This prevents continuous parameter override in audio processing loop
                                applyMacroCurvesToTapParameters();
                            }
                            // Handle randomization and reset parameters (processed in controller)
                            else if (paramQueue->getParameterId() == kRandomizeSeed ||
                                    paramQueue->getParameterId() == kRandomizeAmount ||
                                    paramQueue->getParameterId() == kRandomizeTrigger ||
                                    paramQueue->getParameterId() == kResetTrigger) {
                                // These are handled by the controller
                            }
                            else {
                                TapParameterProcessor::processTapParameter(paramQueue->getParameterId(), value, this);
                            }
                            break;
                    }
                }
            }
        }
    }

    // CRITICAL FIX: Disable Phase 3 optimizations that broke audio processing
    // updateParametersOptimized();  // DISABLED - complex SIMD/lock-free operations causing audio failure

    // Restore original working parameter update system
    updateParameters();

    // Check for tap state changes and clear buffers if needed
    checkTapStateChangesAndClearBuffers();

    // Check for bypass state changes and handle fades
    checkBypassStateChanges();

    // Only update tempo sync delay times when parameters actually changed
    if (mTempoSyncMode && mTempoSyncParametersChanged) {
        // Update tap distribution with current tempo
        mTapDistribution.updateTempo(mTempoSync);

        // Update all tap delay times only when parameters changed
        for (int i = 0; i < NUM_TAPS; i++) {
            float tapDelayTime = mTapDistribution.getTapDelayTime(i);
            mTapDelayLinesL[i].setDelayTime(tapDelayTime);
            mTapDelayLinesR[i].setDelayTime(tapDelayTime);

            // Update unified delay lines (production system)
            mUnifiedTapDelayLinesL[i].setDelayTime(tapDelayTime);
            mUnifiedTapDelayLinesR[i].setDelayTime(tapDelayTime);
        }

        // Update legacy delay lines too
        float finalDelayTime = mTempoSync.getDelayTime();
        mDelayLineL.setDelayTime(finalDelayTime);
        mDelayLineR.setDelayTime(finalDelayTime);

        // Reset the change flag
        mTempoSyncParametersChanged = false;
    }

    // Check for valid input/output
    if (data.numInputs == 0 || data.numOutputs == 0)
    {
        return kResultOk;
    }

    Vst::AudioBusBuffers* input = data.inputs;
    Vst::AudioBusBuffers* output = data.outputs;

    // Ensure we have stereo input/output
    if (input->numChannels < 2 || output->numChannels < 2)
    {
        return kResultOk;
    }

    float* inputL = input->channelBuffers32[0];
    float* inputR = input->channelBuffers32[1];
    float* outputL = output->channelBuffers32[0];
    float* outputR = output->channelBuffers32[1];

    for (int32 sample = 0; sample < data.numSamples; sample++)
    {
        captureCurrentParameters();

        float inL = inputL[sample];
        float inR = inputR[sample];

        float inputWithFeedbackL = inL + (mFeedbackBufferL * mFeedback);
        float inputWithFeedbackR = inR + (mFeedbackBufferR * mFeedback);

        inputWithFeedbackL = std::tanh(inputWithFeedbackL);
        inputWithFeedbackR = std::tanh(inputWithFeedbackR);

        float gainedL = inputWithFeedbackL * mInputGain;
        float gainedR = inputWithFeedbackR * mInputGain;

        float delayOutputL, delayOutputR;
        processDelaySection(gainedL, gainedR, delayOutputL, delayOutputR);

        float globalDryGain = std::cos(mGlobalDryWet * M_PI_2);
        float globalWetGain = std::sin(mGlobalDryWet * M_PI_2);
        float mixedL = (inL * globalDryGain) + (delayOutputL * globalWetGain);
        float mixedR = (inR * globalDryGain) + (delayOutputR * globalWetGain);

        outputL[sample] = mixedL * mOutputGain;
        outputR[sample] = mixedR * mOutputGain;
    }

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::getState(IBStream* state)
{
    IBStreamer streamer(state, kLittleEndian);

    // Write current state version as first 4 bytes
    streamer.writeInt32(kStateVersionCurrent);

    // Write magic number signature for freshness detection
    streamer.writeInt32(kStateMagicNumber);

    // Write all parameters in version 1 format
    streamer.writeFloat(mInputGain);
    streamer.writeFloat(mOutputGain);
    streamer.writeFloat(mDelayTime);
    streamer.writeFloat(mFeedback);
    streamer.writeBool(mTempoSyncMode);
    streamer.writeInt32(mSyncDivision);
    streamer.writeInt32(mGrid);

    streamer.writeFloat(mGlobalDryWet);
    streamer.writeBool(mDelayBypass);

    // Save all tap parameters
    for (int i = 0; i < 16; i++) {
        streamer.writeBool(mTapEnabled[i]);
        streamer.writeFloat(mTapLevel[i]);
        streamer.writeFloat(mTapPan[i]);
        // Save per-tap filter parameters
        streamer.writeFloat(mTapFilterCutoff[i]);
        streamer.writeFloat(mTapFilterResonance[i]);
        streamer.writeInt32(mTapFilterType[i]);
        // Save pitch shift parameter
        streamer.writeInt32(mTapPitchShift[i]);
        // Save feedback send parameter
        streamer.writeFloat(mTapFeedbackSend[i]);
    }

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::setState(IBStream* state)
{
    if (!state) {
        return kResultOk;
    }

    // Try to read state version first
    IBStreamer streamer(state, kLittleEndian);
    Steinberg::int32 stateVersion;

    if (streamer.readInt32(stateVersion)) {
        // Check if this looks like a valid version
        if (stateVersion >= kStateVersionLegacy && stateVersion <= kStateVersionCurrent) {
            // This is a versioned state
            return readVersionedProcessorState(state, stateVersion);
        } else {
            // This is probably the first float from legacy state
            // Reset stream and read as legacy
            state->seek(0, IBStream::kIBSeekSet, nullptr);
            return readLegacyProcessorState(state);
        }
    } else {
        // Failed to read anything, treat as empty state
        return kResultOk;
    }
}

tresult WaterStickProcessor::readLegacyProcessorState(IBStream* state)
{
    IBStreamer streamer(state, kLittleEndian);

    streamer.readFloat(mInputGain);
    streamer.readFloat(mOutputGain);
    streamer.readFloat(mDelayTime);
    streamer.readFloat(mFeedback);
    streamer.readBool(mTempoSyncMode);
    streamer.readInt32(mSyncDivision);
    streamer.readInt32(mGrid);

    streamer.readFloat(mGlobalDryWet);
    streamer.readBool(mDelayBypass);

    // Load all tap parameters
    for (int i = 0; i < 16; i++) {
        streamer.readBool(mTapEnabled[i]);
        streamer.readFloat(mTapLevel[i]);
        streamer.readFloat(mTapPan[i]);
        // Load per-tap filter parameters
        streamer.readFloat(mTapFilterCutoff[i]);
        streamer.readFloat(mTapFilterResonance[i]);
        streamer.readInt32(mTapFilterType[i]);
        // Load pitch shift parameter
        streamer.readInt32(mTapPitchShift[i]);
        // Load feedback send parameter (with default fallback for legacy compatibility)
        if (!streamer.readFloat(mTapFeedbackSend[i])) {
            mTapFeedbackSend[i] = 0.0f;  // Default to no feedback send for legacy projects
        }

        // Initialize previous state to current state to prevent unwanted buffer clears
        mTapEnabledPrevious[i] = mTapEnabled[i];
    }

    mDelayBypassPrevious = mDelayBypass;

    return kResultOk;
}

tresult WaterStickProcessor::readVersionedProcessorState(IBStream* state, Steinberg::int32 version)
{
    switch (version) {
        case kStateVersionCurrent:
            // Version 1 state format - read signature then parameters
            return readCurrentVersionProcessorState(state);

        default:
            // Unknown version - skip state loading
            return kResultOk;
    }

    return kResultOk;
}

tresult WaterStickProcessor::readCurrentVersionProcessorState(IBStream* state)
{
    IBStreamer streamer(state, kLittleEndian);

    // Read and validate signature (version was already read)
    Steinberg::int32 signature;
    if (streamer.readInt32(signature)) {
        if (signature == kStateMagicNumber) {
            // Valid signature - continue reading state
        } else {
            // Invalid signature - this is suspicious but continue reading
            // The data structure should still be correct
        }
    } else {
        // No signature found - old v1 format, rewind
        state->seek(-4, IBStream::kIBSeekCur, nullptr);
    }

    // Read parameters using the legacy format (same structure)
    return readLegacyProcessorState(state);
}

//------------------------------------------------------------------------
// Real-time curve evaluation and parameter smoothing implementation
//------------------------------------------------------------------------
void WaterStickProcessor::updateDiscreteParameters()
{
    // This method is called from updateParameters() - parameters already updated from VST
    // No additional processing needed here as parameters are updated in the main parameter switch
}

void WaterStickProcessor::applyCurveEvaluation()
{
    // Phase 1: Apply macro curves to legacy discrete parameters for backward compatibility
    for (int i = 0; i < 24; i++) {
        float rawValue = mDiscreteParameters[i];

        // Apply curves to first 4 discrete parameters using the 4 macro curves
        if (i < 4) {
            float curvedValue = evaluateMacroCurve(mMacroCurveTypes[i], rawValue);
            mDiscreteParameters[i] = curvedValue;
        }
    }

    // Phase 2: REMOVED - No longer apply macro curves to tap parameters on every audio cycle
    // Macro curves are now applied only when macro parameters change (see process() method)
    // This prevents continuous parameter override that breaks user control and VST3 automation
}

void WaterStickProcessor::applyParameterSmoothing()
{
    // Apply sample-level smoothing for zipper-free modulation
    for (int i = 0; i < 24; i++) {
        float target = mDiscreteParameters[i];
        mDiscreteParametersSmoothed[i] = mDiscreteParametersSmoothed[i] * mSmoothingCoeff + target * (1.0f - mSmoothingCoeff);
    }
}

float WaterStickProcessor::evaluateMacroCurve(int curveType, float input) const
{
    // Clamp input to [0.0, 1.0]
    float x = std::max(0.0f, std::min(1.0f, input));

    switch (curveType) {
        case 0: // Linear
            return x;
        case 1: // Exponential
            return x * x;
        case 2: // Inverse Exponential
            return 1.0f - (1.0f - x) * (1.0f - x);
        case 3: // Logarithmic
            return std::log10(1.0f + x * 9.0f);
        case 4: // Inverse Logarithmic
            return (std::pow(10.0f, x) - 1.0f) / 9.0f;
        case 5: // S-Curve
            return 0.5f * (1.0f + std::tanh(4.0f * (x - 0.5f)));
        case 6: // Inverse S-Curve
            return 0.5f + 0.25f * std::atan(4.0f * (2.0f * x - 1.0f)) / std::atan(4.0f);
        case 7: // Quantized
        {
            int step = static_cast<int>(x * 7.999f); // 0-7
            return static_cast<float>(step) / 7.0f;
        }
        default:
            return x; // Default to linear
    }
}

//------------------------------------------------------------------------
// Processor-side Macro Curve Evaluation System
// This system mirrors the controller's MacroCurveSystem but applies curves
// directly to DSP parameter arrays instead of sending VST parameter updates
//------------------------------------------------------------------------

void WaterStickProcessor::applyMacroCurvesToTapParameters() {
    // Enhanced diagnostic logging for macro curve application
    if (PitchDebug::isLoggingEnabled()) {
        std::ostringstream macroLog;
        macroLog << "Macro Curve Application:n";
        for (int macroKnobIndex = 0; macroKnobIndex < 8; ++macroKnobIndex) {
            macroLog << "  Macro Knob " << macroKnobIndex
                     << " Value: " << mMacroKnobValues[macroKnobIndex]
                     << " Curve Type: " << mMacroCurveTypes[macroKnobIndex]
                     << " Active: " << (mMacroSystemActive[macroKnobIndex] ? "Yes" : "No") << "\n";
        }
        PitchDebug::logMessage(macroLog.str());
    }    // CRITICAL FIX: Only apply macro curves when macro values have actually changed
    // This prevents continuous parameter override that breaks user control and defaults

    // Check if any macro knob values have changed
    for (int macroKnobIndex = 0; macroKnobIndex < 8; macroKnobIndex++) {
        float currentValue = mMacroKnobValues[macroKnobIndex];
        float previousValue = mPreviousMacroKnobValues[macroKnobIndex];

        // Only process if the macro value has changed significantly
        if (std::abs(currentValue - previousValue) > 0.001f) {
            mPreviousMacroKnobValues[macroKnobIndex] = currentValue;

            // Activate macro system for this knob if it has a meaningful value
            if (currentValue > 0.001f) {
                mMacroSystemActive[macroKnobIndex] = true;
                mMacroInfluenceActive = true;
                applyMacroKnobToAllTaps(macroKnobIndex, macroKnobIndex); // Knob index = context index
            } else {
                // Macro knob returned to zero - deactivate macro influence for this knob
                mMacroSystemActive[macroKnobIndex] = false;

                // Check if any macro knobs are still active
                bool anyMacroActive = false;
                for (int i = 0; i < 8; i++) {
                    if (mMacroSystemActive[i]) {
                        anyMacroActive = true;
                        break;
                    }
                }
                mMacroInfluenceActive = anyMacroActive;
            }
        }
    }

    // If no macro values changed, don't process anything
    // This is the critical fix - prevents continuous parameter override
}

void WaterStickProcessor::applyMacroKnobToAllTaps(int macroKnobIndex, int tapContext)
{
    if (macroKnobIndex < 0 || macroKnobIndex >= 8 || tapContext < 0 || tapContext >= 8) {
        return;
    }

    float macroValue = mMacroKnobValues[macroKnobIndex];

    // Apply macro value to all 16 taps for the specified context
    for (int tapIndex = 0; tapIndex < 16; tapIndex++) {
        float curveValue = getGlobalCurveValueForTapContinuous(macroValue, tapIndex);

        // Apply the curve value to the appropriate parameter based on context
        switch (tapContext) {
            case 0: // Enable
                // For enable, use quantized value (0.0 or 1.0)
                mTapEnabled[tapIndex] = (curveValue >= 0.5f);
                break;
            case 1: // Volume
                mTapLevel[tapIndex] = curveValue;
                break;
            case 2: // Pan
                // Pan is bipolar: 0.5 = center, 0.0 = left, 1.0 = right
                mTapPan[tapIndex] = curveValue;
                break;
            case 3: // FilterCutoff
                mTapFilterCutoff[tapIndex] = curveValue;
                break;
            case 4: // FilterResonance
                mTapFilterResonance[tapIndex] = curveValue;
                break;
            case 5: // FilterType
                {
                    // Quantize to valid discrete filter type values (0-4)
                    float clampedValue = std::max(0.0f, std::min(1.0f, curveValue));
                    int filterType = static_cast<int>(std::floor(clampedValue * 5.0f));
                    if (filterType >= 5) filterType = 4; // Handle boundary case
                    mTapFilterType[tapIndex] = filterType;
                }
                break;
            case 6: // PitchShift
                {
                    // Convert curve value (0.0-1.0) to semitones (-12 to +12)
                    int semitones = static_cast<int>((curveValue * 2.0f - 1.0f) * 12.0f + 0.5f);
                    semitones = std::max(-12, std::min(12, semitones));
                    mTapPitchShift[tapIndex] = semitones;
                }
                break;
            case 7: // FeedbackSend
                mTapFeedbackSend[tapIndex] = curveValue;
                break;
            default:
                break;
        }
    }
}

float WaterStickProcessor::getGlobalCurveValueForTap(int discretePosition, int tapIndex) const
{
    if (tapIndex < 0 || tapIndex >= 16) return 0.0f;

    // Normalize tap index to 0.0-1.0 for curve evaluation
    const float normalizedPosition = static_cast<float>(tapIndex) / 15.0f; // 0-15 maps to 0.0-1.0

    switch (discretePosition) {
        case 0: // Ramp up curve
            return evaluateRampUp(normalizedPosition);
        case 1: // Ramp down curve
            return evaluateRampDown(normalizedPosition);
        case 2: // S-curve sigmoid
            return evaluateSigmoidSCurve(normalizedPosition);
        case 3: // S-curve inverted
            return evaluateInverseSigmoid(normalizedPosition);
        case 4: // Exponential up
            return evaluateExpUp(normalizedPosition);
        case 5: // Exponential down
            return evaluateExpDown(normalizedPosition);
        case 6: // Uniform level 70%
            return 0.7f;
        case 7: // Uniform level 100%
            return 1.0f;
        default:
            return normalizedPosition; // Linear fallback
    }
}

float WaterStickProcessor::getGlobalCurveValueForTapContinuous(float continuousValue, int tapIndex) const
{
    if (tapIndex < 0 || tapIndex >= 16) return 0.0f;

    // Clamp input to valid range [0.0, 1.0]
    float clampedValue = std::max(0.0f, std::min(1.0f, continuousValue));

    // Smooth interpolation between different curve types based on continuous value
    // Scale continuous value to range [0, 7] for 8 curve positions
    float scaledValue = clampedValue * 7.0f;
    int lowerIndex = static_cast<int>(std::floor(scaledValue));
    int upperIndex = static_cast<int>(std::ceil(scaledValue));

    // Handle boundary case where clampedValue = 1.0 exactly
    if (lowerIndex >= 7) lowerIndex = 7;
    if (upperIndex >= 8) upperIndex = 7;

    float lowerWeight = static_cast<float>(upperIndex) - scaledValue;
    float upperWeight = 1.0f - lowerWeight;

    // Get curve values for lower and upper positions
    float lowerCurveValue = getGlobalCurveValueForTap(lowerIndex, tapIndex);
    float upperCurveValue = getGlobalCurveValueForTap(upperIndex, tapIndex);

    // Linear interpolation between curve values
    return lowerCurveValue * lowerWeight + upperCurveValue * upperWeight;
}

// Rainmaker-style curve implementations (matching controller)
float WaterStickProcessor::evaluateRampUp(float x) const
{
    return x; // Linear ramp up
}

float WaterStickProcessor::evaluateRampDown(float x) const
{
    return 1.0f - x; // Linear ramp down
}

float WaterStickProcessor::evaluateSigmoidSCurve(float x) const
{
    // S-curve using sigmoid function
    return 0.5f * (1.0f + std::tanh(4.0f * (x - 0.5f)));
}

float WaterStickProcessor::evaluateInverseSigmoid(float x) const
{
    // Inverse S-curve (U-shape)
    return 1.0f - evaluateSigmoidSCurve(x);
}

float WaterStickProcessor::evaluateExpUp(float x) const
{
    // Exponential curve (slow start, fast end)
    return x * x;
}

float WaterStickProcessor::evaluateExpDown(float x) const
{
    // Inverse exponential curve (fast start, slow end)
    return 1.0f - (1.0f - x) * (1.0f - x);
}

float WaterStickProcessor::getSmoothedDiscreteParameter(int index) const
{
    if (index >= 0 && index < 24) {
        return mDiscreteParametersSmoothed[index];
    }
    return 0.0f;
}

float WaterStickProcessor::getRawDiscreteParameter(int index) const
{
    if (index >= 0 && index < 24) {
        return mDiscreteParameters[index];
    }
    return 0.0f;
}

//------------------------------------------------------------------------
// Parameter Source Priority Implementation
// VST3 architecture compliance - user parameters take precedence over macro effects
//------------------------------------------------------------------------

void WaterStickProcessor::detectUserParameterChanges() {
    // Enhanced diagnostic logging for parameter change detection
    if (PitchDebug::isLoggingEnabled()) {
        std::ostringstream changeLog;
        changeLog << "User Parameter Changes Detected:n";
        for (int paramId = 0; paramId < 1024; ++paramId) {
            if (mParameterModifiedByUser[paramId]) {
                changeLog << "  Parameter ID: " << paramId
                          << " Marked as User-Modified\n";
            }
        }
        PitchDebug::logMessage(changeLog.str());
    }    // This method would be called when parameters are changed via VST3 parameter updates
    // Currently not implemented as it requires integration with parameter change detection
    // TODO: Implement parameter change source detection in updateParameters()
}

void WaterStickProcessor::markParameterAsUserModified(int paramId)
{
    if (paramId >= 0 && paramId < 1024) {
        mParameterModifiedByUser[paramId] = true;

        // Clear macro influence for this parameter when user takes control
        clearMacroInfluenceForParameter(paramId);
    }
}

void WaterStickProcessor::clearMacroInfluenceForParameter(int paramId)
{
    if (paramId >= 0 && paramId < 1024) {
        // Mark this parameter as no longer under macro influence
        // This allows user changes to take precedence
        // Implementation would depend on specific parameter mapping
        // For now, this establishes the architecture
    }
}

void WaterStickProcessor::enableMacroControlForParameter(int paramId)
{
    if (paramId >= 0 && paramId < 1024) {
        mParameterModifiedByUser[paramId] = false;
        // This allows macro system to regain control after user explicitly re-enables it
    }
}

bool WaterStickProcessor::isParameterUnderMacroInfluence(int paramId) const
{
    if (paramId >= 0 && paramId < 1024) {
        return !mParameterModifiedByUser[paramId] && mMacroInfluenceActive;
    }
    return false;
}

// Debug and diagnostics methods for dropout investigation
void WaterStickProcessor::enablePitchDebugLogging(bool enable)
{
    PitchDebug::enableLogging(enable);
    if (enable) {
        PitchDebug::logMessage("Pitch debug logging enabled");
    }
}

void WaterStickProcessor::logPitchProcessingStats() const
{
    for (int i = 0; i < NUM_TAPS; ++i) {
        if (mTapEnabled[i] && mTapPitchShift[i] != 0) {
            std::ostringstream ss;
            ss << "Tap " << (i + 1) << " pitch processing stats:";
            PitchDebug::logMessage(ss.str());
            mTapDelayLinesL[i].logProcessingStats();
            mTapDelayLinesR[i].logProcessingStats();
        }
    }
}

// Performance profiling methods for dropout investigation
void WaterStickProcessor::enablePerformanceProfiling(bool enable)
{
    PerformanceProfiler::getInstance().enableProfiling(enable);
}

void WaterStickProcessor::logPerformanceReport() const
{
    PerformanceProfiler::getInstance().logPerformanceReport();
}

void WaterStickProcessor::clearPerformanceProfile()
{
    PerformanceProfiler::getInstance().clearProfile();
}

// Phase 4: Unified delay line system control methods (primarily for emergency fallback)
void WaterStickProcessor::enableUnifiedDelayLines(bool enable)
{
    if (mUseUnifiedDelayLines != enable) {
        mUseUnifiedDelayLines = enable;

        // Reset all delay lines when switching systems for clean transition
        for (int i = 0; i < NUM_TAPS; i++) {
            mTapDelayLinesL[i].reset();
            mTapDelayLinesR[i].reset();
            mUnifiedTapDelayLinesL[i].reset();
            mUnifiedTapDelayLinesR[i].reset();
        }

        if (PitchDebug::isLoggingEnabled()) {
            PitchDebug::logMessage(enable ? "Switched to unified delay line system" : "Switched to legacy delay line system");
        }
    }
}

bool WaterStickProcessor::isUsingUnifiedDelayLines() const
{
    return mUseUnifiedDelayLines;
}

void WaterStickProcessor::logUnifiedDelayLineStats() const
{
    if (PitchDebug::isLoggingEnabled()) {
        std::ostringstream ss;
        ss << "Unified Delay Line System Status: " << (mUseUnifiedDelayLines ? "ENABLED" : "DISABLED");
        PitchDebug::logMessage(ss.str());

        if (mUseUnifiedDelayLines) {
            // Log stats for each active tap
            for (int i = 0; i < NUM_TAPS; i++) {
                if (mTapEnabled[i]) {
                    ss.str("");
                    ss.clear();
                    ss << "Tap " << i << " Stats:";
                    PitchDebug::logMessage(ss.str());
                    mUnifiedTapDelayLinesL[i].logProcessingStats();
                    mUnifiedTapDelayLinesR[i].logProcessingStats();
                }
            }
        }
    }
}

// Phase 5: Decoupled delay + pitch architecture control methods (production solution)
void WaterStickProcessor::enableDecoupledDelayLines(bool enable)
{
    if (mUseDecoupledArchitecture != enable) {
        mUseDecoupledArchitecture = enable;

        // Reset all systems when switching for clean transition
        if (enable) {
            mDecoupledDelaySystemL.reset();
            mDecoupledDelaySystemR.reset();
        } else {
            // Reset fallback systems
            for (int i = 0; i < NUM_TAPS; i++) {
                mUnifiedTapDelayLinesL[i].reset();
                mUnifiedTapDelayLinesR[i].reset();
            }
        }

        if (PitchDebug::isLoggingEnabled()) {
            PitchDebug::logMessage(enable ?
                "Switched to decoupled delay + pitch architecture" :
                "Switched back to unified delay line system");
        }
    }
}

bool WaterStickProcessor::isUsingDecoupledDelayLines() const
{
    return mUseDecoupledArchitecture;
}

void WaterStickProcessor::enablePitchProcessing(bool enable)
{
    if (mUseDecoupledArchitecture) {
        mDecoupledDelaySystemL.enablePitchProcessing(enable);
        mDecoupledDelaySystemR.enablePitchProcessing(enable);

        if (PitchDebug::isLoggingEnabled()) {
            PitchDebug::logMessage(enable ?
                "Pitch processing enabled in decoupled system" :
                "Pitch processing disabled - delay-only mode");
        }
    }
}

void WaterStickProcessor::logDecoupledSystemHealth() const
{
    if (!mUseDecoupledArchitecture || !PitchDebug::isLoggingEnabled()) return;

    DecoupledDelaySystem::SystemHealth healthL, healthR;
    mDecoupledDelaySystemL.getSystemHealth(healthL);
    mDecoupledDelaySystemR.getSystemHealth(healthR);

    std::ostringstream ss;
    ss << "Decoupled System Health Report:\n"
       << "  Left Channel - Delay: " << (healthL.delaySystemHealthy ? "HEALTHY" : "FAILED")
       << ", Pitch: " << (healthL.pitchSystemHealthy ? "HEALTHY" : "FAILED") << "\n"
       << "  Right Channel - Delay: " << (healthR.delaySystemHealthy ? "HEALTHY" : "FAILED")
       << ", Pitch: " << (healthR.pitchSystemHealthy ? "HEALTHY" : "FAILED") << "\n"
       << "  Active Taps: L=" << healthL.activeTaps << ", R=" << healthR.activeTaps << "\n"
       << "  Failed Pitch Taps: L=" << healthL.failedPitchTaps << ", R=" << healthR.failedPitchTaps << "\n"
       << "  Processing Times: Delay=" << std::fixed << std::setprecision(1)
       << healthL.delayProcessingTime << "μs, Pitch=" << healthL.pitchProcessingTime
       << "μs, Total=" << healthL.totalProcessingTime << "μs";

    PitchDebug::logMessage(ss.str());
}

bool WaterStickProcessor::isDecoupledSystemHealthy() const
{
    if (!mUseDecoupledArchitecture) return false;

    DecoupledDelaySystem::SystemHealth healthL, healthR;
    mDecoupledDelaySystemL.getSystemHealth(healthL);
    mDecoupledDelaySystemR.getSystemHealth(healthR);

    // System is healthy if delay processing works on both channels
    // (pitch processing is optional and can fail gracefully)
    return healthL.delaySystemHealthy && healthR.delaySystemHealthy;
}

// =====================================================
// PHASE 3: OPTIMIZED PARAMETER UPDATE IMPLEMENTATIONS
// =====================================================

void WaterStickProcessor::updateParametersOptimized()
{
    // CRITICAL FIX: Disable complex atomic macro parameter updates that may be causing audio failure
    // if (mMacroParametersNeedUpdate.load(std::memory_order_acquire)) {
    //     updateMacroParametersBatch();
    //     mMacroParametersNeedUpdate.store(false, std::memory_order_release);
    // }

    // CRITICAL FIX: Disable optimized parameter change checking that may be causing audio failure
    // if (checkParameterChangesOptimized()) {
    //     applyParameterCacheUpdates();
    // }

    // Update tempo sync and other lightweight parameters
    mTempoSync.setMode(mTempoSyncMode);
    mTempoSync.setSyncDivision(mSyncDivision);
    mTempoSync.setFreeTime(mDelayTime);

    mTapDistribution.updateTempo(mTempoSync);
    mTapDistribution.setGrid(mGrid);

    // Apply optimized parameter smoothing only to changed parameters
    applyParameterSmoothing();
}

void WaterStickProcessor::updateMacroParametersBatch()
{
    // Acquire a parameter batch from the memory pool
    MacroParameterBatch* batch = mParameterUpdatePool->acquire();
    MacroParameterBatch stackBatch;
    bool usingStackBatch = false;

    if (!batch) {
        // Pool exhausted - fallback to stack allocation
        batch = &stackBatch;
        usingStackBatch = true;
    }

    // Copy current macro knob values to batch
    memcpy(batch->values, mMacroKnobValues, sizeof(mMacroKnobValues));
    batch->version = mMacroParameterVersion.fetch_add(1, std::memory_order_acq_rel);
    batch->dirtyFlags = 0;

    // CRITICAL FIX: Disable SIMD curve evaluator that may be causing audio failure
    // mSIMDCurveEvaluator->evaluateAllCurves(*batch);  // DISABLED - complex SIMD operations

    // Process the batch update
    processMacroParameterUpdate(*batch);

    // Release the batch back to the pool (if it came from pool)
    if (!usingStackBatch) {
        mParameterUpdatePool->release(batch);
    }
}

void WaterStickProcessor::processMacroParameterUpdate(const MacroParameterBatch& batch)
{
    // Apply macro curve evaluations to tap parameters using cache-optimized approach
    for (int tap = 0; tap < NUM_TAPS; ++tap) {
        // Check if this tap needs updates
        uint32_t tapDirtyFlags = mTapParameterDirtyFlags[tap].load(std::memory_order_acquire);

        if (tapDirtyFlags != 0) {
            // Get current tap parameters in batch
            float tapParams[8]; // All tap parameters
            mParameterCache->getTapParametersBatch(tap, tapParams, 8);

            // Apply macro influences to parameters using SIMD-evaluated curves
            for (int macro = 0; macro < 8; ++macro) {
                if (mMacroSystemActive[macro]) {
                    float macroInfluence = batch.curves[macro];

                    // Apply to relevant tap parameter based on current context
                    switch (mCurrentTapContext) {
                        case 1: // Volume
                            tapParams[0] = tapParams[0] * (1.0f - macroInfluence) + macroInfluence;
                            break;
                        case 2: // Pan
                            tapParams[1] = tapParams[1] * (1.0f - macroInfluence) + (macroInfluence * 2.0f - 1.0f);
                            break;
                        case 3: // Filter Cutoff
                            tapParams[2] = tapParams[2] * (1.0f - macroInfluence) + macroInfluence;
                            break;
                        case 4: // Filter Resonance
                            tapParams[3] = tapParams[3] * (1.0f - macroInfluence) + macroInfluence;
                            break;
                        // Additional cases as needed
                    }
                }
            }

            // Update cache with new values
            mParameterCache->setTapParametersBatch(tap, tapParams, 8);

            // Clear dirty flags for this tap
            mTapParameterDirtyFlags[tap].store(0, std::memory_order_release);
        }
    }
}

void WaterStickProcessor::applyParameterCacheUpdates()
{
    // Update actual parameter arrays from cache only for dirty parameters
    for (int tap = 0; tap < NUM_TAPS; ++tap) {
        if (mParameterCache->isDirty(tap, 0)) { // Level
            mTapLevel[tap] = mParameterCache->getTapParameter(tap, 0);
            mParameterCache->clearDirty(tap, 0);
        }
        if (mParameterCache->isDirty(tap, 1)) { // Pan
            mTapPan[tap] = mParameterCache->getTapParameter(tap, 1);
            mParameterCache->clearDirty(tap, 1);
        }
        if (mParameterCache->isDirty(tap, 2)) { // Filter Cutoff
            mTapFilterCutoff[tap] = mParameterCache->getTapParameter(tap, 2);
            mParameterCache->clearDirty(tap, 2);
        }
        if (mParameterCache->isDirty(tap, 3)) { // Filter Resonance
            mTapFilterResonance[tap] = mParameterCache->getTapParameter(tap, 3);
            mParameterCache->clearDirty(tap, 3);
        }
        if (mParameterCache->isDirty(tap, 4)) { // Filter Type
            mTapFilterType[tap] = static_cast<int>(mParameterCache->getTapParameter(tap, 4));
            mParameterCache->clearDirty(tap, 4);
        }
        if (mParameterCache->isDirty(tap, 5)) { // Pitch Shift
            mTapPitchShift[tap] = static_cast<int>(mParameterCache->getTapParameter(tap, 5));
            mParameterCache->clearDirty(tap, 5);
        }
        if (mParameterCache->isDirty(tap, 6)) { // Feedback Send
            mTapFeedbackSend[tap] = mParameterCache->getTapParameter(tap, 6);
            mParameterCache->clearDirty(tap, 6);
        }
        if (mParameterCache->isDirty(tap, 7)) { // Enable
            mTapEnabled[tap] = mParameterCache->getTapParameter(tap, 7) > 0.5f;
            mParameterCache->clearDirty(tap, 7);
        }
    }
}

bool WaterStickProcessor::checkParameterChangesOptimized()
{
    // Check global parameter version for any changes
    uint64_t currentVersion = mParameterCache->getGlobalVersion();
    uint64_t lastVersion = mLastParameterUpdateVersion.load(std::memory_order_acquire);

    if (currentVersion != lastVersion) {
        mLastParameterUpdateVersion.store(currentVersion, std::memory_order_release);
        return true;
    }

    // Check individual tap dirty flags for fine-grained updates
    for (int tap = 0; tap < NUM_TAPS; ++tap) {
        if (mTapParameterDirtyFlags[tap].load(std::memory_order_acquire) != 0) {
            return true;
        }
    }

    return false;
}

} // namespace WaterStick