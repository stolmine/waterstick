# EnhancedAdaptiveSmoother Architecture

## Overview

The **EnhancedAdaptiveSmoother** is a sophisticated hybrid smoothing system that unifies and extends the capabilities of the existing WaterStick smoothing components. It represents the next evolution of parameter smoothing by intelligently combining multiple advanced techniques with automatic selection and graceful fallback mechanisms.

## Architecture Components

### Core Technologies Unified

1. **CascadedSmoother**: Multi-stage filtering for superior frequency response and Gaussian-like characteristics
2. **PerceptualVelocityDetector**: Psychoacoustic parameter analysis using Bark scale and A-weighting
3. **SimpleAdaptiveSmoother**: Lightweight adaptive smoothing with velocity-based time constant adjustment
4. **Intelligent Decision System**: Automatic technique selection based on signal characteristics

### Key Features

- **Hybrid Architecture**: Seamlessly combines cascaded filtering with perceptual velocity detection
- **Intelligent Mode Selection**: Automatically chooses optimal smoothing approach based on signal analysis
- **Parameter Type Awareness**: Specialized optimization for different parameter types (DelayTime, CombSize, PitchCV, etc.)
- **Performance Profiles**: Configurable optimization levels from PowerSaver to HighQuality
- **Graceful Fallback**: Automatic degradation to simpler modes under performance constraints
- **Backward Compatibility**: Maintains compatibility with existing AdaptiveSmoother interface
- **Multi-Parameter Support**: Coordinated smoothing for multiple related parameters

## Usage Examples

### Basic Usage

```cpp
#include "EnhancedAdaptiveSmoother.h"

// Create smoother for delay time parameter
EnhancedAdaptiveSmoother delaySmoother(
    44100.0,  // Sample rate
    EnhancedAdaptiveSmoother::ParameterType::DelayTime,
    EnhancedAdaptiveSmoother::SmoothingMode::Auto,
    EnhancedAdaptiveSmoother::PerformanceProfile::HighQuality
);

// Process samples
float smoothedValue = delaySmoother.processSample(inputValue);
```

### Advanced Configuration

```cpp
// Configure adaptive parameters
delaySmoother.setAdaptiveParameters(
    0.0003f,  // Fast time constant
    0.005f,   // Slow time constant
    2.0f,     // Velocity sensitivity
    0.1f      // Hysteresis threshold
);

// Configure cascaded filtering
delaySmoother.setCascadedParameters(
    true,     // Enable cascaded filtering
    4,        // Maximum stages
    0.2f,     // Stage hysteresis
    true      // Adaptive stage count
);

// Configure perceptual analysis
delaySmoother.setPerceptualParameters(
    true,     // Enable perceptual analysis
    20.0,     // Min frequency (Hz)
    20000.0,  // Max frequency (Hz)
    1.8f,     // Perceptual sensitivity
    false     // Use full analysis (not simplified)
);
```

### Multi-Parameter Coordination

```cpp
// Create multi-parameter smoother
EnhancedMultiParameterSmoother multiSmoother(4, 44100.0);

// Configure different parameter types
multiSmoother.setParameterType(0, EnhancedAdaptiveSmoother::ParameterType::DelayTime);
multiSmoother.setParameterType(1, EnhancedAdaptiveSmoother::ParameterType::CombSize);
multiSmoother.setParameterType(2, EnhancedAdaptiveSmoother::ParameterType::PitchCV);
multiSmoother.setParameterType(3, EnhancedAdaptiveSmoother::ParameterType::FilterCutoff);

// Process multiple parameters
float inputs[4] = {0.1f, 0.2f, 0.3f, 0.4f};
float outputs[4];
multiSmoother.processAllSamples(inputs, outputs);
```

## Smoothing Modes

### Auto Mode (Recommended)
Automatically selects the optimal technique based on:
- Signal velocity and complexity
- Parameter type characteristics
- Performance constraints
- Decision confidence

### Enhanced Mode
Uses the full hybrid approach combining:
- Cascaded filtering with adaptive stage count
- Perceptual velocity weighting
- Intelligent blending based on signal characteristics

### Perceptual Mode
Focuses on psychoacoustic optimization:
- Bark scale frequency analysis
- A-weighting for perceptual importance
- Optimized for audible parameter changes

### Cascaded Mode
Uses pure cascaded filtering:
- Multiple filtering stages for smooth response
- Adaptive stage count based on velocity
- Superior artifact rejection

### Traditional Mode
Fallback to basic adaptive smoothing:
- Velocity-based time constant adjustment
- Compatible with existing systems
- Minimal computational overhead

### Bypass Mode
Direct pass-through for debugging or performance testing

## Parameter Type Specializations

### DelayTime Parameters
- **Sensitivity**: High (2.0x)
- **Time Constants**: 0.3ms fast, 5ms slow
- **Optimization**: Enhanced mode with perceptual weighting
- **Use Case**: Delay line modulation, echo parameters

