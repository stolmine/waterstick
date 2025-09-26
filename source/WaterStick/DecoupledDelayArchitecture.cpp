#include "DecoupledDelayArchitecture.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iostream>

namespace WaterStick {

// ===================================================================
// 1. PURE DELAY LINE IMPLEMENTATION
// ===================================================================

PureDelayLine::PureDelayLine()
: mBufferSize(0)
, mWriteIndexA(0)
, mWriteIndexB(0)
, mSampleRate(44100.0)
, mInitialized(false)
, mUsingLineA(true)
, mCrossfadeState(STABLE)
, mTargetDelayTime(0.1f)
, mCurrentDelayTime(0.1f)
, mStabilityCounter(0)
, mStabilityThreshold(2048)
, mCrossfadeLength(0)
, mCrossfadePosition(0)
, mCrossfadeGainA(1.0f)
, mCrossfadeGainB(0.0f) {
    mStateA.delayInSamples = 0.5f;
    mStateA.readIndex = 0;
    mStateA.allpassCoeff = 0.0f;
    mStateA.apInput = 0.0f;
    mStateA.lastOutput = 0.0f;
    mStateA.doNextOut = true;
    mStateA.nextOutput = 0.0f;

    mStateB.delayInSamples = 0.5f;
    mStateB.readIndex = 0;
    mStateB.allpassCoeff = 0.0f;
    mStateB.apInput = 0.0f;
    mStateB.lastOutput = 0.0f;
    mStateB.doNextOut = true;
    mStateB.nextOutput = 0.0f;
}

PureDelayLine::~PureDelayLine() = default;

void PureDelayLine::initialize(double sampleRate, double maxDelaySeconds) {
    mSampleRate = sampleRate;

    // Dual-buffer sizing for crossfading
    mBufferSize = static_cast<int>(maxDelaySeconds * sampleRate) + 1024;
    mBufferA.resize(mBufferSize, 0.0f);
    mBufferB.resize(mBufferSize, 0.0f);

    mWriteIndexA = 0;
    mWriteIndexB = 0;

    // Initialize delay states
    updateDelayState(mStateA, mCurrentDelayTime);
    updateDelayState(mStateB, mCurrentDelayTime);

    mStabilityThreshold = static_cast<int>(sampleRate * 0.05);

    mInitialized = true;
}

void PureDelayLine::setDelayTime(float delayTimeSeconds) {
    if (!mInitialized) return;

    // Use crossfading for smooth delay time changes (eliminates zipper noise)
    if (std::abs(delayTimeSeconds - mTargetDelayTime) > 0.001f) {
        mTargetDelayTime = delayTimeSeconds;
        mStabilityCounter = 0;
    }
}

void PureDelayLine::processSample(float input, float& output) {
    if (!mInitialized) {
        output = input;
        return;
    }

    // Check for delay time changes and manage crossfading
    if (std::abs(mTargetDelayTime - mCurrentDelayTime) > 0.001f) {
        mStabilityCounter++;

        if (mStabilityCounter >= mStabilityThreshold && mCrossfadeState == STABLE) {
            startCrossfade();
        }
    } else {
        mStabilityCounter = 0;
    }

    updateCrossfade();

    // Process both delay lines
    float outputA = processDelayLine(mBufferA, mWriteIndexA, mStateA, input);
    float outputB = processDelayLine(mBufferB, mWriteIndexB, mStateB, input);

    // Mix outputs based on crossfade state
    if (mCrossfadeState == STABLE) {
        output = mUsingLineA ? outputA : outputB;
    } else {
        output = (outputA * mCrossfadeGainA) + (outputB * mCrossfadeGainB);
    }
}

void PureDelayLine::reset() {
    if (!mInitialized) return;

    // Reset dual buffers
    std::fill(mBufferA.begin(), mBufferA.end(), 0.0f);
    std::fill(mBufferB.begin(), mBufferB.end(), 0.0f);

    mWriteIndexA = 0;
    mWriteIndexB = 0;

    mUsingLineA = true;
    mCrossfadeState = STABLE;
    mStabilityCounter = 0;
    mCrossfadePosition = 0;
    mCrossfadeGainA = 1.0f;
    mCrossfadeGainB = 0.0f;

    mStateA.delayInSamples = 0.5f;
    mStateA.readIndex = 0;
    mStateA.apInput = 0.0f;
    mStateA.lastOutput = 0.0f;
    mStateA.doNextOut = true;
    mStateA.nextOutput = 0.0f;

    mStateB.delayInSamples = 0.5f;
    mStateB.readIndex = 0;
    mStateB.apInput = 0.0f;
    mStateB.lastOutput = 0.0f;
    mStateB.doNextOut = true;
    mStateB.nextOutput = 0.0f;
}

// Crossfading implementation for zipper-free delay time changes
void PureDelayLine::updateDelayState(DelayLineState& state, float delayTime) {
    float delaySamples = delayTime * static_cast<float>(mSampleRate);
    float maxDelaySamples = static_cast<float>(mBufferSize - 1);

    state.delayInSamples = std::max(0.5f, std::min(delaySamples, maxDelaySamples));
    updateAllpassCoeff(state);
}

void PureDelayLine::updateAllpassCoeff(DelayLineState& state) {
    if (state.delayInSamples <= 0.0f) {
        state.allpassCoeff = 0.0f;
        return;
    }

    float integerDelay = std::floor(state.delayInSamples);
    float fraction = state.delayInSamples - integerDelay;

    if (fraction < 1e-6f) {
        state.allpassCoeff = 0.0f;
    } else {
        // Allpass coefficient for fractional delay
        state.allpassCoeff = (1.0f - fraction) / (1.0f + fraction);
    }
}

float PureDelayLine::processDelayLine(std::vector<float>& buffer, int& writeIndex, DelayLineState& state, float input) {
    // Write input to buffer
    buffer[writeIndex] = input;

    // Calculate read position with fractional delay
    float readPosFloat = static_cast<float>(writeIndex) - state.delayInSamples;
    if (readPosFloat < 0.0f) {
        readPosFloat += static_cast<float>(mBufferSize);
    }

    // Get integer and fractional parts
    int readIndex = static_cast<int>(readPosFloat);
    float fraction = readPosFloat - static_cast<float>(readIndex);

    // Ensure read index is in bounds
    readIndex = readIndex % mBufferSize;
    if (readIndex < 0) readIndex += mBufferSize;

    // Get delayed sample with allpass interpolation
    float delayedSample = buffer[readIndex];
    float output;

    if (fraction > 1e-6f) {
        // Use allpass interpolation for fractional delay
        output = delayedSample + state.allpassCoeff * (input - state.lastOutput);
        state.lastOutput = output;
    } else {
        // No interpolation needed
        output = delayedSample;
        state.lastOutput = output;
    }

    // Advance write index
    writeIndex = (writeIndex + 1) % mBufferSize;

    return output;
}

float PureDelayLine::nextOut(DelayLineState& state, const std::vector<float>& buffer) {
    // Calculate read position
    float readPosFloat = static_cast<float>(mWriteIndexA) - state.delayInSamples;
    if (readPosFloat < 0.0f) {
        readPosFloat += static_cast<float>(mBufferSize);
    }

    int readIndex = static_cast<int>(readPosFloat);
    readIndex = readIndex % mBufferSize;
    if (readIndex < 0) readIndex += mBufferSize;

    return buffer[readIndex];
}

void PureDelayLine::startCrossfade() {
    mCrossfadeState = CROSSFADING;
    mCrossfadeLength = calculateCrossfadeLength(mTargetDelayTime);
    mCrossfadePosition = 0;

    // Update the standby line with new delay time
    if (mUsingLineA) {
        updateDelayState(mStateB, mTargetDelayTime);
    } else {
        updateDelayState(mStateA, mTargetDelayTime);
    }
}

void PureDelayLine::updateCrossfade() {
    if (mCrossfadeState != CROSSFADING) return;

    float progress = static_cast<float>(mCrossfadePosition) / static_cast<float>(mCrossfadeLength);
    progress = std::min(progress, 1.0f);

    // Exponential fade curves for smooth transitions
    float fadeOut = 0.5f * (1.0f + cosf(progress * 3.14159265f));
    float fadeIn = 1.0f - fadeOut;

    if (mUsingLineA) {
        mCrossfadeGainA = fadeOut;
        mCrossfadeGainB = fadeIn;
    } else {
        mCrossfadeGainA = fadeIn;
        mCrossfadeGainB = fadeOut;
    }

    mCrossfadePosition++;

    if (mCrossfadePosition >= mCrossfadeLength) {
        mCrossfadeState = STABLE;
        mUsingLineA = !mUsingLineA;
        mCurrentDelayTime = mTargetDelayTime;

        if (mUsingLineA) {
            mCrossfadeGainA = 1.0f;
            mCrossfadeGainB = 0.0f;
        } else {
            mCrossfadeGainA = 0.0f;
            mCrossfadeGainB = 1.0f;
        }
    }
}

int PureDelayLine::calculateCrossfadeLength(float delayTime) {
    // Adaptive crossfade length based on delay time
    float baseCrossfadeMs = 50.0f + (delayTime * 1000.0f * 0.25f);
    baseCrossfadeMs = std::min(baseCrossfadeMs, 500.0f);

    return static_cast<int>(baseCrossfadeMs * 0.001f * mSampleRate);
}

// ===================================================================
// 2. PITCH COORDINATOR IMPLEMENTATION
// ===================================================================

PitchCoordinator::PitchCoordinator()
: mSampleRate(44100.0) {
    reset();
}

PitchCoordinator::~PitchCoordinator() = default;

void PitchCoordinator::initialize(double sampleRate) {
    mSampleRate = sampleRate;
    mSystemHealthy.store(true, std::memory_order_release);
    mActiveTaps.store(0, std::memory_order_release);
    mFailedTaps.store(0, std::memory_order_release);
    mMaxProcessingTime.store(0.0, std::memory_order_release);

    // Initialize all tap states
    for (int i = 0; i < MAX_TAPS; ++i) {
        auto& state = mTapStates[i];
        state.semitones = 0;
        state.pitchRatio = 1.0f;
        state.enabled = false;
        state.needsReset = false;
        state.pitchWriteIndex = 0;
        state.pitchReadPosition = 0.0f;
        state.targetPitchRatio = 1.0f;
        state.currentPitchRatio = 1.0f;

        // Calculate smoothing coefficient (5ms time constant)
        const float timeConstantSec = 5.0f / 1000.0f;
        state.smoothingCoeff = std::exp(-1.0f / (timeConstantSec * static_cast<float>(mSampleRate)));

        // Clear pitch buffer
        state.pitchBuffer.fill(0.0f);
    }
}

void PitchCoordinator::enableTap(int tapIndex, bool enable) {
    if (tapIndex < 0 || tapIndex >= MAX_TAPS) return;

    auto& state = mTapStates[tapIndex];

    if (state.enabled != enable) {
        state.enabled = enable;
        state.needsReset = true;

        if (enable) {
            mActiveTaps.fetch_add(1, std::memory_order_acq_rel);
        } else {
            mActiveTaps.fetch_sub(1, std::memory_order_acq_rel);
        }
    }
}

void PitchCoordinator::setPitchShift(int tapIndex, int semitones) {
    if (tapIndex < 0 || tapIndex >= MAX_TAPS) return;

    auto& state = mTapStates[tapIndex];

    if (state.semitones != semitones) {
        state.semitones = std::max(-12, std::min(12, semitones));

        // Calculate pitch ratio: 2^(semitones/12)
        if (state.semitones == 0) {
            state.targetPitchRatio = 1.0f;
        } else {
            state.targetPitchRatio = std::pow(2.0f, static_cast<float>(state.semitones) / 12.0f);
        }

        // Clamp to safe bounds
        state.targetPitchRatio = std::max(0.25f, std::min(4.0f, state.targetPitchRatio));
    }
}

void PitchCoordinator::processAllTaps(const float* delayOutputs, float* pitchOutputs) {
    if (!mSystemHealthy.load(std::memory_order_acquire)) {
        // System unhealthy - pass through delay outputs
        for (int i = 0; i < MAX_TAPS; ++i) {
            pitchOutputs[i] = delayOutputs[i];
        }
        return;
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    int processedTaps = 0;
    int failedTaps = 0;

    // Process all enabled taps
    for (int i = 0; i < MAX_TAPS; ++i) {
        auto& state = mTapStates[i];

        if (!state.enabled) {
            pitchOutputs[i] = delayOutputs[i];
            continue;
        }

        try {
            processSingleTap(i, delayOutputs[i], pitchOutputs[i]);
            processedTaps++;

            // Check processing time budget
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto elapsedUs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                currentTime - startTime).count() / 1000.0;

            if (elapsedUs > PROCESSING_TIMEOUT_US) {
                // Budget exceeded - pass through remaining taps
                for (int j = i + 1; j < MAX_TAPS; ++j) {
                    pitchOutputs[j] = delayOutputs[j];
                }
                break;
            }

        } catch (...) {
            // Tap failed - pass through delay output
            pitchOutputs[i] = delayOutputs[i];
            performTapRecovery(i);
            failedTaps++;
        }
    }

    // Update system stats
    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalTimeUs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        endTime - startTime).count() / 1000.0;

