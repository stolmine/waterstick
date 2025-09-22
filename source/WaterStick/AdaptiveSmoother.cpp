#include "AdaptiveSmoother.h"
#include <cmath>
#include <algorithm>

namespace WaterStick {

// AdaptiveSmoother Implementation

AdaptiveSmoother::AdaptiveSmoother(double sampleRate,
                                   float fastTimeConstant,
                                   float slowTimeConstant,
                                   float velocitySensitivity,
                                   float hysteresisThreshold)
    : mSampleRate(sampleRate)
    , mFastTimeConstant(fastTimeConstant)
    , mSlowTimeConstant(slowTimeConstant)
    , mVelocitySensitivity(velocitySensitivity)
    , mHysteresisThreshold(hysteresisThreshold)
    , mAdaptiveEnabled(true)
    , mFixedTimeConstant(0.01f)
    , mCascadedEnabled(false)
    , mCascadedSmoother(sampleRate, slowTimeConstant, 1)
    , mMaxStages(3)
    , mStageHysteresis(0.2f)
    , mLowVelocityThreshold(0.1f)
    , mHighVelocityThreshold(2.0f)
    , mVelocityScaling(1.0f)
    , mTargetStageCount(1)
    , mCurrentStageCount(1)
    , mPrevInput(0.0f)
    , mAllpassState(0.0f)
    , mSmoothingCoeff(0.0f)
    , mCurrentVelocity(0.0f)
    , mPrevVelocity(0.0f)
    , mCurrentTimeConstant(slowTimeConstant)
    , mInFastMode(false)
    , mFastThreshold(0.0f)
    , mSlowThreshold(0.0f)
    , mSampleTime(1.0f / static_cast<float>(sampleRate))
{
    // Calculate hysteresis thresholds
    // These are velocity thresholds that determine mode switching
    mFastThreshold = mHysteresisThreshold * mVelocitySensitivity;
    mSlowThreshold = mFastThreshold * 0.5f; // Exit fast mode at half the entry threshold

    // Initialize with slow time constant
    updateSmoothingCoeff(mSlowTimeConstant);
}

void AdaptiveSmoother::setSampleRate(double sampleRate)
{
    mSampleRate = sampleRate;
    mSampleTime = 1.0f / static_cast<float>(sampleRate);

    // Update cascaded smoother sample rate
    mCascadedSmoother.setSampleRate(sampleRate);

    // Recalculate smoothing coefficient for current time constant
    updateSmoothingCoeff(mCurrentTimeConstant);
}

void AdaptiveSmoother::setAdaptiveParameters(float fastTimeConstant,
                                            float slowTimeConstant,
                                            float velocitySensitivity,
                                            float hysteresisThreshold)
{
    // Clamp parameters to safe ranges
    mFastTimeConstant = clamp(fastTimeConstant, 0.0001f, 0.01f);
    mSlowTimeConstant = clamp(slowTimeConstant, 0.001f, 0.05f);
    mVelocitySensitivity = clamp(velocitySensitivity, 0.1f, 10.0f);
    mHysteresisThreshold = clamp(hysteresisThreshold, 0.01f, 0.5f);

    // Ensure fast <= slow time constants
    if (mFastTimeConstant > mSlowTimeConstant) {
        mFastTimeConstant = mSlowTimeConstant * 0.5f;
    }

    // Recalculate hysteresis thresholds
    mFastThreshold = mHysteresisThreshold * mVelocitySensitivity;
    mSlowThreshold = mFastThreshold * 0.5f;

    // Update current smoothing coefficient
    updateSmoothingCoeff(mCurrentTimeConstant);
}

float AdaptiveSmoother::processSample(float input)
{
    // Calculate velocity using finite difference
    float velocity = calculateVelocity(input);

    // Update velocity state
    mPrevVelocity = mCurrentVelocity;
    mCurrentVelocity = velocity;

    // Determine adaptive time constant
    if (mAdaptiveEnabled) {
        // Apply hysteresis logic to determine smoothing mode
        updateSmoothingMode(std::abs(velocity));

        // Map velocity to time constant using exponential curve
        mCurrentTimeConstant = velocityToTimeConstant(std::abs(velocity));

        // Update cascade stages if enabled
        if (mCascadedEnabled) {
            updateCascadeStages(std::abs(velocity));
        }

        // Update smoothing coefficient
        updateSmoothingCoeff(mCurrentTimeConstant);
    } else {
        // Use fixed time constant
        mCurrentTimeConstant = mFixedTimeConstant;
        updateSmoothingCoeff(mFixedTimeConstant);
    }

    float smoothedOutput;

    if (mCascadedEnabled) {
        // Use cascaded smoother
        float effectiveTimeConstant = getEffectiveTimeConstant(mCurrentTimeConstant);
        mCascadedSmoother.setTimeConstant(effectiveTimeConstant);
        smoothedOutput = mCascadedSmoother.processSample(input);
    } else {
        // Use traditional allpass interpolation for smooth delay modulation
        // Using Stanford formula: Δ ≈ (1-η)/(1+η) where η is the smoothing coefficient
        float inputDiff = input - mPrevInput;
        float eta = mSmoothingCoeff;
        float allpassCoeff = (1.0f - eta) / (1.0f + eta);

        // Update allpass state with the input difference
        mAllpassState = allpassCoeff * (inputDiff + mAllpassState);

        // Calculate smoothed output
        smoothedOutput = mPrevInput + mAllpassState;
    }

    // Update previous input for next sample
    mPrevInput = smoothedOutput;

    return smoothedOutput;
}

void AdaptiveSmoother::reset()
{
    mPrevInput = 0.0f;
    mAllpassState = 0.0f;
    mCurrentVelocity = 0.0f;
    mPrevVelocity = 0.0f;
    mInFastMode = false;

    // Reset cascade stages
    mTargetStageCount = 1;
    mCurrentStageCount = 1;
    mCascadedSmoother.reset();
    mCascadedSmoother.setStageCount(1);

    // Reset to slow time constant
    mCurrentTimeConstant = mSlowTimeConstant;
    updateSmoothingCoeff(mSlowTimeConstant);
}

void AdaptiveSmoother::setAdaptiveEnabled(bool enabled, float fixedTimeConstant)
{
    mAdaptiveEnabled = enabled;
    mFixedTimeConstant = clamp(fixedTimeConstant, 0.0001f, 0.05f);

    if (!enabled) {
        mCurrentTimeConstant = mFixedTimeConstant;
        updateSmoothingCoeff(mFixedTimeConstant);
        mInFastMode = false;

        // Reset cascade stages to single stage when adaptive disabled
        if (mCascadedEnabled) {
            mTargetStageCount = 1;
            mCurrentStageCount = 1;
            mCascadedSmoother.setStageCount(1);
        }
    }
}

float AdaptiveSmoother::calculateVelocity(float input)
{
    // First-order finite difference: v[n] = (x[n] - x[n-1]) / T
    return (input - mPrevInput) / mSampleTime;
}

float AdaptiveSmoother::velocityToTimeConstant(float velocity)
{
    // Exponential mapping: τ(v) = τ_min + (τ_max - τ_min) * exp(-k * |v|)
    // where k is the velocity sensitivity factor

    float velocityMagnitude = velocity * mVelocitySensitivity;
    float exponentialFactor = std::exp(-velocityMagnitude);

    // Interpolate between fast and slow time constants
    float adaptiveTimeConstant = mFastTimeConstant +
                                (mSlowTimeConstant - mFastTimeConstant) * exponentialFactor;

    return clamp(adaptiveTimeConstant, mFastTimeConstant, mSlowTimeConstant);
}

void AdaptiveSmoother::updateSmoothingCoeff(float timeConstant)
{
    // Calculate smoothing coefficient: α = 1 - exp(-T/τ)
    // For allpass interpolation, we need η = exp(-T/τ)
    mSmoothingCoeff = std::exp(-mSampleTime / timeConstant);
}

void AdaptiveSmoother::updateSmoothingMode(float velocity)
{
    // Hysteresis logic to prevent oscillation between fast/slow modes
    if (!mInFastMode && velocity > mFastThreshold) {
        // Enter fast mode
        mInFastMode = true;
    } else if (mInFastMode && velocity < mSlowThreshold) {
        // Exit fast mode
        mInFastMode = false;
    }

    // The actual time constant is determined by velocityToTimeConstant(),
    // but the mode flag can be used for additional logic or display purposes
}

// CombParameterSmoother Implementation

CombParameterSmoother::CombParameterSmoother()
    : mInitialized(false)
{
}

void CombParameterSmoother::initialize(double sampleRate)
{
    // Initialize both smoothers with parameter-specific settings
    mCombSizeSmoother.setSampleRate(sampleRate);
    mPitchCVSmoother.setSampleRate(sampleRate);

    // Set default adaptive parameters
    setAdaptiveParameters();

    mInitialized = true;
}

void CombParameterSmoother::setAdaptiveParameters(float combSizeSensitivity,
                                                  float pitchCVSensitivity,
                                                  float fastTimeConstant,
                                                  float slowTimeConstant)
{
    // Configure comb size smoother with higher sensitivity
    // Comb size changes are more audible and need faster response
    mCombSizeSmoother.setAdaptiveParameters(
        fastTimeConstant,                   // 0.5ms fast response
        slowTimeConstant,                   // 8ms slow response
        combSizeSensitivity,                // 2.0x sensitivity
        0.05f                              // 5% hysteresis threshold
    );

    // Configure pitch CV smoother with moderate sensitivity
    // Pitch changes are important but can tolerate slightly more smoothing
    mPitchCVSmoother.setAdaptiveParameters(
        fastTimeConstant * 1.5f,           // 0.75ms fast response (slightly slower)
        slowTimeConstant * 1.2f,           // 9.6ms slow response (slightly slower)
        pitchCVSensitivity,                // 1.5x sensitivity
        0.08f                              // 8% hysteresis threshold
    );
}

float CombParameterSmoother::processComSize(float combSize)
{
    if (!mInitialized) {
        return combSize; // Pass through if not initialized
    }

    return mCombSizeSmoother.processSample(combSize);
}

float CombParameterSmoother::processPitchCV(float pitchCV)
{
    if (!mInitialized) {
        return pitchCV; // Pass through if not initialized
    }

    return mPitchCVSmoother.processSample(pitchCV);
}

void CombParameterSmoother::reset()
{
    mCombSizeSmoother.reset();
    mPitchCVSmoother.reset();
}

void CombParameterSmoother::setAdaptiveEnabled(bool enabled)
{
    mCombSizeSmoother.setAdaptiveEnabled(enabled, 0.008f);  // 8ms fixed for comb size
    mPitchCVSmoother.setAdaptiveEnabled(enabled, 0.010f);   // 10ms fixed for pitch CV
}

void CombParameterSmoother::setCascadedEnabled(bool enabled, int maxStages, float stageHysteresis)
{
    mCombSizeSmoother.setCascadedEnabled(enabled, maxStages, stageHysteresis);
    mPitchCVSmoother.setCascadedEnabled(enabled, maxStages, stageHysteresis);
}

void CombParameterSmoother::setStageMapping(float lowVelocityThreshold,
                                           float highVelocityThreshold,
                                           float velocityScaling)
{
    // Comb size parameter needs more responsive stage transitions
    // due to its direct impact on pitch/timbre
    mCombSizeSmoother.setStageMapping(
        lowVelocityThreshold * 0.8f,    // Slightly lower threshold for faster response
        highVelocityThreshold * 1.2f,   // Wider range for more gradual transitions
        velocityScaling * 1.5f          // Higher sensitivity for comb size changes
    );

    // Pitch CV can use standard settings
    mPitchCVSmoother.setStageMapping(
        lowVelocityThreshold,
        highVelocityThreshold,
        velocityScaling
    );
}

void CombParameterSmoother::getDebugInfo(float& combSizeTimeConstant,
                                        float& pitchCVTimeConstant,
                                        float& combSizeVelocity,
                                        float& pitchCVVelocity) const
{
    combSizeTimeConstant = mCombSizeSmoother.getCurrentTimeConstant();
    pitchCVTimeConstant = mPitchCVSmoother.getCurrentTimeConstant();
    combSizeVelocity = mCombSizeSmoother.getCurrentVelocity();
    pitchCVVelocity = mPitchCVSmoother.getCurrentVelocity();
}

void CombParameterSmoother::getExtendedDebugInfo(int& combSizeStages, int& pitchCVStages,
                                                 bool& combSizeCascaded, bool& pitchCVCascaded) const
{
    combSizeStages = mCombSizeSmoother.getCurrentStageCount();
    pitchCVStages = mPitchCVSmoother.getCurrentStageCount();
    combSizeCascaded = mCombSizeSmoother.isCascadedEnabled();
    pitchCVCascaded = mPitchCVSmoother.isCascadedEnabled();
}

void AdaptiveSmoother::setCascadedEnabled(bool enabled, int maxStages, float stageHysteresis)
{
    mCascadedEnabled = enabled;
    mMaxStages = clampInt(maxStages, 1, CascadedSmoother::MAX_STAGES);
    mStageHysteresis = clamp(stageHysteresis, 0.05f, 0.5f);

    if (enabled) {
        // Initialize cascaded smoother with current settings
        mCascadedSmoother.setSampleRate(mSampleRate);
        mCascadedSmoother.setTimeConstant(mCurrentTimeConstant);
        mCascadedSmoother.setStageCount(1);

        // Reset to current input value to prevent transients
        mCascadedSmoother.reset(mPrevInput);

        // Initialize stage counts
        mTargetStageCount = 1;
        mCurrentStageCount = 1;
    } else {
        // Reset to single stage when disabled
        mTargetStageCount = 1;
        mCurrentStageCount = 1;
    }
}

void AdaptiveSmoother::setStageMapping(float lowVelocityThreshold,
                                      float highVelocityThreshold,
                                      float velocityScaling)
{
    mLowVelocityThreshold = clamp(lowVelocityThreshold, 0.01f, 1.0f);
    mHighVelocityThreshold = clamp(highVelocityThreshold, mLowVelocityThreshold, 10.0f);
    mVelocityScaling = clamp(velocityScaling, 0.1f, 5.0f);

    // Ensure high threshold is greater than low threshold
    if (mHighVelocityThreshold <= mLowVelocityThreshold) {
        mHighVelocityThreshold = mLowVelocityThreshold * 2.0f;
    }
}

int AdaptiveSmoother::calculateTargetStageCount(float velocity)
{
    if (!mCascadedEnabled) {
        return 1;
    }

    // Apply velocity scaling
    float scaledVelocity = velocity * mVelocityScaling;

    // Map velocity to stage count using inverse relationship
    // Low velocity (stable) -> more stages (better smoothing)
    // High velocity (changing) -> fewer stages (better responsiveness)

    if (scaledVelocity <= mLowVelocityThreshold) {
        // Use maximum stages for very stable parameters
        return mMaxStages;
    } else if (scaledVelocity >= mHighVelocityThreshold) {
        // Use minimum stages for rapidly changing parameters
        return 1;
    } else {
        // Interpolate between min and max stages
        float velocityRange = mHighVelocityThreshold - mLowVelocityThreshold;
        float normalizedVelocity = (scaledVelocity - mLowVelocityThreshold) / velocityRange;

        // Use exponential curve for more natural stage transitions
        // Higher exponent = more aggressive stage reduction
        float stageFactor = 1.0f - std::pow(normalizedVelocity, 0.7f);
        int targetStages = 1 + static_cast<int>(stageFactor * (mMaxStages - 1));

        return clampInt(targetStages, 1, mMaxStages);
    }
}

void AdaptiveSmoother::updateCascadeStages(float velocity)
{
    if (!mCascadedEnabled) {
        return;
    }

    // Calculate target stage count based on velocity
    mTargetStageCount = calculateTargetStageCount(velocity);

    // Apply hysteresis to prevent rapid stage switching
    int stageDifference = std::abs(mTargetStageCount - mCurrentStageCount);
    float hysteresisThreshold = mStageHysteresis * mMaxStages;

    if (stageDifference >= static_cast<int>(std::ceil(hysteresisThreshold))) {
        // Update stage count with smooth transition
        if (mTargetStageCount > mCurrentStageCount) {
            // Gradually increase stages
            mCurrentStageCount = std::min(mCurrentStageCount + 1, mTargetStageCount);
        } else if (mTargetStageCount < mCurrentStageCount) {
            // Gradually decrease stages
            mCurrentStageCount = std::max(mCurrentStageCount - 1, mTargetStageCount);
        }

        // Apply stage count to cascaded smoother
        mCascadedSmoother.setStageCount(mCurrentStageCount);
    }
}

float AdaptiveSmoother::getEffectiveTimeConstant(float baseTimeConstant) const
{
    if (!mCascadedEnabled || mCurrentStageCount <= 1) {
        return baseTimeConstant;
    }

    // For cascaded systems, the effective time constant needs adjustment
    // to maintain equivalent response time across different stage counts
    // Using the design document formula: τ_stage = τ_total / N
    return baseTimeConstant;
}

} // namespace WaterStick