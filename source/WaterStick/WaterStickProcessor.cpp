#include "WaterStickProcessor.h"
#include "WaterStickCIDs.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include <cmath>

using namespace Steinberg;

namespace WaterStick {

// Static constant definition for PitchShiftingDelayLine
const float PitchShiftingDelayLine::PITCH_CHANGE_THRESHOLD = 0.01f;
const float PitchShiftingDelayLine::MAX_GAIN_COMPENSATION = 1.8f;

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

    static int convertPitchShift(double value) {
        // Convert 0.0-1.0 range to -12 to +12 semitones
        return static_cast<int>(round((value * 24.0) - 12.0));
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
    static const TapParameterRange kTapPitchRange;

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
            return;
        }

        if (kTapPitchRange.contains(paramId)) {
            int tapIndex, paramType;
            kTapPitchRange.getIndices(paramId, tapIndex, paramType);

            if (tapIndex < 16) {
                processor->mTapPitchShift[tapIndex] = ParameterConverter::convertPitchShift(value);
            }
        }
    }
};

const TapParameterRange TapParameterProcessor::kTapBasicRange = {kTap1Enable, kTap16Pan, 3};
const TapParameterRange TapParameterProcessor::kTapFilterRange = {kTap1FilterCutoff, kTap16FilterType, 3};
const TapParameterRange TapParameterProcessor::kTapPitchRange = {kTap1PitchShift, kTap16PitchShift, 1};

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

PitchShiftingDelayLine::PitchShiftingDelayLine()
: DualDelayLine()
, mPitchSemitones(0)
, mPitchRatio(1.0f)
, mTargetPitchRatio(1.0f)
, mSmoothingCoeff(1.0f)
, mNextGrainSample(0.0f)
, mSampleCount(0.0f)
, mAdaptiveSpacing(static_cast<float>(GRAIN_SIZE - GRAIN_OVERLAP))
, mLastPitchRatio(1.0f)
, mTransitionIntensity(0.0f)
, mTransitionDecayRate(0.0f)
, mTransitionSamples(0)
, mBufferReadinessSamples(0.0f)
, mMinBufferForUpward(0.0f)
, mUpwardPitchReady(false)
, mUpwardFadeInGain(0.0f)
, mUpwardFadeInSamples(0)
, mActiveGrainCount(0)
, mGainCompensation(1.0f)
, mTargetGainCompensation(1.0f)
, mGainSmoothingCoeff(1.0f)
, mLastPitchRatioForGain(1.0f)
{
    initializeGrains();
}

PitchShiftingDelayLine::~PitchShiftingDelayLine()
{
}

void PitchShiftingDelayLine::initialize(double sampleRate, double maxDelaySeconds)
{
    DualDelayLine::initialize(sampleRate, maxDelaySeconds);

    // Increased buffer size for upward pitch shifting lookahead
    int pitchBufferSize = mBufferSize + LOOKAHEAD_BUFFER;
    mPitchBufferA.resize(pitchBufferSize, 0.0f);
    mPitchBufferB.resize(pitchBufferSize, 0.0f);

    // Initialize buffer readiness tracking - reduced warmup time
    mBufferReadinessSamples = 0.0f;
    mMinBufferForUpward = static_cast<float>(WARMUP_BUFFER_SIZE);  // Reduced to 1/4 grain size
    mUpwardPitchReady = false;
    mUpwardFadeInGain = 0.0f;
    mUpwardFadeInSamples = 0;
    mActiveGrainCount = 1;  // Start with one grain during warmup

    initializeGrains();
    mSampleCount = 0.0f;
    mNextGrainSample = 0.0f;
    updateSmoothingCoeff();
    calculateAdaptiveSpacing();

    // Initialize gain compensation smoothing coefficient (15ms time constant at 44.1kHz)
    float timeFactor = std::max(1.0f, static_cast<float>(sampleRate) / 44100.0f);
    mGainSmoothingCoeff = exp(-1.0f / (0.015f * sampleRate));
    mGainCompensation = 1.0f;
    mTargetGainCompensation = 1.0f;
    mLastPitchRatioForGain = 1.0f;
}

void PitchShiftingDelayLine::setPitchShift(int semitones)
{
    if (mPitchSemitones != semitones) {
        mPitchSemitones = std::max(-12, std::min(12, semitones));

        // Calculate target pitch ratio
        if (mPitchSemitones == 0) {
            mTargetPitchRatio = 1.0f;
        } else {
            mTargetPitchRatio = powf(2.0f, static_cast<float>(mPitchSemitones) / 12.0f);
        }

        // Don't immediately reinitialize grains - let smoothing handle the transition
        // This prevents popping artifacts during parameter changes
    }
}

void PitchShiftingDelayLine::updatePitchRatio()
{
    if (mPitchSemitones == 0) {
        mPitchRatio = 1.0f;
        mTargetPitchRatio = 1.0f;
    } else {
        mTargetPitchRatio = powf(2.0f, static_cast<float>(mPitchSemitones) / 12.0f);
    }
}

void PitchShiftingDelayLine::updateSmoothingCoeff()
{
    // 15ms smoothing time constant - optimal for click elimination + responsiveness
    const float timeConstantMs = 15.0f;
    const float timeConstantSec = timeConstantMs / 1000.0f;

    // Coefficient calculation: α = exp(-1/(timeConstant × sampleRate))
    // This gives us a first-order IIR filter: y[n] = α·y[n-1] + (1-α)·x[n]
    mSmoothingCoeff = expf(-1.0f / (timeConstantSec * static_cast<float>(mSampleRate)));
}