    mMaxProcessingTime.store(
        std::max(mMaxProcessingTime.load(std::memory_order_acquire), totalTimeUs),
        std::memory_order_release
    );

    // Update tap counts
    mActiveTaps.store(processedTaps, std::memory_order_release);
    mFailedTaps.store(failedTaps, std::memory_order_release);

    // Check system health
    if (failedTaps > MAX_TAPS / 2) {
        mSystemHealthy.store(false, std::memory_order_release);
    }
}

void PitchCoordinator::processSingleTap(int tapIndex, float delayOutput, float& pitchOutput) {
    auto& state = mTapStates[tapIndex];

    // Handle reset if needed
    if (state.needsReset) {
        resetTapBuffer(tapIndex);
        state.needsReset = false;
    }

    // Update parameters
    updateTapParameters(tapIndex);

    // If no pitch shift, pass through
    if (std::abs(state.currentPitchRatio - 1.0f) < 1e-6f) {
        pitchOutput = delayOutput;
        return;
    }

    // Write to pitch buffer
    state.pitchBuffer[state.pitchWriteIndex] = delayOutput;
    state.pitchWriteIndex = (state.pitchWriteIndex + 1) % PITCH_BUFFER_SIZE;

    // Calculate read position based on pitch ratio
    state.pitchReadPosition += state.currentPitchRatio;

    // Wrap read position
    if (state.pitchReadPosition >= PITCH_BUFFER_SIZE) {
        state.pitchReadPosition -= PITCH_BUFFER_SIZE;
    }

    // Validate read position
    if (!validateTapState(tapIndex)) {
        performTapRecovery(tapIndex);
        pitchOutput = delayOutput;
        return;
    }

    // Interpolate output
    pitchOutput = interpolatePitchBuffer(tapIndex, state.pitchReadPosition);
}

