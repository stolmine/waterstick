#include "EnhancedAdaptiveSmoother.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace WaterStick {

// EnhancedAdaptiveSmoother Implementation

EnhancedAdaptiveSmoother::EnhancedAdaptiveSmoother(double sampleRate,
                                                   ParameterType parameterType,
                                                   SmoothingMode smoothingMode,
                                                   PerformanceProfile performanceProfile)
    : mCascadedSmoother(std::make_unique<CascadedSmoother>(sampleRate, 0.008f, 3))
    , mPerceptualDetector(std::make_unique<PerceptualVelocityDetector>(sampleRate, 20.0, 20000.0, 1.5f))
    , mLegacySmoother(sampleRate, 0.0005f, 0.008f)
    , mMultiSmoother(1, sampleRate, 0.008f, 3)
    , mSampleRate(sampleRate)
    , mConfiguredMode(smoothingMode)
    , mCurrentMode(smoothingMode)
    , mPerformanceProfile(performanceProfile)
    , mParameterType(parameterType)
    , mFastTimeConstant(0.0005f)
    , mSlowTimeConstant(0.008f)
    , mVelocitySensitivity(1.5f)
    , mHysteresisThreshold(0.15f)
    , mAutoSwitchThreshold(0.3f)
    , mVelocityThreshold(0.2f)
    , mComplexityThreshold(0.4f)
    , mStabilityFactor(1.2f)
    , mAdaptationRate(0.6f)
    , mDecisionConfidence(1.0f)
    , mCpuUsageEstimate(0.0f)
    , mProcessingLatency(0.0f)
    , mQualityMetric(1.0f)
    , mEngineUsage({0.0f, 0.0f, 0.0f, 0.0f})
    , mLegacyCompatibilityMode(false)
    , mAutomaticFallbackEnabled(true)
    , mCpuThreshold(0.8f)
    , mCurrentFallbackLevel(0)
    , mFallbackCount(0)
    , mIsRecovering(false)
    , mCurrentVelocity(0.0f)
    , mPreviousVelocity(0.0f)
    , mPerceptualVelocity(0.0f)
    , mSignalComplexity(0.0f)
    , mStabilityMeasure(1.0f)
    , mPreviousInput(0.0f)
    , mPreviousOutput(0.0f)
    , mInitialized(false)
    , mSampleCounter(0)
    , mCascadedEnabled(true)
    , mMaxStages(4)
    , mStageHysteresis(0.2f)
    , mAdaptiveStages(true)
    , mPerceptualEnabled(true)
    , mMinFrequency(20.0)
    , mMaxFrequency(20000.0)
    , mPerceptualSensitivity(1.8f)
    , mUseSimplifiedAnalysis(false)
{
    initializeEngines();
    applyPerformanceProfile();
    validateConfiguration();
    mInitialized = true;
}

float EnhancedAdaptiveSmoother::processSample(float input)
{
    if (!mInitialized) {
        return input;
    }

    // Increment sample counter for periodic updates
    mSampleCounter++;

    // Analyze signal characteristics for decision-making
    analyzeSignalCharacteristics(input);

    // Determine optimal smoothing mode if in auto mode
    SmoothingMode targetMode = mConfiguredMode;
    if (mConfiguredMode == SmoothingMode::Auto) {
        targetMode = makeIntelligentDecision();
    }

    // Apply hysteresis to mode switching for stability
    if (targetMode != mCurrentMode) {
        float switchingThreshold = mAutoSwitchThreshold * mDecisionConfidence;
        if (mSampleCounter % 16 == 0) { // Check every 16 samples for stability
            if (mStabilityMeasure > switchingThreshold) {
                mCurrentMode = targetMode;
            }
        }
    }

    // Check for automatic fallback if enabled
    if (mAutomaticFallbackEnabled) {
        checkAndExecuteFallback();
    }

    // Process sample with current mode
    float output = processSampleWithMode(input, mCurrentMode);

    // Update performance metrics periodically
    if (mSampleCounter % 64 == 0) {
        updatePerformanceMetrics();
    }

    // Store for next iteration
    mPreviousInput = input;
    mPreviousOutput = output;

    return output;
}

