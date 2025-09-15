#pragma once

#include "public.sdk/source/vst/vstguieditor.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/controls/icontrollistener.h"

namespace WaterStick {

// Custom tap button class with specific styling
class TapButton : public VSTGUI::CControl
{
public:
    TapButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag);

    void draw(VSTGUI::CDrawContext* context) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;

    CLASS_METHODS(TapButton, VSTGUI::CControl)
};

class WaterStickEditor : public Steinberg::Vst::VSTGUIEditor, public VSTGUI::IControlListener
{
public:
    WaterStickEditor(Steinberg::Vst::EditController* controller);

    bool PLUGIN_API open(void* parent, const VSTGUI::PlatformType& platformType) SMTG_OVERRIDE;
    void PLUGIN_API close() SMTG_OVERRIDE;

    // IControlListener
    void valueChanged(VSTGUI::CControl* control) SMTG_OVERRIDE;

private:
    static constexpr int kEditorWidth = 400;
    static constexpr int kEditorHeight = 300;

    // Tap button array for easy access
    VSTGUI::CControl* tapButtons[16];

    // Helper methods
    void createTapButtons(VSTGUI::CViewContainer* container);
};

} // namespace WaterStick