void PitchCoordinator::updateTapParameters(int tapIndex) {
    auto& state = mTapStates[tapIndex];

    // Smooth pitch ratio changes
    state.currentPitchRatio = state.smoothingCoeff * state.currentPitchRatio +
                             (1.0f - state.smoothingCoeff) * state.targetPitchRatio;

    // Clamp to safe bounds
    state.currentPitchRatio = std::max(0.25f, std::min(4.0f, state.currentPitchRatio));
}

void PitchCoordinator::performTapRecovery(int tapIndex) {
    auto& state = mTapStates[tapIndex];

    // Reset pitch buffer
    resetTapBuffer(tapIndex);

    // Reset processing state
    state.currentPitchRatio = state.targetPitchRatio;
    state.pitchReadPosition = static_cast<float>(PITCH_BUFFER_SIZE / 2);
}

bool PitchCoordinator::validateTapState(int tapIndex) const {
    const auto& state = mTapStates[tapIndex];

    return std::isfinite(state.pitchReadPosition) &&
           std::isfinite(state.currentPitchRatio) &&
           state.pitchReadPosition >= 0.0f &&
           state.pitchReadPosition < PITCH_BUFFER_SIZE &&
           state.currentPitchRatio > 0.0f;
}

float PitchCoordinator::interpolatePitchBuffer(int tapIndex, float position) const {
    const auto& state = mTapStates[tapIndex];

    // Get integer and fractional parts
    int intPos = static_cast<int>(position);
    float frac = position - static_cast<float>(intPos);

    // Ensure bounds
    intPos = intPos % PITCH_BUFFER_SIZE;
    if (intPos < 0) intPos += PITCH_BUFFER_SIZE;

    int nextPos = (intPos + 1) % PITCH_BUFFER_SIZE;

    // Linear interpolation
    return state.pitchBuffer[intPos] * (1.0f - frac) + state.pitchBuffer[nextPos] * frac;
}