void PitchShiftingDelayLine::updatePitchRatioSmoothing()
{
    // First-order IIR exponential smoothing
    // y[n] = α·y[n-1] + (1-α)·x[n]
    mPitchRatio = mSmoothingCoeff * mPitchRatio + (1.0f - mSmoothingCoeff) * mTargetPitchRatio;
}

void PitchShiftingDelayLine::updateTransitionState()
{
    // Detect significant pitch ratio changes
    float pitchDelta = fabsf(mPitchRatio - mLastPitchRatio);
    if (pitchDelta > PITCH_CHANGE_THRESHOLD) {
        // Reset transition state on parameter change
        mTransitionIntensity = 1.0f;
        mTransitionSamples = 0;
        // Calculate decay rate so that transition intensity reaches 0.01 at TRANSITION_WINDOW samples
        // Using exponential decay: intensity = exp(-decay_rate * time)
        // 0.01 = exp(-decay_rate * TRANSITION_WINDOW), so decay_rate = -ln(0.01) / TRANSITION_WINDOW
        mTransitionDecayRate = 4.605f / static_cast<float>(TRANSITION_WINDOW); // -ln(0.01) ≈ 4.605
    } else if (mTransitionSamples < TRANSITION_WINDOW) {
        // Decay transition intensity exponentially
        mTransitionSamples++;
        mTransitionIntensity = expf(-mTransitionDecayRate * static_cast<float>(mTransitionSamples));
        if (mTransitionIntensity < 0.01f) {
            mTransitionIntensity = 0.0f;
        }
    } else {
        mTransitionIntensity = 0.0f;
    }

    mLastPitchRatio = mPitchRatio;
}

void PitchShiftingDelayLine::initializeGrains()
{
    for (int i = 0; i < NUM_GRAINS; i++) {
        mGrains[i].active = false;
        mGrains[i].position = 0.0f;
        mGrains[i].phase = 0.0f;
        mGrains[i].pitchRatio = mPitchRatio;
        mGrains[i].grainStart = 0.0f;
        mGrains[i].readPosition = 0.0f;

        // Initialize fade-out state
        mGrains[i].fadingOut = false;
        mGrains[i].fadeOutRemaining = 0;
        mGrains[i].fadeOutTotal = 0;
        mGrains[i].fadeOutGain = 1.0f;

        // Initialize transition state
        mGrains[i].spawnSampleCount = 0.0f;
        mGrains[i].isTransitionGrain = false;
    }
}

float PitchShiftingDelayLine::calculateHannWindow(float phase, bool isStarting, bool isEnding, float transitionIntensity)
{
    // Base Hann window calculation
    float baseWindow = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * phase));

    // Apply enhanced windowing during parameter transitions
    if (transitionIntensity > 0.0f) {
        if (isStarting) {
            // Enhanced fade-in: Apply 4x faster attack for newly spawned grains during transitions
            float enhancedPhase = std::min(1.0f, phase * 4.0f);
            float enhancedWindow = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * enhancedPhase));

            // Blend between base and enhanced window based on transition intensity
            return baseWindow * (1.0f - transitionIntensity) + enhancedWindow * transitionIntensity;
        }
        else if (isEnding) {
            // Enhanced fade-out: Apply 4x faster release for ending grains
            float enhancedPhase = std::max(0.0f, (phase - 0.75f) * 4.0f);
            if (enhancedPhase > 0.0f) {
                float enhancedWindow = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * enhancedPhase));
                enhancedWindow = 1.0f - enhancedWindow; // Invert for fade-out

                // Blend between base and enhanced window based on transition intensity
                return baseWindow * (1.0f - transitionIntensity) + enhancedWindow * transitionIntensity;
            }
        }
    }

    return baseWindow;
}

void PitchShiftingDelayLine::startNewGrain()
{
    // Limit active grains during warmup for upward pitch shifting
    int maxGrains = (mPitchRatio > 1.0f && !mUpwardPitchReady) ? mActiveGrainCount : NUM_GRAINS;

    for (int i = 0; i < maxGrains; i++) {
        if (!mGrains[i].active) {
            mGrains[i].active = true;
            mGrains[i].position = 0.0f;
            mGrains[i].phase = 0.0f;
            mGrains[i].pitchRatio = mPitchRatio;

            // Initialize fade-out state for new grain
            mGrains[i].fadingOut = false;
            mGrains[i].fadeOutRemaining = 0;
            mGrains[i].fadeOutTotal = 0;
            mGrains[i].fadeOutGain = 1.0f;

            // Initialize transition state for new grain
            mGrains[i].spawnSampleCount = mSampleCount;
            mGrains[i].isTransitionGrain = (mTransitionIntensity > 0.0f);

            // Set grain start position correctly for pitch shifting
            if (mPitchRatio > 1.0f) {
                // For upward pitch shifting, start grain behind current position
                float startOffset = mUpwardPitchReady ? (GRAIN_SIZE / (mPitchRatio * 2.0f)) : (WARMUP_BUFFER_SIZE / 4.0f);
                mGrains[i].grainStart = std::max(0.0f, mSampleCount - startOffset);
                mGrains[i].readPosition = mGrains[i].grainStart;  // Initialize read position to grain start
            } else {
                // For downward pitch shifting or no shift, start at current position
                mGrains[i].grainStart = mSampleCount;
                mGrains[i].readPosition = mSampleCount;
            }
            break;
        }
    }
}

