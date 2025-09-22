#pragma once

#include <cmath>
#include <algorithm>

namespace WaterStick {

/**
 * @class PerceptualVelocityDetector
 * @brief Psychoacoustic parameter velocity analysis using Bark scale and A-weighting
 *
 * This class provides perceptual analysis of parameter changes based on psychoacoustic
 * principles. It converts delay times to frequency domain, applies Bark scale conversion
 * for perceptual relevance, and includes A-weighting for frequency importance.
 *
 * Key Features:
 * - Bark scale conversion using Traunmüller formula for perceptual frequency mapping
 * - Logarithmic frequency ratio calculation for musical parameter control
 * - A-weighting factor for frequency importance weighting
 * - Real-time safe processing with configurable analysis modes
 * - Integration with WaterStick adaptive smoothing framework
 *
 * Mathematical Foundation:
 * - Frequency from delay: f = sampleRate / delayTime
 * - Bark scale (Traunmüller): Bark = 26.81 * f / (1960 + f) - 0.53
 * - A-weighting: approximated curve for frequency perceptual importance
 * - Perceptual velocity: v_p = log2(f_new / f_old) * A_weight(f_avg)
 */
class PerceptualVelocityDetector {
public:
    /**
     * @brief Constructor with configurable analysis parameters
     * @param sampleRate Sample rate in Hz for frequency calculations
     * @param minFrequency Minimum frequency for analysis bounds (default: 20 Hz)
     * @param maxFrequency Maximum frequency for analysis bounds (default: 20000 Hz)
     * @param velocitySensitivity Sensitivity multiplier for perceptual velocity (default: 1.0)
     */
    PerceptualVelocityDetector(double sampleRate = 44100.0,
                              double minFrequency = 20.0,
                              double maxFrequency = 20000.0,
                              float velocitySensitivity = 1.0f);

    ~PerceptualVelocityDetector() = default;

    /**
     * @brief Initialize or update sample rate
     * @param sampleRate New sample rate in Hz
     */
    void setSampleRate(double sampleRate);

    /**
     * @brief Configure analysis parameters
     * @param minFrequency Minimum frequency for analysis bounds (10 Hz to 100 Hz)
     * @param maxFrequency Maximum frequency for analysis bounds (10000 Hz to 22050 Hz)
     * @param velocitySensitivity Sensitivity multiplier (0.1f to 10.0f)
     */
    void setAnalysisParameters(double minFrequency,
                              double maxFrequency,
                              float velocitySensitivity);

    /**
     * @brief Analyze perceptual velocity of delay time changes
     * @param currentDelayTime Current delay time in seconds
     * @param previousDelayTime Previous delay time in seconds
     * @return Perceptual velocity incorporating Bark scale and A-weighting
     */
    float analyzeDelayTimeVelocity(float currentDelayTime, float previousDelayTime);

    /**
     * @brief Simplified perceptual velocity analysis for real-time performance
     * @param currentDelayTime Current delay time in seconds
     * @param previousDelayTime Previous delay time in seconds
     * @return Simplified perceptual velocity (faster computation)
     */
    float analyzeDelayTimeVelocitySimplified(float currentDelayTime, float previousDelayTime);

    /**
     * @brief Get Bark scale value for a given frequency
     * @param frequency Frequency in Hz
     * @return Bark scale value (0-24 Bark range)
     */
    static double frequencyToBark(double frequency);

    /**
     * @brief Get A-weighting factor for a given frequency
     * @param frequency Frequency in Hz
     * @return A-weighting factor (0.0 to 1.0, normalized)
     */
    static double getAWeightingFactor(double frequency);

    /**
     * @brief Convert delay time to equivalent frequency
     * @param delayTime Delay time in seconds
     * @param sampleRate Sample rate in Hz
     * @return Equivalent frequency in Hz
     */
    static double delayTimeToFrequency(double delayTime, double sampleRate);

    /**
     * @brief Calculate logarithmic frequency ratio for perceptual analysis
     * @param frequency1 First frequency in Hz
     * @param frequency2 Second frequency in Hz
     * @return Log2 frequency ratio (musical intervals)
     */
    static double calculateFrequencyRatio(double frequency1, double frequency2);

    /**
     * @brief Full perceptual analysis with all psychoacoustic factors
     * @param currentDelayTime Current delay time in seconds
     * @param previousDelayTime Previous delay time in seconds
     * @return Comprehensive perceptual velocity analysis
     */
    float fullPerceptualAnalysis(float currentDelayTime, float previousDelayTime);

    /**
     * @brief Reset detector state
     */
    void reset();

    /**
     * @brief Get current analysis parameters for debugging
     * @param minFreq [out] Current minimum frequency
     * @param maxFreq [out] Current maximum frequency
     * @param sensitivity [out] Current velocity sensitivity
     */
    void getAnalysisParameters(double& minFreq, double& maxFreq, float& sensitivity) const;

    /**
     * @brief Enable/disable different analysis modes
     * @param useBarkScale True to use Bark scale conversion
     * @param useAWeighting True to apply A-weighting factors
     * @param useFrequencyClipping True to clip frequencies to analysis bounds
     */
    void setAnalysisModes(bool useBarkScale, bool useAWeighting, bool useFrequencyClipping);

    /**
     * @brief Get frequency analysis for debugging
     * @param delayTime Delay time in seconds
     * @param frequency [out] Converted frequency
     * @param barkValue [out] Bark scale value
     * @param aWeight [out] A-weighting factor
     */
    void getFrequencyAnalysis(float delayTime, double& frequency, double& barkValue, double& aWeight) const;

private:
    // Analysis parameters
    double mSampleRate;          // Sample rate for frequency calculations
    double mMinFrequency;        // Minimum frequency for analysis bounds
    double mMaxFrequency;        // Maximum frequency for analysis bounds
    float mVelocitySensitivity;  // Velocity sensitivity multiplier