float EnhancedAdaptiveSmoother::processSampleWithMode(float input, SmoothingMode mode)
{
    switch (mode) {
        case SmoothingMode::Enhanced:
            mEngineUsage[0] += 1.0f;
            return processEnhanced(input);

        case SmoothingMode::Perceptual:
            mEngineUsage[1] += 1.0f;
            return processPerceptual(input);

        case SmoothingMode::Cascaded:
            mEngineUsage[2] += 1.0f;
            return processCascaded(input);

        case SmoothingMode::Traditional:
            mEngineUsage[3] += 1.0f;
            return processTraditional(input);

        case SmoothingMode::Bypass:
            return input;

        case SmoothingMode::Auto:
        default:
            // Auto mode should have been resolved before calling this method
            return processEnhanced(input);
    }
}

void EnhancedAdaptiveSmoother::reset()
{
    reset(0.0f);
}

void EnhancedAdaptiveSmoother::reset(float value)
{
    if (mCascadedSmoother) {
        mCascadedSmoother->reset(value);
    }

    if (mPerceptualDetector) {
        mPerceptualDetector->reset();
    }

    mLegacySmoother.reset();
    mMultiSmoother.resetAll(&value);

    // Reset internal state
    mCurrentVelocity = 0.0f;
    mPreviousVelocity = 0.0f;
    mPerceptualVelocity = 0.0f;
    mSignalComplexity = 0.0f;
    mStabilityMeasure = 1.0f;
    mPreviousInput = value;
    mPreviousOutput = value;
    mDecisionConfidence = 1.0f;
    mSampleCounter = 0;

    // Reset performance metrics
    mCpuUsageEstimate = 0.0f;
    mProcessingLatency = 0.0f;
    mQualityMetric = 1.0f;
    mEngineUsage.fill(0.0f);

    // Reset fallback state
    mCurrentFallbackLevel = 0;
    mIsRecovering = false;
}

void EnhancedAdaptiveSmoother::setSampleRate(double sampleRate)
{
    mSampleRate = sampleRate;

    if (mCascadedSmoother) {
        mCascadedSmoother->setSampleRate(sampleRate);
    }

    if (mPerceptualDetector) {
        mPerceptualDetector->setSampleRate(sampleRate);
    }

    mLegacySmoother.setSampleRate(sampleRate);
    mMultiSmoother.setSampleRate(sampleRate);

    updateEngineConfigurations();
}

void EnhancedAdaptiveSmoother::setSmoothingMode(SmoothingMode mode, float autoSwitchThreshold)
{
    mConfiguredMode = mode;
    mAutoSwitchThreshold = clamp(autoSwitchThreshold, 0.1f, 1.0f);

    // If not in auto mode, immediately switch to the configured mode
    if (mode != SmoothingMode::Auto) {
        mCurrentMode = mode;
    }
}

void EnhancedAdaptiveSmoother::setPerformanceProfile(PerformanceProfile profile)
{
    mPerformanceProfile = profile;
    applyPerformanceProfile();
}

void EnhancedAdaptiveSmoother::setParameterType(ParameterType parameterType)
{
    mParameterType = parameterType;
    updateEngineConfigurations();
}

void EnhancedAdaptiveSmoother::setAdaptiveParameters(float fastTimeConstant,
                                                    float slowTimeConstant,
                                                    float velocitySensitivity,
                                                    float hysteresisThreshold)
{
    mFastTimeConstant = clamp(fastTimeConstant, 0.0001f, 0.01f);
    mSlowTimeConstant = clamp(slowTimeConstant, 0.001f, 0.05f);
    mVelocitySensitivity = clamp(velocitySensitivity, 0.1f, 10.0f);
    mHysteresisThreshold = clamp(hysteresisThreshold, 0.01f, 0.5f);

    // Ensure fast <= slow
    if (mFastTimeConstant > mSlowTimeConstant) {
        mFastTimeConstant = mSlowTimeConstant * 0.5f;
    }

    updateEngineConfigurations();
}

void EnhancedAdaptiveSmoother::setCascadedParameters(bool enabled,
                                                    int maxStages,
                                                    float stageHysteresis,
                                                    bool adaptiveStages)
{
    mCascadedEnabled = enabled;
    mMaxStages = clampInt(maxStages, 1, CascadedSmoother::MAX_STAGES);
    mStageHysteresis = clamp(stageHysteresis, 0.05f, 0.5f);
    mAdaptiveStages = adaptiveStages;

    if (mCascadedSmoother) {
        mCascadedSmoother->setStageCount(enabled ? mMaxStages : 1);
    }

    updateEngineConfigurations();
}

