#include "WaterStickProcessor.h"
#include "WaterStickCIDs.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include <cmath>

using namespace Steinberg;

namespace WaterStick {


struct ParameterConverter {
    static float convertFilterCutoff(double value) {
        return static_cast<float>(20.0 * std::pow(1000.0, value));
    }

    static float convertFilterResonance(double value) {
        if (value >= 0.5) {
            float positiveValue = (value - 0.5f) * 2.0f;
            if (positiveValue >= 0.9f) {
                float highResValue = (positiveValue - 0.9f) / 0.1f;
                return 0.7f + highResValue * 0.3f;
            } else {
                float lowResValue = positiveValue / 0.9f;
                return lowResValue * 0.7f;
            }
        } else {
            return static_cast<float>((value - 0.5f) * 2.0f);
        }
    }

    static int convertFilterType(double value) {
        return static_cast<int>(value * (kNumFilterTypes - 1) + 0.5);
    }

    static float convertGain(double value) {
        float dbValue = -40.0f + (static_cast<float>(value) * 52.0f);
        return std::pow(10.0f, dbValue / 20.0f);
    }

    static float convertFeedback(double value) {
        float normalizedValue = static_cast<float>(value);
        return normalizedValue * normalizedValue * normalizedValue;
    }

    static float convertCombSize(double value) {
        float normalizedValue = static_cast<float>(value);
        return 0.0001f * std::pow(20000.0f, normalizedValue);
    }

    static float convertCombFeedback(double value) {
        float normalizedValue = static_cast<float>(value);
        return normalizedValue * normalizedValue * normalizedValue * 0.99f;
    }

    static float convertCombPitchCV(double value) {
        return -5.0f + (static_cast<float>(value) * 10.0f);
    }

    static int convertCombTaps(double value) {
        return static_cast<int>(value * 63.0 + 1.0 + 0.5);
    }
};

struct TapParameterRange {
    Vst::ParamID startId;
    Vst::ParamID endId;
    int paramsPerTap;

    bool contains(Vst::ParamID paramId) const {
        return paramId >= startId && paramId <= endId;
    }

    void getIndices(Vst::ParamID paramId, int& tapIndex, int& paramType) const {
        int paramOffset = paramId - startId;
        tapIndex = paramOffset / paramsPerTap;
        paramType = paramOffset % paramsPerTap;
    }
};

struct TapParameterProcessor {
    static const TapParameterRange kTapBasicRange;
    static const TapParameterRange kTapFilterRange;

    static void processTapParameter(Vst::ParamID paramId, Vst::ParamValue value, WaterStickProcessor* processor) {
        if (kTapBasicRange.contains(paramId)) {
            int tapIndex, paramType;
            kTapBasicRange.getIndices(paramId, tapIndex, paramType);

            if (tapIndex < 16) {
                switch (paramType) {
                    case 0: processor->mTapEnabled[tapIndex] = value > 0.5; break;
                    case 1: processor->mTapLevel[tapIndex] = static_cast<float>(value); break;
                    case 2: processor->mTapPan[tapIndex] = static_cast<float>(value); break;
                }
            }
            return;
        }

        if (kTapFilterRange.contains(paramId)) {
            int tapIndex, paramType;
            kTapFilterRange.getIndices(paramId, tapIndex, paramType);

            if (tapIndex < 16) {
                switch (paramType) {
                    case 0: processor->mTapFilterCutoff[tapIndex] = ParameterConverter::convertFilterCutoff(value); break;
                    case 1: processor->mTapFilterResonance[tapIndex] = ParameterConverter::convertFilterResonance(value); break;
                    case 2: processor->mTapFilterType[tapIndex] = ParameterConverter::convertFilterType(value); break;
                }
            }
        }
    }
};

const TapParameterRange TapParameterProcessor::kTapBasicRange = {kTap1Enable, kTap16Pan, 3};
const TapParameterRange TapParameterProcessor::kTapFilterRange = {kTap1FilterCutoff, kTap16FilterType, 3};

const char* TempoSync::sDivisionTexts[kNumSyncDivisions] = {
    "1/64", "1/32T", "1/64.", "1/32", "1/16T", "1/32.", "1/16",
    "1/8T", "1/16.", "1/8", "1/4T", "1/8.", "1/4",
    "1/2T", "1/4.", "1/2", "1T", "1/2.", "1", "2", "4", "8"
};

const float TempoSync::sDivisionValues[kNumSyncDivisions] = {
    0.0625f,     // 1/64
    0.08333f,    // 1/32T (1/32 triplet)
    0.09375f,    // 1/64. (1/64 dotted)
    0.125f,      // 1/32
    0.16667f,    // 1/16T (1/16 triplet)
    0.1875f,     // 1/32. (1/32 dotted)
    0.25f,       // 1/16
    0.33333f,    // 1/8T (1/8 triplet)
    0.375f,      // 1/16. (1/16 dotted)
    0.5f,        // 1/8
    0.66667f,    // 1/4T (1/4 triplet)
    0.75f,       // 1/8. (1/8 dotted)
    1.0f,        // 1/4
    1.33333f,    // 1/2T (1/2 triplet)
    1.5f,        // 1/4. (1/4 dotted)
    2.0f,        // 1/2
    2.66667f,    // 1T (1 bar triplet)
    3.0f,        // 1/2. (1/2 dotted)
    4.0f,        // 1 (1 bar)
    8.0f,        // 2 (2 bars)
    16.0f,       // 4 (4 bars)
    32.0f        // 8 (8 bars)
};

TempoSync::TempoSync()
: mSampleRate(44100.0)
, mHostTempo(120.0)
, mHostTempoValid(false)
, mIsSynced(false)
, mSyncDivision(kSync_1_4)
, mFreeTime(0.25f)
{
}

TempoSync::~TempoSync()
{
}

void TempoSync::initialize(double sampleRate)
{
    mSampleRate = sampleRate;
}

void TempoSync::updateTempo(double hostTempo, bool isValid)
{
    mHostTempo = hostTempo;
    mHostTempoValid = isValid;
}

void TempoSync::setMode(bool isSynced)
{
    mIsSynced = isSynced;
}

void TempoSync::setSyncDivision(int division)
{
    if (division >= 0 && division < kNumSyncDivisions) {
        mSyncDivision = division;
    }
}

void TempoSync::setFreeTime(float timeSeconds)
{
    mFreeTime = timeSeconds;
}

float TempoSync::getDelayTime() const
{
    if (mIsSynced && mHostTempoValid) {
        return calculateSyncTime();
    } else {
        return mFreeTime;
    }
}

float TempoSync::calculateSyncTime() const
{
    if (!mHostTempoValid || mHostTempo <= 0.0) {
        return mFreeTime;
    }

    double quarterNoteTime = 60.0 / mHostTempo;
    float divisionValue = sDivisionValues[mSyncDivision];
    return static_cast<float>(quarterNoteTime * divisionValue);
}

const char* TempoSync::getDivisionText() const
{
    return sDivisionTexts[mSyncDivision];
}

const char* TempoSync::getModeText() const
{
    return mIsSynced ? "Synced" : "Free";
}

const float TapDistribution::sGridValues[kNumGridValues] = {
    1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f, 16.0f
};

const char* TapDistribution::sGridTexts[kNumGridValues] = {
    "1", "2", "3", "4", "6", "8", "12", "16"
};

TapDistribution::TapDistribution()
: mSampleRate(44100.0)
, mBeatTime(0.5f)
, mGrid(kGrid_4)
{
    for (int i = 0; i < NUM_TAPS; i++) {
        mTapEnabled[i] = false;
        mTapLevel[i] = 0.8f;
        mTapPan[i] = 0.5f;
        mTapDelayTimes[i] = 0.0f;
    }
    calculateTapTimes();
}

TapDistribution::~TapDistribution()
{
}

void TapDistribution::initialize(double sampleRate)
{
    mSampleRate = sampleRate;
}

void TapDistribution::updateTempo(const TempoSync& tempoSync)
{
    mBeatTime = tempoSync.getDelayTime();
    calculateTapTimes();
}

void TapDistribution::setGrid(int gridValue)
{
    if (gridValue >= 0 && gridValue < kNumGridValues) {
        mGrid = gridValue;
        calculateTapTimes();
    }
}

void TapDistribution::setTapEnable(int tapIndex, bool enabled)
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        mTapEnabled[tapIndex] = enabled;
    }
}

