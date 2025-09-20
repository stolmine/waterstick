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
    , mGain(1.0f)
    , mSampleCounter(0.0f)
{
    // Initialize tap positions vector
    mTapPositions.resize(MAX_TAPS);
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

    // Initialize tap positions
    mTapPositions.resize(MAX_TAPS);
    for (int i = 0; i < MAX_TAPS; ++i) {
        mTapPositions[i].currentPos = static_cast<float>(i);
        mTapPositions[i].targetPos = static_cast<float>(i);
        mTapPositions[i].previousPos = static_cast<float>(i);
    }

    reset();
}

void CombProcessor::reset()
{
    std::fill(mDelayBufferL.begin(), mDelayBufferL.end(), 0.0f);
    std::fill(mDelayBufferR.begin(), mDelayBufferR.end(), 0.0f);
    mWriteIndex = 0;
    mFeedbackBufferL = 0.0f;
    mFeedbackBufferR = 0.0f;
    mSampleCounter = 0.0f;

    // Reset fade state
    mFadeState.fadeType = TapFadeState::FADE_NONE;
    mFadeState.isActive = false;
    mFadeState.fadePosition = 0.0f;
    mFadeState.fadeDuration = 0.0f;
    mFadeState.fadeStartTime = 0.0f;
    mFadeState.targetTapCount = 0;
    mFadeState.previousTapCount = 0;

    // Clear previous output buffers
    mPreviousOutputL.clear();
    mPreviousOutputR.clear();

    // Reset tap positions
    for (int i = 0; i < MAX_TAPS; ++i) {
        mTapPositions[i].currentPos = static_cast<float>(i);
        mTapPositions[i].targetPos = static_cast<float>(i);
        mTapPositions[i].previousPos = static_cast<float>(i);
    }
}

void CombProcessor::setSize(float sizeSeconds)
{
    mCombSize = std::max(0.0001f, std::min(2.0f, sizeSeconds));
}

