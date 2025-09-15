#include "WaterStickProcessor.h"
#include "WaterStickCIDs.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"
#include <cmath>

using namespace Steinberg;

namespace WaterStick {

//------------------------------------------------------------------------
// TempoSync Implementation
//------------------------------------------------------------------------

// Division text lookup table
const char* TempoSync::sDivisionTexts[kNumSyncDivisions] = {
    "1/64", "1/32T", "1/64.", "1/32", "1/16T", "1/32.", "1/16",
    "1/8T", "1/16.", "1/8", "1/4T", "1/8.", "1/4",
    "1/2T", "1/4.", "1/2", "1T", "1/2.", "1", "2", "4", "8"
};

// Division values (in quarter note units)
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
, mSyncDivision(kSync_1_4) // Default to 1/4 note
, mFreeTime(0.25f)         // Default to 250ms
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
        return mFreeTime; // Fallback to free time
    }

    // Calculate time for one quarter note in seconds
    double quarterNoteTime = 60.0 / mHostTempo;

    // Get division multiplier (in quarter note units)
    float divisionValue = sDivisionValues[mSyncDivision];

    // Calculate final delay time
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

//------------------------------------------------------------------------
// TapDistribution Implementation
//------------------------------------------------------------------------

// Grid values lookup table
const float TapDistribution::sGridValues[kNumGridValues] = {
    1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f, 16.0f
};

// Grid text lookup table
const char* TapDistribution::sGridTexts[kNumGridValues] = {
    "1", "2", "3", "4", "6", "8", "12", "16"
};

