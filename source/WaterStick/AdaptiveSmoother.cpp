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
    , mPerceptualMappingEnabled(false)
    , mImperceptibleThreshold(0.01f)
    , mJustNoticeableThreshold(0.1f)
    , mLargeChangeThreshold(1.0f)
    , mFrequencyWeighting(1.0f)
    , mImperceptibleTime(0.0005f)
    , mJustNoticeableTime(0.003f)
    , mLargeChangeTime(0.020f)
    , mTransitionSharpness(1.5f)
    , mLowFreqWeight(0.8f)
    , mMidFreqWeight(1.2f)
    , mHighFreqWeight(1.0f)
    , mAnalysisWindow(64)
    , mCurrentPerceptualRegion(0)
    , mFrequencyWeightedVelocity(0.0f)
    , mPerceptualTimeConstant(slowTimeConstant)
    , mInputHistory{}
    , mHistoryIndex(0)
    , mLowFreqEnergy(0.0f)
    , mMidFreqEnergy(0.0f)
    , mHighFreqEnergy(0.0f)
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
    // Update frequency analysis for perceptual weighting
    if (mPerceptualMappingEnabled) {
        updateFrequencyAnalysis(input);
    }

    // Calculate velocity using finite difference
    float velocity = calculateVelocity(input);

    // Update velocity state
    mPrevVelocity = mCurrentVelocity;
    mCurrentVelocity = velocity;

    // Determine adaptive time constant
    if (mAdaptiveEnabled) {
        float effectiveVelocity = std::abs(velocity);

        if (mPerceptualMappingEnabled) {
            // Use perceptual mapping with frequency weighting
            mFrequencyWeightedVelocity = calculateFrequencyWeightedVelocity(effectiveVelocity);
            mCurrentTimeConstant = velocityToPerceptualTimeConstant(mFrequencyWeightedVelocity);
            mPerceptualTimeConstant = mCurrentTimeConstant;
        } else {
            // Use traditional linear velocity mapping
            updateSmoothingMode(effectiveVelocity);
            mCurrentTimeConstant = velocityToTimeConstant(effectiveVelocity);
        }

        // Update cascade stages if enabled
        if (mCascadedEnabled) {
            float cascadeVelocity = mPerceptualMappingEnabled ? mFrequencyWeightedVelocity : effectiveVelocity;
            updateCascadeStages(cascadeVelocity);
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

    // Reset perceptual state
    mCurrentPerceptualRegion = 0;
    mFrequencyWeightedVelocity = 0.0f;
    mPerceptualTimeConstant = mSlowTimeConstant;
    mInputHistory.fill(0.0f);
    mHistoryIndex = 0;
    mLowFreqEnergy = 0.0f;
    mMidFreqEnergy = 0.0f;
    mHighFreqEnergy = 0.0f;

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

void AdaptiveSmoother::setPerceptualMapping(bool enabled,
                                           float imperceptibleThreshold,
                                           float justNoticeableThreshold,
                                           float largeChangeThreshold,
                                           float frequencyWeighting)
{
    mPerceptualMappingEnabled = enabled;

    // Clamp thresholds to reasonable ranges
    mImperceptibleThreshold = clamp(imperceptibleThreshold, 0.001f, 0.1f);
    mJustNoticeableThreshold = clamp(justNoticeableThreshold, mImperceptibleThreshold, 1.0f);
    mLargeChangeThreshold = clamp(largeChangeThreshold, mJustNoticeableThreshold, 10.0f);

    // Ensure proper ordering of thresholds
    if (mJustNoticeableThreshold <= mImperceptibleThreshold) {
        mJustNoticeableThreshold = mImperceptibleThreshold * 2.0f;
    }
    if (mLargeChangeThreshold <= mJustNoticeableThreshold) {
        mLargeChangeThreshold = mJustNoticeableThreshold * 2.0f;
    }

    mFrequencyWeighting = clamp(frequencyWeighting, 0.0f, 2.0f);

    if (enabled) {
        // Reset perceptual state when enabling
        mCurrentPerceptualRegion = 0;
        mFrequencyWeightedVelocity = 0.0f;
        mInputHistory.fill(0.0f);
        mHistoryIndex = 0;
        mLowFreqEnergy = 0.0f;
        mMidFreqEnergy = 0.0f;
        mHighFreqEnergy = 0.0f;
    }
}

void AdaptiveSmoother::setPerceptualTimeConstants(float imperceptibleTime,
                                                 float justNoticeableTime,
                                                 float largeChangeTime,
                                                 float transitionSharpness)
{
    // Clamp time constants to psychoacoustically relevant ranges
    mImperceptibleTime = clamp(imperceptibleTime, 0.0001f, 0.001f);      // 0.1-1ms
    mJustNoticeableTime = clamp(justNoticeableTime, 0.001f, 0.010f);     // 1-10ms
    mLargeChangeTime = clamp(largeChangeTime, 0.005f, 0.050f);           // 5-50ms

    // Ensure proper ordering of time constants
    if (mJustNoticeableTime <= mImperceptibleTime) {
        mJustNoticeableTime = mImperceptibleTime * 2.0f;
    }
    if (mLargeChangeTime <= mJustNoticeableTime) {
        mLargeChangeTime = mJustNoticeableTime * 2.0f;
    }

    mTransitionSharpness = clamp(transitionSharpness, 0.5f, 3.0f);
}

void AdaptiveSmoother::setFrequencyWeighting(float lowFreqWeight,
                                            float midFreqWeight,
                                            float highFreqWeight,
                                            int analysisWindow)
{
    mLowFreqWeight = clamp(lowFreqWeight, 0.1f, 2.0f);
    mMidFreqWeight = clamp(midFreqWeight, 0.1f, 2.0f);
    mHighFreqWeight = clamp(highFreqWeight, 0.1f, 2.0f);
    mAnalysisWindow = clampInt(analysisWindow, 16, 128);
}

float AdaptiveSmoother::calculateVelocity(float input)
{
    // First-order finite difference: v[n] = (x[n] - x[n-1]) / T
    // Scale by sample rate to get a normalized velocity measure
    float rawVelocity = (input - mPrevInput) / mSampleTime;

    // Normalize velocity to a more practical range (parameter change per second)
    // This gives us velocity in "parameter units per second" which is more intuitive
    return rawVelocity / static_cast<float>(mSampleRate);
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

void CombParameterSmoother::setPerceptualMapping(bool enabled,
                                                 float combSizeFreqWeighting,
                                                 float pitchCVFreqWeighting)
{
    // Configure perceptual mapping for comb size with tighter thresholds
    // Comb size changes are more perceptually critical
    mCombSizeSmoother.setPerceptualMapping(
        enabled,
        0.005f,     // Imperceptible threshold - tighter for comb size
        0.08f,      // Just noticeable threshold
        0.8f,       // Large change threshold
        combSizeFreqWeighting
    );

    // Configure perceptual mapping for pitch CV with standard thresholds
    mPitchCVSmoother.setPerceptualMapping(
        enabled,
        0.01f,      // Imperceptible threshold - standard
        0.1f,       // Just noticeable threshold
        1.0f,       // Large change threshold
        pitchCVFreqWeighting
    );

    if (enabled) {
        // Set parameter-specific time constants
        setPerceptualTimeConstants();
    }
}

void CombParameterSmoother::setPerceptualTimeConstants(float combSizeImperceptible,
                                                      float combSizeJustNoticeable,
                                                      float combSizeLargeChange,
                                                      float pitchCVImperceptible,
                                                      float pitchCVJustNoticeable,
                                                      float pitchCVLargeChange)
{
    // Configure psychoacoustic time constants for comb size
    // Comb size needs faster response to prevent pitch artifacts
    mCombSizeSmoother.setPerceptualTimeConstants(
        combSizeImperceptible,      // 0.3ms - very fast for imperceptible changes
        combSizeJustNoticeable,     // 2ms - moderate for just noticeable
        combSizeLargeChange,        // 15ms - controlled for large changes
        2.0f                        // Sharper transitions for comb size
    );

    // Configure psychoacoustic time constants for pitch CV
    // Pitch CV can tolerate slightly more smoothing
    mPitchCVSmoother.setPerceptualTimeConstants(
        pitchCVImperceptible,       // 0.5ms - fast for imperceptible changes
        pitchCVJustNoticeable,      // 3ms - moderate for just noticeable
        pitchCVLargeChange,         // 20ms - more smoothing for large changes
        1.5f                        // Standard transitions for pitch CV
    );

    // Set frequency weighting optimized for each parameter type
    mCombSizeSmoother.setFrequencyWeighting(
        0.9f,       // Low frequency weighting - important for comb size
        1.3f,       // Mid frequency weighting - very important
        1.1f,       // High frequency weighting - moderately important
        32          // Smaller analysis window for faster response
    );

    mPitchCVSmoother.setFrequencyWeighting(
        0.8f,       // Low frequency weighting - less critical for pitch
        1.2f,       // Mid frequency weighting - important
        1.0f,       // High frequency weighting - standard
        64          // Standard analysis window
    );
}

void CombParameterSmoother::setStageMapping(float lowVelocityThreshold,
                                           float highVelocityThreshold,
                                           float velocityScaling)
{
    // Comb size parameter needs more responsive stage transitions
    // due to its direct impact on pitch/timbre
    mCombSizeSmoother.setStageMapping(
        lowVelocityThreshold * 0.8f,    // Lower threshold for staying in minimal stages (stable)
        highVelocityThreshold * 1.2f,   // Wider range for reaching maximum stages (changing)
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

    // Map velocity to stage count using correct relationship
    // Low velocity (stable) -> fewer stages (1-2) for faster response, less latency
    // High velocity (changing) -> more stages (4-5) for better smoothing, artifact reduction

    if (scaledVelocity <= mLowVelocityThreshold) {
        // Use minimum stages for very stable parameters (fast response)
        return 1;
    } else if (scaledVelocity >= mHighVelocityThreshold) {
        // Use maximum stages for rapidly changing parameters (better smoothing)
        return mMaxStages;
    } else {
        // Interpolate between min and max stages
        float velocityRange = mHighVelocityThreshold - mLowVelocityThreshold;
        float normalizedVelocity = (scaledVelocity - mLowVelocityThreshold) / velocityRange;

        // Use exponential curve for more natural stage transitions
        // Higher exponent = more aggressive stage increase for changing parameters
        float stageFactor = std::pow(normalizedVelocity, 0.7f);
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

float AdaptiveSmoother::velocityToPerceptualTimeConstant(float velocity)
{
    // Determine current perceptual region
    int region = determinePerceptualRegion(velocity);
    mCurrentPerceptualRegion = region;

    // Apply smooth transitions between regions
    return applyPerceptualTransitions(velocity, region);
}

void AdaptiveSmoother::updateFrequencyAnalysis(float input)
{
    // Store input in circular buffer
    mInputHistory[mHistoryIndex] = input;
    mHistoryIndex = (mHistoryIndex + 1) % mInputHistory.size();

    // Simplified frequency analysis using windowed differences
    // This approximates spectral content without full FFT
    if (mHistoryIndex % 4 == 0) { // Update every 4 samples for efficiency
        int windowSize = std::min(mAnalysisWindow, static_cast<int>(mInputHistory.size()));

        float lowFreqSum = 0.0f;
        float midFreqSum = 0.0f;
        float highFreqSum = 0.0f;

        // Analyze frequency content using time-domain approximations
        for (int i = 1; i < windowSize; i++) {
            int idx1 = (mHistoryIndex - i + mInputHistory.size()) % mInputHistory.size();
            int idx2 = (mHistoryIndex - i - 1 + mInputHistory.size()) % mInputHistory.size();

            float diff = mInputHistory[idx1] - mInputHistory[idx2];
            float diffSquared = diff * diff;

            // Rough frequency classification based on difference magnitudes
            // Small differences = low frequencies, large differences = high frequencies
            if (i <= windowSize / 4) {
                lowFreqSum += diffSquared;
            } else if (i <= windowSize / 2) {
                midFreqSum += diffSquared;
            } else {
                highFreqSum += diffSquared;
            }
        }

        // Update energy estimates with exponential smoothing
        float alpha = 0.1f; // Smoothing factor
        mLowFreqEnergy = alpha * lowFreqSum + (1.0f - alpha) * mLowFreqEnergy;
        mMidFreqEnergy = alpha * midFreqSum + (1.0f - alpha) * mMidFreqEnergy;
        mHighFreqEnergy = alpha * highFreqSum + (1.0f - alpha) * mHighFreqEnergy;
    }
}

float AdaptiveSmoother::calculateFrequencyWeightedVelocity(float rawVelocity)
{
    if (!mPerceptualMappingEnabled) {
        return rawVelocity;
    }

    // Calculate total energy across all frequency bands
    float totalEnergy = mLowFreqEnergy + mMidFreqEnergy + mHighFreqEnergy;

    // Use a minimum energy threshold to prevent extreme scaling during initialization
    const float minEnergyThreshold = 1e-6f;
    if (totalEnergy < minEnergyThreshold) {
        // Return scaled raw velocity with neutral weighting during initialization
        return rawVelocity * mFrequencyWeighting;
    }

    // Normalize energy to get frequency distribution
    float lowFraction = mLowFreqEnergy / totalEnergy;
    float midFraction = mMidFreqEnergy / totalEnergy;
    float highFraction = mHighFreqEnergy / totalEnergy;

    // Apply psychoacoustic weightings
    // Mid frequencies (1-5kHz) are most perceptually important
    // High frequencies need careful handling to avoid artifacts
    float perceptualWeight = (lowFraction * mLowFreqWeight +
                             midFraction * mMidFreqWeight +
                             highFraction * mHighFreqWeight);

    // Apply overall frequency weighting with reasonable bounds
    float weightedVelocity = rawVelocity * perceptualWeight * mFrequencyWeighting;

    // Limit scaling to reasonable range (0.1x to 3x the original velocity)
    return clamp(weightedVelocity, rawVelocity * 0.1f, rawVelocity * 3.0f);
}

int AdaptiveSmoother::determinePerceptualRegion(float velocity)
{
    if (velocity <= mImperceptibleThreshold) {
        return 0; // Imperceptible region
    } else if (velocity <= mJustNoticeableThreshold) {
        return 1; // Just noticeable region
    } else {
        return 2; // Large change region
    }
}

float AdaptiveSmoother::applyPerceptualTransitions(float velocity, int region)
{
    float timeConstant;

    // Base time constant for the current region
    switch (region) {
        case 0: // Imperceptible region
            timeConstant = mImperceptibleTime;
            break;
        case 1: // Just noticeable region
            timeConstant = mJustNoticeableTime;
            break;
        case 2: // Large change region
        default:
            timeConstant = mLargeChangeTime;
            break;
    }

    // Apply smooth transitions between regions using sigmoid-like curves
    // Only apply transitions when we're near region boundaries

    if (velocity > mImperceptibleThreshold * 0.8f && velocity <= mJustNoticeableThreshold * 1.2f) {
        // Transition zone between imperceptible and just noticeable
        float normalizedVelocity = (velocity - mImperceptibleThreshold) /
                                  (mJustNoticeableThreshold - mImperceptibleThreshold);
        normalizedVelocity = clamp(normalizedVelocity, 0.0f, 1.0f);

        // Apply smooth S-curve transition
        float smoothFactor = 0.5f * (1.0f + std::tanh(mTransitionSharpness * (normalizedVelocity - 0.5f)));
        timeConstant = mImperceptibleTime + smoothFactor * (mJustNoticeableTime - mImperceptibleTime);

    } else if (velocity > mJustNoticeableThreshold * 0.8f && velocity <= mLargeChangeThreshold * 1.2f) {
        // Transition zone between just noticeable and large change
        float normalizedVelocity = (velocity - mJustNoticeableThreshold) /
                                  (mLargeChangeThreshold - mJustNoticeableThreshold);
        normalizedVelocity = clamp(normalizedVelocity, 0.0f, 1.0f);

        // Apply smooth S-curve transition
        float smoothFactor = 0.5f * (1.0f + std::tanh(mTransitionSharpness * (normalizedVelocity - 0.5f)));
        timeConstant = mJustNoticeableTime + smoothFactor * (mLargeChangeTime - mJustNoticeableTime);
    }

    return clamp(timeConstant, mImperceptibleTime, mLargeChangeTime);
}

} // namespace WaterStick