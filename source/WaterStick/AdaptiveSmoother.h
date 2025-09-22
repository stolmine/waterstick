#pragma once

#include <cmath>
#include <algorithm>
#include "CascadedSmoother.h"

namespace WaterStick {

/**
 * @class AdaptiveSmoother
 * @brief Real-time adaptive parameter smoothing with velocity-based time constant adjustment
 *
 * This class extends the existing allpass smoothing framework with adaptive behavior:
 * - Velocity detection using finite differences
 * - Exponential curves for velocity-to-time-constant mapping
 * - Hysteresis to prevent oscillation between fast/slow modes
 * - Backwards compatibility with existing smoothing
 * - Configurable sensitivity and time constant ranges
 *
 * Mathematical Foundation:
 * - Exponential smoothing: y[n] = α*x[n] + (1-α)*y[n-1] where α = 1 - exp(-T/τ)
 * - Allpass interpolation: Δ ≈ (1-η)/(1+η) where η is the smoothing coefficient
 * - Velocity detection: v[n] = (x[n] - x[n-1]) / T (first-order finite difference)
 * - Adaptive time constant: τ(v) = τ_min + (τ_max - τ_min) * exp(-k * |v|)
 */
class AdaptiveSmoother {
public:
    /**
     * @brief Constructor with configurable parameters
     * @param sampleRate Sample rate in Hz
     * @param fastTimeConstant Fast time constant in seconds (for rapid changes)
     * @param slowTimeConstant Slow time constant in seconds (for stable regions)
     * @param velocitySensitivity Velocity sensitivity factor (higher = more responsive)
     * @param hysteresisThreshold Hysteresis threshold to prevent mode oscillation
     */
    AdaptiveSmoother(double sampleRate = 44100.0,
                     float fastTimeConstant = 0.001f,     // 1ms for fast changes
                     float slowTimeConstant = 0.010f,     // 10ms for stable regions
                     float velocitySensitivity = 1.0f,     // Unity sensitivity
                     float hysteresisThreshold = 0.1f);    // 10% hysteresis band

    ~AdaptiveSmoother() = default;

    /**
     * @brief Initialize or update sample rate
     * @param sampleRate New sample rate in Hz
     */
    void setSampleRate(double sampleRate);

    /**
     * @brief Configure adaptive behavior parameters
     * @param fastTimeConstant Time constant for rapid parameter changes (0.0001f to 0.01f)
     * @param slowTimeConstant Time constant for stable parameter regions (0.001f to 0.05f)
     * @param velocitySensitivity Velocity sensitivity multiplier (0.1f to 10.0f)
     * @param hysteresisThreshold Hysteresis threshold as fraction of parameter range (0.01f to 0.5f)
     */
    void setAdaptiveParameters(float fastTimeConstant,
                              float slowTimeConstant,
                              float velocitySensitivity,
                              float hysteresisThreshold);

    /**
     * @brief Process one sample with adaptive smoothing
     * @param input Current parameter value
     * @return Smoothed parameter value
     */
    float processSample(float input);

    /**
     * @brief Reset smoother state (call when audio processing starts/stops)
     */
    void reset();

    /**
     * @brief Get current smoothing time constant for display/debugging
     * @return Current time constant in seconds
     */
    float getCurrentTimeConstant() const { return mCurrentTimeConstant; }

    /**
     * @brief Get current velocity estimate for display/debugging
     * @return Current velocity estimate
     */
    float getCurrentVelocity() const { return mCurrentVelocity; }

    /**
     * @brief Check if smoother is in fast mode
     * @return True if in fast smoothing mode
     */
    bool isInFastMode() const { return mInFastMode; }

    /**
     * @brief Enable/disable adaptive behavior (fallback to fixed time constant)
     * @param enabled True to enable adaptive behavior
     * @param fixedTimeConstant Time constant to use when adaptive is disabled
     */
    void setAdaptiveEnabled(bool enabled, float fixedTimeConstant = 0.01f);

    /**
     * @brief Enable/disable cascaded filtering with adaptive stage selection
     * @param enabled True to enable cascaded filtering
     * @param maxStages Maximum number of cascade stages (1-5)
     * @param stageHysteresis Hysteresis factor for stage transitions (0.1-0.5)
     */
    void setCascadedEnabled(bool enabled, int maxStages = 3, float stageHysteresis = 0.2f);

