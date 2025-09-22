# Enhanced Perceptual Velocity Analysis System Design

## Executive Summary

This document specifies an enhanced velocity calculation system that replaces simple finite difference methods with perceptual velocity analysis based on psychoacoustic principles. The system integrates frequency-domain analysis, just noticeable differences (JND), and Bark-scale perceptual thresholds to provide musically meaningful parameter smoothing for the WaterStick comb processor.

## 1. Theoretical Foundation

### 1.1 Perceptual Velocity Concept

Traditional velocity calculation using finite differences (v = Δx/Δt) treats all parameter changes equally. Perceptual velocity analysis recognizes that musical significance depends on:

- **Frequency content** of parameter changes
- **Just Noticeable Differences (JND)** thresholds
- **Bark-scale perceptual weighting**
- **Parameter-specific musical sensitivity**

### 1.2 Bark Scale Integration

The Bark scale divides human hearing into 24 critical bands, each 1 Bark wide:
- Frequency range: 20Hz - 20kHz mapped to 0-24 Bark
- Below 500Hz: approximately linear frequency mapping
- Above 500Hz: approximately logarithmic frequency mapping

**Bark-to-Frequency Conversion:**
```
f(bark) = 1960 * (bark + 0.53) / (26.28 - bark)  for bark < 15.5
f(bark) = 20000 * exp(0.07 * (bark - 20.1))      for bark >= 15.5
```

### 1.3 Just Noticeable Differences (JND)

Perceptual thresholds for different parameter types:
- **Comb Size (Pitch)**: ~0.3% frequency change = 1 JND
- **Pitch CV**: ~5 cents = 1 JND
- **Time Domain**: ~2-3ms delay change = 1 JND
- **Amplitude**: ~0.5-1.0 dB = 1 JND

## 2. System Architecture

### 2.1 Enhanced AdaptiveSmoother Class

```cpp
class PerceptualVelocityAnalyzer {
public:
    struct PerceptualConfig {
        float jndThreshold;           // JND threshold for this parameter
        float barkWeighting;          // Bark-scale weighting factor
        float musicalSensitivity;     // Musical importance scaling
        float adaptationRate;         // Perceptual adaptation rate
    };

    // Frequency-domain velocity analysis
    float calculatePerceptualVelocity(float currentValue,
                                    const PerceptualConfig& config);

    // Bark-scale frequency mapping
    float frequencyToBark(float frequency);
    float barkToFrequency(float bark);

private:
    // Short-time FFT analysis (64-128 sample window)
    std::vector<float> mAnalysisWindow;
    std::vector<std::complex<float>> mFFTBuffer;

    // Perceptual weighting filters
    std::vector<float> mBarkWeights;

    // JND threshold tracking
    float mJNDAccumulator;
    float mAdaptationState;
};
```

### 2.2 Parameter-Specific Configurations

#### 2.2.1 Comb Size Parameter
```cpp
PerceptualConfig combSizeConfig = {
    .jndThreshold = 0.003f,        // 0.3% frequency change
    .barkWeighting = 2.0f,         // High bark sensitivity
    .musicalSensitivity = 2.5f,    // Very musically significant
    .adaptationRate = 0.1f         // Fast adaptation (100ms)
};
```

#### 2.2.2 Pitch CV Parameter
```cpp
PerceptualConfig pitchCVConfig = {
    .jndThreshold = 0.05f,         // 5 cents
    .barkWeighting = 1.8f,         // High bark sensitivity
    .musicalSensitivity = 2.0f,    // Highly musically significant
    .adaptationRate = 0.15f        // Moderate adaptation (150ms)
};
```

## 3. Frequency-Domain Analysis Implementation

### 3.1 Short-Time Fourier Transform (STFT)

```cpp
class PerceptualSTFT {
private:
    static constexpr int WINDOW_SIZE = 128;    // ~3ms at 44.1kHz
    static constexpr int HOP_SIZE = 32;        // 75% overlap
    static constexpr int NUM_BARK_BANDS = 24;

    std::array<float, WINDOW_SIZE> mHannWindow;
    std::array<std::complex<float>, WINDOW_SIZE> mFFTBuffer;
    std::array<float, NUM_BARK_BANDS> mBarkSpectrum;

public:
    void processFrame(const float* samples);
    float calculateSpectralVelocity(const std::array<float, NUM_BARK_BANDS>& prevSpectrum);
    void getBarkSpectrum(std::array<float, NUM_BARK_BANDS>& spectrum);
};
```

### 3.2 Bark-Scale Spectral Analysis

