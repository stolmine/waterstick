#pragma once

#include "public.sdk/source/vst/vstguieditor.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/controls/icontrollistener.h"
#include <set>
#include <chrono>
#include <atomic>
#include <mutex>
#include <queue>
#include <unordered_set>
#include <unordered_map>

namespace WaterStick {

// Forward declarations
class WaterStickEditor;
class WaterStickController;

//========================================================================
// ParameterBlockingSystem - Prevents circular parameter updates during user interactions
//========================================================================
class ParameterBlockingSystem {
public:
    ParameterBlockingSystem();
    ~ParameterBlockingSystem();

    // User interaction detection
    void onUserInteractionStart(int32_t controlTag);
    void onUserInteractionEnd(int32_t controlTag);

    // Parameter blocking logic
    bool shouldBlockParameterUpdate(int32_t parameterId) const;
    bool shouldBlockControlUpdate(int32_t controlTag) const;

    // Visual update throttling
    bool shouldAllowVisualUpdate();
    void markVisualUpdateComplete();

    // Parameter queue management for delayed updates
    void queueParameterUpdate(int32_t parameterId, float normalizedValue);
    void processQueuedUpdates(Steinberg::Vst::EditController* controller);

    // Maintenance and diagnostics
    void cleanup();
    void getPerformanceStats(float& avgBlockingDuration, int& totalBlockedUpdates) const;

private:
    struct ParameterUpdate {
        int32_t parameterId;
        float normalizedValue;
        std::chrono::steady_clock::time_point timestamp;
    };

    struct UserInteractionState {
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point lastActivity;
        bool isActive;
    };

    // Configuration constants
    static constexpr std::chrono::milliseconds USER_CONTROL_TIMEOUT{1000};  // 1 second timeout
    static constexpr std::chrono::milliseconds VISUAL_UPDATE_INTERVAL{33};  // ~30Hz (33ms)
    static constexpr size_t MAX_QUEUED_UPDATES{256};

    // Thread-safe state management
    mutable std::mutex mStateMutex;
    std::unordered_map<int32_t, UserInteractionState> mActiveInteractions;
    std::unordered_set<int32_t> mUserControlledParameters;

    // Visual update throttling
    std::atomic<std::chrono::steady_clock::time_point> mLastVisualUpdate;

    // Parameter update queue
    mutable std::mutex mQueueMutex;
    std::queue<ParameterUpdate> mParameterQueue;

    // Performance tracking
    mutable std::atomic<int> mTotalBlockedUpdates{0};
    mutable std::atomic<int> mTotalInteractions{0};
    std::chrono::steady_clock::time_point mLastCleanupTime;

    // Internal helper methods
    void updateUserControlledParameters();
    bool isInteractionExpired(const UserInteractionState& state) const;
    void removeExpiredInteractions();
    int32_t getParameterForControl(int32_t controlTag) const;
};

// Enum for different tap contexts
enum class TapContext {
    Enable = 0,        // Enable/disable context (mode button 1)
    Volume = 1,        // Volume level context (mode button 2)
    Pan = 2,           // Pan position context (mode button 3)
    FilterCutoff = 3,  // Filter cutoff frequency context (mode button 4)
    FilterResonance = 4, // Filter resonance context (mode button 5)
    FilterType = 5,    // Filter type context (mode button 6)
    PitchShift = 6,    // Pitch shift context (mode button 7)
    FeedbackSend = 7,  // Feedback send level context (mode button 8)
    COUNT = 8          // Total number of contexts
};

// Custom minimap tap button class for always-visible tap mute state
class MinimapTapButton : public VSTGUI::CControl
{
public:
    MinimapTapButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag, WaterStickEditor* editor, int tapIndex);

    void draw(VSTGUI::CDrawContext* context) SMTG_OVERRIDE;

private:
    char getFilterTypeChar(float filterTypeValue) const;

    WaterStickEditor* editor;
    int tapIndex;

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

    // Dynamic sizing support
    void updateViewBoundsForContext(TapContext context, WaterStickEditor* editor);

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