    /**
     * @brief Configure velocity-to-stage mapping parameters
     * @param lowVelocityThreshold Velocity threshold for maximum stages
     * @param highVelocityThreshold Velocity threshold for minimum stages
     * @param velocityScaling Velocity scaling factor for stage calculation
     */
    void setStageMapping(float lowVelocityThreshold = 0.1f,
                        float highVelocityThreshold = 2.0f,
                        float velocityScaling = 1.0f);

    /**
     * @brief Get current cascade stage count for display/debugging
     * @return Current number of active cascade stages (1 if cascaded disabled)
     */
    int getCurrentStageCount() const { return mCascadedEnabled ? mCascadedSmoother.getStageCount() : 1; }

    /**
     * @brief Check if cascaded filtering is enabled
     * @return True if cascaded filtering is active
     */
    bool isCascadedEnabled() const { return mCascadedEnabled; }

private:
    // Core smoothing parameters
    double mSampleRate;
    float mFastTimeConstant;     // Fast time constant (rapid changes)
    float mSlowTimeConstant;     // Slow time constant (stable regions)
    float mVelocitySensitivity;  // Velocity sensitivity factor
    float mHysteresisThreshold;  // Hysteresis threshold

    // Adaptive behavior control
    bool mAdaptiveEnabled;       // Enable/disable adaptive behavior
    float mFixedTimeConstant;    // Fixed time constant when adaptive disabled

    // Cascaded filtering control
    bool mCascadedEnabled;       // Enable/disable cascaded filtering
    CascadedSmoother mCascadedSmoother;  // Internal cascaded smoother
    int mMaxStages;              // Maximum number of cascade stages
    float mStageHysteresis;      // Hysteresis factor for stage transitions

    // Velocity-to-stage mapping parameters
    float mLowVelocityThreshold;  // Velocity for maximum stages
    float mHighVelocityThreshold; // Velocity for minimum stages
    float mVelocityScaling;       // Velocity scaling factor

    // Stage transition state
    int mTargetStageCount;        // Target stage count from velocity mapping
    int mCurrentStageCount;       // Current active stage count

    // State variables for allpass interpolation
    float mPrevInput;            // Previous input sample
    float mAllpassState;         // Allpass filter state
    float mSmoothingCoeff;       // Current smoothing coefficient

    // Velocity detection state
    float mCurrentVelocity;      // Current velocity estimate
    float mPrevVelocity;         // Previous velocity for hysteresis
    float mCurrentTimeConstant;  // Current adaptive time constant

    // Hysteresis state management
    bool mInFastMode;            // Current smoothing mode
    float mFastThreshold;        // Threshold to enter fast mode
    float mSlowThreshold;        // Threshold to exit fast mode

    // Sample time for velocity calculation
    float mSampleTime;           // 1.0 / sampleRate

    /**
     * @brief Calculate velocity using first-order finite difference
     * @param input Current input sample
     * @return Velocity estimate
     */
    float calculateVelocity(float input);

    /**
     * @brief Map velocity to adaptive time constant using exponential curve
     * @param velocity Absolute velocity value
     * @return Adaptive time constant
     */
    float velocityToTimeConstant(float velocity);

    /**
     * @brief Update smoothing coefficient based on current time constant
     * @param timeConstant Time constant in seconds
     */
    void updateSmoothingCoeff(float timeConstant);

    /**
     * @brief Apply hysteresis logic to determine smoothing mode
     * @param velocity Current velocity estimate
     */
    void updateSmoothingMode(float velocity);

    /**
     * @brief Calculate target stage count based on velocity
     * @param velocity Current velocity estimate
     * @return Target stage count (1 to mMaxStages)
     */
    int calculateTargetStageCount(float velocity);

    /**
     * @brief Update cascade stage count with hysteresis
     * @param velocity Current velocity estimate
     */
    void updateCascadeStages(float velocity);

    /**
     * @brief Get effective time constant for current configuration
     * @param baseTimeConstant Base time constant before cascade adjustment
     * @return Effective time constant accounting for cascade stages
     */
    float getEffectiveTimeConstant(float baseTimeConstant) const;

