#pragma once

#include "public.sdk/source/vst/vstguieditor.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/controls/icontrollistener.h"
#include <set>

namespace WaterStick {

// Enum for different tap contexts
enum class TapContext {
    Enable = 0,    // Enable/disable context (mode button 1)
    Volume = 1,    // Volume level context (mode button 2)
    Pan = 2,       // Pan position context (mode button 3) - future
    Filter = 3,    // Filter frequency context (mode button 4) - future
    COUNT          // Total number of contexts
};

// Custom tap button class with specific styling and context-aware behavior
class TapButton : public VSTGUI::CControl
{
public:
    TapButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag);

    void draw(VSTGUI::CDrawContext* context) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;

    // Context management
    void setContext(TapContext context) { currentContext = context; }
    TapContext getContext() const { return currentContext; }

    // Context-aware value management
    void setContextValue(TapContext context, float value) { contextValues[static_cast<int>(context)] = value; }
    float getContextValue(TapContext context) const { return contextValues[static_cast<int>(context)]; }
    float getCurrentContextValue() const { return contextValues[static_cast<int>(currentContext)]; }

    // Helper methods for drag functionality
    bool isDragOperation() const { return dragMode; }
    void resetDragAffectedSet() { dragAffectedButtons.clear(); }
    bool isButtonAlreadyAffected(TapButton* button) const;
    void markButtonAsAffected(TapButton* button);

    CLASS_METHODS(TapButton, VSTGUI::CControl)

private:
    bool dragMode = false;
    static std::set<TapButton*> dragAffectedButtons;

    // Context state management
    TapContext currentContext = TapContext::Enable;
    float contextValues[static_cast<int>(TapContext::COUNT)] = {0.0f};
};

// Custom mode button class with center dot styling
class ModeButton : public VSTGUI::CControl
{
public:
    ModeButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag);

    void draw(VSTGUI::CDrawContext* context) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;

    CLASS_METHODS(ModeButton, VSTGUI::CControl)
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

    // Mode button mutual exclusion
    void handleModeButtonSelection(ModeButton* selectedButton);

    // Context management
    void switchToContext(TapContext newContext);
    TapContext getCurrentContext() const { return currentContext; }
    int getSelectedModeButtonIndex() const;

private:
    static constexpr int kEditorWidth = 400;
    static constexpr int kEditorHeight = 300;

    // Tap button array for easy access
    VSTGUI::CControl* tapButtons[16];

    // Mode button references (8 total, one under each column)
    ModeButton* modeButtons[8];

    // Context state management
    TapContext currentContext = TapContext::Enable;

    // Helper methods
    void createTapButtons(VSTGUI::CViewContainer* container);
    void createModeButtons(VSTGUI::CViewContainer* container);
};

} // namespace WaterStick