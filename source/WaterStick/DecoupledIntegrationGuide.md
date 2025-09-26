# Decoupled Delay + Pitch Architecture Integration Guide

## Overview

This guide shows how to replace the current `UnifiedPitchDelayLine` system with the new **fully decoupled architecture** that completely separates delay processing from pitch processing.

## Architecture Comparison

### Current System (PROBLEMATIC)
```
16 × UnifiedPitchDelayLine instances
├── Each has own RecoveryManager
├── Each has own LockFreeParameterManager
├── Each competes for processing resources
├── Upward pitch shifts cause buffer underruns
├── Cascading failures when one tap times out
└── "Only one tap plays" chaos under load
```

### New System (SOLUTION)
```
DecoupledDelaySystem
├── 16 × PureDelayLine (always works, no pitch coupling)
├── 1 × PitchCoordinator (manages all 16 pitch processors)
├── Complete resource isolation
├── Graceful degradation (delay works even if pitch fails)
└── Unified coordination prevents resource competition
```

## Integration Steps

### Step 1: Update WaterStickProcessor.h

Replace the current unified delay line declarations:

```cpp
// OLD: Remove these lines
UnifiedPitchDelayLine mUnifiedTapDelayLinesL[NUM_TAPS];
UnifiedPitchDelayLine mUnifiedTapDelayLinesR[NUM_TAPS];
bool mUseUnifiedDelayLines;

// NEW: Add these lines
#include "DecoupledDelayArchitecture.h"
DecoupledDelaySystem mDecoupledDelaySystemL;
DecoupledDelaySystem mDecoupledDelaySystemR;
bool mUseDecoupledDelayLines;  // Feature flag for A/B testing
```

### Step 2: Update Initialization

In `WaterStickProcessor::setupProcessing()`:

```cpp
// OLD: Remove unified delay line initialization
for (int i = 0; i < NUM_TAPS; i++) {
    mUnifiedTapDelayLinesL[i].initialize(mSampleRate, maxDelayTime);
    mUnifiedTapDelayLinesR[i].initialize(mSampleRate, maxDelayTime);
}

// NEW: Add decoupled system initialization
mDecoupledDelaySystemL.initialize(mSampleRate, maxDelayTime);
mDecoupledDelaySystemR.initialize(mSampleRate, maxDelayTime);
mUseDecoupledDelayLines = true;  // Enable by default
```

### Step 3: Update Parameter Setting

Replace all pitch shift and delay time parameter updates:

```cpp
// OLD: Individual tap parameter setting
if (mUseUnifiedDelayLines) {
    mUnifiedTapDelayLinesL[i].setPitchShift(historicParams.pitchShift);
    mUnifiedTapDelayLinesR[i].setPitchShift(historicParams.pitchShift);
    mUnifiedTapDelayLinesL[i].setDelayTime(tapDelayTime);
    mUnifiedTapDelayLinesR[i].setDelayTime(tapDelayTime);
}

// NEW: Decoupled system parameter setting
if (mUseDecoupledDelayLines) {
    mDecoupledDelaySystemL.setTapPitchShift(i, historicParams.pitchShift);
    mDecoupledDelaySystemR.setTapPitchShift(i, historicParams.pitchShift);
    mDecoupledDelaySystemL.setTapDelayTime(i, tapDelayTime);
    mDecoupledDelaySystemR.setTapDelayTime(i, tapDelayTime);
    mDecoupledDelaySystemL.setTapEnabled(i, mTapEnabled[i]);
    mDecoupledDelaySystemR.setTapEnabled(i, mTapEnabled[i]);
}
```

### Step 4: Update Main Processing Loop

Replace the tap processing loop in `processDelaySection()`:

```cpp
// OLD: Individual tap processing with potential failures
for (int tap = 0; tap < NUM_TAPS; tap++) {
    if (!mTapEnabled[tap] && !mTapFadingOut[tap]) continue;

    ParameterSnapshot historicParams = getHistoricParameters(tap, tapDelayTime);
    float tapOutputL, tapOutputR;

    if (mUseUnifiedDelayLines) {
        mUnifiedTapDelayLinesL[tap].setPitchShift(historicParams.pitchShift);
        mUnifiedTapDelayLinesR[tap].setPitchShift(historicParams.pitchShift);
        mUnifiedTapDelayLinesL[tap].processSample(inputL, tapOutputL);
        mUnifiedTapDelayLinesR[tap].processSample(inputR, tapOutputR);
    }

    // ... individual tap processing continues
}

// NEW: Batch processing with guaranteed delay functionality
if (mUseDecoupledDelayLines) {
    // Process all taps in one coordinated batch
    float tapOutputsL[NUM_TAPS];
    float tapOutputsR[NUM_TAPS];

    mDecoupledDelaySystemL.processAllTaps(inputL, tapOutputsL);
    mDecoupledDelaySystemR.processAllTaps(inputR, tapOutputsR);

    // Apply per-tap processing (filters, panning, etc.)
    for (int tap = 0; tap < NUM_TAPS; tap++) {
        if (!mTapEnabled[tap] && !mTapFadingOut[tap]) continue;

        ParameterSnapshot historicParams = getHistoricParameters(tap, mTapDistribution.getTapDelayTime(tap));

        float tapOutputL = tapOutputsL[tap] * historicParams.level;
        float tapOutputR = tapOutputsR[tap] * historicParams.level;

        // Apply filtering
        if (historicParams.filterType != FILTER_BYPASS) {
            mTapFiltersL[tap].setParameters(historicParams.filterCutoff,
                                          historicParams.filterResonance,
                                          historicParams.filterType);
            mTapFiltersR[tap].setParameters(historicParams.filterCutoff,
                                          historicParams.filterResonance,
                                          historicParams.filterType);
            tapOutputL = mTapFiltersL[tap].process(tapOutputL);
            tapOutputR = mTapFiltersR[tap].process(tapOutputR);
        }

        // Apply panning and fade
        float panGainL, panGainR;
        float panValue = historicParams.pan;
        panGainL = std::cos((panValue + 1.0f) * 0.25f * M_PI);
        panGainR = std::sin((panValue + 1.0f) * 0.25f * M_PI);

        tapOutputL *= panGainL * mTapFadeGain[tap];
        tapOutputR *= panGainR * mTapFadeGain[tap];

        // Accumulate outputs
        outputL += tapOutputL;
        outputR += tapOutputR;

        // Accumulate feedback sends
        mFeedbackSubMixerL += tapOutputL * historicParams.feedbackSend;
        mFeedbackSubMixerR += tapOutputR * historicParams.feedbackSend;
    }
} else {
    // Fallback to legacy system for comparison
    // ... existing tap loop code
}
```

### Step 5: Update Reset and Buffer Management

Replace buffer reset calls:

```cpp
// OLD: Individual buffer resets
if (mUseUnifiedDelayLines) {
    mUnifiedTapDelayLinesL[i].reset();
    mUnifiedTapDelayLinesR[i].reset();
}

// NEW: System-wide coordinated reset
if (mUseDecoupledDelayLines) {
    mDecoupledDelaySystemL.reset();
    mDecoupledDelaySystemR.reset();
}
```

### Step 6: Add System Health Monitoring

Add methods for monitoring the decoupled system health:

```cpp
// Add to WaterStickProcessor.h
void logDecoupledSystemHealth() const;
bool isDecoupledSystemHealthy() const;

// Add to WaterStickProcessor.cpp
void WaterStickProcessor::logDecoupledSystemHealth() const {
    if (!mUseDecoupledDelayLines || !PitchDebug::isLoggingEnabled()) return;

    DecoupledDelaySystem::SystemHealth healthL, healthR;
    mDecoupledDelaySystemL.getSystemHealth(healthL);
    mDecoupledDelaySystemR.getSystemHealth(healthR);

    std::ostringstream ss;
    ss << "Decoupled System Health:\n"
       << "  Left Channel - Delay: " << (healthL.delaySystemHealthy ? "OK" : "FAILED")
       << ", Pitch: " << (healthL.pitchSystemHealthy ? "OK" : "FAILED") << "\n"
       << "  Right Channel - Delay: " << (healthR.delaySystemHealthy ? "OK" : "FAILED")
       << ", Pitch: " << (healthR.pitchSystemHealthy ? "OK" : "FAILED") << "\n"
       << "  Active Taps: L=" << healthL.activeTaps << ", R=" << healthR.activeTaps << "\n"
       << "  Failed Pitch Taps: L=" << healthL.failedPitchTaps << ", R=" << healthR.failedPitchTaps << "\n"
       << "  Processing Times: Delay=" << healthL.delayProcessingTime << "μs, Pitch=" << healthL.pitchProcessingTime << "μs";

    PitchDebug::logMessage(ss.str());
}

bool WaterStickProcessor::isDecoupledSystemHealthy() const {
    if (!mUseDecoupledDelayLines) return false;

    DecoupledDelaySystem::SystemHealth healthL, healthR;
    mDecoupledDelaySystemL.getSystemHealth(healthL);
    mDecoupledDelaySystemR.getSystemHealth(healthR);

    return healthL.delaySystemHealthy && healthR.delaySystemHealthy;
}
```

