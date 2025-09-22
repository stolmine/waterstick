# WaterStick Real-Time Optimization System

## Overview

The WaterStick VST3 plugin includes a comprehensive real-time optimization system designed to maintain audio quality while ensuring real-time safety under all CPU load conditions. The system provides:

- **Lookup tables** for expensive mathematical operations
- **CPU usage monitoring** with automatic quality reduction
- **Emergency fallback mechanisms** for overload protection
- **SIMD optimizations** for high-performance processing
- **Cache-friendly memory layout** optimizations
- **Numerical stability** safeguards
- **Graceful degradation** under CPU pressure

## Architecture

### Core Components

1. **RealTimeOptimizer** - Main coordination class
2. **CPUMonitor** - Real-time CPU usage tracking
3. **QualityController** - Automatic quality level management
4. **LookupTable** - High-performance mathematical function tables
5. **SIMDOptimizer** - SIMD instruction optimizations
6. **MemoryOptimizer** - Cache-efficient memory management
7. **NumericalStabilizer** - Safe mathematical operations

### Quality Levels

The system operates at five quality levels with automatic transitions based on CPU load:

- **ULTRA** (4): Maximum quality, all features enabled
- **HIGH** (3): Full processing, standard operation
- **MEDIUM** (2): Moderate processing, some optimizations disabled
- **LOW** (1): Reduced processing, simplified algorithms
- **EMERGENCY** (0): Minimal processing, bypass non-essential features

## Integration Guide

### Basic Setup

```cpp
// In WaterStickProcessor initialization
void WaterStickProcessor::initialize(FUnknown* context)
{
    // Initialize the real-time optimizer
    mRealTimeOptimizer.initialize(mSampleRate, mBufferSize);

    // Connect optimizer to comb processor
    mCombProcessor.setRealTimeOptimizer(&mRealTimeOptimizer);
    mCombProcessor.setOptimizationEnabled(true);
}
```

### Processing Loop Integration

```cpp
tresult WaterStickProcessor::process(ProcessData& data)
{
    // Start CPU timing
    mRealTimeOptimizer.getCPUMonitor().startTiming();

    // Process audio...
    processAudio(data);

    // End CPU timing and update optimization state
    mRealTimeOptimizer.getCPUMonitor().endTiming();
    mRealTimeOptimizer.updatePerFrame();

    return kResultOk;
}
```

### Adaptive Smoother Integration

```cpp
// Configure adaptive smoothing with optimization
AdaptiveSmoother smoother;
smoother.setRealTimeOptimizer(&mRealTimeOptimizer);
smoother.setOptimizationEnabled(true);

// Enable advanced features
smoother.setCascadedEnabled(true, 5, 0.2f);
smoother.setPerceptualMapping(true);

// Process with automatic optimization
float smoothedOutput = smoother.processSample(input);
```

## Feature Details

### Lookup Tables

Pre-computed tables for expensive operations:

```cpp
// Exponential function lookup (-20 to 0 range)
const LookupTable<1024>& expTable = optimizer.getExpTable();
float result = expTable.lookup(-5.0f); // Fast exp(-5.0f)

// Natural logarithm lookup (1e-6 to 10 range)
const LookupTable<1024>& logTable = optimizer.getLogTable();
float result = logTable.lookup(2.5f); // Fast log(2.5f)

// Bark scale conversion (20Hz to 20kHz)
const LookupTable<512>& barkTable = optimizer.getBarkTable();
float bark = barkTable.lookup(1000.0f); // Fast frequency to Bark
```

### CPU Monitoring

Real-time CPU usage tracking with configurable thresholds:

```cpp
// Configure CPU thresholds
CPUMonitor& monitor = optimizer.getCPUMonitor();
monitor.setThresholds(60.0f, 80.0f, 95.0f); // Warning, Critical, Emergency

// Check CPU status
float cpuUsage = monitor.getSmoothedCPUUsage();
bool overloaded = monitor.isOverloaded(75.0f);
bool emergency = monitor.isEmergencyOverload();
```

### Quality Control

Automatic quality adjustment based on CPU load:

```cpp
QualityController& quality = optimizer.getQualityController();

// Get current quality level
QualityController::QualityLevel level = quality.getCurrentQuality();

// Get processing configuration
int maxStages;
bool enablePerceptual, enableFreqAnalysis, simdEnabled;
quality.getProcessingConfig(maxStages, enablePerceptual,
                          enableFreqAnalysis, simdEnabled);

// Apply time constant adjustment
float multiplier = quality.getTimeConstantMultiplier();
float adjustedTimeConstant = baseTimeConstant * multiplier;
```

### SIMD Optimizations

High-performance parallel processing:

```cpp
// Process 4 cascade stages in parallel
float states[4] = {0.1f, 0.2f, 0.3f, 0.4f};
float coeffs[4] = {0.5f, 0.5f, 0.5f, 0.5f};
SIMDOptimizer::processCascadedStages4(input, states, coeffs);

// Multiple parameter smoothing
float inputs[8], outputs[8], states[8], coeffs[8];
SIMDOptimizer::processMultipleParameters(inputs, outputs, states, coeffs, 8);

// Vectorized lookup table operations
float inputs[4] = {1.0f, 2.0f, 3.0f, 4.0f};
float outputs[4];
SIMDOptimizer::lookupTable4(inputs, outputs, tableData, minVal, scale, size);
```

### Memory Optimization

Cache-friendly memory management:

```cpp
// Aligned memory allocation for SIMD
void* alignedBuffer = MemoryOptimizer::alignedAlloc(1024, 16);

// Memory prefetching for performance
MemoryOptimizer::prefetch(nextDataPointer, 3);

// Optimal buffer sizing
size_t optimalSize = MemoryOptimizer::calculateOptimalBufferSize(minSize, sizeof(float));

// Free aligned memory
MemoryOptimizer::alignedFree(alignedBuffer);
```