void EnhancedAdaptiveSmoother::setPerceptualParameters(bool enabled,
                                                      double minFrequency,
                                                      double maxFrequency,
                                                      float perceptualSensitivity,
                                                      bool useSimplifiedAnalysis)
{
    mPerceptualEnabled = enabled;
    mMinFrequency = std::max(10.0, std::min(minFrequency, 100.0));
    mMaxFrequency = std::max(10000.0, std::min(maxFrequency, mSampleRate * 0.45));
    mPerceptualSensitivity = clamp(perceptualSensitivity, 0.1f, 5.0f);
    mUseSimplifiedAnalysis = useSimplifiedAnalysis;

    if (mPerceptualDetector) {
        mPerceptualDetector->setAnalysisParameters(mMinFrequency, mMaxFrequency, mPerceptualSensitivity);
    }
}

void EnhancedAdaptiveSmoother::setDecisionParameters(float velocityThreshold,
                                                    float complexityThreshold,
                                                    float stabilityFactor,
                                                    float adaptationRate)
{
    mVelocityThreshold = clamp(velocityThreshold, 0.01f, 2.0f);
    mComplexityThreshold = clamp(complexityThreshold, 0.1f, 1.0f);
    mStabilityFactor = clamp(stabilityFactor, 0.5f, 3.0f);
    mAdaptationRate = clamp(adaptationRate, 0.1f, 1.0f);
}

void EnhancedAdaptiveSmoother::setLegacyCompatibilityMode(bool enabled, bool preserveSettings)
{
    mLegacyCompatibilityMode = enabled;

    if (enabled) {
        // Force traditional mode when in legacy compatibility
        if (mConfiguredMode == SmoothingMode::Auto) {
            mCurrentMode = SmoothingMode::Traditional;
        }

        if (preserveSettings) {
            // Copy current settings to legacy smoother
            mLegacySmoother.setAdaptiveParameters(mFastTimeConstant, mSlowTimeConstant,
                                                 mVelocitySensitivity, mHysteresisThreshold);
        }
    }
}

void EnhancedAdaptiveSmoother::getPerformanceMetrics(float& cpuUsage, float& latency, float& quality) const
{
    cpuUsage = mCpuUsageEstimate;
    latency = mProcessingLatency;
    quality = mQualityMetric;
}

void EnhancedAdaptiveSmoother::getDetailedStatus(float& velocity, float& timeConstant, int& stageCount,
                                                float& perceptualVelocity, float& decisionConfidence) const
{
    velocity = mCurrentVelocity;
    timeConstant = mSlowTimeConstant; // Simplified for now
    stageCount = mCascadedSmoother ? mCascadedSmoother->getStageCount() : 1;
    perceptualVelocity = mPerceptualVelocity;
    decisionConfidence = mDecisionConfidence;
}

void EnhancedAdaptiveSmoother::getEngineUtilization(float& enhancedUsage, float& cascadedUsage,
                                                   float& perceptualUsage, float& traditionalUsage) const
{
    float totalSamples = mEngineUsage[0] + mEngineUsage[1] + mEngineUsage[2] + mEngineUsage[3];
    if (totalSamples > 0.0f) {
        enhancedUsage = mEngineUsage[0] / totalSamples;
        cascadedUsage = mEngineUsage[2] / totalSamples;
        perceptualUsage = mEngineUsage[1] / totalSamples;
        traditionalUsage = mEngineUsage[3] / totalSamples;
    } else {
        enhancedUsage = cascadedUsage = perceptualUsage = traditionalUsage = 0.0f;
    }
}

bool EnhancedAdaptiveSmoother::isOptimalState() const
{
    return mCpuUsageEstimate < mCpuThreshold &&
           mDecisionConfidence > 0.7f &&
           mCurrentFallbackLevel == 0 &&
           !mIsRecovering;
}