    // Double-click detection state
    std::chrono::steady_clock::time_point lastClickTime;
    static constexpr std::chrono::milliseconds DOUBLE_CLICK_TIMEOUT{400};

    // Double-click helper methods
    bool isDoubleClick(const std::chrono::steady_clock::time_point& currentTime);
    void resetToDefaultValue();
    float getContextDefaultValue() const;
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

    // Double-click detection state
    std::chrono::steady_clock::time_point lastClickTime;
    static constexpr std::chrono::milliseconds DOUBLE_CLICK_TIMEOUT{400};

    // Double-click helper methods
    bool isDoubleClick(const std::chrono::steady_clock::time_point& currentTime);
    void resetToDefaultValue();
};

// Custom bypass toggle button class for section bypass controls
class BypassToggle : public VSTGUI::CControl
{
public:
    BypassToggle(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag);

    void draw(VSTGUI::CDrawContext* context) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;

    CLASS_METHODS(BypassToggle, VSTGUI::CControl)
};

// Custom sync toggle button class for tempo sync control
class SyncToggle : public VSTGUI::CControl
{
public:
    SyncToggle(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag);

    void draw(VSTGUI::CDrawContext* context) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;

    CLASS_METHODS(SyncToggle, VSTGUI::CControl)
};

// Custom macro knob control with smooth continuous behavior
class MacroKnobControl : public VSTGUI::CControl
{
public:
    MacroKnobControl(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag);

    void draw(VSTGUI::CDrawContext* context) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;

    // Continuous value management - override base class behavior to avoid reset issues
    void setValue(float value) SMTG_OVERRIDE;
    float getValue() const SMTG_OVERRIDE;

    // Context assignment for control isolation
    void setAssignedContext(TapContext context) { assignedContext = context; }
    TapContext getAssignedContext() const { return assignedContext; }


    CLASS_METHODS(MacroKnobControl, VSTGUI::CControl)

private:
    bool isDragging = false;
    VSTGUI::CPoint lastMousePos;

    // Internal value storage to avoid base class reset behavior
    float internalValue = 0.0f;

    // Double-click detection state
    std::chrono::steady_clock::time_point lastClickTime;
    static constexpr std::chrono::milliseconds DOUBLE_CLICK_TIMEOUT{400};

    // Context assignment for control isolation
    TapContext assignedContext = TapContext::Enable;  // Default to Enable context

    bool isDoubleClick(const std::chrono::steady_clock::time_point& currentTime);
    void resetToDefaultValue();
};

// Custom action button class for R/× buttons
class ActionButton : public VSTGUI::CControl
{
public:
    enum ActionType {
        Randomize,  // R button - randomizes current context values
        Reset       // × button - resets current context values to defaults
    };

    ActionButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag, ActionType type, int columnIndex);

    void draw(VSTGUI::CDrawContext* context) SMTG_OVERRIDE;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) SMTG_OVERRIDE;

    ActionType getActionType() const { return actionType; }
    int getColumnIndex() const { return columnIndex; }

    CLASS_METHODS(ActionButton, VSTGUI::CControl)

private:
    ActionType actionType;
    int columnIndex;  // Which column (0-7) this button affects
    bool isPressed;   // Visual state for feedback without triggering valueChanged
};

class WaterStickEditor : public Steinberg::Vst::VSTGUIEditor, public VSTGUI::IControlListener
{
public:
    WaterStickEditor(Steinberg::Vst::EditController* controller);

    bool PLUGIN_API open(void* parent, const VSTGUI::PlatformType& platformType) SMTG_OVERRIDE;
    void PLUGIN_API close() SMTG_OVERRIDE;

    // IControlListener
    void valueChanged(VSTGUI::CControl* control) SMTG_OVERRIDE;

    // Parameter blocking system access
    ParameterBlockingSystem& getParameterBlockingSystem() { return mParameterBlockingSystem; }

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

    // TapButton sizing helpers
    int getTapButtonSizeForContext(TapContext context) const;

    // Font management
    VSTGUI::SharedPointer<VSTGUI::CFontDesc> getWorkSansFont(float size) const;

