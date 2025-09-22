#pragma once

#include <cmath>
#include <algorithm>
#include <array>

namespace WaterStick {

/**
 * @class CascadedSmoother
 * @brief Multi-stage cascaded filtering system for superior parameter smoothing
 *
 * This class implements a configurable cascaded filtering system that provides
 * Gaussian-like response characteristics for smooth parameter transitions.
 * The cascaded approach eliminates artifacts common in single-stage filters
 * and provides superior frequency response shaping.
 *
 * Mathematical Foundation:
 * - For N cascade stages with total time constant τ_total:
 *   τ_stage = τ_total / N (equivalent response time)
 * - Each stage: y[n] = α*x[n] + (1-α)*y[n-1] where α = 1 - exp(-T/τ_stage)
 * - Gaussian approximation improves with increasing stage count
 * - Optimal stage count: 3-5 stages for real-time applications
 *
 * Key Features:
 * - Configurable stage count (1-5 stages)
 * - Real-time safe with bounded calculations
 * - Gaussian-like response for smooth transitions
 * - Equivalent response time preservation
 * - Superior artifact rejection vs single-stage filters
 */
class CascadedSmoother {
public:
    static constexpr int MAX_STAGES = 5;
    static constexpr int MIN_STAGES = 1;
    static constexpr int DEFAULT_STAGES = 3;

    /**
     * @brief Constructor with configurable parameters
     * @param sampleRate Sample rate in Hz
     * @param timeConstant Initial time constant in seconds
     * @param stageCount Number of cascade stages (1-5)
     */
    CascadedSmoother(double sampleRate = 44100.0,
                     float timeConstant = 0.01f,
                     int stageCount = DEFAULT_STAGES);

    ~CascadedSmoother() = default;

    /**
     * @brief Initialize or update sample rate
     * @param sampleRate New sample rate in Hz
     */
    void setSampleRate(double sampleRate);

    /**
     * @brief Set total time constant for equivalent response
     * @param timeConstant Total time constant in seconds (0.0001f to 1.0f)
     */
    void setTimeConstant(float timeConstant);

    /**
     * @brief Configure number of cascade stages
     * @param stageCount Number of stages (1-5)
     * @note Changing stage count resets internal state
     */
    void setStageCount(int stageCount);

    /**
     * @brief Process one sample through cascaded stages
     * @param input Input sample value
     * @return Smoothed output value
     */
    float processSample(float input);

    /**
     * @brief Reset all cascade stages to zero state
     */
    void reset();

    /**
     * @brief Reset to specific value (prevents initial transients)
     * @param value Initial value for all stages
     */
    void reset(float value);

    /**
     * @brief Get current total time constant
     * @return Total time constant in seconds
     */
    float getTimeConstant() const { return mTotalTimeConstant; }

    /**
     * @brief Get current stage count
     * @return Number of active cascade stages
     */
    int getStageCount() const { return mStageCount; }

    /**
     * @brief Get per-stage time constant
     * @return Individual stage time constant in seconds
     */
    float getStageTimeConstant() const { return mStageTimeConstant; }

    /**
     * @brief Get current smoothing coefficient
     * @return Smoothing coefficient (0-1)
     */
    float getSmoothingCoeff() const { return mSmoothingCoeff; }

    /**
     * @brief Check if smoother is settled (for optimization)
     * @param threshold Settling threshold (default: 0.001f)
     * @return True if output has settled within threshold
     */
    bool isSettled(float threshold = 0.001f) const;

    /**
     * @brief Get output from specific stage (for debugging/visualization)
     * @param stage Stage index (0 to stageCount-1)
     * @return Stage output value, or final output if stage invalid
     */
    float getStageOutput(int stage) const;

    /**
     * @brief Enable/disable processing (bypass mode)
     * @param enabled True to enable processing, false for bypass
     */
    void setEnabled(bool enabled) { mEnabled = enabled; }

    /**
     * @brief Check if processing is enabled
     * @return True if processing enabled
     */
    bool isEnabled() const { return mEnabled; }

private:
    // Core parameters
    double mSampleRate;              // Sample rate in Hz
    float mTotalTimeConstant;        // Total time constant in seconds
    float mStageTimeConstant;        // Per-stage time constant
    int mStageCount;                 // Number of active stages
    float mSmoothingCoeff;           // Current smoothing coefficient
    bool mEnabled;                   // Enable/disable processing

    // Cascade stage states
    std::array<float, MAX_STAGES> mStageStates;  // State for each cascade stage
    float mPreviousInput;                        // Previous input for settling detection
    float mPreviousOutput;                       // Previous output for settling detection