void EnhancedAdaptiveSmoother::getRecommendedSettings(SmoothingMode& mode, PerformanceProfile& profile,
                                                     float adaptiveParams[4]) const
{
    // Provide parameter-type specific recommendations
    switch (mParameterType) {
        case ParameterType::DelayTime:
            mode = SmoothingMode::Enhanced;
            profile = PerformanceProfile::HighQuality;
            adaptiveParams[0] = 0.0003f; // Fast time constant
            adaptiveParams[1] = 0.005f;  // Slow time constant
            adaptiveParams[2] = 2.0f;    // Velocity sensitivity
            adaptiveParams[3] = 0.1f;    // Hysteresis threshold
            break;

        case ParameterType::CombSize:
            mode = SmoothingMode::Enhanced;
            profile = PerformanceProfile::HighQuality;
            adaptiveParams[0] = 0.0002f; // Very fast for comb size
            adaptiveParams[1] = 0.003f;
            adaptiveParams[2] = 2.5f;    // High sensitivity
            adaptiveParams[3] = 0.08f;
            break;

        case ParameterType::PitchCV:
            mode = SmoothingMode::Perceptual;
            profile = PerformanceProfile::Balanced;
            adaptiveParams[0] = 0.0005f;
            adaptiveParams[1] = 0.008f;
            adaptiveParams[2] = 1.8f;
            adaptiveParams[3] = 0.12f;
            break;

        case ParameterType::FilterCutoff:
            mode = SmoothingMode::Cascaded;
            profile = PerformanceProfile::Balanced;
            adaptiveParams[0] = 0.0008f;
            adaptiveParams[1] = 0.015f;
            adaptiveParams[2] = 1.2f;
            adaptiveParams[3] = 0.2f;
            break;

        case ParameterType::Amplitude:
            mode = SmoothingMode::Traditional;
            profile = PerformanceProfile::LowLatency;
            adaptiveParams[0] = 0.001f;
            adaptiveParams[1] = 0.01f;
            adaptiveParams[2] = 1.0f;
            adaptiveParams[3] = 0.15f;
            break;

        case ParameterType::Generic:
        default:
            mode = SmoothingMode::Auto;
            profile = PerformanceProfile::Balanced;
            adaptiveParams[0] = 0.0005f;
            adaptiveParams[1] = 0.008f;
            adaptiveParams[2] = 1.5f;
            adaptiveParams[3] = 0.15f;
            break;
    }
}

void EnhancedAdaptiveSmoother::forceFallback(int level)
{
    mCurrentFallbackLevel = clampInt(level, 0, 3);
    mFallbackCount++;
    mIsRecovering = false;

    // Immediately switch to fallback mode
    switch (mCurrentFallbackLevel) {
        case 0: mCurrentMode = SmoothingMode::Enhanced; break;
        case 1: mCurrentMode = SmoothingMode::Perceptual; break;
        case 2: mCurrentMode = SmoothingMode::Cascaded; break;
        case 3: mCurrentMode = SmoothingMode::Traditional; break;
    }
}

void EnhancedAdaptiveSmoother::setAutomaticFallback(bool enabled, float cpuThreshold)
{
    mAutomaticFallbackEnabled = enabled;
    mCpuThreshold = clamp(cpuThreshold, 0.5f, 1.0f);
}

void EnhancedAdaptiveSmoother::getFallbackStatus(int& currentLevel, int& fallbackCount, bool& isRecovering) const
{
    currentLevel = mCurrentFallbackLevel;
    fallbackCount = mFallbackCount;
    isRecovering = mIsRecovering;
}

// Private Methods Implementation

void EnhancedAdaptiveSmoother::analyzeSignalCharacteristics(float input)
{
    // Calculate basic velocity
    mCurrentVelocity = calculateHybridVelocity(input);

    // Calculate signal complexity
    mSignalComplexity = calculateSignalComplexity(input);

    // Update stability measure with exponential smoothing
    float velocityChange = std::abs(mCurrentVelocity - mPreviousVelocity);
    float stabilityUpdate = 1.0f / (1.0f + velocityChange * 10.0f);
    mStabilityMeasure = 0.9f * mStabilityMeasure + 0.1f * stabilityUpdate;

    // Calculate perceptual velocity if enabled
    if (mPerceptualEnabled && mPerceptualDetector) {
        if (mUseSimplifiedAnalysis) {
            mPerceptualVelocity = mPerceptualDetector->analyzeDelayTimeVelocitySimplified(input, mPreviousInput);
        } else {
            mPerceptualVelocity = mPerceptualDetector->analyzeDelayTimeVelocity(input, mPreviousInput);
        }
    }

    mPreviousVelocity = mCurrentVelocity;
}

EnhancedAdaptiveSmoother::SmoothingMode EnhancedAdaptiveSmoother::makeIntelligentDecision()
{
    // Calculate decision confidence
    mDecisionConfidence = calculateDecisionConfidence();

    // Apply parameter-specific decision logic
    float effectiveVelocity = applyParameterOptimizations(mCurrentVelocity);

    // Decision matrix based on velocity and complexity
    if (effectiveVelocity > mVelocityThreshold * 2.0f && mSignalComplexity > mComplexityThreshold) {
        // High velocity and complexity -> Enhanced mode
        return SmoothingMode::Enhanced;
    } else if (mPerceptualEnabled && effectiveVelocity > mVelocityThreshold) {
        // Medium velocity -> Perceptual mode
        return SmoothingMode::Perceptual;
    } else if (mCascadedEnabled && mSignalComplexity > mComplexityThreshold) {
        // High complexity, low velocity -> Cascaded mode
        return SmoothingMode::Cascaded;
    } else {
        // Low velocity and complexity -> Traditional mode
        return SmoothingMode::Traditional;
    }
}

