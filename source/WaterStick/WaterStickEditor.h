#pragma once

#include "public.sdk/source/vst/vstguieditor.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/controls/icontrollistener.h"

namespace WaterStick {

// Custom tap button class with specific styling and click-drag support
class TapButton : public VSTGUI::CControl
{
public:
    TapButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag);

    void draw(VSTGUI::CDrawContext* context) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;

    // Helper methods for drag functionality
    void setDragTargetValue(double value) { dragTargetValue = value; }
    bool isDragOperation() const { return dragMode; }

    CLASS_METHODS(TapButton, VSTGUI::CControl)

private:
    bool dragMode = false;
    double dragTargetValue = 0.0;
    double initialValue = 0.0;
};

class WaterStickEditor : public Steinberg::Vst::VSTGUIEditor, public VSTGUI::IControlListener
{
public:
    WaterStickEditor(Steinberg::Vst::EditController* controller);

    bool PLUGIN_API open(void* parent, const VSTGUI::PlatformType& platformType) SMTG_OVERRIDE;
    void PLUGIN_API close() SMTG_OVERRIDE;

    // IControlListener
    void valueChanged(VSTGUI::CControl* control) SMTG_OVERRIDE;

    // Public helper for drag operations
    TapButton* getTapButtonAtPoint(const VSTGUI::CPoint& point);

private:
    static constexpr int kEditorWidth = 400;
    static constexpr int kEditorHeight = 300;

    // Tap button array for easy access
    VSTGUI::CControl* tapButtons[16];

    // Helper methods
    void createTapButtons(VSTGUI::CViewContainer* container);
};

} // namespace WaterStick