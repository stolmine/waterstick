#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include <memory>
#include "CascadedSmoother.h"
#include "PerceptualVelocityDetector.h"

namespace WaterStick {

/**
 * @class SimpleAdaptiveSmoother
 * @brief Lightweight adaptive smoother for internal use in EnhancedAdaptiveSmoother
 *
 * This provides basic adaptive smoothing functionality without external dependencies
 * to ensure EnhancedAdaptiveSmoother can operate independently.
 */
class SimpleAdaptiveSmoother {
public:
    SimpleAdaptiveSmoother(double sampleRate = 44100.0, float fastTC = 0.0005f, float slowTC = 0.008f)
        : mSampleRate(sampleRate)
        , mFastTimeConstant(fastTC)
        , mSlowTimeConstant(slowTC)
        , mCurrentTimeConstant(slowTC)
        , mSmoothingCoeff(0.0f)
        , mPreviousInput(0.0f)
        , mPreviousOutput(0.0f)
        , mCurrentVelocity(0.0f)
        , mVelocitySensitivity(1.5f)
    {
        updateCoefficient();
    }

    void setSampleRate(double sampleRate) {
        mSampleRate = sampleRate;
        updateCoefficient();
    }

    void setAdaptiveParameters(float fastTC, float slowTC, float sensitivity, float /*hysteresis*/) {
        mFastTimeConstant = std::max(0.0001f, std::min(fastTC, 0.01f));
        mSlowTimeConstant = std::max(0.001f, std::min(slowTC, 0.05f));
        mVelocitySensitivity = std::max(0.1f, std::min(sensitivity, 10.0f));
        updateCoefficient();
    }

    float processSample(float input) {
        // Calculate velocity
        mCurrentVelocity = std::abs(input - mPreviousInput);

        // Determine time constant based on velocity
        float velocityMagnitude = mCurrentVelocity * mVelocitySensitivity;
        float exponentialFactor = std::exp(-velocityMagnitude);
        mCurrentTimeConstant = mFastTimeConstant + (mSlowTimeConstant - mFastTimeConstant) * exponentialFactor;

        updateCoefficient();

        // Apply exponential smoothing
        float output = mSmoothingCoeff * input + (1.0f - mSmoothingCoeff) * mPreviousOutput;

        mPreviousInput = input;
        mPreviousOutput = output;

        return output;
    }

    void reset() {
        mPreviousInput = 0.0f;
        mPreviousOutput = 0.0f;
        mCurrentVelocity = 0.0f;
    }

    float getCurrentTimeConstant() const { return mCurrentTimeConstant; }
    float getCurrentVelocity() const { return mCurrentVelocity; }

private:
    double mSampleRate;
    float mFastTimeConstant;
    float mSlowTimeConstant;
    float mCurrentTimeConstant;
    float mSmoothingCoeff;
    float mPreviousInput;
    float mPreviousOutput;
    float mCurrentVelocity;
    float mVelocitySensitivity;

    void updateCoefficient() {
        float sampleTime = 1.0f / static_cast<float>(mSampleRate);
        mSmoothingCoeff = 1.0f - std::exp(-sampleTime / mCurrentTimeConstant);
        mSmoothingCoeff = std::max(0.0f, std::min(1.0f, mSmoothingCoeff));
    }
};

/**
 * @class EnhancedAdaptiveSmoother
 * @brief Unified hybrid smoothing system combining cascaded filtering with perceptual velocity detection
 *
 * This class represents the next evolution of parameter smoothing by intelligently combining:
 * - CascadedSmoother for superior frequency response
 * - PerceptualVelocityDetector for psychoacoustic analysis
 * - AdaptiveSmoother for velocity-based adaptation
 * - Intelligent decision-making for optimal technique selection
 * - Graceful fallback mechanisms for robust operation
 *
 * Key Features:
 * - Hybrid architecture with automatic technique selection
 * - Backward compatibility with existing AdaptiveSmoother interface
 * - Intelligent switching between smoothing approaches based on signal characteristics
 * - Performance optimization with adaptive computational complexity
 * - Real-time safe operation with bounded processing time
 * - Seamless integration with WaterStick parameter systems
 *
 * Architecture Overview:
 * - Primary Engine: Enhanced cascade system with perceptual analysis
 * - Fallback Engine: Traditional adaptive smoothing for compatibility
 * - Decision System: Intelligent selection based on parameter characteristics
 * - Compatibility Layer: Maintains existing API contracts
 */
class EnhancedAdaptiveSmoother {
public:
    /**
     * @enum SmoothingMode
     * @brief Available smoothing modes for different use cases
     */
    enum class SmoothingMode {
        Auto,           // Automatic mode selection based on signal analysis
        Enhanced,       // Force enhanced mode (cascaded + perceptual)
        Perceptual,     // Perceptual velocity detection only
        Cascaded,       // Cascaded filtering only
        Traditional,    // Traditional adaptive smoothing (compatibility)
        Bypass          // No smoothing (pass-through)
    };