float EnhancedAdaptiveSmoother::processEnhanced(float input)
{
    // Combine cascaded filtering with perceptual velocity detection
    float cascadedOutput = input;
    float perceptualOutput = input;

    if (mCascadedEnabled && mCascadedSmoother) {
        // Update cascaded smoother with adaptive time constant
        float timeConstant = (mCurrentVelocity > mVelocityThreshold) ? mFastTimeConstant : mSlowTimeConstant;
        mCascadedSmoother->setTimeConstant(timeConstant);

        // Adaptive stage count based on velocity
        if (mAdaptiveStages) {
            int targetStages = 1 + static_cast<int>(mCurrentVelocity * 3.0f);
            targetStages = clampInt(targetStages, 1, mMaxStages);
            mCascadedSmoother->setStageCount(targetStages);
        }

        cascadedOutput = mCascadedSmoother->processSample(input);
    }

    // Blend cascaded and perceptual outputs based on signal characteristics
    float blendFactor = mSignalComplexity;
    return cascadedOutput * (1.0f - blendFactor) + perceptualOutput * blendFactor;
}

float EnhancedAdaptiveSmoother::processPerceptual(float input)
{
    // Use perceptual velocity to drive adaptive smoothing
    if (mPerceptualEnabled && mPerceptualDetector) {
        float perceptualTimeConstant = mSlowTimeConstant;

        if (std::abs(mPerceptualVelocity) > mVelocityThreshold) {
            perceptualTimeConstant = mFastTimeConstant +
                (mSlowTimeConstant - mFastTimeConstant) * std::exp(-std::abs(mPerceptualVelocity) * mPerceptualSensitivity);
        }

        // Use legacy smoother with perceptual time constant
        mLegacySmoother.setAdaptiveParameters(mFastTimeConstant, perceptualTimeConstant,
                                             mPerceptualSensitivity, mHysteresisThreshold);
        return mLegacySmoother.processSample(input);
    }

    return input;
}

float EnhancedAdaptiveSmoother::processCascaded(float input)
{
    if (mCascadedEnabled && mCascadedSmoother) {
        // Update time constant based on velocity
        float timeConstant = (mCurrentVelocity > mVelocityThreshold) ? mFastTimeConstant : mSlowTimeConstant;
        mCascadedSmoother->setTimeConstant(timeConstant);
        return mCascadedSmoother->processSample(input);
    }

    return input;
}

float EnhancedAdaptiveSmoother::processTraditional(float input)
{
    return mLegacySmoother.processSample(input);
}

void EnhancedAdaptiveSmoother::updatePerformanceMetrics()
{
    // Estimate CPU usage based on current mode and configuration
    switch (mCurrentMode) {
        case SmoothingMode::Enhanced:
            mCpuUsageEstimate = 0.8f;
            mProcessingLatency = 4.0f;
            mQualityMetric = 0.95f;
            break;
        case SmoothingMode::Perceptual:
            mCpuUsageEstimate = 0.6f;
            mProcessingLatency = 3.0f;
            mQualityMetric = 0.85f;
            break;
        case SmoothingMode::Cascaded:
            mCpuUsageEstimate = 0.4f;
            mProcessingLatency = 2.0f;
            mQualityMetric = 0.80f;
            break;
        case SmoothingMode::Traditional:
            mCpuUsageEstimate = 0.2f;
            mProcessingLatency = 1.0f;
            mQualityMetric = 0.70f;
            break;
        case SmoothingMode::Bypass:
            mCpuUsageEstimate = 0.01f;
            mProcessingLatency = 0.0f;
            mQualityMetric = 0.0f;
            break;
        default:
            break;
    }

    // Normalize engine usage statistics
    if (mSampleCounter > 0) {
        float totalUsage = mEngineUsage[0] + mEngineUsage[1] + mEngineUsage[2] + mEngineUsage[3];
        if (totalUsage > 0.0f) {
            for (int i = 0; i < 4; ++i) {
                mEngineUsage[i] /= totalUsage;
            }
        }
    }
}

