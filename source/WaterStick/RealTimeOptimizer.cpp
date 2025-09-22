#include "RealTimeOptimizer.h"
#include <cstring>
#include <immintrin.h>

namespace WaterStick {

// CPUMonitor Implementation

CPUMonitor::CPUMonitor()
    : mTargetTimePerBuffer(0.0)
    , mCurrentCPUUsage(0.0f)
    , mSmoothedCPUUsage(0.0f)
    , mMaxCPUUsage(0.0f)
    , mMinCPUUsage(100.0f)
    , mWarningThreshold(60.0f)
    , mCriticalThreshold(80.0f)
    , mEmergencyThreshold(95.0f)
    , mSmoothingFactor(0.1f)
    , mStabilityCounter(0)
    , mTimingHistory({0.0f})
    , mHistoryIndex(0)
    , mHistoryFilled(false)
{
}

void CPUMonitor::initialize(double sampleRate, int bufferSize)
{
    // Calculate target processing time per buffer
    mTargetTimePerBuffer = static_cast<double>(bufferSize) / sampleRate;

    // Reset statistics
    reset();
}

void CPUMonitor::startTiming()
{
    mStartTime = std::chrono::high_resolution_clock::now();
}

void CPUMonitor::endTiming()
{
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - mStartTime);

    // Convert to seconds
    double processingTime = duration.count() * 1e-9;

    // Calculate CPU usage percentage
    float currentUsage = static_cast<float>((processingTime / mTargetTimePerBuffer) * 100.0);

    // Update current usage (atomic)
    mCurrentCPUUsage.store(currentUsage);

    // Update min/max statistics
    float currentMax = mMaxCPUUsage.load();
    float currentMin = mMinCPUUsage.load();

    if (currentUsage > currentMax) {
        mMaxCPUUsage.store(currentUsage);
    }
    if (currentUsage < currentMin) {
        mMinCPUUsage.store(currentUsage);
    }

    // Update timing history
    mTimingHistory[mHistoryIndex] = currentUsage;
    mHistoryIndex = (mHistoryIndex + 1) % HISTORY_SIZE;
    if (mHistoryIndex == 0) {
        mHistoryFilled = true;
    }

    // Update smoothed usage
    updateSmoothedUsage(currentUsage);
}

void CPUMonitor::setThresholds(float warningThreshold, float criticalThreshold, float emergencyThreshold)
{
    mWarningThreshold = std::max(10.0f, std::min(warningThreshold, 99.0f));
    mCriticalThreshold = std::max(mWarningThreshold + 5.0f, std::min(criticalThreshold, 99.0f));
    mEmergencyThreshold = std::max(mCriticalThreshold + 5.0f, std::min(emergencyThreshold, 99.9f));
}

int CPUMonitor::getPerformanceLevel() const
{
    float smoothedUsage = getSmoothedCPUUsage();

    if (smoothedUsage >= mEmergencyThreshold || getCPUUsage() > mEmergencyThreshold + 5.0f) {
        return 0; // Emergency
    } else if (smoothedUsage >= mCriticalThreshold) {
        return 1; // Critical
    } else if (smoothedUsage >= mWarningThreshold) {
        return 2; // Warning
    } else {
        return 3; // Normal
    }
}

void CPUMonitor::reset()
{
    mCurrentCPUUsage.store(0.0f);
    mSmoothedCPUUsage.store(0.0f);
    mMaxCPUUsage.store(0.0f);
    mMinCPUUsage.store(100.0f);
    mStabilityCounter = 0;
    mTimingHistory.fill(0.0f);
    mHistoryIndex = 0;
    mHistoryFilled = false;
}

void CPUMonitor::getTimingStats(float& avgTime, float& maxTime, float& minTime) const
{
    avgTime = calculateHistoryAverage();
    maxTime = mMaxCPUUsage.load();
    minTime = mMinCPUUsage.load();
}