    // Analysis mode flags
    bool mUseBarkScale;          // Enable Bark scale conversion
    bool mUseAWeighting;         // Enable A-weighting factors
    bool mUseFrequencyClipping;  // Enable frequency bounds clipping

    // Cached values for optimization
    double mCachedMinBark;       // Cached Bark value for min frequency
    double mCachedMaxBark;       // Cached Bark value for max frequency
    double mCachedMinAWeight;    // Cached A-weight for min frequency
    double mCachedMaxAWeight;    // Cached A-weight for max frequency

    /**
     * @brief Update cached values when parameters change
     */
    void updateCachedValues();

    /**
     * @brief Clip frequency to analysis bounds
     * @param frequency Input frequency
     * @return Clipped frequency within bounds
     */
    double clipFrequency(double frequency) const;

    /**
     * @brief Apply velocity sensitivity scaling
     * @param rawVelocity Raw velocity value
     * @return Scaled velocity value
     */
    float applyVelocityScaling(float rawVelocity) const;

    /**
     * @brief Calculate Bark scale difference for perceptual distance
     * @param bark1 First Bark value
     * @param bark2 Second Bark value
     * @return Bark scale difference (perceptual distance)
     */
    static double calculateBarkDifference(double bark1, double bark2);

    /**
     * @brief Optimized A-weighting calculation for real-time use
     * @param frequency Frequency in Hz
     * @return Normalized A-weighting factor (simplified)
     */
    static double fastAWeighting(double frequency);

    /**
     * @brief Clamp value to specified range (optimized inline)
     * @param value Input value
     * @param minVal Minimum value
     * @param maxVal Maximum value
     * @return Clamped value
     */
    inline double clamp(double value, double minVal, double maxVal) const {
        return std::max(minVal, std::min(maxVal, value));
    }

    /**
     * @brief Fast logarithm base 2 approximation for real-time use
     * @param x Input value (must be positive)
     * @return Approximate log2(x)
     */
    static double fastLog2(double x);

    /**
     * @brief Safe division with epsilon protection
     * @param numerator Numerator value
     * @param denominator Denominator value
     * @return Safe division result
     */
    static double safeDivide(double numerator, double denominator);
};

/**
 * @class PerceptualParameterSmoother
 * @brief Specialized perceptual velocity detector for parameter smoothing integration
 *
 * Extends PerceptualVelocityDetector with parameter-specific optimizations:
 * - Integration with existing AdaptiveSmoother architecture
 * - Parameter-aware perceptual scaling
 * - Real-time performance optimizations for audio processing
 */
class PerceptualParameterSmoother {
public:
    PerceptualParameterSmoother();
    ~PerceptualParameterSmoother() = default;

    /**
     * @brief Initialize with sample rate and parameter type
     * @param sampleRate Sample rate in Hz
     * @param parameterType Type hint for parameter-specific optimization
     */
    void initialize(double sampleRate, const char* parameterType = "delay");

    /**
     * @brief Configure perceptual analysis for parameter smoothing
     * @param delayTimeSensitivity Perceptual sensitivity for delay time changes
     * @param frequencyBounds Frequency analysis bounds (min, max)
     * @param useSimplifiedAnalysis True for faster computation
     */
    void setPerceptualParameters(float delayTimeSensitivity = 1.5f,
                                double frequencyBounds[2] = nullptr,
                                bool useSimplifiedAnalysis = true);

    /**
     * @brief Analyze parameter velocity with perceptual weighting
     * @param currentValue Current parameter value
     * @param previousValue Previous parameter value
     * @return Perceptually-weighted velocity for smoothing adaptation
     */
    float analyzeParameterVelocity(float currentValue, float previousValue);

    /**
     * @brief Reset perceptual analysis state
     */
    void reset();

    /**
     * @brief Enable/disable perceptual analysis (fallback to linear analysis)
     * @param enabled True to enable perceptual analysis
     */
    void setPerceptualEnabled(bool enabled);

    /**
     * @brief Get perceptual analysis debug information
     * @param frequency [out] Current equivalent frequency
     * @param barkValue [out] Current Bark scale value
     * @param aWeight [out] Current A-weighting factor
     * @param perceptualVelocity [out] Current perceptual velocity
     */
    void getPerceptualDebugInfo(double& frequency, double& barkValue,
                               double& aWeight, float& perceptualVelocity) const;

private:
    PerceptualVelocityDetector mDetector;  // Core perceptual analysis engine
    bool mPerceptualEnabled;               // Enable/disable perceptual analysis
    bool mUseSimplifiedAnalysis;           // Use simplified analysis for performance

    // Parameter type optimization
    const char* mParameterType;            // Parameter type hint for optimization

    // Cached analysis results for debugging
    double mLastFrequency;                 // Last computed frequency
    double mLastBarkValue;                 // Last computed Bark value
    double mLastAWeight;                   // Last computed A-weighting
    float mLastPerceptualVelocity;         // Last computed perceptual velocity

    /**
     * @brief Apply parameter-specific scaling based on parameter type
     * @param rawVelocity Raw perceptual velocity
     * @return Parameter-optimized velocity
     */
    float applyParameterScaling(float rawVelocity) const;

    /**
     * @brief Linear fallback velocity calculation
     * @param currentValue Current parameter value
     * @param previousValue Previous parameter value
     * @return Linear velocity estimate
     */
    float calculateLinearVelocity(float currentValue, float previousValue) const;
};

} // namespace WaterStick