#pragma once

#include "vstgui/plugin-bindings/vst3editor.h"

namespace WaterStick {

class WaterStickEditor : public VSTGUI::VST3Editor
{
public:
    WaterStickEditor(Steinberg::Vst::EditController* controller);

    bool PLUGIN_API open(void* parent, const VSTGUI::PlatformType& platformType = VSTGUI::kDefaultNative) SMTG_OVERRIDE;

private:
    static constexpr int kEditorWidth = 400;
    static constexpr int kEditorHeight = 300;
};

} // namespace WaterStick