    /**
     * @enum PerformanceProfile
     * @brief Performance optimization profiles
     */
    enum class PerformanceProfile {
        HighQuality,    // Maximum quality, higher CPU usage
        Balanced,       // Balance between quality and performance
        LowLatency,     // Minimize processing delay
        PowerSaver      // Minimal CPU usage
    };

    /**
     * @enum ParameterType
     * @brief Parameter type hints for optimization
     */
    enum class ParameterType {
        DelayTime,      // Delay line parameters
        CombSize,       // Comb filter size parameters
        PitchCV,        // Pitch control voltage
        FilterCutoff,   // Filter frequency parameters
        Amplitude,      // Gain/volume parameters
        Generic         // General purpose parameters
    };

    /**
     * @brief Constructor with comprehensive configuration options
     * @param sampleRate Sample rate in Hz
     * @param parameterType Type hint for parameter-specific optimization
     * @param smoothingMode Initial smoothing mode
     * @param performanceProfile Performance optimization profile
     */
    EnhancedAdaptiveSmoother(double sampleRate = 44100.0,
                            ParameterType parameterType = ParameterType::Generic,
                            SmoothingMode smoothingMode = SmoothingMode::Auto,
                            PerformanceProfile performanceProfile = PerformanceProfile::Balanced);

    ~EnhancedAdaptiveSmoother() = default;

    // === Core Processing Interface ===

    /**
     * @brief Process one sample with intelligent smoothing selection
     * @param input Current parameter value
     * @return Smoothed parameter value
     */
    float processSample(float input);

    /**
     * @brief Process sample with explicit mode override
     * @param input Current parameter value
     * @param mode Smoothing mode to use for this sample
     * @return Smoothed parameter value
     */
    float processSampleWithMode(float input, SmoothingMode mode);

    /**
     * @brief Reset all smoothing engines to clean state
     */
    void reset();

    /**
     * @brief Reset to specific value (prevents initial transients)
     * @param value Initial value for all engines
     */
    void reset(float value);

    // === Configuration Interface ===

    /**
     * @brief Set or update sample rate for all engines
     * @param sampleRate New sample rate in Hz
     */
    void setSampleRate(double sampleRate);

    /**
     * @brief Configure smoothing mode and behavior
     * @param mode Smoothing mode selection
     * @param autoSwitchThreshold Threshold for automatic mode switching (0.0-1.0)
     */
    void setSmoothingMode(SmoothingMode mode, float autoSwitchThreshold = 0.3f);

    /**
     * @brief Set performance optimization profile
     * @param profile Performance profile selection
     */
    void setPerformanceProfile(PerformanceProfile profile);

    /**
     * @brief Configure parameter type for optimized processing
     * @param parameterType Parameter type hint
     */
    void setParameterType(ParameterType parameterType);

    /**
     * @brief Configure enhanced adaptive parameters
     * @param fastTimeConstant Fast response time constant (0.0001f to 0.01f)
     * @param slowTimeConstant Slow response time constant (0.001f to 0.05f)
     * @param velocitySensitivity Velocity detection sensitivity (0.1f to 10.0f)
     * @param hysteresisThreshold Mode switching hysteresis (0.01f to 0.5f)
     */
    void setAdaptiveParameters(float fastTimeConstant = 0.0005f,
                              float slowTimeConstant = 0.008f,
                              float velocitySensitivity = 1.5f,
                              float hysteresisThreshold = 0.15f);