    // Sample period for coefficient calculation
    float mSamplePeriod;             // 1.0 / sampleRate

    /**
     * @brief Update smoothing coefficient based on current parameters
     */
    void updateCoefficients();

    /**
     * @brief Calculate per-stage time constant for equivalent response
     * @return Per-stage time constant in seconds
     */
    float calculateStageTimeConstant() const;

    /**
     * @brief Clamp value to specified range
     * @param value Input value
     * @param minVal Minimum value
     * @param maxVal Maximum value
     * @return Clamped value
     */
    inline float clamp(float value, float minVal, float maxVal) const {
        return std::max(minVal, std::min(maxVal, value));
    }

    /**
     * @brief Clamp integer to specified range
     * @param value Input value
     * @param minVal Minimum value
     * @param maxVal Maximum value
     * @return Clamped value
     */
    inline int clampInt(int value, int minVal, int maxVal) const {
        return std::max(minVal, std::min(maxVal, value));
    }
};

/**
 * @class MultiParameterCascadedSmoother
 * @brief Specialized cascaded smoother for multiple parameter smoothing
 *
 * Provides independent cascaded smoothing for multiple parameters with
 * shared configuration but independent state management. Optimized for
 * scenarios where multiple parameters need consistent smoothing behavior.
 */
class MultiParameterCascadedSmoother {
public:
    static constexpr int MAX_PARAMETERS = 8;

    /**
     * @brief Constructor
     * @param parameterCount Number of parameters to smooth (1-8)
     * @param sampleRate Sample rate in Hz
     * @param timeConstant Initial time constant in seconds
     * @param stageCount Number of cascade stages (1-5)
     */
    MultiParameterCascadedSmoother(int parameterCount = 4,
                                   double sampleRate = 44100.0,
                                   float timeConstant = 0.01f,
                                   int stageCount = CascadedSmoother::DEFAULT_STAGES);

    ~MultiParameterCascadedSmoother() = default;

    /**
     * @brief Initialize with parameter count and sample rate
     * @param parameterCount Number of parameters (1-8)
     * @param sampleRate Sample rate in Hz
     */
    void initialize(int parameterCount, double sampleRate);

    /**
     * @brief Set sample rate for all smoothers
     * @param sampleRate Sample rate in Hz
     */
    void setSampleRate(double sampleRate);

    /**
     * @brief Set time constant for all smoothers
     * @param timeConstant Time constant in seconds
     */
    void setTimeConstant(float timeConstant);

    /**
     * @brief Set stage count for all smoothers
     * @param stageCount Number of stages (1-5)
     */
    void setStageCount(int stageCount);

    /**
     * @brief Process sample for specific parameter
     * @param parameterIndex Parameter index (0 to parameterCount-1)
     * @param input Input sample value
     * @return Smoothed output value
     */
    float processSample(int parameterIndex, float input);

    /**
     * @brief Process samples for all parameters
     * @param inputs Array of input values (must have parameterCount elements)
     * @param outputs Array to store output values (must have parameterCount elements)
     */
    void processAllSamples(const float* inputs, float* outputs);

    /**
     * @brief Reset all parameter smoothers
     */
    void resetAll();

    /**
     * @brief Reset specific parameter smoother
     * @param parameterIndex Parameter index to reset
     */
    void resetParameter(int parameterIndex);

    /**
     * @brief Reset all smoothers to specific values
     * @param values Array of values (must have parameterCount elements)
     */
    void resetAll(const float* values);

    /**
     * @brief Get number of active parameters
     * @return Parameter count
     */
    int getParameterCount() const { return mParameterCount; }

    /**
     * @brief Enable/disable processing for all smoothers
     * @param enabled True to enable processing
     */
    void setEnabled(bool enabled);

    /**
     * @brief Get smoother for specific parameter (for advanced control)
     * @param parameterIndex Parameter index
     * @return Reference to CascadedSmoother, or first smoother if index invalid
     */
    CascadedSmoother& getSmoother(int parameterIndex);
    const CascadedSmoother& getSmoother(int parameterIndex) const;

private:
    std::array<CascadedSmoother, MAX_PARAMETERS> mSmoothers;  // Individual smoothers
    int mParameterCount;                                      // Number of active parameters
    bool mInitialized;                                        // Initialization state

    /**
     * @brief Validate parameter index
     * @param parameterIndex Index to validate
     * @return True if valid
     */
    bool isValidParameterIndex(int parameterIndex) const {
        return parameterIndex >= 0 && parameterIndex < mParameterCount;
    }
};

} // namespace WaterStick