void PitchCoordinator::resetTapBuffer(int tapIndex) {
    auto& state = mTapStates[tapIndex];

    state.pitchBuffer.fill(0.0f);
    state.pitchWriteIndex = 0;
    state.pitchReadPosition = static_cast<float>(PITCH_BUFFER_SIZE / 2);
}

void PitchCoordinator::getSystemStats(int& activeTaps, int& failedTaps, double& maxProcessingTime) const {
    activeTaps = mActiveTaps.load(std::memory_order_acquire);
    failedTaps = mFailedTaps.load(std::memory_order_acquire);
    maxProcessingTime = mMaxProcessingTime.load(std::memory_order_acquire);
}

void PitchCoordinator::reset() {
    mSystemHealthy.store(true, std::memory_order_release);
    mActiveTaps.store(0, std::memory_order_release);
    mFailedTaps.store(0, std::memory_order_release);
    mMaxProcessingTime.store(0.0, std::memory_order_release);

    for (int i = 0; i < MAX_TAPS; ++i) {
        resetTapBuffer(i);
        auto& state = mTapStates[i];
        state.enabled = false;
        state.semitones = 0;
        state.targetPitchRatio = 1.0f;
        state.currentPitchRatio = 1.0f;
        state.needsReset = false;
    }
}

// ===================================================================
// 3. DECOUPLED TAP PROCESSOR IMPLEMENTATION
// ===================================================================

