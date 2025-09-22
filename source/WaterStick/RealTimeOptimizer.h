#pragma once

#include <cmath>
#include <array>
#include <atomic>
#include <chrono>
#include <immintrin.h>
#include <algorithm>

namespace WaterStick {

/**
 * @class LookupTable
 * @brief High-performance lookup table for expensive mathematical operations
 *
 * Provides pre-computed values for expensive operations like exp(), log(), Bark conversion
 * with linear interpolation for smooth intermediate values.
 */
template<int TableSize = 1024>
class LookupTable {
public:
    LookupTable() = default;
    ~LookupTable() = default;

    /**
     * @brief Initialize table for exponential function
     * @param minVal Minimum input value
     * @param maxVal Maximum input value
     */
    void initializeExp(float minVal, float maxVal) {
        mMinVal = minVal;
        mMaxVal = maxVal;
        mRange = maxVal - minVal;
        mScale = static_cast<float>(TableSize - 1) / mRange;

        for (int i = 0; i < TableSize; ++i) {
            float x = minVal + (static_cast<float>(i) / (TableSize - 1)) * mRange;
            mTable[i] = std::exp(x);
        }
        mInitialized = true;
    }

    /**
     * @brief Initialize table for natural logarithm
     * @param minVal Minimum input value (must be > 0)
     * @param maxVal Maximum input value
     */
    void initializeLog(float minVal, float maxVal) {
        mMinVal = minVal;
        mMaxVal = maxVal;
        mRange = maxVal - minVal;
        mScale = static_cast<float>(TableSize - 1) / mRange;

        for (int i = 0; i < TableSize; ++i) {
            float x = minVal + (static_cast<float>(i) / (TableSize - 1)) * mRange;
            mTable[i] = std::log(x);
        }
        mInitialized = true;
    }

    /**
     * @brief Initialize table for Bark scale conversion
     * @param minFreq Minimum frequency in Hz
     * @param maxFreq Maximum frequency in Hz
     */
    void initializeBark(float minFreq, float maxFreq) {
        mMinVal = minFreq;
        mMaxVal = maxFreq;
        mRange = maxFreq - minFreq;
        mScale = static_cast<float>(TableSize - 1) / mRange;

        for (int i = 0; i < TableSize; ++i) {
            float freq = minFreq + (static_cast<float>(i) / (TableSize - 1)) * mRange;
            // Bark scale formula: Bark = 13 * arctan(0.00076 * freq) + 3.5 * arctan((freq/7500)^2)
            float bark = 13.0f * std::atan(0.00076f * freq) +
                        3.5f * std::atan(std::pow(freq / 7500.0f, 2.0f));
            mTable[i] = bark;
        }
        mInitialized = true;
    }

    /**
     * @brief Get interpolated value from lookup table
     * @param input Input value
     * @return Interpolated output value
     */
    inline float lookup(float input) const {
        if (!mInitialized) {
            return 0.0f; // Safety fallback
        }

        // Clamp input to valid range
        input = std::max(mMinVal, std::min(mMaxVal, input));

        // Calculate table position
        float scaledInput = (input - mMinVal) * mScale;
        int index = static_cast<int>(scaledInput);
        float fraction = scaledInput - static_cast<float>(index);

        // Bounds checking
        if (index >= TableSize - 1) {
            return mTable[TableSize - 1];
        }

        // Linear interpolation
        return mTable[index] + fraction * (mTable[index + 1] - mTable[index]);
    }

    /**
     * @brief Check if table is initialized
     * @return True if initialized
     */
    bool isInitialized() const { return mInitialized; }

    /**
     * @brief Get direct table access for SIMD operations
     * @return Pointer to table data
     */
    const float* getTableData() const { return mTable.data(); }

    /**
     * @brief Get table parameters for manual indexing
     */
    void getTableParams(float& minVal, float& scale, int& size) const {
        minVal = mMinVal;
        scale = mScale;
        size = TableSize;
    }

private:
    std::array<float, TableSize> mTable;
    float mMinVal = 0.0f;
    float mMaxVal = 1.0f;
    float mRange = 1.0f;
    float mScale = static_cast<float>(TableSize - 1);
    bool mInitialized = false;
};

/**
 * @class CPUMonitor
 * @brief Real-time CPU usage monitoring for adaptive quality control
 *
 * Monitors processing time per audio buffer and provides feedback for
 * automatic quality reduction under CPU pressure.
 */
class CPUMonitor {
public:
    CPUMonitor();
    ~CPUMonitor() = default;