```cpp
void PerceptualSTFT::processFrame(const float* samples) {
    // Apply Hann window
    for (int i = 0; i < WINDOW_SIZE; ++i) {
        mFFTBuffer[i] = samples[i] * mHannWindow[i];
    }

    // Compute FFT
    performFFT(mFFTBuffer.data(), WINDOW_SIZE);

    // Map FFT bins to Bark bands
    mapToBarkScale();
}

void PerceptualSTFT::mapToBarkScale() {
    const float sampleRate = 44100.0f;
    const float binWidth = sampleRate / WINDOW_SIZE;

    std::fill(mBarkSpectrum.begin(), mBarkSpectrum.end(), 0.0f);

    for (int bin = 0; bin < WINDOW_SIZE/2; ++bin) {
        float frequency = bin * binWidth;
        int barkBand = static_cast<int>(frequencyToBark(frequency));

        if (barkBand >= 0 && barkBand < NUM_BARK_BANDS) {
            float magnitude = std::abs(mFFTBuffer[bin]);
            mBarkSpectrum[barkBand] += magnitude * magnitude; // Power spectrum
        }
    }

    // Apply perceptual weighting
    for (int band = 0; band < NUM_BARK_BANDS; ++band) {
        mBarkSpectrum[band] = std::sqrt(mBarkSpectrum[band]); // Back to magnitude
    }
}
```

## 4. Perceptual Velocity Calculation

### 4.1 Multi-Domain Velocity Analysis

```cpp
struct VelocityComponents {
    float temporalVelocity;      // Traditional finite difference
    float spectralVelocity;      // Frequency-domain changes
    float perceptualVelocity;    // JND-weighted velocity
    float musicalVelocity;       // Musically-weighted final value
};

VelocityComponents PerceptualVelocityAnalyzer::analyzeVelocity(
    float currentValue,
    const PerceptualConfig& config) {

    VelocityComponents result = {};

    // 1. Traditional temporal velocity
    result.temporalVelocity = (currentValue - mPrevValue) / mSampleTime;

    // 2. Spectral velocity (if frequency-domain analysis enabled)
    if (mSpectralAnalysisEnabled) {
        float samples[] = {currentValue}; // Convert parameter to signal
        mSTFT.processFrame(samples);
        result.spectralVelocity = mSTFT.calculateSpectralVelocity(mPrevSpectrum);
        mSTFT.getBarkSpectrum(mPrevSpectrum);
    }

    // 3. JND-weighted velocity
    float jndNormalizedChange = std::abs(result.temporalVelocity) / config.jndThreshold;
    result.perceptualVelocity = jndNormalizedChange * config.barkWeighting;

    // 4. Musical significance weighting
    result.musicalVelocity = result.perceptualVelocity * config.musicalSensitivity;

    // Apply perceptual adaptation
    updateAdaptationState(result.musicalVelocity, config.adaptationRate);

    return result;
}
```

### 4.2 Adaptive Threshold Calculation

```cpp
float PerceptualVelocityAnalyzer::calculateAdaptiveThreshold(
    const VelocityComponents& velocity,
    const PerceptualConfig& config) {

    // Base threshold from JND
    float baseThreshold = config.jndThreshold;

    // Spectral complexity factor
    float spectralFactor = 1.0f;
    if (mSpectralAnalysisEnabled) {
        spectralFactor = 1.0f + 0.5f * velocity.spectralVelocity;
    }

    // Perceptual adaptation (Weber-Fechner law approximation)
    float adaptationFactor = 1.0f + 0.3f * std::log10(1.0f + mAdaptationState);

    // Musical context weighting
    float musicalFactor = config.musicalSensitivity;

    return baseThreshold * spectralFactor * adaptationFactor / musicalFactor;
}
```

## 5. Integration with Existing AdaptiveSmoother

### 5.1 Enhanced AdaptiveSmoother Interface

```cpp
class AdaptiveSmoother {
public:
    // Enhanced constructor with perceptual analysis
    AdaptiveSmoother(double sampleRate = 44100.0,
                     bool enablePerceptualAnalysis = true,
                     bool enableSpectralAnalysis = false);

    // Configure perceptual analysis
    void setPerceptualConfig(const PerceptualVelocityAnalyzer::PerceptualConfig& config);

    // Enhanced velocity calculation modes
    enum VelocityMode {
        FINITE_DIFFERENCE,    // Traditional method (fallback)
        PERCEPTUAL_JND,      // JND-based analysis
        SPECTRAL_BARK,       // Bark-scale spectral analysis
        HYBRID_ADAPTIVE      // Adaptive combination
    };

    void setVelocityMode(VelocityMode mode);

private:
    std::unique_ptr<PerceptualVelocityAnalyzer> mPerceptualAnalyzer;
    VelocityMode mVelocityMode;
    bool mPerceptualEnabled;

    // Enhanced velocity calculation
    float calculateEnhancedVelocity(float input) override;
};
```

### 5.2 Fallback Mechanisms