void CPUMonitor::updateSmoothedUsage(float currentUsage)
{
    float previousSmoothed = mSmoothedCPUUsage.load();

    // Use adaptive smoothing factor based on change magnitude
    float changeMagnitude = std::abs(currentUsage - previousSmoothed);
    float adaptiveFactor = mSmoothingFactor;

    // Faster response to rapid increases (CPU spikes)
    if (currentUsage > previousSmoothed && changeMagnitude > 10.0f) {
        adaptiveFactor *= 2.0f; // React faster to spikes
    }

    // Slower response to decreases (allow time to stabilize)
    if (currentUsage < previousSmoothed) {
        adaptiveFactor *= 0.5f; // React slower to improvements
    }

    adaptiveFactor = std::max(0.05f, std::min(0.5f, adaptiveFactor));

    float newSmoothed = previousSmoothed + adaptiveFactor * (currentUsage - previousSmoothed);
    mSmoothedCPUUsage.store(newSmoothed);
}

float CPUMonitor::calculateHistoryAverage() const
{
    if (!mHistoryFilled && mHistoryIndex == 0) {
        return 0.0f;
    }

    int count = mHistoryFilled ? HISTORY_SIZE : mHistoryIndex;
    float sum = 0.0f;

    for (int i = 0; i < count; ++i) {
        sum += mTimingHistory[i];
    }

    return sum / static_cast<float>(count);
}

// QualityController Implementation

// Quality configuration table
const QualityController::QualityConfig QualityController::sQualityConfigs[5] = {
    // EMERGENCY: Minimal processing
    { 1, false, false, false, 4.0f },
    // LOW: Reduced processing
    { 2, false, false, true, 2.0f },
    // MEDIUM: Moderate processing
    { 3, false, true, true, 1.5f },
    // HIGH: Full processing
    { 4, true, true, true, 1.0f },
    // ULTRA: Maximum quality
    { 5, true, true, true, 0.8f }
};

QualityController::QualityController()
    : mCurrentQuality(HIGH)
    , mTargetQuality(HIGH)
    , mPreviousQuality(HIGH)
    , mAutoQualityEnabled(true)
    , mTransitionCounter(0)
    , mTransitionDelay(64)     // 64 buffers before quality change
    , mStabilityCounter(0)
    , mMinStabilityTime(128)   // 128 buffers of stability required
    , mUpgradeHysteresis(5.0f) // 5% hysteresis for upgrades
    , mDowngradeHysteresis(5.0f) // 5% hysteresis for downgrades
{
}

void QualityController::initialize(QualityLevel initialQuality)
{
    mCurrentQuality = initialQuality;
    mTargetQuality = initialQuality;
    mPreviousQuality = initialQuality;
    mTransitionCounter = 0;
    mStabilityCounter = 0;
}

void QualityController::updateQualityLevel(const CPUMonitor& cpuMonitor)
{
    if (!mAutoQualityEnabled) {
        return;
    }

    float cpuUsage = cpuMonitor.getCPUUsage();
    float smoothedUsage = cpuMonitor.getSmoothedCPUUsage();

    // Determine target quality based on CPU usage
    QualityLevel newTarget = determineTargetQuality(cpuUsage, smoothedUsage);

    // Handle transitions
    if (newTarget != mTargetQuality) {
        mTargetQuality = newTarget;
        mTransitionCounter = mTransitionDelay;
        mStabilityCounter = 0;
    } else {
        // Count stable periods
        if (mTransitionCounter > 0) {
            mTransitionCounter--;
        }
        mStabilityCounter++;
    }

    // Apply quality change if transition period has elapsed and system is stable
    if (mTransitionCounter == 0 && mStabilityCounter >= mMinStabilityTime) {
        if (mCurrentQuality != mTargetQuality) {
            mPreviousQuality = mCurrentQuality;
            mCurrentQuality = mTargetQuality;
            mStabilityCounter = 0;
        }
    }
}

void QualityController::getProcessingConfig(int& maxStages, bool& enablePerceptual,
                                          bool& enableFreqAnalysis, bool& simdEnabled) const
{
    const QualityConfig& config = sQualityConfigs[static_cast<int>(mCurrentQuality)];

    maxStages = config.maxStages;
    enablePerceptual = config.enablePerceptual;
    enableFreqAnalysis = config.enableFreqAnalysis;
    simdEnabled = config.simdEnabled;
}

