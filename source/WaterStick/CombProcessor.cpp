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
    , mFadeMode(FADE_MODE_AUTO)
    , mUserFadeTime(25.0f)
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
    mFadeState.parameterType = TAP_COUNT;
    mFadeState.previousValue = 0.0f;
    mFadeState.targetValue = 0.0f;
    mFadeState.currentValue = 0.0f;


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
    float clampedSize = std::max(0.0001f, std::min(2.0f, sizeSeconds));

    // Deadband with hysteresis to prevent oscillation during parameter movement
    static const float DEADBAND_THRESHOLD = 0.005f;  // 0.5% deadband threshold
    static const float HYSTERESIS_FACTOR = 1.5f;     // 1.5x hysteresis factor

    // Determine reference value and threshold based on fade state
    float referenceValue;
    float threshold;

    if (mFadeState.isActive && mFadeState.parameterType == SIZE) {
        // During active fade: use targetValue and hysteresis threshold
        referenceValue = mFadeState.targetValue;
        threshold = referenceValue * DEADBAND_THRESHOLD * HYSTERESIS_FACTOR; // 0.75% threshold
    } else {
        // No active fade: use current value and deadband threshold
        referenceValue = mCombSize;
        threshold = referenceValue * DEADBAND_THRESHOLD; // 0.5% threshold
    }

    if (std::abs(clampedSize - referenceValue) > threshold) {
        // Significant change - start parameter fade
        startParameterFade(SIZE, clampedSize);
    }
    // For small changes: let existing fade complete, no direct updates
}

void CombProcessor::setNumTaps(int numTaps)
{
    // Start a fade transition to the new tap count
    startTapCountFade(numTaps);
}

void CombProcessor::setFeedback(float feedback)
{
    float newFeedback = std::max(0.0f, std::min(0.99f, feedback));

    // Check if change is significant enough to warrant smoothing (threshold: 0.01)
    float currentFeedbackValue = getSmoothedParameterValue(FEEDBACK);
    float changeAmount = std::abs(newFeedback - currentFeedbackValue);

    if (changeAmount > 0.01f) {
        // Start parameter fade for significant changes
        startParameterFade(FEEDBACK, newFeedback);
    } else {
        // For small changes, update directly
        mFeedback = newFeedback;
    }
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
    int clampedPattern = std::max(0, std::min(kNumCombPatterns - 1, pattern));

    // Only start fade if pattern actually changes
    if (clampedPattern != mPattern) {
        startParameterFade(PATTERN, static_cast<float>(clampedPattern));
    }
}

void CombProcessor::setSlope(int slope)
{
    mSlope = std::max(0, std::min(kNumCombSlopes - 1, slope));
}

void CombProcessor::setGain(float gain)
{
    mGain = std::max(0.0f, gain);  // Ensure non-negative gain
}

void CombProcessor::setFadeTime(float fadeTimeMs)
{
    // Set mUserFadeTime and mFadeMode based on fade time
    if (fadeTimeMs <= 1.0f) {
        // If fadeTime <= 1ms: set FADE_MODE_INSTANT
        mFadeMode = FADE_MODE_INSTANT;
        mUserFadeTime = 1.0f;  // Minimum time for instant mode
    } else if (fadeTimeMs <= 0.0f) {
        // If fadeTime is special value (0 or negative): set FADE_MODE_AUTO
        mFadeMode = FADE_MODE_AUTO;
        mUserFadeTime = 25.0f;  // Default auto time
    } else {
        // Otherwise: set FADE_MODE_FIXED
        mFadeMode = FADE_MODE_FIXED;
        mUserFadeTime = std::max(1.0f, std::min(2000.0f, fadeTimeMs));  // Clamp to valid range
    }
}

void CombProcessor::updateTempo(double hostTempo, bool isValid)
{
    mHostTempo = static_cast<float>(hostTempo);
    mHostTempoValid = isValid;
}

void CombProcessor::startParameterFade(ParameterType paramType, float newValue)
{
    // If a different parameter type is already fading, complete it first
    if (mFadeState.isActive && mFadeState.parameterType != paramType) {
        // Complete current fade by setting the parameter directly
        switch (mFadeState.parameterType) {
            case SIZE:
                mCombSize = mFadeState.targetValue;
                break;
            case TAP_COUNT:
                mNumActiveTaps = static_cast<int>(mFadeState.targetValue);
                break;
            case FEEDBACK:
                mFeedback = mFadeState.targetValue;
                break;
            case PATTERN:
                mPattern = static_cast<int>(mFadeState.targetValue);
                break;
            case PITCH:
                mPitchCV = mFadeState.targetValue;
                break;
            default:
                break;
        }
    }

    // Set up new parameter fade
    mFadeState.parameterType = paramType;
    mFadeState.fadeStartTime = mSampleCounter;
    mFadeState.targetValue = newValue;
    mFadeState.fadeDuration = calculateFadeDurationSamples();
    mFadeState.fadePosition = 0.0f;
    mFadeState.isActive = true;

    // Set previous value and current value based on parameter type
    switch (paramType) {
        case SIZE:
            mFadeState.previousValue = mCombSize;
            mFadeState.currentValue = mCombSize;
            break;
        case TAP_COUNT:
            mFadeState.previousValue = static_cast<float>(mNumActiveTaps);
            mFadeState.currentValue = static_cast<float>(mNumActiveTaps);
            break;
        case FEEDBACK:
            mFadeState.previousValue = mFeedback;
            mFadeState.currentValue = mFeedback;
            break;
        case PATTERN:
            mFadeState.previousValue = static_cast<float>(mPattern);
            mFadeState.currentValue = static_cast<float>(mPattern);
            break;
        case PITCH:
            mFadeState.previousValue = mPitchCV;
            mFadeState.currentValue = mPitchCV;
            break;
        default:
            mFadeState.previousValue = 0.0f;
            mFadeState.currentValue = 0.0f;
            break;
    }
}

