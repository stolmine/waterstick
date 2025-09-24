#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "WaterStickParameters.h"
#include <random>
#include <unordered_map>
#include <cmath>
#include <vector>

namespace WaterStick {

class RandomizationEngine {
public:
    RandomizationEngine();
    ~RandomizationEngine();

    void initialize(unsigned int seed = 0);
    void setSeed(unsigned int seed);
    void setAmount(float amount); // 0.0-1.0

    // Context-aware randomization with musical constraints
    float randomizeParameter(Steinberg::Vst::ParamID id, float currentValue, RandomizationConstraints constraint = kRandomConstraint_Full);
    void randomizeAllParameters(class WaterStickController* controller);
    void randomizeParameterGroup(class WaterStickController* controller, const std::vector<Steinberg::Vst::ParamID>& paramIds);

    // Musical constraint helpers
    float applyMusicalConstraint(float value, Steinberg::Vst::ParamID id);
    float applyConservativeConstraint(float value, float currentValue);
    float applyBipolarConstraint(float value, float currentValue);

private:
    std::mt19937 mGenerator;
    std::uniform_real_distribution<float> mDistribution;
    float mAmount;
    unsigned int mCurrentSeed;

    // Parameter type classification for intelligent randomization
    RandomizationConstraints classifyParameter(Steinberg::Vst::ParamID id);
    float generateConstrainedValue(RandomizationConstraints constraint, float currentValue, Steinberg::Vst::ParamID id);
};

class DefaultResetSystem {
public:
    DefaultResetSystem();
    ~DefaultResetSystem();

    void initialize();
    void resetAllParameters(class WaterStickController* controller);
    void resetParameterGroup(class WaterStickController* controller, const std::vector<Steinberg::Vst::ParamID>& paramIds);

    // Category-specific resets
    void resetGlobalParameters(class WaterStickController* controller);
    void resetTapParameters(class WaterStickController* controller);
    void resetFilterParameters(class WaterStickController* controller);
    void resetDiscreteParameters(class WaterStickController* controller);

    float getDefaultValue(Steinberg::Vst::ParamID id);

private:
    std::unordered_map<Steinberg::Vst::ParamID, float> mDefaultValues;
    void buildDefaultValueMap();
};

class MacroCurveSystem {
public:
    MacroCurveSystem();
    ~MacroCurveSystem();

    void initialize();
    void setCurveType(int curveIndex, MacroCurveTypes type); // curveIndex 0-3
    MacroCurveTypes getCurveType(int curveIndex) const;

    // Real-time curve evaluation with immediate application
    float evaluateCurve(MacroCurveTypes type, float input) const;
    float evaluateCurve(int curveIndex, float input) const;

    // Curve type names for UI
    const char* getCurveTypeName(MacroCurveTypes type) const;

    // Apply curves to parameter values in real-time
    float applyCurveToParameter(int curveIndex, float parameterValue) const;

private:
    MacroCurveTypes mCurveTypes[4]; // 4 macro curves

    // Individual curve implementations
    float evaluateLinear(float x) const;
    float evaluateExponential(float x) const;
    float evaluateInverseExponential(float x) const;
    float evaluateLogarithmic(float x) const;
    float evaluateInverseLogarithmic(float x) const;
    float evaluateSCurve(float x) const;
    float evaluateInverseSCurve(float x) const;
    float evaluateQuantized(float x) const;

    static const char* sCurveTypeNames[kNumCurveTypes];
};

class WaterStickController : public Steinberg::Vst::EditControllerEx1
{
public:
    WaterStickController();
    ~WaterStickController() SMTG_OVERRIDE;

    // Create function used by factory
    static Steinberg::FUnknown* createInstance(void* /*context*/)
    {
        return (Steinberg::Vst::IEditController*)new WaterStickController;
    }

    // IPluginBase
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

    // EditController
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;

    // IEditController
    Steinberg::Vst::ParamValue PLUGIN_API getParamNormalized(Steinberg::Vst::ParamID id) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getParamStringByValue(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized, Steinberg::Vst::String128 string) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getParamValueByString(Steinberg::Vst::ParamID id, Steinberg::Vst::TChar* string, Steinberg::Vst::ParamValue& valueNormalized) SMTG_OVERRIDE;

    // IPlugView creation
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;

private:
    // State version constants
    static constexpr Steinberg::int32 kStateVersionLegacy = 0;  // Legacy unversioned state
    static constexpr Steinberg::int32 kStateVersionCurrent = 1; // First versioned release

    // State signature for freshness detection (magic number)
    static constexpr Steinberg::int32 kStateMagicNumber = 0x57415453; // "WATS" in hex

    // Helper method to set all parameters to their default values
    void setDefaultParameters();

    // Enhanced parameter validation methods
    bool isValidParameterValue(Steinberg::Vst::ParamID id, float value);
    float getDefaultParameterValue(Steinberg::Vst::ParamID id);

    // Semantic validation for state freshness detection
    bool isSemanticallySuspiciousState();
    bool hasValidStateSignature(Steinberg::IBStream* state, bool& hasSignature);

    // State versioning methods
    bool tryReadStateVersion(Steinberg::IBStream* state, Steinberg::int32& version);
    Steinberg::tresult readLegacyState(Steinberg::IBStream* state);
    Steinberg::tresult readCurrentVersionState(Steinberg::IBStream* state);
    Steinberg::tresult readVersionedState(Steinberg::IBStream* state, Steinberg::int32 version);

    // Backend parameter architecture systems
    RandomizationEngine mRandomizationEngine;
    DefaultResetSystem mDefaultResetSystem;
    MacroCurveSystem mMacroCurveSystem;

    // System integration methods
    void handleRandomizeTrigger();
    void handleResetTrigger();
    void updateCurveTypes();

    // Discrete parameter management
    float mDiscreteParameters[24];
    void updateDiscreteParameters();
    void applyParameterSmoothing();

public:
    // Public access for processor integration
    RandomizationEngine& getRandomizationEngine() { return mRandomizationEngine; }
    DefaultResetSystem& getDefaultResetSystem() { return mDefaultResetSystem; }
    MacroCurveSystem& getMacroCurveSystem() { return mMacroCurveSystem; }
};

} // namespace WaterStick