DecoupledTapProcessor::DecoupledTapProcessor()
: mTapIndex(-1)
, mEnabled(false)
, mDelayHealthy(false)
, mPitchHealthy(true)
, mPitchEnabled(false)
, mPitchSemitones(0)
, mLastDelayOutput(0.0f) {
    mDelayLine = std::make_unique<PureDelayLine>();
}

DecoupledTapProcessor::~DecoupledTapProcessor() = default;

void DecoupledTapProcessor::initialize(double sampleRate, double maxDelaySeconds, int tapIndex) {
    mTapIndex = tapIndex;
    mDelayLine->initialize(sampleRate, maxDelaySeconds);
    mDelayHealthy = mDelayLine->isInitialized();
    mPitchHealthy = true;
    mLastDelayOutput = 0.0f;
}

void DecoupledTapProcessor::setDelayTime(float delayTimeSeconds) {
    if (mDelayHealthy) {
        mDelayLine->setDelayTime(delayTimeSeconds);
    }
}

void DecoupledTapProcessor::setEnabled(bool enabled) {
    mEnabled = enabled;
    if (!enabled) {
        mLastDelayOutput = 0.0f;
    }
}

void DecoupledTapProcessor::setPitchShift(int semitones) {
    mPitchSemitones = semitones;
    mPitchEnabled = (semitones != 0);
}

void DecoupledTapProcessor::processSample(float input, float& output) {
    if (!mEnabled || !mDelayHealthy) {
        output = 0.0f;
        mLastDelayOutput = 0.0f;
        return;
    }

    // Process delay (always works)
    mDelayLine->processSample(input, mLastDelayOutput);

    // Output is delay result - pitch processing happens at coordinator level
    output = mLastDelayOutput;
}

void DecoupledTapProcessor::reset() {
    if (mDelayHealthy) {
        mDelayLine->reset();
    }
    mLastDelayOutput = 0.0f;
}

// ===================================================================
// 4. UNIFIED DECOUPLED SYSTEM IMPLEMENTATION
// ===================================================================

DecoupledDelaySystem::DecoupledDelaySystem()
: mSampleRate(44100.0)
, mPitchProcessingEnabled(true) {
    mPitchCoordinator = std::make_unique<PitchCoordinator>();
    mDelayOutputs.fill(0.0f);
    mPitchOutputs.fill(0.0f);
}

DecoupledDelaySystem::~DecoupledDelaySystem() = default;

void DecoupledDelaySystem::initialize(double sampleRate, double maxDelaySeconds) {
    mSampleRate = sampleRate;

    // Initialize delay processors
    for (int i = 0; i < NUM_TAPS; ++i) {
        mTapProcessors[i].initialize(sampleRate, maxDelaySeconds, i);
    }

    // Initialize pitch coordinator
    mPitchCoordinator->initialize(sampleRate);

    mDelayProcessingTime.store(0.0, std::memory_order_release);
    mPitchProcessingTime.store(0.0, std::memory_order_release);
}

void DecoupledDelaySystem::setTapDelayTime(int tapIndex, float delayTimeSeconds) {
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        mTapProcessors[tapIndex].setDelayTime(delayTimeSeconds);
    }
}

void DecoupledDelaySystem::setTapEnabled(int tapIndex, bool enabled) {
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        mTapProcessors[tapIndex].setEnabled(enabled);
        mPitchCoordinator->enableTap(tapIndex, enabled);
    }
}