    /**
     * @brief Initialize monitor with sample rate and buffer size
     * @param sampleRate Audio sample rate
     * @param bufferSize Processing buffer size
     */
    void initialize(double sampleRate, int bufferSize);

    /**
     * @brief Start timing measurement for current buffer
     */
    void startTiming();

    /**
     * @brief End timing measurement and update statistics
     */
    void endTiming();

    /**
     * @brief Get current CPU usage as percentage of available time
     * @return CPU usage percentage (0.0 to 100.0+)
     */
    float getCPUUsage() const { return mCurrentCPUUsage.load(); }

    /**
     * @brief Get smoothed CPU usage for stable quality decisions
     * @return Smoothed CPU usage percentage
     */
    float getSmoothedCPUUsage() const { return mSmoothedCPUUsage.load(); }

    /**
     * @brief Check if CPU usage is above threshold
     * @param threshold Threshold percentage (default: 75.0%)
     * @return True if overloaded
     */
    bool isOverloaded(float threshold = 75.0f) const {
        return getSmoothedCPUUsage() > threshold;
    }

    /**
     * @brief Check if emergency fallback is needed
     * @return True if immediate action required
     */
    bool isEmergencyOverload() const {
        return getCPUUsage() > 95.0f || getSmoothedCPUUsage() > 90.0f;
    }

    /**
     * @brief Set CPU usage thresholds for quality control
     * @param warningThreshold Warning threshold (50-80%)
     * @param criticalThreshold Critical threshold (80-95%)
     * @param emergencyThreshold Emergency threshold (90-99%)
     */
    void setThresholds(float warningThreshold = 60.0f,
                      float criticalThreshold = 80.0f,
                      float emergencyThreshold = 95.0f);

    /**
     * @brief Get current performance level (0=emergency, 1=critical, 2=warning, 3=normal)
     * @return Performance level
     */
    int getPerformanceLevel() const;

    /**
     * @brief Reset statistics
     */
    void reset();

    /**
     * @brief Get timing statistics for debugging
     */
    void getTimingStats(float& avgTime, float& maxTime, float& minTime) const;

private:
    // Timing infrastructure
    std::chrono::high_resolution_clock::time_point mStartTime;
    double mTargetTimePerBuffer;           // Target processing time per buffer

    // Statistics (atomic for thread safety)
    std::atomic<float> mCurrentCPUUsage;   // Current buffer CPU usage
    std::atomic<float> mSmoothedCPUUsage;  // Exponentially smoothed CPU usage
    std::atomic<float> mMaxCPUUsage;       // Peak CPU usage
    std::atomic<float> mMinCPUUsage;       // Minimum CPU usage

    // Thresholds
    float mWarningThreshold;
    float mCriticalThreshold;
    float mEmergencyThreshold;

    // Smoothing parameters
    float mSmoothingFactor;                // Exponential smoothing factor
    int mStabilityCounter;                 // Counter for stable readings

    // Buffer management
    static const int HISTORY_SIZE = 64;
    std::array<float, HISTORY_SIZE> mTimingHistory;
    int mHistoryIndex;
    bool mHistoryFilled;

    // Internal methods
    void updateSmoothedUsage(float currentUsage);
    float calculateHistoryAverage() const;
};

/**
 * @class QualityController
 * @brief Automatic quality reduction system for CPU overload protection
 *
 * Manages adaptive quality reduction based on CPU usage, providing
 * graceful degradation while maintaining real-time safety.
 */
class QualityController {
public:
    /**
     * @brief Quality levels for adaptive processing
     */
    enum QualityLevel {
        EMERGENCY = 0,      // Minimal processing, bypass non-essential features
        LOW = 1,            // Reduced processing, simplified algorithms
        MEDIUM = 2,         // Moderate processing, some optimizations disabled
        HIGH = 3,           // Full processing, all features enabled
        ULTRA = 4           // Maximum quality, all optimizations enabled
    };