void PitchShiftingDelayLine::updateGrains()
{
    for (int i = 0; i < NUM_GRAINS; i++) {
        if (mGrains[i].active) {
            // Handle fade-out processing
            if (mGrains[i].fadingOut) {
                mGrains[i].fadeOutRemaining--;
                if (mGrains[i].fadeOutRemaining <= 0) {
                    // Fade-out completed, deactivate grain
                    mGrains[i].active = false;
                    mGrains[i].fadingOut = false;
                    continue;
                } else {
                    // Calculate exponential fade-out curve: gain = exp(-6.0 * progress)
                    float progress = 1.0f - (static_cast<float>(mGrains[i].fadeOutRemaining) / static_cast<float>(mGrains[i].fadeOutTotal));
                    mGrains[i].fadeOutGain = expf(-6.0f * progress);
                }
            }

            // Skip grain processing if pitch shift is disabled and not fading out
            if (mPitchSemitones == 0 && !mGrains[i].fadingOut) {
                continue;
            }

            // Use floating-point advancement for smooth pitch shifting
            mGrains[i].position += mGrains[i].pitchRatio;
            mGrains[i].phase = mGrains[i].position / static_cast<float>(GRAIN_SIZE);

            // Update read position for buffer access
            mGrains[i].readPosition += mGrains[i].pitchRatio;

            // Check for grain completion (natural end or fade-out completion)
            if (mGrains[i].phase >= 1.0f && !mGrains[i].fadingOut) {
                mGrains[i].active = false;
            }
        }
    }
}

float PitchShiftingDelayLine::processPitchShifting(float input)
{
    // Update pitch ratio smoothing first - this handles parameter changes smoothly
    updatePitchRatioSmoothing();

    // Update transition state detection for parameter-aware windowing
    updateTransitionState();

    // Update dynamic gain compensation for parameter transitions
    updateGainCompensation();

    if (mPitchSemitones == 0 && fabsf(mPitchRatio - 1.0f) < 1e-6f) {
        return input;
    }

    // Update buffer readiness tracking
    updateBufferReadiness(input);

    // Store input in pitch buffer
    std::vector<float>& currentPitchBuffer = mUsingLineA ? mPitchBufferA : mPitchBufferB;
    int bufferIndex = static_cast<int>(mSampleCount) % static_cast<int>(currentPitchBuffer.size());
    currentPitchBuffer[bufferIndex] = input;

    // Recalculate adaptive spacing with smoothed pitch ratio
    calculateAdaptiveSpacing();

    // Check if we need to start grains for the first time (transition from no pitch shift)
    bool hasActiveGrains = false;
    for (int i = 0; i < NUM_GRAINS; i++) {
        if (mGrains[i].active) {
            hasActiveGrains = true;
            break;
        }
    }

    // If pitch ratio has moved away from 1.0 and we have no active grains, initialize them
    if (!hasActiveGrains && fabsf(mPitchRatio - 1.0f) > 0.01f) {
        if (mPitchRatio <= 1.0f) {
            // Downward pitch shifting - can start immediately
            mUpwardFadeInGain = 1.0f;
            mUpwardFadeInSamples = 0;
        } else {
            // Upward pitch shifting - start with fade-in
            mUpwardFadeInGain = 0.0f;
            mUpwardFadeInSamples = UPWARD_FADE_LENGTH;
            mActiveGrainCount = 1;
        }
    }

    // If pitch ratio is very close to 1.0, initiate fade-out for grains to avoid popping
    if (hasActiveGrains && fabsf(mPitchRatio - 1.0f) < 0.001f) {
        for (int i = 0; i < NUM_GRAINS; i++) {
            if (mGrains[i].active && !mGrains[i].fadingOut) {
                // Initiate fade-out instead of immediate deactivation
                mGrains[i].fadingOut = true;
                mGrains[i].fadeOutRemaining = GRAIN_FADEOUT_LENGTH;
                mGrains[i].fadeOutTotal = GRAIN_FADEOUT_LENGTH;
                mGrains[i].fadeOutGain = 1.0f;  // Start at full volume
            }
        }
        mUpwardFadeInGain = 0.0f;
        mUpwardFadeInSamples = 0;
    }

    // Track if we're in warmup phase for upward pitch shifting
    bool isUpwardWarmup = (mPitchRatio > 1.0f && !mUpwardPitchReady);

    // Update fade-in state for upward pitch shifting
    if (mPitchRatio > 1.0f) {
        updateUpwardPitchFadeIn();
    }

    // Use adaptive grain spacing for better quality
    if (mSampleCount >= mNextGrainSample) {
        // Start grains progressively - always allow at least one grain
        if (mPitchRatio <= 1.0f || mSampleCount >= static_cast<float>(WARMUP_BUFFER_SIZE / 2)) {
            startNewGrain();
        }
        mNextGrainSample = mSampleCount + mAdaptiveSpacing;
    }

    float output = 0.0f;
    int activeGrains = 0;
    int validGrains = 0;

    for (int i = 0; i < NUM_GRAINS; i++) {
        if (mGrains[i].active) {
            activeGrains++;
            // Calculate floating-point read position with bounds checking
            float absoluteReadPos = mGrains[i].grainStart + mGrains[i].position;

            // Ensure we don't read beyond buffer bounds
            if (isBufferPositionValid(currentPitchBuffer, absoluteReadPos)) {
                validGrains++;
                float sample = interpolateBuffer(currentPitchBuffer, absoluteReadPos);

                // Determine transition windowing parameters
                bool isStarting = (mGrains[i].isTransitionGrain && mGrains[i].phase < 0.25f);
                bool isEnding = (mGrains[i].fadingOut || (mGrains[i].phase > 0.75f && mTransitionIntensity > 0.0f));
                float transitionIntensity = mTransitionIntensity;

                // Apply parameter-aware windowing
                float window = calculateHannWindow(mGrains[i].phase, isStarting, isEnding, transitionIntensity);

                // Apply fade-out gain if grain is fading out
                float finalGain = window;
                if (mGrains[i].fadingOut) {
                    finalGain *= mGrains[i].fadeOutGain;
                }

                output += sample * finalGain;
            }
        }
    }


    updateGrains();
    mSampleCount += 1.0f;

    // Normalize by active grains and apply gain compensation
    if (activeGrains > 0) {
        output /= static_cast<float>(activeGrains);

        // Compensate for energy loss in upward pitch shifting
        if (mPitchRatio > 1.0f) {
            output *= std::min(1.5f, 1.0f + (mPitchRatio - 1.0f) * 0.3f);
            // Apply fade-in gain for smooth transition
            output *= mUpwardFadeInGain;
        }

        // Apply dynamic gain compensation after grain mixing but before final output
        output *= mGainCompensation;

        // During warmup phase, blend grain output with passthrough for smooth transition
        if (isUpwardWarmup) {
            float passthroughGain = 0.3f;  // Reduced passthrough during grain processing
            return output + (input * passthroughGain);
        }

        return output;
    }

    // Fallback: return attenuated passthrough when no grains are active
    if (isUpwardWarmup) {
        return input * 0.6f;  // Attenuated passthrough during warmup
    }

    return input;
}

