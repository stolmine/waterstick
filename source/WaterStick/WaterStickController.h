#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

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
};

} // namespace WaterStick