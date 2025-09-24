#include "WaterStickEditor.h"
#include "WaterStickController.h"
#include "WaterStickParameters.h"
#include "WaterStickLogger.h"
#include "ControlFactory.h"
#include "vstgui/lib/cviewcontainer.h"
#include "vstgui/lib/controls/ctextlabel.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/cframe.h"
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstdlib>
#include <algorithm>

namespace WaterStick {

WaterStickEditor::WaterStickEditor(Steinberg::Vst::EditController* controller)
: VSTGUIEditor(controller)
{
    Steinberg::ViewRect viewRect(0, 0, kEditorWidth, kEditorHeight);
    setRect(viewRect);

    for (int i = 0; i < 16; i++) {
        tapButtons[i] = nullptr;
    }

    for (int i = 0; i < 8; i++) {
        modeButtons[i] = nullptr;
        macroKnobs[i] = nullptr;
        randomizeButtons[i] = nullptr;
        resetButtons[i] = nullptr;
    }

    for (int i = 0; i < 16; i++) {
        minimapButtons[i] = nullptr;
    }

    syncModeKnob = nullptr;
    timeDivisionKnob = nullptr;
    feedbackKnob = nullptr;
    gridKnob = nullptr;
    inputGainKnob = nullptr;
    outputGainKnob = nullptr;
    dryWetKnob = nullptr;
    delayBypassToggle = nullptr;
    globalDryWetKnob = nullptr;

    delayBypassLabel = nullptr;
    syncModeLabel = nullptr;
    timeDivisionLabel = nullptr;
    feedbackLabel = nullptr;
    gridLabel = nullptr;
    inputGainLabel = nullptr;
    outputGainLabel = nullptr;
    dryWetLabel = nullptr;
    globalDryWetLabel = nullptr;

    delayBypassValue = nullptr;
    syncModeValue = nullptr;
    timeDivisionValue = nullptr;
    feedbackValue = nullptr;
    gridValue = nullptr;
    inputGainValue = nullptr;
    outputGainValue = nullptr;
    dryWetValue = nullptr;
    globalDryWetValue = nullptr;
}

bool PLUGIN_API WaterStickEditor::open(void* parent, const VSTGUI::PlatformType& platformType)
{
    VSTGUI::CRect frameSize(0, 0, kEditorWidth, kEditorHeight);

    frame = new VSTGUI::CFrame(frameSize, this);
    frame->open(parent, platformType);

    auto container = new VSTGUI::CViewContainer(frameSize);
    container->setBackgroundColor(VSTGUI::kWhiteCColor);

    // Create all content first, then position with equal margins
    createTapButtons(container);
    createSmartHierarchy(container);
    createModeButtons(container);
    createGlobalControls(container);
    createMinimap(container);

    // Apply equal margin positioning after all content is created
    applyEqualMarginLayout(container);

    frame->addView(container);

    // Force parameter synchronization for VST3 lifecycle compliance
    forceParameterSynchronization();

    updateValueReadouts();
    updateMinimapState();

    return true;
}

void PLUGIN_API WaterStickEditor::close()
{
    if (frame)
    {
        frame->forget();
        frame = nullptr;
    }
}

void WaterStickEditor::createTapButtons(VSTGUI::CViewContainer* container)
{
    // Button grid configuration (scaled by 1.75x)
    const int buttonSize = 53;           // Button diameter (30 * 1.75 = 52.5, rounded to 53)
    const int buttonSpacing = buttonSize / 2;  // Half diameter spacing
    const int gridWidth = 8;            // 8 columns
    const int gridHeight = 2;           // 2 rows

    // Calculate total grid dimensions
    const int totalGridHeight = (gridHeight * buttonSize) + ((gridHeight - 1) * buttonSpacing);

    // Position grid in left section (2/3 of window width) with margin and improved centering
    const int upperTwoThirdsHeight = (kEditorHeight * 2) / 3;
    const int delayMargin = 30;
    const int gridLeft = delayMargin;  // Align to left margin within delay section
    // Improved vertical centering for better overall balance (reduced from -23px to -15px)
    const int gridTop = ((upperTwoThirdsHeight - totalGridHeight) / 2) - 15;

    for (int i = 0; i < 16; i++) {
        int row = i / 8;
        int col = i % 8;

        int x = gridLeft + col * (buttonSize + buttonSpacing);
        int y = gridTop + row * (buttonSize + buttonSpacing);

        VSTGUI::CRect buttonRect(x, y, x + buttonSize, y + buttonSize);

        auto button = new TapButton(buttonRect, this, kTap1Enable + (i * 3));
        button->setContext(TapContext::Enable);

        // Load initial values from parameters for all contexts
        auto controller = getController();
        if (controller) {
            int enableParamId = getTapParameterIdForContext(i, TapContext::Enable);
            auto enableValue = controller->getParamNormalized(enableParamId);
            button->setContextValue(TapContext::Enable, enableValue);

            int volumeParamId = getTapParameterIdForContext(i, TapContext::Volume);
            auto volumeValue = controller->getParamNormalized(volumeParamId);
            button->setContextValue(TapContext::Volume, volumeValue);

            int panParamId = getTapParameterIdForContext(i, TapContext::Pan);
            auto panValue = controller->getParamNormalized(panParamId);
            button->setContextValue(TapContext::Pan, panValue);

            int filterCutoffParamId = getTapParameterIdForContext(i, TapContext::FilterCutoff);
            auto filterCutoffValue = controller->getParamNormalized(filterCutoffParamId);
            button->setContextValue(TapContext::FilterCutoff, filterCutoffValue);

            int filterResonanceParamId = getTapParameterIdForContext(i, TapContext::FilterResonance);
            auto filterResonanceValue = controller->getParamNormalized(filterResonanceParamId);
            button->setContextValue(TapContext::FilterResonance, filterResonanceValue);

            int filterTypeParamId = getTapParameterIdForContext(i, TapContext::FilterType);
            auto filterTypeValue = controller->getParamNormalized(filterTypeParamId);
            button->setContextValue(TapContext::FilterType, filterTypeValue);

            int pitchShiftParamId = getTapParameterIdForContext(i, TapContext::PitchShift);
            auto pitchShiftValue = controller->getParamNormalized(pitchShiftParamId);
            button->setContextValue(TapContext::PitchShift, pitchShiftValue);

            int feedbackSendParamId = getTapParameterIdForContext(i, TapContext::FeedbackSend);
            auto feedbackSendValue = controller->getParamNormalized(feedbackSendParamId);
            button->setContextValue(TapContext::FeedbackSend, feedbackSendValue);

            button->setValue(enableValue);
        }

        tapButtons[i] = button;

        container->addView(button);
    }
}

void WaterStickEditor::createModeButtons(VSTGUI::CViewContainer* container)
{
    // Button grid configuration (matching tap buttons, scaled by 1.75x)
    const int buttonSize = 53;           // Button diameter (30 * 1.75 = 52.5, rounded to 53)
    const int buttonSpacing = buttonSize / 2;  // Half diameter spacing
    const int gridWidth = 8;            // 8 columns
    const int gridHeight = 2;           // 2 rows

    // Calculate total grid dimensions
    const int totalGridHeight = (gridHeight * buttonSize) + ((gridHeight - 1) * buttonSpacing);

    // Match the tap button positioning with improved centering
    const int upperTwoThirdsHeight = (kEditorHeight * 2) / 3;
    const int delayMargin = 30;
    const int gridLeft = delayMargin;  // Match repositioned tap buttons
    // Improved vertical centering for better overall balance (reduced from -23px to -15px)
    const int tapGridTop = ((upperTwoThirdsHeight - totalGridHeight) / 2) - 15;

    // Calculate mode button position with improved spacing
    // Place with increased spacing below the tap button grid for better visual hierarchy
    const int modeButtonY = tapGridTop + (gridHeight * buttonSize) + buttonSpacing + (buttonSpacing * 3.0);

    // Calculate expanded view bounds to accommodate the rectangle (scaled by 1.75x)
    // Circle size: 53px - 5px stroke = 48px (keeping 5px stroke width)
    // Rectangle size: 48px * 1.5 = 72px
    // Expansion needed: (72px - 53px) / 2 = 9.5px per side
    const int expansionNeeded = 10; // Round up to 10px for safety

    // Create 8 mode buttons, one under each column
    for (int i = 0; i < 8; i++) {
        // Calculate X position for this mode button (under column i)
        const int modeButtonX = gridLeft + i * (buttonSize + buttonSpacing);

        VSTGUI::CRect modeButtonRect(
            modeButtonX - expansionNeeded,
            modeButtonY - expansionNeeded,
            modeButtonX + buttonSize + expansionNeeded,
            modeButtonY + buttonSize + expansionNeeded
        );

        // Create mode button with temporary tag (-1 for now, will be updated later)
        modeButtons[i] = new ModeButton(modeButtonRect, this, -1);

        // Set the first button as selected by default
        if (i == 0) {
            modeButtons[i]->setValue(1.0);
        }

        // Add to container
        container->addView(modeButtons[i]);
    }

    // Add labels below mode buttons with optimized spacing
    const char* modeLabels[] = {"Mutes", "Level", "Pan", "Cutoff", "Res", "Type", "Pitch", "FB Send"};
    const int labelHeight = 20; // Match global control labels
    const int labelY = modeButtonY + buttonSize + 12; // Reduced gap for tighter layout while maintaining clearance

    for (int i = 0; i < 8; i++) {
        const int modeButtonX = gridLeft + i * (buttonSize + buttonSpacing);

        // Create label centered under mode button
        VSTGUI::CRect labelRect(modeButtonX, labelY, modeButtonX + buttonSize, labelY + labelHeight);
        modeButtonLabels[i] = new VSTGUI::CTextLabel(labelRect, modeLabels[i]);

        // Set label styling to match global control labels exactly
        modeButtonLabels[i]->setHoriAlign(VSTGUI::kCenterText);
        modeButtonLabels[i]->setFontColor(VSTGUI::kBlackCColor);
        modeButtonLabels[i]->setBackColor(VSTGUI::kTransparentCColor);
        modeButtonLabels[i]->setFrameColor(VSTGUI::kTransparentCColor);
        modeButtonLabels[i]->setStyle(VSTGUI::CTextLabel::kNoFrame);

        // Use same font size as global control labels (11.0f)
        auto customFont = getWorkSansFont(11.0f);
        if (customFont) {
            modeButtonLabels[i]->setFont(customFont);
        }

        container->addView(modeButtonLabels[i]);
    }
}

void WaterStickEditor::createGlobalControls(VSTGUI::CViewContainer* container)
{
    // Improved spacing calculation with better centering and visual hierarchy
    const int bottomThirdTop = ((kEditorHeight * 2) / 3);
    const int knobSize = 53;
    const int buttonSize = 53;
    const int buttonSpacing = buttonSize / 2; // 26.5px base spacing
    const int gridWidth = 8;
    const int delayMargin = 30;
    const int tapGridLeft = delayMargin;
    // Refined spacing: fine-tuned for optimal visual balance between context labels and global controls
    const int modeButtonSpacing = static_cast<int>(buttonSpacing * 3.05); // Precise spacing for ~6.5px gap
    const int knobY = bottomThirdTop + modeButtonSpacing - 10; // Slight adjustment for centering

    ControlFactory factory(this, container);

    // Create bypass toggle in first column
    int bypassX = tapGridLeft;
    int bypassY = knobY + (knobSize - knobSize) / 2; // Center vertically with knobs
    VSTGUI::CRect bypassRect(bypassX, bypassY, bypassX + knobSize, bypassY + knobSize);
    delayBypassToggle = new BypassToggle(bypassRect, this, kDelayBypass);
    container->addView(delayBypassToggle);

    // Create bypass label
    VSTGUI::CRect bypassLabelRect(bypassX, bypassY + knobSize + 5, bypassX + knobSize, bypassY + knobSize + 25);
    delayBypassLabel = factory.createLabel(bypassLabelRect, "D-BYP", 11.0f, false);
    container->addView(delayBypassLabel);

    // Create bypass value display
    VSTGUI::CRect bypassValueRect(bypassX, bypassLabelRect.bottom + 2, bypassX + knobSize, bypassLabelRect.bottom + 20);
    delayBypassValue = factory.createLabel(bypassValueRect, "OFF", 9.0f, true);
    container->addView(delayBypassValue);

    KnobDefinition globalKnobs[] = {
        {"SYNC", kTempoSyncMode, &syncModeKnob, &syncModeLabel, &syncModeValue, false},
        {"TIME", kDelayTime, &timeDivisionKnob, &timeDivisionLabel, &timeDivisionValue, true},
        {"FEEDBACK", kFeedback, &feedbackKnob, &feedbackLabel, &feedbackValue, false},
        {"GRID", kGrid, &gridKnob, &gridLabel, &gridValue, false},
        {"INPUT", kInputGain, &inputGainKnob, &inputGainLabel, &inputGainValue, false},
        {"OUTPUT", kOutputGain, &outputGainKnob, &outputGainLabel, &outputGainValue, false},
        {"G-MIX", kGlobalDryWet, &globalDryWetKnob, &globalDryWetLabel, &globalDryWetValue, false},
    };

    // Create remaining 7 knobs in columns 2-8
    for (int i = 0; i < 7; i++) {
        // Calculate X position (skip column 0 which has the bypass toggle)
        int knobX = tapGridLeft + (i + 1) * (buttonSize + buttonSpacing);
        factory.createKnobWithLayout(knobX, knobY, knobSize, globalKnobs[i]);
    }

    auto controller = getController();
    if (controller) {
        // Load bypass toggle value
        float bypassValue = controller->getParamNormalized(kDelayBypass);
        if (delayBypassToggle) {
            delayBypassToggle->setValue(bypassValue);
            delayBypassToggle->invalid();
        }

        // Load knob values
        for (int i = 0; i < 7; i++) {
            float value = controller->getParamNormalized(globalKnobs[i].tag);
            std::string paramName;
            switch (globalKnobs[i].tag) {
                case kInputGain: paramName = "InputGain"; break;
                case kOutputGain: paramName = "OutputGain"; break;
                case kTempoSyncMode: paramName = "TempoSyncMode"; break;
                case kDelayTime: paramName = "DelayTime"; break;
                case kFeedback: paramName = "Feedback"; break;
                case kGrid: paramName = "Grid"; break;
                case kGlobalDryWet: paramName = "GlobalDryWet"; break;
                default: paramName = "Unknown"; break;
            }

            // Update the knob with parameter value
            if (globalKnobs[i].knobPtr && *(globalKnobs[i].knobPtr)) {
                (*(globalKnobs[i].knobPtr))->setValue(value);
                (*(globalKnobs[i].knobPtr))->invalid();
            }
        }
    }
}

std::string WaterStickEditor::formatParameterValue(int parameterId, float normalizedValue) const
{
    std::ostringstream oss;

    switch (parameterId) {
        case kTempoSyncMode:
            return normalizedValue > 0.5f ? "SYNC" : "FREE";

        case kDelayTime:
            // Convert normalized to seconds (0-2s range)
            oss << std::fixed << std::setprecision(3) << (normalizedValue * 2.0f) << "s";
            return oss.str();

        case kSyncDivision:
        {
            // Convert to sync division index and get description
            int divIndex = static_cast<int>(normalizedValue * (kNumSyncDivisions - 1));
            const char* divNames[] = {
                "1/64", "1/32T", "1/64.", "1/32", "1/16T", "1/32.", "1/16", "1/8T", "1/16.",
                "1/8", "1/4T", "1/8.", "1/4", "1/2T", "1/4.", "1/2", "1T", "1/2.", "1", "2", "4", "8"
            };
            if (divIndex >= 0 && divIndex < kNumSyncDivisions) {
                return divNames[divIndex];
            }
            return "1/4";
        }

        case kInputGain:
        case kOutputGain:
        {
            // Convert normalized to dB (-40dB to +12dB range)
            float dbValue = -40.0f + (normalizedValue * 52.0f);
            oss << std::fixed << std::setprecision(1) << dbValue << "dB";
            return oss.str();
        }

        case kGlobalDryWet:
            // Convert to percentage
            oss << std::fixed << std::setprecision(0) << (normalizedValue * 100.0f) << "%";
            return oss.str();

        case kGrid:
        {
            // Convert normalized to grid index and get taps per beat
            int gridIndex = static_cast<int>(normalizedValue * 7); // 8 grid values (0-7)
            const int tapsPerBeat[] = {1, 2, 3, 4, 6, 8, 12, 16};
            if (gridIndex >= 0 && gridIndex <= 7) {
                oss << tapsPerBeat[gridIndex] << " Taps/Beat";
            } else {
                oss << "4 Taps/Beat"; // Default fallback
            }
            return oss.str();
        }

        case kDelayBypass:
            // Format as bypass state
            return normalizedValue > 0.5f ? "BYP" : "OFF";

        default:
            oss << std::fixed << std::setprecision(2) << normalizedValue;
            return oss.str();
    }
}

void WaterStickEditor::updateValueReadouts()
{
    auto controller = getController();
    if (!controller) return;

    // Update all global control value readouts
    // Update bypass value display separately
    if (delayBypassValue) {
        float bypassValue = controller->getParamNormalized(kDelayBypass);
        std::string valueText = formatParameterValue(kDelayBypass, bypassValue);
        delayBypassValue->setText(valueText.c_str());
    }

    const int knobTags[] = {kTempoSyncMode, kDelayTime, kFeedback, kGrid, kInputGain, kOutputGain, kGlobalDryWet};
    VSTGUI::CTextLabel* valueLabels[] = {syncModeValue, timeDivisionValue, feedbackValue, gridValue, inputGainValue, outputGainValue, globalDryWetValue};

    for (int i = 0; i < 7; i++) {
        if (valueLabels[i]) {
            int paramId = knobTags[i];

            // Special handling for time/division knob
            if (i == 1) {
                float syncMode = controller->getParamNormalized(kTempoSyncMode);
                if (syncMode > 0.5f) {
                    paramId = kSyncDivision;
                } else {
                    paramId = kDelayTime;
                }
            }

            float value = controller->getParamNormalized(paramId);
            std::string valueText = formatParameterValue(paramId, value);
            valueLabels[i]->setText(valueText.c_str());
            valueLabels[i]->invalid();
        }
    }

}

void WaterStickEditor::valueChanged(VSTGUI::CControl* control)
{
    // Check if this is a Smart Hierarchy control
    auto macroKnob = dynamic_cast<MacroKnobControl*>(control);
    if (macroKnob) {
        // Find which macro knob this is (0-7)
        int columnIndex = -1;
        for (int i = 0; i < 8; i++) {
            if (macroKnobs[i] == macroKnob) {
                columnIndex = i;
                break;
            }
        }
        // DIAGNOSTIC: Log macro knob events in valueChanged
        printf("[MacroKnob] valueChanged - control tag: %d, columnIndex: %d, value: %.3f\n",
               control->getTag(), columnIndex, control->getValue());

        if (columnIndex >= 0) {
            handleMacroKnobChange(columnIndex, control->getValue());
        } else {
            printf("[MacroKnob] ERROR: Could not find macro knob in array\n");
        }
        return;
    }

    auto actionButton = dynamic_cast<ActionButton*>(control);
    if (actionButton) {
        // DIAGNOSTIC: This should no longer occur after fixing setValue issue
        printf("[ActionButton] ERROR: ActionButton triggered valueChanged - this should not happen after fix\n");
        return;
    }

    // Check if this is a mode button being selected
    auto modeButton = dynamic_cast<ModeButton*>(control);
    if (modeButton && modeButton->getValue() > 0.5) {
        // Handle mutual exclusion for mode buttons
        handleModeButtonSelection(modeButton);
        return; // Mode buttons don't have VST parameters
    }

    // Check if this is a tap button
    auto tapButton = dynamic_cast<TapButton*>(control);
    if (tapButton) {
        // Find which tap button this is (0-15)
        int tapButtonIndex = -1;
        for (int i = 0; i < 16; i++) {
            if (tapButtons[i] == tapButton) {
                tapButtonIndex = i;
                break;
            }
        }

        if (tapButtonIndex >= 0) {
            // Get the correct parameter ID based on the button's current context
            TapContext buttonContext = tapButton->getContext();
            int parameterId = getTapParameterIdForContext(tapButtonIndex, buttonContext);

            // Update parameter in controller
            auto controller = getController();
            if (controller) {
                controller->setParamNormalized(parameterId, control->getValue());
                controller->performEdit(parameterId, control->getValue());

                // Update minimap if this was a tap enable change
                if (buttonContext == TapContext::Enable) {
                    updateMinimapState();
                }
            }
        }
    }
    else if (control && control->getTag() != -1) {
        // Check if this is the time/division knob with special handling
        auto knobControl = dynamic_cast<KnobControl*>(control);
        if (knobControl && knobControl->getIsTimeDivisionKnob()) {
            // Handle time/division knob - switch between kDelayTime and kSyncDivision based on sync mode
            auto controller = getController();
            if (controller) {
                float syncMode = controller->getParamNormalized(kTempoSyncMode);

                if (syncMode > 0.5f) {
                    // Sync mode: control sync division
                    controller->setParamNormalized(kSyncDivision, control->getValue());
                    controller->performEdit(kSyncDivision, control->getValue());
                } else {
                    // Free mode: control delay time
                    controller->setParamNormalized(kDelayTime, control->getValue());
                    controller->performEdit(kDelayTime, control->getValue());
                }

                // Update value readouts for time/division knob
                updateValueReadouts();
            }
        }
        else {
            // Handle other controls (non-tap, non-mode buttons)
            auto controller = getController();
            if (controller) {
                controller->setParamNormalized(control->getTag(), control->getValue());
                controller->performEdit(control->getTag(), control->getValue());

                // Special case: if sync mode changed, update the time/division knob value
                if (control->getTag() == kTempoSyncMode && timeDivisionKnob) {
                    float newSyncMode = control->getValue();
                    if (newSyncMode > 0.5f) {
                        // Switched to sync mode: load sync division value
                        float syncDivValue = controller->getParamNormalized(kSyncDivision);
                        timeDivisionKnob->setValue(syncDivValue);
                        timeDivisionKnob->invalid();
                    } else {
                        // Switched to free mode: load delay time value
                        float delayTimeValue = controller->getParamNormalized(kDelayTime);
                        timeDivisionKnob->setValue(delayTimeValue);
                        timeDivisionKnob->invalid();
                    }
                }

                // Special handling for bypass toggle value display - do this first
                if (control->getTag() == kDelayBypass) {
                    // Force immediate update of bypass value display
                    if (delayBypassValue) {
                        std::string valueText = formatParameterValue(kDelayBypass, control->getValue());
                        delayBypassValue->setText(valueText.c_str());
                        delayBypassValue->invalid(); // Force redraw
                    }
                }

                // Update value readouts for any global control change
                updateValueReadouts();
            }
        }
    }
}

TapButton* WaterStickEditor::getTapButtonAtPoint(const VSTGUI::CPoint& point)
{
    // Check all tap buttons to see if the point is within their bounds
    for (int i = 0; i < 16; i++) {
        if (tapButtons[i]) {
            auto button = static_cast<TapButton*>(tapButtons[i]);
            VSTGUI::CRect buttonRect = button->getViewSize();

            // Convert button rect to frame coordinates for comparison
            VSTGUI::CPoint topLeft = buttonRect.getTopLeft();
            VSTGUI::CPoint bottomRight = buttonRect.getBottomRight();
            button->localToFrame(topLeft);
            button->localToFrame(bottomRight);

            VSTGUI::CRect frameRect(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
            if (frameRect.pointInside(point)) {
                return button;
            }
        }
    }
    return nullptr;
}

void WaterStickEditor::handleModeButtonSelection(ModeButton* selectedButton)
{
    // Implement mutual exclusion - only one mode button can be selected at a time
    for (int i = 0; i < 8; i++) {
        if (modeButtons[i] && modeButtons[i] != selectedButton) {
            // Deselect all other mode buttons
            modeButtons[i]->setValue(0.0);
            modeButtons[i]->invalid(); // Trigger redraw
        }
    }

    // Switch to the corresponding context
    int selectedIndex = getSelectedModeButtonIndex();
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(TapContext::COUNT)) {
        switchToContext(static_cast<TapContext>(selectedIndex));
    }
}

void WaterStickEditor::switchToContext(TapContext newContext)
{
    std::string contextNames[] = {"Enable", "Volume", "Pan", "FilterCutoff", "FilterResonance", "FilterType", "PitchShift", "FeedbackSend"};
    std::string oldContextName = (currentContext < TapContext::COUNT) ? contextNames[static_cast<int>(currentContext)] : "Unknown";
    std::string newContextName = (newContext < TapContext::COUNT) ? contextNames[static_cast<int>(newContext)] : "Unknown";


    if (newContext == currentContext) {
        return; // Already in this context
    }

    // Save current context values from tap buttons to VST parameters
    auto controller = getController();
    if (controller) {
        for (int i = 0; i < 16; i++) {
            auto tapButton = static_cast<TapButton*>(tapButtons[i]);
            if (tapButton) {
                // Get the current parameter ID for the current context
                int currentParamId = getTapParameterIdForContext(i, currentContext);

                // Save the button's current value to the parameter
                controller->setParamNormalized(currentParamId, tapButton->getValue());

                // Also update the button's internal storage
                tapButton->setContextValue(currentContext, tapButton->getValue());
            }
        }
    }

    // Switch to new context
    currentContext = newContext;

    // Synchronize the context with the controller for macro knob coordination
    auto waterStickController = dynamic_cast<WaterStickController*>(controller);
    if (waterStickController) {
        waterStickController->setCurrentTapContext(static_cast<int>(newContext));
        printf("[Context] Synchronized context with controller: %s (%d)\n",
               newContextName.c_str(), static_cast<int>(newContext));
    }


    // Load new context values from VST parameters
    if (controller) {
        for (int i = 0; i < 16; i++) {
            auto tapButton = static_cast<TapButton*>(tapButtons[i]);
            if (tapButton) {
                // CRITICAL FIX: Force complete state clearing before context switch
                // This prevents graphics artifacts from previous context (especially PitchShift)
                tapButton->setDirty(true);

                // Set the context for the button
                tapButton->setContext(newContext);

                // Update view bounds for the new context (architectural fix for text clipping)
                tapButton->updateViewBoundsForContext(newContext, this);

                // Get the parameter ID for the new context
                int newParamId = getTapParameterIdForContext(i, newContext);

                // Load the parameter value
                float paramValue = controller->getParamNormalized(newParamId);


                // Set the button's value and internal storage
                tapButton->setValue(paramValue);
                tapButton->setContextValue(newContext, paramValue);

                // Trigger comprehensive visual update with state clearing
                tapButton->setDirty(true);
                tapButton->invalid();
            }
        }
    }

    // Force minimap redraw for context change
    for (int i = 0; i < 16; i++) {
        if (minimapButtons[i]) {
            minimapButtons[i]->invalid();
        }
    }
}

int WaterStickEditor::getSelectedModeButtonIndex() const
{
    for (int i = 0; i < 8; i++) {
        if (modeButtons[i] && modeButtons[i]->getValue() > 0.5) {
            return i;
        }
    }
    return 0; // Default to first button if none selected
}

VSTGUI::SharedPointer<VSTGUI::CFontDesc> WaterStickEditor::getWorkSansFont(float size) const
{
    // Create CFontDesc with the font file path relative to the plugin bundle
    // VSTGUI will handle loading the font from the file system
    return VSTGUI::makeOwned<VSTGUI::CFontDesc>("fonts/WorkSans-Regular.otf", size);
}

int WaterStickEditor::getTapParameterIdForContext(int tapButtonIndex, TapContext context) const
{
    // Convert tap button index to tap number (1-16)
    // The grid layout: taps 1-8 are top row (indices 0-7), taps 9-16 are bottom row (indices 8-15)
    int tapNumber = tapButtonIndex + 1; // Convert from 0-15 to 1-16

    switch (context) {
        case TapContext::Enable:
        {
            // Calculate base parameter offset for tap control (each tap has 3 params: Enable, Level, Pan)
            int baseOffset = (tapNumber - 1) * 3;
            return kTap1Enable + baseOffset;  // Enable parameter
        }
        case TapContext::Volume:
        {
            // Calculate base parameter offset for tap control (each tap has 3 params: Enable, Level, Pan)
            int baseOffset = (tapNumber - 1) * 3;
            return kTap1Level + baseOffset;   // Level parameter
        }
        case TapContext::Pan:
        {
            // Calculate base parameter offset for tap control (each tap has 3 params: Enable, Level, Pan)
            int baseOffset = (tapNumber - 1) * 3;
            return kTap1Pan + baseOffset;     // Pan parameter
        }
        case TapContext::FilterCutoff:
        {
            // Calculate base parameter offset for filter control (each tap has 3 filter params: Cutoff, Resonance, Type)
            int filterBaseOffset = (tapNumber - 1) * 3;
            return kTap1FilterCutoff + filterBaseOffset;  // Filter cutoff parameter
        }
        case TapContext::FilterResonance:
        {
            // Calculate base parameter offset for filter control (each tap has 3 filter params: Cutoff, Resonance, Type)
            int filterBaseOffset = (tapNumber - 1) * 3;
            return kTap1FilterResonance + filterBaseOffset;  // Filter resonance parameter
        }
        case TapContext::FilterType:
        {
            // Calculate base parameter offset for filter control (each tap has 3 filter params: Cutoff, Resonance, Type)
            int filterBaseOffset = (tapNumber - 1) * 3;
            return kTap1FilterType + filterBaseOffset;  // Filter type parameter
        }
        case TapContext::PitchShift:
        {
            // Pitch shift parameters are sequential: kTap1PitchShift to kTap16PitchShift
            return kTap1PitchShift + (tapNumber - 1);  // Pitch shift parameter
        }
        case TapContext::FeedbackSend:
        {
            // Feedback send parameters are sequential: kTap1FeedbackSend to kTap16FeedbackSend
            return kTap1FeedbackSend + (tapNumber - 1);  // Feedback send parameter
        }
        default:
            // Calculate base parameter offset for tap control (each tap has 3 params: Enable, Level, Pan)
            int baseOffset = (tapNumber - 1) * 3;
            return kTap1Enable + baseOffset;  // Default to Enable
    }
}

int WaterStickEditor::getTapButtonSizeForContext(TapContext context) const
{
    switch (context) {
        case TapContext::PitchShift:
            // PitchShift context needs enlarged bounds to accommodate 3-character text ("+12")
            // Expand from 53px to 73px to provide adequate text space
            return 73;
        default:
            // All other contexts use standard 53px button size
            return 53;
    }
}

//------------------------------------------------------------------------
// TapButton Implementation
//------------------------------------------------------------------------

// Static member definition
std::set<TapButton*> TapButton::dragAffectedButtons;

TapButton::TapButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag)
: VSTGUI::CControl(size, listener, tag)
{
    setMax(1.0);  // Binary on/off button
    setMin(0.0);
}