```cpp
float AdaptiveSmoother::calculateEnhancedVelocity(float input) {
    switch (mVelocityMode) {
        case FINITE_DIFFERENCE:
            return calculateVelocity(input); // Existing method

        case PERCEPTUAL_JND:
            if (mPerceptualAnalyzer) {
                auto components = mPerceptualAnalyzer->analyzeVelocity(input, mPerceptualConfig);
                return components.perceptualVelocity;
            }
            // Fallback to finite difference
            return calculateVelocity(input);

        case SPECTRAL_BARK:
            if (mPerceptualAnalyzer && mPerceptualAnalyzer->isSpectralEnabled()) {
                auto components = mPerceptualAnalyzer->analyzeVelocity(input, mPerceptualConfig);
                return components.spectralVelocity;
            }
            // Fallback to perceptual or finite difference
            return mPerceptualAnalyzer ?
                   mPerceptualAnalyzer->analyzeVelocity(input, mPerceptualConfig).perceptualVelocity :
                   calculateVelocity(input);

        case HYBRID_ADAPTIVE:
            return calculateHybridVelocity(input);

        default:
            return calculateVelocity(input);
    }
}
```

### 5.3 Hybrid Adaptive Mode

```cpp
float AdaptiveSmoother::calculateHybridVelocity(float input) {
    // Get all velocity components
    float finiteDiffVel = calculateVelocity(input);

    if (!mPerceptualAnalyzer) {
        return finiteDiffVel;
    }

    auto components = mPerceptualAnalyzer->analyzeVelocity(input, mPerceptualConfig);

    // Adaptive weighting based on parameter change magnitude
    float changeMagnitude = std::abs(finiteDiffVel);

    // For small changes: prefer perceptual analysis
    // For large changes: prefer finite difference (more responsive)
    float perceptualWeight = 1.0f / (1.0f + changeMagnitude * 10.0f);
    float temporalWeight = 1.0f - perceptualWeight;

    return perceptualWeight * components.musicalVelocity +
           temporalWeight * finiteDiffVel;
}
```

## 6. Parameter-Specific Calibration

### 6.1 Comb Size vs Pitch CV Optimization

```cpp
class CombParameterSmoother {
private:
    struct CombParameterConfig {
        // Comb size: direct pitch relationship
        PerceptualVelocityAnalyzer::PerceptualConfig combSize = {
            .jndThreshold = 0.003f,     // 0.3% pitch change
            .barkWeighting = 2.5f,      // High frequency sensitivity
            .musicalSensitivity = 3.0f, // Critical for pitch perception
            .adaptationRate = 0.08f     // Fast adaptation (80ms)
        };

        // Pitch CV: modulation-based changes
        PerceptualVelocityAnalyzer::PerceptualConfig pitchCV = {
            .jndThreshold = 0.05f,      // 5 cents
            .barkWeighting = 2.0f,      // High frequency sensitivity
            .musicalSensitivity = 2.5f, // Important for musical expression
            .adaptationRate = 0.12f     // Moderate adaptation (120ms)
        };
    };

public:
    void calibrateForMusicalContext(float fundamentalFreq, float combSizeRange);
};

void CombParameterSmoother::calibrateForMusicalContext(
    float fundamentalFreq, float combSizeRange) {

    // Adjust JND thresholds based on fundamental frequency
    float barkPosition = mCombSizeSmoother.getPerceptualAnalyzer()->frequencyToBark(fundamentalFreq);

    // Lower frequencies need finer JND thresholds
    float frequencyFactor = 1.0f + 0.5f * std::exp(-barkPosition / 5.0f);

    auto combConfig = mConfig.combSize;
    combConfig.jndThreshold /= frequencyFactor;
    combConfig.barkWeighting *= (1.0f + barkPosition / 24.0f);

    mCombSizeSmoother.setPerceptualConfig(combConfig);

    // Adjust pitch CV sensitivity based on comb size range
    auto pitchConfig = mConfig.pitchCV;
    pitchConfig.musicalSensitivity *= (1.0f + combSizeRange * 2.0f);

    mPitchCVSmoother.setPerceptualConfig(pitchConfig);
}
```

## 7. Mathematical Formulations

### 7.1 Smooth Transition Functions

```cpp
// Exponential transition between velocity modes
float calculateModeTransition(float velocity, float threshold) {
    float normalizedVel = velocity / threshold;
    return 1.0f - std::exp(-normalizedVel * normalizedVel);
}

// Sigmoid transition for hysteresis
float calculateHysteresisTransition(float velocity, float lowerThresh, float upperThresh) {
    if (velocity <= lowerThresh) return 0.0f;
    if (velocity >= upperThresh) return 1.0f;

    float normalizedPos = (velocity - lowerThresh) / (upperThresh - lowerThresh);
    return 0.5f * (1.0f + std::tanh(6.0f * (normalizedPos - 0.5f)));
}

// Bark-weighted frequency importance
float calculateBarkWeight(float frequency) {
    float bark = frequencyToBark(frequency);
    // Peak sensitivity around 1-4 Bark (1000-3400 Hz)
    float peakResponse = std::exp(-0.5f * std::pow((bark - 2.5f) / 2.0f, 2.0f));
    float baseResponse = 0.3f; // Minimum weight
    return baseResponse + 0.7f * peakResponse;
}
```