    QualityController();
    ~QualityController() = default;

    /**
     * @brief Initialize quality controller
     * @param initialQuality Starting quality level
     */
    void initialize(QualityLevel initialQuality = HIGH);

    /**
     * @brief Update quality level based on CPU monitor feedback
     * @param cpuMonitor Reference to CPU monitor
     */
    void updateQualityLevel(const CPUMonitor& cpuMonitor);

    /**
     * @brief Get current quality level
     * @return Current quality level
     */
    QualityLevel getCurrentQuality() const { return mCurrentQuality; }

    /**
     * @brief Get target quality level (for smooth transitions)
     * @return Target quality level
     */
    QualityLevel getTargetQuality() const { return mTargetQuality; }

    /**
     * @brief Check if quality reduction is active
     * @return True if operating below HIGH quality
     */
    bool isQualityReduced() const { return mCurrentQuality < HIGH; }

    /**
     * @brief Check if emergency mode is active
     * @return True if in emergency mode
     */
    bool isEmergencyMode() const { return mCurrentQuality == EMERGENCY; }

    /**
     * @brief Get quality-dependent processing parameters
     * @param maxStages [out] Maximum cascade stages allowed
     * @param enablePerceptual [out] Enable perceptual mapping
     * @param enableFreqAnalysis [out] Enable frequency analysis
     * @param simdEnabled [out] Enable SIMD optimizations
     */
    void getProcessingConfig(int& maxStages, bool& enablePerceptual,
                           bool& enableFreqAnalysis, bool& simdEnabled) const;

    /**
     * @brief Get smoothing time constant multiplier for current quality
     * @return Time constant multiplier (1.0 = normal, >1.0 = faster/less smooth)
     */
    float getTimeConstantMultiplier() const;

    /**
     * @brief Force specific quality level (for testing/debugging)
     * @param quality Quality level to force
     */
    void forceQualityLevel(QualityLevel quality);

    /**
     * @brief Enable/disable automatic quality adjustment
     * @param enabled True to enable automatic adjustment
     */
    void setAutoQualityEnabled(bool enabled) { mAutoQualityEnabled = enabled; }

    /**
     * @brief Get quality level name for display
     * @param quality Quality level
     * @return Human-readable quality name
     */
    static const char* getQualityName(QualityLevel quality);

    /**
     * @brief Reset quality controller to default state
     */
    void reset();

private:
    QualityLevel mCurrentQuality;
    QualityLevel mTargetQuality;
    QualityLevel mPreviousQuality;

    // Transition management
    bool mAutoQualityEnabled;
    int mTransitionCounter;
    int mTransitionDelay;
    int mStabilityCounter;
    int mMinStabilityTime;

    // Hysteresis thresholds to prevent oscillation
    float mUpgradeHysteresis;    // CPU usage must drop below this to upgrade
    float mDowngradeHysteresis;  // CPU usage must exceed this to downgrade

    // Quality level mappings
    struct QualityConfig {
        int maxStages;
        bool enablePerceptual;
        bool enableFreqAnalysis;
        bool simdEnabled;
        float timeConstantMultiplier;
    };

    static const QualityConfig sQualityConfigs[5];

    // Internal methods
    QualityLevel determineTargetQuality(float cpuUsage, float smoothedUsage) const;
    void updateTransition();
    bool canUpgrade(float cpuUsage, float smoothedUsage) const;
    bool shouldDowngrade(float cpuUsage, float smoothedUsage) const;
};

/**
 * @class SIMDOptimizer
 * @brief SIMD optimization utilities for high-performance processing
 *
 * Provides optimized SIMD implementations for common smoothing operations.
 */
class SIMDOptimizer {
public:
    SIMDOptimizer() = default;
    ~SIMDOptimizer() = default;

    /**
     * @brief Check if SIMD instructions are available
     * @return True if SIMD optimizations can be used
     */
    static bool isAvailable();

    /**
     * @brief Process 4 smoothing stages in parallel using SIMD
     * @param input Input value (replicated to 4 values)
     * @param states Array of 4 stage states (modified in place)
     * @param coefficients Array of 4 smoothing coefficients
     */
    static void processCascadedStages4(float input, float* states, const float* coefficients);