    /**
     * @brief Configure cascaded filtering system
     * @param enabled Enable cascaded filtering
     * @param maxStages Maximum cascade stages (1-5)
     * @param stageHysteresis Stage switching hysteresis (0.05f to 0.5f)
     * @param adaptiveStages Enable adaptive stage count based on velocity
     */
    void setCascadedParameters(bool enabled = true,
                              int maxStages = 4,
                              float stageHysteresis = 0.2f,
                              bool adaptiveStages = true);

    /**
     * @brief Configure perceptual velocity detection
     * @param enabled Enable perceptual analysis
     * @param minFrequency Minimum analysis frequency (10 Hz to 100 Hz)
     * @param maxFrequency Maximum analysis frequency (10000 Hz to 22050 Hz)
     * @param perceptualSensitivity Perceptual sensitivity factor (0.1f to 5.0f)
     * @param useSimplifiedAnalysis Use simplified analysis for performance
     */
    void setPerceptualParameters(bool enabled = true,
                                double minFrequency = 20.0,
                                double maxFrequency = 20000.0,
                                float perceptualSensitivity = 1.8f,
                                bool useSimplifiedAnalysis = false);

    /**
     * @brief Configure intelligent decision-making system
     * @param velocityThreshold Velocity threshold for mode switching (0.01f to 2.0f)
     * @param complexityThreshold Signal complexity threshold (0.1f to 1.0f)
     * @param stabilityFactor Stability factor for decision persistence (0.5f to 3.0f)
     * @param adaptationRate Rate of mode adaptation (0.1f to 1.0f)
     */
    void setDecisionParameters(float velocityThreshold = 0.2f,
                              float complexityThreshold = 0.4f,
                              float stabilityFactor = 1.2f,
                              float adaptationRate = 0.6f);

    // === Backward Compatibility Interface ===

    /**
     * @brief Legacy AdaptiveSmoother compatibility method
     * @return Reference to internal SimpleAdaptiveSmoother for compatibility
     */
    SimpleAdaptiveSmoother& getLegacySmoother() { return mLegacySmoother; }
    const SimpleAdaptiveSmoother& getLegacySmoother() const { return mLegacySmoother; }

    /**
     * @brief Enable/disable legacy compatibility mode
     * @param enabled True to enable full compatibility with existing AdaptiveSmoother
     * @param preserveSettings True to preserve existing AdaptiveSmoother settings
     */
    void setLegacyCompatibilityMode(bool enabled, bool preserveSettings = true);

    // === Status and Debugging Interface ===

    /**
     * @brief Get current smoothing mode (actual mode being used)
     * @return Currently active smoothing mode
     */
    SmoothingMode getCurrentMode() const { return mCurrentMode; }

    /**
     * @brief Get current performance metrics
     * @param cpuUsage [out] Estimated CPU usage (0.0-1.0)
     * @param latency [out] Processing latency in samples
     * @param quality [out] Quality metric (0.0-1.0)
     */
    void getPerformanceMetrics(float& cpuUsage, float& latency, float& quality) const;

    /**
     * @brief Get detailed status information for debugging
     * @param velocity [out] Current velocity estimate
     * @param timeConstant [out] Current time constant
     * @param stageCount [out] Current cascade stage count
     * @param perceptualVelocity [out] Perceptual velocity estimate
     * @param decisionConfidence [out] Decision confidence (0.0-1.0)
     */
    void getDetailedStatus(float& velocity, float& timeConstant, int& stageCount,
                          float& perceptualVelocity, float& decisionConfidence) const;

    /**
     * @brief Get engine utilization statistics
     * @param enhancedUsage [out] Enhanced engine usage percentage
     * @param cascadedUsage [out] Cascaded engine usage percentage
     * @param perceptualUsage [out] Perceptual engine usage percentage
     * @param traditionalUsage [out] Traditional engine usage percentage
     */
    void getEngineUtilization(float& enhancedUsage, float& cascadedUsage,
                             float& perceptualUsage, float& traditionalUsage) const;