### Step 7: Add Feature Toggle Methods

Add methods to control the decoupled system:

```cpp
// Add to WaterStickProcessor.h
void enableDecoupledDelayLines(bool enable);
bool isUsingDecoupledDelayLines() const;
void enablePitchProcessing(bool enable);

// Add to WaterStickProcessor.cpp
void WaterStickProcessor::enableDecoupledDelayLines(bool enable) {
    if (mUseDecoupledDelayLines != enable) {
        mUseDecoupledDelayLines = enable;

        // Reset all systems when switching
        if (enable) {
            mDecoupledDelaySystemL.reset();
            mDecoupledDelaySystemR.reset();
        } else {
            for (int i = 0; i < NUM_TAPS; i++) {
                mUnifiedTapDelayLinesL[i].reset();
                mUnifiedTapDelayLinesR[i].reset();
            }
        }

        if (PitchDebug::isLoggingEnabled()) {
            PitchDebug::logMessage(enable ?
                "Switched to decoupled delay line system" :
                "Switched to unified delay line system");
        }
    }
}

bool WaterStickProcessor::isUsingDecoupledDelayLines() const {
    return mUseDecoupledDelayLines;
}

void WaterStickProcessor::enablePitchProcessing(bool enable) {
    if (mUseDecoupledDelayLines) {
        mDecoupledDelaySystemL.enablePitchProcessing(enable);
        mDecoupledDelaySystemR.enablePitchProcessing(enable);

        if (PitchDebug::isLoggingEnabled()) {
            PitchDebug::logMessage(enable ?
                "Pitch processing enabled" :
                "Pitch processing disabled - delay-only mode");
        }
    }
}
```

### Step 8: Update CMakeLists.txt

Add the new source files to the build:

```cmake
# Add to the source file list
set(sources
    # ... existing files
    source/WaterStick/DecoupledDelayArchitecture.cpp
    source/WaterStick/DecoupledDelayArchitecture.h
)
```

## Key Benefits of the New Architecture

### 1. Complete System Decoupling
- **Delay processing is completely independent** - never waits for or depends on pitch
- **Pitch processing gets pre-computed delay outputs** - can't block or interfere with delay
- **Each system can fail independently** without affecting the other

### 2. Unified Resource Coordination
- **Single PitchCoordinator manages all 16 taps** - no resource competition
- **Shared processing budget** prevents cascading timeouts
- **Centralized recovery** handles failures gracefully

### 3. Guaranteed Delay Functionality
- **PureDelayLine always works** - no pitch coupling whatsoever
- **Graceful degradation** - delay works even if pitch completely fails
- **Clean fallback to delay-only operation** if pitch system becomes unhealthy

### 4. Performance Isolation
- **Delay processing cost is constant** regardless of pitch settings
- **Pitch processing budget is isolated** and bounded (100μs total)
- **Clear performance attribution** between delay and pitch subsystems

### 5. Operational Reliability
- **No "only one tap plays" failures** - all 16 taps processed in batch
- **No cascading emergency bypasses** - failures are localized
- **Predictable behavior under all load conditions**

## Migration Strategy

### Phase 1: Parallel Implementation
- Keep existing `UnifiedPitchDelayLine` system as fallback
- Add new `DecoupledDelaySystem` alongside
- Use feature flag to switch between systems
- A/B test in development environment

### Phase 2: Validation
- Compare audio output between systems
- Monitor performance metrics
- Validate that delay functionality is always available
- Test pitch processing graceful degradation

### Phase 3: Production Deployment
- Enable decoupled system by default
- Keep unified system for emergency fallback
- Monitor system health in production
- Gradually remove old system once stability confirmed

This architecture fundamentally solves the resource contention and coupling issues by ensuring that the most critical functionality (delay) is completely decoupled from the optional enhancement (pitch shifting).