    /**
     * @brief Process multiple parameter smoothing in parallel
     * @param inputs Array of input values (must be 4-aligned)
     * @param outputs Array of output values (must be 4-aligned)
     * @param states Array of state values (must be 4-aligned)
     * @param coefficients Array of coefficients (must be 4-aligned)
     * @param count Number of parameters (must be multiple of 4)
     */
    static void processMultipleParameters(const float* inputs, float* outputs,
                                        float* states, const float* coefficients,
                                        int count);

    /**
     * @brief SIMD lookup table interpolation for 4 values
     * @param inputs Array of 4 input values
     * @param outputs Array of 4 output values
     * @param tableData Lookup table data
     * @param minVal Minimum table value
     * @param scale Table scaling factor
     * @param tableSize Table size
     */
    static void lookupTable4(const float* inputs, float* outputs,
                           const float* tableData, float minVal, float scale, int tableSize);

    /**
     * @brief Check memory alignment for SIMD operations
     * @param ptr Pointer to check
     * @return True if properly aligned for SIMD
     */
    static bool isAligned(const void* ptr) {
        return (reinterpret_cast<uintptr_t>(ptr) & 15) == 0;
    }

    /**
     * @brief Get required alignment for SIMD operations
     * @return Alignment in bytes
     */
    static constexpr int getRequiredAlignment() { return 16; }
};

/**
 * @class MemoryOptimizer
 * @brief Cache-friendly memory layout optimization
 *
 * Manages memory layout for optimal cache performance in real-time processing.
 */
class MemoryOptimizer {
public:
    /**
     * @brief Aligned memory allocator for SIMD operations
     * @param size Size in bytes
     * @param alignment Alignment requirement (default: 16)
     * @return Aligned memory pointer or nullptr on failure
     */
    static void* alignedAlloc(size_t size, size_t alignment = 16);

    /**
     * @brief Free aligned memory
     * @param ptr Pointer to free
     */
    static void alignedFree(void* ptr);

    /**
     * @brief Prefetch memory for upcoming operations
     * @param ptr Memory address to prefetch
     * @param locality Locality hint (0-3, higher = more temporal locality)
     */
    static void prefetch(const void* ptr, int locality = 3);

    /**
     * @brief Calculate optimal buffer size for cache efficiency
     * @param minSize Minimum required size
     * @param elementSize Size of each element
     * @return Optimized buffer size
     */
    static size_t calculateOptimalBufferSize(size_t minSize, size_t elementSize);

    /**
     * @brief Check if memory layout is cache-friendly
     * @param ptr Memory pointer
     * @param size Memory size
     * @param stride Access stride
     * @return True if layout is optimal
     */
    static bool isCacheFriendly(const void* ptr, size_t size, size_t stride);
};

/**
 * @class NumericalStabilizer
 * @brief Numerical stability and bounds checking utilities
 *
 * Provides safe mathematical operations with overflow/underflow protection.
 */
class NumericalStabilizer {
public:
    // Constants for numerical stability
    static constexpr float EPSILON = 1e-8f;
    static constexpr float MIN_POSITIVE = 1e-6f;
    static constexpr float MAX_FINITE = 1e6f;

    /**
     * @brief Safe exponential function with overflow protection
     * @param x Input value
     * @return Safe exponential result
     */
    static float safeExp(float x);

    /**
     * @brief Safe logarithm with underflow protection
     * @param x Input value
     * @return Safe logarithm result
     */
    static float safeLog(float x);

    /**
     * @brief Safe division with zero protection
     * @param numerator Numerator
     * @param denominator Denominator
     * @param fallback Fallback value if denominator is zero
     * @return Safe division result
     */
    static float safeDivide(float numerator, float denominator, float fallback = 0.0f);

    /**
     * @brief Check if value is finite and within reasonable bounds
     * @param value Value to check
     * @return True if value is safe to use
     */
    static bool isFiniteAndSafe(float value);

    /**
     * @brief Clamp value to safe range
     * @param value Input value
     * @param minVal Minimum safe value
     * @param maxVal Maximum safe value
     * @return Clamped value
     */
    static float clampSafe(float value, float minVal, float maxVal);