bool TapButton::isButtonAlreadyAffected(TapButton* button) const
{
    return dragAffectedButtons.find(button) != dragAffectedButtons.end();
}

void TapButton::markButtonAsAffected(TapButton* button)
{
    dragAffectedButtons.insert(button);
}

void TapButton::draw(VSTGUI::CDrawContext* context)
{
    const VSTGUI::CRect& rect = getViewSize();
    float currentValue = getValue();

    // Set stroke width to 5px
    context->setLineWidth(5.0);
    context->setFrameColor(VSTGUI::kBlackCColor);

    // Create drawing rect that accounts for stroke width
    // Inset by half the stroke width to prevent clipping
    VSTGUI::CRect drawRect = rect;
    const double strokeInset = 2.5; // Half of 5px stroke
    drawRect.inset(strokeInset, strokeInset);

    // Draw different visualizations based on context
    switch (currentContext) {
        case TapContext::Enable:
        {
            // Original enable/disable behavior
            bool isEnabled = (currentValue > 0.5);
            if (isEnabled) {
                // Enabled state: black fill with black stroke
                context->setFillColor(VSTGUI::kBlackCColor);
                context->drawEllipse(drawRect, VSTGUI::kDrawFilled);
                context->drawEllipse(drawRect, VSTGUI::kDrawStroked);
            } else {
                // Disabled state: no fill, black stroke only
                context->drawEllipse(drawRect, VSTGUI::kDrawStroked);
            }
            break;
        }

        case TapContext::Volume:
        {
            // Volume context: simulate circular clipping by drawing fill only in circle area
            if (currentValue > 0.0) {
                // Apply intelligent scaling curve for better visual-to-audio correlation
                double scaledValue = currentValue;

                // Apply a subtle curve to prevent visual "100%" until truly at max
                if (currentValue < 1.0) {
                    scaledValue = currentValue * 0.95 + (currentValue * currentValue * 0.05);
                }

                // Calculate the circular area and fill height
                VSTGUI::CPoint center = drawRect.getCenter();
                double radius = std::min(drawRect.getWidth(), drawRect.getHeight()) / 2.0;
                double fillHeight = drawRect.getHeight() * scaledValue;
                double fillTop = drawRect.bottom - fillHeight;

                // Draw horizontal lines to create the fill effect within the circle
                context->setFillColor(VSTGUI::kBlackCColor);

                for (double y = fillTop; y <= drawRect.bottom; y += 0.5) {
                    // Calculate the width of the circle at this Y position
                    double yFromCenter = y - center.y;
                    double distanceFromCenter = std::abs(yFromCenter);

                    if (distanceFromCenter < radius) {
                        // Calculate line width using circle equation: x² + y² = r²
                        double halfLineWidth = std::sqrt(radius * radius - distanceFromCenter * distanceFromCenter);

                        VSTGUI::CRect lineRect(
                            center.x - halfLineWidth,
                            y,
                            center.x + halfLineWidth,
                            y + 0.5
                        );

                        context->drawRect(lineRect, VSTGUI::kDrawFilled);
                    }
                }
            }

            // Always draw the circle stroke on top
            context->drawEllipse(drawRect, VSTGUI::kDrawStroked);
            break;
        }

        case TapContext::Pan:
        {
            // Pan context: full-width rectangle with black fill, clipped to circle bounds
            // All pan positions show minimum 5px height rectangle centered vertically
            // Left pan (0.0) = 5px baseline + bottom half expansion
            // Center pan (0.5) = 5px baseline only
            // Right pan (1.0) = 5px baseline + top half expansion

            VSTGUI::CPoint center = drawRect.getCenter();
            double radius = std::min(drawRect.getWidth(), drawRect.getHeight()) / 2.0;

            context->setFillColor(VSTGUI::kBlackCColor);

            // Always start with the 5px baseline rectangle (2.5px above and below center)
            const double baselineHalfHeight = 2.5; // 5px total baseline height
            double fillTop = center.y - baselineHalfHeight;
            double fillBottom = center.y + baselineHalfHeight;

            if (currentValue < 0.5) {
                // Left pan (0.0-0.5): expand downward beyond baseline
                // Map 0.0-0.5 to 1.0-0.0 for additional fill amount
                double fillAmount = (0.5 - currentValue) * 2.0;

                // Calculate additional fill area from baseline bottom downward
                double additionalFillHeight = ((drawRect.getHeight() / 2.0) - baselineHalfHeight) * fillAmount;
                fillBottom = center.y + baselineHalfHeight + additionalFillHeight;
            }
            else if (currentValue > 0.5) {
                // Right pan (0.5-1.0): expand upward beyond baseline
                // Map 0.5-1.0 to 0.0-1.0 for additional fill amount
                double fillAmount = (currentValue - 0.5) * 2.0;

                // Calculate additional fill area from baseline top upward
                double additionalFillHeight = ((drawRect.getHeight() / 2.0) - baselineHalfHeight) * fillAmount;
                fillTop = center.y - baselineHalfHeight - additionalFillHeight;
            }
            // For exactly center (0.5), fillTop and fillBottom remain at baseline values

            // Draw horizontal lines to create the fill effect within the circle
            for (double y = fillTop; y <= fillBottom; y += 0.5) {
                double yFromCenter = y - center.y;
                double distanceFromCenter = std::abs(yFromCenter);

                if (distanceFromCenter < radius) {
                    // Calculate line width using circle equation: x² + y² = r²
                    double halfLineWidth = std::sqrt(radius * radius - distanceFromCenter * distanceFromCenter);

                    VSTGUI::CRect lineRect(
                        center.x - halfLineWidth,
                        y,
                        center.x + halfLineWidth,
                        y + 0.5
                    );

                    context->drawRect(lineRect, VSTGUI::kDrawFilled);
                }
            }

            // Always draw the circle stroke on top
            context->drawEllipse(drawRect, VSTGUI::kDrawStroked);
            break;
        }

        case TapContext::FilterCutoff:
        {
            // Filter Cutoff context: reuse volume visualization (circular fill)
            // Volume context: simulate circular clipping by drawing fill only in circle area
            if (currentValue > 0.0) {
                // Apply intelligent scaling curve for better visual-to-audio correlation
                double scaledValue = currentValue;

                // Apply a subtle curve to prevent visual "100%" until truly at max
                if (currentValue < 1.0) {
                    scaledValue = currentValue * 0.95 + (currentValue * currentValue * 0.05);
                }

                // Calculate the circular area and fill height
                VSTGUI::CPoint center = drawRect.getCenter();
                double radius = std::min(drawRect.getWidth(), drawRect.getHeight()) / 2.0;
                double fillHeight = drawRect.getHeight() * scaledValue;
                double fillTop = drawRect.bottom - fillHeight;

                // Draw horizontal lines to create the fill effect within the circle
                context->setFillColor(VSTGUI::kBlackCColor);

                for (double y = fillTop; y <= drawRect.bottom; y += 0.5) {
                    // Calculate the width of the circle at this Y position
                    double yFromCenter = y - center.y;
                    double distanceFromCenter = std::abs(yFromCenter);

                    if (distanceFromCenter < radius) {
                        // Calculate line width using circle equation: x² + y² = r²
                        double halfLineWidth = std::sqrt(radius * radius - distanceFromCenter * distanceFromCenter);

                        VSTGUI::CRect lineRect(
                            center.x - halfLineWidth,
                            y,
                            center.x + halfLineWidth,
                            y + 0.5
                        );

                        context->drawRect(lineRect, VSTGUI::kDrawFilled);
                    }
                }
            }

            // Always draw the circle stroke on top
            context->drawEllipse(drawRect, VSTGUI::kDrawStroked);
            break;
        }

        case TapContext::FilterResonance:
        {
            // Filter Resonance context: reuse pan visualization (rectangle with 5px baseline)
            // All resonance values show minimum 5px height rectangle centered vertically
            // Low resonance (0.0) = 5px baseline + bottom half expansion
            // Center resonance (0.5) = 5px baseline only
            // High resonance (1.0) = 5px baseline + top half expansion

            VSTGUI::CPoint center = drawRect.getCenter();
            double radius = std::min(drawRect.getWidth(), drawRect.getHeight()) / 2.0;

            context->setFillColor(VSTGUI::kBlackCColor);

            // Always start with the 5px baseline rectangle (2.5px above and below center)
            const double baselineHalfHeight = 2.5; // 5px total baseline height
            double fillTop = center.y - baselineHalfHeight;
            double fillBottom = center.y + baselineHalfHeight;

            if (currentValue < 0.5) {
                // Low resonance (0.0-0.5): expand downward beyond baseline
                // Map 0.0-0.5 to 1.0-0.0 for additional fill amount
                double fillAmount = (0.5 - currentValue) * 2.0;

                // Calculate additional fill area from baseline bottom downward
                double additionalFillHeight = ((drawRect.getHeight() / 2.0) - baselineHalfHeight) * fillAmount;
                fillBottom = center.y + baselineHalfHeight + additionalFillHeight;
            }
            else if (currentValue > 0.5) {
                // High resonance (0.5-1.0): expand upward beyond baseline
                // Map 0.5-1.0 to 0.0-1.0 for additional fill amount
                double fillAmount = (currentValue - 0.5) * 2.0;

                // Calculate additional fill area from baseline top upward
                double additionalFillHeight = ((drawRect.getHeight() / 2.0) - baselineHalfHeight) * fillAmount;
                fillTop = center.y - baselineHalfHeight - additionalFillHeight;
            }
            // For exactly center (0.5), fillTop and fillBottom remain at baseline values

            // Draw horizontal lines to create the fill effect within the circle
            for (double y = fillTop; y <= fillBottom; y += 0.5) {
                double yFromCenter = y - center.y;
                double distanceFromCenter = std::abs(yFromCenter);

                if (distanceFromCenter < radius) {
                    // Calculate line width using circle equation: x² + y² = r²
                    double halfLineWidth = std::sqrt(radius * radius - distanceFromCenter * distanceFromCenter);

                    VSTGUI::CRect lineRect(
                        center.x - halfLineWidth,
                        y,
                        center.x + halfLineWidth,
                        y + 0.5
                    );

                    context->drawRect(lineRect, VSTGUI::kDrawFilled);
                }
            }

            // Always draw the circle stroke on top
            context->drawEllipse(drawRect, VSTGUI::kDrawStroked);
            break;
        }

        case TapContext::FilterType:
        {
            // Filter Type context: display letters X, L, H, B, N based on filter type value
            // Parameter values: 0.0-0.2=X (bypass), 0.2-0.4=L, 0.4-0.6=H, 0.6-0.8=B, 0.8-1.0=N

            char letter;
            if (currentValue < 0.2) {
                letter = 'X';  // Bypass (no filtering)
            } else if (currentValue < 0.4) {
                letter = 'L';  // Low Pass
            } else if (currentValue < 0.6) {
                letter = 'H';  // High Pass
            } else if (currentValue < 0.8) {
                letter = 'B';  // Band Pass
            } else {
                letter = 'N';  // Notch
            }


            // Use WorkSans-Regular custom font sized to fill the circle area
            // Circle diameter is 48px, make font large enough so ascenders/descenders reach edges
            auto editor = dynamic_cast<WaterStickEditor*>(listener);
            auto customFont = editor ? editor->getWorkSansFont(48.0f) : nullptr;

            if (customFont) {
                // Set custom font in context
                context->setFont(customFont);
                context->setFontColor(VSTGUI::kBlackCColor);
            } else {
                // Fallback to system font if custom font fails
                auto systemFont = VSTGUI::kSystemFont;
                systemFont->setSize(48);
                context->setFont(systemFont);
                context->setFontColor(VSTGUI::kBlackCColor);
            }

            // Create single character string
            std::string letterStr(1, letter);

            // Calculate text position to center the letter properly
            VSTGUI::CPoint center = drawRect.getCenter();

            // Get text dimensions for precise centering
            auto textWidth = context->getStringWidth(letterStr.c_str());

            // For proper vertical centering, we need to account for the baseline
            // Use a more accurate method to center the text
            float fontSize = customFont ? customFont->getSize() : 48.0f;
            VSTGUI::CPoint textPos(
                center.x - textWidth / 2.0,
                center.y + fontSize / 3.0  // Baseline adjustment for visual centering
            );

            // Draw the letter (no circle stroke in FilterType context)
            context->drawString(letterStr.c_str(), textPos);
            break;
        }

        case TapContext::PitchShift:
        {
            // Pitch Shift context: display numerical semitone value as text (similar to FilterType)
            // Parameter range: 0.0 = -12 semitones, 0.5 = 0 semitones, 1.0 = +12 semitones

            // CRITICAL FIX: Explicit background clearing to prevent graphics persistence
            // PitchShift context uses enlarged bounds (73px vs 53px) and must clear the entire area
            context->setFillColor(VSTGUI::kWhiteCColor);
            context->drawRect(rect, VSTGUI::kDrawFilled);

            // Convert normalized value to semitones (-12 to +12) with proper rounding
            int semitones = static_cast<int>(std::round((currentValue - 0.5) * 24.0));

            // Create semitone text with appropriate formatting
            std::string semitoneText;
            if (semitones == 0) {
                semitoneText = "0";
            } else if (semitones > 0) {
                semitoneText = std::to_string(semitones);
            } else {
                semitoneText = std::to_string(semitones);
            }

            // Use WorkSans-Regular custom font sized consistently at 48.0f
            auto editor = dynamic_cast<WaterStickEditor*>(listener);
            auto customFont = editor ? editor->getWorkSansFont(48.0f) : nullptr;

            if (customFont) {
                // Set custom font in context
                context->setFont(customFont);
                context->setFontColor(VSTGUI::kBlackCColor);
            } else {
                // Fallback to system font if custom font fails
                auto systemFont = VSTGUI::kSystemFont;
                systemFont->setSize(48);
                context->setFont(systemFont);
                context->setFontColor(VSTGUI::kBlackCColor);
            }

            // Calculate text position to center the semitone value properly
            // With enlarged view bounds, we can now use the full drawing rect
            VSTGUI::CPoint center = drawRect.getCenter();

            // Get text dimensions for precise centering
            auto textWidth = context->getStringWidth(semitoneText.c_str());

            // For proper vertical centering, we need to account for the baseline
            // Use a more accurate method to center the text
            float fontSize = customFont ? customFont->getSize() : 48.0f;
            VSTGUI::CPoint textPos(
                center.x - textWidth / 2.0,
                center.y + fontSize / 3.0  // Baseline adjustment for visual centering
            );

            // Draw the semitone value (no circle stroke in PitchShift context)
            context->drawString(semitoneText.c_str(), textPos);
            break;
        }

        case TapContext::FeedbackSend:
        {
            // FeedbackSend context: reuse Volume visualization (circular fill from bottom)
            // Display 0-100% feedback send levels in tap buttons
            if (currentValue > 0.0) {
                // Apply intelligent scaling curve for better visual-to-audio correlation
                double scaledValue = currentValue;

                // Apply a subtle curve to prevent visual "100%" until truly at max
                if (currentValue < 1.0) {
                    scaledValue = currentValue * 0.95 + (currentValue * currentValue * 0.05);
                }

                // Calculate the circular area and fill height
                VSTGUI::CPoint center = drawRect.getCenter();
                double radius = std::min(drawRect.getWidth(), drawRect.getHeight()) / 2.0;
                double fillHeight = drawRect.getHeight() * scaledValue;
                double fillTop = drawRect.bottom - fillHeight;

                // Draw horizontal lines to create the fill effect within the circle
                context->setFillColor(VSTGUI::kBlackCColor);

                for (double y = fillTop; y <= drawRect.bottom; y += 0.5) {
                    // Calculate the width of the circle at this Y position
                    double yFromCenter = y - center.y;
                    double distanceFromCenter = std::abs(yFromCenter);

                    if (distanceFromCenter < radius) {
                        // Calculate line width using circle equation: x² + y² = r²
                        double halfLineWidth = std::sqrt(radius * radius - distanceFromCenter * distanceFromCenter);

                        VSTGUI::CRect lineRect(
                            center.x - halfLineWidth,
                            y,
                            center.x + halfLineWidth,
                            y + 0.5
                        );

                        context->drawRect(lineRect, VSTGUI::kDrawFilled);
                    }
                }
            }

            // Always draw the circle stroke on top
            context->drawEllipse(drawRect, VSTGUI::kDrawStroked);
            break;
        }

        default:
        {
            // Future contexts: for now, just draw outline
            context->drawEllipse(drawRect, VSTGUI::kDrawStroked);
            break;
        }
    }

    setDirty(false);
}

