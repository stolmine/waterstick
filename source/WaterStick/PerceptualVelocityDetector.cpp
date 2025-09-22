#include "PerceptualVelocityDetector.h"
#include <cmath>
#include <algorithm>

namespace WaterStick {

// Mathematical constants for psychoacoustic calculations
static constexpr double kEpsilon = 1e-12;           // Small value for safe division
static constexpr double kMinDelayTime = 1e-6;      // Minimum delay time (1 microsecond)
static constexpr double kMaxDelayTime = 10.0;      // Maximum delay time (10 seconds)
static constexpr double kBarkConstant1 = 26.81;    // Traunmüller formula constant
static constexpr double kBarkConstant2 = 1960.0;   // Traunmüller formula constant
static constexpr double kBarkOffset = 0.53;        // Traunmüller formula offset
static constexpr double kLog2Constant = 1.4426950408889634; // 1/ln(2) for fast log2

// A-weighting approximation constants (simplified for real-time use)
static constexpr double kAWeight1kHz = 1000.0;     // Reference frequency
static constexpr double kAWeightLowCutoff = 100.0; // Low frequency cutoff
static constexpr double kAWeightHighCutoff = 8000.0; // High frequency cutoff

PerceptualVelocityDetector::PerceptualVelocityDetector(double sampleRate,
                                                       double minFrequency,
                                                       double maxFrequency,
                                                       float velocitySensitivity)
    : mSampleRate(sampleRate)
    , mMinFrequency(minFrequency)
    , mMaxFrequency(maxFrequency)
    , mVelocitySensitivity(velocitySensitivity)
    , mUseBarkScale(true)
    , mUseAWeighting(true)
    , mUseFrequencyClipping(true)
    , mCachedMinBark(0.0)
    , mCachedMaxBark(0.0)
    , mCachedMinAWeight(0.0)
    , mCachedMaxAWeight(0.0)
{
    updateCachedValues();
}

void PerceptualVelocityDetector::setSampleRate(double sampleRate)
{
    mSampleRate = std::max(1000.0, sampleRate); // Minimum 1kHz for safety
}

void PerceptualVelocityDetector::setAnalysisParameters(double minFrequency,
                                                       double maxFrequency,
                                                       float velocitySensitivity)
{
    mMinFrequency = clamp(minFrequency, 10.0, 100.0);
    mMaxFrequency = clamp(maxFrequency, 10000.0, mSampleRate * 0.45); // Nyquist safety margin
    mVelocitySensitivity = std::max(0.1f, std::min(10.0f, velocitySensitivity));

    updateCachedValues();
}

float PerceptualVelocityDetector::analyzeDelayTimeVelocity(float currentDelayTime, float previousDelayTime)
{
    // Clamp delay times to safe bounds
    currentDelayTime = static_cast<float>(clamp(static_cast<double>(currentDelayTime), kMinDelayTime, kMaxDelayTime));
    previousDelayTime = static_cast<float>(clamp(static_cast<double>(previousDelayTime), kMinDelayTime, kMaxDelayTime));

    // Convert delay times to frequencies
    double currentFreq = delayTimeToFrequency(currentDelayTime, mSampleRate);
    double previousFreq = delayTimeToFrequency(previousDelayTime, mSampleRate);

    // Apply frequency clipping if enabled
    if (mUseFrequencyClipping) {
        currentFreq = clipFrequency(currentFreq);
        previousFreq = clipFrequency(previousFreq);
    }

    // Calculate base frequency ratio (musical intervals)
    double freqRatio = calculateFrequencyRatio(currentFreq, previousFreq);

    // Apply perceptual transformations
    float perceptualVelocity = static_cast<float>(freqRatio);

    if (mUseBarkScale) {
        // Convert to Bark scale for perceptual frequency mapping
        double currentBark = frequencyToBark(currentFreq);
        double previousBark = frequencyToBark(previousFreq);
        double barkDifference = calculateBarkDifference(currentBark, previousBark);

        // Weight by Bark scale difference (more perceptual than raw frequency)
        perceptualVelocity = static_cast<float>(barkDifference * freqRatio);
    }

    if (mUseAWeighting) {
        // Apply A-weighting for frequency importance
        double avgFreq = (currentFreq + previousFreq) * 0.5;
        double aWeight = getAWeightingFactor(avgFreq);
        perceptualVelocity *= static_cast<float>(aWeight);
    }

    // Apply velocity sensitivity scaling
    return applyVelocityScaling(perceptualVelocity);
}

float PerceptualVelocityDetector::analyzeDelayTimeVelocitySimplified(float currentDelayTime, float previousDelayTime)
{
    // Simplified version for real-time performance
    currentDelayTime = static_cast<float>(clamp(static_cast<double>(currentDelayTime), kMinDelayTime, kMaxDelayTime));
    previousDelayTime = static_cast<float>(clamp(static_cast<double>(previousDelayTime), kMinDelayTime, kMaxDelayTime));

    // Fast frequency conversion
    double currentFreq = mSampleRate / currentDelayTime;
    double previousFreq = mSampleRate / previousDelayTime;

    // Simple frequency ratio with fast log2
    double ratio = safeDivide(currentFreq, previousFreq);
    float velocity = static_cast<float>(fastLog2(ratio));

    // Apply basic A-weighting if enabled
    if (mUseAWeighting) {
        double avgFreq = (currentFreq + previousFreq) * 0.5;
        velocity *= static_cast<float>(fastAWeighting(avgFreq));
    }

    return applyVelocityScaling(velocity);
}

double PerceptualVelocityDetector::frequencyToBark(double frequency)
{
    // Traunmüller formula: Bark = 26.81 * f / (1960 + f) - 0.53
    return kBarkConstant1 * frequency / (kBarkConstant2 + frequency) - kBarkOffset;
}

double PerceptualVelocityDetector::getAWeightingFactor(double frequency)
{
    // Simplified A-weighting approximation for real-time use
    // Based on the A-weighting curve, normalized to 0-1 range

    if (frequency <= 0.0) return 0.0;

    // Low frequency rolloff
    double lowFactor = frequency / (frequency + kAWeightLowCutoff);
    lowFactor *= lowFactor; // Second-order rolloff

    // High frequency rolloff
    double highFactor = kAWeightHighCutoff / (frequency + kAWeightHighCutoff);

    // Peak around 1kHz
    double peakFactor = 1.0 - std::abs(frequency - kAWeight1kHz) / (kAWeight1kHz * 2.0);
    peakFactor = std::max(0.3, peakFactor); // Minimum weighting

    return lowFactor * highFactor * peakFactor;
}

double PerceptualVelocityDetector::delayTimeToFrequency(double delayTime, double sampleRate)
{
    return safeDivide(sampleRate, delayTime);
}

double PerceptualVelocityDetector::calculateFrequencyRatio(double frequency1, double frequency2)
{
    if (frequency1 <= 0.0 || frequency2 <= 0.0) return 0.0;
    return fastLog2(safeDivide(frequency1, frequency2));
}

float PerceptualVelocityDetector::fullPerceptualAnalysis(float currentDelayTime, float previousDelayTime)
{
    // Most comprehensive analysis with all psychoacoustic factors
    return analyzeDelayTimeVelocity(currentDelayTime, previousDelayTime);
}

void PerceptualVelocityDetector::reset()
{
    // Reset any internal state if needed
    updateCachedValues();
}

void PerceptualVelocityDetector::getAnalysisParameters(double& minFreq, double& maxFreq, float& sensitivity) const
{
    minFreq = mMinFrequency;
    maxFreq = mMaxFrequency;
    sensitivity = mVelocitySensitivity;
}

void PerceptualVelocityDetector::setAnalysisModes(bool useBarkScale, bool useAWeighting, bool useFrequencyClipping)
{
    mUseBarkScale = useBarkScale;
    mUseAWeighting = useAWeighting;
    mUseFrequencyClipping = useFrequencyClipping;
}

void PerceptualVelocityDetector::getFrequencyAnalysis(float delayTime, double& frequency, double& barkValue, double& aWeight) const
{
    frequency = delayTimeToFrequency(delayTime, mSampleRate);

    if (mUseFrequencyClipping) {
        frequency = clipFrequency(frequency);
    }

    barkValue = mUseBarkScale ? frequencyToBark(frequency) : frequency;
    aWeight = mUseAWeighting ? getAWeightingFactor(frequency) : 1.0;
}

// Private methods implementation

void PerceptualVelocityDetector::updateCachedValues()
{
    mCachedMinBark = frequencyToBark(mMinFrequency);
    mCachedMaxBark = frequencyToBark(mMaxFrequency);
    mCachedMinAWeight = getAWeightingFactor(mMinFrequency);
    mCachedMaxAWeight = getAWeightingFactor(mMaxFrequency);
}

double PerceptualVelocityDetector::clipFrequency(double frequency) const
{
    return clamp(frequency, mMinFrequency, mMaxFrequency);
}

float PerceptualVelocityDetector::applyVelocityScaling(float rawVelocity) const
{
    return rawVelocity * mVelocitySensitivity;
}

double PerceptualVelocityDetector::calculateBarkDifference(double bark1, double bark2)
{
    return std::abs(bark1 - bark2);
}

double PerceptualVelocityDetector::fastAWeighting(double frequency)
{
    // Fast approximation of A-weighting for real-time use
    if (frequency <= 0.0) return 0.0;

    // Simple piecewise linear approximation
    if (frequency < 100.0) {
        return frequency / 100.0 * 0.3; // Low frequency rolloff
    } else if (frequency < 1000.0) {
        return 0.3 + (frequency - 100.0) / 900.0 * 0.7; // Rising to peak
    } else if (frequency < 4000.0) {
        return 1.0; // Peak region
    } else {
        return std::max(0.3, 1.0 - (frequency - 4000.0) / 16000.0); // High frequency rolloff
    }
}

double PerceptualVelocityDetector::fastLog2(double x)
{
    if (x <= 0.0) return 0.0;
    return std::log(x) * kLog2Constant;
}

double PerceptualVelocityDetector::safeDivide(double numerator, double denominator)
{
    if (std::abs(denominator) < kEpsilon) {
        return 0.0;
    }
    return numerator / denominator;
}

// PerceptualParameterSmoother implementation

PerceptualParameterSmoother::PerceptualParameterSmoother()
    : mDetector()
    , mPerceptualEnabled(true)
    , mUseSimplifiedAnalysis(true)
    , mParameterType("delay")
    , mLastFrequency(0.0)
    , mLastBarkValue(0.0)
    , mLastAWeight(0.0)
    , mLastPerceptualVelocity(0.0f)
{
}

void PerceptualParameterSmoother::initialize(double sampleRate, const char* parameterType)
{
    mDetector.setSampleRate(sampleRate);
    mParameterType = parameterType ? parameterType : "delay";

    // Set parameter-specific defaults
    if (std::strcmp(mParameterType, "delay") == 0) {
        mDetector.setAnalysisParameters(20.0, 20000.0, 1.5f);
    } else if (std::strcmp(mParameterType, "comb") == 0) {
        mDetector.setAnalysisParameters(30.0, 15000.0, 2.0f);
    } else {
        mDetector.setAnalysisParameters(20.0, 20000.0, 1.0f);
    }
}

void PerceptualParameterSmoother::setPerceptualParameters(float delayTimeSensitivity,
                                                          double frequencyBounds[2],
                                                          bool useSimplifiedAnalysis)
{
    if (frequencyBounds) {
        mDetector.setAnalysisParameters(frequencyBounds[0], frequencyBounds[1], delayTimeSensitivity);
    } else {
        double minFreq, maxFreq;
        float currentSensitivity;
        mDetector.getAnalysisParameters(minFreq, maxFreq, currentSensitivity);
        mDetector.setAnalysisParameters(minFreq, maxFreq, delayTimeSensitivity);
    }

    mUseSimplifiedAnalysis = useSimplifiedAnalysis;
}

float PerceptualParameterSmoother::analyzeParameterVelocity(float currentValue, float previousValue)
{
    if (!mPerceptualEnabled) {
        return calculateLinearVelocity(currentValue, previousValue);
    }

    float perceptualVelocity;

    if (mUseSimplifiedAnalysis) {
        perceptualVelocity = mDetector.analyzeDelayTimeVelocitySimplified(currentValue, previousValue);
    } else {
        perceptualVelocity = mDetector.analyzeDelayTimeVelocity(currentValue, previousValue);
    }

    // Cache for debugging
    mLastPerceptualVelocity = perceptualVelocity;
    mDetector.getFrequencyAnalysis(currentValue, mLastFrequency, mLastBarkValue, mLastAWeight);

    return applyParameterScaling(perceptualVelocity);
}

void PerceptualParameterSmoother::reset()
{
    mDetector.reset();
    mLastFrequency = 0.0;
    mLastBarkValue = 0.0;
    mLastAWeight = 0.0;
    mLastPerceptualVelocity = 0.0f;
}

void PerceptualParameterSmoother::setPerceptualEnabled(bool enabled)
{
    mPerceptualEnabled = enabled;
}

void PerceptualParameterSmoother::getPerceptualDebugInfo(double& frequency, double& barkValue,
                                                         double& aWeight, float& perceptualVelocity) const
{
    frequency = mLastFrequency;
    barkValue = mLastBarkValue;
    aWeight = mLastAWeight;
    perceptualVelocity = mLastPerceptualVelocity;
}

float PerceptualParameterSmoother::applyParameterScaling(float rawVelocity) const
{
    // Parameter-specific velocity scaling
    if (std::strcmp(mParameterType, "delay") == 0) {
        return rawVelocity * 1.0f; // Standard scaling for delay parameters
    } else if (std::strcmp(mParameterType, "comb") == 0) {
        return rawVelocity * 1.3f; // Slightly more sensitive for comb parameters
    } else if (std::strcmp(mParameterType, "pitch") == 0) {
        return rawVelocity * 1.8f; // More sensitive for pitch parameters
    }

    return rawVelocity; // Default scaling
}

float PerceptualParameterSmoother::calculateLinearVelocity(float currentValue, float previousValue) const
{
    // Simple linear velocity fallback
    return std::abs(currentValue - previousValue);
}

} // namespace WaterStick