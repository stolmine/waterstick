#include "CascadedSmoother.h"
#include <cmath>
#include <algorithm>

namespace WaterStick {

// Static constexpr definitions
constexpr int CascadedSmoother::MAX_STAGES;
constexpr int CascadedSmoother::MIN_STAGES;
constexpr int CascadedSmoother::DEFAULT_STAGES;
constexpr int MultiParameterCascadedSmoother::MAX_PARAMETERS;

// CascadedSmoother Implementation

CascadedSmoother::CascadedSmoother(double sampleRate,
                                   float timeConstant,
                                   int stageCount)
    : mSampleRate(sampleRate)
    , mTotalTimeConstant(timeConstant)
    , mStageTimeConstant(timeConstant)
    , mStageCount(stageCount)
    , mSmoothingCoeff(0.0f)
    , mEnabled(true)
    , mStageStates({0.0f})
    , mPreviousInput(0.0f)
    , mPreviousOutput(0.0f)
    , mSamplePeriod(1.0f / static_cast<float>(sampleRate))
{
    // Clamp stage count to valid range
    mStageCount = clampInt(stageCount, MIN_STAGES, MAX_STAGES);

    // Clamp time constant to safe range
    mTotalTimeConstant = clamp(timeConstant, 0.0001f, 1.0f);

    // Calculate coefficients
    updateCoefficients();
}

void CascadedSmoother::setSampleRate(double sampleRate)
{
    mSampleRate = sampleRate;
    mSamplePeriod = 1.0f / static_cast<float>(sampleRate);

    // Recalculate coefficients for new sample rate
    updateCoefficients();
}

void CascadedSmoother::setTimeConstant(float timeConstant)
{
    // Clamp to safe range
    mTotalTimeConstant = clamp(timeConstant, 0.0001f, 1.0f);

    // Recalculate coefficients
    updateCoefficients();
}

void CascadedSmoother::setStageCount(int stageCount)
{
    // Clamp to valid range
    int newStageCount = clampInt(stageCount, MIN_STAGES, MAX_STAGES);

    if (newStageCount != mStageCount) {
        // Store current output for continuity
        float currentOutput = mStageStates[mStageCount - 1];

        // Update stage count
        mStageCount = newStageCount;

        // Reset all stages to maintain continuity
        reset(currentOutput);

        // Recalculate coefficients
        updateCoefficients();
    }
}

float CascadedSmoother::processSample(float input)
{
    if (!mEnabled) {
        return input; // Bypass mode
    }

    // Store input for settling detection
    mPreviousInput = input;

    // Process through cascade stages
    float stageInput = input;

    for (int stage = 0; stage < mStageCount; ++stage) {
        // Apply exponential smoothing: y[n] = α*x[n] + (1-α)*y[n-1]
        // where α = mSmoothingCoeff = 1 - exp(-T/τ_stage)
        mStageStates[stage] = mSmoothingCoeff * stageInput +
                             (1.0f - mSmoothingCoeff) * mStageStates[stage];

        // Output of current stage becomes input to next stage
        stageInput = mStageStates[stage];
    }

    // Final output is from last stage
    float output = mStageStates[mStageCount - 1];

    // Store output for settling detection
    mPreviousOutput = output;

    return output;
}

void CascadedSmoother::reset()
{
    // Reset all stages to zero
    mStageStates.fill(0.0f);
    mPreviousInput = 0.0f;
    mPreviousOutput = 0.0f;
}

void CascadedSmoother::reset(float value)
{
    // Reset all stages to specified value (prevents initial transients)
    for (int stage = 0; stage < MAX_STAGES; ++stage) {
        mStageStates[stage] = value;
    }
    mPreviousInput = value;
    mPreviousOutput = value;
}

bool CascadedSmoother::isSettled(float threshold) const
{
    // Check if input and output are close (indicates settling)
    float difference = std::abs(mPreviousOutput - mPreviousInput);
    return difference < threshold;
}

float CascadedSmoother::getStageOutput(int stage) const
{
    // Validate stage index
    if (stage >= 0 && stage < mStageCount) {
        return mStageStates[stage];
    }

    // Return final output if invalid stage requested
    return mStageStates[mStageCount - 1];
}

void CascadedSmoother::updateCoefficients()
{
    // Calculate per-stage time constant for equivalent response
    mStageTimeConstant = calculateStageTimeConstant();

    // Calculate smoothing coefficient: α = 1 - exp(-T/τ_stage)
    float exponent = -mSamplePeriod / mStageTimeConstant;

    // Clamp exponent to prevent numerical issues
    exponent = clamp(exponent, -20.0f, 0.0f); // exp(-20) ≈ 2e-9, effectively zero

    mSmoothingCoeff = 1.0f - std::exp(exponent);

    // Ensure coefficient is in valid range [0, 1]
    mSmoothingCoeff = clamp(mSmoothingCoeff, 0.0f, 1.0f);
}

float CascadedSmoother::calculateStageTimeConstant() const
{
    // For equivalent response time: τ_stage = τ_total / N
    // This ensures the overall response time matches the specified total time constant
    float stageTimeConstant = mTotalTimeConstant / static_cast<float>(mStageCount);

    // Ensure minimum time constant to prevent numerical issues
    return std::max(stageTimeConstant, mSamplePeriod * 0.1f);
}

// MultiParameterCascadedSmoother Implementation

MultiParameterCascadedSmoother::MultiParameterCascadedSmoother(int parameterCount,
                                                               double sampleRate,
                                                               float timeConstant,
                                                               int stageCount)
    : mParameterCount(0)
    , mInitialized(false)
{
    initialize(parameterCount, sampleRate);
    setTimeConstant(timeConstant);
    setStageCount(stageCount);
}

void MultiParameterCascadedSmoother::initialize(int parameterCount, double sampleRate)
{
    // Clamp parameter count to valid range
    mParameterCount = std::max(1, std::min(parameterCount, MAX_PARAMETERS));

    // Initialize all smoothers with the sample rate
    for (int i = 0; i < mParameterCount; ++i) {
        mSmoothers[i].setSampleRate(sampleRate);
    }

    mInitialized = true;
}

void MultiParameterCascadedSmoother::setSampleRate(double sampleRate)
{
    for (int i = 0; i < mParameterCount; ++i) {
        mSmoothers[i].setSampleRate(sampleRate);
    }
}

void MultiParameterCascadedSmoother::setTimeConstant(float timeConstant)
{
    for (int i = 0; i < mParameterCount; ++i) {
        mSmoothers[i].setTimeConstant(timeConstant);
    }
}

void MultiParameterCascadedSmoother::setStageCount(int stageCount)
{
    for (int i = 0; i < mParameterCount; ++i) {
        mSmoothers[i].setStageCount(stageCount);
    }
}

float MultiParameterCascadedSmoother::processSample(int parameterIndex, float input)
{
    if (!mInitialized || !isValidParameterIndex(parameterIndex)) {
        return input; // Pass through if not initialized or invalid index
    }

    return mSmoothers[parameterIndex].processSample(input);
}

void MultiParameterCascadedSmoother::processAllSamples(const float* inputs, float* outputs)
{
    if (!mInitialized || !inputs || !outputs) {
        return; // Safety check
    }

    for (int i = 0; i < mParameterCount; ++i) {
        outputs[i] = mSmoothers[i].processSample(inputs[i]);
    }
}

void MultiParameterCascadedSmoother::resetAll()
{
    for (int i = 0; i < mParameterCount; ++i) {
        mSmoothers[i].reset();
    }
}

void MultiParameterCascadedSmoother::resetParameter(int parameterIndex)
{
    if (isValidParameterIndex(parameterIndex)) {
        mSmoothers[parameterIndex].reset();
    }
}

void MultiParameterCascadedSmoother::resetAll(const float* values)
{
    if (!values) {
        return; // Safety check
    }

    for (int i = 0; i < mParameterCount; ++i) {
        mSmoothers[i].reset(values[i]);
    }
}

void MultiParameterCascadedSmoother::setEnabled(bool enabled)
{
    for (int i = 0; i < mParameterCount; ++i) {
        mSmoothers[i].setEnabled(enabled);
    }
}

CascadedSmoother& MultiParameterCascadedSmoother::getSmoother(int parameterIndex)
{
    if (isValidParameterIndex(parameterIndex)) {
        return mSmoothers[parameterIndex];
    }

    // Return first smoother if invalid index (safer than throwing)
    return mSmoothers[0];
}

const CascadedSmoother& MultiParameterCascadedSmoother::getSmoother(int parameterIndex) const
{
    if (isValidParameterIndex(parameterIndex)) {
        return mSmoothers[parameterIndex];
    }

    // Return first smoother if invalid index (safer than throwing)
    return mSmoothers[0];
}

} // namespace WaterStick