void PitchShiftingDelayLine::processSample(float input, float& output)
{
    if (mPitchSemitones == 0) {
        DualDelayLine::processSample(input, output);
    } else {
        float delayedInput;
        DualDelayLine::processSample(input, delayedInput);
        output = processPitchShifting(delayedInput);
    }
}

void PitchShiftingDelayLine::calculateAdaptiveSpacing()
{
    // Adapt grain spacing based on pitch ratio for consistent grain density
    if (mPitchRatio > 1.0f) {
        // For upward pitch shifting, reduce spacing to maintain grain density
        mAdaptiveSpacing = static_cast<float>(GRAIN_SIZE - GRAIN_OVERLAP) / mPitchRatio;
    } else if (mPitchRatio < 1.0f) {
        // For downward pitch shifting, increase spacing slightly
        mAdaptiveSpacing = static_cast<float>(GRAIN_SIZE - GRAIN_OVERLAP) * (2.0f - mPitchRatio);
    } else {
        // No pitch shift
        mAdaptiveSpacing = static_cast<float>(GRAIN_SIZE - GRAIN_OVERLAP);
    }

    // Ensure minimum and maximum spacing bounds
    mAdaptiveSpacing = std::max(64.0f, std::min(mAdaptiveSpacing, static_cast<float>(GRAIN_SIZE)));
}

float PitchShiftingDelayLine::interpolateBuffer(const std::vector<float>& buffer, float position) const
{
    // Linear interpolation for fractional sample positions
    int intPos = static_cast<int>(std::floor(position));
    float fracPos = position - static_cast<float>(intPos);

    // Ensure we're within buffer bounds
    int bufferSize = static_cast<int>(buffer.size());
    intPos = ((intPos % bufferSize) + bufferSize) % bufferSize;
    int nextPos = (intPos + 1) % bufferSize;

    // Linear interpolation
    return buffer[intPos] * (1.0f - fracPos) + buffer[nextPos] * fracPos;
}

bool PitchShiftingDelayLine::isBufferPositionValid(const std::vector<float>& buffer, float position) const
{
    // Check if the position is within reasonable bounds for reading
    float currentWritePos = mSampleCount;

    // For upward pitch shifting, grains need to read ahead in the buffer
    if (mPitchRatio > 1.0f) {
        if (!mUpwardPitchReady) {
            // During warmup, allow reading with sufficient lookahead for upward pitch
            float warmupReadRange = static_cast<float>(WARMUP_BUFFER_SIZE * 2);  // More generous range
            float maxValidPos = currentWritePos + static_cast<float>(LOOKAHEAD_BUFFER / 2);  // Allow reading ahead
            float minValidPos = std::max(0.0f, currentWritePos - warmupReadRange);
            return (position >= minValidPos && position <= maxValidPos);
        } else {
            // After warmup, allow full range with generous lookahead for upward pitch
            float maxValidPos = currentWritePos + static_cast<float>(LOOKAHEAD_BUFFER);  // Full lookahead buffer
            float minValidPos = std::max(0.0f, currentWritePos - static_cast<float>(buffer.size()) + static_cast<float>(LOOKAHEAD_BUFFER));
            return (position >= minValidPos && position <= maxValidPos);
        }
    } else {
        // For downward pitch shifting, use moderate lookahead
        float maxReadAhead = static_cast<float>(LOOKAHEAD_BUFFER / 4);
        float maxValidPos = currentWritePos + maxReadAhead;
        float minValidPos = currentWritePos - static_cast<float>(buffer.size()) + maxReadAhead;
        return (position >= minValidPos && position <= maxValidPos);
    }
}