TapDistribution::TapDistribution()
: mSampleRate(44100.0)
, mBeatTime(0.5f)  // Default to 120 BPM (0.5s per beat)
, mGrid(kGrid_4)   // Default to 4 taps per beat
{
    // Initialize all taps as enabled with unity gain and center pan
    for (int i = 0; i < NUM_TAPS; i++) {
        mTapEnabled[i] = true;
        mTapLevel[i] = 1.0f;
        mTapPan[i] = 0.5f;      // Center pan
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
    // Get beat time from tempo sync
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
        mTapLevel[tapIndex] = std::max(0.0f, std::min(level, 1.0f));
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
    // Rainmaker formula: tap delay time = BEAT TIME * tap number / GRID
    float gridValue = sGridValues[mGrid];

    for (int tap = 0; tap < NUM_TAPS; tap++) {
        // Tap numbers are 1-16 (not 0-15)
        int tapNumber = tap + 1;
        mTapDelayTimes[tap] = mBeatTime * static_cast<float>(tapNumber) / gridValue;

        // Ensure minimum delay time for stability
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

//------------------------------------------------------------------------
// DualDelayLine Implementation - Crossfading STK DelayA Algorithm
//------------------------------------------------------------------------

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
, mStabilityThreshold(2048) // ~46ms at 44.1kHz
, mCrossfadeLength(0)
, mCrossfadePosition(0)
, mCrossfadeGainA(1.0f)
, mCrossfadeGainB(0.0f)
{
    // Initialize delay states
    mStateA.delayInSamples = 0.5f;
    mStateA.readIndex = 0;
    mStateA.allpassCoeff = 0.0f;
    mStateA.apInput = 0.0f;
    mStateA.lastOutput = 0.0f;
    mStateA.doNextOut = true;
    mStateA.nextOutput = 0.0f;

    mStateB = mStateA; // Copy state
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

    // Initialize both delay line states
    updateDelayState(mStateA, mCurrentDelayTime);
    updateDelayState(mStateB, mCurrentDelayTime);

    // Calculate stability threshold (proportional to sample rate)
    mStabilityThreshold = static_cast<int>(sampleRate * 0.05); // 50ms
}

void DualDelayLine::setDelayTime(float delayTimeSeconds)
{
    if (std::abs(delayTimeSeconds - mTargetDelayTime) > 0.001f) {
        mTargetDelayTime = delayTimeSeconds;
        mStabilityCounter = 0; // Reset stability counter on movement
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
    // Crossfade length proportional to delay time
    // Short delays: 50-100ms crossfade
    // Long delays: 200-500ms crossfade
    float baseCrossfadeMs = 50.0f + (delayTime * 1000.0f * 0.25f);
    baseCrossfadeMs = std::min(baseCrossfadeMs, 500.0f);

    return static_cast<int>(baseCrossfadeMs * 0.001f * mSampleRate);
}

void DualDelayLine::startCrossfade()
{
    mCrossfadeState = CROSSFADING;
    mCrossfadeLength = calculateCrossfadeLength(mTargetDelayTime);
    mCrossfadePosition = 0;

    // Update standby line with new delay time
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

    // Smooth crossfade curve (cosine)
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
        // Crossfade complete
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
    // Movement detection and crossfade triggering
    if (std::abs(mTargetDelayTime - mCurrentDelayTime) > 0.001f) {
        mStabilityCounter++;

        if (mStabilityCounter >= mStabilityThreshold && mCrossfadeState == STABLE) {
            startCrossfade();
        }
    } else {
        mStabilityCounter = 0;
    }

    // Update crossfade if active
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

    // Reset delay states
    mStateA.delayInSamples = 0.5f;
    mStateA.readIndex = 0;
    mStateA.apInput = 0.0f;
    mStateA.lastOutput = 0.0f;
    mStateA.doNextOut = true;
    mStateA.nextOutput = 0.0f;
    updateAllpassCoeff(mStateA);

    mStateB = mStateA;
}

//------------------------------------------------------------------------
// STKDelayLine Implementation - Exact STK DelayA Algorithm (Legacy)
//------------------------------------------------------------------------

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

    // STK default: 0.5 samples minimum delay
    mDelayInSamples = 0.5f;
    updateAllpassCoeff();

    mApInput = 0.0f;
    mLastOutput = 0.0f;
    mDoNextOut = true;
    mNextOutput = 0.0f;
}

void STKDelayLine::setDelayTime(float delayTimeSeconds)
{
    // Convert to samples - STK approach: NO SMOOTHING
    float delaySamples = delayTimeSeconds * static_cast<float>(mSampleRate);
    float maxDelaySamples = static_cast<float>(mBufferSize - 1);

    // STK range: 0.5 to maxDelay
    mDelayInSamples = std::max(0.5f, std::min(delaySamples, maxDelaySamples));
    updateAllpassCoeff();
}

void STKDelayLine::updateAllpassCoeff()
{
    // STK DelayA coefficient calculation for fractional part
    float integerPart = floorf(mDelayInSamples);
    float fracPart = mDelayInSamples - integerPart;

    // Ensure minimum fractional delay of 0.5
    if (fracPart < 0.5f) {
        fracPart = 0.5f;
    }

    // STK allpass coefficient: a = (1-D)/(1+D) where D is fractional delay
    mAllpassCoeff = (1.0f - fracPart) / (1.0f + fracPart);
}

float STKDelayLine::nextOut()
{
    // Exact STK DelayA nextOut implementation
    if (mDoNextOut) {
        // Do allpass interpolation delay
        mNextOutput = -mAllpassCoeff * mLastOutput;
        mNextOutput += mApInput + (mAllpassCoeff * mBuffer[mReadIndex]);
        mDoNextOut = false;
    }
    return mNextOutput;
}

void STKDelayLine::processSample(float input, float& output)
{
    // Calculate integer delay part
    int integerDelay = static_cast<int>(floorf(mDelayInSamples));

    // Update read index for integer delay
    mReadIndex = mWriteIndex - integerDelay;
    if (mReadIndex < 0) mReadIndex += mBufferSize;
    mReadIndex %= mBufferSize;

    // Write input to buffer
    mBuffer[mWriteIndex] = input;

    // Get output using STK allpass interpolation
    output = nextOut();
    mLastOutput = output;
    mDoNextOut = true;
    mApInput = mBuffer[mReadIndex];

    // Advance write index
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

//------------------------------------------------------------------------
// WaterStickProcessor Implementation
//------------------------------------------------------------------------

WaterStickProcessor::WaterStickProcessor()
: mInputGain(1.0f)
, mOutputGain(1.0f)
, mDelayTime(0.1f)  // 100ms default
, mDryWet(0.5f)     // 50% wet default
, mTempoSyncMode(false)  // Default to free mode
, mSyncDivision(kSync_1_4)  // Default to 1/4 note
, mGrid(kGrid_4)    // Default to 4 taps per beat
, mSampleRate(44100.0)
{
    // Initialize tap parameters
    for (int i = 0; i < 16; i++) {
        mTapEnabled[i] = true;    // All taps enabled by default
        mTapLevel[i] = 1.0f;      // Unity gain
        mTapPan[i] = 0.5f;        // Center pan
    }

    setControllerClass(kWaterStickControllerUID);
}

WaterStickProcessor::~WaterStickProcessor()
{
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
}

tresult PLUGIN_API WaterStickProcessor::process(Vst::ProcessData& data)
{
    // Update tempo info from host
    if (data.processContext && data.processContext->state & Vst::ProcessContext::kTempoValid)
    {
        double hostTempo = data.processContext->tempo;
        mTempoSync.updateTempo(hostTempo, true);
    }
    else
    {
        mTempoSync.updateTempo(120.0, false); // Fallback tempo
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
                            mInputGain = static_cast<float>(value);
                            break;
                        case kOutputGain:
                            mOutputGain = static_cast<float>(value);
                            break;
                        case kDelayTime:
                            mDelayTime = static_cast<float>(value * 2.0); // 0-2 seconds
                            break;
                        case kDryWet:
                            mDryWet = static_cast<float>(value); // 0-1 (dry to wet)
                            break;
                        case kTempoSyncMode:
                            mTempoSyncMode = value > 0.5; // Toggle: >0.5 = synced
                            break;
                        case kSyncDivision:
                            mSyncDivision = static_cast<int>(value * (kNumSyncDivisions - 1) + 0.5); // Round to nearest
                            break;
                        case kGrid:
                            mGrid = static_cast<int>(value * (kNumGridValues - 1) + 0.5); // Round to nearest
                            break;
                        // Tap enable parameters
                        case kTap1Enable:
                            mTapEnabled[0] = value > 0.5;
                            break;
                        case kTap2Enable:
                            mTapEnabled[1] = value > 0.5;
                            break;
                        case kTap3Enable:
                            mTapEnabled[2] = value > 0.5;
                            break;
                        case kTap4Enable:
                            mTapEnabled[3] = value > 0.5;
                            break;
                        case kTap5Enable:
                            mTapEnabled[4] = value > 0.5;
                            break;
                        case kTap6Enable:
                            mTapEnabled[5] = value > 0.5;
                            break;
                        case kTap7Enable:
                            mTapEnabled[6] = value > 0.5;
                            break;
                        case kTap8Enable:
                            mTapEnabled[7] = value > 0.5;
                            break;
                        case kTap9Enable:
                            mTapEnabled[8] = value > 0.5;
                            break;
                        case kTap10Enable:
                            mTapEnabled[9] = value > 0.5;
                            break;
                        case kTap11Enable:
                            mTapEnabled[10] = value > 0.5;
                            break;
                        case kTap12Enable:
                            mTapEnabled[11] = value > 0.5;
                            break;
                        case kTap13Enable:
                            mTapEnabled[12] = value > 0.5;
                            break;
                        case kTap14Enable:
                            mTapEnabled[13] = value > 0.5;
                            break;
                        case kTap15Enable:
                            mTapEnabled[14] = value > 0.5;
                            break;
                        case kTap16Enable:
                            mTapEnabled[15] = value > 0.5;
                            break;
                        // Tap level parameters
                        case kTap1Level:
                            mTapLevel[0] = static_cast<float>(value);
                            break;
                        case kTap2Level:
                            mTapLevel[1] = static_cast<float>(value);
                            break;
                        case kTap3Level:
                            mTapLevel[2] = static_cast<float>(value);
                            break;
                        case kTap4Level:
                            mTapLevel[3] = static_cast<float>(value);
                            break;
                        case kTap5Level:
                            mTapLevel[4] = static_cast<float>(value);
                            break;
                        case kTap6Level:
                            mTapLevel[5] = static_cast<float>(value);
                            break;
                        case kTap7Level:
                            mTapLevel[6] = static_cast<float>(value);
                            break;
                        case kTap8Level:
                            mTapLevel[7] = static_cast<float>(value);
                            break;
                        case kTap9Level:
                            mTapLevel[8] = static_cast<float>(value);
                            break;
                        case kTap10Level:
                            mTapLevel[9] = static_cast<float>(value);
                            break;
                        case kTap11Level:
                            mTapLevel[10] = static_cast<float>(value);
                            break;
                        case kTap12Level:
                            mTapLevel[11] = static_cast<float>(value);
                            break;
                        case kTap13Level:
                            mTapLevel[12] = static_cast<float>(value);
                            break;
                        case kTap14Level:
                            mTapLevel[13] = static_cast<float>(value);
                            break;
                        case kTap15Level:
                            mTapLevel[14] = static_cast<float>(value);
                            break;
                        case kTap16Level:
                            mTapLevel[15] = static_cast<float>(value);
                            break;
                        // Tap pan parameters
                        case kTap1Pan:
                            mTapPan[0] = static_cast<float>(value);
                            break;
                        case kTap2Pan:
                            mTapPan[1] = static_cast<float>(value);
                            break;
                        case kTap3Pan:
                            mTapPan[2] = static_cast<float>(value);
                            break;
                        case kTap4Pan:
                            mTapPan[3] = static_cast<float>(value);
                            break;
                        case kTap5Pan:
                            mTapPan[4] = static_cast<float>(value);
                            break;
                        case kTap6Pan:
                            mTapPan[5] = static_cast<float>(value);
                            break;
                        case kTap7Pan:
                            mTapPan[6] = static_cast<float>(value);
                            break;
                        case kTap8Pan:
                            mTapPan[7] = static_cast<float>(value);
                            break;
                        case kTap9Pan:
                            mTapPan[8] = static_cast<float>(value);
                            break;
                        case kTap10Pan:
                            mTapPan[9] = static_cast<float>(value);
                            break;
                        case kTap11Pan:
                            mTapPan[10] = static_cast<float>(value);
                            break;
                        case kTap12Pan:
                            mTapPan[11] = static_cast<float>(value);
                            break;
                        case kTap13Pan:
                            mTapPan[12] = static_cast<float>(value);
                            break;
                        case kTap14Pan:
                            mTapPan[13] = static_cast<float>(value);
                            break;
                        case kTap15Pan:
                            mTapPan[14] = static_cast<float>(value);
                            break;
                        case kTap16Pan:
                            mTapPan[15] = static_cast<float>(value);
                            break;
                    }
                }
            }
        }
    }

    updateParameters();

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

    // Process samples
    for (int32 sample = 0; sample < data.numSamples; sample++)
    {
        // Get input samples
        float inL = inputL[sample];
        float inR = inputR[sample];

        // Apply input gain
        float gainedL = inL * mInputGain;
        float gainedR = inR * mInputGain;

        // Process through all 16 tap delay lines
        float sumL = 0.0f;
        float sumR = 0.0f;

        for (int tap = 0; tap < NUM_TAPS; tap++) {
            if (mTapDistribution.isTapEnabled(tap)) {
                // Process through both L and R delay lines for this tap
                float tapOutputL, tapOutputR;
                mTapDelayLinesL[tap].processSample(gainedL, tapOutputL);
                mTapDelayLinesR[tap].processSample(gainedR, tapOutputR);

                // Apply tap level
                float tapLevel = mTapDistribution.getTapLevel(tap);
                tapOutputL *= tapLevel;
                tapOutputR *= tapLevel;

                // Apply stereo panning (0.0 = full left, 0.5 = center, 1.0 = full right)
                float pan = mTapDistribution.getTapPan(tap);
                float leftGain = 1.0f - pan;   // Left channel gain
                float rightGain = pan;         // Right channel gain

                // Pan the tap output and add to sum
                sumL += (tapOutputL * leftGain) + (tapOutputR * leftGain);
                sumR += (tapOutputL * rightGain) + (tapOutputR * rightGain);
            }
        }

        // Dry/wet mix
        float dryGain = 1.0f - mDryWet;
        float wetGain = mDryWet;
        float mixedL = (inL * dryGain) + (sumL * wetGain);
        float mixedR = (inR * dryGain) + (sumR * wetGain);

        // Apply output gain and write to output
        outputL[sample] = mixedL * mOutputGain;
        outputR[sample] = mixedR * mOutputGain;
    }

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::getState(IBStream* state)
{
    IBStreamer streamer(state, kLittleEndian);

    streamer.writeFloat(mInputGain);
    streamer.writeFloat(mOutputGain);
    streamer.writeFloat(mDelayTime);
    streamer.writeFloat(mDryWet);
    streamer.writeBool(mTempoSyncMode);
    streamer.writeInt32(mSyncDivision);
    streamer.writeInt32(mGrid);

    // Save all tap parameters
    for (int i = 0; i < 16; i++) {
        streamer.writeBool(mTapEnabled[i]);
        streamer.writeFloat(mTapLevel[i]);
        streamer.writeFloat(mTapPan[i]);
    }

    return kResultOk;
}

tresult PLUGIN_API WaterStickProcessor::setState(IBStream* state)
{
    IBStreamer streamer(state, kLittleEndian);

    streamer.readFloat(mInputGain);
    streamer.readFloat(mOutputGain);
    streamer.readFloat(mDelayTime);
    streamer.readFloat(mDryWet);
    streamer.readBool(mTempoSyncMode);
    streamer.readInt32(mSyncDivision);
    streamer.readInt32(mGrid);

    // Load all tap parameters
    for (int i = 0; i < 16; i++) {
        streamer.readBool(mTapEnabled[i]);
        streamer.readFloat(mTapLevel[i]);
        streamer.readFloat(mTapPan[i]);
    }

    return kResultOk;
}

} // namespace WaterStick