### 7.2 Perceptual Time Constant Mapping

```cpp
float mapPerceptualVelocityToTimeConstant(float perceptualVelocity,
                                        float fastTC, float slowTC) {
    // Use power law based on Stevens' Law (psychophysical scaling)
    const float stevensExponent = 0.6f; // Typical for temporal perception

    float normalizedVel = std::pow(perceptualVelocity, stevensExponent);
    float exponentialFactor = std::exp(-normalizedVel);

    return fastTC + (slowTC - fastTC) * exponentialFactor;
}
```

## 8. Performance Optimization

### 8.1 Real-Time Processing Constraints

```cpp
class OptimizedPerceptualAnalyzer {
private:
    // Circular buffer for streaming analysis
    static constexpr int BUFFER_SIZE = 256;
    std::array<float, BUFFER_SIZE> mCircularBuffer;
    int mBufferIndex = 0;

    // Pre-computed lookup tables
    std::array<float, 1024> mBarkLookup;
    std::array<float, 24> mBarkWeights;

    // SIMD-optimized FFT (if available)
    #ifdef USE_SIMD_FFT
    alignas(16) std::array<float, BUFFER_SIZE> mSIMDBuffer;
    #endif

public:
    void initializeLookupTables();
    float fastBarkConversion(float frequency);
};

void OptimizedPerceptualAnalyzer::initializeLookupTables() {
    // Pre-compute Bark conversion for common frequency range
    for (int i = 0; i < 1024; ++i) {
        float freq = 20.0f + i * (20000.0f - 20.0f) / 1023.0f;
        mBarkLookup[i] = preciseFrequencyToBark(freq);
    }

    // Pre-compute Bark weighting function
    for (int bark = 0; bark < 24; ++bark) {
        mBarkWeights[bark] = calculateBarkWeight(barkToFrequency(bark));
    }
}
```

### 8.2 Computational Budget Management

```cpp
class AdaptiveComputationManager {
private:
    float mCPUBudget = 0.1f;           // 10% CPU budget for perceptual analysis
    float mCurrentLoad = 0.0f;         // Current computational load
    bool mSpectralAnalysisEnabled = true;

public:
    bool shouldPerformSpectralAnalysis() {
        if (mCurrentLoad > mCPUBudget * 0.8f) {
            // Disable spectral analysis if approaching budget limit
            mSpectralAnalysisEnabled = false;
            return false;
        }
        return mSpectralAnalysisEnabled;
    }

    void updateComputationalLoad(float processingTime) {
        const float alpha = 0.1f; // Smoothing factor
        mCurrentLoad = alpha * processingTime + (1.0f - alpha) * mCurrentLoad;
    }
};
```

## 9. Implementation Phases

### Phase 1: Basic Perceptual Integration (Week 1)
- Implement JND-based velocity calculation
- Add perceptual configuration structures
- Create fallback mechanisms to existing finite difference

### Phase 2: Bark-Scale Analysis (Week 2)
- Implement Bark-scale frequency mapping
- Add basic spectral analysis capabilities
- Integrate with parameter-specific configurations

### Phase 3: Hybrid Adaptive System (Week 3)
- Implement hybrid velocity calculation
- Add adaptive mode switching
- Optimize for real-time performance

### Phase 4: Musical Calibration (Week 4)
- Implement parameter-specific calibration
- Add musical context awareness
- Performance profiling and optimization

## 10. Validation and Testing

### 10.1 Objective Metrics
- **Latency**: < 1ms additional processing delay
- **CPU Usage**: < 10% increase over finite difference
- **Memory**: < 64KB additional allocation
- **Accuracy**: Perceptual velocity within 5% of subjective assessments

### 10.2 Subjective Testing
- **A/B Testing**: Compare finite difference vs perceptual smoothing
- **Parameter Modulation**: Test with various automation curves
- **Musical Context**: Test with different musical material and tempos
- **User Preference**: Survey experienced audio engineers and musicians

## 11. Future Enhancements

### 11.1 Machine Learning Integration
- Train perceptual models on user interaction data
- Adaptive JND thresholds based on musical context
- Predictive parameter smoothing

### 11.2 Multi-Parameter Correlation
- Cross-parameter perceptual analysis
- Musical phrase-aware smoothing
- Tempo-synchronized perceptual adaptation

This design provides a comprehensive framework for implementing perceptual velocity analysis while maintaining compatibility with the existing AdaptiveSmoother system and ensuring robust real-time performance.