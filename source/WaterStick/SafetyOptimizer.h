#pragma once

#include <cmath>
#include <array>
#include <atomic>
#include <chrono>
#include <algorithm>

namespace WaterStick {

/**
 * @class NumericalSafety
 * @brief Essential numerical stability utilities for real-time safety
 *
 * Provides safe mathematical operations with overflow/underflow protection
 * without dependencies on complex optimization systems.
 */
class NumericalSafety {
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
    static inline float safeExp(float x) {
        // Clamp input to prevent overflow
        x = std::max(-20.0f, std::min(20.0f, x));
        float result = std::exp(x);

        if (!std::isfinite(result)) {
            return (x > 0) ? MAX_FINITE : EPSILON;
        }

        return result;
    }

    /**
     * @brief Safe logarithm with underflow protection
     * @param x Input value
     * @return Safe logarithm result
     */
    static inline float safeLog(float x) {
        // Ensure positive input
        x = std::max(MIN_POSITIVE, x);
        float result = std::log(x);

        if (!std::isfinite(result)) {
            return -20.0f; // log(MIN_POSITIVE) approximately
        }

        return std::max(-20.0f, std::min(20.0f, result));
    }

    /**
     * @brief Check if value is finite and within safe bounds
     * @param value Value to check
     * @return True if value is safe to use
     */
    static inline bool isFiniteAndSafe(float value) {
        return std::isfinite(value) &&
               std::abs(value) < MAX_FINITE &&
               std::abs(value) > EPSILON;
    }

    /**
     * @brief Clamp value to safe range
     * @param value Input value
     * @param minVal Minimum safe value
     * @param maxVal Maximum safe value
     * @return Clamped value
     */
    static inline float clampSafe(float value, float minVal, float maxVal) {
        if (!std::isfinite(value)) {
            return (minVal + maxVal) * 0.5f; // Return middle value for invalid inputs
        }
        return std::max(minVal, std::min(maxVal, value));
    }

    /**
     * @brief Denormal number elimination
     * @param value Input value
     * @return Value with denormals flushed to zero
     */
    static inline float flushDenormals(float value) {
        // Flush denormal numbers to zero
        if (std::abs(value) < 1e-25f) {
            return 0.0f;
        }
        return value;
    }
};

/**
 * @class SimpleCPUMonitor
 * @brief Lightweight CPU usage monitoring for basic overload detection
 *
 * Provides simple CPU monitoring without complex optimization infrastructure.
 */
class SimpleCPUMonitor {
public:
    SimpleCPUMonitor();
    ~SimpleCPUMonitor() = default;

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
     * @brief Check if CPU usage is above emergency threshold (95%)
     * @return True if emergency overload detected
     */
    bool isEmergencyOverload() const {
        return getCPUUsage() > 95.0f;
    }

    /**
     * @brief Check if CPU usage is above warning threshold (80%)
     * @return True if high CPU usage detected
     */
    bool isHighCPUUsage() const {
        return getCPUUsage() > 80.0f;
    }

    /**
     * @brief Reset statistics
     */
    void reset();

private:
    // Timing infrastructure
    std::chrono::high_resolution_clock::time_point mStartTime;
    double mTargetTimePerBuffer;           // Target processing time per buffer

    // Statistics (atomic for thread safety)
    std::atomic<float> mCurrentCPUUsage;   // Current buffer CPU usage

    bool mInitialized;
};

/**
 * @class EmergencyFallback
 * @brief Emergency fallback system for critical overload conditions
 *
 * Provides simple emergency bypass mechanisms when CPU usage becomes critical.
 */
class EmergencyFallback {
public:
    EmergencyFallback();
    ~EmergencyFallback() = default;

    /**
     * @brief Update emergency state based on CPU monitor
     * @param cpuMonitor Reference to CPU monitor
     */
    void updateEmergencyState(const SimpleCPUMonitor& cpuMonitor);

    /**
     * @brief Check if emergency mode is active
     * @return True if emergency mode is active
     */
    bool isEmergencyActive() const { return mEmergencyActive; }

    /**
     * @brief Get time constant multiplier for emergency mode
     * @return Multiplier for time constants (>1.0 = faster, less smooth)
     */
    float getTimeConstantMultiplier() const {
        if (mEmergencyActive) {
            return 4.0f; // Much faster smoothing in emergency
        } else if (mWarningActive) {
            return 2.0f; // Moderately faster smoothing
        }
        return 1.0f; // Normal smoothing
    }

    /**
     * @brief Check if parameter should bypass processing entirely
     * @return True if should bypass
     */
    bool shouldBypassProcessing() const {
        return mEmergencyActive;
    }

    /**
     * @brief Reset emergency state
     */
    void reset();

private:
    bool mEmergencyActive;
    bool mWarningActive;
    int mEmergencyCounter;
    int mWarningCounter;
    static constexpr int EMERGENCY_THRESHOLD = 5;  // 5 consecutive high readings
    static constexpr int WARNING_THRESHOLD = 3;    // 3 consecutive moderate readings
};

/**
 * @class SafetyOptimizer
 * @brief Simplified optimization system focused on real-time safety
 *
 * Provides essential real-time safety features without complex optimization
 * infrastructure that can cause compilation issues.
 */
class SafetyOptimizer {
public:
    SafetyOptimizer();
    ~SafetyOptimizer() = default;

    /**
     * @brief Initialize safety system
     * @param sampleRate Audio sample rate
     * @param bufferSize Processing buffer size
     */
    void initialize(double sampleRate, int bufferSize);

    /**
     * @brief Update safety state (call once per audio buffer)
     */
    void updatePerFrame();

    /**
     * @brief Get CPU monitor reference
     * @return CPU monitor
     */
    SimpleCPUMonitor& getCPUMonitor() { return mCPUMonitor; }

    /**
     * @brief Get emergency fallback reference
     * @return Emergency fallback
     */
    EmergencyFallback& getEmergencyFallback() { return mEmergencyFallback; }

    /**
     * @brief Check if emergency mode is active
     * @return True if emergency fallback is active
     */
    bool isEmergencyMode() const {
        return mEmergencyFallback.isEmergencyActive();
    }

    /**
     * @brief Get time constant multiplier for current safety level
     * @return Time constant multiplier
     */
    float getTimeConstantMultiplier() const {
        return mEmergencyFallback.getTimeConstantMultiplier();
    }

    /**
     * @brief Check if processing should be bypassed
     * @return True if should bypass
     */
    bool shouldBypassProcessing() const {
        return mEmergencyFallback.shouldBypassProcessing();
    }

    /**
     * @brief Enable/disable safety system
     * @param enabled True to enable safety features
     */
    void setEnabled(bool enabled) { mEnabled = enabled; }

    /**
     * @brief Check if safety system is enabled
     * @return True if enabled
     */
    bool isEnabled() const { return mEnabled; }

    /**
     * @brief Reset all safety state
     */
    void reset();

private:
    // Core safety components
    SimpleCPUMonitor mCPUMonitor;
    EmergencyFallback mEmergencyFallback;

    // System state
    bool mEnabled;
    bool mInitialized;
    double mSampleRate;
    int mBufferSize;

    // Performance tracking
    int mFrameCounter;
    static constexpr int UPDATE_INTERVAL = 64; // Update every 64 frames
};

} // namespace WaterStick