### CombSize Parameters
- **Sensitivity**: Very High (2.5x)
- **Time Constants**: 0.2ms fast, 3ms slow
- **Optimization**: Enhanced mode with tight thresholds
- **Use Case**: Comb filter size, pitch-critical parameters

### PitchCV Parameters
- **Sensitivity**: High (1.8x)
- **Time Constants**: 0.5ms fast, 8ms slow
- **Optimization**: Perceptual mode preferred
- **Use Case**: Pitch control voltage, musical tuning

### FilterCutoff Parameters
- **Sensitivity**: Medium (1.2x)
- **Time Constants**: 0.8ms fast, 15ms slow
- **Optimization**: Cascaded mode for smooth sweeps
- **Use Case**: Filter frequency modulation

### Amplitude Parameters
- **Sensitivity**: Low (1.0x)
- **Time Constants**: 1ms fast, 10ms slow
- **Optimization**: Traditional mode for efficiency
- **Use Case**: Volume, gain, envelope parameters

## Performance Profiles

### HighQuality
- Full perceptual analysis
- Maximum cascade stages (5)
- Tightest mode switching thresholds
- **CPU Usage**: High, **Quality**: Maximum

### Balanced (Default)
- Standard perceptual analysis
- Moderate cascade stages (3)
- Balanced mode switching
- **CPU Usage**: Medium, **Quality**: High

### LowLatency
- Simplified perceptual analysis
- Reduced cascade stages (2)
- Faster mode switching
- **CPU Usage**: Low, **Quality**: Good

### PowerSaver
- Minimal perceptual analysis
- Single cascade stage
- Conservative mode switching
- **CPU Usage**: Minimal, **Quality**: Adequate

## Fallback System

The EnhancedAdaptiveSmoother includes automatic fallback mechanisms:

1. **Performance Monitoring**: Continuously tracks CPU usage
2. **Automatic Degradation**: Reduces complexity when thresholds exceeded
3. **Graceful Recovery**: Attempts to restore higher quality when possible
4. **Emergency Mode**: Falls back to bypass if all else fails

### Fallback Levels
0. **Enhanced**: Full hybrid processing
1. **Perceptual**: Perceptual analysis only
2. **Cascaded**: Cascaded filtering only
3. **Traditional**: Basic adaptive smoothing

## Integration Guidelines

### WaterStick Integration
```cpp
// Replace existing AdaptiveSmoother usage
// Old:
// AdaptiveSmoother smoother(sampleRate);

// New:
EnhancedAdaptiveSmoother smoother(
    sampleRate,
    EnhancedAdaptiveSmoother::ParameterType::DelayTime,
    EnhancedAdaptiveSmoother::SmoothingMode::Auto
);

// Backward compatibility maintained
auto& legacySmoother = smoother.getLegacySmoother();
```

### Performance Considerations
- Use appropriate performance profiles for real-time constraints
- Monitor fallback status for performance debugging
- Consider parameter type specializations for optimal results
- Enable automatic fallback for robust operation

## Status and Debugging

### Performance Metrics
```cpp
float cpuUsage, latency, quality;
smoother.getPerformanceMetrics(cpuUsage, latency, quality);
```

### Detailed Status
```cpp
float velocity, timeConstant, perceptualVelocity, confidence;
int stageCount;
smoother.getDetailedStatus(velocity, timeConstant, stageCount,
                          perceptualVelocity, confidence);
```

### Engine Utilization
```cpp
float enhanced, cascaded, perceptual, traditional;
smoother.getEngineUtilization(enhanced, cascaded, perceptual, traditional);
```

## Benefits Over Individual Systems

1. **Intelligent Selection**: Automatically chooses the best technique for each situation
2. **Unified API**: Single interface for all smoothing capabilities
3. **Optimized Performance**: Parameter-specific optimizations and fallback mechanisms
4. **Robust Operation**: Graceful degradation prevents audio dropouts
5. **Future-Proof**: Extensible architecture for new smoothing techniques
6. **Backward Compatible**: Drop-in replacement for existing AdaptiveSmoother

## Implementation Details

- **Language**: C++17
- **Dependencies**: CascadedSmoother, PerceptualVelocityDetector
- **Memory**: Static allocation, no dynamic memory in audio thread
- **Threading**: Real-time safe, no locks or blocking operations
- **Precision**: 32-bit float processing with 64-bit internal calculations where needed

## Files

- `EnhancedAdaptiveSmoother.h` - Main header with class definitions
- `EnhancedAdaptiveSmoother.cpp` - Implementation
- `ENHANCED_SMOOTHER_README.md` - This documentation

## Conclusion

The EnhancedAdaptiveSmoother represents a significant advancement in parameter smoothing technology for the WaterStick project. By intelligently combining multiple proven techniques with automatic decision-making and robust fallback mechanisms, it provides superior audio quality while maintaining compatibility and performance requirements.

The system is designed to be a drop-in replacement for existing smoothing systems while providing access to advanced capabilities when needed. Its parameter-aware optimizations and intelligent mode selection ensure optimal results across the wide range of parameters used in the WaterStick VST3 plugin.