    /**
     * @brief Clamp value to specified range (optimized inline)
     * @param value Input value
     * @param minVal Minimum value
     * @param maxVal Maximum value
     * @return Clamped value
     */
    inline float clamp(float value, float minVal, float maxVal) const {
        return std::max(minVal, std::min(maxVal, value));
    }

    /**
     * @brief Clamp integer to specified range (optimized inline)
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
 * @class CombParameterSmoother
 * @brief Specialized adaptive smoother for comb processor parameters
 *
 * Extends AdaptiveSmoother with parameter-specific optimizations:
 * - Separate smoothers for comb size and pitch CV
 * - Parameter-aware velocity scaling
 * - Integration with existing CombProcessor architecture
 */
class CombParameterSmoother {
public:
    CombParameterSmoother();
    ~CombParameterSmoother() = default;

    /**
     * @brief Initialize smoothers with sample rate
     * @param sampleRate Sample rate in Hz
     */
    void initialize(double sampleRate);

    /**
     * @brief Configure adaptive behavior for both parameters
     * @param combSizeSensitivity Velocity sensitivity for comb size parameter
     * @param pitchCVSensitivity Velocity sensitivity for pitch CV parameter
     * @param fastTimeConstant Fast time constant for both parameters
     * @param slowTimeConstant Slow time constant for both parameters
     */
    void setAdaptiveParameters(float combSizeSensitivity = 2.0f,     // More sensitive to size changes
                              float pitchCVSensitivity = 1.5f,      // Moderately sensitive to pitch changes
                              float fastTimeConstant = 0.0005f,     // 0.5ms for rapid changes
                              float slowTimeConstant = 0.008f);     // 8ms for stable regions

    /**
     * @brief Process comb size parameter with adaptive smoothing
     * @param combSize Raw comb size in seconds
     * @return Smoothed comb size
     */
    float processComSize(float combSize);

    /**
     * @brief Process pitch CV parameter with adaptive smoothing
     * @param pitchCV Raw pitch CV value
     * @return Smoothed pitch CV
     */
    float processPitchCV(float pitchCV);

    /**
     * @brief Reset both smoothers
     */
    void reset();

    /**
     * @brief Enable/disable adaptive behavior
     * @param enabled True to enable adaptive behavior
     */
    void setAdaptiveEnabled(bool enabled);

    /**
     * @brief Enable/disable cascaded filtering for both parameters
     * @param enabled True to enable cascaded filtering
     * @param maxStages Maximum number of cascade stages (1-5)
     * @param stageHysteresis Hysteresis factor for stage transitions
     */
    void setCascadedEnabled(bool enabled, int maxStages = 3, float stageHysteresis = 0.2f);

    /**
     * @brief Configure velocity-to-stage mapping for both parameters
     * @param lowVelocityThreshold Velocity threshold for maximum stages
     * @param highVelocityThreshold Velocity threshold for minimum stages
     * @param velocityScaling Velocity scaling factor for stage calculation
     */
    void setStageMapping(float lowVelocityThreshold = 0.1f,
                        float highVelocityThreshold = 2.0f,
                        float velocityScaling = 1.0f);

    /**
     * @brief Get current smoothing status for debugging
     * @param combSizeTimeConstant [out] Current comb size time constant
     * @param pitchCVTimeConstant [out] Current pitch CV time constant
     * @param combSizeVelocity [out] Current comb size velocity
     * @param pitchCVVelocity [out] Current pitch CV velocity
     */
    void getDebugInfo(float& combSizeTimeConstant, float& pitchCVTimeConstant,
                     float& combSizeVelocity, float& pitchCVVelocity) const;

    /**
     * @brief Get extended debug information including cascaded smoothing status
     * @param combSizeStages [out] Current comb size cascade stages
     * @param pitchCVStages [out] Current pitch CV cascade stages
     * @param combSizeCascaded [out] True if comb size cascaded filtering enabled
     * @param pitchCVCascaded [out] True if pitch CV cascaded filtering enabled
     */
    void getExtendedDebugInfo(int& combSizeStages, int& pitchCVStages,
                             bool& combSizeCascaded, bool& pitchCVCascaded) const;

private:
    AdaptiveSmoother mCombSizeSmoother;  // Smoother for comb size parameter
    AdaptiveSmoother mPitchCVSmoother;   // Smoother for pitch CV parameter

    bool mInitialized;                   // Initialization state flag
};

} // namespace WaterStick