float QualityController::getTimeConstantMultiplier() const
{
    return sQualityConfigs[static_cast<int>(mCurrentQuality)].timeConstantMultiplier;
}

void QualityController::forceQualityLevel(QualityLevel quality)
{
    mCurrentQuality = quality;
    mTargetQuality = quality;
    mPreviousQuality = quality;
    mTransitionCounter = 0;
    mStabilityCounter = 0;
}

const char* QualityController::getQualityName(QualityLevel quality)
{
    static const char* names[] = { "Emergency", "Low", "Medium", "High", "Ultra" };
    int index = static_cast<int>(quality);
    if (index < 0 || index >= 5) {
        return "Unknown";
    }
    return names[index];
}

void QualityController::reset()
{
    initialize(HIGH);
}

QualityController::QualityLevel QualityController::determineTargetQuality(float cpuUsage, float smoothedUsage) const
{
    // Emergency override for immediate danger
    if (cpuUsage > 95.0f || smoothedUsage > 90.0f) {
        return EMERGENCY;
    }

    // Use smoothed usage for primary decisions with hysteresis
    bool shouldUpgrade = canUpgrade(cpuUsage, smoothedUsage);
    bool shouldDowngrade = shouldDowngrade(cpuUsage, smoothedUsage);

    if (shouldDowngrade) {
        // Downgrade quality
        if (mCurrentQuality > EMERGENCY) {
            return static_cast<QualityLevel>(static_cast<int>(mCurrentQuality) - 1);
        }
    } else if (shouldUpgrade) {
        // Upgrade quality
        if (mCurrentQuality < ULTRA) {
            return static_cast<QualityLevel>(static_cast<int>(mCurrentQuality) + 1);
        }
    }

    return mCurrentQuality;
}

bool QualityController::canUpgrade(float cpuUsage, float smoothedUsage) const
{
    // Don't upgrade if current usage is too high
    if (cpuUsage > 80.0f || smoothedUsage > 70.0f) {
        return false;
    }

    // Require stable low usage for upgrades
    const float upgradeThresholds[] = { 10.0f, 35.0f, 50.0f, 65.0f, 75.0f };
    int currentIndex = static_cast<int>(mCurrentQuality);

    if (currentIndex >= 4) { // Already at maximum
        return false;
    }

    float threshold = upgradeThresholds[currentIndex + 1] - mUpgradeHysteresis;
    return smoothedUsage < threshold && mStabilityCounter >= mMinStabilityTime;
}

bool QualityController::shouldDowngrade(float cpuUsage, float smoothedUsage) const
{
    const float downgradeThresholds[] = { 0.0f, 40.0f, 55.0f, 70.0f, 80.0f };
    int currentIndex = static_cast<int>(mCurrentQuality);

    if (currentIndex <= 0) { // Already at minimum
        return false;
    }

    float threshold = downgradeThresholds[currentIndex] + mDowngradeHysteresis;

    // Immediate downgrade for high CPU spikes
    if (cpuUsage > threshold + 15.0f) {
        return true;
    }

    // Gradual downgrade for sustained high usage
    return smoothedUsage > threshold;
}

// SIMDOptimizer Implementation

bool SIMDOptimizer::isAvailable()
{
#ifdef __AVX__
    return true;
#else
    return false;
#endif
}