    /**
     * @brief Denormal number elimination
     * @param value Input value
     * @return Value with denormals flushed to zero
     */
    static float flushDenormals(float value);

    /**
     * @brief SIMD denormal flushing for 4 values
     * @param values Array of 4 values (modified in place)
     */
    static void flushDenormals4(float* values);
};

/**
 * @class RealTimeOptimizer
 * @brief Main optimization coordinator for the hybrid smoothing system
 *
 * Integrates all optimization systems for real-time safe operation with
 * automatic quality adaptation and emergency fallback mechanisms.
 */
class RealTimeOptimizer {
public:
    RealTimeOptimizer();
    ~RealTimeOptimizer() = default;

    /**
     * @brief Initialize optimizer with audio settings
     * @param sampleRate Audio sample rate
     * @param bufferSize Processing buffer size
     */
    void initialize(double sampleRate, int bufferSize);

    /**
     * @brief Update optimization state (call once per audio buffer)
     */
    void updatePerFrame();

    /**
     * @brief Get CPU monitor reference
     * @return CPU monitor
     */
    CPUMonitor& getCPUMonitor() { return mCPUMonitor; }

    /**
     * @brief Get quality controller reference
     * @return Quality controller
     */
    QualityController& getQualityController() { return mQualityController; }

    /**
     * @brief Get lookup table for exponential function
     * @return Exponential lookup table
     */
    const LookupTable<1024>& getExpTable() const { return mExpTable; }

    /**
     * @brief Get lookup table for logarithm function
     * @return Logarithm lookup table
     */
    const LookupTable<1024>& getLogTable() const { return mLogTable; }

    /**
     * @brief Get lookup table for Bark scale conversion
     * @return Bark scale lookup table
     */
    const LookupTable<512>& getBarkTable() const { return mBarkTable; }

    /**
     * @brief Check if system is in emergency mode
     * @return True if emergency fallback is active
     */
    bool isEmergencyMode() const {
        return mQualityController.isEmergencyMode() || mCPUMonitor.isEmergencyOverload();
    }

    /**
     * @brief Get recommended processing configuration
     * @param maxStages [out] Maximum cascade stages
     * @param enablePerceptual [out] Enable perceptual processing
     * @param enableFreqAnalysis [out] Enable frequency analysis
     * @param simdEnabled [out] Enable SIMD optimizations
     */
    void getProcessingConfig(int& maxStages, bool& enablePerceptual,
                           bool& enableFreqAnalysis, bool& simdEnabled) const {
        mQualityController.getProcessingConfig(maxStages, enablePerceptual,
                                             enableFreqAnalysis, simdEnabled);
    }

    /**
     * @brief Get time constant multiplier for current performance level
     * @return Time constant multiplier
     */
    float getTimeConstantMultiplier() const {
        return mQualityController.getTimeConstantMultiplier();
    }

    /**
     * @brief Enable/disable optimization system
     * @param enabled True to enable optimizations
     */
    void setEnabled(bool enabled) { mEnabled = enabled; }

    /**
     * @brief Check if optimizer is enabled
     * @return True if enabled
     */
    bool isEnabled() const { return mEnabled; }

    /**
     * @brief Reset all optimization state
     */
    void reset();

    /**
     * @brief Get optimization statistics for debugging
     */
    void getOptimizationStats(float& cpuUsage, int& qualityLevel,
                            bool& emergencyActive, float& timeMultiplier) const;

private:
    // Core optimization components
    CPUMonitor mCPUMonitor;
    QualityController mQualityController;

    // Lookup tables for expensive operations
    LookupTable<1024> mExpTable;        // Exponential function (-20 to 0)
    LookupTable<1024> mLogTable;        // Natural logarithm (1e-6 to 10)
    LookupTable<512> mBarkTable;        // Bark scale (20Hz to 20kHz)

    // System state
    bool mEnabled;
    bool mInitialized;
    double mSampleRate;
    int mBufferSize;

    // Performance tracking
    int mFrameCounter;
    static constexpr int UPDATE_INTERVAL = 64; // Update every 64 frames

    // Initialize lookup tables
    void initializeLookupTables();
};

} // namespace WaterStick