### Numerical Stability

Safe mathematical operations with overflow protection:

```cpp
// Safe exponential with bounds checking
float safeResult = NumericalStabilizer::safeExp(largeValue);

// Safe logarithm with underflow protection
float safeLog = NumericalStabilizer::safeLog(smallValue);

// Safe division with zero protection
float safeRatio = NumericalStabilizer::safeDivide(num, denom, fallback);

// Denormal number elimination
float cleanValue = NumericalStabilizer::flushDenormals(denormalValue);

// SIMD denormal flushing
float values[4] = {1e-30f, 1e-40f, 1.0f, 2.0f};
NumericalStabilizer::flushDenormals4(values);
```

## Performance Monitoring

### Real-Time Statistics

```cpp
// Get optimization statistics
float cpuUsage;
int qualityLevel;
bool emergencyActive;
float timeMultiplier;
optimizer.getOptimizationStats(cpuUsage, qualityLevel,
                             emergencyActive, timeMultiplier);

// Get detailed CPU timing
CPUMonitor& monitor = optimizer.getCPUMonitor();
float avgTime, maxTime, minTime;
monitor.getTimingStats(avgTime, maxTime, minTime);
```

### Debug Information

```cpp
// Check system status
bool optimizationEnabled = optimizer.isEnabled();
bool emergencyMode = optimizer.isEmergencyMode();

// Get quality level name
const char* qualityName = QualityController::getQualityName(
    quality.getCurrentQuality());

// Check SIMD availability
bool simdAvailable = SIMDOptimizer::isAvailable();
```

## Best Practices

### 1. Initialization Order

Always initialize the optimizer before connecting it to other components:

```cpp
// 1. Initialize optimizer first
mRealTimeOptimizer.initialize(sampleRate, bufferSize);

// 2. Then connect to processors
mCombProcessor.setRealTimeOptimizer(&mRealTimeOptimizer);
mAdaptiveSmoother.setRealTimeOptimizer(&mRealTimeOptimizer);

// 3. Enable optimization features
mCombProcessor.setOptimizationEnabled(true);
mAdaptiveSmoother.setOptimizationEnabled(true);
```

### 2. CPU Monitoring

Place timing calls at the outermost processing level:

```cpp
void processAudioBuffer(ProcessData& data)
{
    // Start timing at buffer level
    mRealTimeOptimizer.getCPUMonitor().startTiming();

    // Process all audio
    processDelaySection(data);
    processCombSection(data);

    // End timing and update
    mRealTimeOptimizer.getCPUMonitor().endTiming();
    mRealTimeOptimizer.updatePerFrame();
}
```

### 3. Memory Alignment

Ensure proper alignment for SIMD operations:

```cpp
// Check alignment before SIMD operations
if (SIMDOptimizer::isAligned(bufferPtr)) {
    // Use SIMD path
    SIMDOptimizer::processMultipleParameters(inputs, outputs, states, coeffs, count);
} else {
    // Fallback to scalar processing
    for (int i = 0; i < count; ++i) {
        outputs[i] = processScalar(inputs[i], states[i], coeffs[i]);
    }
}
```

### 4. Error Handling

Always check for valid optimizer state:

```cpp
float AdaptiveSmoother::processSample(float input)
{
    // Validate input
    if (!NumericalStabilizer::isFiniteAndSafe(input)) {
        input = mPrevInput; // Use last safe value
    }

    // Check for emergency conditions
    if (shouldUseEmergencyFallback()) {
        return input; // Bypass processing
    }

    // Continue with normal processing...
}
```

### 5. Quality Transitions

Handle quality changes gracefully:

```cpp
void updateProcessingQuality()
{
    static QualityController::QualityLevel lastQuality = QualityController::HIGH;
    QualityController::QualityLevel currentQuality =
        mRealTimeOptimizer.getQualityController().getCurrentQuality();

    if (currentQuality != lastQuality) {
        // Quality changed - update processing configuration
        updateProcessingConfig();
        lastQuality = currentQuality;
    }
}
```

## Troubleshooting

### Common Issues

1. **High CPU Usage**: Check if optimization is enabled and quality levels are responding correctly
2. **Audio Artifacts**: Verify numerical stability safeguards are active
3. **SIMD Crashes**: Ensure memory alignment requirements are met
4. **Performance Regression**: Check if lookup tables are properly initialized

### Debug Output

Enable detailed logging for troubleshooting:

```cpp
// Log optimization state
float cpuUsage, timeMultiplier;
int qualityLevel;
bool emergencyActive;
optimizer.getOptimizationStats(cpuUsage, qualityLevel, emergencyActive, timeMultiplier);

printf("CPU: %.1f%%, Quality: %s, Emergency: %s, Time Mult: %.2f\n",
       cpuUsage,
       QualityController::getQualityName(static_cast<QualityController::QualityLevel>(qualityLevel)),
       emergencyActive ? "YES" : "NO",
       timeMultiplier);
```

## Conclusion

The WaterStick real-time optimization system provides comprehensive protection against CPU overload while maintaining the highest possible audio quality. By integrating lookup tables, CPU monitoring, quality control, SIMD optimizations, and numerical stability safeguards, the system ensures reliable real-time performance under all conditions.

The key to effective use is proper integration at initialization time and consistent monitoring throughout the processing chain. The system's automatic quality adaptation means that users experience smooth operation even under heavy CPU load, with graceful degradation that prioritizes audio continuity over feature completeness.