VSTGUI::CMouseEventResult TapButton::onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (buttons & VSTGUI::kLButton) {
        auto currentTime = std::chrono::steady_clock::now();

        // Check for double-click (only for non-Enable contexts)
        if (currentContext != TapContext::Enable && isDoubleClick(currentTime)) {
            resetToDefaultValue();
            lastClickTime = currentTime;
            return VSTGUI::kMouseEventHandled;
        }

        // Update last click time for next double-click detection
        lastClickTime = currentTime;

        if (currentContext == TapContext::Volume || currentContext == TapContext::Pan ||
            currentContext == TapContext::FilterCutoff || currentContext == TapContext::FilterResonance ||
            currentContext == TapContext::FilterType || currentContext == TapContext::PitchShift ||
            currentContext == TapContext::FeedbackSend) {
            // Volume, Pan, Filter Cutoff, Filter Resonance, Filter Type, and Pitch Shift contexts: Handle continuous control
            isVolumeInteracting = true;
            initialClickPoint = where;
            initialVolumeValue = getValue();

            // For now, don't change the value - wait to see if it's a click or drag
            return VSTGUI::kMouseEventHandled;
        }
        else {
            // Enable context: Original toggle behavior
            dragMode = true;
            resetDragAffectedSet();

            setValue(getValue() > 0.5 ? 0.0 : 1.0);
            invalid();
            markButtonAsAffected(this);

            if (listener) {
                listener->valueChanged(this);
            }

            return VSTGUI::kMouseEventHandled;
        }
    }
    return VSTGUI::kMouseEventNotHandled;
}