    /**
     * @brief Check if system is in optimal operating state
     * @return True if all engines are functioning optimally
     */
    bool isOptimalState() const;

    /**
     * @brief Get recommended settings for current parameter type
     * @param mode [out] Recommended smoothing mode
     * @param profile [out] Recommended performance profile
     * @param adaptiveParams [out] Recommended adaptive parameters (4 values)
     */
    void getRecommendedSettings(SmoothingMode& mode, PerformanceProfile& profile,
                               float adaptiveParams[4]) const;

    // === Fallback and Recovery Interface ===

    /**
     * @brief Force fallback to simpler mode (emergency recovery)
     * @param level Fallback level (0=Enhanced, 1=Perceptual, 2=Cascaded, 3=Traditional)
     */
    void forceFallback(int level);

    /**
     * @brief Enable/disable automatic fallback on performance issues
     * @param enabled True to enable automatic fallback
     * @param cpuThreshold CPU usage threshold for fallback (0.5-1.0)
     */
    void setAutomaticFallback(bool enabled, float cpuThreshold = 0.8f);

    /**
     * @brief Get fallback status and history
     * @param currentLevel [out] Current fallback level (0-3)
     * @param fallbackCount [out] Number of fallbacks since last reset
     * @param isRecovering [out] True if system is recovering from fallback
     */
    void getFallbackStatus(int& currentLevel, int& fallbackCount, bool& isRecovering) const;

private:
    // === Core Engine Components ===
    std::unique_ptr<CascadedSmoother> mCascadedSmoother;        // Enhanced cascaded filtering
    std::unique_ptr<PerceptualVelocityDetector> mPerceptualDetector; // Perceptual analysis
    SimpleAdaptiveSmoother mLegacySmoother;                     // Legacy compatibility
    MultiParameterCascadedSmoother mMultiSmoother;              // Multi-parameter support

    // === Configuration State ===
    double mSampleRate;                     // Current sample rate
    SmoothingMode mConfiguredMode;          // User-configured mode
    SmoothingMode mCurrentMode;             // Currently active mode
    PerformanceProfile mPerformanceProfile; // Performance optimization profile
    ParameterType mParameterType;           // Parameter type hint

    // === Adaptive Behavior Parameters ===
    float mFastTimeConstant;                // Fast response time constant
    float mSlowTimeConstant;                // Slow response time constant
    float mVelocitySensitivity;             // Velocity sensitivity factor
    float mHysteresisThreshold;             // Mode switching hysteresis
    float mAutoSwitchThreshold;             // Automatic mode switching threshold

    // === Decision System State ===
    float mVelocityThreshold;               // Velocity threshold for decisions
    float mComplexityThreshold;             // Signal complexity threshold
    float mStabilityFactor;                 // Decision stability factor
    float mAdaptationRate;                  // Mode adaptation rate
    float mDecisionConfidence;              // Current decision confidence

    // === Performance Tracking ===
    float mCpuUsageEstimate;                // Estimated CPU usage
    float mProcessingLatency;               // Processing latency in samples
    float mQualityMetric;                   // Quality assessment metric
    std::array<float, 4> mEngineUsage;      // Usage statistics for each engine

    // === Fallback System ===
    bool mLegacyCompatibilityMode;          // Legacy compatibility enabled
    bool mAutomaticFallbackEnabled;         // Automatic fallback enabled
    float mCpuThreshold;                    // CPU threshold for fallback
    int mCurrentFallbackLevel;              // Current fallback level (0-3)
    int mFallbackCount;                     // Fallback occurrence counter
    bool mIsRecovering;                     // Recovery state flag

    // === Signal Analysis State ===
    float mCurrentVelocity;                 // Current velocity estimate
    float mPreviousVelocity;                // Previous velocity for hysteresis
    float mPerceptualVelocity;              // Current perceptual velocity
    float mSignalComplexity;                // Current signal complexity measure
    float mStabilityMeasure;                // Current stability measure

    // === Internal Processing State ===
    float mPreviousInput;                   // Previous input for velocity calculation
    float mPreviousOutput;                  // Previous output for continuity
    bool mInitialized;                      // Initialization state flag
    int mSampleCounter;                     // Sample counter for periodic updates

