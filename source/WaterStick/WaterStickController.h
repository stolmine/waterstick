#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

namespace WaterStick {

class WaterStickController : public Steinberg::Vst::EditController
{
public:
    WaterStickController();
    ~WaterStickController() SMTG_OVERRIDE;

    // Create function
    static Steinberg::FUnknown* createInstance(void* /*context*/)
    {
        return (Steinberg::Vst::IEditController*)new WaterStickController;
    }

    // IPluginBase
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

    // EditController
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;

protected:
    // Parameters (must match processor)
    enum {
        // Delay Section
        kDelayTime,
        kDelayFeedback,
        kDelayMix,

        // Comb Section
        kCombSize,
        kCombFeedback,
        kCombDamping,
        kCombDensity,
        kCombMix,

        // Global
        kInputGain,
        kOutputGain,
        kBypass,

        kNumParams
    };
};

} // namespace WaterStick