    // Value formatting helpers
    std::string formatParameterValue(int parameterId, float normalizedValue) const;
    void updateValueReadouts();
    void updateBypassValueDisplay();

    // VST3 lifecycle compliance
    void forceParameterSynchronization();

    // Smart Hierarchy helper methods (public for ActionButton access)
    void handleMacroKnobChange(int columnIndex, float value);
    void handleGlobalMacroKnobChange(float continuousValue, TapContext currentCtx);
    void handleRandomizeAction(int columnIndex);
    void handleResetAction(int columnIndex);
    float generateRandomValue();
    float getContextDefaultValue(TapContext context);

private:
    static constexpr int kEditorWidth = 670;   // Optimized to eliminate excess whitespace while maintaining 30px margins
    static constexpr int kEditorHeight = 480;  // +40px expansion to resolve context labels collision

    // Tap button array for easy access
    VSTGUI::CControl* tapButtons[16];

    // Mode button references (8 total, one under each column)
    ModeButton* modeButtons[8];

    // Smart Hierarchy macro controls (8 columns × 3 rows = 24 controls)
    MacroKnobControl* macroKnobs[8];        // Row 1: Macro knobs for each column
    ActionButton* randomizeButtons[8];      // Row 2: Randomize buttons (R)
    ActionButton* resetButtons[8];          // Row 3: Reset buttons (×)

    // Global control knobs
    SyncToggle* syncModeToggle;
    KnobControl* timeDivisionKnob;
    KnobControl* feedbackKnob;
    KnobControl* inputGainKnob;
    KnobControl* outputGainKnob;
    KnobControl* dryWetKnob;
    KnobControl* gridKnob;
    BypassToggle* delayBypassToggle; // D-BYP: Delay bypass control
    KnobControl* globalDryWetKnob;  // G-MIX: Global dry/wet control


    // Knob labels
    VSTGUI::CTextLabel* syncModeLabel;
    VSTGUI::CTextLabel* timeDivisionLabel;
    VSTGUI::CTextLabel* feedbackLabel;
    VSTGUI::CTextLabel* inputGainLabel;
    VSTGUI::CTextLabel* outputGainLabel;
    VSTGUI::CTextLabel* dryWetLabel;
    VSTGUI::CTextLabel* gridLabel;
    VSTGUI::CTextLabel* globalDryWetLabel;


    // Value readout labels
    VSTGUI::CTextLabel* syncModeValue;
    VSTGUI::CTextLabel* timeDivisionValue;
    VSTGUI::CTextLabel* feedbackValue;
    VSTGUI::CTextLabel* inputGainValue;
    VSTGUI::CTextLabel* outputGainValue;
    VSTGUI::CTextLabel* dryWetValue;
    VSTGUI::CTextLabel* gridValue;
    VSTGUI::CTextLabel* globalDryWetValue;
    VSTGUI::CTextLabel* delayBypassValue;

    // Mode button labels
    VSTGUI::CTextLabel* modeButtonLabels[8];

    // Bypass toggle labels
    VSTGUI::CTextLabel* delayBypassLabel;

    // Minimap components
    MinimapTapButton* minimapButtons[16];

    // Context state management
    TapContext currentContext = TapContext::Enable;

    // Parameter blocking system for preventing circular updates
    ParameterBlockingSystem mParameterBlockingSystem;

    // Enhanced parameter update handling
    void performParameterUpdate(int32_t parameterId, float normalizedValue);
    bool shouldAllowParameterUpdate(int32_t parameterId) const;
    void updateParameterBlockingSystem();

    // Helper methods
    void createTapButtons(VSTGUI::CViewContainer* container);
    void createModeButtons(VSTGUI::CViewContainer* container);
    void createSmartHierarchy(VSTGUI::CViewContainer* container);
    void createMinimap(VSTGUI::CViewContainer* container);
    void updateMinimapState();
    void refreshAllContextsGUIState();
    void createGlobalControls(VSTGUI::CViewContainer* container);
    void applyEqualMarginLayout(VSTGUI::CViewContainer* container);

};

} // namespace WaterStick