void SIMDOptimizer::processCascadedStages4(float input, float* states, const float* coefficients)
{
#ifdef __AVX__
    if (!isAligned(states) || !isAligned(coefficients)) {
        // Fallback to scalar processing
        for (int i = 0; i < 4; ++i) {
            states[i] = coefficients[i] * input + (1.0f - coefficients[i]) * states[i];
            input = states[i]; // Output of current stage becomes input to next
        }
        return;
    }

    // Load SIMD data
    __m128 vInput = _mm_set1_ps(input);
    __m128 vStates = _mm_load_ps(states);
    __m128 vCoeffs = _mm_load_ps(coefficients);
    __m128 vOnes = _mm_set1_ps(1.0f);

    // Calculate (1 - coefficients)
    __m128 vOneMinusCoeffs = _mm_sub_ps(vOnes, vCoeffs);

    // For cascaded stages, we need to process sequentially
    // Stage 0: states[0] = coeffs[0] * input + (1-coeffs[0]) * states[0]
    float newState0 = coefficients[0] * input + (1.0f - coefficients[0]) * states[0];

    // Stage 1: states[1] = coeffs[1] * newState0 + (1-coeffs[1]) * states[1]
    float newState1 = coefficients[1] * newState0 + (1.0f - coefficients[1]) * states[1];

    // Stage 2: states[2] = coeffs[2] * newState1 + (1-coeffs[2]) * states[2]
    float newState2 = coefficients[2] * newState1 + (1.0f - coefficients[2]) * states[2];

    // Stage 3: states[3] = coeffs[3] * newState2 + (1-coeffs[3]) * states[3]
    float newState3 = coefficients[3] * newState2 + (1.0f - coefficients[3]) * states[3];

    // Store results
    states[0] = newState0;
    states[1] = newState1;
    states[2] = newState2;
    states[3] = newState3;
#else
    // Fallback scalar implementation
    for (int i = 0; i < 4; ++i) {
        states[i] = coefficients[i] * input + (1.0f - coefficients[i]) * states[i];
        input = states[i];
    }
#endif
}

void SIMDOptimizer::processMultipleParameters(const float* inputs, float* outputs,
                                            float* states, const float* coefficients, int count)
{
#ifdef __AVX__
    if (!isAligned(inputs) || !isAligned(outputs) || !isAligned(states) || !isAligned(coefficients)) {
        // Fallback to scalar processing
        for (int i = 0; i < count; ++i) {
            outputs[i] = coefficients[i] * inputs[i] + (1.0f - coefficients[i]) * states[i];
            states[i] = outputs[i];
        }
        return;
    }

    int simdCount = count & ~3; // Process in groups of 4

    for (int i = 0; i < simdCount; i += 4) {
        __m128 vInputs = _mm_load_ps(&inputs[i]);
        __m128 vStates = _mm_load_ps(&states[i]);
        __m128 vCoeffs = _mm_load_ps(&coefficients[i]);
        __m128 vOnes = _mm_set1_ps(1.0f);

        // Calculate smoothing: coeff * input + (1-coeff) * state
        __m128 vOneMinusCoeffs = _mm_sub_ps(vOnes, vCoeffs);
        __m128 vTerm1 = _mm_mul_ps(vCoeffs, vInputs);
        __m128 vTerm2 = _mm_mul_ps(vOneMinusCoeffs, vStates);
        __m128 vResult = _mm_add_ps(vTerm1, vTerm2);

        _mm_store_ps(&outputs[i], vResult);
        _mm_store_ps(&states[i], vResult);
    }

    // Handle remaining elements
    for (int i = simdCount; i < count; ++i) {
        outputs[i] = coefficients[i] * inputs[i] + (1.0f - coefficients[i]) * states[i];
        states[i] = outputs[i];
    }
#else
    // Fallback scalar implementation
    for (int i = 0; i < count; ++i) {
        outputs[i] = coefficients[i] * inputs[i] + (1.0f - coefficients[i]) * states[i];
        states[i] = outputs[i];
    }
#endif
}

