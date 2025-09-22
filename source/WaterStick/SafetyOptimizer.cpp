#include "SafetyOptimizer.h"

namespace WaterStick {

// SimpleCPUMonitor Implementation

SimpleCPUMonitor::SimpleCPUMonitor()
    : mTargetTimePerBuffer(0.0)
    , mCurrentCPUUsage(0.0f)
    , mInitialized(false)
{
}

void SimpleCPUMonitor::initialize(double sampleRate, int bufferSize)
{
    // Calculate target processing time per buffer
    mTargetTimePerBuffer = static_cast<double>(bufferSize) / sampleRate;
    mInitialized = true;

    // Reset statistics
    reset();
}

void SimpleCPUMonitor::startTiming()
{
    if (!mInitialized) return;
    mStartTime = std::chrono::high_resolution_clock::now();
}

void SimpleCPUMonitor::endTiming()
{
    if (!mInitialized) return;

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - mStartTime);

    // Convert to seconds
    double processingTime = duration.count() * 1e-9;

    // Calculate CPU usage percentage
    float currentUsage = static_cast<float>((processingTime / mTargetTimePerBuffer) * 100.0);

    // Update current usage (atomic)
    mCurrentCPUUsage.store(currentUsage);
}

void SimpleCPUMonitor::reset()
{
    mCurrentCPUUsage.store(0.0f);
}

// EmergencyFallback Implementation

EmergencyFallback::EmergencyFallback()
    : mEmergencyActive(false)
    , mWarningActive(false)
    , mEmergencyCounter(0)
    , mWarningCounter(0)
{
}

void EmergencyFallback::updateEmergencyState(const SimpleCPUMonitor& cpuMonitor)
{
    float cpuUsage = cpuMonitor.getCPUUsage();

    // Check for emergency conditions (>95% CPU)
    if (cpuUsage > 95.0f) {
        mEmergencyCounter++;
        if (mEmergencyCounter >= EMERGENCY_THRESHOLD) {
            mEmergencyActive = true;
        }
    } else {
        mEmergencyCounter = std::max(0, mEmergencyCounter - 1);
        if (mEmergencyCounter == 0) {
            mEmergencyActive = false;
        }
    }

    // Check for warning conditions (>80% CPU)
    if (cpuUsage > 80.0f && !mEmergencyActive) {
        mWarningCounter++;
        if (mWarningCounter >= WARNING_THRESHOLD) {
            mWarningActive = true;
        }
    } else {
        mWarningCounter = std::max(0, mWarningCounter - 1);
        if (mWarningCounter == 0) {
            mWarningActive = false;
        }
    }
}

void EmergencyFallback::reset()
{
    mEmergencyActive = false;
    mWarningActive = false;
    mEmergencyCounter = 0;
    mWarningCounter = 0;
}

// SafetyOptimizer Implementation

SafetyOptimizer::SafetyOptimizer()
    : mEnabled(true)
    , mInitialized(false)
    , mSampleRate(44100.0)
    , mBufferSize(512)
    , mFrameCounter(0)
{
}

void SafetyOptimizer::initialize(double sampleRate, int bufferSize)
{
    mSampleRate = sampleRate;
    mBufferSize = bufferSize;

    // Initialize subsystems
    mCPUMonitor.initialize(sampleRate, bufferSize);

    mFrameCounter = 0;
    mInitialized = true;
}

void SafetyOptimizer::updatePerFrame()
{
    if (!mEnabled || !mInitialized) {
        return;
    }

    mFrameCounter++;

    // Update emergency fallback periodically
    if (mFrameCounter % UPDATE_INTERVAL == 0) {
        mEmergencyFallback.updateEmergencyState(mCPUMonitor);
    }
}

void SafetyOptimizer::reset()
{
    mCPUMonitor.reset();
    mEmergencyFallback.reset();
    mFrameCounter = 0;
}

} // namespace WaterStick