void EnhancedAdaptiveSmoother::updateEngineConfigurations()
{
    // Update legacy smoother configuration
    mLegacySmoother.setAdaptiveParameters(mFastTimeConstant, mSlowTimeConstant,
                                         mVelocitySensitivity, mHysteresisThreshold);

    // Update cascaded smoother if available
    if (mCascadedSmoother) {
        mCascadedSmoother->setTimeConstant(mSlowTimeConstant);
        mCascadedSmoother->setStageCount(mCascadedEnabled ? mMaxStages : 1);
    }

    // Update perceptual detector if available
    if (mPerceptualDetector) {
        mPerceptualDetector->setAnalysisParameters(mMinFrequency, mMaxFrequency, mPerceptualSensitivity);
    }
}

bool EnhancedAdaptiveSmoother::checkAndExecuteFallback()
{
    // Check if CPU usage exceeds threshold
    if (mCpuUsageEstimate > mCpuThreshold) {
        // Trigger fallback to less computationally intensive mode
        if (mCurrentFallbackLevel < 3) {
            forceFallback(mCurrentFallbackLevel + 1);
            return true;
        }
    } else if (mIsRecovering || mCpuUsageEstimate < mCpuThreshold * 0.7f) {
        // Try to recover to higher quality mode
        if (mCurrentFallbackLevel > 0) {
            mCurrentFallbackLevel--;
            mIsRecovering = true;
        }
    }

    return false;
}

float EnhancedAdaptiveSmoother::calculateHybridVelocity(float input)
{
    // Basic finite difference velocity
    float basicVelocity = std::abs(input - mPreviousInput);

    // Apply parameter-specific optimizations
    return applyParameterOptimizations(basicVelocity);
}

float EnhancedAdaptiveSmoother::applyParameterOptimizations(float velocity)
{
    // Parameter-specific velocity scaling
    switch (mParameterType) {
        case ParameterType::DelayTime:
            return velocity * 1.5f; // More sensitive to delay changes
        case ParameterType::CombSize:
            return velocity * 2.0f; // Very sensitive to comb size changes
        case ParameterType::PitchCV:
            return velocity * 1.8f; // Sensitive to pitch changes
        case ParameterType::FilterCutoff:
            return velocity * 1.2f; // Moderately sensitive to filter changes
        case ParameterType::Amplitude:
            return velocity * 0.8f; // Less sensitive to amplitude changes
        case ParameterType::Generic:
        default:
            return velocity; // Standard scaling
    }
}

float EnhancedAdaptiveSmoother::calculateSignalComplexity(float input)
{
    // Simplified complexity measure based on input variation
    static float complexityHistory[8] = {0.0f};
    static int complexityIndex = 0;

    // Store current input difference
    complexityHistory[complexityIndex] = std::abs(input - mPreviousInput);
    complexityIndex = (complexityIndex + 1) % 8;

    // Calculate variance as complexity measure
    float mean = 0.0f;
    for (int i = 0; i < 8; ++i) {
        mean += complexityHistory[i];
    }
    mean /= 8.0f;

    float variance = 0.0f;
    for (int i = 0; i < 8; ++i) {
        float diff = complexityHistory[i] - mean;
        variance += diff * diff;
    }
    variance /= 8.0f;

    return clamp(variance * 100.0f, 0.0f, 1.0f);
}

float EnhancedAdaptiveSmoother::calculateDecisionConfidence()
{
    // Base confidence on stability and consistency of measurements
    float velocityConfidence = 1.0f / (1.0f + std::abs(mCurrentVelocity - mPreviousVelocity) * 10.0f);
    float stabilityConfidence = mStabilityMeasure;

    return (velocityConfidence + stabilityConfidence) * 0.5f;
}

void EnhancedAdaptiveSmoother::applyPerformanceProfile()
{
    switch (mPerformanceProfile) {
        case PerformanceProfile::HighQuality:
            mUseSimplifiedAnalysis = false;
            mMaxStages = 5;
            mAutoSwitchThreshold = 0.2f;
            break;

        case PerformanceProfile::Balanced:
            mUseSimplifiedAnalysis = false;
            mMaxStages = 3;
            mAutoSwitchThreshold = 0.3f;
            break;

        case PerformanceProfile::LowLatency:
            mUseSimplifiedAnalysis = true;
            mMaxStages = 2;
            mAutoSwitchThreshold = 0.4f;
            break;

        case PerformanceProfile::PowerSaver:
            mUseSimplifiedAnalysis = true;
            mMaxStages = 1;
            mAutoSwitchThreshold = 0.5f;
            break;
    }

    updateEngineConfigurations();
}