void PitchShiftingDelayLine::updateBufferReadiness(float input)
{
    // Track accumulation of all audio samples for buffer readiness
    mBufferReadinessSamples += 1.0f;

    // Check if we have enough content for upward pitch shifting
    bool wasReady = mUpwardPitchReady;
    mUpwardPitchReady = (mBufferReadinessSamples >= mMinBufferForUpward);

    // Progressive grain activation during warmup
    if (mPitchRatio > 1.0f && !mUpwardPitchReady) {
        // Gradually increase active grains as buffer builds
        float bufferProgress = mBufferReadinessSamples / mMinBufferForUpward;
        mActiveGrainCount = std::max(1, static_cast<int>(bufferProgress * NUM_GRAINS));
    } else if (mUpwardPitchReady) {
        mActiveGrainCount = NUM_GRAINS;  // Full grain count when ready
    }

    // If we just became ready and have upward pitch shifting requested, enhance fade-in
    if (!wasReady && mUpwardPitchReady && mPitchRatio > 1.0f && mPitchSemitones != 0) {
        // Buffer is now ready for full upward pitch shifting
        if (mUpwardFadeInSamples <= 0) {
            // Start fade-in if not already active
            mUpwardFadeInGain = std::max(0.5f, mUpwardFadeInGain);  // Start from current or 50%
            mUpwardFadeInSamples = UPWARD_FADE_LENGTH / 2;  // Shorter fade since we're already partway
        }
    }
}

void PitchShiftingDelayLine::updateUpwardPitchFadeIn()
{
    if (mUpwardFadeInSamples > 0) {
        mUpwardFadeInSamples--;
        // Smooth fade-in curve using cosine
        float fadeLength = static_cast<float>(UPWARD_FADE_LENGTH);
        float fadeProgress = 1.0f - (static_cast<float>(mUpwardFadeInSamples) / fadeLength);
        mUpwardFadeInGain = 0.5f * (1.0f - std::cos(fadeProgress * 3.14159265f));
    } else if (mPitchRatio > 1.0f) {
        // Always provide some output for upward pitch shifting, even before full readiness
        if (mUpwardPitchReady) {
            mUpwardFadeInGain = 1.0f;
        } else {
            // Provide partial gain during warmup based on buffer progress
            float bufferProgress = mBufferReadinessSamples / mMinBufferForUpward;
            mUpwardFadeInGain = std::min(0.8f, 0.3f + (bufferProgress * 0.5f));
        }
    }
}

bool PitchShiftingDelayLine::isUpwardPitchBufferReady() const
{
    return mUpwardPitchReady;
}

void PitchShiftingDelayLine::updateGainCompensation()
{
    // Calculate base compensation based on pitch ratio deviation from 1.0
    float basePitchDeviation = fabsf(mPitchRatio - 1.0f);
    float baseCompensation = 1.0f + (basePitchDeviation * 0.15f);

    // Calculate transition compensation based on rate of pitch ratio change
    float pitchRatioChange = fabsf(mPitchRatio - mLastPitchRatioForGain);
    float transitionCompensation = 1.0f;

    if (pitchRatioChange > PITCH_CHANGE_THRESHOLD) {
        // Extra compensation during parameter transitions
        // Scale compensation based on magnitude of change and current transition intensity
        float changeIntensity = std::min(1.0f, pitchRatioChange * 10.0f);  // Scale to 0-1 range
        float transitionFactor = mTransitionIntensity * changeIntensity;
        transitionCompensation = 1.0f + (transitionFactor * 0.25f);  // Up to 25% extra during transitions
    }

    // Combine base and transition compensation
    float totalCompensation = baseCompensation * transitionCompensation;

    // Apply maximum gain limit to prevent excessive boosting
    mTargetGainCompensation = std::min(MAX_GAIN_COMPENSATION, totalCompensation);

    // Apply exponential smoothing to avoid level jumps
    mGainCompensation = mTargetGainCompensation + (mGainCompensation - mTargetGainCompensation) * mGainSmoothingCoeff;

    // Update previous pitch ratio for next frame's transition detection
    mLastPitchRatioForGain = mPitchRatio;
}

void PitchShiftingDelayLine::reset()
{
    DualDelayLine::reset();

    std::fill(mPitchBufferA.begin(), mPitchBufferA.end(), 0.0f);
    std::fill(mPitchBufferB.begin(), mPitchBufferB.end(), 0.0f);

    // Reset buffer readiness tracking
    mBufferReadinessSamples = 0.0f;
    mUpwardPitchReady = false;
    mUpwardFadeInGain = 0.0f;
    mUpwardFadeInSamples = 0;
    mActiveGrainCount = 1;  // Start with one grain

    mSampleCount = 0.0f;
    mNextGrainSample = 0.0f;
    initializeGrains();
    calculateAdaptiveSpacing();

    // Reset gain compensation to neutral state
    mGainCompensation = 1.0f;
    mTargetGainCompensation = 1.0f;
    mLastPitchRatioForGain = 1.0f;
}



