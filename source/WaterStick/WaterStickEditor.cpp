#include "WaterStickEditor.h"
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

    createTapButtons(container);
    createModeButtons(container);
    createGlobalControls(container);
    createMinimap(container);

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

    // Position grid in left section (2/3 of window width) with margin
    const int upperTwoThirdsHeight = (kEditorHeight * 2) / 3;
    const int delayMargin = 30;
    const int gridLeft = delayMargin;  // Align to left margin within delay section
    const int gridTop = (upperTwoThirdsHeight - totalGridHeight) / 2;

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

    // Match the tap button positioning
    const int upperTwoThirdsHeight = (kEditorHeight * 2) / 3;
    const int delayMargin = 30;
    const int gridLeft = delayMargin;  // Match repositioned tap buttons
    const int tapGridTop = (upperTwoThirdsHeight - totalGridHeight) / 2;

    // Calculate mode button position
    // Place 1.5x button spacing below the tap button grid
    const int modeButtonY = tapGridTop + (gridHeight * buttonSize) + buttonSpacing + (buttonSpacing * 1.5);

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

    // Add labels below mode buttons
    const char* modeLabels[] = {"Mutes", "Level", "Pan", "Cutoff", "Res", "Type", "X", "X"};
    const int labelHeight = 20; // Match global control labels
    const int labelY = modeButtonY + buttonSize + 15; // Increased gap to clear selection rectangle

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
    const int bottomThirdTop = (kEditorHeight * 2) / 3;
    const int knobSize = 53;
    const int buttonSize = 53;
    const int buttonSpacing = buttonSize / 2;
    const int gridWidth = 8;
    const int delayMargin = 30;
    const int tapGridLeft = delayMargin;
    const int modeButtonSpacing = static_cast<int>(buttonSpacing * 1.5);
    const int knobY = bottomThirdTop + modeButtonSpacing;

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
    std::string contextNames[] = {"Enable", "Volume", "Pan", "FilterCutoff", "FilterResonance", "FilterType"};
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


    // Load new context values from VST parameters
    if (controller) {
        for (int i = 0; i < 16; i++) {
            auto tapButton = static_cast<TapButton*>(tapButtons[i]);
            if (tapButton) {
                // Set the context for the button
                tapButton->setContext(newContext);

                // Get the parameter ID for the new context
                int newParamId = getTapParameterIdForContext(i, newContext);

                // Load the parameter value
                float paramValue = controller->getParamNormalized(newParamId);


                // Set the button's value and internal storage
                tapButton->setValue(paramValue);
                tapButton->setContextValue(newContext, paramValue);

                // Trigger visual update
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
        default:
            // Calculate base parameter offset for tap control (each tap has 3 params: Enable, Level, Pan)
            int baseOffset = (tapNumber - 1) * 3;
            return kTap1Enable + baseOffset;  // Default to Enable
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
            currentContext == TapContext::FilterType) {
            // Volume, Pan, Filter Cutoff, Filter Resonance, and Filter Type contexts: Handle continuous control
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
                        currentContext == TapContext::FilterType) {
                        // Volume, Filter Cutoff, and Filter Type: bottom = 0.0, top = 1.0
                        double relativeY = (targetDrawRect.bottom - localPoint.y) / targetDrawRect.getHeight();
                        newValue = std::max(0.0, std::min(1.0, relativeY));
                    } else { // TapContext::Pan or TapContext::FilterResonance
                        // Pan and Filter Resonance: bottom = 0.0 (left), center = 0.5, top = 1.0 (right)
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
                currentContext == TapContext::FilterType) {
                // Volume, Filter Cutoff, and Filter Type: bottom = 0.0, top = 1.0
                double relativeY = (drawRect.bottom - where.y) / drawRect.getHeight();
                newValue = std::max(0.0, std::min(1.0, relativeY));
            } else { // TapContext::Pan or TapContext::FilterResonance
                // Pan and Filter Resonance: bottom = 0.0 (left), center = 0.5, top = 1.0 (right)
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
    const int gridTop = (upperTwoThirdsHeight - totalGridHeight) / 2;

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


} // namespace WaterStick