VSTGUI::CMouseEventResult TapButton::onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (isVolumeInteracting && (buttons & VSTGUI::kLButton)) {
        // Volume and Pan contexts: Handle continuous dragging
        double deltaX = where.x - initialClickPoint.x;
        double deltaY = initialClickPoint.y - where.y;  // Negative deltaY = drag up = increase value

        // Determine drag direction if not already set
        if (currentDragDirection == DragDirection::None) {
            double distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);

            if (distance > DRAG_THRESHOLD) {
                // Determine primary direction based on larger component
                if (std::abs(deltaX) > std::abs(deltaY)) {
                    currentDragDirection = DragDirection::Horizontal;
                } else {
                    currentDragDirection = DragDirection::Vertical;
                }
            }
        }

        if (currentDragDirection == DragDirection::Horizontal && currentContext != TapContext::FilterType) {
            // Horizontal drag: Set value on different taps based on mouse position
            // Note: FilterType context does not support horizontal drag
            VSTGUI::CPoint framePoint = where;
            localToFrame(framePoint);

            auto editor = dynamic_cast<WaterStickEditor*>(listener);
            if (editor) {
                auto targetButton = editor->getTapButtonAtPoint(framePoint);
                if (targetButton) {
                    // Calculate value based on vertical position within the target button
                    const VSTGUI::CRect& targetRect = targetButton->getViewSize();
                    VSTGUI::CRect targetDrawRect = targetRect;
                    const double strokeInset = 2.5; // Half of 5px stroke
                    targetDrawRect.inset(strokeInset, strokeInset);

                    // Convert frame coordinates to target button local coordinates
                    VSTGUI::CPoint localPoint = framePoint;
                    targetButton->frameToLocal(localPoint);

                    double newValue;
                    if (currentContext == TapContext::Volume || currentContext == TapContext::FilterCutoff ||
                        currentContext == TapContext::FilterType || currentContext == TapContext::FeedbackSend) {
                        // Volume, Filter Cutoff, Filter Type, FeedbackSend: bottom = 0.0, top = 1.0
                        double relativeY = (targetDrawRect.bottom - localPoint.y) / targetDrawRect.getHeight();
                        newValue = std::max(0.0, std::min(1.0, relativeY));
                    } else { // TapContext::Pan, TapContext::FilterResonance, or TapContext::PitchShift
                        // Pan, Filter Resonance, and Pitch Shift: bottom = 0.0, center = 0.5, top = 1.0 (bidirectional)
                        double relativeY = (targetDrawRect.bottom - localPoint.y) / targetDrawRect.getHeight();
                        newValue = std::max(0.0, std::min(1.0, relativeY));
                    }

                    // Update the target button's value
                    targetButton->setValue(newValue);
                    targetButton->invalid();
                    editor->valueChanged(targetButton);
                }
            }
        }
        else if (currentDragDirection == DragDirection::Vertical) {
            // Vertical drag: Relative adjustment on this button
            double sensitivity = 1.0 / 52.5;  // 52.5 pixels = full range (0.0 to 1.0) - scaled by 1.75x
            double valueChange = deltaY * sensitivity;

            double newValue = initialVolumeValue + valueChange;
            newValue = std::max(0.0, std::min(1.0, newValue));  // Clamp to [0.0, 1.0]

            setValue(newValue);
            invalid();

            if (listener) {
                listener->valueChanged(this);
            }
        }

        return VSTGUI::kMouseEventHandled;
    }
    else if (dragMode && (buttons & VSTGUI::kLButton)) {
        // Enable context: Original drag behavior
        VSTGUI::CPoint framePoint = where;
        localToFrame(framePoint);

        auto editor = dynamic_cast<WaterStickEditor*>(listener);
        if (editor) {
            auto targetButton = editor->getTapButtonAtPoint(framePoint);
            if (targetButton && !isButtonAlreadyAffected(targetButton)) {
                double newValue = targetButton->getValue() > 0.5 ? 0.0 : 1.0;
                targetButton->setValue(newValue);
                targetButton->invalid();
                editor->valueChanged(targetButton);
                markButtonAsAffected(targetButton);
            }
        }

        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

VSTGUI::CMouseEventResult TapButton::onMouseUp(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (isVolumeInteracting) {
        // Volume and Pan contexts: Check if this was a click or drag
        if (currentDragDirection == DragDirection::None) {
            // This was a click - set value based on absolute position within circle
            const VSTGUI::CRect& rect = getViewSize();
            VSTGUI::CRect drawRect = rect;
            const double strokeInset = 2.5; // Half of 5px stroke
            drawRect.inset(strokeInset, strokeInset);

            double newValue;
            if (currentContext == TapContext::Volume || currentContext == TapContext::FilterCutoff ||
                currentContext == TapContext::FilterType || currentContext == TapContext::FeedbackSend) {
                // Volume, Filter Cutoff, Filter Type, FeedbackSend: bottom = 0.0, top = 1.0
                double relativeY = (drawRect.bottom - where.y) / drawRect.getHeight();
                newValue = std::max(0.0, std::min(1.0, relativeY));
            } else { // TapContext::Pan, TapContext::FilterResonance, or TapContext::PitchShift
                // Pan, Filter Resonance, and Pitch Shift: bottom = 0.0, center = 0.5, top = 1.0 (bidirectional)
                double relativeY = (drawRect.bottom - where.y) / drawRect.getHeight();
                newValue = std::max(0.0, std::min(1.0, relativeY));
            }

            // Set the value
            setValue(newValue);
            invalid();

            // Notify listener
            if (listener) {
                listener->valueChanged(this);
            }
        }
        // If it was a drag (vertical or horizontal), the value was already set during onMouseMoved

        // Reset interaction state
        isVolumeInteracting = false;
        currentDragDirection = DragDirection::None;
        return VSTGUI::kMouseEventHandled;
    }
    else if (dragMode) {
        // Enable context: Original behavior
        dragMode = false;
        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

//------------------------------------------------------------------------
// TapButton Double-Click Helper Methods
//------------------------------------------------------------------------

bool TapButton::isDoubleClick(const std::chrono::steady_clock::time_point& currentTime)
{
    auto timeSinceLastClick = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastClickTime);
    return timeSinceLastClick <= DOUBLE_CLICK_TIMEOUT && timeSinceLastClick > std::chrono::milliseconds{0};
}

void TapButton::resetToDefaultValue()
{
    // Only reset if not in Enable context (per requirements)
    if (currentContext == TapContext::Enable) {
        return;
    }

    float defaultValue = getContextDefaultValue();
    setValue(defaultValue);

    invalid(); // Trigger visual update

    if (listener) {
        listener->valueChanged(this);
    }
}

void TapButton::updateViewBoundsForContext(TapContext context, WaterStickEditor* editor)
{
    if (!editor) return;

    // Get the required size for this context
    int requiredSize = editor->getTapButtonSizeForContext(context);
    int currentSize = static_cast<int>(getViewSize().getWidth());

    // Only update if the size actually needs to change
    if (requiredSize != currentSize) {
        // Get current view bounds
        VSTGUI::CRect currentBounds = getViewSize();
        VSTGUI::CPoint center = currentBounds.getCenter();

        // CRITICAL FIX: When shrinking from larger bounds (e.g., PitchShift 73px -> 53px),
        // we need to ensure the parent container invalidates the larger area to clear artifacts
        if (requiredSize < currentSize) {
            // Force parent container to redraw the larger area before we shrink
            if (getParentView()) {
                getParentView()->invalid();
            }
        }

        // Calculate new bounds centered at the same position
        double halfSize = requiredSize / 2.0;
        VSTGUI::CRect newBounds(
            center.x - halfSize,
            center.y - halfSize,
            center.x + halfSize,
            center.y + halfSize
        );

        // Update view size
        setViewSize(newBounds);
        setMouseableArea(newBounds);

        // Force complete redraw with proper state clearing
        setDirty(true);
        invalid(); // Trigger redraw with new bounds
    }
}

float TapButton::getContextDefaultValue() const
{
    switch (currentContext) {
        case TapContext::Volume:
            return 0.8f; // 80% volume
        case TapContext::Pan:
            return 0.5f; // Center pan
        case TapContext::FilterCutoff:
            return 0.566323334778673f; // 1kHz cutoff (matches controller default)
        case TapContext::FilterResonance:
            return 0.5f; // Moderate resonance
        case TapContext::FilterType:
            return 0.0f; // Bypass filter
        case TapContext::PitchShift:
            return 0.5f; // 0 semitones (no pitch shift)
        case TapContext::FeedbackSend:
            return 0.0f; // No feedback send (0%)
        case TapContext::Enable:
        default:
            return 0.0f; // Not used for Enable context
    }
}

//------------------------------------------------------------------------
// ModeButton Implementation
//------------------------------------------------------------------------

ModeButton::ModeButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag)
: VSTGUI::CControl(size, listener, tag)
{
    setMax(1.0);  // Binary on/off button
    setMin(0.0);
    setValue(0.0); // Start in unselected state
}

void ModeButton::draw(VSTGUI::CDrawContext* context)
{
    const VSTGUI::CRect& rect = getViewSize();
    bool isSelected = (getValue() > 0.5);

    // Define the logical button area (53x53px) centered in the expanded view (scaled by 1.75x)
    const double buttonSize = 53.0;
    VSTGUI::CPoint viewCenter = rect.getCenter();
    const double halfButtonSize = buttonSize / 2.0;

    VSTGUI::CRect buttonRect(
        viewCenter.x - halfButtonSize,
        viewCenter.y - halfButtonSize,
        viewCenter.x + halfButtonSize,
        viewCenter.y + halfButtonSize
    );

    if (isSelected) {
        // Calculate rectangle size as 1.5x the circle size
        const double strokeInset = 2.5; // Half of 5px stroke width (keeping 5px)
        const double circleSize = buttonSize - (strokeInset * 2); // 53px - 5px = 48px
        const double rectangleSize = circleSize * 1.5; // 48px * 1.5 = 72px

        // Center rectangle on the button's center
        const double halfRectSize = rectangleSize / 2.0;

        VSTGUI::CRect backgroundRect(
            viewCenter.x - halfRectSize,
            viewCenter.y - halfRectSize,
            viewCenter.x + halfRectSize,
            viewCenter.y + halfRectSize
        );

        // Draw black rounded rectangle background (no stroke, fill only)
        const double cornerRadius = 14.0; // Circular corner radius (8 * 1.75 = 14)
        context->setFillColor(VSTGUI::kBlackCColor);
        context->setDrawMode(VSTGUI::kAntiAliasing);

        // Use graphics path for rounded rectangle
        VSTGUI::CGraphicsPath* path = context->createRoundRectGraphicsPath(backgroundRect, cornerRadius);
        if (path) {
            context->drawGraphicsPath(path, VSTGUI::CDrawContext::kPathFilled);
            path->forget();
        }
    }

    // Set stroke width to 5px (matching tap buttons)
    context->setLineWidth(5.0);

    // Create drawing rect that accounts for stroke width
    // Inset by half the stroke width to prevent clipping
    VSTGUI::CRect drawRect = buttonRect;
    const double strokeInset = 2.5; // Half of 5px stroke
    drawRect.inset(strokeInset, strokeInset);

    if (isSelected) {
        // Selected state: white circle stroke, no fill
        context->setFrameColor(VSTGUI::kWhiteCColor);
        context->drawEllipse(drawRect, VSTGUI::kDrawStroked);
    } else {
        // Unselected state: black circle stroke, no fill
        context->setFrameColor(VSTGUI::kBlackCColor);
        context->drawEllipse(drawRect, VSTGUI::kDrawStroked);
    }

    // Draw center dot (12px diameter, scaled by 1.75x)
    const double centerDotRadius = 6.125; // 12.25px diameter = 6.125px radius (7 * 1.75)
    VSTGUI::CPoint center = drawRect.getCenter();
    VSTGUI::CRect centerDotRect(
        center.x - centerDotRadius,
        center.y - centerDotRadius,
        center.x + centerDotRadius,
        center.y + centerDotRadius
    );

    if (isSelected) {
        // Selected state: white center dot (inverted)
        context->setFillColor(VSTGUI::kWhiteCColor);
        context->drawEllipse(centerDotRect, VSTGUI::kDrawFilled);
    } else {
        // Unselected state: black center dot
        context->setFillColor(VSTGUI::kBlackCColor);
        context->drawEllipse(centerDotRect, VSTGUI::kDrawFilled);
    }

    setDirty(false);
}

VSTGUI::CMouseEventResult ModeButton::onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (buttons & VSTGUI::kLButton) {
        // Only allow setting to selected state (1.0), don't allow deselecting
        // The mutual exclusion system will handle deselecting other buttons
        if (getValue() <= 0.5) {
            setValue(1.0);
            invalid();  // Trigger redraw

            // Notify listener
            if (listener) {
                listener->valueChanged(this);
            }
        }

        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

//------------------------------------------------------------------------
// KnobControl Implementation

KnobControl::KnobControl(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag)
: CControl(size, listener, tag)
{
}

void KnobControl::draw(VSTGUI::CDrawContext* context)
{

    VSTGUI::CRect drawRect = getViewSize();
    drawRect.makeIntegral();

    // Inset the drawing rect to prevent stroke clipping (5px stroke = 2.5px inset on each side)
    drawRect.inset(2.5, 2.5);

    // Draw black circle outline (5px stroke, same as tap buttons)
    context->setLineWidth(5.0);
    context->setLineStyle(VSTGUI::kLineSolid);
    context->setDrawMode(VSTGUI::kAliasing);
    context->setFrameColor(VSTGUI::kBlackCColor);
    context->drawEllipse(drawRect, VSTGUI::kDrawStroked);

    // Calculate dot position based on value (pot range: ~300 degrees, rotated 90 degrees left)
    float value = getValue();
    float angle = -225.0f + (value * 270.0f); // Start at 10:30, end at 1:30 (rotated 90 degrees left)
    float angleRad = angle * M_PI / 180.0f;

    // Get circle center and calculate inner radius for dot positioning
    VSTGUI::CPoint center = drawRect.getCenter();
    float outerRadius = (drawRect.getWidth() / 2.0f) - 2.5f; // Account for stroke width
    float dotRadius = 6.125f; // Same size as mode button dots (12.25px diameter)

    // Calculate the distance from center to dot center
    // Maintain half dot width space from inner edge
    float dotCenterDistance = outerRadius - dotRadius - (dotRadius / 2.0f);

    // Calculate dot position
    VSTGUI::CPoint dotCenter(
        center.x + dotCenterDistance * cos(angleRad),
        center.y + dotCenterDistance * sin(angleRad)
    );

    // Draw the dot
    VSTGUI::CRect dotRect(
        dotCenter.x - dotRadius,
        dotCenter.y - dotRadius,
        dotCenter.x + dotRadius,
        dotCenter.y + dotRadius
    );

    context->setFillColor(VSTGUI::kBlackCColor);
    context->drawEllipse(dotRect, VSTGUI::kDrawFilled);

    setDirty(false);
}

VSTGUI::CMouseEventResult KnobControl::onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (buttons & VSTGUI::kLButton) {
        auto currentTime = std::chrono::steady_clock::now();

        // Check for double-click
        if (isDoubleClick(currentTime)) {
            resetToDefaultValue();
            lastClickTime = currentTime;
            return VSTGUI::kMouseEventHandled;
        }

        // Update last click time for next double-click detection
        lastClickTime = currentTime;

        isDragging = true;
        lastMousePos = where;
        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

VSTGUI::CMouseEventResult KnobControl::onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (isDragging && (buttons & VSTGUI::kLButton)) {
        // Calculate vertical movement for value change
        float deltaY = lastMousePos.y - where.y; // Positive = mouse moved up
        float sensitivity = 0.005f; // Adjust for desired sensitivity

        float newValue = getValue() + (deltaY * sensitivity);
        newValue = std::max(0.0f, std::min(1.0f, newValue)); // Clamp to 0-1

        setValue(newValue);
        invalid(); // Trigger redraw

        if (listener) {
            listener->valueChanged(this);
        }

        lastMousePos = where;
        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

VSTGUI::CMouseEventResult KnobControl::onMouseUp(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (isDragging) {
        isDragging = false;
        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

//------------------------------------------------------------------------
// KnobControl Double-Click Helper Methods
//------------------------------------------------------------------------

bool KnobControl::isDoubleClick(const std::chrono::steady_clock::time_point& currentTime)
{
    auto timeSinceLastClick = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastClickTime);
    return timeSinceLastClick <= DOUBLE_CLICK_TIMEOUT && timeSinceLastClick > std::chrono::milliseconds{0};
}

void KnobControl::resetToDefaultValue()
{
    auto editor = dynamic_cast<WaterStickEditor*>(listener);
    if (!editor) return;

    auto controller = editor->getController();
    if (!controller) return;

    // Get parameter default value from controller
    int paramTag = getTag();
    float defaultValue = 0.0f;

    // Get default based on parameter type
    switch (paramTag) {
        case kInputGain:
        case kOutputGain:
            defaultValue = 40.0f/52.0f; // 0dB gain
            break;
        case kTempoSyncMode:
            defaultValue = 0.0f; // Free mode
            break;
        case kDelayTime:
            defaultValue = 0.05f; // Short delay
            break;
        case kFeedback:
            defaultValue = 0.0f; // No feedback
            break;
        case kGrid:
            defaultValue = static_cast<float>(kGrid_4) / (kNumGridValues - 1); // 4 taps/beat
            break;
        case kGlobalDryWet:
            defaultValue = 0.5f; // 50% mix
            break;
        default:
            return; // Unknown parameter, don't reset
    }

    setValue(defaultValue);

    invalid(); // Trigger visual update

    if (listener) {
        listener->valueChanged(this);
    }
}

//========================================================================
// MinimapTapButton Implementation
//========================================================================

MinimapTapButton::MinimapTapButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag, WaterStickEditor* editor, int tapIndex)
: VSTGUI::CControl(size, listener, tag), editor(editor), tapIndex(tapIndex)
{
}

char MinimapTapButton::getFilterTypeChar(float filterTypeValue) const
{
    // Match the exact mapping from TapButton implementation
    if (filterTypeValue < 0.2) {
        return 'X';  // Bypass (no filtering)
    } else if (filterTypeValue < 0.4) {
        return 'L';  // Low Pass
    } else if (filterTypeValue < 0.6) {
        return 'H';  // High Pass
    } else if (filterTypeValue < 0.8) {
        return 'B';  // Band Pass
    } else {
        return 'N';  // Notch
    }
}

void MinimapTapButton::draw(VSTGUI::CDrawContext* context)
{
    // Get the actual drawing rectangle
    VSTGUI::CRect rect = getViewSize();

    // Define circle parameters - conditional sizing based on context
    // Enable context (filter type text): 16px circles for 11.0f font
    // Other contexts (enable/disable): 13px circles for traditional sizing
    const double circleSize = (editor && editor->getCurrentContext() == TapContext::Enable) ? 16.0 : 13.0;
    VSTGUI::CPoint center = rect.getCenter();
    const double radius = circleSize / 2.0;

    // Create circle rect centered in the button area
    VSTGUI::CRect circleRect(
        center.x - radius,
        center.y - radius,
        center.x + radius,
        center.y + radius
    );

    context->setLineWidth(1.0);
    context->setDrawMode(VSTGUI::kAntiAliasing);
    context->setFrameColor(VSTGUI::kBlackCColor);

    // Check if we're in tap mutes context (Enable context) and should show filter types
    if (editor && editor->getCurrentContext() == TapContext::Enable) {
        // Get filter type value from the corresponding tap button
        auto controller = editor->getController();
        if (controller) {
            int filterTypeParamId = editor->getTapParameterIdForContext(tapIndex, TapContext::FilterType);
            float filterTypeValue = controller->getParamNormalized(filterTypeParamId);
            char filterChar = getFilterTypeChar(filterTypeValue);

            // Draw filter type letter without background circle
            auto customFont = editor->getWorkSansFont(11.0f);
            if (customFont) {
                context->setFont(customFont);
                context->setFontColor(VSTGUI::kBlackCColor);

                // Create string and measure it for centering
                std::string letterStr(1, filterChar);
                auto textSize = context->getStringWidth(letterStr.c_str());

                // Center text with proper baseline positioning
                VSTGUI::CPoint textPos(
                    center.x - textSize / 2.0,
                    center.y + (customFont->getSize() * 0.3f)
                );

                context->drawString(letterStr.c_str(), textPos);
            }
        }
    } else {
        // Standard circle display for other contexts
        // Determine if tap is enabled (getValue > 0.5 means enabled)
        bool isEnabled = (getValue() > 0.5);

        // Fill based on enabled state
        if (isEnabled) {
            context->setFillColor(VSTGUI::kBlackCColor);
            context->drawEllipse(circleRect, VSTGUI::kDrawFilledAndStroked);
        } else {
            context->setFillColor(VSTGUI::kWhiteCColor);
            context->drawEllipse(circleRect, VSTGUI::kDrawFilledAndStroked);
        }
    }

    setDirty(false);
}

//========================================================================
// Minimap Implementation
//========================================================================

void WaterStickEditor::createMinimap(VSTGUI::CViewContainer* container)
{
    // Integrated minimap positioning: Place minimap circles directly above corresponding tap buttons
    const int minimapCircleSize = 13; // Keep existing size

    // Get tap array positioning constants (must match createTapButtons exactly)
    const int buttonSize = 53;
    const int buttonSpacing = buttonSize / 2; // 26.5px spacing
    const int gridWidth = 8;
    const int gridHeight = 2;
    const int totalGridHeight = (gridHeight * buttonSize) + ((gridHeight - 1) * buttonSpacing);
    const int upperTwoThirdsHeight = (kEditorHeight * 2) / 3;
    const int delayMargin = 30;
    const int gridLeft = delayMargin;
    // Center content vertically by moving up 15px from calculated position (synchronized with tap button positioning)
    const int gridTop = ((upperTwoThirdsHeight - totalGridHeight) / 2) - 15;

    // Create minimap buttons positioned relative to their corresponding tap buttons
    for (int i = 0; i < 16; i++) {
        int row = i / 8;        // 0 for taps 1-8, 1 for taps 9-16
        int col = i % 8;        // 0-7 for column position

        // Calculate position using the specified positioning logic
        double minimapX = gridLeft + col * (buttonSize + buttonSpacing) + (buttonSize / 2.0) - 6.5;
        double minimapY;

        if (row == 0) {
            // Row 1 circles: Position 13.25px above tap buttons (consistent clearance)
            minimapY = gridTop - 19.75;
        } else {
            // Row 2 circles: Position at center of 26.5px gap between rows
            minimapY = gridTop + 53 + 6.75;
        }

        // Create minimap button rect
        VSTGUI::CRect buttonRect(
            static_cast<int>(minimapX),
            static_cast<int>(minimapY),
            static_cast<int>(minimapX + minimapCircleSize),
            static_cast<int>(minimapY + minimapCircleSize)
        );

        // Create minimap button (non-interactive, display only)
        minimapButtons[i] = new MinimapTapButton(buttonRect, nullptr, -1, this, i);

        // Initialize with current tap enable state
        auto controller = getController();
        if (controller) {
            Steinberg::Vst::ParamID paramId = kTap1Enable + (i * 3);
            Steinberg::Vst::ParamValue paramValue = controller->getParamNormalized(paramId);
            minimapButtons[i]->setValue(static_cast<float>(paramValue));
        }

        // Add directly to main container (not a separate minimap container)
        container->addView(minimapButtons[i]);
    }

    // No separate minimap container needed - circles are positioned individually
}

void WaterStickEditor::updateMinimapState()
{
    // Update minimap buttons to reflect current tap enable states
    auto controller = getController();
    if (!controller) return;

    for (int i = 0; i < 16; i++) {
        if (minimapButtons[i]) {
            // Use proper parameter ID mapping (each tap has 3 params: Enable, Level, Pan)
            Steinberg::Vst::ParamID paramId = kTap1Enable + (i * 3);
            Steinberg::Vst::ParamValue paramValue = controller->getParamNormalized(paramId);
            minimapButtons[i]->setValue(static_cast<float>(paramValue));
            minimapButtons[i]->invalid(); // Trigger redraw
        }
    }
}

void WaterStickEditor::forceParameterSynchronization()
{
    auto controller = getController();
    if (!controller) return;


    // VST3 Lifecycle Compliance: Ensure GUI displays correct parameter values regardless of
    // timing between setComponentState, createView, and host parameter cache behavior

    // Sync all tap button contexts with current parameter values
    for (int i = 0; i < 16; i++) {
        if (tapButtons[i]) {
            auto tapButton = static_cast<TapButton*>(tapButtons[i]);

            // Load current parameter values for ALL contexts (not just current)
            for (int contextIndex = 0; contextIndex < static_cast<int>(TapContext::COUNT); contextIndex++) {
                TapContext context = static_cast<TapContext>(contextIndex);
                int paramId = getTapParameterIdForContext(i, context);
                float paramValue = controller->getParamNormalized(paramId);
                tapButton->setContextValue(context, paramValue);

                // Log critical parameter loading for all problematic contexts
                std::ostringstream contextStr, paramName;
                contextStr << "TAP-LOAD[" << (i+1) << "]";

                switch (context) {
                    case TapContext::FilterType:
                        paramName << "Tap" << (i+1) << "FilterType";
                        WS_LOG_PARAM_CONTEXT(contextStr.str(), paramId, paramName.str(), paramValue);
                        break;
                    case TapContext::Volume:
                        paramName << "Tap" << (i+1) << "Level";
                        WS_LOG_PARAM_CONTEXT(contextStr.str(), paramId, paramName.str(), paramValue);
                        break;
                    case TapContext::Pan:
                        paramName << "Tap" << (i+1) << "Pan";
                        WS_LOG_PARAM_CONTEXT(contextStr.str(), paramId, paramName.str(), paramValue);
                        break;
                    case TapContext::FilterCutoff:
                        paramName << "Tap" << (i+1) << "FilterCutoff";
                        WS_LOG_PARAM_CONTEXT(contextStr.str(), paramId, paramName.str(), paramValue);
                        break;
                    case TapContext::FeedbackSend:
                        paramName << "Tap" << (i+1) << "FeedbackSend";
                        WS_LOG_PARAM_CONTEXT(contextStr.str(), paramId, paramName.str(), paramValue);
                        break;
                    default:
                        break;
                }
            }

            // Set the button's displayed value to match its current context
            TapContext buttonContext = tapButton->getContext();
            float currentContextValue = tapButton->getContextValue(buttonContext);
            tapButton->setValue(currentContextValue);
            tapButton->invalid();

        }
    }

    // Sync bypass toggle separately
    if (delayBypassToggle) {
        float bypassValue = controller->getParamNormalized(kDelayBypass);
        delayBypassToggle->setValue(bypassValue);
        delayBypassToggle->invalid();
    }

    // Sync all global knobs with current parameter values
    const int knobTags[] = {kTempoSyncMode, kDelayTime, kFeedback, kGrid, kInputGain, kOutputGain, kGlobalDryWet};
    KnobControl* knobs[] = {syncModeKnob, timeDivisionKnob, feedbackKnob, gridKnob, inputGainKnob, outputGainKnob, globalDryWetKnob};

    for (int i = 0; i < 7; i++) {
        if (knobs[i]) {
            int paramId = knobTags[i];

            // Special handling for time/division knob
            if (i == 1 && knobs[i]->getIsTimeDivisionKnob()) {
                float syncMode = controller->getParamNormalized(kTempoSyncMode);
                if (syncMode > 0.5f) {
                    paramId = kSyncDivision;
                } else {
                    paramId = kDelayTime;
                }
            }

            float paramValue = controller->getParamNormalized(paramId);
            knobs[i]->setValue(paramValue);
            knobs[i]->invalid();
        }
    }

    // Force visual updates
    updateValueReadouts();
    updateMinimapState();

}


//========================================================================
// BypassToggle Implementation
//========================================================================

BypassToggle::BypassToggle(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag)
: VSTGUI::CControl(size, listener, tag)
{
    setMax(1.0);  // Binary on/off toggle
    setMin(0.0);
}

void BypassToggle::draw(VSTGUI::CDrawContext* context)
{
    const VSTGUI::CRect& rect = getViewSize();
    bool isBypassed = (getValue() > 0.5);

    // Match mode button approach: use fixed 53px circle with precise centering
    const double buttonSize = 53.0;
    VSTGUI::CPoint viewCenter = rect.getCenter();
    const double halfButtonSize = buttonSize / 2.0;

    VSTGUI::CRect buttonRect(
        viewCenter.x - halfButtonSize,
        viewCenter.y - halfButtonSize,
        viewCenter.x + halfButtonSize,
        viewCenter.y + halfButtonSize
    );

    // Set stroke properties (same as mode buttons)
    context->setLineWidth(5.0);
    context->setDrawMode(VSTGUI::kAntiAliasing);
    context->setFrameColor(VSTGUI::CColor(35, 31, 32, 255)); // #231f20 from SVG
    context->setFillColor(VSTGUI::CColor(35, 31, 32, 255));

    // Apply stroke inset compensation (exactly like mode buttons)
    VSTGUI::CRect drawRect = buttonRect;
    const double strokeInset = 2.5; // Half of 5px stroke
    drawRect.inset(strokeInset, strokeInset);

    // Draw outer circle using stroke-compensated rect
    context->drawEllipse(drawRect, VSTGUI::kDrawStroked);

    // Draw inner circle using mode button centering technique
    if (isBypassed) {
        // Use stroke-compensated center for perfect alignment
        VSTGUI::CPoint center = drawRect.getCenter();
        // Calculate inner radius with another 10% reduction
        // Previous: 19.35px, reduced by 10%: 19.35 * 0.9 = 17.415px
        const double innerRadius = 17.415;

        VSTGUI::CRect innerRect(
            center.x - innerRadius,
            center.y - innerRadius,
            center.x + innerRadius,
            center.y + innerRadius
        );
        context->drawEllipse(innerRect, VSTGUI::kDrawFilled);
    }

    setDirty(false);
}

VSTGUI::CMouseEventResult BypassToggle::onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (buttons & VSTGUI::kLButton) {
        // Toggle the value
        setValue(getValue() > 0.5 ? 0.0 : 1.0);
        invalid();

        // Notify listener
        if (listener) {
            listener->valueChanged(this);

            // Force immediate value display update for bypass toggle
            auto editor = dynamic_cast<WaterStickEditor*>(listener);
            if (editor) {
                editor->updateBypassValueDisplay();
            }
        }

        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

void WaterStickEditor::updateBypassValueDisplay()
{
    auto controller = getController();
    if (delayBypassValue && controller) {
        float bypassValue = controller->getParamNormalized(kDelayBypass);
        std::string valueText = formatParameterValue(kDelayBypass, bypassValue);
        delayBypassValue->setText(valueText.c_str());
        delayBypassValue->invalid();
    }
}

void WaterStickEditor::applyEqualMarginLayout(VSTGUI::CViewContainer* container)
{
    // Calculate the true content bounding box including ALL visual elements
    VSTGUI::CRect contentBounds;
    bool firstElement = true;

    // Helper lambda to expand bounds to include a view
    auto expandBounds = [&](VSTGUI::CView* view) {
        if (view) {
            VSTGUI::CRect viewRect = view->getViewSize();
            if (firstElement) {
                contentBounds = viewRect;
                firstElement = false;
            } else {
                contentBounds.unite(viewRect);
            }
        }
    };

    // Include all minimap buttons (topmost elements)
    for (int i = 0; i < 16; i++) {
        expandBounds(minimapButtons[i]);
    }

    // Include all tap buttons
    for (int i = 0; i < 16; i++) {
        expandBounds(tapButtons[i]);
    }

    // Include all mode buttons
    for (int i = 0; i < 8; i++) {
        expandBounds(modeButtons[i]);
    }

    // Include all mode button labels
    for (int i = 0; i < 8; i++) {
        expandBounds(modeButtonLabels[i]);
    }

    // Include global controls
    expandBounds(delayBypassToggle);
    expandBounds(syncModeKnob);
    expandBounds(timeDivisionKnob);
    expandBounds(feedbackKnob);
    expandBounds(gridKnob);
    expandBounds(inputGainKnob);
    expandBounds(outputGainKnob);
    expandBounds(globalDryWetKnob);

    // Include global control labels
    expandBounds(delayBypassLabel);
    expandBounds(syncModeLabel);
    expandBounds(timeDivisionLabel);
    expandBounds(feedbackLabel);
    expandBounds(gridLabel);
    expandBounds(inputGainLabel);
    expandBounds(outputGainLabel);
    expandBounds(globalDryWetLabel);

    // Include global control value labels (bottommost elements)
    expandBounds(delayBypassValue);
    expandBounds(syncModeValue);
    expandBounds(timeDivisionValue);
    expandBounds(feedbackValue);
    expandBounds(gridValue);
    expandBounds(inputGainValue);
    expandBounds(outputGainValue);
    expandBounds(globalDryWetValue);

    // Calculate available space and desired margins
    const int windowWidth = kEditorWidth;
    const int windowHeight = kEditorHeight;
    const int contentWidth = static_cast<int>(contentBounds.getWidth());
    const int contentHeight = static_cast<int>(contentBounds.getHeight());

    // Calculate equal margins
    const int horizontalMargin = (windowWidth - contentWidth) / 2;
    const int verticalMargin = (windowHeight - contentHeight) / 2;

    // Calculate the offset needed to center content with equal margins
    const int currentContentLeft = static_cast<int>(contentBounds.left);
    const int currentContentTop = static_cast<int>(contentBounds.top);
    const int desiredContentLeft = horizontalMargin;
    const int desiredContentTop = verticalMargin;

    const int xOffset = desiredContentLeft - currentContentLeft;
    const int yOffset = desiredContentTop - currentContentTop;

    // Helper lambda to move a view by the calculated offset
    auto moveView = [&](VSTGUI::CView* view) {
        if (view) {
            VSTGUI::CRect currentRect = view->getViewSize();
            currentRect.offset(xOffset, yOffset);
            view->setViewSize(currentRect);
            view->setMouseableArea(currentRect);
        }
    };

    // Apply offset to all elements

    // Move minimap buttons
    for (int i = 0; i < 16; i++) {
        moveView(minimapButtons[i]);
    }

    // Move tap buttons
    for (int i = 0; i < 16; i++) {
        moveView(tapButtons[i]);
    }

    // Move mode buttons
    for (int i = 0; i < 8; i++) {
        moveView(modeButtons[i]);
    }

    // Move mode button labels
    for (int i = 0; i < 8; i++) {
        moveView(modeButtonLabels[i]);
    }

    // Move global controls
    moveView(delayBypassToggle);
    moveView(syncModeKnob);
    moveView(timeDivisionKnob);
    moveView(feedbackKnob);
    moveView(gridKnob);
    moveView(inputGainKnob);
    moveView(outputGainKnob);
    moveView(globalDryWetKnob);

    // Move global control labels
    moveView(delayBypassLabel);
    moveView(syncModeLabel);
    moveView(timeDivisionLabel);
    moveView(feedbackLabel);
    moveView(gridLabel);
    moveView(inputGainLabel);
    moveView(outputGainLabel);
    moveView(globalDryWetLabel);

    // Move global control value labels
    moveView(delayBypassValue);
    moveView(syncModeValue);
    moveView(timeDivisionValue);
    moveView(feedbackValue);
    moveView(gridValue);
    moveView(inputGainValue);
    moveView(outputGainValue);
    moveView(globalDryWetValue);

    // Move Smart Hierarchy controls
    for (int i = 0; i < 8; i++) {
        moveView(macroKnobs[i]);
        moveView(randomizeButtons[i]);
        moveView(resetButtons[i]);
    }

    // Include Smart Hierarchy controls in layout calculation
    for (int i = 0; i < 8; i++) {
        expandBounds(macroKnobs[i]);
        expandBounds(randomizeButtons[i]);
        expandBounds(resetButtons[i]);
    }

    // Force invalidation of all moved views
    container->invalid();
}

void WaterStickEditor::createSmartHierarchy(VSTGUI::CViewContainer* container)
{
    // TRIANGULAR LAYOUT: Smart Hierarchy positioning configuration
    // Each column has triangular arrangement: Macro knob at top, R/X buttons at bottom corners
    const int buttonSize = 53;           // Match tap button size for consistent spacing
    const int buttonSpacing = buttonSize / 2;  // Half diameter spacing
    const int delayMargin = 30;          // Match existing layout
    const int gridLeft = delayMargin;    // Align to existing grid
    const int gridHeight = 2;           // 2 rows for tap buttons

    // Calculate positioning relative to existing layout system with improved centering
    const int upperTwoThirdsHeight = (kEditorHeight * 2) / 3;
    const int totalGridHeight = (gridHeight * buttonSize) + ((gridHeight - 1) * buttonSpacing);
    const int tapGridTop = ((upperTwoThirdsHeight - totalGridHeight) / 2) - 15; // Improved centering

    // Calculate the space between tap grid and mode buttons (INCREASED SPACING)
    const int tapGridBottom = tapGridTop + (gridHeight * buttonSize) + buttonSpacing;
    const int modeButtonY = tapGridTop + (gridHeight * buttonSize) + buttonSpacing + (buttonSpacing * 3.0);  // Increased from 2.4 to 3.0
    const int availableSpace = static_cast<int>(modeButtonY - tapGridBottom);  // Now 78px (was 62px)

    // Triangular layout design specifications
    const int triangleHeight = 50;       // Total height of each triangle
    const int triangleBaseWidth = 40;    // Width of triangle base
    const int macroKnobSize = 24;       // Exact specification
    const int actionButtonSize = 14;    // Exact specification for R/× buttons

    // Position triangles in center of available space
    const int triangleStartY = tapGridBottom + ((availableSpace - triangleHeight) / 2);

    // Y-coordinates for triangular layout
    const int macroKnobY = triangleStartY;  // Macro knob at triangle top
    const int actionButtonsY = triangleStartY + triangleHeight - actionButtonSize;  // R/X buttons at triangle bottom

    // Create 8 columns of triangular Smart Hierarchy controls
    for (int i = 0; i < 8; i++) {
        // Calculate base X position for this column (aligned with tap button columns)
        int columnX = gridLeft + i * (buttonSize + buttonSpacing);

        // TRIANGULAR POSITIONING:
        // - Macro knob at top center of triangle
        int macroKnobX = columnX + (buttonSize - macroKnobSize) / 2;

        // - R button at bottom left corner of triangle
        int triangleLeftOffset = (buttonSize - triangleBaseWidth) / 2;
        int rButtonX = columnX + triangleLeftOffset;

        // - X button at bottom right corner of triangle
        int xButtonX = columnX + triangleLeftOffset + triangleBaseWidth - actionButtonSize;

        // Create Macro Knob (Triangle Top)
        VSTGUI::CRect macroRect(macroKnobX, macroKnobY, macroKnobX + macroKnobSize, macroKnobY + macroKnobSize);
        macroKnobs[i] = new MacroKnobControl(macroRect, this, kMacroKnob1 + i);

        // Assign specific context to each macro knob for control isolation
        TapContext assignedContext = static_cast<TapContext>(i);
        macroKnobs[i]->setAssignedContext(assignedContext);

        container->addView(macroKnobs[i]);

        // Create Randomize Button (Triangle Bottom Left)
        VSTGUI::CRect randomizeRect(rButtonX, actionButtonsY, rButtonX + actionButtonSize, actionButtonsY + actionButtonSize);
        randomizeButtons[i] = new ActionButton(randomizeRect, this, -1, ActionButton::Randomize, i);
        container->addView(randomizeButtons[i]);

        // Create Reset Button (Triangle Bottom Right)
        VSTGUI::CRect resetRect(xButtonX, actionButtonsY, xButtonX + actionButtonSize, actionButtonsY + actionButtonSize);
        resetButtons[i] = new ActionButton(resetRect, this, -1, ActionButton::Reset, i);
        container->addView(resetButtons[i]);
    }
}

//========================================================================
// Smart Hierarchy Helper Methods
//========================================================================

void WaterStickEditor::handleMacroKnobChange(int columnIndex, float value)
{
    if (columnIndex < 0 || columnIndex >= 8) return;

    // Get the discrete position (0-7) from the normalized value
    MacroKnobControl* knob = macroKnobs[columnIndex];
    if (!knob) return;

    int discretePos = knob->getDiscretePosition();

    // CONTEXT ISOLATION: Use the knob's assigned context instead of current active context
    TapContext assignedCtx = knob->getAssignedContext();

    auto controller = getController();
    if (!controller) return;

    auto waterStickController = dynamic_cast<WaterStickController*>(controller);
    if (!waterStickController) return;

    // DIAGNOSTIC: Log macro knob changes with assigned context
    printf("[MacroKnob] handleMacroKnobChange - columnIndex: %d, value: %.3f, discretePos: %d, assignedContext: %d\n",
           columnIndex, value, discretePos, static_cast<int>(assignedCtx));
    printf("[MacroKnob] Applying context-specific macro curve to assigned context only\n");

    // Apply macro curve to the knob's assigned context only (context isolation)
    handleGlobalMacroKnobChange(discretePos, assignedCtx);

    // Update the corresponding VST macro knob parameter to trigger DAW automation
    int macroParamId = kMacroKnob1 + columnIndex;
    controller->setParamNormalized(macroParamId, value);
    controller->performEdit(macroParamId, value);

    printf("[MacroKnob] Updated VST macro parameter %d with value %.3f\n", macroParamId, value);
}

void WaterStickEditor::handleGlobalMacroKnobChange(int discretePosition, TapContext currentCtx)
{
    auto controller = getController();
    if (!controller) return;

    auto waterStickController = dynamic_cast<WaterStickController*>(controller);
    if (!waterStickController) return;

    // Apply Rainmaker-style global macro curve using the MacroCurveSystem
    auto& macroCurveSystem = waterStickController->getMacroCurveSystem();
    macroCurveSystem.applyGlobalMacroCurve(discretePosition, static_cast<int>(currentCtx), waterStickController);

    // Update all tap button visuals to reflect the curve application
    for (int tapIndex = 0; tapIndex < 16; tapIndex++) {
        if (tapButtons[tapIndex]) {
            auto tapButton = static_cast<TapButton*>(tapButtons[tapIndex]);

            // Get the curve value for this tap
            float curveValue = macroCurveSystem.getGlobalCurveValueForTap(discretePosition, tapIndex);

            // Apply context-specific value adjustments for display
            switch (currentCtx) {
                case TapContext::FilterType:
                    // For filter type, quantize to valid discrete values (0-4)
                    curveValue = std::floor(curveValue * 4.999f) / 4.0f;
                    break;
                case TapContext::PitchShift:
                    // For pitch shift, map to bipolar range (-1.0 to +1.0)
                    curveValue = (curveValue * 2.0f) - 1.0f;
                    break;
                default:
                    // Other contexts use the curve value directly
                    break;
            }

            // Update the tap button's context value and display
            tapButton->setContextValue(currentCtx, curveValue);
            if (tapButton->getContext() == currentCtx) {
                tapButton->setValue(curveValue);
                tapButton->invalid();
            }
        }
    }

    // Update all macro knobs to show the global curve has been applied
    // Set non-global knobs to neutral position to indicate global mode
    for (int i = 1; i < 8; i++) {
        if (macroKnobs[i]) {
            macroKnobs[i]->setValue(0.5f); // Neutral position
            macroKnobs[i]->invalid();
        }
    }
}

void WaterStickEditor::handleRandomizeAction(int columnIndex)
{
    // CONTEXT ISOLATION: Use column-assigned context instead of current active context
    if (columnIndex < 0 || columnIndex >= 8) return;
    TapContext assignedCtx = static_cast<TapContext>(columnIndex);

    float totalRandomValue = 0.0f;

    // Randomize all 16 taps
    for (int tapIndex = 0; tapIndex < 16; ++tapIndex) {
        float randomValue = generateRandomValue();
        totalRandomValue += randomValue;

        if (tapButtons[tapIndex]) {
            auto tapButton = static_cast<TapButton*>(tapButtons[tapIndex]);
            tapButton->setContextValue(assignedCtx, randomValue);
            if (tapButton->getContext() == assignedCtx) {
                tapButton->setValue(randomValue);
                tapButton->invalid();
            }

            int paramId = getTapParameterIdForContext(tapIndex, assignedCtx);
            if (paramId >= 0) {
                auto controller = getController();
                if (controller) {
                    controller->setParamNormalized(paramId, randomValue);
                    controller->performEdit(paramId, randomValue);
                }
            }
        }
    }

    // Update all macro knobs to reflect the global average
    float globalAverageValue = totalRandomValue / 16.0f;
    for (int i = 0; i < 8; ++i) {
        if (macroKnobs[i]) {
            macroKnobs[i]->setValue(globalAverageValue);
            macroKnobs[i]->invalid();
        }
    }
}

void WaterStickEditor::handleResetAction(int columnIndex)
{
    // CONTEXT ISOLATION: Use column-assigned context instead of current active context
    if (columnIndex < 0 || columnIndex >= 8) return;
    TapContext assignedCtx = static_cast<TapContext>(columnIndex);
    float defaultValue = getContextDefaultValue(assignedCtx);

    // Reset all 16 taps
    for (int tapIndex = 0; tapIndex < 16; ++tapIndex) {
        if (tapButtons[tapIndex]) {
            auto tapButton = static_cast<TapButton*>(tapButtons[tapIndex]);
            tapButton->setContextValue(assignedCtx, defaultValue);
            if (tapButton->getContext() == assignedCtx) {
                tapButton->setValue(defaultValue);
                tapButton->invalid();
            }

            int paramId = getTapParameterIdForContext(tapIndex, assignedCtx);
            if (paramId >= 0) {
                auto controller = getController();
                if (controller) {
                    controller->setParamNormalized(paramId, defaultValue);
                    controller->performEdit(paramId, defaultValue);
                }
            }
        }
    }

    // Update all macro knobs to reflect the global default value
    for (int i = 0; i < 8; ++i) {
        if (macroKnobs[i]) {
            macroKnobs[i]->setValue(defaultValue);
            macroKnobs[i]->invalid();
        }
    }
}

float WaterStickEditor::generateRandomValue()
{
    // Generate random value between 0.0 and 1.0
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

float WaterStickEditor::getContextDefaultValue(TapContext context)
{
    // Return appropriate default values for each context
    switch (context) {
        case TapContext::Enable:        return 0.0f;   // Disabled by default
        case TapContext::Volume:        return 0.8f;   // 80% volume
        case TapContext::Pan:           return 0.5f;   // Center pan
        case TapContext::FilterCutoff:  return 1.0f;   // Full cutoff (no filtering)
        case TapContext::FilterResonance: return 0.0f; // No resonance
        case TapContext::FilterType:    return 0.0f;   // Bypass filter
        case TapContext::PitchShift:    return 0.5f;   // No pitch shift (center)
        case TapContext::FeedbackSend:  return 0.0f;   // No feedback send
        default:                        return 0.5f;   // Safe middle value
    }
}

//========================================================================
// MacroKnobControl Implementation
//========================================================================

MacroKnobControl::MacroKnobControl(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag)
: VSTGUI::CControl(size, listener, tag)
{
    setMax(1.0);
    setMin(0.0);
    setValue(0.0);  // Default to first position
}

void MacroKnobControl::draw(VSTGUI::CDrawContext* context)
{
    const VSTGUI::CRect& rect = getViewSize();

    // Get discrete position (0-7)
    int discretePos = getDiscretePosition();

    // Visual feedback now working - dot rotates through 8 positions

    // FIXED: Visual feedback now working properly

    // Calculate rotation angle for 8 positions
    // Positions are evenly distributed around a circle (0-7 = 8 positions)
    // Start at top (-90°) and go clockwise
    const float startAngle = -90.0f; // Start at top
    const float angleRange = 315.0f; // 7/8 of full rotation (315° instead of 360°)
    float angle = startAngle + (discretePos * angleRange / 7.0f);

    // Calculate angle for current discrete position

    // Convert to radians
    float angleRad = angle * M_PI / 180.0f;

    // Colors matching hardware aesthetic
    VSTGUI::CColor knobColor(35, 31, 32, 255);    // Dark knob body

    VSTGUI::CColor dotColor(255, 255, 255, 255);  // White position dot

    context->setDrawMode(VSTGUI::kAntiAliasing);

    // Draw knob body (filled circle)
    context->setFillColor(knobColor);
    context->setFrameColor(knobColor);
    context->setLineWidth(1.0);

    VSTGUI::CRect knobRect = rect;
    knobRect.inset(1.0, 1.0);  // Slight inset for clean edges
    context->drawEllipse(knobRect, VSTGUI::kDrawFilled);

    // Draw position indicator dot using improved positioning calculation
    VSTGUI::CPoint center = rect.getCenter();
    float outerRadius = (rect.getWidth() / 2.0f) - 1.0f; // Account for inset
    const float dotRadius = 2.5f; // Appropriate size for visibility

    // Calculate the distance from center to dot center
    // Position dot closer to edge for better visibility
    float dotCenterDistance = outerRadius - dotRadius - 1.0f;

    // Calculate dot position using trigonometry
    VSTGUI::CPoint dotCenter(
        center.x + dotCenterDistance * cos(angleRad),
        center.y + dotCenterDistance * sin(angleRad)
    );

    // Draw the dot as a filled circle
    VSTGUI::CRect dotRect(
        dotCenter.x - dotRadius,
        dotCenter.y - dotRadius,
        dotCenter.x + dotRadius,
        dotCenter.y + dotRadius
    );

    // Position dot at calculated angle

    // 8 discrete positions: 0°(-90°), 1°(-45°), 2°(0°), 3°(45°), 4°(90°), 5°(135°), 6°(180°), 7°(225°)

    context->setFillColor(dotColor);
    context->drawEllipse(dotRect, VSTGUI::kDrawFilled);

    // REMOVED: Let VSTGUI manage dirty state automatically instead of forcing setDirty(false)
    // setDirty(false);
}

void MacroKnobControl::setValue(float value)
{
    // CRITICAL FIX: Quantize INPUT value directly, not current value (fixes circular logic bug)
    int discretePos = static_cast<int>(value * 7.0f + 0.5f); // Round to nearest position
    discretePos = std::max(0, std::min(7, discretePos));     // Clamp to valid range (0-7)
    float quantizedValue = static_cast<float>(discretePos) / 7.0f; // Convert back to normalized

    // Quantize to 8 discrete positions (0-7)

    VSTGUI::CControl::setValue(quantizedValue);

    // STANDARDIZED: Use setDirty(true) consistently instead of invalid()
    setDirty(true);
}

float MacroKnobControl::getDiscreteValue() const
{
    // Convert current value to discrete position and back to normalized
    int pos = getDiscretePosition();
    return static_cast<float>(pos) / 7.0f;
}

int MacroKnobControl::getDiscretePosition() const
{
    // Map normalized value (0.0-1.0) to discrete position (0-7)
    float normalizedValue = getValue();
    int pos = static_cast<int>(normalizedValue * 7.0f + 0.5f); // Round to nearest
    return std::max(0, std::min(7, pos)); // Clamp to valid range
}

VSTGUI::CMouseEventResult MacroKnobControl::onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (buttons & VSTGUI::kLButton) {
        auto currentTime = std::chrono::steady_clock::now();

        // DIAGNOSTIC: Log mouse down events
        printf("[MacroKnob] Mouse down - tag: %d, pos: (%.1f, %.1f), currentValue: %.3f\n",
               getTag(), where.x, where.y, getValue());

        // Check for double-click to reset to default
        if (isDoubleClick(currentTime)) {
            printf("[MacroKnob] Double-click detected - resetting to default\n");
            resetToDefaultValue();
            if (listener) {
                listener->valueChanged(this);
            }
            return VSTGUI::kMouseEventHandled;
        }

        lastClickTime = currentTime;
        isDragging = true;
        lastMousePos = where;
        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

VSTGUI::CMouseEventResult MacroKnobControl::onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (isDragging && (buttons & VSTGUI::kLButton)) {
        // Calculate vertical drag distance
        float deltaY = lastMousePos.y - where.y;
        float sensitivity = 0.05f; // INCREASED: Higher sensitivity for discrete positions

        // Update value based on drag
        float currentValue = getValue();
        float newValue = currentValue + (deltaY * sensitivity);
        newValue = std::max(0.0f, std::min(1.0f, newValue)); // Clamp to 0-1

        // Update value with improved sensitivity
        setValue(newValue);

        setDirty(true); // STANDARDIZED: Use setDirty(true) consistently

        // Ensure visual update with comprehensive invalidation
        setDirty(true);
        invalid();
        if (getParentView()) {
            getParentView()->invalid();
        }

        if (listener) {
            listener->valueChanged(this);
        }

        lastMousePos = where;
        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

VSTGUI::CMouseEventResult MacroKnobControl::onMouseUp(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (isDragging) {
        isDragging = false;
        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

bool MacroKnobControl::isDoubleClick(const std::chrono::steady_clock::time_point& currentTime)
{
    auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastClickTime);
    return timeDiff <= DOUBLE_CLICK_TIMEOUT;
}

void MacroKnobControl::resetToDefaultValue()
{
    setValue(0.0f); // Reset to first position
    setDirty(true); // STANDARDIZED: Use setDirty(true) consistently
}

//========================================================================
// ActionButton Implementation
//========================================================================

ActionButton::ActionButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag, ActionType type, int columnIndex)
: VSTGUI::CControl(size, listener, tag), actionType(type), columnIndex(columnIndex), isPressed(false)
{
    setMax(1.0);
    setMin(0.0);
}

void ActionButton::draw(VSTGUI::CDrawContext* context)
{
    const VSTGUI::CRect& rect = getViewSize();

    // Colors matching hardware aesthetic
    VSTGUI::CColor buttonColor(35, 31, 32, 255);   // Dark button background
    VSTGUI::CColor textColor(255, 255, 255, 255);  // White text
    VSTGUI::CColor hoverColor(60, 56, 57, 255);    // Slightly lighter when pressed

    context->setDrawMode(VSTGUI::kAntiAliasing);

    // Draw button background (small square with rounded corners)
    VSTGUI::CColor bgColor = isPressed ? hoverColor : buttonColor;
    context->setFillColor(bgColor);
    context->setFrameColor(buttonColor);
    context->setLineWidth(1.0);

    // Draw rectangle (VSTGUI doesn't have rounded rectangle methods)
    context->drawRect(rect, VSTGUI::kDrawStroked);
    context->drawRect(rect, VSTGUI::kDrawFilled);

    // Draw symbol/text centered in button
    context->setFontColor(textColor);

    // Use a small, bold font
    auto editor = static_cast<WaterStickEditor*>(listener);
    if (editor) {
        auto font = editor->getWorkSansFont(10.0f);
        if (font) {
            context->setFont(font);
        }
    } else {
        context->setFont(VSTGUI::kNormalFontSmall);
    }

    // Draw appropriate symbol
    const char* symbol = (actionType == Randomize) ? "R" : "×";

    // Center the text
    context->drawString(symbol, rect, VSTGUI::kCenterText, true);

    setDirty(false);
}

VSTGUI::CMouseEventResult ActionButton::onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (buttons & VSTGUI::kLButton) {
        // Brief visual feedback using internal state
        isPressed = true;
        invalid();

        // Trigger action via editor
        auto editor = static_cast<WaterStickEditor*>(listener);
        if (editor) {
            if (actionType == Randomize) {
                editor->handleRandomizeAction(columnIndex);
            } else {
                editor->handleResetAction(columnIndex);
            }
        }

        // Reset visual state after brief delay
        isPressed = false;
        invalid();

        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}


} // namespace WaterStick