void EnhancedAdaptiveSmoother::initializeEngines()
{
    // Initialize cascaded smoother
    if (!mCascadedSmoother) {
        mCascadedSmoother = std::make_unique<CascadedSmoother>(mSampleRate, mSlowTimeConstant, mMaxStages);
    }

    // Initialize perceptual detector
    if (!mPerceptualDetector) {
        mPerceptualDetector = std::make_unique<PerceptualVelocityDetector>(mSampleRate, mMinFrequency, mMaxFrequency, mPerceptualSensitivity);
    }

    // Initialize multi-parameter smoother
    mMultiSmoother.initialize(1, mSampleRate);
}

void EnhancedAdaptiveSmoother::validateConfiguration()
{
    // Ensure time constants are properly ordered
    if (mFastTimeConstant > mSlowTimeConstant) {
        mFastTimeConstant = mSlowTimeConstant * 0.5f;
    }

    // Ensure frequency bounds are valid
    if (mMinFrequency >= mMaxFrequency) {
        mMinFrequency = 20.0;
        mMaxFrequency = 20000.0;
    }

    // Ensure threshold values are reasonable
    mVelocityThreshold = clamp(mVelocityThreshold, 0.01f, 2.0f);
    mComplexityThreshold = clamp(mComplexityThreshold, 0.1f, 1.0f);
    mAutoSwitchThreshold = clamp(mAutoSwitchThreshold, 0.1f, 1.0f);
}

// Static utility methods

const char* EnhancedAdaptiveSmoother::smoothingModeToString(SmoothingMode mode)
{
    switch (mode) {
        case SmoothingMode::Auto: return "Auto";
        case SmoothingMode::Enhanced: return "Enhanced";
        case SmoothingMode::Perceptual: return "Perceptual";
        case SmoothingMode::Cascaded: return "Cascaded";
        case SmoothingMode::Traditional: return "Traditional";
        case SmoothingMode::Bypass: return "Bypass";
        default: return "Unknown";
    }
}

const char* EnhancedAdaptiveSmoother::performanceProfileToString(PerformanceProfile profile)
{
    switch (profile) {
        case PerformanceProfile::HighQuality: return "HighQuality";
        case PerformanceProfile::Balanced: return "Balanced";
        case PerformanceProfile::LowLatency: return "LowLatency";
        case PerformanceProfile::PowerSaver: return "PowerSaver";
        default: return "Unknown";
    }
}

const char* EnhancedAdaptiveSmoother::parameterTypeToString(ParameterType type)
{
    switch (type) {
        case ParameterType::DelayTime: return "DelayTime";
        case ParameterType::CombSize: return "CombSize";
        case ParameterType::PitchCV: return "PitchCV";
        case ParameterType::FilterCutoff: return "FilterCutoff";
        case ParameterType::Amplitude: return "Amplitude";
        case ParameterType::Generic: return "Generic";
        default: return "Unknown";
    }
}

// EnhancedMultiParameterSmoother Implementation

EnhancedMultiParameterSmoother::EnhancedMultiParameterSmoother(int parameterCount,
                                                               double sampleRate,
                                                               EnhancedAdaptiveSmoother::ParameterType defaultParameterType)
    : mParameterCount(0)
    , mInitialized(false)
    , mGlobalMode(EnhancedAdaptiveSmoother::SmoothingMode::Auto)
    , mGlobalDecisionConfidence(1.0f)
    , mCoordinatedMode(true)
{
    initialize(parameterCount, sampleRate);

    // Initialize all smoothers with default parameter type
    for (int i = 0; i < mParameterCount; ++i) {
        if (mSmoothers[i]) {
            mSmoothers[i]->setParameterType(defaultParameterType);
        }
    }
}

void EnhancedMultiParameterSmoother::initialize(int parameterCount, double sampleRate)
{
    mParameterCount = std::max(1, std::min(parameterCount, MAX_PARAMETERS));

    for (int i = 0; i < mParameterCount; ++i) {
        mSmoothers[i] = std::make_unique<EnhancedAdaptiveSmoother>(
            sampleRate,
            EnhancedAdaptiveSmoother::ParameterType::Generic,
            EnhancedAdaptiveSmoother::SmoothingMode::Auto,
            EnhancedAdaptiveSmoother::PerformanceProfile::Balanced
        );
    }

    mInitialized = true;
}

