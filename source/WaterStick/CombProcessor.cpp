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
    , mHostTempoValid(false)
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

void CombProcessor::updateTempo(double hostTempo, bool isValid)
{
    mHostTempo = static_cast<float>(hostTempo);
    mHostTempoValid = isValid;
}

float CombProcessor::getSyncedCombSize() const
{
    if (mIsSynced && mHostTempoValid) {
        double quarterNoteTime = 60.0 / mHostTempo;

        float divisionValue;
        switch (mClockDivision) {
            case 0: divisionValue = 0.0625f; break;   // 1/64
            case 1: divisionValue = 0.08333f; break;  // 1/32T
            case 2: divisionValue = 0.09375f; break;  // 1/64.
            case 3: divisionValue = 0.125f; break;    // 1/32
            case 4: divisionValue = 0.16667f; break;  // 1/16T
            case 5: divisionValue = 0.1875f; break;   // 1/32.
            case 6: divisionValue = 0.25f; break;     // 1/16
            case 7: divisionValue = 0.33333f; break;  // 1/8T
            case 8: divisionValue = 0.375f; break;    // 1/16.
            case 9: divisionValue = 0.5f; break;      // 1/8
            case 10: divisionValue = 0.66667f; break; // 1/4T
            case 11: divisionValue = 0.75f; break;    // 1/8.
            case 12: divisionValue = 1.0f; break;     // 1/4
            case 13: divisionValue = 1.33333f; break; // 1/2T
            case 14: divisionValue = 1.5f; break;     // 1/4.
            case 15: divisionValue = 2.0f; break;     // 1/2
            case 16: divisionValue = 2.66667f; break; // 1T
            case 17: divisionValue = 3.0f; break;     // 1/2.
            case 18: divisionValue = 4.0f; break;     // 1
            case 19: divisionValue = 8.0f; break;     // 2
            case 20: divisionValue = 16.0f; break;    // 4
            case 21: divisionValue = 32.0f; break;    // 8
            default: divisionValue = 1.0f; break;
        }

        float syncTime = static_cast<float>(quarterNoteTime * divisionValue);
        return std::max(0.0001f, std::min(2.0f, syncTime));
    } else {
        return mCombSize;
    }
}

float CombProcessor::getTapDelay(int tapIndex) const
{
    float tapRatio;

    if (mPattern == 0) {
        tapRatio = static_cast<float>(tapIndex + 1) / static_cast<float>(MAX_TAPS);
    } else {
        switch (mPattern) {
            case 1:
                tapRatio = std::log(tapIndex + 1.0f) / std::log(MAX_TAPS);
                break;
            case 2:
                tapRatio = (std::exp(static_cast<float>(tapIndex + 1) / MAX_TAPS) - 1.0f) / (std::exp(1.0f) - 1.0f);
                break;
            case 3:
                tapRatio = std::pow(static_cast<float>(tapIndex + 1) / static_cast<float>(MAX_TAPS), 2.0f);
                break;
            case 4:
                tapRatio = std::sqrt(static_cast<float>(tapIndex + 1) / static_cast<float>(MAX_TAPS));
                break;
            default:
                float normalizedIndex = static_cast<float>(tapIndex + 1) / static_cast<float>(MAX_TAPS);
                float patternOffset = static_cast<float>(mPattern - 5) / 10.0f;
                tapRatio = std::pow(normalizedIndex, 1.0f + patternOffset);
                break;
        }
    }

    tapRatio = std::max(0.0f, std::min(1.0f, tapRatio));

    float effectiveSize = getSyncedCombSize();
    return applyCVScaling(effectiveSize * tapRatio);
}

float CombProcessor::applyCVScaling(float baseDelay) const
{
    float scaleFactor = std::pow(2.0f, -mPitchCV);
    return baseDelay * scaleFactor;
}

float CombProcessor::getTapGain(int tapIndex) const
{
    float tapPosition = static_cast<float>(tapIndex) / static_cast<float>(mNumActiveTaps - 1);
    float slopeGain = 1.0f;

    switch (mSlope) {
        case 0:
            slopeGain = 1.0f;
            break;
        case 1:
            slopeGain = tapPosition;
            break;
        case 2:
            slopeGain = 1.0f - tapPosition;
            break;
        case 3:
            if (tapPosition <= 0.5f) {
                slopeGain = tapPosition * 2.0f;
            } else {
                slopeGain = 2.0f - (tapPosition * 2.0f);
            }
            break;
        default:
            slopeGain = 1.0f;
            break;
    }

    return std::max(0.0f, std::min(1.0f, slopeGain));
}

float CombProcessor::tanhLimiter(float input) const
{
    return std::tanh(input);
}

void CombProcessor::processStereo(float inputL, float inputR, float& outputL, float& outputR)
{
    float mixedL = inputL + mFeedbackBufferL * mFeedback;
    float mixedR = inputR + mFeedbackBufferR * mFeedback;

    mDelayBufferL[mWriteIndex] = mixedL;
    mDelayBufferR[mWriteIndex] = mixedR;

    outputL = 0.0f;
    outputR = 0.0f;

    for (int tap = 0; tap < mNumActiveTaps; ++tap)
    {
        int physicalTap = tap * MAX_TAPS / mNumActiveTaps;
        if (physicalTap >= MAX_TAPS) physicalTap = MAX_TAPS - 1;

        float tapDelay = getTapDelay(physicalTap);
        float delaySamples = tapDelay * mSampleRate;

        int delayInt = static_cast<int>(delaySamples);
        float delayFrac = delaySamples - delayInt;

        int readIdx1 = (mWriteIndex - delayInt + mBufferSize) % mBufferSize;
        int readIdx2 = (readIdx1 - 1 + mBufferSize) % mBufferSize;

        float tapOutL = mDelayBufferL[readIdx1] * (1.0f - delayFrac) +
                       mDelayBufferL[readIdx2] * delayFrac;
        float tapOutR = mDelayBufferR[readIdx1] * (1.0f - delayFrac) +
                       mDelayBufferR[readIdx2] * delayFrac;

        float densityGain = 1.0f / std::sqrt(static_cast<float>(mNumActiveTaps));
        float slopeGain = getTapGain(tap);
        float totalGain = densityGain * slopeGain;

        outputL += tapOutL * totalGain;
        outputR += tapOutR * totalGain;
    }

    float maxDelay = applyCVScaling(getSyncedCombSize());
    float maxDelaySamples = maxDelay * mSampleRate;
    int maxDelayInt = static_cast<int>(maxDelaySamples);
    float maxDelayFrac = maxDelaySamples - maxDelayInt;

    int feedbackIdx1 = (mWriteIndex - maxDelayInt + mBufferSize) % mBufferSize;
    int feedbackIdx2 = (feedbackIdx1 - 1 + mBufferSize) % mBufferSize;

    float feedbackL = mDelayBufferL[feedbackIdx1] * (1.0f - maxDelayFrac) +
                     mDelayBufferL[feedbackIdx2] * maxDelayFrac;
    float feedbackR = mDelayBufferR[feedbackIdx1] * (1.0f - maxDelayFrac) +
                     mDelayBufferR[feedbackIdx2] * maxDelayFrac;

    mFeedbackBufferL = tanhLimiter(feedbackL);
    mFeedbackBufferR = tanhLimiter(feedbackR);

    mWriteIndex = (mWriteIndex + 1) % mBufferSize;
}

} // namespace WaterStick