void TapDistribution::setTapLevel(int tapIndex, float level)
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        mTapLevel[tapIndex] = std::isfinite(level) ? std::max(0.0f, std::min(level, 1.0f)) : 1.0f;
    } else {
    }
}

void TapDistribution::setTapPan(int tapIndex, float pan)
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        mTapPan[tapIndex] = std::max(0.0f, std::min(pan, 1.0f));
    }
}

void TapDistribution::calculateTapTimes()
{
    float gridValue = sGridValues[mGrid];

    for (int tap = 0; tap < NUM_TAPS; tap++) {
        int tapNumber = tap + 1;
        mTapDelayTimes[tap] = mBeatTime * static_cast<float>(tapNumber) / gridValue;
        mTapDelayTimes[tap] = std::max(mTapDelayTimes[tap], 0.001f);
    }
}

float TapDistribution::getTapDelayTime(int tapIndex) const
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        return mTapDelayTimes[tapIndex];
    }
    return 0.0f;
}

bool TapDistribution::isTapEnabled(int tapIndex) const
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        return mTapEnabled[tapIndex];
    }
    return false;
}

float TapDistribution::getTapLevel(int tapIndex) const
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        return mTapLevel[tapIndex];
    }
    return 0.0f;
}

float TapDistribution::getTapPan(int tapIndex) const
{
    if (tapIndex >= 0 && tapIndex < NUM_TAPS) {
        return mTapPan[tapIndex];
    }
    return 0.5f;
}

const char* TapDistribution::getGridText() const
{
    return sGridTexts[mGrid];
}


DualDelayLine::DualDelayLine()
: mBufferSize(0)
, mWriteIndexA(0)
, mWriteIndexB(0)
, mSampleRate(44100.0)
, mUsingLineA(true)
, mCrossfadeState(STABLE)
, mTargetDelayTime(0.1f)
, mCurrentDelayTime(0.1f)
, mStabilityCounter(0)
, mStabilityThreshold(2048)
, mCrossfadeLength(0)
, mCrossfadePosition(0)
, mCrossfadeGainA(1.0f)
, mCrossfadeGainB(0.0f)
{
    mStateA.delayInSamples = 0.5f;
    mStateA.readIndex = 0;
    mStateA.allpassCoeff = 0.0f;
    mStateA.apInput = 0.0f;
    mStateA.lastOutput = 0.0f;
    mStateA.doNextOut = true;
    mStateA.nextOutput = 0.0f;

    mStateB = mStateA;
}

DualDelayLine::~DualDelayLine()
{
}

void DualDelayLine::initialize(double sampleRate, double maxDelaySeconds)
{
    mSampleRate = sampleRate;
    mBufferSize = static_cast<int>(maxDelaySeconds * sampleRate) + 1;

    mBufferA.resize(mBufferSize, 0.0f);
    mBufferB.resize(mBufferSize, 0.0f);

    mWriteIndexA = 0;
    mWriteIndexB = 0;

    updateDelayState(mStateA, mCurrentDelayTime);
    updateDelayState(mStateB, mCurrentDelayTime);

    mStabilityThreshold = static_cast<int>(sampleRate * 0.05);
}

void DualDelayLine::setDelayTime(float delayTimeSeconds)
{
    if (std::abs(delayTimeSeconds - mTargetDelayTime) > 0.001f) {
        mTargetDelayTime = delayTimeSeconds;
        mStabilityCounter = 0;
    }
}

void DualDelayLine::updateDelayState(DelayLineState& state, float delayTime)
{
    float delaySamples = delayTime * static_cast<float>(mSampleRate);
    float maxDelaySamples = static_cast<float>(mBufferSize - 1);

    state.delayInSamples = std::max(0.5f, std::min(delaySamples, maxDelaySamples));
    updateAllpassCoeff(state);
}

void DualDelayLine::updateAllpassCoeff(DelayLineState& state)
{
    float integerPart = floorf(state.delayInSamples);
    float fracPart = state.delayInSamples - integerPart;

    if (fracPart < 0.5f) {
        fracPart = 0.5f;
    }

    state.allpassCoeff = (1.0f - fracPart) / (1.0f + fracPart);
}

float DualDelayLine::nextOut(DelayLineState& state, const std::vector<float>& buffer)
{
    if (state.doNextOut) {
        state.nextOutput = -state.allpassCoeff * state.lastOutput;
        state.nextOutput += state.apInput + (state.allpassCoeff * buffer[state.readIndex]);
        state.doNextOut = false;
    }
    return state.nextOutput;
}

float DualDelayLine::processDelayLine(std::vector<float>& buffer, int& writeIndex, DelayLineState& state, float input)
{
    // Calculate integer delay part
    int integerDelay = static_cast<int>(floorf(state.delayInSamples));

    // Update read index for integer delay
    state.readIndex = writeIndex - integerDelay;
    if (state.readIndex < 0) state.readIndex += mBufferSize;
    state.readIndex %= mBufferSize;

    // Write input to buffer
    buffer[writeIndex] = input;

    // Get output using STK allpass interpolation
    float output = nextOut(state, buffer);
    state.lastOutput = output;
    state.doNextOut = true;
    state.apInput = buffer[state.readIndex];

    // Advance write index
    writeIndex = (writeIndex + 1) % mBufferSize;

    return output;
}