float CombProcessor::getSmoothedParameterValue(ParameterType paramType) const
{
    if (mFadeState.isActive && mFadeState.parameterType == paramType) {
        return mFadeState.currentValue;
    }

    // Return current direct value if not fading
    switch (paramType) {
        case SIZE:
            return mCombSize;
        case TAP_COUNT:
            return static_cast<float>(mNumActiveTaps);
        case FEEDBACK:
            return mFeedback;
        case PATTERN:
            return static_cast<float>(mPattern);
        case PITCH:
            return mPitchCV;
        default:
            return 0.0f;
    }
}

bool CombProcessor::isParameterFading(ParameterType paramType) const
{
    return mFadeState.isActive && mFadeState.parameterType == paramType;
}

float CombProcessor::getSyncedCombSize() const
{
    // Use smoothed size value if fading, otherwise use direct size
    float baseSize = getSmoothedParameterValue(SIZE);

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
        return baseSize;
    }
}

float CombProcessor::getTapDelay(int tapIndex) const
{
    // Use smoothed pattern value for smooth transitions
    float smoothedPattern = getSmoothedParameterValue(PATTERN);
    return calculateTapRatioForPattern(tapIndex, smoothedPattern);
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

    // Use smoothed feedback value instead of direct mFeedback
    float smoothedFeedback = getSmoothedParameterValue(FEEDBACK);

    float mixedL = inputL + mFeedbackBufferL * smoothedFeedback;
    float mixedR = inputR + mFeedbackBufferR * smoothedFeedback;

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
    mFadeState.parameterType = TAP_COUNT;
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

    // Check if fade is complete (99.8% to avoid interpolation errors)
    if (mFadeState.fadePosition >= 0.998f) {
        mFadeState.fadePosition = 1.0f;
        mFadeState.isActive = false;

        // Force exact target value to eliminate settling issues
        mFadeState.currentValue = mFadeState.targetValue;

        // Update the actual parameter value when fade completes
        if (mFadeState.parameterType == TAP_COUNT) {
            mNumActiveTaps = mFadeState.targetTapCount;
        } else {
            switch (mFadeState.parameterType) {
                case SIZE:
                    mCombSize = mFadeState.targetValue;
                    break;
                case FEEDBACK:
                    mFeedback = mFadeState.targetValue;
                    break;
                case PATTERN:
                    mPattern = static_cast<int>(mFadeState.targetValue);
                    break;
                case PITCH:
                    mPitchCV = mFadeState.targetValue;
                    break;
                default:
                    break;
            }
        }

        // Clear previous output buffers
        mPreviousOutputL.clear();
        mPreviousOutputR.clear();
    }

    // Update current interpolated value for parameter fades
    if (mFadeState.parameterType != TAP_COUNT) {
        float t = mFadeState.fadePosition;
        // Use exponential approach for better settling behavior
        float alpha = 1.0f - std::exp(-6.0f * t);
        mFadeState.currentValue = mFadeState.previousValue + (mFadeState.targetValue - mFadeState.previousValue) * alpha;
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

float CombProcessor::getMaxFadeTimeForParameter(ParameterType paramType) const
{
    switch (paramType) {
        case SIZE:
        case PATTERN:
        case PITCH:
            return mUserFadeTime; // Full range: 1ms-2000ms
        case FEEDBACK:
        case TAP_COUNT:
            return std::min(mUserFadeTime, 100.0f); // Capped at 100ms for responsiveness
        default:
            return mUserFadeTime;
    }
}

float CombProcessor::calculateFadeDurationSamples() const
{
    float clampedFadeMs;

    switch (mFadeMode) {
        case FADE_MODE_AUTO:
            {
                // Parameter-type-specific fade durations for optimal responsiveness
                switch (mFadeState.parameterType) {
                    case FEEDBACK:
                    case TAP_COUNT:
                        // Fast fade for real-time control parameters - keep responsive (5ms-100ms max)
                        if (mFadeState.parameterType == TAP_COUNT) {
                            // Adaptive timing for discrete parameter changes
                            // Calculate tap count change magnitude for adaptive timing
                            int tapCountChange = std::abs(mFadeState.targetTapCount - mFadeState.previousTapCount);
                            float tapChangeRatio = static_cast<float>(tapCountChange) / static_cast<float>(MAX_TAPS);

                            // Base fade time: 25ms for discrete changes
                            float baseFadeMs = 25.0f;

                            // Adaptive scaling based on magnitude of change:
                            // - Small changes (1-8 taps): 15-25ms
                            // - Medium changes (9-32 taps): 25-60ms
                            // - Large changes (33-64 taps): 60-100ms
                            float adaptiveScale = 1.0f + (tapChangeRatio * 3.0f);  // Scale up to 4x for full range
                            float adaptiveFadeMs = baseFadeMs * adaptiveScale;

                            // Clamp to responsive range: 15-100ms for tap count
                            clampedFadeMs = std::max(15.0f, std::min(100.0f, adaptiveFadeMs));
                        } else {
                            // Fast fade for continuous feedback parameter - reduces artifacts during real-time movement
                            clampedFadeMs = 5.0f;
                        }
                        break;
                    case SIZE:
                    case PITCH:
                        // Fast fade for continuous parameters - reduces artifacts during real-time movement
                        clampedFadeMs = 5.0f;
                        break;
                    case PATTERN:
                        // Fast fade for discrete parameter changes
                        clampedFadeMs = 25.0f;
                        break;
                    default:
                        // Fallback for unknown parameter types
                        clampedFadeMs = 25.0f;
                        break;
                }
            }
            break;

        case FADE_MODE_FIXED:
            // Use parameter-specific maximum fade time
            clampedFadeMs = getMaxFadeTimeForParameter(mFadeState.parameterType);
            break;

        case FADE_MODE_INSTANT:
            // Return minimal samples (1-2 samples)
            return std::max(1.0f, 2.0f);

        default:
            // Fallback to auto mode logic
            clampedFadeMs = 25.0f;
            break;
    }

    // Convert to samples
    float fadeDurationSamples = (clampedFadeMs / 1000.0f) * static_cast<float>(mSampleRate);

    // Ensure minimum of 32 samples (about 0.73ms at 44.1kHz) for numerical stability
    // Reduced from 64 samples for faster response to continuous parameter changes
    // Exception: INSTANT mode can go below this threshold
    if (mFadeMode == FADE_MODE_INSTANT) {
        return std::max(1.0f, fadeDurationSamples);
    } else {
        return std::max(32.0f, fadeDurationSamples);
    }
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

    // Use smoothed pattern value for smooth transitions
    float smoothedPattern = getSmoothedParameterValue(PATTERN);

    // Calculate delay for this tap index using smoothed pattern
    float tapRatio = calculateTapRatioForPattern(tapIndex, smoothedPattern);

    // If there's a fractional part, interpolate with next tap
    if (frac > 0.0f && tapIndex < MAX_TAPS - 1) {
        int nextTapIndex = tapIndex + 1;
        float nextTapRatio = calculateTapRatioForPattern(nextTapIndex, smoothedPattern);

        // Interpolate between the two tap ratios
        tapRatio = tapRatio + (nextTapRatio - tapRatio) * frac;
    }

    tapRatio = std::max(0.0f, std::min(1.0f, tapRatio));

    float effectiveSize = getSyncedCombSize();
    return applyCVScaling(effectiveSize * tapRatio);
}


void CombProcessor::updateParameterFades()
{
    // This is now handled by updateFadeState() which supports both tap count and parameter fades
    updateFadeState();
}

float CombProcessor::calculateTapRatioForPattern(int tapIndex, float patternValue) const
{
    // Handle fractional pattern values by interpolating between adjacent patterns
    int patternLow = static_cast<int>(patternValue);
    int patternHigh = std::min(patternLow + 1, kNumCombPatterns - 1);
    float patternFrac = patternValue - static_cast<float>(patternLow);

    // Calculate tap ratio for lower pattern
    float tapRatioLow = calculateTapRatioForDiscretePattern(tapIndex, patternLow);

    // If no interpolation needed (integer pattern value), return directly
    if (patternFrac <= 0.0f || patternLow == patternHigh) {
        float effectiveSize = getSyncedCombSize();
        return applyCVScaling(effectiveSize * tapRatioLow);
    }

    // Calculate tap ratio for higher pattern
    float tapRatioHigh = calculateTapRatioForDiscretePattern(tapIndex, patternHigh);

    // Interpolate between the two patterns
    float interpolatedRatio = tapRatioLow + (tapRatioHigh - tapRatioLow) * patternFrac;
    interpolatedRatio = std::max(0.0f, std::min(1.0f, interpolatedRatio));

    float effectiveSize = getSyncedCombSize();
    return applyCVScaling(effectiveSize * interpolatedRatio);
}

float CombProcessor::calculateTapRatioForDiscretePattern(int tapIndex, int pattern) const
{
    float tapRatio;

    if (pattern == 0) {
        tapRatio = static_cast<float>(tapIndex + 1) / static_cast<float>(MAX_TAPS);
    } else {
        switch (pattern) {
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
                float patternOffset = static_cast<float>(pattern - 5) / 10.0f;
                tapRatio = std::pow(normalizedIndex, 1.0f + patternOffset);
                break;
        }
    }

    return std::max(0.0f, std::min(1.0f, tapRatio));
}

} // namespace WaterStick