void SIMDOptimizer::lookupTable4(const float* inputs, float* outputs,
                                const float* tableData, float minVal, float scale, int tableSize)
{
#ifdef __AVX__
    __m128 vInputs = _mm_loadu_ps(inputs);
    __m128 vMinVal = _mm_set1_ps(minVal);
    __m128 vScale = _mm_set1_ps(scale);
    __m128 vTableSize = _mm_set1_ps(static_cast<float>(tableSize - 1));

    // Calculate scaled inputs
    __m128 vScaledInputs = _mm_mul_ps(_mm_sub_ps(vInputs, vMinVal), vScale);

    // Clamp to table bounds
    __m128 vZero = _mm_setzero_ps();
    vScaledInputs = _mm_max_ps(vZero, _mm_min_ps(vTableSize, vScaledInputs));

    // Extract indices and do scalar lookups (SIMD gather would be ideal but complex)
    float scaledValues[4];
    _mm_storeu_ps(scaledValues, vScaledInputs);

    for (int i = 0; i < 4; ++i) {
        int index = static_cast<int>(scaledValues[i]);
        float fraction = scaledValues[i] - static_cast<float>(index);

        if (index >= tableSize - 1) {
            outputs[i] = tableData[tableSize - 1];
        } else {
            outputs[i] = tableData[index] + fraction * (tableData[index + 1] - tableData[index]);
        }
    }
#else
    // Fallback scalar implementation
    for (int i = 0; i < 4; ++i) {
        float scaledInput = (inputs[i] - minVal) * scale;
        scaledInput = std::max(0.0f, std::min(static_cast<float>(tableSize - 1), scaledInput));

        int index = static_cast<int>(scaledInput);
        float fraction = scaledInput - static_cast<float>(index);

        if (index >= tableSize - 1) {
            outputs[i] = tableData[tableSize - 1];
        } else {
            outputs[i] = tableData[index] + fraction * (tableData[index + 1] - tableData[index]);
        }
    }
#endif
}

// MemoryOptimizer Implementation

void* MemoryOptimizer::alignedAlloc(size_t size, size_t alignment)
{
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return nullptr;
    }
    return ptr;
#endif
}

void MemoryOptimizer::alignedFree(void* ptr)
{
    if (ptr == nullptr) {
        return;
    }

#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

void MemoryOptimizer::prefetch(const void* ptr, int locality)
{
#ifdef __GNUC__
    __builtin_prefetch(ptr, 0, locality);
#elif defined(_MSC_VER)
    _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0 + locality);
#else
    // No prefetch available
    (void)ptr;
    (void)locality;
#endif
}

size_t MemoryOptimizer::calculateOptimalBufferSize(size_t minSize, size_t elementSize)
{
    // Target L1 cache size (32KB on most processors)
    const size_t L1_CACHE_SIZE = 32768;

    // Ensure minimum size
    size_t targetSize = std::max(minSize, static_cast<size_t>(64)); // At least 64 elements

    // Round up to cache line boundary (64 bytes)
    size_t totalBytes = targetSize * elementSize;
    size_t cacheLines = (totalBytes + 63) / 64;
    totalBytes = cacheLines * 64;

    // Don't exceed reasonable portion of L1 cache
    if (totalBytes > L1_CACHE_SIZE / 4) {
        totalBytes = L1_CACHE_SIZE / 4;
    }

    return totalBytes / elementSize;
}

bool MemoryOptimizer::isCacheFriendly(const void* ptr, size_t size, size_t stride)
{
    // Check alignment
    if (reinterpret_cast<uintptr_t>(ptr) % 16 != 0) {
        return false;
    }

    // Check stride efficiency
    if (stride > 64) { // Larger than cache line
        return false;
    }

    // Check total size reasonability
    if (size > 1024 * 1024) { // Larger than 1MB
        return false;
    }

    return true;
}

// NumericalStabilizer Implementation

float NumericalStabilizer::safeExp(float x)
{
    // Clamp input to prevent overflow
    x = std::max(-20.0f, std::min(20.0f, x));

    float result = std::exp(x);

    if (!std::isfinite(result)) {
        return (x > 0) ? MAX_FINITE : EPSILON;
    }

    return result;
}

float NumericalStabilizer::safeLog(float x)
{
    // Ensure positive input
    x = std::max(MIN_POSITIVE, x);

    float result = std::log(x);

    if (!std::isfinite(result)) {
        return -20.0f; // log(MIN_POSITIVE) approximately
    }

    return std::max(-20.0f, std::min(20.0f, result));
}