int DualDelayLine::calculateCrossfadeLength(float delayTime)
{
    float baseCrossfadeMs = 50.0f + (delayTime * 1000.0f * 0.25f);
    baseCrossfadeMs = std::min(baseCrossfadeMs, 500.0f);

    return static_cast<int>(baseCrossfadeMs * 0.001f * mSampleRate);
}

void DualDelayLine::startCrossfade()
{
    mCrossfadeState = CROSSFADING;
    mCrossfadeLength = calculateCrossfadeLength(mTargetDelayTime);
    mCrossfadePosition = 0;

    if (mUsingLineA) {
        updateDelayState(mStateB, mTargetDelayTime);
    } else {
        updateDelayState(mStateA, mTargetDelayTime);
    }
}

void DualDelayLine::updateCrossfade()
{
    if (mCrossfadeState != CROSSFADING) return;

    float progress = static_cast<float>(mCrossfadePosition) / static_cast<float>(mCrossfadeLength);
    progress = std::min(progress, 1.0f);

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

void DualDelayLine::processSample(float input, float& output)
{
    if (std::abs(mTargetDelayTime - mCurrentDelayTime) > 0.001f) {
        mStabilityCounter++;

        if (mStabilityCounter >= mStabilityThreshold && mCrossfadeState == STABLE) {
            startCrossfade();
        }
    } else {
        mStabilityCounter = 0;
    }

    updateCrossfade();

    float outputA = processDelayLine(mBufferA, mWriteIndexA, mStateA, input);
    float outputB = processDelayLine(mBufferB, mWriteIndexB, mStateB, input);

    if (mCrossfadeState == STABLE) {
        output = mUsingLineA ? outputA : outputB;
    } else {
        output = (outputA * mCrossfadeGainA) + (outputB * mCrossfadeGainB);
    }
}

void DualDelayLine::reset()
{
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
    updateAllpassCoeff(mStateA);

    mStateB = mStateA;
}


STKDelayLine::STKDelayLine()
: mBufferSize(0)
, mWriteIndex(0)
, mReadIndex(0)
, mSampleRate(44100.0)
, mDelayInSamples(0.5f) // STK minimum delay
, mAllpassCoeff(0.0f)
, mApInput(0.0f)
, mLastOutput(0.0f)
, mDoNextOut(true)
, mNextOutput(0.0f)
{
}

STKDelayLine::~STKDelayLine()
{
}

void STKDelayLine::initialize(double sampleRate, double maxDelaySeconds)
{
    mSampleRate = sampleRate;
    mBufferSize = static_cast<int>(maxDelaySeconds * sampleRate) + 1;
    mBuffer.resize(mBufferSize, 0.0f);
    mWriteIndex = 0;
    mReadIndex = 0;

    mDelayInSamples = 0.5f;
    updateAllpassCoeff();

    mApInput = 0.0f;
    mLastOutput = 0.0f;
    mDoNextOut = true;
    mNextOutput = 0.0f;
}

void STKDelayLine::setDelayTime(float delayTimeSeconds)
{
    float delaySamples = delayTimeSeconds * static_cast<float>(mSampleRate);
    float maxDelaySamples = static_cast<float>(mBufferSize - 1);

    mDelayInSamples = std::max(0.5f, std::min(delaySamples, maxDelaySamples));
    updateAllpassCoeff();
}

void STKDelayLine::updateAllpassCoeff()
{
    float integerPart = floorf(mDelayInSamples);
    float fracPart = mDelayInSamples - integerPart;

    if (fracPart < 0.5f) {
        fracPart = 0.5f;
    }

    mAllpassCoeff = (1.0f - fracPart) / (1.0f + fracPart);
}

float STKDelayLine::nextOut()
{
    if (mDoNextOut) {
        mNextOutput = -mAllpassCoeff * mLastOutput;
        mNextOutput += mApInput + (mAllpassCoeff * mBuffer[mReadIndex]);
        mDoNextOut = false;
    }
    return mNextOutput;
}

void STKDelayLine::processSample(float input, float& output)
{
    int integerDelay = static_cast<int>(floorf(mDelayInSamples));

    mReadIndex = mWriteIndex - integerDelay;
    if (mReadIndex < 0) mReadIndex += mBufferSize;
    mReadIndex %= mBufferSize;

    mBuffer[mWriteIndex] = input;

    output = nextOut();
    mLastOutput = output;
    mDoNextOut = true;
    mApInput = mBuffer[mReadIndex];

    mWriteIndex = (mWriteIndex + 1) % mBufferSize;
}

void STKDelayLine::reset()
{
    std::fill(mBuffer.begin(), mBuffer.end(), 0.0f);
    mWriteIndex = 0;
    mReadIndex = 0;
    mDelayInSamples = 0.5f;
    updateAllpassCoeff();
    mApInput = 0.0f;
    mLastOutput = 0.0f;
    mDoNextOut = true;
    mNextOutput = 0.0f;
}

const char* RoutingManager::sRouteModeTexts[3] = {
    "Delay>Comb",
    "Comb>Delay",
    "Delay+Comb"
};

RoutingManager::RoutingManager()
: mRouteMode(DelayToComb)
, mPendingRouteMode(DelayToComb)
, mTransitionInProgress(false)
, mSampleRate(44100.0)
, mTransitionSamples(0)
, mTransitionCounter(0)
{
}

RoutingManager::~RoutingManager()
{
}

void RoutingManager::initialize(double sampleRate)
{
    mSampleRate = sampleRate;

    mTransitionSamples = static_cast<int>(sampleRate * 0.01);

    reset();
}

void RoutingManager::setRouteMode(RouteMode mode)
{
    if (mode != mRouteMode) {
        startTransition(mode);
    }
}

bool RoutingManager::isValidRouting() const
{
    return (mRouteMode >= DelayToComb && mRouteMode <= DelayPlusComb) &&
           (mPendingRouteMode >= DelayToComb && mPendingRouteMode <= DelayPlusComb);
}

void RoutingManager::processRouteTransition()
{
    if (!mTransitionInProgress) {
        return;
    }

    mTransitionCounter = (mTransitionCounter < mTransitionSamples)
        ? mTransitionCounter + 1
        : mTransitionSamples;

    if (mTransitionCounter >= mTransitionSamples) {
        if (mPendingRouteMode >= DelayToComb && mPendingRouteMode <= DelayPlusComb) {
            completeTransition();
        } else {
            mPendingRouteMode = DelayToComb;
            mRouteMode = DelayToComb;
            mTransitionInProgress = false;
            mTransitionCounter = 0;
        }
    }
}

const char* RoutingManager::getRouteModeText() const
{
    if (mRouteMode >= DelayToComb && mRouteMode <= DelayPlusComb) {
        return sRouteModeTexts[mRouteMode];
    }
    return "Unknown";
}

void RoutingManager::reset()
{
    mRouteMode = DelayToComb;
    mPendingRouteMode = DelayToComb;
    mTransitionInProgress = false;
    mTransitionCounter = 0;
}

void RoutingManager::startTransition(RouteMode newMode)
{
    mPendingRouteMode = newMode;
    mTransitionInProgress = true;
    mTransitionCounter = 0;
}

void RoutingManager::completeTransition()
{
    mRouteMode = mPendingRouteMode;
    mTransitionInProgress = false;
    mTransitionCounter = 0;
}


WaterStickProcessor::WaterStickProcessor()
: mInputGain(1.0f)
, mOutputGain(1.0f)
, mDelayTime(0.1f)
, mDryWet(0.5f)
, mFeedback(0.0f)
, mTempoSyncMode(false)
, mSyncDivision(kSync_1_4)
, mGrid(kGrid_4)
, mRouteMode(DelayToComb)
, mGlobalDryWet(0.5f)
, mDelayDryWet(1.0f)
, mDelayBypass(false)
, mCombBypass(false)
, mCombSize(0.1f)
, mCombFeedback(0.0f)
, mCombPitchCV(0.0f)
, mCombTaps(16)
, mCombSync(false)
, mCombDivision(kSync_1_4)
, mCombPattern(0)       // Default pattern 1
, mCombSlope(0)         // Default flat slope
, mCombGain(1.0f)       // Default 0dB gain (unity)
, mDelayBypassPrevious(false)
, mCombBypassPrevious(false)
, mDelayFadingOut(false)
, mDelayFadingIn(false)
, mCombFadingOut(false)
, mCombFadingIn(false)
, mDelayFadeRemaining(0)
, mDelayFadeTotalLength(0)
, mCombFadeRemaining(0)
, mCombFadeTotalLength(0)
, mDelayFadeGain(1.0f)
, mCombFadeGain(1.0f)
, mSampleRate(44100.0)
{
    for (int i = 0; i < 16; i++) {
        mTapEnabled[i] = false;
        mTapEnabledPrevious[i] = false;
        mTapLevel[i] = 0.8f;
        mTapPan[i] = 0.5f;

        mTapFilterCutoff[i] = 1000.0f;
        mTapFilterResonance[i] = 0.0f;
        mTapFilterType[i] = kFilterType_Bypass;

        mTapFadingOut[i] = false;
        mTapFadeOutRemaining[i] = 0;
        mTapFadeOutTotalLength[i] = 0;

        mTapFadingIn[i] = false;
        mTapFadeInRemaining[i] = 0;
        mTapFadeInTotalLength[i] = 0;

        mTapFadeGain[i] = 1.0f;
    }

    mFeedbackBufferL = 0.0f;
    mFeedbackBufferR = 0.0f;

    setControllerClass(kWaterStickControllerUID);
}

WaterStickProcessor::~WaterStickProcessor()
{
}

void WaterStickProcessor::checkTapStateChangesAndClearBuffers()
{
    for (int i = 0; i < 16; i++) {
        // Check if tap went from enabled to disabled
        if (mTapEnabledPrevious[i] && !mTapEnabled[i]) {
            // Start fade-out instead of immediate cut
            mTapFadingOut[i] = true;
            mTapFadingIn[i] = false;  // Stop any fade-in
            mTapFadeGain[i] = 1.0f;   // Start at full gain

            // Calculate fade-out length proportional to delay time (but capped)
            float tapDelayTime = mTapDistribution.getTapDelayTime(i);
            int fadeLength = static_cast<int>(tapDelayTime * mSampleRate * 0.01f); // 1% of delay time
            fadeLength = std::max(64, std::min(fadeLength, 2048)); // Cap between 64-2048 samples
            mTapFadeOutRemaining[i] = fadeLength;
            mTapFadeOutTotalLength[i] = fadeLength;
        }
        // Check if tap went from disabled to enabled
        else if (!mTapEnabledPrevious[i] && mTapEnabled[i]) {
            // Stop any ongoing fade-out and start fade-in
            mTapFadingOut[i] = false;
            mTapFadingIn[i] = true;
            mTapFadeGain[i] = 0.0f;   // Start at zero gain

            // Clear buffers for clean start
            mTapDelayLinesL[i].reset();
            mTapDelayLinesR[i].reset();

            // Calculate fade-in length - much shorter than fade-out (0.25% of delay time)
            float tapDelayTime = mTapDistribution.getTapDelayTime(i);
            int fadeLength = static_cast<int>(tapDelayTime * mSampleRate * 0.0025f); // 0.25% of delay time
            fadeLength = std::max(16, std::min(fadeLength, 512)); // Cap between 16-512 samples (0.3ms-11.6ms)
            mTapFadeInRemaining[i] = fadeLength;
            mTapFadeInTotalLength[i] = fadeLength;
        }

        // Update previous state for next call
        mTapEnabledPrevious[i] = mTapEnabled[i];
    }
}

void WaterStickProcessor::checkBypassStateChanges()
{
    const float MIN_SAMPLE_RATE = 8000.0f;  // Minimum reasonable sample rate
    const float MAX_SAMPLE_RATE = 192000.0f;  // Maximum reasonable sample rate

    // Validate sample rate before processing
    if (mSampleRate < MIN_SAMPLE_RATE || mSampleRate > MAX_SAMPLE_RATE) {
        // Reset to default if sample rate is unreasonable
        mSampleRate = 44100.0f;
    }

    // Check delay bypass state change with enhanced safety
    if (mDelayBypassPrevious != mDelayBypass) {
        // Prevent multiple simultaneous fade states
        if (!mDelayFadingOut && !mDelayFadingIn) {
            if (!mDelayBypassPrevious && mDelayBypass) {
                // Delay section was enabled, now bypassed - start fade-out
                mDelayFadingOut = true;
                mDelayFadingIn = false;
                mDelayFadeGain = 1.0f;

                // Adaptive fade length based on sample rate (10ms)
                int fadeLength = static_cast<int>(std::max(64.0f, std::min(static_cast<float>(mSampleRate * 0.01f), 2048.0f)));
                mDelayFadeRemaining = fadeLength;
                mDelayFadeTotalLength = fadeLength;
            }
            else if (mDelayBypassPrevious && !mDelayBypass) {
                // Delay section was bypassed, now enabled - start fade-in
                mDelayFadingOut = false;
                mDelayFadingIn = true;
                mDelayFadeGain = 0.0f;

                // Adaptive fade length (5ms)
                int fadeLength = static_cast<int>(std::max(32.0f, std::min(static_cast<float>(mSampleRate * 0.005f), 1024.0f)));
                mDelayFadeRemaining = fadeLength;
                mDelayFadeTotalLength = fadeLength;
            }
        }
        mDelayBypassPrevious = mDelayBypass;
    }

    // Check comb bypass state change with enhanced safety
    if (mCombBypassPrevious != mCombBypass) {
        // Prevent multiple simultaneous fade states
        if (!mCombFadingOut && !mCombFadingIn) {
            if (!mCombBypassPrevious && mCombBypass) {
                // Comb section was enabled, now bypassed - start fade-out
                mCombFadingOut = true;
                mCombFadingIn = false;
                mCombFadeGain = 1.0f;

                // Adaptive fade length based on sample rate (10ms)
                int fadeLength = static_cast<int>(std::max(64.0f, std::min(static_cast<float>(mSampleRate * 0.01f), 2048.0f)));
                mCombFadeRemaining = fadeLength;
                mCombFadeTotalLength = fadeLength;
            }
            else if (mCombBypassPrevious && !mCombBypass) {
                // Comb section was bypassed, now enabled - start fade-in
                mCombFadingOut = false;
                mCombFadingIn = true;
                mCombFadeGain = 0.0f;

                // Clear comb processor buffers for clean start with safety check
                // Directly call reset on mCombProcessor
                mCombProcessor.reset();

                // Adaptive fade length (5ms)
                int fadeLength = static_cast<int>(std::max(32.0f, std::min(static_cast<float>(mSampleRate * 0.005f), 1024.0f)));
                mCombFadeRemaining = fadeLength;
                mCombFadeTotalLength = fadeLength;
            }
        }
        mCombBypassPrevious = mCombBypass;
    }
}

void WaterStickProcessor::processDelaySection(float inputL, float inputR, float& outputL, float& outputR)
{
    // Process through all 16 tap delay lines (existing delay processing logic)
    float sumL = 0.0f;
    float sumR = 0.0f;

    for (int tap = 0; tap < NUM_TAPS; tap++) {
        bool processTap = mTapDistribution.isTapEnabled(tap) || mTapFadingOut[tap] || mTapFadingIn[tap];

        if (processTap) {
            // Process through both L and R delay lines for this tap
            float tapOutputL, tapOutputR;
            mTapDelayLinesL[tap].processSample(inputL, tapOutputL);
            mTapDelayLinesR[tap].processSample(inputR, tapOutputR);

            // Apply tap level
            float tapLevel = mTapDistribution.getTapLevel(tap);
            tapOutputL *= tapLevel;
            tapOutputR *= tapLevel;

            // Apply per-tap filter
            tapOutputL = static_cast<float>(mTapFiltersL[tap].process(tapOutputL));
            tapOutputR = static_cast<float>(mTapFiltersR[tap].process(tapOutputR));

            // Apply fade-out if in progress
            if (mTapFadingOut[tap]) {
                tapOutputL *= mTapFadeGain[tap];
                tapOutputR *= mTapFadeGain[tap];

                // Update fade-out state
                mTapFadeOutRemaining[tap]--;
                if (mTapFadeOutRemaining[tap] <= 0) {
                    // Fade-out complete - clear buffers and stop fading
                    mTapFadingOut[tap] = false;
                    mTapFadeGain[tap] = 1.0f;
                    mTapDelayLinesL[tap].reset();
                    mTapDelayLinesR[tap].reset();
                } else {
                    // Calculate exponential fade-out (1.0 to 0.0)
                    float fadeProgress = 1.0f - (static_cast<float>(mTapFadeOutRemaining[tap]) / static_cast<float>(mTapFadeOutTotalLength[tap]));
                    mTapFadeGain[tap] = std::exp(-6.0f * fadeProgress); // -60dB fade curve
                }
            }
            // Apply fade-in if in progress
            else if (mTapFadingIn[tap]) {
                tapOutputL *= mTapFadeGain[tap];
                tapOutputR *= mTapFadeGain[tap];

                // Update fade-in state
                mTapFadeInRemaining[tap]--;
                if (mTapFadeInRemaining[tap] <= 0) {
                    // Fade-in complete - set to full gain and stop fading
                    mTapFadingIn[tap] = false;
                    mTapFadeGain[tap] = 1.0f;
                } else {
                    // Calculate exponential fade-in (0.0 to 1.0)
                    float fadeProgress = 1.0f - (static_cast<float>(mTapFadeInRemaining[tap]) / static_cast<float>(mTapFadeInTotalLength[tap]));
                    mTapFadeGain[tap] = 1.0f - std::exp(-6.0f * fadeProgress); // Inverse exponential for fade-in
                }
            }

            // Apply stereo panning (0.0 = full left, 0.5 = center, 1.0 = full right)
            float pan = mTapDistribution.getTapPan(tap);
            float leftGain = 1.0f - pan;   // Left channel gain
            float rightGain = pan;         // Right channel gain

            // Pan the tap output and add to sum
            sumL += (tapOutputL * leftGain) + (tapOutputR * leftGain);
            sumR += (tapOutputL * rightGain) + (tapOutputR * rightGain);
        }
    }

    // Update feedback buffers with wet signal for next sample
    mFeedbackBufferL = sumL;
    mFeedbackBufferR = sumR;

    // Apply delay section dry/wet mix
    float delayDryGain = 1.0f - mDelayDryWet;
    float delayWetGain = mDelayDryWet;
    outputL = (inputL * delayDryGain) + (sumL * delayWetGain);
    outputR = (inputR * delayDryGain) + (sumR * delayWetGain);

    // Apply delay section bypass fade
    if (mDelayFadingOut) {
        outputL *= mDelayFadeGain;
        outputR *= mDelayFadeGain;

        // Update delay fade-out state
        mDelayFadeRemaining--;
        if (mDelayFadeRemaining <= 0) {
            mDelayFadingOut = false;
            mDelayFadeGain = 1.0f;
        } else {
            float fadeProgress = 1.0f - (static_cast<float>(mDelayFadeRemaining) / static_cast<float>(mDelayFadeTotalLength));
            mDelayFadeGain = std::exp(-6.0f * fadeProgress);
        }
    }
    else if (mDelayFadingIn) {
        outputL *= mDelayFadeGain;
        outputR *= mDelayFadeGain;

        // Update delay fade-in state
        mDelayFadeRemaining--;
        if (mDelayFadeRemaining <= 0) {
            mDelayFadingIn = false;
            mDelayFadeGain = 1.0f;
        } else {
            float fadeProgress = 1.0f - (static_cast<float>(mDelayFadeRemaining) / static_cast<float>(mDelayFadeTotalLength));
            mDelayFadeGain = 1.0f - std::exp(-6.0f * fadeProgress);
        }
    }

    // If delay is bypassed and not fading, pass through input
    if (mDelayBypass && !mDelayFadingOut && !mDelayFadingIn) {
        outputL = inputL;
        outputR = inputR;
    }
}

void WaterStickProcessor::processCombSection(float inputL, float inputR, float& outputL, float& outputR)
{
    // Process through comb processor (always 100% wet)
    mCombProcessor.processStereo(inputL, inputR, outputL, outputR);

    // Apply comb section bypass fade
    if (mCombFadingOut) {
        outputL *= mCombFadeGain;
        outputR *= mCombFadeGain;

        // Update comb fade-out state
        mCombFadeRemaining--;
        if (mCombFadeRemaining <= 0) {
            mCombFadingOut = false;
            mCombFadeGain = 1.0f;
        } else {
            float fadeProgress = 1.0f - (static_cast<float>(mCombFadeRemaining) / static_cast<float>(mCombFadeTotalLength));
            mCombFadeGain = std::exp(-6.0f * fadeProgress);
        }
    }
    else if (mCombFadingIn) {
        outputL *= mCombFadeGain;
        outputR *= mCombFadeGain;

        // Update comb fade-in state
        mCombFadeRemaining--;
        if (mCombFadeRemaining <= 0) {
            mCombFadingIn = false;
            mCombFadeGain = 1.0f;
        } else {
            float fadeProgress = 1.0f - (static_cast<float>(mCombFadeRemaining) / static_cast<float>(mCombFadeTotalLength));
            mCombFadeGain = 1.0f - std::exp(-6.0f * fadeProgress);
        }
    }

    // If comb is bypassed and not fading, pass through input
    if (mCombBypass && !mCombFadingOut && !mCombFadingIn) {
        outputL = inputL;
        outputR = inputR;
    }
}

tresult PLUGIN_API WaterStickProcessor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk)
    {
        return result;
    }

    // Add audio input/output busses
    addAudioInput(STR16("Stereo In"), Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::terminate()
{
    return AudioEffect::terminate();
}

tresult PLUGIN_API WaterStickProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
{
    mSampleRate = newSetup.sampleRate;

    // Initialize delay lines with 2 second max delay
    mDelayLineL.initialize(mSampleRate, 2.0);
    mDelayLineR.initialize(mSampleRate, 2.0);

    // Initialize all 16 tap delay lines with larger buffer for longer delays
    // Need longer delays since tap 16 at grid 1 could be 16 beats long
    double maxDelayTime = 20.0; // 20 seconds should handle very long delays
    for (int i = 0; i < NUM_TAPS; i++) {
        mTapDelayLinesL[i].initialize(mSampleRate, maxDelayTime);
        mTapDelayLinesR[i].initialize(mSampleRate, maxDelayTime);
    }

    // Initialize tempo sync
    mTempoSync.initialize(mSampleRate);

    // Initialize tap distribution
    mTapDistribution.initialize(mSampleRate);

    // Initialize per-tap filters
    for (int i = 0; i < NUM_TAPS; i++) {
        mTapFiltersL[i].setSampleRate(mSampleRate);
        mTapFiltersR[i].setSampleRate(mSampleRate);
    }

    // Initialize routing manager
    mRoutingManager.initialize(mSampleRate);

    // Initialize comb processor
    mCombProcessor.initialize(mSampleRate, maxDelayTime);

    return AudioEffect::setupProcessing(newSetup);
}


void WaterStickProcessor::updateParameters()
{
    // Update tempo sync settings
    mTempoSync.setMode(mTempoSyncMode);
    mTempoSync.setSyncDivision(mSyncDivision);
    mTempoSync.setFreeTime(mDelayTime);

    // Update tap distribution
    mTapDistribution.setGrid(mGrid);
    mTapDistribution.updateTempo(mTempoSync);

    // Update per-tap settings
    for (int i = 0; i < 16; i++) {
        mTapDistribution.setTapEnable(i, mTapEnabled[i]);
        mTapDistribution.setTapLevel(i, mTapLevel[i]);
        mTapDistribution.setTapPan(i, mTapPan[i]);
    }

    // Update all tap delay times based on current grid and tempo settings
    for (int i = 0; i < NUM_TAPS; i++) {
        float tapDelayTime = mTapDistribution.getTapDelayTime(i);
        mTapDelayLinesL[i].setDelayTime(tapDelayTime);
        mTapDelayLinesR[i].setDelayTime(tapDelayTime);
    }

    // Only update legacy delay lines if not in sync mode (sync mode updates continuously)
    if (!mTempoSyncMode) {
        float finalDelayTime = mTempoSync.getDelayTime();
        mDelayLineL.setDelayTime(finalDelayTime);
        mDelayLineR.setDelayTime(finalDelayTime);
    }

    // Update per-tap filter parameters
    for (int i = 0; i < NUM_TAPS; i++) {
        mTapFiltersL[i].setParameters(mTapFilterCutoff[i], mTapFilterResonance[i], mTapFilterType[i]);
        mTapFiltersR[i].setParameters(mTapFilterCutoff[i], mTapFilterResonance[i], mTapFilterType[i]);
    }

    // Update routing manager
    mRoutingManager.setRouteMode(mRouteMode);

    // Update comb processor with available parameters
    mCombProcessor.setFeedback(mCombFeedback);  // Use comb-specific feedback
    mCombProcessor.setSize(mCombSize);          // Use actual comb size parameter
}

tresult PLUGIN_API WaterStickProcessor::process(Vst::ProcessData& data)
{
    // Update tempo info from host
    if (data.processContext && data.processContext->state & Vst::ProcessContext::kTempoValid)
    {
        double hostTempo = data.processContext->tempo;
        mTempoSync.updateTempo(hostTempo, true);
        mCombProcessor.updateTempo(hostTempo, true); // Update comb processor tempo
    }
    else
    {
        mTempoSync.updateTempo(120.0, false); // Fallback tempo
        mCombProcessor.updateTempo(120.0, false); // Update comb processor tempo
    }

    // Process parameter changes
    if (data.inputParameterChanges)
    {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < numParamsChanged; i++)
        {
            Vst::IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData(i);
            if (paramQueue)
            {
                Vst::ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();

                if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                {
                    switch (paramQueue->getParameterId())
                    {
                        case kInputGain:
                            mInputGain = ParameterConverter::convertGain(value);
                            break;
                        case kOutputGain:
                            mOutputGain = ParameterConverter::convertGain(value);
                            break;
                        case kDelayTime:
                            mDelayTime = static_cast<float>(value * 2.0); // 0-2 seconds
                            break;
                        case kDryWet:
                            mDryWet = static_cast<float>(value); // 0-1 (dry to wet)
                            break;
                        case kFeedback:
                            mFeedback = ParameterConverter::convertFeedback(value);
                            break;
                        case kTempoSyncMode:
                            mTempoSyncMode = value > 0.5; // Toggle: >0.5 = synced
                            break;
                        case kSyncDivision:
                            mSyncDivision = static_cast<int>(value * (kNumSyncDivisions - 1) + 0.5); // Round to nearest
                            break;
                        case kGrid:
                            mGrid = static_cast<int>(value * (kNumGridValues - 1) + 0.5);
                            break;
                        // Routing and Wet/Dry controls
                        case kRouteMode:
                            mRouteMode = static_cast<RouteMode>(static_cast<int>(value * 2.0 + 0.5)); // 0-2 routing modes
                            break;
                        case kGlobalDryWet:
                            mGlobalDryWet = static_cast<float>(value); // 0-1 (dry to wet)
                            break;
                        case kDelayDryWet:
                            mDelayDryWet = static_cast<float>(value); // 0-1 (dry to wet)
                            break;
                        case kDelayBypass:
                            mDelayBypass = value > 0.5; // Toggle: >0.5 = bypassed
                            break;
                        case kCombBypass:
                            mCombBypass = value > 0.5; // Toggle: >0.5 = bypassed
                            break;
                        case kCombSize:
                            mCombSize = ParameterConverter::convertCombSize(value);
                            mCombProcessor.setSize(mCombSize);
                            break;
                        case kCombFeedback:
                            mCombFeedback = ParameterConverter::convertCombFeedback(value);
                            mCombProcessor.setFeedback(mCombFeedback);
                            break;
                        case kCombPitchCV:
                            mCombPitchCV = ParameterConverter::convertCombPitchCV(value);
                            mCombProcessor.setPitchCV(mCombPitchCV);
                            break;
                        case kCombTaps:
                            mCombTaps = ParameterConverter::convertCombTaps(value);
                            mCombProcessor.setNumTaps(mCombTaps);
                            break;
                        case kCombSync:
                            mCombSync = value > 0.5;
                            mCombProcessor.setSyncMode(mCombSync);
                            break;
                        case kCombDivision:
                            mCombDivision = static_cast<int>(value * (kNumSyncDivisions - 1) + 0.5);
                            mCombProcessor.setClockDivision(mCombDivision);
                            break;
                        case kCombPattern:
                            mCombPattern = static_cast<int>(value * (kNumCombPatterns - 1) + 0.5);
                            mCombProcessor.setPattern(mCombPattern);
                            break;
                        case kCombSlope:
                            mCombSlope = static_cast<int>(value * (kNumCombSlopes - 1) + 0.5);
                            mCombProcessor.setSlope(mCombSlope);
                            break;
                        case kCombGain:
                            {
                                // Convert normalized value to dB range (-40dB to +12dB), then to linear gain
                                float gain_dB = (value * 52.0f) - 40.0f;
                                mCombGain = std::pow(10.0f, gain_dB / 20.0f);  // Convert dB to linear gain
                                mCombProcessor.setGain(mCombGain);
                            }
                            break;
                        default:
                            TapParameterProcessor::processTapParameter(paramQueue->getParameterId(), value, this);
                            break;
                    }
                }
            }
        }
    }

    updateParameters();

    // Check for tap state changes and clear buffers if needed
    checkTapStateChangesAndClearBuffers();

    // Check for bypass state changes and handle fades
    checkBypassStateChanges();

    // Update tempo sync delay time every process cycle (tempo can change without parameter changes)
    if (mTempoSyncMode) {
        // Update tap distribution with current tempo
        mTapDistribution.updateTempo(mTempoSync);

        // Update all tap delay times
        for (int i = 0; i < NUM_TAPS; i++) {
            float tapDelayTime = mTapDistribution.getTapDelayTime(i);
            mTapDelayLinesL[i].setDelayTime(tapDelayTime);
            mTapDelayLinesR[i].setDelayTime(tapDelayTime);
        }

        // Update legacy delay lines too
        float finalDelayTime = mTempoSync.getDelayTime();
        mDelayLineL.setDelayTime(finalDelayTime);
        mDelayLineR.setDelayTime(finalDelayTime);
    }

    // Check for valid input/output
    if (data.numInputs == 0 || data.numOutputs == 0)
    {
        return kResultOk;
    }

    Vst::AudioBusBuffers* input = data.inputs;
    Vst::AudioBusBuffers* output = data.outputs;

    // Ensure we have stereo input/output
    if (input->numChannels < 2 || output->numChannels < 2)
    {
        return kResultOk;
    }

    float* inputL = input->channelBuffers32[0];
    float* inputR = input->channelBuffers32[1];
    float* outputL = output->channelBuffers32[0];
    float* outputR = output->channelBuffers32[1];

    // Process routing transitions
    mRoutingManager.processRouteTransition();

    // Process samples
    for (int32 sample = 0; sample < data.numSamples; sample++)
    {
        // Get input samples
        float inL = inputL[sample];
        float inR = inputR[sample];

        // Apply input gain and feedback with tanh limiting
        float inputWithFeedbackL = inL + (mFeedbackBufferL * mFeedback);
        float inputWithFeedbackR = inR + (mFeedbackBufferR * mFeedback);

        // Apply tanh saturation to prevent runaway feedback
        inputWithFeedbackL = std::tanh(inputWithFeedbackL);
        inputWithFeedbackR = std::tanh(inputWithFeedbackR);

        float gainedL = inputWithFeedbackL * mInputGain;
        float gainedR = inputWithFeedbackR * mInputGain;

        // Route processing based on current mode
        float delayOutputL = 0.0f, delayOutputR = 0.0f;
        float combOutputL = 0.0f, combOutputR = 0.0f;
        float finalOutputL = 0.0f, finalOutputR = 0.0f;

        switch (mRoutingManager.getRouteMode()) {
            case DelayToComb:
            {
                // Delay feeds into Comb
                processDelaySection(gainedL, gainedR, delayOutputL, delayOutputR);
                processCombSection(delayOutputL, delayOutputR, combOutputL, combOutputR);
                finalOutputL = combOutputL;
                finalOutputR = combOutputR;
                break;
            }
            case CombToDelay:
            {
                // Comb feeds into Delay
                processCombSection(gainedL, gainedR, combOutputL, combOutputR);
                processDelaySection(combOutputL, combOutputR, delayOutputL, delayOutputR);
                finalOutputL = delayOutputL;
                finalOutputR = delayOutputR;
                break;
            }
            case DelayPlusComb:
            {
                // Parallel processing: both sections process input independently
                processDelaySection(gainedL, gainedR, delayOutputL, delayOutputR);
                processCombSection(gainedL, gainedR, combOutputL, combOutputR);

                // Mix parallel outputs equally
                finalOutputL = (delayOutputL + combOutputL) * 0.5f;
                finalOutputR = (delayOutputR + combOutputR) * 0.5f;
                break;
            }
        }

        // Apply global dry/wet mix
        float globalDryGain = 1.0f - mGlobalDryWet;
        float globalWetGain = mGlobalDryWet;
        float mixedL = (inL * globalDryGain) + (finalOutputL * globalWetGain);
        float mixedR = (inR * globalDryGain) + (finalOutputR * globalWetGain);

        // Apply output gain and write to output
        outputL[sample] = mixedL * mOutputGain;
        outputR[sample] = mixedR * mOutputGain;
    }

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::getState(IBStream* state)
{
    IBStreamer streamer(state, kLittleEndian);

    // Write current state version as first 4 bytes
    streamer.writeInt32(kStateVersionCurrent);

    // Write magic number signature for freshness detection
    streamer.writeInt32(kStateMagicNumber);

    // Write all parameters in version 1 format
    streamer.writeFloat(mInputGain);
    streamer.writeFloat(mOutputGain);
    streamer.writeFloat(mDelayTime);
    streamer.writeFloat(mDryWet);
    streamer.writeFloat(mFeedback);
    streamer.writeBool(mTempoSyncMode);
    streamer.writeInt32(mSyncDivision);
    streamer.writeInt32(mGrid);

    // Save routing and wet/dry parameters
    streamer.writeInt32(static_cast<int32>(mRouteMode));
    streamer.writeFloat(mGlobalDryWet);
    streamer.writeFloat(mDelayDryWet);
    streamer.writeBool(mDelayBypass);
    streamer.writeBool(mCombBypass);

    // Save comb control parameters
    streamer.writeFloat(mCombSize);
    streamer.writeFloat(mCombFeedback);
    streamer.writeFloat(mCombPitchCV);
    streamer.writeInt32(mCombTaps);
    streamer.writeBool(mCombSync);
    streamer.writeInt32(mCombDivision);

    // Save all tap parameters
    for (int i = 0; i < 16; i++) {
        streamer.writeBool(mTapEnabled[i]);
        streamer.writeFloat(mTapLevel[i]);
        streamer.writeFloat(mTapPan[i]);
        // Save per-tap filter parameters
        streamer.writeFloat(mTapFilterCutoff[i]);
        streamer.writeFloat(mTapFilterResonance[i]);
        streamer.writeInt32(mTapFilterType[i]);
    }

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::setState(IBStream* state)
{
    if (!state) {
        return kResultOk;
    }

    // Try to read state version first
    IBStreamer streamer(state, kLittleEndian);
    Steinberg::int32 stateVersion;

    if (streamer.readInt32(stateVersion)) {
        // Check if this looks like a valid version
        if (stateVersion >= kStateVersionLegacy && stateVersion <= kStateVersionCurrent) {
            // This is a versioned state
            return readVersionedProcessorState(state, stateVersion);
        } else {
            // This is probably the first float from legacy state
            // Reset stream and read as legacy
            state->seek(0, IBStream::kIBSeekSet, nullptr);
            return readLegacyProcessorState(state);
        }
    } else {
        // Failed to read anything, treat as empty state
        return kResultOk;
    }
}

tresult WaterStickProcessor::readLegacyProcessorState(IBStream* state)
{
    IBStreamer streamer(state, kLittleEndian);

    streamer.readFloat(mInputGain);
    streamer.readFloat(mOutputGain);
    streamer.readFloat(mDelayTime);
    streamer.readFloat(mDryWet);
    streamer.readFloat(mFeedback);
    streamer.readBool(mTempoSyncMode);
    streamer.readInt32(mSyncDivision);
    streamer.readInt32(mGrid);

    // Load routing and wet/dry parameters
    int32 routeModeInt;
    streamer.readInt32(routeModeInt);
    mRouteMode = static_cast<RouteMode>(routeModeInt);
    streamer.readFloat(mGlobalDryWet);
    streamer.readFloat(mDelayDryWet);
    streamer.readBool(mDelayBypass);
    streamer.readBool(mCombBypass);

    // Load comb control parameters (with defaults for older state versions)
    if (!streamer.readFloat(mCombSize)) mCombSize = 0.1f;
    if (!streamer.readFloat(mCombFeedback)) mCombFeedback = 0.0f;
    if (!streamer.readFloat(mCombPitchCV)) mCombPitchCV = 0.0f;
    if (!streamer.readInt32(mCombTaps)) mCombTaps = 16;
    if (!streamer.readBool(mCombSync)) mCombSync = false;
    if (!streamer.readInt32(mCombDivision)) mCombDivision = kSync_1_4;

    // Apply loaded comb parameters to processor
    mCombProcessor.setSize(mCombSize);
    mCombProcessor.setFeedback(mCombFeedback);
    mCombProcessor.setPitchCV(mCombPitchCV);
    mCombProcessor.setNumTaps(mCombTaps);
    mCombProcessor.setSyncMode(mCombSync);
    mCombProcessor.setClockDivision(mCombDivision);

    // Load all tap parameters
    for (int i = 0; i < 16; i++) {
        streamer.readBool(mTapEnabled[i]);
        streamer.readFloat(mTapLevel[i]);
        streamer.readFloat(mTapPan[i]);
        // Load per-tap filter parameters
        streamer.readFloat(mTapFilterCutoff[i]);
        streamer.readFloat(mTapFilterResonance[i]);
        streamer.readInt32(mTapFilterType[i]);

        // Initialize previous state to current state to prevent unwanted buffer clears
        mTapEnabledPrevious[i] = mTapEnabled[i];
    }

    // Initialize bypass previous states to prevent unwanted fades on load
    mDelayBypassPrevious = mDelayBypass;
    mCombBypassPrevious = mCombBypass;

    return kResultOk;
}

tresult WaterStickProcessor::readVersionedProcessorState(IBStream* state, Steinberg::int32 version)
{
    switch (version) {
        case kStateVersionCurrent:
            // Version 1 state format - read signature then parameters
            return readCurrentVersionProcessorState(state);

        default:
            // Unknown version - skip state loading
            return kResultOk;
    }

    return kResultOk;
}

tresult WaterStickProcessor::readCurrentVersionProcessorState(IBStream* state)
{
    IBStreamer streamer(state, kLittleEndian);

    // Read and validate signature (version was already read)
    Steinberg::int32 signature;
    if (streamer.readInt32(signature)) {
        if (signature == kStateMagicNumber) {
            // Valid signature - continue reading state
        } else {
            // Invalid signature - this is suspicious but continue reading
            // The data structure should still be correct
        }
    } else {
        // No signature found - old v1 format, rewind
        state->seek(-4, IBStream::kIBSeekCur, nullptr);
    }

    // Read parameters using the legacy format (same structure)
    return readLegacyProcessorState(state);
}

} // namespace WaterStick