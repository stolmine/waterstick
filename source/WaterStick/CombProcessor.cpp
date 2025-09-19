#include "WaterStickProcessor.h"
#include <cmath>
#include <algorithm>

namespace WaterStick {

CombProcessor::CombProcessor()
    : mBufferSize(0)
    , mWriteIndex(0)
    , mSampleRate(44100.0)
    , mCombSize(0.1f)
    , mNumActiveTaps(64)
    , mFeedback(0.0f)
    , mPitchCV(0.0f)
    , mFeedbackBufferL(0.0f)
    , mFeedbackBufferR(0.0f)
    , mIsSynced(false)
    , mClockDivision(0)
    , mHostTempo(120.0f)
    , mPattern(0)
    , mSlope(0)
{
}

CombProcessor::~CombProcessor()
{
}

void CombProcessor::initialize(double sampleRate, double maxDelaySeconds)
{
    mSampleRate = sampleRate;
    mBufferSize = static_cast<int>(maxDelaySeconds * sampleRate + 1);

    // Allocate delay buffers
    mDelayBufferL.resize(mBufferSize, 0.0f);
    mDelayBufferR.resize(mBufferSize, 0.0f);

    reset();
}

void CombProcessor::reset()
{
    std::fill(mDelayBufferL.begin(), mDelayBufferL.end(), 0.0f);
    std::fill(mDelayBufferR.begin(), mDelayBufferR.end(), 0.0f);
    mWriteIndex = 0;
    mFeedbackBufferL = 0.0f;
    mFeedbackBufferR = 0.0f;
}

void CombProcessor::setSize(float sizeSeconds)
{
    // Clamp to reasonable range (100us to 2s)
    mCombSize = std::max(0.0001f, std::min(2.0f, sizeSeconds));
}

void CombProcessor::setNumTaps(int numTaps)
{
    mNumActiveTaps = std::max(1, std::min(MAX_TAPS, numTaps));
}

void CombProcessor::setFeedback(float feedback)
{
    mFeedback = std::max(0.0f, std::min(0.99f, feedback));
}

void CombProcessor::setSyncMode(bool synced)
{
    mIsSynced = synced;
}

void CombProcessor::setClockDivision(int division)
{
    mClockDivision = std::max(0, std::min(15, division));
}

void CombProcessor::setPitchCV(float cv)
{
    mPitchCV = cv;
}

void CombProcessor::setPattern(int pattern)
{
    mPattern = std::max(0, std::min(kNumCombPatterns - 1, pattern));
}

void CombProcessor::setSlope(int slope)
{
    mSlope = std::max(0, std::min(kNumCombSlopes - 1, slope));
}

float CombProcessor::getTapDelay(int tapIndex) const
{
    // Apply tap spacing pattern
    float tapRatio;

    if (mPattern == 0) {
        // Pattern 1: Uniform spacing (original implementation)
        tapRatio = static_cast<float>(tapIndex + 1) / static_cast<float>(MAX_TAPS);
    } else {
        // Patterns 2-16: Various non-uniform spacing algorithms
        // For now, implement a few example patterns
        switch (mPattern) {
            case 1: // Pattern 2: Logarithmic spacing
                tapRatio = std::log(tapIndex + 1.0f) / std::log(MAX_TAPS);
                break;
            case 2: // Pattern 3: Exponential spacing
                tapRatio = (std::exp(static_cast<float>(tapIndex + 1) / MAX_TAPS) - 1.0f) / (std::exp(1.0f) - 1.0f);
                break;
            case 3: // Pattern 4: Square law spacing
                tapRatio = std::pow(static_cast<float>(tapIndex + 1) / static_cast<float>(MAX_TAPS), 2.0f);
                break;
            case 4: // Pattern 5: Square root spacing
                tapRatio = std::sqrt(static_cast<float>(tapIndex + 1) / static_cast<float>(MAX_TAPS));
                break;
            default:
                // For patterns 6-16, use variations of the above with different curves
                float normalizedIndex = static_cast<float>(tapIndex + 1) / static_cast<float>(MAX_TAPS);
                float patternOffset = static_cast<float>(mPattern - 5) / 10.0f; // 0.0 to 1.1
                tapRatio = std::pow(normalizedIndex, 1.0f + patternOffset);
                break;
        }
    }

    // Ensure tapRatio is in valid range
    tapRatio = std::max(0.0f, std::min(1.0f, tapRatio));

    return applyCVScaling(mCombSize * tapRatio);
}

float CombProcessor::applyCVScaling(float baseDelay) const
{
    // 1V/oct scaling: each volt doubles frequency (halves period)
    // delay_scaled = delay_base * 2^(-cv)
    float scaleFactor = std::pow(2.0f, -mPitchCV);
    return baseDelay * scaleFactor;
}

float CombProcessor::getTapGain(int tapIndex) const
{
    // Calculate slope envelope gain for this tap
    float tapPosition = static_cast<float>(tapIndex) / static_cast<float>(mNumActiveTaps - 1);
    float slopeGain = 1.0f;

    switch (mSlope) {
        case 0: // Flat
            slopeGain = 1.0f;
            break;
        case 1: // Rising
            slopeGain = tapPosition;
            break;
        case 2: // Falling
            slopeGain = 1.0f - tapPosition;
            break;
        case 3: // Rise/Fall
            // Triangle wave: rises to middle, then falls
            if (tapPosition <= 0.5f) {
                slopeGain = tapPosition * 2.0f; // Rise to 1.0 at middle
            } else {
                slopeGain = 2.0f - (tapPosition * 2.0f); // Fall from 1.0 to 0.0
            }
            break;
        default:
            slopeGain = 1.0f;
            break;
    }

    // Ensure gain is in valid range
    return std::max(0.0f, std::min(1.0f, slopeGain));
}

float CombProcessor::tanhLimiter(float input) const
{
    // Soft limiting using tanh
    return std::tanh(input);
}

void CombProcessor::processStereo(float inputL, float inputR, float& outputL, float& outputR)
{
    // Mix input with feedback
    float mixedL = inputL + mFeedbackBufferL * mFeedback;
    float mixedR = inputR + mFeedbackBufferR * mFeedback;

    // Write to delay buffer
    mDelayBufferL[mWriteIndex] = mixedL;
    mDelayBufferR[mWriteIndex] = mixedR;

    // Sum all active taps
    outputL = 0.0f;
    outputR = 0.0f;

    // Tap dropout pattern from Rainmaker (see manual page 19)
    // We'll implement uniform distribution first
    for (int tap = 0; tap < mNumActiveTaps; ++tap)
    {
        // Calculate which physical tap to use based on density
        int physicalTap = tap * MAX_TAPS / mNumActiveTaps;
        if (physicalTap >= MAX_TAPS) physicalTap = MAX_TAPS - 1;

        float tapDelay = getTapDelay(physicalTap);
        float delaySamples = tapDelay * mSampleRate;

        // Simple linear interpolation for fractional delays
        int delayInt = static_cast<int>(delaySamples);
        float delayFrac = delaySamples - delayInt;

        // Calculate read indices with wrap
        int readIdx1 = (mWriteIndex - delayInt + mBufferSize) % mBufferSize;
        int readIdx2 = (readIdx1 - 1 + mBufferSize) % mBufferSize;

        // Read with interpolation
        float tapOutL = mDelayBufferL[readIdx1] * (1.0f - delayFrac) +
                       mDelayBufferL[readIdx2] * delayFrac;
        float tapOutR = mDelayBufferR[readIdx1] * (1.0f - delayFrac) +
                       mDelayBufferR[readIdx2] * delayFrac;

        // Accumulate with appropriate gain compensation for tap density and slope envelope
        // Use sqrt(N) scaling to balance density vs output level
        float densityGain = 1.0f / std::sqrt(static_cast<float>(mNumActiveTaps));
        float slopeGain = getTapGain(tap);
        float totalGain = densityGain * slopeGain;

        outputL += tapOutL * totalGain;
        outputR += tapOutR * totalGain;
    }

    // Read tap 64 for feedback (longest delay)
    float maxDelay = applyCVScaling(mCombSize);
    float maxDelaySamples = maxDelay * mSampleRate;
    int maxDelayInt = static_cast<int>(maxDelaySamples);
    float maxDelayFrac = maxDelaySamples - maxDelayInt;

    int feedbackIdx1 = (mWriteIndex - maxDelayInt + mBufferSize) % mBufferSize;
    int feedbackIdx2 = (feedbackIdx1 - 1 + mBufferSize) % mBufferSize;

    float feedbackL = mDelayBufferL[feedbackIdx1] * (1.0f - maxDelayFrac) +
                     mDelayBufferL[feedbackIdx2] * maxDelayFrac;
    float feedbackR = mDelayBufferR[feedbackIdx1] * (1.0f - maxDelayFrac) +
                     mDelayBufferR[feedbackIdx2] * maxDelayFrac;

    // Apply tanh limiting to feedback
    mFeedbackBufferL = tanhLimiter(feedbackL);
    mFeedbackBufferR = tanhLimiter(feedbackR);

    // Advance write index
    mWriteIndex = (mWriteIndex + 1) % mBufferSize;
}

} // namespace WaterStick