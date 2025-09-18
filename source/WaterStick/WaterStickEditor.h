#pragma once

#include "public.sdk/source/vst/vstguieditor.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/controls/icontrollistener.h"
#include <set>

namespace WaterStick {

// Enum for different tap contexts
enum class TapContext {
    Enable = 0,        // Enable/disable context (mode button 1)
    Volume = 1,        // Volume level context (mode button 2)
    Pan = 2,           // Pan position context (mode button 3)
    FilterCutoff = 3,  // Filter cutoff frequency context (mode button 4)
    FilterResonance = 4, // Filter resonance context (mode button 5)
    FilterType = 5,    // Filter type context (mode button 6) - future
    COUNT              // Total number of contexts
};

// Custom minimap tap button class for always-visible tap mute state
class MinimapTapButton : public VSTGUI::CControl
{
public:
    MinimapTapButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag);

    void draw(VSTGUI::CDrawContext* context) SMTG_OVERRIDE;

    CLASS_METHODS(MinimapTapButton, VSTGUI::CControl)
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

    // Volume control interaction state
    bool isVolumeInteracting = false;
    VSTGUI::CPoint initialClickPoint;
    float initialVolumeValue = 0.0f;
    static constexpr double DRAG_THRESHOLD = 5.25;  // Pixels before we consider it a drag (3 * 1.75)

    // Drag direction tracking
    enum class DragDirection {
        None,
        Vertical,
        Horizontal
    };
    DragDirection currentDragDirection = DragDirection::None;
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

// Custom knob control with dot indicator design
class KnobControl : public VSTGUI::CControl
{
public:
    KnobControl(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag);

    void draw(VSTGUI::CDrawContext* context) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;

    // Special handling for Time/Division knob
    void setIsTimeDivisionKnob(bool isTimeDivision) { isTimeDivisionKnob = isTimeDivision; }
    bool getIsTimeDivisionKnob() const { return isTimeDivisionKnob; }

    CLASS_METHODS(KnobControl, VSTGUI::CControl)

private:
    bool isDragging = false;
    VSTGUI::CPoint lastMousePos;
    bool isTimeDivisionKnob = false;
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

    // Parameter mapping helpers
    int getTapParameterIdForContext(int tapButtonIndex, TapContext context) const;

    // Font management
    VSTGUI::SharedPointer<VSTGUI::CFontDesc> getWorkSansFont(float size) const;

    // Value formatting helpers
    std::string formatParameterValue(int parameterId, float normalizedValue) const;
    void updateValueReadouts();

private:
    static constexpr int kEditorWidth = 1050;  // 600 * 1.75 (expanded for comb section)
    static constexpr int kEditorHeight = 525;  // 300 * 1.75

    // Tap button array for easy access
    VSTGUI::CControl* tapButtons[16];

    // Mode button references (8 total, one under each column)
    ModeButton* modeButtons[8];

    // Global control knobs
    KnobControl* syncModeKnob;
    KnobControl* timeDivisionKnob;
    KnobControl* feedbackKnob;
    KnobControl* inputGainKnob;
    KnobControl* outputGainKnob;
    KnobControl* dryWetKnob;
    KnobControl* gridKnob;

    // Comb section knobs
    KnobControl* combSizeKnob;
    KnobControl* combTapsKnob;
    KnobControl* combSlopeKnob;
    KnobControl* combWaveKnob;
    KnobControl* combFeedbackKnob;
    KnobControl* combRateKnob;

    // Knob labels
    VSTGUI::CTextLabel* syncModeLabel;
    VSTGUI::CTextLabel* timeDivisionLabel;
    VSTGUI::CTextLabel* feedbackLabel;
    VSTGUI::CTextLabel* inputGainLabel;
    VSTGUI::CTextLabel* outputGainLabel;
    VSTGUI::CTextLabel* dryWetLabel;
    VSTGUI::CTextLabel* gridLabel;

    // Comb section labels
    VSTGUI::CTextLabel* combSizeLabel;
    VSTGUI::CTextLabel* combTapsLabel;
    VSTGUI::CTextLabel* combSlopeLabel;
    VSTGUI::CTextLabel* combWaveLabel;
    VSTGUI::CTextLabel* combFeedbackLabel;
    VSTGUI::CTextLabel* combRateLabel;

    // Value readout labels
    VSTGUI::CTextLabel* syncModeValue;
    VSTGUI::CTextLabel* timeDivisionValue;
    VSTGUI::CTextLabel* feedbackValue;
    VSTGUI::CTextLabel* inputGainValue;
    VSTGUI::CTextLabel* outputGainValue;
    VSTGUI::CTextLabel* dryWetValue;
    VSTGUI::CTextLabel* gridValue;

    // Comb section value readouts
    VSTGUI::CTextLabel* combSizeValue;
    VSTGUI::CTextLabel* combTapsValue;
    VSTGUI::CTextLabel* combSlopeValue;
    VSTGUI::CTextLabel* combWaveValue;
    VSTGUI::CTextLabel* combFeedbackValue;
    VSTGUI::CTextLabel* combRateValue;

    // Mode button labels
    VSTGUI::CTextLabel* modeButtonLabels[8];

    // Minimap components
    VSTGUI::CViewContainer* minimapContainer;
    MinimapTapButton* minimapButtons[16];

    // Context state management
    TapContext currentContext = TapContext::Enable;

    // Helper methods
    void createTapButtons(VSTGUI::CViewContainer* container);
    void createModeButtons(VSTGUI::CViewContainer* container);
    void createMinimap(VSTGUI::CViewContainer* container);
    void updateMinimapState();
    void createGlobalControls(VSTGUI::CViewContainer* container);
    void createCombControls(VSTGUI::CViewContainer* container);
};

} // namespace WaterStick