WaterStickProcessor::WaterStickProcessor()
: mInputGain(1.0f)
, mOutputGain(1.0f)
, mDelayTime(0.1f)
, mFeedback(0.0f)
, mTempoSyncMode(false)
, mSyncDivision(kSync_1_4)
, mGrid(kGrid_4)
, mGlobalDryWet(0.5f)
, mDelayBypass(false)
, mDelayBypassPrevious(false)
, mDelayFadingOut(false)
, mDelayFadingIn(false)
, mDelayFadeRemaining(0)
, mDelayFadeTotalLength(0)
, mDelayFadeGain(1.0f)
, mSampleRate(44100.0)
, mLastTempoSyncDelayTime(-1.0f)
, mTempoSyncParametersChanged(false)
, mParameterHistoryWriteIndex(0)
{
    for (int i = 0; i < 16; i++) {
        mTapEnabled[i] = false;
        mTapEnabledPrevious[i] = false;
        mTapLevel[i] = 0.8f;
        mTapPan[i] = 0.5f;

        mTapFilterCutoff[i] = 1000.0f;
        mTapFilterResonance[i] = 0.0f;
        mTapFilterType[i] = kFilterType_Bypass;

        mTapPitchShift[i] = 0;  // Default to no pitch shift

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

    // Initialize parameter history with current values
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < PARAM_HISTORY_SIZE; j++) {
            mTapParameterHistory[i][j].level = mTapLevel[i];
            mTapParameterHistory[i][j].pan = mTapPan[i];
            mTapParameterHistory[i][j].filterCutoff = mTapFilterCutoff[i];
            mTapParameterHistory[i][j].filterResonance = mTapFilterResonance[i];
            mTapParameterHistory[i][j].filterType = mTapFilterType[i];
            mTapParameterHistory[i][j].pitchShift = mTapPitchShift[i];
            mTapParameterHistory[i][j].enabled = mTapEnabled[i];
        }
    }

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
    const float MIN_SAMPLE_RATE = 8000.0f;
    const float MAX_SAMPLE_RATE = 192000.0f;

    if (mSampleRate < MIN_SAMPLE_RATE || mSampleRate > MAX_SAMPLE_RATE) {
        mSampleRate = 44100.0f;
    }

    if (mDelayBypassPrevious != mDelayBypass) {
        if (!mDelayFadingOut && !mDelayFadingIn) {
            if (!mDelayBypassPrevious && mDelayBypass) {
                mDelayFadingOut = true;
                mDelayFadingIn = false;
                mDelayFadeGain = 1.0f;

                int fadeLength = static_cast<int>(std::max(64.0f, std::min(static_cast<float>(mSampleRate * 0.01f), 2048.0f)));
                mDelayFadeRemaining = fadeLength;
                mDelayFadeTotalLength = fadeLength;
            }
            else if (mDelayBypassPrevious && !mDelayBypass) {
                mDelayFadingOut = false;
                mDelayFadingIn = true;
                mDelayFadeGain = 0.0f;

                int fadeLength = static_cast<int>(std::max(32.0f, std::min(static_cast<float>(mSampleRate * 0.005f), 1024.0f)));
                mDelayFadeRemaining = fadeLength;
                mDelayFadeTotalLength = fadeLength;
            }
        }
        mDelayBypassPrevious = mDelayBypass;
    }
}

