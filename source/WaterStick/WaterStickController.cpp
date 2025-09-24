#include "WaterStickController.h"
#include "WaterStickEditor.h"
#include "WaterStickCIDs.h"
#include "WaterStickLogger.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/vsttypes.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>

using namespace Steinberg;

namespace WaterStick {

//------------------------------------------------------------------------
// RandomizationEngine Implementation
//------------------------------------------------------------------------
RandomizationEngine::RandomizationEngine() :
    mGenerator(),
    mDistribution(0.0f, 1.0f),
    mAmount(1.0f),
    mCurrentSeed(0)
{
}

RandomizationEngine::~RandomizationEngine() {}

void RandomizationEngine::initialize(unsigned int seed) {
    setSeed(seed);
}

void RandomizationEngine::setSeed(unsigned int seed) {
    mCurrentSeed = seed;
    mGenerator.seed(seed);
}

void RandomizationEngine::setAmount(float amount) {
    mAmount = std::max(0.0f, std::min(1.0f, amount));
}

float RandomizationEngine::randomizeParameter(Steinberg::Vst::ParamID id, float currentValue, RandomizationConstraints constraint) {
    float randomValue = generateConstrainedValue(constraint, currentValue, id);
    return currentValue + mAmount * (randomValue - currentValue);
}

void RandomizationEngine::randomizeAllParameters(WaterStickController* controller) {
    if (!controller) return;

    // Randomize discrete parameters with full scale
    for (int i = 0; i < 24; i++) {
        Steinberg::Vst::ParamID paramId = kDiscrete1 + i;
        float currentValue = controller->getParamNormalized(paramId);
        float newValue = randomizeParameter(paramId, currentValue, kRandomConstraint_Full);
        controller->setParamNormalized(paramId, newValue);
    }

    // Randomize filter resonance with FULL SCALE 0.0-1.0 as specified
    for (int i = 0; i < 16; i++) {
        Steinberg::Vst::ParamID resonanceId = kTap1FilterResonance + (i * 3);
        float newValue = mDistribution(mGenerator); // Full scale 0.0-1.0
        controller->setParamNormalized(resonanceId, newValue);
    }

    // Randomize other tap parameters with musical constraints
    for (int i = 0; i < 16; i++) {
        Steinberg::Vst::ParamID levelId = kTap1Level + (i * 3);
        Steinberg::Vst::ParamID panId = kTap1Pan + (i * 3);
        Steinberg::Vst::ParamID cutoffId = kTap1FilterCutoff + (i * 3);

        float currentLevel = controller->getParamNormalized(levelId);
        float currentPan = controller->getParamNormalized(panId);
        float currentCutoff = controller->getParamNormalized(cutoffId);

        float newLevel = randomizeParameter(levelId, currentLevel, kRandomConstraint_Musical);
        float newPan = randomizeParameter(panId, currentPan, kRandomConstraint_Bipolar);
        float newCutoff = randomizeParameter(cutoffId, currentCutoff, kRandomConstraint_Musical);

        controller->setParamNormalized(levelId, newLevel);
        controller->setParamNormalized(panId, newPan);
        controller->setParamNormalized(cutoffId, newCutoff);
    }
}

void RandomizationEngine::randomizeParameterGroup(WaterStickController* controller, const std::vector<Steinberg::Vst::ParamID>& paramIds) {
    if (!controller) return;

    for (auto paramId : paramIds) {
        RandomizationConstraints constraint = classifyParameter(paramId);
        float currentValue = controller->getParamNormalized(paramId);
        float newValue = randomizeParameter(paramId, currentValue, constraint);
        controller->setParamNormalized(paramId, newValue);
    }
}

float RandomizationEngine::applyMusicalConstraint(float value, Steinberg::Vst::ParamID id) {
    // Apply musical quantization based on parameter type
    if (id >= kTap1FilterCutoff && id <= kTap16FilterCutoff) {
        // Quantize filter cutoffs to musical intervals
        const float musicalSteps[] = {0.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 0.875f, 1.0f};
        const int numSteps = sizeof(musicalSteps) / sizeof(musicalSteps[0]);

        float bestDistance = 2.0f;
        float quantizedValue = value;

        for (int i = 0; i < numSteps; i++) {
            float distance = std::abs(value - musicalSteps[i]);
            if (distance < bestDistance) {
                bestDistance = distance;
                quantizedValue = musicalSteps[i];
            }
        }
        return quantizedValue;
    }

    return value;
}

float RandomizationEngine::applyConservativeConstraint(float value, float currentValue) {
    // Limit randomization to ±30% of current value
    float minValue = std::max(0.0f, currentValue - 0.3f);
    float maxValue = std::min(1.0f, currentValue + 0.3f);
    return minValue + value * (maxValue - minValue);
}

float RandomizationEngine::applyBipolarConstraint(float value, float currentValue) {
    // Center around 0.5 with controlled deviation
    float deviation = (value - 0.5f) * 0.6f; // ±30% max deviation
    return std::max(0.0f, std::min(1.0f, 0.5f + deviation));
}

RandomizationConstraints RandomizationEngine::classifyParameter(Steinberg::Vst::ParamID id) {
    if (id >= kDiscrete1 && id <= kDiscrete24) {
        return kRandomConstraint_Full;
    } else if (id >= kTap1FilterResonance && id <= kTap16FilterResonance) {
        return kRandomConstraint_Full; // Specified requirement
    } else if (id >= kTap1Pan && id <= kTap16Pan) {
        return kRandomConstraint_Bipolar;
    } else if (id >= kTap1FilterCutoff && id <= kTap16FilterCutoff) {
        return kRandomConstraint_Musical;
    } else if (id >= kTap1Level && id <= kTap16Level) {
        return kRandomConstraint_Musical;
    } else {
        return kRandomConstraint_Conservative;
    }
}

float RandomizationEngine::generateConstrainedValue(RandomizationConstraints constraint, float currentValue, Steinberg::Vst::ParamID id) {
    float randomValue = mDistribution(mGenerator);

    switch (constraint) {
        case kRandomConstraint_Full:
            return randomValue;
        case kRandomConstraint_Musical:
            return applyMusicalConstraint(randomValue, id);
        case kRandomConstraint_Conservative:
            return applyConservativeConstraint(randomValue, currentValue);
        case kRandomConstraint_Bipolar:
            return applyBipolarConstraint(randomValue, currentValue);
        default:
            return randomValue;
    }
}

//------------------------------------------------------------------------
// DefaultResetSystem Implementation
//------------------------------------------------------------------------
DefaultResetSystem::DefaultResetSystem() {}

DefaultResetSystem::~DefaultResetSystem() {}

void DefaultResetSystem::initialize() {
    buildDefaultValueMap();
}

void DefaultResetSystem::resetAllParameters(WaterStickController* controller) {
    if (!controller) return;

    for (const auto& pair : mDefaultValues) {
        controller->setParamNormalized(pair.first, pair.second);
    }
}

void DefaultResetSystem::resetParameterGroup(WaterStickController* controller, const std::vector<Steinberg::Vst::ParamID>& paramIds) {
    if (!controller) return;

    for (auto paramId : paramIds) {
        float defaultValue = getDefaultValue(paramId);
        controller->setParamNormalized(paramId, defaultValue);
    }
}

void DefaultResetSystem::resetGlobalParameters(WaterStickController* controller) {
    if (!controller) return;

    std::vector<Steinberg::Vst::ParamID> globalParams = {
        kInputGain, kOutputGain, kDelayTime, kFeedback, kTempoSyncMode,
        kSyncDivision, kGrid, kGlobalDryWet, kDelayBypass
    };
    resetParameterGroup(controller, globalParams);
}

void DefaultResetSystem::resetTapParameters(WaterStickController* controller) {
    if (!controller) return;

    std::vector<Steinberg::Vst::ParamID> tapParams;
    for (int i = 0; i < 16; i++) {
        tapParams.push_back(kTap1Enable + (i * 3));
        tapParams.push_back(kTap1Level + (i * 3));
        tapParams.push_back(kTap1Pan + (i * 3));
        tapParams.push_back(kTap1PitchShift + i);
        tapParams.push_back(kTap1FeedbackSend + i);
    }
    resetParameterGroup(controller, tapParams);
}

void DefaultResetSystem::resetFilterParameters(WaterStickController* controller) {
    if (!controller) return;

    std::vector<Steinberg::Vst::ParamID> filterParams;
    for (int i = 0; i < 16; i++) {
        filterParams.push_back(kTap1FilterCutoff + (i * 3));
        filterParams.push_back(kTap1FilterResonance + (i * 3));
        filterParams.push_back(kTap1FilterType + (i * 3));
    }
    resetParameterGroup(controller, filterParams);
}

void DefaultResetSystem::resetDiscreteParameters(WaterStickController* controller) {
    if (!controller) return;

    std::vector<Steinberg::Vst::ParamID> discreteParams;
    for (int i = 0; i < 24; i++) {
        discreteParams.push_back(kDiscrete1 + i);
    }
    resetParameterGroup(controller, discreteParams);
}

float DefaultResetSystem::getDefaultValue(Steinberg::Vst::ParamID id) {
    auto it = mDefaultValues.find(id);
    if (it != mDefaultValues.end()) {
        return it->second;
    }
    return 0.0f; // Safe fallback
}

void DefaultResetSystem::buildDefaultValueMap() {
    // Global parameters
    mDefaultValues[kInputGain] = 40.0f/52.0f;  // 0dB
    mDefaultValues[kOutputGain] = 40.0f/52.0f; // 0dB
    mDefaultValues[kDelayTime] = 0.05f;        // 50ms
    mDefaultValues[kFeedback] = 0.0f;          // 0%
    mDefaultValues[kTempoSyncMode] = 0.0f;     // Free mode
    mDefaultValues[kSyncDivision] = static_cast<float>(kSync_1_4) / (kNumSyncDivisions - 1);
    mDefaultValues[kGrid] = static_cast<float>(kGrid_4) / (kNumGridValues - 1);
    mDefaultValues[kGlobalDryWet] = 0.5f;      // 50%
    mDefaultValues[kDelayBypass] = 0.0f;       // Active

    // Tap parameters
    for (int i = 0; i < 16; i++) {
        mDefaultValues[kTap1Enable + (i * 3)] = 0.0f;   // Disabled
        mDefaultValues[kTap1Level + (i * 3)] = 0.8f;    // 80%
        mDefaultValues[kTap1Pan + (i * 3)] = 0.5f;      // Center
        mDefaultValues[kTap1FilterCutoff + (i * 3)] = 0.566323334778673f; // 1kHz
        mDefaultValues[kTap1FilterResonance + (i * 3)] = 0.5f; // Moderate
        mDefaultValues[kTap1FilterType + (i * 3)] = 0.0f; // Bypass
        mDefaultValues[kTap1PitchShift + i] = 0.5f;     // 0 semitones
        mDefaultValues[kTap1FeedbackSend + i] = 0.0f;   // 0%
    }

    // Discrete parameters default to 0.0
    for (int i = 0; i < 24; i++) {
        mDefaultValues[kDiscrete1 + i] = 0.0f;
    }

    // Macro curve parameters default to Linear
    for (int i = 0; i < 4; i++) {
        mDefaultValues[kMacroCurve1Type + i] = 0.0f; // Linear
    }

    // Macro knob parameters
    mDefaultValues[kMacroKnob1] = 0.0f;
    mDefaultValues[kMacroKnob2] = 0.0f;
    mDefaultValues[kMacroKnob3] = 0.0f;
    mDefaultValues[kMacroKnob4] = 0.0f;
    mDefaultValues[kMacroKnob5] = 0.0f;
    mDefaultValues[kMacroKnob6] = 0.0f;
    mDefaultValues[kMacroKnob7] = 0.0f;
    mDefaultValues[kMacroKnob8] = 0.0f;

    // System parameters
    mDefaultValues[kRandomizeSeed] = 0.0f;
    mDefaultValues[kRandomizeAmount] = 1.0f;
    mDefaultValues[kRandomizeTrigger] = 0.0f;
    mDefaultValues[kResetTrigger] = 0.0f;
}

//------------------------------------------------------------------------
// MacroCurveSystem Implementation
//------------------------------------------------------------------------
const char* MacroCurveSystem::sCurveTypeNames[kNumCurveTypes] = {
    "Linear", "Exponential", "Inv Exponential", "Logarithmic",
    "Inv Logarithmic", "S-Curve", "Inv S-Curve", "Quantized"
};

// Rainmaker-style macro curve names for positions 0-7
const char* MacroCurveSystem::sRainmakerCurveNames[8] = {
    "Ramp Up",         // Position 0: Linear ramp up (tap 1=0%, tap 16=100%)
    "Ramp Down",       // Position 1: Linear ramp down (tap 1=100%, tap 16=0%)
    "S-Curve",         // Position 2: S-curve sigmoid (smooth acceleration/deceleration)
    "S-Curve Inv",     // Position 3: Inverted S-curve
    "Exp Up",          // Position 4: Exponential up (slow start, fast finish)
    "Exp Down",        // Position 5: Exponential down (fast start, slow finish)
    "Level 70%",       // Position 6: Uniform level at 70%
    "Level 90%"        // Position 7: Uniform level at 90%
};

MacroCurveSystem::MacroCurveSystem() {
    // Initialize to linear curves
    for (int i = 0; i < 4; i++) {
        mCurveTypes[i] = kCurveType_Linear;
    }
}

MacroCurveSystem::~MacroCurveSystem() {}

void MacroCurveSystem::initialize() {
    // Curves initialized in constructor
}

void MacroCurveSystem::setCurveType(int curveIndex, MacroCurveTypes type) {
    if (curveIndex >= 0 && curveIndex < 4) {
        mCurveTypes[curveIndex] = type;
    }
}

MacroCurveTypes MacroCurveSystem::getCurveType(int curveIndex) const {
    if (curveIndex >= 0 && curveIndex < 4) {
        return mCurveTypes[curveIndex];
    }
    return kCurveType_Linear;
}

float MacroCurveSystem::evaluateCurve(MacroCurveTypes type, float input) const {
    // Clamp input to [0.0, 1.0]
    float x = std::max(0.0f, std::min(1.0f, input));

    switch (type) {
        case kCurveType_Linear:
            return evaluateLinear(x);
        case kCurveType_Exponential:
            return evaluateExponential(x);
        case kCurveType_InverseExp:
            return evaluateInverseExponential(x);
        case kCurveType_Logarithmic:
            return evaluateLogarithmic(x);
        case kCurveType_InverseLog:
            return evaluateInverseLogarithmic(x);
        case kCurveType_SCurve:
            return evaluateSCurve(x);
        case kCurveType_InverseSCurve:
            return evaluateInverseSCurve(x);
        case kCurveType_Quantized:
            return evaluateQuantized(x);
        default:
            return evaluateLinear(x);
    }
}

float MacroCurveSystem::evaluateCurve(int curveIndex, float input) const {
    MacroCurveTypes type = getCurveType(curveIndex);
    return evaluateCurve(type, input);
}

const char* MacroCurveSystem::getCurveTypeName(MacroCurveTypes type) const {
    if (type >= 0 && type < kNumCurveTypes) {
        return sCurveTypeNames[type];
    }
    return "Unknown";
}

float MacroCurveSystem::applyCurveToParameter(int curveIndex, float parameterValue) const {
    return evaluateCurve(curveIndex, parameterValue);
}

float MacroCurveSystem::evaluateLinear(float x) const {
    return x;
}

float MacroCurveSystem::evaluateExponential(float x) const {
    return x * x;
}

float MacroCurveSystem::evaluateInverseExponential(float x) const {
    return 1.0f - (1.0f - x) * (1.0f - x);
}

float MacroCurveSystem::evaluateLogarithmic(float x) const {
    return std::log10(1.0f + x * 9.0f);
}

float MacroCurveSystem::evaluateInverseLogarithmic(float x) const {
    return (std::pow(10.0f, x) - 1.0f) / 9.0f;
}

float MacroCurveSystem::evaluateSCurve(float x) const {
    return 0.5f * (1.0f + std::tanh(4.0f * (x - 0.5f)));
}

float MacroCurveSystem::evaluateInverseSCurve(float x) const {
    return 0.5f + 0.25f * std::atan(4.0f * (2.0f * x - 1.0f)) / std::atan(4.0f);
}

float MacroCurveSystem::evaluateQuantized(float x) const {
    // 8 quantized steps
    int step = static_cast<int>(x * 7.999f); // 0-7
    return static_cast<float>(step) / 7.0f;
}

// Rainmaker-style global curve implementations
float MacroCurveSystem::evaluateRampUp(float x) const {
    // Linear ascending: tap 1 = 0%, tap 16 = 100%
    return x;
}

float MacroCurveSystem::evaluateRampDown(float x) const {
    // Linear descending: tap 1 = 100%, tap 16 = 0%
    return 1.0f - x;
}

float MacroCurveSystem::evaluateSigmoidSCurve(float x) const {
    // Professional S-curve sigmoid with smooth acceleration/deceleration
    // Using tanh-based sigmoid for smooth transitions
    const float steepness = 6.0f; // Professional steepness for audio applications
    return 0.5f * (1.0f + std::tanh(steepness * (x - 0.5f)));
}

float MacroCurveSystem::evaluateInverseSigmoid(float x) const {
    // Inverse sigmoid curve - inverse of the S-curve
    const float steepness = 6.0f;
    return 1.0f - (0.5f * (1.0f + std::tanh(steepness * (x - 0.5f))));
}

float MacroCurveSystem::evaluateExpUp(float x) const {
    // Exponential up: slow start, fast finish
    return x * x * x; // Cubic curve for professional feel
}

float MacroCurveSystem::evaluateExpDown(float x) const {
    // Exponential down: fast start, slow finish
    const float invX = 1.0f - x;
    return 1.0f - (invX * invX * invX);
}

float MacroCurveSystem::getGlobalCurveValueForTap(int discretePosition, int tapIndex) const {
    if (tapIndex < 0 || tapIndex >= 16) return 0.0f;

    // Normalize tap index to 0.0-1.0 for curve evaluation
    const float normalizedPosition = static_cast<float>(tapIndex) / 15.0f; // 0-15 maps to 0.0-1.0

    switch (discretePosition) {
        case 0: // Ramp up curve
            return evaluateRampUp(normalizedPosition);
        case 1: // Ramp down curve
            return evaluateRampDown(normalizedPosition);
        case 2: // S-curve sigmoid
            return evaluateSigmoidSCurve(normalizedPosition);
        case 3: // S-curve inverted
            return evaluateInverseSigmoid(normalizedPosition);
        case 4: // Exponential up
            return evaluateExpUp(normalizedPosition);
        case 5: // Exponential down
            return evaluateExpDown(normalizedPosition);
        case 6: // Uniform level 70%
            return 0.7f;
        case 7: // Uniform level 90%
            return 0.9f;
        default:
            return 0.0f;
    }
}

float MacroCurveSystem::getUniformLevel(int discretePosition) const {
    switch (discretePosition) {
        case 6: return 0.7f; // 70% uniform level
        case 7: return 0.9f; // 90% uniform level
        default: return 0.0f;
    }
}

void MacroCurveSystem::applyGlobalMacroCurve(int discretePosition, int currentTapContext, WaterStickController* controller) const {
    if (!controller || discretePosition < 0 || discretePosition > 7) return;

    // DIAGNOSTIC: Log global macro curve application
    const char* contextNames[] = {"Enable", "Volume", "Pan", "FilterCutoff", "FilterResonance", "FilterType", "PitchShift", "FeedbackSend"};
    const char* contextName = (currentTapContext >= 0 && currentTapContext < 8) ? contextNames[currentTapContext] : "Unknown";

    printf("[MacroCurveSystem] applyGlobalMacroCurve START - discretePos: %d, context: %s (%d), applying to ALL 16 taps\n",
           discretePosition, contextName, currentTapContext);
    printf("[MacroCurveSystem] Curve pattern: %s\n", getRainmakerCurveName(discretePosition));

    // Apply curve to all 16 taps based on current context
    for (int tapIndex = 0; tapIndex < 16; tapIndex++) {
        float curveValue = getGlobalCurveValueForTap(discretePosition, tapIndex);

        // Get the appropriate parameter ID based on context and tap index
        Steinberg::Vst::ParamID paramId = -1;

        switch (currentTapContext) {
            case 0: // TapContext::Enable
                paramId = kTap1Enable + (tapIndex * 3);
                break;
            case 1: // TapContext::Volume
                paramId = kTap1Level + (tapIndex * 3);
                break;
            case 2: // TapContext::Pan
                paramId = kTap1Pan + (tapIndex * 3);
                break;
            case 3: // TapContext::FilterCutoff
                paramId = kTap1FilterCutoff + (tapIndex * 3);
                break;
            case 4: // TapContext::FilterResonance
                paramId = kTap1FilterResonance + (tapIndex * 3);
                break;
            case 5: // TapContext::FilterType
                paramId = kTap1FilterType + (tapIndex * 3);
                // For filter type, quantize to valid discrete values (0-4)
                curveValue = std::floor(curveValue * 4.999f) / 4.0f;
                break;
            case 6: // TapContext::PitchShift
                paramId = kTap1PitchShift + tapIndex;
                // For pitch shift, map to bipolar range (-1.0 to +1.0)
                curveValue = (curveValue * 2.0f) - 1.0f;
                break;
            case 7: // TapContext::FeedbackSend
                paramId = kTap1FeedbackSend + tapIndex;
                break;
            default:
                continue;
        }

        // Apply the curve value to the parameter
        if (paramId >= 0) {
            controller->setParamNormalized(paramId, curveValue);
            controller->performEdit(paramId, curveValue);

            // DIAGNOSTIC: Log each tap parameter update
            printf("[MacroCurveSystem] Tap %d: paramId %d → %.3f\n",
                   tapIndex + 1, static_cast<int>(paramId), curveValue);
        } else {
            printf("[MacroCurveSystem] WARNING: Invalid paramId for tap %d, context %d\n",
                   tapIndex + 1, currentTapContext);
        }
    }

    printf("[MacroCurveSystem] applyGlobalMacroCurve COMPLETE - Updated %d tap parameters\n", 16);
}

void MacroCurveSystem::applyGlobalMacroCurveWithType(int discretePosition, int currentTapContext, MacroCurveTypes curveType, WaterStickController* controller) const {
    if (!controller || discretePosition < 0 || discretePosition > 7) return;

    // Apply specified curve type to all 16 taps based on current context
    for (int tapIndex = 0; tapIndex < 16; tapIndex++) {
        float curveValue = getCurveValueForTapWithType(curveType, tapIndex);

        // Get the appropriate parameter ID based on context and tap index
        Steinberg::Vst::ParamID paramId = -1;

        switch (currentTapContext) {
            case 0: // TapContext::Enable
                paramId = kTap1Enable + (tapIndex * 3);
                break;
            case 1: // TapContext::Volume
                paramId = kTap1Level + (tapIndex * 3);
                break;
            case 2: // TapContext::Pan
                paramId = kTap1Pan + (tapIndex * 3);
                break;
            case 3: // TapContext::FilterCutoff
                paramId = kTap1FilterCutoff + (tapIndex * 3);
                break;
            case 4: // TapContext::FilterResonance
                paramId = kTap1FilterResonance + (tapIndex * 3);
                break;
            case 5: // TapContext::FilterType
                paramId = kTap1FilterType + (tapIndex * 3);
                // For filter type, quantize to valid discrete values (0-4)
                curveValue = std::floor(curveValue * 4.999f) / 4.0f;
                break;
            case 6: // TapContext::PitchShift
                paramId = kTap1PitchShift + tapIndex;
                // For pitch shift, map to bipolar range (-1.0 to +1.0)
                curveValue = (curveValue * 2.0f) - 1.0f;
                break;
            case 7: // TapContext::FeedbackSend
                paramId = kTap1FeedbackSend + tapIndex;
                break;
            default:
                continue;
        }

        // Apply the curve value to the parameter
        if (paramId >= 0) {
            controller->setParamNormalized(paramId, curveValue);
            controller->performEdit(paramId, curveValue);
        }
    }
}

float MacroCurveSystem::getCurveValueForTapWithType(MacroCurveTypes curveType, int tapIndex) const {
    if (tapIndex < 0 || tapIndex >= 16) return 0.0f;

    // Normalize tap index to 0.0-1.0 for curve evaluation
    const float normalizedPosition = static_cast<float>(tapIndex) / 15.0f; // 0-15 maps to 0.0-1.0

    return evaluateCurve(curveType, normalizedPosition);
}

const char* MacroCurveSystem::getRainmakerCurveName(int discretePosition) const {
    if (discretePosition >= 0 && discretePosition < 8) {
        return sRainmakerCurveNames[discretePosition];
    }
    return "Unknown";
}

//------------------------------------------------------------------------
WaterStickController::WaterStickController()
{
    // Initialize discrete parameters
    for (int i = 0; i < 24; i++) {
        mDiscreteParameters[i] = 0.0f;
    }
}

//------------------------------------------------------------------------
WaterStickController::~WaterStickController()
{
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::initialize(FUnknown* context)
{
    WS_LOG_SESSION_START();

    tresult result = EditControllerEx1::initialize(context);
    if (result != kResultOk)
    {
        return result;
    }

    // Initialize backend systems
    mRandomizationEngine.initialize();
    mDefaultResetSystem.initialize();
    mMacroCurveSystem.initialize();

    // Add parameters
    parameters.addParameter(STR16("Input Gain"), STR16("dB"), 0, 40.0/52.0,
                           Vst::ParameterInfo::kCanAutomate, kInputGain, 0,
                           STR16("Input"));

    parameters.addParameter(STR16("Output Gain"), STR16("dB"), 0, 40.0/52.0,
                           Vst::ParameterInfo::kCanAutomate, kOutputGain, 0,
                           STR16("Output"));

    parameters.addParameter(STR16("Delay Time"), STR16("s"), 0, 0.05,
                           Vst::ParameterInfo::kCanAutomate, kDelayTime, 0,
                           STR16("Delay"));

    parameters.addParameter(STR16("Feedback"), STR16("%"), 0, 0.0,
                           Vst::ParameterInfo::kCanAutomate, kFeedback, 0,
                           STR16("Global"));

    // Tempo sync parameters
    parameters.addParameter(STR16("Sync Mode"), nullptr, 1, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTempoSyncMode, 0,
                           STR16("Sync"));

    parameters.addParameter(STR16("Sync Division"), nullptr, kNumSyncDivisions - 1,
                           static_cast<Vst::ParamValue>(kSync_1_4) / (kNumSyncDivisions - 1),
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kSyncDivision, 0,
                           STR16("Sync"));

    // Grid parameter
    parameters.addParameter(STR16("Grid"), nullptr, kNumGridValues - 1,
                           static_cast<Vst::ParamValue>(kGrid_4) / (kNumGridValues - 1),
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kGrid, 0,
                           STR16("Tap"));

    // Tap parameters (16 taps) - need to use individual strings for STR16 macro
    parameters.addParameter(STR16("Tap 1 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap1Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 1 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap1Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 1 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap1Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 2 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap2Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 2 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap2Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 2 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap2Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 3 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap3Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 3 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap3Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 3 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap3Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 4 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap4Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 4 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap4Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 4 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap4Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 5 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap5Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 5 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap5Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 5 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap5Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 6 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap6Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 6 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap6Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 6 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap6Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 7 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap7Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 7 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap7Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 7 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap7Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 8 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap8Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 8 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap8Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 8 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap8Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 9 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap9Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 9 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap9Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 9 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap9Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 10 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap10Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 10 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap10Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 10 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap10Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 11 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap11Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 11 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap11Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 11 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap11Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 12 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap12Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 12 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap12Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 12 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap12Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 13 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap13Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 13 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap13Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 13 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap13Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 14 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap14Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 14 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap14Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 14 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap14Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 15 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap15Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 15 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap15Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 15 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap15Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 16 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap16Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 16 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap16Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 16 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap16Pan, 0, STR16("Tap"));

    // Per-tap filter parameters (16 taps × 3 parameters each)
    parameters.addParameter(STR16("Tap 1 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap1FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 1 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap1FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 1 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap1FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 2 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap2FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 2 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap2FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 2 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap2FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 3 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap3FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 3 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap3FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 3 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap3FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 4 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap4FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 4 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap4FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 4 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap4FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 5 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap5FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 5 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap5FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 5 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap5FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 6 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap6FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 6 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap6FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 6 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap6FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 7 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap7FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 7 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap7FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 7 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap7FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 8 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap8FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 8 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap8FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 8 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap8FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 9 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap9FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 9 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap9FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 9 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap9FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 10 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap10FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 10 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap10FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 10 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap10FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 11 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap11FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 11 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap11FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 11 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap11FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 12 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap12FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 12 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap12FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 12 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap12FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 13 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap13FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 13 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap13FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 13 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap13FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 14 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap14FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 14 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap14FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 14 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap14FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 15 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap15FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 15 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap15FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 15 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap15FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 16 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap16FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 16 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap16FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 16 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap16FilterType, 0, STR16("Filter"));

    // Per-tap pitch shift parameters
    parameters.addParameter(STR16("Tap 1 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap1PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 2 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap2PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 3 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap3PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 4 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap4PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 5 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap5PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 6 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap6PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 7 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap7PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 8 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap8PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 9 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap9PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 10 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap10PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 11 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap11PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 12 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap12PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 13 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap13PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 14 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap14PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 15 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap15PitchShift, 0, STR16("Pitch"));
    parameters.addParameter(STR16("Tap 16 Pitch Shift"), STR16("st"), 24, 0.5, Vst::ParameterInfo::kCanAutomate, kTap16PitchShift, 0, STR16("Pitch"));

    // Per-tap feedback send parameters
    parameters.addParameter(STR16("Tap 1 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap1FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 2 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap2FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 3 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap3FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 4 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap4FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 5 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap5FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 6 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap6FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 7 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap7FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 8 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap8FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 9 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap9FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 10 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap10FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 11 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap11FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 12 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap12FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 13 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap13FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 14 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap14FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 15 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap15FeedbackSend, 0, STR16("Feedback"));
    parameters.addParameter(STR16("Tap 16 Feedback Send"), STR16("%"), 0, 0.0, Vst::ParameterInfo::kCanAutomate, kTap16FeedbackSend, 0, STR16("Feedback"));

    // Global controls
    parameters.addParameter(STR16("Global Dry/Wet"), STR16("%"), 0, 0.5,
                           Vst::ParameterInfo::kCanAutomate, kGlobalDryWet, 0,
                           STR16("Mix"));

    parameters.addParameter(STR16("Delay Bypass"), nullptr, 1, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kDelayBypass, 0,
                           STR16("Control"));

    // Discrete control parameters (24 parameters)
    // Use static strings for VST3 parameter names to avoid dynamic allocation issues
    const Steinberg::Vst::TChar* discreteNames[24] = {
        STR16("Discrete 1"), STR16("Discrete 2"), STR16("Discrete 3"), STR16("Discrete 4"),
        STR16("Discrete 5"), STR16("Discrete 6"), STR16("Discrete 7"), STR16("Discrete 8"),
        STR16("Discrete 9"), STR16("Discrete 10"), STR16("Discrete 11"), STR16("Discrete 12"),
        STR16("Discrete 13"), STR16("Discrete 14"), STR16("Discrete 15"), STR16("Discrete 16"),
        STR16("Discrete 17"), STR16("Discrete 18"), STR16("Discrete 19"), STR16("Discrete 20"),
        STR16("Discrete 21"), STR16("Discrete 22"), STR16("Discrete 23"), STR16("Discrete 24")
    };

    for (int i = 0; i < 24; i++) {
        parameters.addParameter(discreteNames[i], STR16("%"), 0, 0.0,
                               Vst::ParameterInfo::kCanAutomate, kDiscrete1 + i, 0,
                               STR16("Discrete"));
    }

    // Macro curve type parameters
    parameters.addParameter(STR16("Macro Curve 1 Type"), nullptr, kNumCurveTypes - 1, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kMacroCurve1Type, 0,
                           STR16("Macro"));
    parameters.addParameter(STR16("Macro Curve 2 Type"), nullptr, kNumCurveTypes - 1, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kMacroCurve2Type, 0,
                           STR16("Macro"));
    parameters.addParameter(STR16("Macro Curve 3 Type"), nullptr, kNumCurveTypes - 1, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kMacroCurve3Type, 0,
                           STR16("Macro"));
    parameters.addParameter(STR16("Macro Curve 4 Type"), nullptr, kNumCurveTypes - 1, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kMacroCurve4Type, 0,
                           STR16("Macro"));

    // Macro knob parameters - global curve controls
    parameters.addParameter(STR16("Macro Knob 1"), STR16("%"), 0, 0.0,
                           Vst::ParameterInfo::kCanAutomate, kMacroKnob1, 0,
                           STR16("Macro"));
    parameters.addParameter(STR16("Macro Knob 2"), STR16("%"), 0, 0.0,
                           Vst::ParameterInfo::kCanAutomate, kMacroKnob2, 0,
                           STR16("Macro"));
    parameters.addParameter(STR16("Macro Knob 3"), STR16("%"), 0, 0.0,
                           Vst::ParameterInfo::kCanAutomate, kMacroKnob3, 0,
                           STR16("Macro"));
    parameters.addParameter(STR16("Macro Knob 4"), STR16("%"), 0, 0.0,
                           Vst::ParameterInfo::kCanAutomate, kMacroKnob4, 0,
                           STR16("Macro"));
    parameters.addParameter(STR16("Macro Knob 5"), STR16("%"), 0, 0.0,
                           Vst::ParameterInfo::kCanAutomate, kMacroKnob5, 0,
                           STR16("Macro"));
    parameters.addParameter(STR16("Macro Knob 6"), STR16("%"), 0, 0.0,
                           Vst::ParameterInfo::kCanAutomate, kMacroKnob6, 0,
                           STR16("Macro"));
    parameters.addParameter(STR16("Macro Knob 7"), STR16("%"), 0, 0.0,
                           Vst::ParameterInfo::kCanAutomate, kMacroKnob7, 0,
                           STR16("Macro"));
    parameters.addParameter(STR16("Macro Knob 8"), STR16("%"), 0, 0.0,
                           Vst::ParameterInfo::kCanAutomate, kMacroKnob8, 0,
                           STR16("Macro"));

    // Randomization and reset control parameters
    parameters.addParameter(STR16("Randomization Seed"), nullptr, 0, 0.0,
                           Vst::ParameterInfo::kCanAutomate, kRandomizeSeed, 0,
                           STR16("System"));
    parameters.addParameter(STR16("Randomization Amount"), STR16("%"), 0, 1.0,
                           Vst::ParameterInfo::kCanAutomate, kRandomizeAmount, 0,
                           STR16("System"));
    parameters.addParameter(STR16("Randomize Trigger"), nullptr, 1, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kRandomizeTrigger, 0,
                           STR16("System"));
    parameters.addParameter(STR16("Reset Trigger"), nullptr, 1, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kResetTrigger, 0,
                           STR16("System"));

    // Initialize all parameters to their default values
    // This ensures proper display even if setComponentState is never called
    setDefaultParameters();



    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::terminate()
{
    return EditControllerEx1::terminate();
}

//------------------------------------------------------------------------
void WaterStickController::setDefaultParameters()
{
    // Set global parameters to their design defaults
    setParamNormalized(kInputGain, 40.0/52.0);   // 0dB
    setParamNormalized(kOutputGain, 40.0/52.0);  // 0dB
    setParamNormalized(kDelayTime, 0.05);        // 50ms
    setParamNormalized(kFeedback, 0.0);          // 0%
    setParamNormalized(kTempoSyncMode, 0.0);     // Free mode
    setParamNormalized(kSyncDivision, static_cast<Vst::ParamValue>(kSync_1_4) / (kNumSyncDivisions - 1)); // 1/4 note
    setParamNormalized(kGrid, static_cast<Vst::ParamValue>(kGrid_4) / (kNumGridValues - 1)); // 4 grid

    // Initialize all tap parameters to their defaults
    for (int i = 0; i < 16; i++) {
        setParamNormalized(kTap1Enable + (i * 3), 0.0);   // Disabled
        setParamNormalized(kTap1Level + (i * 3), 0.8);    // 80% level
        setParamNormalized(kTap1Pan + (i * 3), 0.5);      // Center pan
        setParamNormalized(kTap1FilterCutoff + (i * 3), 0.566323334778673);  // 1kHz
        setParamNormalized(kTap1FilterResonance + (i * 3), 0.5);  // Moderate resonance
        setParamNormalized(kTap1FilterType + (i * 3), 0.0);       // Bypass filter
    }

    // Set global mix parameters to defaults
    setParamNormalized(kGlobalDryWet, 0.5);      // 50%
    setParamNormalized(kDelayBypass, 0.0);       // Active
}

//------------------------------------------------------------------------
bool WaterStickController::isValidParameterValue(Vst::ParamID id, float value)
{
    // Check if parameter value is within expected range
    if (std::isnan(value) || std::isinf(value)) {
        return false;
    }

    // Most normalized parameters should be 0.0-1.0
    if (value < 0.0f || value > 1.0f) {
        return false;
    }

    // Additional specific validation for certain parameter types
    if (id >= kTap1Level && id <= kTap16Level && ((id - kTap1Level) % 3 == 0)) {
        // Tap levels: 0.0-1.0
        return true;  // Already validated above
    }

    if (id >= kTap1Pan && id <= kTap16Pan && ((id - kTap1Pan) % 3 == 0)) {
        // Tap pans: 0.0-1.0
        return true;  // Already validated above
    }

    if (id >= kTap1FilterCutoff && id <= kTap16FilterCutoff && ((id - kTap1FilterCutoff) % 3 == 0)) {
        // Filter cutoffs: 0.0-1.0
        return true;  // Already validated above
    }

    if (id >= kTap1FilterType && id <= kTap16FilterType && ((id - kTap1FilterType) % 3 == 2)) {
        // Filter types: 0.0-1.0 (but should normalize to valid enum values)
        return true;  // Already validated above
    }

    if (id == kInputGain || id == kOutputGain) {
        // Input/Output gains: 0.0-1.0
        return true;  // Already validated above
    }

    return true;  // Default case: basic 0.0-1.0 range is valid
}

//------------------------------------------------------------------------
float WaterStickController::getDefaultParameterValue(Vst::ParamID id)
{
    // Return appropriate default for specific parameter types
    if (id >= kTap1Level && id <= kTap16Level && ((id - kTap1Level) % 3 == 0)) {
        return 0.8f;  // Tap levels default to 80%
    }

    if (id >= kTap1Pan && id <= kTap16Pan && ((id - kTap1Pan) % 3 == 0)) {
        return 0.5f;  // Tap pans default to center
    }

    if (id >= kTap1FilterCutoff && id <= kTap16FilterCutoff && ((id - kTap1FilterCutoff) % 3 == 0)) {
        return 0.566323334778673f;  // Filter cutoffs default to 1kHz
    }

    if (id >= kTap1FilterType && id <= kTap16FilterType && ((id - kTap1FilterType) % 3 == 2)) {
        return 0.0f;  // Filter types default to Bypass
    }

    if (id == kInputGain || id == kOutputGain) {
        return 40.0f/52.0f;  // Input/Output gains default to 0dB
    }

    if (id >= kTap1FilterResonance && id <= kTap16FilterResonance && ((id - kTap1FilterResonance) % 3 == 1)) {
        return 0.5f;  // Filter resonance default to moderate
    }

    // Global parameters
    if (id == kDelayTime) return 0.05f;
    if (id == kFeedback) return 0.0f;
    if (id == kTempoSyncMode) return 0.0f;
    if (id == kSyncDivision) return static_cast<float>(kSync_1_4) / (kNumSyncDivisions - 1);
    if (id == kGrid) return static_cast<float>(kGrid_4) / (kNumGridValues - 1);
    if (id >= kTap1Enable && id <= kTap16Enable && ((id - kTap1Enable) % 3 == 0)) return 0.0f;
    if (id >= kTap1FeedbackSend && id <= kTap16FeedbackSend) return 0.0f;  // Feedback sends default to 0%
    if (id == kGlobalDryWet) return 0.5f;
    if (id == kDelayBypass) return 0.0f;

    return 0.0f;  // Safe default
}

//------------------------------------------------------------------------
bool WaterStickController::isSemanticallySuspiciousState()
{
    // Check for patterns that indicate corrupted/old cache state

    // Pattern 1: All tap levels are 0.0 (likely corrupted)
    bool allLevelsZero = true;
    for (int i = 0; i < 16; i++) {
        float level = getParamNormalized(kTap1Level + (i * 3));
        if (level > 0.001f) {  // Use small epsilon for floating point comparison
            allLevelsZero = false;
            break;
        }
    }

    if (allLevelsZero) {
        return true;
    }

    // Pattern 2: All gains are exactly 1.0 (likely from old development state)
    float inputGain = getParamNormalized(kInputGain);
    float outputGain = getParamNormalized(kOutputGain);
    if (std::abs(inputGain - 1.0f) < 0.001f && std::abs(outputGain - 1.0f) < 0.001f) {
        return true;
    }

    // Pattern 3: No taps enabled (likely fresh instance)
    bool anyTapEnabled = false;
    for (int i = 0; i < 16; i++) {
        float enabled = getParamNormalized(kTap1Enable + (i * 3));
        if (enabled > 0.5f) {
            anyTapEnabled = true;
            break;
        }
    }

    if (!anyTapEnabled) {
        return true;
    }

    // Pattern 4: All filter types are not bypass (unusual for default state)
    bool allFiltersNonBypass = true;
    for (int i = 0; i < 16; i++) {
        float filterType = getParamNormalized(kTap1FilterType + (i * 3));
        if (filterType < 0.001f) {  // 0.0 is bypass
            allFiltersNonBypass = false;
            break;
        }
    }

    if (allFiltersNonBypass) {
        return true;
    }

    return false;  // State appears semantically valid
}

//------------------------------------------------------------------------
bool WaterStickController::hasValidStateSignature(IBStream* state, bool& hasSignature)
{
    hasSignature = false;

    if (!state) {
        return false;
    }

    // Save current position
    int64 originalPos;
    state->tell(&originalPos);

    // Go to beginning
    state->seek(0, IBStream::kIBSeekSet, nullptr);

    IBStreamer streamer(state, kLittleEndian);

    // Try to read version first
    Steinberg::int32 version;
    if (!streamer.readInt32(version)) {
        // Reset position and return
        state->seek(originalPos, IBStream::kIBSeekSet, nullptr);
        return false;
    }

    // Check if version is reasonable
    if (version < kStateVersionLegacy || version > kStateVersionCurrent) {
        // This is probably not a versioned state
        state->seek(originalPos, IBStream::kIBSeekSet, nullptr);
        return false;
    }

    // Try to read signature
    Steinberg::int32 signature;
    if (!streamer.readInt32(signature)) {
        // No signature present (old format)
        state->seek(originalPos, IBStream::kIBSeekSet, nullptr);
        return false;
    }

    hasSignature = true;

    // Check if signature matches
    bool isValid = (signature == kStateMagicNumber);

    // Reset position
    state->seek(originalPos, IBStream::kIBSeekSet, nullptr);


    return isValid;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::setComponentState(IBStream* state)
{

    if (!state) {
        setDefaultParameters();
        return kResultOk;
    }

    // Check for valid state signature first
    bool hasSignature = false;
    bool validSignature = hasValidStateSignature(state, hasSignature);

    if (hasSignature && !validSignature) {
        setDefaultParameters();
        return kResultOk;
    }

    // Try to read state version first
    Steinberg::int32 stateVersion;
    tresult readResult;

    if (tryReadStateVersion(state, stateVersion)) {
        readResult = readVersionedState(state, stateVersion);
    } else {
        // Reset stream position for legacy reading
        state->seek(0, IBStream::kIBSeekSet, nullptr);
        readResult = readLegacyState(state);
    }

    // After loading state, perform semantic validation
    if (readResult == kResultOk) {
        if (isSemanticallySuspiciousState()) {
            setDefaultParameters();
            return kResultOk;
        }
    }

    return readResult;
}

//------------------------------------------------------------------------
bool WaterStickController::tryReadStateVersion(IBStream* state, Steinberg::int32& version)
{
    IBStreamer streamer(state, kLittleEndian);

    // Try to read version header
    if (!streamer.readInt32(version)) {
        return false;
    }

    // Validate version is reasonable
    if (version < kStateVersionLegacy || version > kStateVersionCurrent) {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------
tresult WaterStickController::readLegacyState(IBStream* state)
{

    IBStreamer streamer(state, kLittleEndian);
    int invalidParameterCount = 0;
    int validParameterCount = 0;

    // Read and validate global parameters with individual fallback
    float inputGain, outputGain, delayTime, dryWet, feedback;
    bool tempoSyncMode;
    int32 syncDivision, grid;

    // Input Gain
    if (streamer.readFloat(inputGain) && isValidParameterValue(kInputGain, inputGain)) {
        setParamNormalized(kInputGain, inputGain);
        validParameterCount++;
    } else {
        float defaultValue = getDefaultParameterValue(kInputGain);
        setParamNormalized(kInputGain, defaultValue);
        invalidParameterCount++;
    }

    // Output Gain
    if (streamer.readFloat(outputGain) && isValidParameterValue(kOutputGain, outputGain)) {
        setParamNormalized(kOutputGain, outputGain);
        validParameterCount++;
    } else {
        float defaultValue = getDefaultParameterValue(kOutputGain);
        setParamNormalized(kOutputGain, defaultValue);
        invalidParameterCount++;
    }

    // Delay Time
    if (streamer.readFloat(delayTime) && delayTime >= 0.0f && delayTime <= 2.0f) {
        setParamNormalized(kDelayTime, delayTime / 2.0f); // Normalize 0-2s to 0-1
        validParameterCount++;
    } else {
        setParamNormalized(kDelayTime, getDefaultParameterValue(kDelayTime));
        invalidParameterCount++;
    }

    // Dry/Wet
    if (streamer.readFloat(dryWet) && isValidParameterValue(kGlobalDryWet, dryWet)) {
        setParamNormalized(kGlobalDryWet, dryWet);
        validParameterCount++;
    } else {
        setParamNormalized(kGlobalDryWet, getDefaultParameterValue(kGlobalDryWet));
        invalidParameterCount++;
    }

    // Feedback
    if (streamer.readFloat(feedback) && isValidParameterValue(kFeedback, feedback)) {
        setParamNormalized(kFeedback, feedback);
        validParameterCount++;
    } else {
        setParamNormalized(kFeedback, getDefaultParameterValue(kFeedback));
        invalidParameterCount++;
    }

    // Tempo Sync Mode
    if (streamer.readBool(tempoSyncMode)) {
        setParamNormalized(kTempoSyncMode, tempoSyncMode ? 1.0 : 0.0);
        validParameterCount++;
    } else {
        setParamNormalized(kTempoSyncMode, getDefaultParameterValue(kTempoSyncMode));
        invalidParameterCount++;
    }

    // Sync Division
    if (streamer.readInt32(syncDivision) && syncDivision >= 0 && syncDivision < kNumSyncDivisions) {
        setParamNormalized(kSyncDivision, static_cast<Vst::ParamValue>(syncDivision) / (kNumSyncDivisions - 1));
        validParameterCount++;
    } else {
        setParamNormalized(kSyncDivision, getDefaultParameterValue(kSyncDivision));
        invalidParameterCount++;
    }

    // Grid
    if (streamer.readInt32(grid) && grid >= 0 && grid < kNumGridValues) {
        setParamNormalized(kGrid, static_cast<Vst::ParamValue>(grid) / (kNumGridValues - 1));
        validParameterCount++;
    } else {
        setParamNormalized(kGrid, getDefaultParameterValue(kGrid));
        invalidParameterCount++;
    }

    // Load all tap parameters with individual validation
    for (int i = 0; i < 16; i++) {
        bool tapEnabled;
        float tapLevel, tapPan;
        float tapFilterCutoff, tapFilterResonance;
        int32 tapFilterType;

        // Tap Enable
        if (streamer.readBool(tapEnabled)) {
            setParamNormalized(kTap1Enable + (i * 3), tapEnabled ? 1.0 : 0.0);
            validParameterCount++;
        } else {
            setParamNormalized(kTap1Enable + (i * 3), getDefaultParameterValue(kTap1Enable + (i * 3)));
            invalidParameterCount++;
        }

        // Tap Level
        if (streamer.readFloat(tapLevel) && isValidParameterValue(kTap1Level + (i * 3), tapLevel)) {
            setParamNormalized(kTap1Level + (i * 3), tapLevel);
            validParameterCount++;
        } else {
            float defaultValue = getDefaultParameterValue(kTap1Level + (i * 3));
            setParamNormalized(kTap1Level + (i * 3), defaultValue);
            invalidParameterCount++;
            }

        // Tap Pan
        if (streamer.readFloat(tapPan) && isValidParameterValue(kTap1Pan + (i * 3), tapPan)) {
            setParamNormalized(kTap1Pan + (i * 3), tapPan);
            validParameterCount++;
        } else {
            float defaultValue = getDefaultParameterValue(kTap1Pan + (i * 3));
            setParamNormalized(kTap1Pan + (i * 3), defaultValue);
            invalidParameterCount++;
            }

        // Tap Filter Cutoff
        if (streamer.readFloat(tapFilterCutoff) && tapFilterCutoff >= 20.0f && tapFilterCutoff <= 20000.0f) {
            // Convert frequency to normalized value
            float normalizedCutoff = std::log(tapFilterCutoff / 20.0f) / std::log(1000.0f);
            if (isValidParameterValue(kTap1FilterCutoff + (i * 3), normalizedCutoff)) {
                setParamNormalized(kTap1FilterCutoff + (i * 3), normalizedCutoff);
                validParameterCount++;
            } else {
                float defaultValue = getDefaultParameterValue(kTap1FilterCutoff + (i * 3));
                setParamNormalized(kTap1FilterCutoff + (i * 3), defaultValue);
                invalidParameterCount++;
                }
        } else {
            float defaultValue = getDefaultParameterValue(kTap1FilterCutoff + (i * 3));
            setParamNormalized(kTap1FilterCutoff + (i * 3), defaultValue);
            invalidParameterCount++;
            }

        // Tap Filter Resonance
        if (streamer.readFloat(tapFilterResonance) && tapFilterResonance >= -1.0f && tapFilterResonance <= 1.0f) {
            // Convert resonance to normalized value
            float normalizedResonance;
            if (tapFilterResonance >= 0.0f) {
                normalizedResonance = 0.5f + 0.5f * std::cbrt(tapFilterResonance);
            } else {
                normalizedResonance = 0.5f + tapFilterResonance / 2.0f;
            }

            if (isValidParameterValue(kTap1FilterResonance + (i * 3), normalizedResonance)) {
                setParamNormalized(kTap1FilterResonance + (i * 3), normalizedResonance);
                validParameterCount++;
            } else {
                float defaultValue = getDefaultParameterValue(kTap1FilterResonance + (i * 3));
                setParamNormalized(kTap1FilterResonance + (i * 3), defaultValue);
                invalidParameterCount++;
            }
        } else {
            float defaultValue = getDefaultParameterValue(kTap1FilterResonance + (i * 3));
            setParamNormalized(kTap1FilterResonance + (i * 3), defaultValue);
            invalidParameterCount++;
        }

        // Tap Filter Type
        if (streamer.readInt32(tapFilterType) && tapFilterType >= 0 && tapFilterType < kNumFilterTypes) {
            float normalizedType = static_cast<Vst::ParamValue>(tapFilterType) / (kNumFilterTypes - 1);
            if (isValidParameterValue(kTap1FilterType + (i * 3), normalizedType)) {
                setParamNormalized(kTap1FilterType + (i * 3), normalizedType);
                validParameterCount++;
            } else {
                float defaultValue = getDefaultParameterValue(kTap1FilterType + (i * 3));
                setParamNormalized(kTap1FilterType + (i * 3), defaultValue);
                invalidParameterCount++;
                }
        } else {
            float defaultValue = getDefaultParameterValue(kTap1FilterType + (i * 3));
            setParamNormalized(kTap1FilterType + (i * 3), defaultValue);
            invalidParameterCount++;
            }
    }

    // Load global control parameters with individual validation
    float globalDryWet;
    bool delayBypass;

    // Global Dry/Wet
    if (streamer.readFloat(globalDryWet) && isValidParameterValue(kGlobalDryWet, globalDryWet)) {
        setParamNormalized(kGlobalDryWet, globalDryWet);
        validParameterCount++;
    } else {
        setParamNormalized(kGlobalDryWet, getDefaultParameterValue(kGlobalDryWet));
        invalidParameterCount++;
    }

    // Delay Bypass
    if (streamer.readBool(delayBypass)) {
        setParamNormalized(kDelayBypass, delayBypass ? 1.0 : 0.0);
        validParameterCount++;
    } else {
        setParamNormalized(kDelayBypass, getDefaultParameterValue(kDelayBypass));
        invalidParameterCount++;
    }

    return kResultOk;
}

//------------------------------------------------------------------------
tresult WaterStickController::readCurrentVersionState(IBStream* state)
{

    IBStreamer streamer(state, kLittleEndian);
    int invalidParameterCount = 0;
    int validParameterCount = 0;

    // Read and validate signature (version was already read)
    Steinberg::int32 signature;
    if (streamer.readInt32(signature)) {
        if (signature == kStateMagicNumber) {
        } else {
            // Continue reading but this is suspicious
        }
    } else {
        // Reset stream position and continue without signature
        state->seek(-4, IBStream::kIBSeekCur, nullptr);
    }

    // Read and validate global parameters with individual fallback
    float inputGain, outputGain, delayTime, dryWet, feedback;
    bool tempoSyncMode;
    int32 syncDivision, grid;

    // Input Gain
    if (streamer.readFloat(inputGain) && isValidParameterValue(kInputGain, inputGain)) {
        setParamNormalized(kInputGain, inputGain);
        validParameterCount++;
    } else {
        float defaultValue = getDefaultParameterValue(kInputGain);
        setParamNormalized(kInputGain, defaultValue);
        invalidParameterCount++;
    }

    // Output Gain
    if (streamer.readFloat(outputGain) && isValidParameterValue(kOutputGain, outputGain)) {
        setParamNormalized(kOutputGain, outputGain);
        validParameterCount++;
    } else {
        float defaultValue = getDefaultParameterValue(kOutputGain);
        setParamNormalized(kOutputGain, defaultValue);
        invalidParameterCount++;
    }

    // Delay Time
    if (streamer.readFloat(delayTime) && delayTime >= 0.0f && delayTime <= 2.0f) {
        setParamNormalized(kDelayTime, delayTime / 2.0f); // Normalize 0-2s to 0-1
        validParameterCount++;
    } else {
        setParamNormalized(kDelayTime, getDefaultParameterValue(kDelayTime));
        invalidParameterCount++;
    }

    // Dry/Wet
    if (streamer.readFloat(dryWet) && isValidParameterValue(kGlobalDryWet, dryWet)) {
        setParamNormalized(kGlobalDryWet, dryWet);
        validParameterCount++;
    } else {
        setParamNormalized(kGlobalDryWet, getDefaultParameterValue(kGlobalDryWet));
        invalidParameterCount++;
    }

    // Feedback
    if (streamer.readFloat(feedback) && isValidParameterValue(kFeedback, feedback)) {
        setParamNormalized(kFeedback, feedback);
        validParameterCount++;
    } else {
        setParamNormalized(kFeedback, getDefaultParameterValue(kFeedback));
        invalidParameterCount++;
    }

    // Tempo Sync Mode
    if (streamer.readBool(tempoSyncMode)) {
        setParamNormalized(kTempoSyncMode, tempoSyncMode ? 1.0 : 0.0);
        validParameterCount++;
    } else {
        setParamNormalized(kTempoSyncMode, getDefaultParameterValue(kTempoSyncMode));
        invalidParameterCount++;
    }

    // Sync Division
    if (streamer.readInt32(syncDivision) && syncDivision >= 0 && syncDivision < kNumSyncDivisions) {
        setParamNormalized(kSyncDivision, static_cast<Vst::ParamValue>(syncDivision) / (kNumSyncDivisions - 1));
        validParameterCount++;
    } else {
        setParamNormalized(kSyncDivision, getDefaultParameterValue(kSyncDivision));
        invalidParameterCount++;
    }

    // Grid
    if (streamer.readInt32(grid) && grid >= 0 && grid < kNumGridValues) {
        setParamNormalized(kGrid, static_cast<Vst::ParamValue>(grid) / (kNumGridValues - 1));
        validParameterCount++;
    } else {
        setParamNormalized(kGrid, getDefaultParameterValue(kGrid));
        invalidParameterCount++;
    }

    // Load all tap parameters with individual validation
    for (int i = 0; i < 16; i++) {
        bool tapEnabled;
        float tapLevel, tapPan;
        float tapFilterCutoff, tapFilterResonance;
        int32 tapFilterType;

        // Tap Enable
        if (streamer.readBool(tapEnabled)) {
            setParamNormalized(kTap1Enable + (i * 3), tapEnabled ? 1.0 : 0.0);
            validParameterCount++;
        } else {
            setParamNormalized(kTap1Enable + (i * 3), getDefaultParameterValue(kTap1Enable + (i * 3)));
            invalidParameterCount++;
        }

        // Tap Level
        if (streamer.readFloat(tapLevel) && isValidParameterValue(kTap1Level + (i * 3), tapLevel)) {
            setParamNormalized(kTap1Level + (i * 3), tapLevel);
            validParameterCount++;
        } else {
            float defaultValue = getDefaultParameterValue(kTap1Level + (i * 3));
            setParamNormalized(kTap1Level + (i * 3), defaultValue);
            invalidParameterCount++;
            }

        // Tap Pan
        if (streamer.readFloat(tapPan) && isValidParameterValue(kTap1Pan + (i * 3), tapPan)) {
            setParamNormalized(kTap1Pan + (i * 3), tapPan);
            validParameterCount++;
        } else {
            float defaultValue = getDefaultParameterValue(kTap1Pan + (i * 3));
            setParamNormalized(kTap1Pan + (i * 3), defaultValue);
            invalidParameterCount++;
            }

        // Tap Filter Cutoff
        if (streamer.readFloat(tapFilterCutoff) && tapFilterCutoff >= 20.0f && tapFilterCutoff <= 20000.0f) {
            // Convert frequency to normalized value
            float normalizedCutoff = std::log(tapFilterCutoff / 20.0f) / std::log(1000.0f);
            if (isValidParameterValue(kTap1FilterCutoff + (i * 3), normalizedCutoff)) {
                setParamNormalized(kTap1FilterCutoff + (i * 3), normalizedCutoff);
                validParameterCount++;
            } else {
                float defaultValue = getDefaultParameterValue(kTap1FilterCutoff + (i * 3));
                setParamNormalized(kTap1FilterCutoff + (i * 3), defaultValue);
                invalidParameterCount++;
                }
        } else {
            float defaultValue = getDefaultParameterValue(kTap1FilterCutoff + (i * 3));
            setParamNormalized(kTap1FilterCutoff + (i * 3), defaultValue);
            invalidParameterCount++;
            }

        // Tap Filter Resonance
        if (streamer.readFloat(tapFilterResonance) && tapFilterResonance >= -1.0f && tapFilterResonance <= 1.0f) {
            // Convert resonance to normalized value
            float normalizedResonance;
            if (tapFilterResonance >= 0.0f) {
                normalizedResonance = 0.5f + 0.5f * std::cbrt(tapFilterResonance);
            } else {
                normalizedResonance = 0.5f + tapFilterResonance / 2.0f;
            }

            if (isValidParameterValue(kTap1FilterResonance + (i * 3), normalizedResonance)) {
                setParamNormalized(kTap1FilterResonance + (i * 3), normalizedResonance);
                validParameterCount++;
            } else {
                float defaultValue = getDefaultParameterValue(kTap1FilterResonance + (i * 3));
                setParamNormalized(kTap1FilterResonance + (i * 3), defaultValue);
                invalidParameterCount++;
            }
        } else {
            float defaultValue = getDefaultParameterValue(kTap1FilterResonance + (i * 3));
            setParamNormalized(kTap1FilterResonance + (i * 3), defaultValue);
            invalidParameterCount++;
        }

        // Tap Filter Type
        if (streamer.readInt32(tapFilterType) && tapFilterType >= 0 && tapFilterType < kNumFilterTypes) {
            float normalizedType = static_cast<Vst::ParamValue>(tapFilterType) / (kNumFilterTypes - 1);
            if (isValidParameterValue(kTap1FilterType + (i * 3), normalizedType)) {
                setParamNormalized(kTap1FilterType + (i * 3), normalizedType);
                validParameterCount++;
            } else {
                float defaultValue = getDefaultParameterValue(kTap1FilterType + (i * 3));
                setParamNormalized(kTap1FilterType + (i * 3), defaultValue);
                invalidParameterCount++;
                }
        } else {
            float defaultValue = getDefaultParameterValue(kTap1FilterType + (i * 3));
            setParamNormalized(kTap1FilterType + (i * 3), defaultValue);
            invalidParameterCount++;
            }
    }

    // Load global control parameters with individual validation
    float globalDryWet;
    bool delayBypass;

    // Global Dry/Wet
    if (streamer.readFloat(globalDryWet) && isValidParameterValue(kGlobalDryWet, globalDryWet)) {
        setParamNormalized(kGlobalDryWet, globalDryWet);
        validParameterCount++;
    } else {
        setParamNormalized(kGlobalDryWet, getDefaultParameterValue(kGlobalDryWet));
        invalidParameterCount++;
    }

    // Delay Bypass
    if (streamer.readBool(delayBypass)) {
        setParamNormalized(kDelayBypass, delayBypass ? 1.0 : 0.0);
        validParameterCount++;
    } else {
        setParamNormalized(kDelayBypass, getDefaultParameterValue(kDelayBypass));
        invalidParameterCount++;
    }

    return kResultOk;
}

//------------------------------------------------------------------------
tresult WaterStickController::readVersionedState(IBStream* state, Steinberg::int32 version)
{

    switch (version) {
        case kStateVersionCurrent:
            // Version 1 state format - use proper version 1 reading with enhanced validation
            return readCurrentVersionState(state);

        default:
            setDefaultParameters();
            return kResultOk;
    }

    return kResultOk;
}

//------------------------------------------------------------------------
Vst::ParamValue PLUGIN_API WaterStickController::getParamNormalized(Vst::ParamID id)
{
    Vst::ParamValue value = EditControllerEx1::getParamNormalized(id);

    bool shouldLog = false;
    std::string paramName;

    // Check if this is a problematic parameter we want to track
    if (id >= kTap1FilterType && id <= kTap16FilterType && ((id - kTap1FilterType) % 3 == 2)) {
        // Filter type parameter
        int tapIndex = (id - kTap1FilterType) / 3;
        paramName = "Tap" + std::to_string(tapIndex + 1) + "FilterType";
        shouldLog = true;
    } else if (id >= kTap1Level && id <= kTap16Level && ((id - kTap1Level) % 3 == 0)) {
        // Level parameter
        int tapIndex = (id - kTap1Level) / 3;
        paramName = "Tap" + std::to_string(tapIndex + 1) + "Level";
        shouldLog = true;
    } else if (id >= kTap1Pan && id <= kTap16Pan && ((id - kTap1Pan) % 3 == 0)) {
        // Pan parameter
        int tapIndex = (id - kTap1Pan) / 3;
        paramName = "Tap" + std::to_string(tapIndex + 1) + "Pan";
        shouldLog = true;
    } else if (id >= kTap1FilterCutoff && id <= kTap16FilterCutoff && ((id - kTap1FilterCutoff) % 3 == 0)) {
        // Filter cutoff parameter
        int tapIndex = (id - kTap1FilterCutoff) / 3;
        paramName = "Tap" + std::to_string(tapIndex + 1) + "FilterCutoff";
        shouldLog = true;
    } else if (id == kInputGain || id == kOutputGain) {
        paramName = (id == kInputGain) ? "InputGain" : "OutputGain";
        shouldLog = true;
    }


    return value;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::setParamNormalized(Vst::ParamID id, Vst::ParamValue value)
{
    tresult result = EditControllerEx1::setParamNormalized(id, value);

    // Handle system triggers
    if (id == kRandomizeTrigger && value > 0.5f) {
        handleRandomizeTrigger();
    } else if (id == kResetTrigger && value > 0.5f) {
        handleResetTrigger();
    }

    // Update curve types
    if (id >= kMacroCurve1Type && id <= kMacroCurve4Type) {
        updateCurveTypes();
    }

    // Update discrete parameters
    if (id >= kDiscrete1 && id <= kDiscrete24) {
        updateDiscreteParameters();
    }

    // Handle macro knob parameter changes for automation support
    if (id >= kMacroKnob1 && id <= kMacroKnob8) {
        handleMacroKnobParameterChange(id, value);
    }

    // Update randomization settings
    if (id == kRandomizeSeed) {
        unsigned int seed = static_cast<unsigned int>(value * 4294967295.0); // Max uint32
        mRandomizationEngine.setSeed(seed);
    } else if (id == kRandomizeAmount) {
        mRandomizationEngine.setAmount(static_cast<float>(value));
    }

    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::getParamStringByValue(Vst::ParamID id, Vst::ParamValue valueNormalized, Vst::String128 string)
{
    switch (id)
    {
        case kTempoSyncMode:
        {
            const char* text = (valueNormalized > 0.5) ? "Synced" : "Free";
            Steinberg::UString(string, 128).fromAscii(text);
            return kResultTrue;
        }
        case kSyncDivision:
        {
            int division = static_cast<int>(valueNormalized * (kNumSyncDivisions - 1) + 0.5);
            if (division >= 0 && division < kNumSyncDivisions) {
                // Division text lookup table (matches TempoSync class)
                static const char* divisionTexts[kNumSyncDivisions] = {
                    "1/64", "1/32T", "1/64.", "1/32", "1/16T", "1/32.", "1/16",
                    "1/8T", "1/16.", "1/8", "1/4T", "1/8.", "1/4",
                    "1/2T", "1/4.", "1/2", "1T", "1/2.", "1", "2", "4", "8"
                };
                Steinberg::UString(string, 128).fromAscii(divisionTexts[division]);
                return kResultTrue;
            }
            break;
        }
        case kGrid:
        {
            int grid = static_cast<int>(valueNormalized * (kNumGridValues - 1) + 0.5);
            if (grid >= 0 && grid < kNumGridValues) {
                // Grid text lookup table (matches TapDistribution class)
                static const char* gridTexts[kNumGridValues] = {
                    "1", "2", "3", "4", "6", "8", "12", "16"
                };
                Steinberg::UString(string, 128).fromAscii(gridTexts[grid]);
                return kResultTrue;
            }
            break;
        }
        case kFeedback:
        {
            // Convert normalized value to percentage for display
            float percentage = valueNormalized * 100.0f;
            char percentText[128];
            snprintf(percentText, sizeof(percentText), "%.1f%%", percentage);
            Steinberg::UString(string, 128).fromAscii(percentText);
            return kResultTrue;
        }
        case kGlobalDryWet:
        {
            // Convert normalized value to percentage for display
            float percentage = valueNormalized * 100.0f;
            char percentText[128];
            snprintf(percentText, sizeof(percentText), "%.1f%%", percentage);
            Steinberg::UString(string, 128).fromAscii(percentText);
            return kResultTrue;
        }
        case kDelayBypass:
        {
            const char* text = (valueNormalized > 0.5) ? "Bypassed" : "Active";
            Steinberg::UString(string, 128).fromAscii(text);
            return kResultTrue;
        }
        case kMacroCurve1Type:
        case kMacroCurve2Type:
        case kMacroCurve3Type:
        case kMacroCurve4Type:
        {
            int curveType = static_cast<int>(valueNormalized * (kNumCurveTypes - 1) + 0.5);
            if (curveType >= 0 && curveType < kNumCurveTypes) {
                const char* curveName = mMacroCurveSystem.getCurveTypeName(static_cast<MacroCurveTypes>(curveType));
                Steinberg::UString(string, 128).fromAscii(curveName);
                return kResultTrue;
            }
            break;
        }
        case kRandomizeTrigger:
        {
            const char* text = (valueNormalized > 0.5) ? "Triggered" : "Idle";
            Steinberg::UString(string, 128).fromAscii(text);
            return kResultTrue;
        }
        case kResetTrigger:
        {
            const char* text = (valueNormalized > 0.5) ? "Triggered" : "Idle";
            Steinberg::UString(string, 128).fromAscii(text);
            return kResultTrue;
        }
        default:
        {
            // Handle macro knob parameters
            if (id >= kMacroKnob1 && id <= kMacroKnob8) {
                float percentage = valueNormalized * 100.0f;
                char macroText[128];
                snprintf(macroText, sizeof(macroText), "%.1f%%", percentage);
                Steinberg::UString(string, 128).fromAscii(macroText);
                return kResultTrue;
            }

            // Handle discrete parameters
            if (id >= kDiscrete1 && id <= kDiscrete24) {
                float percentage = valueNormalized * 100.0f;
                char discreteText[128];
                snprintf(discreteText, sizeof(discreteText), "%.1f%%", percentage);
                Steinberg::UString(string, 128).fromAscii(discreteText);
                return kResultTrue;
            }

            // Handle randomization parameters
            if (id == kRandomizeSeed) {
                unsigned int seed = static_cast<unsigned int>(valueNormalized * 4294967295.0);
                char seedText[128];
                snprintf(seedText, sizeof(seedText), "%u", seed);
                Steinberg::UString(string, 128).fromAscii(seedText);
                return kResultTrue;
            }

            if (id == kRandomizeAmount) {
                float percentage = valueNormalized * 100.0f;
                char amountText[128];
                snprintf(amountText, sizeof(amountText), "%.1f%%", percentage);
                Steinberg::UString(string, 128).fromAscii(amountText);
                return kResultTrue;
            }

            // Handle per-tap pitch shift parameters
            if (id >= kTap1PitchShift && id <= kTap16PitchShift) {
                int semitones = static_cast<int>(round((valueNormalized * 24.0) - 12.0));
                char pitchText[128];
                if (semitones == 0) {
                    snprintf(pitchText, sizeof(pitchText), "0 st");
                } else if (semitones > 0) {
                    snprintf(pitchText, sizeof(pitchText), "+%d st", semitones);
                } else {
                    snprintf(pitchText, sizeof(pitchText), "%d st", semitones);
                }
                Steinberg::UString(string, 128).fromAscii(pitchText);
                return kResultTrue;
            }

            // Handle per-tap feedback send parameters
            if (id >= kTap1FeedbackSend && id <= kTap16FeedbackSend) {
                float percentage = valueNormalized * 100.0f;
                char feedbackText[128];
                snprintf(feedbackText, sizeof(feedbackText), "%.1f%%", percentage);
                Steinberg::UString(string, 128).fromAscii(feedbackText);
                return kResultTrue;
            }

            // Handle per-tap filter parameters
            if (id >= kTap1FilterCutoff && id <= kTap16FilterType) {
                int paramOffset = id - kTap1FilterCutoff;
                int paramType = paramOffset % 3; // 0=cutoff, 1=resonance, 2=type

                switch (paramType) {
                    case 0: // Filter Cutoff
                    {
                        // Convert normalized value to frequency using same logarithmic scale as processor
                        float frequency = 20.0f * std::pow(1000.0f, valueNormalized); // 20Hz to 20kHz logarithmic

                        char freqText[128];
                        if (frequency < 1000.0f) {
                            snprintf(freqText, sizeof(freqText), "%.1f Hz", frequency);
                        } else {
                            snprintf(freqText, sizeof(freqText), "%.2f kHz", frequency / 1000.0f);
                        }
                        Steinberg::UString(string, 128).fromAscii(freqText);
                        return kResultTrue;
                    }
                    case 1: // Filter Resonance
                    {
                        // Convert normalized value to actual resonance using same scaling as processor
                        float resonance;
                        if (valueNormalized >= 0.5f) {
                            // Positive resonance with cubic curve
                            float positiveValue = (valueNormalized - 0.5f) * 2.0f;
                            resonance = positiveValue * positiveValue * positiveValue;
                        } else {
                            // Negative resonance with linear mapping
                            resonance = (valueNormalized - 0.5f) * 2.0f;
                        }

                        char resText[128];
                        snprintf(resText, sizeof(resText), "%.2f", resonance);
                        Steinberg::UString(string, 128).fromAscii(resText);
                        return kResultTrue;
                    }
                    case 2: // Filter Type
                    {
                        int filterType = static_cast<int>(valueNormalized * (kNumFilterTypes - 1) + 0.5);
                        if (filterType >= 0 && filterType < kNumFilterTypes) {
                            static const char* filterTypeTexts[kNumFilterTypes] = {
                                "Bypass", "Low Pass", "High Pass", "Band Pass", "Notch"
                            };
                            Steinberg::UString(string, 128).fromAscii(filterTypeTexts[filterType]);
                            return kResultTrue;
                        }
                        break;
                    }
                }
            }
            break;
        }
    }
    return EditControllerEx1::getParamStringByValue(id, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::getParamValueByString(Vst::ParamID id, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
    switch (id)
    {
        case kTempoSyncMode:
        {
            Steinberg::UString wrapper(string, tstrlen(string));
            Steinberg::UString syncedString(USTRING("Synced"));
            Steinberg::UString freeString(USTRING("Free"));
            if (wrapper == syncedString) {
                valueNormalized = 1.0;
                return kResultTrue;
            }
            if (wrapper == freeString) {
                valueNormalized = 0.0;
                return kResultTrue;
            }
            break;
        }
        case kSyncDivision:
        {
            // This could be implemented to parse division strings, but typically not needed
            // for VST3 hosts - they usually use the getParamStringByValue direction
            break;
        }
    }
    return EditControllerEx1::getParamValueByString(id, string, valueNormalized);
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API WaterStickController::createView(FIDString name)
{

    if (FIDStringsEqual(name, Vst::ViewType::kEditor))
    {

        // Log all problematic parameter values before GUI creation
        for (int i = 0; i < 16; i++) {
            int filterTypeId = kTap1FilterType + (i * 3);
            int levelId = kTap1Level + (i * 3);
            int panId = kTap1Pan + (i * 3);
            int cutoffId = kTap1FilterCutoff + (i * 3);

            std::ostringstream filterTypeName, levelName, panName, cutoffName;
            filterTypeName << "Tap" << (i+1) << "FilterType";
            levelName << "Tap" << (i+1) << "Level";
            panName << "Tap" << (i+1) << "Pan";
            cutoffName << "Tap" << (i+1) << "FilterCutoff";

        }



        auto* editor = new WaterStickEditor(this);
        return editor;
    }
    return nullptr;
}

//------------------------------------------------------------------------
// System Integration Methods
//------------------------------------------------------------------------
void WaterStickController::handleRandomizeTrigger()
{
    // Reset trigger parameter to idle state
    setParamNormalized(kRandomizeTrigger, 0.0);

    // Execute randomization
    mRandomizationEngine.randomizeAllParameters(this);
}

void WaterStickController::handleResetTrigger()
{
    // Reset trigger parameter to idle state
    setParamNormalized(kResetTrigger, 0.0);

    // Execute reset to defaults
    mDefaultResetSystem.resetAllParameters(this);
}

void WaterStickController::updateCurveTypes()
{
    // Update macro curve system with current parameter values
    for (int i = 0; i < 4; i++) {
        float curveValue = getParamNormalized(kMacroCurve1Type + i);
        int curveTypeIndex = static_cast<int>(curveValue * (kNumCurveTypes - 1) + 0.5);
        MacroCurveTypes curveType = static_cast<MacroCurveTypes>(curveTypeIndex);
        mMacroCurveSystem.setCurveType(i, curveType);
    }
}

void WaterStickController::handleMacroKnobParameterChange(Steinberg::Vst::ParamID paramId, Steinberg::Vst::ParamValue value)
{
    // Convert parameter ID to macro knob index (0-7)
    int macroKnobIndex = static_cast<int>(paramId - kMacroKnob1);
    if (macroKnobIndex < 0 || macroKnobIndex >= 8) return;

    // Convert normalized value (0.0-1.0) to discrete position (0-7)
    int discretePosition = static_cast<int>(value * 7.0f + 0.5f);
    if (discretePosition > 7) discretePosition = 7;

    // DIAGNOSTIC: Log macro knob parameter changes from DAW automation
    printf("[MacroKnobDAW] handleMacroKnobParameterChange - paramId: %d, macroKnobIndex: %d, value: %.3f, discretePos: %d, currentContext: %d\n",
           static_cast<int>(paramId), macroKnobIndex, value, discretePosition, mCurrentTapContext);

    // Apply Rainmaker-style global macro curve using the current synchronized context
    // This ensures DAW automation respects the currently active GUI context
    mMacroCurveSystem.applyGlobalMacroCurve(discretePosition, mCurrentTapContext, this);

    printf("[MacroKnobDAW] Applied global macro curve with context %d\n", mCurrentTapContext);
}

void WaterStickController::updateDiscreteParameters()
{
    // Update discrete parameter array with current values
    for (int i = 0; i < 24; i++) {
        mDiscreteParameters[i] = static_cast<float>(getParamNormalized(kDiscrete1 + i));
    }
}

void WaterStickController::applyParameterSmoothing()
{
    // Apply curve evaluation to discrete parameters in real-time
    // This method can be called from the processor for real-time parameter smoothing
    for (int i = 0; i < 24; i++) {
        float rawValue = mDiscreteParameters[i];

        // Apply curves if assigned (example: use first 4 curves for first 4 discrete parameters)
        if (i < 4) {
            float curvedValue = mMacroCurveSystem.applyCurveToParameter(i, rawValue);
            mDiscreteParameters[i] = curvedValue;
        }
    }
}

} // namespace WaterStick