void EnhancedMultiParameterSmoother::processAllSamples(const float* inputs, float* outputs)
{
    if (!mInitialized || !inputs || !outputs) {
        return;
    }

    for (int i = 0; i < mParameterCount; ++i) {
        if (mSmoothers[i]) {
            outputs[i] = mSmoothers[i]->processSample(inputs[i]);
        } else {
            outputs[i] = inputs[i];
        }
    }

    // Update coordinated decision-making
    if (mCoordinatedMode) {
        updateCoordinatedDecisions();
    }
}

float EnhancedMultiParameterSmoother::processSample(int parameterIndex, float input)
{
    if (!isValidParameterIndex(parameterIndex) || !mSmoothers[parameterIndex]) {
        return input;
    }

    return mSmoothers[parameterIndex]->processSample(input);
}

void EnhancedMultiParameterSmoother::setParameterType(int parameterIndex,
                                                     EnhancedAdaptiveSmoother::ParameterType parameterType)
{
    if (isValidParameterIndex(parameterIndex) && mSmoothers[parameterIndex]) {
        mSmoothers[parameterIndex]->setParameterType(parameterType);
    }
}

void EnhancedMultiParameterSmoother::setGlobalSmoothingMode(EnhancedAdaptiveSmoother::SmoothingMode mode)
{
    mGlobalMode = mode;

    for (int i = 0; i < mParameterCount; ++i) {
        if (mSmoothers[i]) {
            mSmoothers[i]->setSmoothingMode(mode);
        }
    }
}

void EnhancedMultiParameterSmoother::setGlobalPerformanceProfile(EnhancedAdaptiveSmoother::PerformanceProfile profile)
{
    for (int i = 0; i < mParameterCount; ++i) {
        if (mSmoothers[i]) {
            mSmoothers[i]->setPerformanceProfile(profile);
        }
    }
}

void EnhancedMultiParameterSmoother::resetAll()
{
    for (int i = 0; i < mParameterCount; ++i) {
        if (mSmoothers[i]) {
            mSmoothers[i]->reset();
        }
    }
}

void EnhancedMultiParameterSmoother::resetAll(const float* values)
{
    if (!values) {
        return;
    }

    for (int i = 0; i < mParameterCount; ++i) {
        if (mSmoothers[i]) {
            mSmoothers[i]->reset(values[i]);
        }
    }
}

EnhancedAdaptiveSmoother& EnhancedMultiParameterSmoother::getSmoother(int parameterIndex)
{
    if (isValidParameterIndex(parameterIndex) && mSmoothers[parameterIndex]) {
        return *mSmoothers[parameterIndex];
    }

    // Return first smoother as fallback
    return *mSmoothers[0];
}

const EnhancedAdaptiveSmoother& EnhancedMultiParameterSmoother::getSmoother(int parameterIndex) const
{
    if (isValidParameterIndex(parameterIndex) && mSmoothers[parameterIndex]) {
        return *mSmoothers[parameterIndex];
    }

    // Return first smoother as fallback
    return *mSmoothers[0];
}

void EnhancedMultiParameterSmoother::getSystemMetrics(float& avgCpuUsage, float& maxLatency, float& avgQuality) const
{
    float totalCpu = 0.0f;
    float totalQuality = 0.0f;
    maxLatency = 0.0f;

    for (int i = 0; i < mParameterCount; ++i) {
        if (mSmoothers[i]) {
            float cpu, latency, quality;
            mSmoothers[i]->getPerformanceMetrics(cpu, latency, quality);

            totalCpu += cpu;
            totalQuality += quality;
            maxLatency = std::max(maxLatency, latency);
        }
    }

    avgCpuUsage = mParameterCount > 0 ? totalCpu / mParameterCount : 0.0f;
    avgQuality = mParameterCount > 0 ? totalQuality / mParameterCount : 0.0f;
}

void EnhancedMultiParameterSmoother::updateCoordinatedDecisions()
{
    // Calculate global decision confidence
    float totalConfidence = 0.0f;
    int validSmoothers = 0;

    for (int i = 0; i < mParameterCount; ++i) {
        if (mSmoothers[i]) {
            float velocity, timeConstant, perceptualVelocity, confidence;
            int stageCount;
            mSmoothers[i]->getDetailedStatus(velocity, timeConstant, stageCount, perceptualVelocity, confidence);

            totalConfidence += confidence;
            validSmoothers++;
        }
    }

    mGlobalDecisionConfidence = validSmoothers > 0 ? totalConfidence / validSmoothers : 1.0f;
}


} // namespace WaterStick