    // === Configuration Caching ===
    bool mCascadedEnabled;                  // Cascaded filtering enabled
    int mMaxStages;                         // Maximum cascade stages
    float mStageHysteresis;                 // Stage switching hysteresis
    bool mAdaptiveStages;                   // Adaptive stage count enabled

    bool mPerceptualEnabled;                // Perceptual analysis enabled
    double mMinFrequency;                   // Minimum analysis frequency
    double mMaxFrequency;                   // Maximum analysis frequency
    float mPerceptualSensitivity;           // Perceptual sensitivity factor
    bool mUseSimplifiedAnalysis;            // Use simplified perceptual analysis

    // === Core Processing Methods ===

    /**
     * @brief Analyze input signal characteristics for decision-making
     * @param input Current input sample
     */
    void analyzeSignalCharacteristics(float input);

    /**
     * @brief Make intelligent decision about optimal smoothing mode
     * @return Optimal smoothing mode for current conditions
     */
    SmoothingMode makeIntelligentDecision();

    /**
     * @brief Process sample using enhanced hybrid approach
     * @param input Current input sample
     * @return Smoothed output
     */
    float processEnhanced(float input);

    /**
     * @brief Process sample using perceptual analysis only
     * @param input Current input sample
     * @return Smoothed output
     */
    float processPerceptual(float input);

    /**
     * @brief Process sample using cascaded filtering only
     * @param input Current input sample
     * @return Smoothed output
     */
    float processCascaded(float input);

    /**
     * @brief Process sample using traditional adaptive smoothing
     * @param input Current input sample
     * @return Smoothed output
     */
    float processTraditional(float input);

    /**
     * @brief Update performance metrics and optimization parameters
     */
    void updatePerformanceMetrics();

    /**
     * @brief Update engine configurations based on current parameters
     */
    void updateEngineConfigurations();

    /**
     * @brief Check if fallback is needed and execute if necessary
     * @return True if fallback was triggered
     */
    bool checkAndExecuteFallback();

    /**
     * @brief Calculate velocity using multiple approaches and select best
     * @param input Current input sample
     * @return Best velocity estimate
     */
    float calculateHybridVelocity(float input);

    /**
     * @brief Apply parameter-specific optimizations
     * @param velocity Current velocity estimate
     * @return Optimized velocity for current parameter type
     */
    float applyParameterOptimizations(float velocity);

    /**
     * @brief Calculate signal complexity measure for decision-making
     * @param input Current input sample
     * @return Signal complexity measure (0.0-1.0)
     */
    float calculateSignalComplexity(float input);

    /**
     * @brief Calculate decision confidence based on signal characteristics
     * @return Decision confidence (0.0-1.0)
     */
    float calculateDecisionConfidence();

    /**
     * @brief Apply performance profile optimizations
     */
    void applyPerformanceProfile();

    /**
     * @brief Initialize engine configurations based on parameter type
     */
    void initializeEngines();

    /**
     * @brief Validate and clamp configuration parameters
     */
    void validateConfiguration();

    /**
     * @brief Utility function to clamp float values
     * @param value Input value
     * @param minVal Minimum value
     * @param maxVal Maximum value
     * @return Clamped value
     */
    inline float clamp(float value, float minVal, float maxVal) const {
        return std::max(minVal, std::min(maxVal, value));
    }

    /**
     * @brief Utility function to clamp integer values
     * @param value Input value
     * @param minVal Minimum value
     * @param maxVal Maximum value
     * @return Clamped value
     */
    inline int clampInt(int value, int minVal, int maxVal) const {
        return std::max(minVal, std::min(maxVal, value));
    }

    /**
     * @brief Convert smoothing mode enum to string for debugging
     * @param mode Smoothing mode
     * @return String representation
     */
    static const char* smoothingModeToString(SmoothingMode mode);

    /**
     * @brief Convert performance profile enum to string for debugging
     * @param profile Performance profile
     * @return String representation
     */
    static const char* performanceProfileToString(PerformanceProfile profile);