void CombProcessor::setNumTaps(int numTaps)
{
    // Start a fade transition to the new tap count
    startTapCountFade(numTaps);
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

void CombProcessor::setGain(float gain)
{
    mGain = std::max(0.0f, gain);  // Ensure non-negative gain
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
    // Update fade state and increment sample counter
    updateFadeState();

    float mixedL = inputL + mFeedbackBufferL * mFeedback;
    float mixedR = inputR + mFeedbackBufferR * mFeedback;

    mDelayBufferL[mWriteIndex] = mixedL;
    mDelayBufferR[mWriteIndex] = mixedR;

    outputL = 0.0f;
    outputR = 0.0f;

    // Use appropriate tap count based on fade state
    int activeTapCount = mFadeState.isActive ? mFadeState.targetTapCount : mNumActiveTaps;

    for (int tap = 0; tap < activeTapCount; ++tap)
    {
        float physicalTapFloat = getInterpolatedTapPosition(tap);
        float tapDelay = getTapDelayFromFloat(physicalTapFloat);
        float delaySamples = tapDelay * mSampleRate;

        int delayInt = static_cast<int>(delaySamples);
        float delayFrac = delaySamples - delayInt;

        int readIdx1 = (mWriteIndex - delayInt + mBufferSize) % mBufferSize;
        int readIdx2 = (readIdx1 - 1 + mBufferSize) % mBufferSize;

        float tapOutL = mDelayBufferL[readIdx1] * (1.0f - delayFrac) +
                       mDelayBufferL[readIdx2] * delayFrac;
        float tapOutR = mDelayBufferR[readIdx1] * (1.0f - delayFrac) +
                       mDelayBufferR[readIdx2] * delayFrac;

        float densityGain = 1.0f / std::sqrt(static_cast<float>(activeTapCount));
        float slopeGain = getTapGain(tap);
        float totalGain = densityGain * slopeGain;

        outputL += tapOutL * totalGain;
        outputR += tapOutR * totalGain;
    }

    // Apply fade processing if active
    if (mFadeState.isActive) {
        processFadedOutput(outputL, outputR);
    }

    // Apply the gain control to the final output
    outputL *= mGain;
    outputR *= mGain;

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

    // Increment sample counter for fade timing
    mSampleCounter += 1.0f;

    // Update tap positions for smooth interpolation
    updateTapPositions();
}

void CombProcessor::startTapCountFade(int newTapCount)
{
    // Clamp the new tap count to valid range
    int clampedTapCount = std::max(1, std::min(MAX_TAPS, newTapCount));

    // If the tap count isn't actually changing, don't start a fade
    if (clampedTapCount == mNumActiveTaps) {
        return;
    }

    // Store the current state
    mFadeState.previousTapCount = mNumActiveTaps;
    mFadeState.targetTapCount = clampedTapCount;
    mFadeState.fadeStartTime = mSampleCounter;
    mFadeState.fadeDuration = calculateFadeDurationSamples();
    mFadeState.fadePosition = 0.0f;
    mFadeState.isActive = true;

    // Determine fade type based on tap count change
    if (clampedTapCount > mNumActiveTaps) {
        mFadeState.fadeType = TapFadeState::FADE_IN;
    } else if (clampedTapCount < mNumActiveTaps) {
        mFadeState.fadeType = TapFadeState::FADE_OUT;
    } else {
        mFadeState.fadeType = TapFadeState::CROSSFADE;
    }

    // Initialize tap positions for interpolation
    for (int tap = 0; tap < std::max(mNumActiveTaps, clampedTapCount); ++tap) {
        if (tap < mNumActiveTaps) {
            mTapPositions[tap].previousPos = static_cast<float>(tap) * static_cast<float>(MAX_TAPS - 1) / static_cast<float>(mNumActiveTaps - 1);
        } else {
            mTapPositions[tap].previousPos = static_cast<float>(MAX_TAPS - 1);
        }

        if (tap < clampedTapCount) {
            mTapPositions[tap].targetPos = static_cast<float>(tap) * static_cast<float>(MAX_TAPS - 1) / static_cast<float>(clampedTapCount - 1);
        } else {
            mTapPositions[tap].targetPos = static_cast<float>(MAX_TAPS - 1);
        }

        mTapPositions[tap].currentPos = mTapPositions[tap].previousPos;
    }

    // Initialize previous output buffers for crossfade if needed
    if (mFadeState.fadeType == TapFadeState::CROSSFADE) {
        int bufferSize = static_cast<int>(mFadeState.fadeDuration) + 1;
        mPreviousOutputL.resize(bufferSize, 0.0f);
        mPreviousOutputR.resize(bufferSize, 0.0f);
    }
}

void CombProcessor::updateFadeState()
{
    if (!mFadeState.isActive) {
        return;
    }

    // Calculate fade progress
    float elapsed = mSampleCounter - mFadeState.fadeStartTime;
    mFadeState.fadePosition = elapsed / mFadeState.fadeDuration;

    // Check if fade is complete
    if (mFadeState.fadePosition >= 1.0f) {
        mFadeState.fadePosition = 1.0f;
        mFadeState.isActive = false;

        // Update the actual tap count when fade completes
        mNumActiveTaps = mFadeState.targetTapCount;

        // Clear previous output buffers
        mPreviousOutputL.clear();
        mPreviousOutputR.clear();
    }
}

float CombProcessor::calculateFadeGain(float fadePosition, TapFadeState::FadeType fadeType) const
{
    // Clamp fade position to valid range
    fadePosition = std::max(0.0f, std::min(1.0f, fadePosition));

    switch (fadeType) {
        case TapFadeState::FADE_IN:
            // Exponential fade in: starts at 0, curves up to 1
            return 1.0f - std::exp(-5.0f * fadePosition);

        case TapFadeState::FADE_OUT:
            // Exponential fade out: starts at 1, curves down to 0
            return std::exp(-5.0f * fadePosition);

        case TapFadeState::CROSSFADE:
            // Symmetric crossfade curve
            if (fadePosition <= 0.5f) {
                // First half: fade out old
                return std::exp(-5.0f * (fadePosition * 2.0f));
            } else {
                // Second half: fade in new
                return 1.0f - std::exp(-5.0f * ((fadePosition - 0.5f) * 2.0f));
            }

        case TapFadeState::FADE_NONE:
        default:
            return 1.0f;
    }
}

void CombProcessor::processFadedOutput(float& outputL, float& outputR)
{
    if (!mFadeState.isActive) {
        return;
    }

    // Calculate current fade gain
    float currentGain = calculateFadeGain(mFadeState.fadePosition, mFadeState.fadeType);

    switch (mFadeState.fadeType) {
        case TapFadeState::FADE_IN:
            // For fade in, apply gain to new output (gradually bring it in)
            outputL *= currentGain;
            outputR *= currentGain;
            break;

        case TapFadeState::FADE_OUT:
            // For fade out, apply gain to current output (gradually remove it)
            outputL *= currentGain;
            outputR *= currentGain;
            break;

        case TapFadeState::CROSSFADE:
            // For crossfade, blend between old and new output
            // This would require storing previous output, which is complex
            // For now, just apply the gain
            outputL *= currentGain;
            outputR *= currentGain;
            break;

        case TapFadeState::FADE_NONE:
        default:
            // No processing needed
            break;
    }
}

float CombProcessor::calculateFadeDurationSamples() const
{
    // Calculate fade duration as one echo period (the time for the longest delay)
    float maxDelay = applyCVScaling(getSyncedCombSize());
    float fadeDurationSeconds = maxDelay;

    // Convert to samples and ensure minimum duration
    float fadeDurationSamples = fadeDurationSeconds * static_cast<float>(mSampleRate);

    // Ensure minimum fade duration of 64 samples (about 1.45ms at 44.1kHz)
    return std::max(64.0f, fadeDurationSamples);
}

void CombProcessor::updateTapPositions()
{
    if (!mFadeState.isActive) {
        return;
    }

    // Use hermite interpolation for smooth curves
    float t = mFadeState.fadePosition;
    float smoothT = t * t * (3.0f - 2.0f * t);

    int activeTapCount = mFadeState.isActive ? mFadeState.targetTapCount : mNumActiveTaps;
    int previousTapCount = mFadeState.previousTapCount;

    for (int tap = 0; tap < std::max(activeTapCount, previousTapCount); ++tap) {
        if (tap < activeTapCount && tap < previousTapCount) {
            // Calculate target and previous positions
            float targetPos = static_cast<float>(tap) * static_cast<float>(MAX_TAPS - 1) / static_cast<float>(activeTapCount - 1);
            float previousPos = static_cast<float>(tap) * static_cast<float>(MAX_TAPS - 1) / static_cast<float>(previousTapCount - 1);

            // Interpolate between previous and target positions
            mTapPositions[tap].targetPos = targetPos;
            mTapPositions[tap].previousPos = previousPos;
            mTapPositions[tap].currentPos = previousPos + (targetPos - previousPos) * smoothT;
        }
    }
}

float CombProcessor::getInterpolatedTapPosition(int tap) const
{
    if (tap >= MAX_TAPS) {
        return static_cast<float>(MAX_TAPS - 1);
    }

    if (!mFadeState.isActive) {
        // No fade active, use direct mapping
        int activeTapCount = mNumActiveTaps;
        if (tap >= activeTapCount) {
            return static_cast<float>(MAX_TAPS - 1);
        }
        return static_cast<float>(tap) * static_cast<float>(MAX_TAPS - 1) / static_cast<float>(activeTapCount - 1);
    }

    // Use interpolated position during fade
    return mTapPositions[tap].currentPos;
}

float CombProcessor::getTapDelayFromFloat(float tapPosition) const
{
    // Clamp to valid range
    tapPosition = std::max(0.0f, std::min(static_cast<float>(MAX_TAPS - 1), tapPosition));

    // Convert float position to tap index for pattern calculation
    int tapIndex = static_cast<int>(tapPosition);
    float frac = tapPosition - static_cast<float>(tapIndex);

    // Calculate delay for this tap index using existing pattern logic
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

    // If there's a fractional part, interpolate with next tap
    if (frac > 0.0f && tapIndex < MAX_TAPS - 1) {
        int nextTapIndex = tapIndex + 1;
        float nextTapRatio;

        if (mPattern == 0) {
            nextTapRatio = static_cast<float>(nextTapIndex + 1) / static_cast<float>(MAX_TAPS);
        } else {
            switch (mPattern) {
                case 1:
                    nextTapRatio = std::log(nextTapIndex + 1.0f) / std::log(MAX_TAPS);
                    break;
                case 2:
                    nextTapRatio = (std::exp(static_cast<float>(nextTapIndex + 1) / MAX_TAPS) - 1.0f) / (std::exp(1.0f) - 1.0f);
                    break;
                case 3:
                    nextTapRatio = std::pow(static_cast<float>(nextTapIndex + 1) / static_cast<float>(MAX_TAPS), 2.0f);
                    break;
                case 4:
                    nextTapRatio = std::sqrt(static_cast<float>(nextTapIndex + 1) / static_cast<float>(MAX_TAPS));
                    break;
                default:
                    float normalizedIndex = static_cast<float>(nextTapIndex + 1) / static_cast<float>(MAX_TAPS);
                    float patternOffset = static_cast<float>(mPattern - 5) / 10.0f;
                    nextTapRatio = std::pow(normalizedIndex, 1.0f + patternOffset);
                    break;
            }
        }

        // Interpolate between the two tap ratios
        tapRatio = tapRatio + (nextTapRatio - tapRatio) * frac;
    }

    tapRatio = std::max(0.0f, std::min(1.0f, tapRatio));

    float effectiveSize = getSyncedCombSize();
    return applyCVScaling(effectiveSize * tapRatio);
}

} // namespace WaterStick