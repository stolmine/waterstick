#pragma once

#include "public.sdk/source/vst/vstguieditor.h"

namespace WaterStick {

class WaterStickEditor : public Steinberg::Vst::VSTGUIEditor
{
public:
    WaterStickEditor(Steinberg::Vst::EditController* controller);

    bool PLUGIN_API open(void* parent, const VSTGUI::PlatformType& platformType) SMTG_OVERRIDE;
    void PLUGIN_API close() SMTG_OVERRIDE;

private:
    static constexpr int kEditorWidth = 400;
    static constexpr int kEditorHeight = 300;
};

} // namespace WaterStick