    /**
     * @brief Convert parameter type enum to string for debugging
     * @param type Parameter type
     * @return String representation
     */
    static const char* parameterTypeToString(ParameterType type);
};

/**
 * @class EnhancedMultiParameterSmoother
 * @brief Multi-parameter version of EnhancedAdaptiveSmoother for complex processors
 *
 * Provides coordinated smoothing for multiple related parameters with:
 * - Shared decision-making across parameters
 * - Cross-parameter optimization
 * - Synchronized mode switching
 * - Collective fallback management
 */
class EnhancedMultiParameterSmoother {
public:
    static constexpr int MAX_PARAMETERS = 8;

    /**
     * @brief Constructor for multi-parameter smoothing
     * @param parameterCount Number of parameters (1-8)
     * @param sampleRate Sample rate in Hz
     * @param defaultParameterType Default parameter type for all parameters
     */
    EnhancedMultiParameterSmoother(int parameterCount = 4,
                                  double sampleRate = 44100.0,
                                  EnhancedAdaptiveSmoother::ParameterType defaultParameterType =
                                      EnhancedAdaptiveSmoother::ParameterType::Generic);

    ~EnhancedMultiParameterSmoother() = default;

    /**
     * @brief Initialize multi-parameter system
     * @param parameterCount Number of parameters
     * @param sampleRate Sample rate in Hz
     */
    void initialize(int parameterCount, double sampleRate);

    /**
     * @brief Process samples for all parameters
     * @param inputs Array of input values (parameterCount elements)
     * @param outputs Array to store output values (parameterCount elements)
     */
    void processAllSamples(const float* inputs, float* outputs);

    /**
     * @brief Process sample for specific parameter
     * @param parameterIndex Parameter index (0 to parameterCount-1)
     * @param input Input sample value
     * @return Smoothed output value
     */
    float processSample(int parameterIndex, float input);

    /**
     * @brief Configure parameter type for specific parameter
     * @param parameterIndex Parameter index
     * @param parameterType Parameter type
     */
    void setParameterType(int parameterIndex, EnhancedAdaptiveSmoother::ParameterType parameterType);

    /**
     * @brief Set global smoothing mode for all parameters
     * @param mode Smoothing mode
     */
    void setGlobalSmoothingMode(EnhancedAdaptiveSmoother::SmoothingMode mode);

    /**
     * @brief Set performance profile for all parameters
     * @param profile Performance profile
     */
    void setGlobalPerformanceProfile(EnhancedAdaptiveSmoother::PerformanceProfile profile);

    /**
     * @brief Reset all parameter smoothers
     */
    void resetAll();

    /**
     * @brief Reset all smoothers to specific values
     * @param values Array of values (parameterCount elements)
     */
    void resetAll(const float* values);

    /**
     * @brief Get smoother for specific parameter
     * @param parameterIndex Parameter index
     * @return Reference to EnhancedAdaptiveSmoother
     */
    EnhancedAdaptiveSmoother& getSmoother(int parameterIndex);
    const EnhancedAdaptiveSmoother& getSmoother(int parameterIndex) const;

    /**
     * @brief Get overall system performance metrics
     * @param avgCpuUsage [out] Average CPU usage across all parameters
     * @param maxLatency [out] Maximum latency across all parameters
     * @param avgQuality [out] Average quality metric
     */
    void getSystemMetrics(float& avgCpuUsage, float& maxLatency, float& avgQuality) const;

    /**
     * @brief Get number of active parameters
     * @return Parameter count
     */
    int getParameterCount() const { return mParameterCount; }

private:
    std::array<std::unique_ptr<EnhancedAdaptiveSmoother>, MAX_PARAMETERS> mSmoothers;
    int mParameterCount;
    bool mInitialized;

    // Coordinated decision-making state
    EnhancedAdaptiveSmoother::SmoothingMode mGlobalMode;
    float mGlobalDecisionConfidence;
    bool mCoordinatedMode;

    /**
     * @brief Validate parameter index
     * @param parameterIndex Index to validate
     * @return True if valid
     */
    bool isValidParameterIndex(int parameterIndex) const {
        return parameterIndex >= 0 && parameterIndex < mParameterCount;
    }

    /**
     * @brief Update coordinated decision-making
     */
    void updateCoordinatedDecisions();
};

} // namespace WaterStick