void DecoupledDelaySystem::setTapPitchShift(int tapIndex, int semitones) {
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        mTapProcessors[tapIndex].setPitchShift(semitones);
        mPitchCoordinator->setPitchShift(tapIndex, semitones);
    }
}

void DecoupledDelaySystem::processAllTaps(float input, float* outputs) {
    mProcessingStartTime = std::chrono::high_resolution_clock::now();

    // Stage 1: Process delays (always works, never fails)
    processDelayStage(input);

    auto delayEndTime = std::chrono::high_resolution_clock::now();
    auto delayTimeUs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        delayEndTime - mProcessingStartTime).count() / 1000.0;
    mDelayProcessingTime.store(delayTimeUs, std::memory_order_release);

    // Stage 2: Process pitch (optional, can fail gracefully)
    if (mPitchProcessingEnabled) {
        processPitchStage();
    } else {
        // Pitch disabled - copy delay outputs
        mPitchOutputs = mDelayOutputs;
    }

    auto pitchEndTime = std::chrono::high_resolution_clock::now();
    auto pitchTimeUs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        pitchEndTime - delayEndTime).count() / 1000.0;
    mPitchProcessingTime.store(pitchTimeUs, std::memory_order_release);

    // Stage 3: Combine outputs
    combineOutputs(outputs);

    updatePerformanceMetrics();
}

void DecoupledDelaySystem::processDelayStage(float input) {
    // Process all delay taps - this always works
    for (int i = 0; i < NUM_TAPS; ++i) {
        mTapProcessors[i].processSample(input, mDelayOutputs[i]);
    }
}

void DecoupledDelaySystem::processPitchStage() {
    if (mPitchCoordinator->isHealthy()) {
        // Coordinated pitch processing
        mPitchCoordinator->processAllTaps(mDelayOutputs.data(), mPitchOutputs.data());
    } else {
        // Pitch coordinator unhealthy - pass through delay outputs
        mPitchOutputs = mDelayOutputs;
    }
}

void DecoupledDelaySystem::combineOutputs(float* outputs) {
    // Simply copy pitch outputs (which are delay outputs if pitch failed/disabled)
    for (int i = 0; i < NUM_TAPS; ++i) {
        outputs[i] = mPitchOutputs[i];
    }
}

void DecoupledDelaySystem::enablePitchProcessing(bool enable) {
    mPitchProcessingEnabled = enable;

    if (!enable) {
        // Reset pitch coordinator when disabling
        mPitchCoordinator->reset();
    }
}

void DecoupledDelaySystem::reset() {
    // Reset all tap processors
    for (auto& processor : mTapProcessors) {
        processor.reset();
    }

    // Reset pitch coordinator
    mPitchCoordinator->reset();

    // Clear buffers
    mDelayOutputs.fill(0.0f);
    mPitchOutputs.fill(0.0f);

    // Reset performance metrics
    mDelayProcessingTime.store(0.0, std::memory_order_release);
    mPitchProcessingTime.store(0.0, std::memory_order_release);
}

void DecoupledDelaySystem::getSystemHealth(SystemHealth& health) const {
    // Check delay system health
    health.delaySystemHealthy = true;
    health.activeTaps = 0;

    for (int i = 0; i < NUM_TAPS; ++i) {
        if (mTapProcessors[i].isDelayHealthy()) {
            if (mTapProcessors[i].mEnabled) {  // Access via friend
                health.activeTaps++;
            }
        } else {
            health.delaySystemHealthy = false;
        }
    }

    // Check pitch system health
    health.pitchSystemHealthy = mPitchCoordinator->isHealthy();

    int activePitchTaps, failedPitchTaps;
    double maxPitchTime;
    mPitchCoordinator->getSystemStats(activePitchTaps, failedPitchTaps, maxPitchTime);

    health.failedPitchTaps = failedPitchTaps;

    // Get performance metrics
    health.delayProcessingTime = mDelayProcessingTime.load(std::memory_order_acquire);
    health.pitchProcessingTime = mPitchProcessingTime.load(std::memory_order_acquire);
    health.totalProcessingTime = health.delayProcessingTime + health.pitchProcessingTime;
}

void DecoupledDelaySystem::updatePerformanceMetrics() const {
    // Performance metrics are updated in processAllTaps()
    // This method is here for future extensions
}

} // namespace WaterStick