void WaterStickProcessor::processDelaySection(float inputL, float inputR, float& outputL, float& outputR)
{
    float sumL = 0.0f;
    float sumR = 0.0f;

    for (int tap = 0; tap < NUM_TAPS; tap++) {
        bool processTap = mTapDistribution.isTapEnabled(tap) || mTapFadingOut[tap] || mTapFadingIn[tap];

        if (processTap) {
            float tapDelayTime = mTapDistribution.getTapDelayTime(tap);
            ParameterSnapshot historicParams = getHistoricParameters(tap, tapDelayTime);

            // Set pitch shift parameters for the delay lines
            mTapDelayLinesL[tap].setPitchShift(historicParams.pitchShift);
            mTapDelayLinesR[tap].setPitchShift(historicParams.pitchShift);

            float tapOutputL, tapOutputR;
            mTapDelayLinesL[tap].processSample(inputL, tapOutputL);
            mTapDelayLinesR[tap].processSample(inputR, tapOutputR);

            tapOutputL *= historicParams.level;
            tapOutputR *= historicParams.level;

            mTapFiltersL[tap].setParameters(historicParams.filterCutoff, historicParams.filterResonance, historicParams.filterType);
            mTapFiltersR[tap].setParameters(historicParams.filterCutoff, historicParams.filterResonance, historicParams.filterType);
            tapOutputL = static_cast<float>(mTapFiltersL[tap].process(tapOutputL));
            tapOutputR = static_cast<float>(mTapFiltersR[tap].process(tapOutputR));

            if (mTapFadingOut[tap]) {
                tapOutputL *= mTapFadeGain[tap];
                tapOutputR *= mTapFadeGain[tap];

                mTapFadeOutRemaining[tap]--;
                if (mTapFadeOutRemaining[tap] <= 0) {
                    mTapFadingOut[tap] = false;
                    mTapFadeGain[tap] = 1.0f;
                    mTapDelayLinesL[tap].reset();
                    mTapDelayLinesR[tap].reset();
                } else {
                    float fadeProgress = 1.0f - (static_cast<float>(mTapFadeOutRemaining[tap]) / static_cast<float>(mTapFadeOutTotalLength[tap]));
                    mTapFadeGain[tap] = std::exp(-6.0f * fadeProgress);
                }
            }
            else if (mTapFadingIn[tap]) {
                tapOutputL *= mTapFadeGain[tap];
                tapOutputR *= mTapFadeGain[tap];

                mTapFadeInRemaining[tap]--;
                if (mTapFadeInRemaining[tap] <= 0) {
                    mTapFadingIn[tap] = false;
                    mTapFadeGain[tap] = 1.0f;
                } else {
                    float fadeProgress = 1.0f - (static_cast<float>(mTapFadeInRemaining[tap]) / static_cast<float>(mTapFadeInTotalLength[tap]));
                    mTapFadeGain[tap] = 1.0f - std::exp(-6.0f * fadeProgress);
                }
            }

            float pan = historicParams.pan;
            float leftGain = 1.0f - pan;
            float rightGain = pan;

            sumL += (tapOutputL * leftGain) + (tapOutputR * leftGain);
            sumR += (tapOutputL * rightGain) + (tapOutputR * rightGain);
        }
    }

    mFeedbackBufferL = sumL;
    mFeedbackBufferR = sumR;

    // Delay section is always 100% wet now
    float dryGain = 0.0f;
    float wetGain = 1.0f;
    float delayWetGain = wetGain;

    outputL = (inputL * dryGain) + (sumL * delayWetGain);
    outputR = (inputR * dryGain) + (sumR * delayWetGain);

    if (mDelayFadingOut) {
        outputL *= mDelayFadeGain;
        outputR *= mDelayFadeGain;

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

        mDelayFadeRemaining--;
        if (mDelayFadeRemaining <= 0) {
            mDelayFadingIn = false;
            mDelayFadeGain = 1.0f;
        } else {
            float fadeProgress = 1.0f - (static_cast<float>(mDelayFadeRemaining) / static_cast<float>(mDelayFadeTotalLength));
            mDelayFadeGain = 1.0f - std::exp(-6.0f * fadeProgress);
        }
    }

    if (mDelayBypass && !mDelayFadingOut && !mDelayFadingIn) {
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

    mDelayLineL.initialize(mSampleRate, 2.0);
    mDelayLineR.initialize(mSampleRate, 2.0);

    double maxDelayTime = 20.0;
    for (int i = 0; i < NUM_TAPS; i++) {
        mTapDelayLinesL[i].initialize(mSampleRate, maxDelayTime);
        mTapDelayLinesR[i].initialize(mSampleRate, maxDelayTime);
    }

    mTempoSync.initialize(mSampleRate);
    mTapDistribution.initialize(mSampleRate);

    for (int i = 0; i < NUM_TAPS; i++) {
        mTapFiltersL[i].setSampleRate(mSampleRate);
        mTapFiltersR[i].setSampleRate(mSampleRate);
    }

    return AudioEffect::setupProcessing(newSetup);
}


void WaterStickProcessor::captureCurrentParameters()
{
    // Store current parameter values for all taps
    for (int i = 0; i < 16; i++) {
        ParameterSnapshot& snapshot = mTapParameterHistory[i][mParameterHistoryWriteIndex];
        snapshot.level = mTapLevel[i];
        snapshot.pan = mTapPan[i];
        snapshot.filterCutoff = mTapFilterCutoff[i];
        snapshot.filterResonance = mTapFilterResonance[i];
        snapshot.filterType = mTapFilterType[i];
        snapshot.pitchShift = mTapPitchShift[i];
        snapshot.enabled = mTapEnabled[i];
    }

    // Advance write index (circular buffer)
    mParameterHistoryWriteIndex = (mParameterHistoryWriteIndex + 1) % PARAM_HISTORY_SIZE;
}

WaterStickProcessor::ParameterSnapshot WaterStickProcessor::getHistoricParameters(int tapIndex, float delayTimeSeconds) const
{
    if (tapIndex < 0 || tapIndex >= 16) {
        // Return current parameters as fallback
        ParameterSnapshot current;
        current.level = mTapLevel[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        current.pan = mTapPan[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        current.filterCutoff = mTapFilterCutoff[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        current.filterResonance = mTapFilterResonance[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        current.filterType = mTapFilterType[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        current.enabled = mTapEnabled[tapIndex >= 0 && tapIndex < 16 ? tapIndex : 0];
        return current;
    }

    // Calculate how many samples back to look
    int samplesBack = static_cast<int>(delayTimeSeconds * mSampleRate);
    samplesBack = std::min(samplesBack, PARAM_HISTORY_SIZE - 1);

    // Calculate history index
    int historyIndex = mParameterHistoryWriteIndex - samplesBack;
    if (historyIndex < 0) {
        historyIndex += PARAM_HISTORY_SIZE;
    }

    return mTapParameterHistory[tapIndex][historyIndex];
}

void WaterStickProcessor::checkTempoSyncParameterChanges()
{
    // Check if tempo sync parameters actually changed
    if (mTempoSyncMode) {
        float currentDelayTime = mTempoSync.getDelayTime();
        if (std::abs(currentDelayTime - mLastTempoSyncDelayTime) > 0.001f) {
            mTempoSyncParametersChanged = true;
            mLastTempoSyncDelayTime = currentDelayTime;
        } else {
            mTempoSyncParametersChanged = false;
        }
    } else {
        // In free mode, always update if parameters changed
        mTempoSyncParametersChanged = true;
    }
}

void WaterStickProcessor::updateParameters()
{
    mTempoSync.setMode(mTempoSyncMode);
    mTempoSync.setSyncDivision(mSyncDivision);
    mTempoSync.setFreeTime(mDelayTime);

    checkTempoSyncParameterChanges();

    mTapDistribution.setGrid(mGrid);
    mTapDistribution.updateTempo(mTempoSync);

    for (int i = 0; i < 16; i++) {
        mTapDistribution.setTapEnable(i, mTapEnabled[i]);
        mTapDistribution.setTapLevel(i, mTapLevel[i]);
        mTapDistribution.setTapPan(i, mTapPan[i]);
    }

    for (int i = 0; i < NUM_TAPS; i++) {
        float tapDelayTime = mTapDistribution.getTapDelayTime(i);
        mTapDelayLinesL[i].setDelayTime(tapDelayTime);
        mTapDelayLinesR[i].setDelayTime(tapDelayTime);
    }

    if (!mTempoSyncMode) {
        float finalDelayTime = mTempoSync.getDelayTime();
        mDelayLineL.setDelayTime(finalDelayTime);
        mDelayLineR.setDelayTime(finalDelayTime);
    }

    for (int i = 0; i < NUM_TAPS; i++) {
        mTapFiltersL[i].setParameters(mTapFilterCutoff[i], mTapFilterResonance[i], mTapFilterType[i]);
        mTapFiltersR[i].setParameters(mTapFilterCutoff[i], mTapFilterResonance[i], mTapFilterType[i]);
    }
}

tresult PLUGIN_API WaterStickProcessor::process(Vst::ProcessData& data)
{
    if (data.processContext && data.processContext->state & Vst::ProcessContext::kTempoValid)
    {
        double hostTempo = data.processContext->tempo;
        mTempoSync.updateTempo(hostTempo, true);
    }
    else
    {
        mTempoSync.updateTempo(120.0, false);
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
                        case kGlobalDryWet:
                            mGlobalDryWet = static_cast<float>(value);
                            break;
                        case kDelayBypass:
                            mDelayBypass = value > 0.5;
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

    // Only update tempo sync delay times when parameters actually changed
    if (mTempoSyncMode && mTempoSyncParametersChanged) {
        // Update tap distribution with current tempo
        mTapDistribution.updateTempo(mTempoSync);

        // Update all tap delay times only when parameters changed
        for (int i = 0; i < NUM_TAPS; i++) {
            float tapDelayTime = mTapDistribution.getTapDelayTime(i);
            mTapDelayLinesL[i].setDelayTime(tapDelayTime);
            mTapDelayLinesR[i].setDelayTime(tapDelayTime);
        }

        // Update legacy delay lines too
        float finalDelayTime = mTempoSync.getDelayTime();
        mDelayLineL.setDelayTime(finalDelayTime);
        mDelayLineR.setDelayTime(finalDelayTime);

        // Reset the change flag
        mTempoSyncParametersChanged = false;
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

    for (int32 sample = 0; sample < data.numSamples; sample++)
    {
        captureCurrentParameters();

        float inL = inputL[sample];
        float inR = inputR[sample];

        float inputWithFeedbackL = inL + (mFeedbackBufferL * mFeedback);
        float inputWithFeedbackR = inR + (mFeedbackBufferR * mFeedback);

        inputWithFeedbackL = std::tanh(inputWithFeedbackL);
        inputWithFeedbackR = std::tanh(inputWithFeedbackR);

        float gainedL = inputWithFeedbackL * mInputGain;
        float gainedR = inputWithFeedbackR * mInputGain;

        float delayOutputL, delayOutputR;
        processDelaySection(gainedL, gainedR, delayOutputL, delayOutputR);

        float globalDryGain = std::cos(mGlobalDryWet * M_PI_2);
        float globalWetGain = std::sin(mGlobalDryWet * M_PI_2);
        float mixedL = (inL * globalDryGain) + (delayOutputL * globalWetGain);
        float mixedR = (inR * globalDryGain) + (delayOutputR * globalWetGain);

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
    streamer.writeFloat(mFeedback);
    streamer.writeBool(mTempoSyncMode);
    streamer.writeInt32(mSyncDivision);
    streamer.writeInt32(mGrid);

    streamer.writeFloat(mGlobalDryWet);
    streamer.writeBool(mDelayBypass);

    // Save all tap parameters
    for (int i = 0; i < 16; i++) {
        streamer.writeBool(mTapEnabled[i]);
        streamer.writeFloat(mTapLevel[i]);
        streamer.writeFloat(mTapPan[i]);
        // Save per-tap filter parameters
        streamer.writeFloat(mTapFilterCutoff[i]);
        streamer.writeFloat(mTapFilterResonance[i]);
        streamer.writeInt32(mTapFilterType[i]);
        // Save pitch shift parameter
        streamer.writeInt32(mTapPitchShift[i]);
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
    streamer.readFloat(mFeedback);
    streamer.readBool(mTempoSyncMode);
    streamer.readInt32(mSyncDivision);
    streamer.readInt32(mGrid);

    streamer.readFloat(mGlobalDryWet);
    streamer.readBool(mDelayBypass);

    // Load all tap parameters
    for (int i = 0; i < 16; i++) {
        streamer.readBool(mTapEnabled[i]);
        streamer.readFloat(mTapLevel[i]);
        streamer.readFloat(mTapPan[i]);
        // Load per-tap filter parameters
        streamer.readFloat(mTapFilterCutoff[i]);
        streamer.readFloat(mTapFilterResonance[i]);
        streamer.readInt32(mTapFilterType[i]);
        // Load pitch shift parameter
        streamer.readInt32(mTapPitchShift[i]);

        // Initialize previous state to current state to prevent unwanted buffer clears
        mTapEnabledPrevious[i] = mTapEnabled[i];
    }

    mDelayBypassPrevious = mDelayBypass;

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