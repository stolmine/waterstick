#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "WaterStickParameters.h"

namespace WaterStick {

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
};

} // namespace WaterStick