float NumericalStabilizer::safeDivide(float numerator, float denominator, float fallback)
{
    if (std::abs(denominator) < EPSILON) {
        return fallback;
    }

    float result = numerator / denominator;

    if (!std::isfinite(result)) {
        return fallback;
    }

    return std::max(-MAX_FINITE, std::min(MAX_FINITE, result));
}

bool NumericalStabilizer::isFiniteAndSafe(float value)
{
    return std::isfinite(value) &&
           std::abs(value) < MAX_FINITE &&
           std::abs(value) > EPSILON;
}

float NumericalStabilizer::clampSafe(float value, float minVal, float maxVal)
{
    if (!std::isfinite(value)) {
        return (minVal + maxVal) * 0.5f; // Return middle value for invalid inputs
    }

    return std::max(minVal, std::min(maxVal, value));
}

float NumericalStabilizer::flushDenormals(float value)
{
    // Flush denormal numbers to zero
    if (std::abs(value) < 1e-25f) {
        return 0.0f;
    }
    return value;
}

void NumericalStabilizer::flushDenormals4(float* values)
{
#ifdef __AVX__
    __m128 vValues = _mm_loadu_ps(values);
    __m128 vThreshold = _mm_set1_ps(1e-25f);
    __m128 vNegThreshold = _mm_set1_ps(-1e-25f);
    __m128 vZero = _mm_setzero_ps();

    // Create mask for values within denormal range
    __m128 vMaskPos = _mm_cmplt_ps(vValues, vThreshold);
    __m128 vMaskNeg = _mm_cmpgt_ps(vValues, vNegThreshold);
    __m128 vMask = _mm_and_ps(vMaskPos, vMaskNeg);

    // Blend with zero where denormals detected
    vValues = _mm_blendv_ps(vValues, vZero, vMask);

    _mm_storeu_ps(values, vValues);
#else
    for (int i = 0; i < 4; ++i) {
        values[i] = flushDenormals(values[i]);
    }
#endif
}

// RealTimeOptimizer Implementation

RealTimeOptimizer::RealTimeOptimizer()
    : mEnabled(true)
    , mInitialized(false)
    , mSampleRate(44100.0)
    , mBufferSize(512)
    , mFrameCounter(0)
{
}

void RealTimeOptimizer::initialize(double sampleRate, int bufferSize)
{
    mSampleRate = sampleRate;
    mBufferSize = bufferSize;

    // Initialize subsystems
    mCPUMonitor.initialize(sampleRate, bufferSize);
    mQualityController.initialize(QualityController::HIGH);

    // Initialize lookup tables
    initializeLookupTables();

    mFrameCounter = 0;
    mInitialized = true;
}

void RealTimeOptimizer::updatePerFrame()
{
    if (!mEnabled || !mInitialized) {
        return;
    }

    mFrameCounter++;

    // Update quality controller periodically
    if (mFrameCounter % UPDATE_INTERVAL == 0) {
        mQualityController.updateQualityLevel(mCPUMonitor);
    }
}

void RealTimeOptimizer::reset()
{
    mCPUMonitor.reset();
    mQualityController.reset();
    mFrameCounter = 0;
}

void RealTimeOptimizer::getOptimizationStats(float& cpuUsage, int& qualityLevel,
                                          bool& emergencyActive, float& timeMultiplier) const
{
    cpuUsage = mCPUMonitor.getSmoothedCPUUsage();
    qualityLevel = static_cast<int>(mQualityController.getCurrentQuality());
    emergencyActive = isEmergencyMode();
    timeMultiplier = mQualityController.getTimeConstantMultiplier();
}

void RealTimeOptimizer::initializeLookupTables()
{
    // Initialize exponential table for smoothing coefficients (-20 to 0)
    mExpTable.initializeExp(-20.0f, 0.0f);

    // Initialize logarithm table for velocity calculations (1e-6 to 10)
    mLogTable.initializeLog(1e-6f, 10.0f);

    // Initialize Bark scale table for perceptual frequency mapping (20Hz to 20kHz)
    mBarkTable.initializeBark(20.0f, 20000.0f);
}

} // namespace WaterStick