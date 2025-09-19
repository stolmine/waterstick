#include "WaterStickEditor.h"
#include "WaterStickParameters.h"
#include "WaterStickLogger.h"
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

    // Initialize tap button array
    for (int i = 0; i < 16; i++) {
        tapButtons[i] = nullptr;
    }

    // Initialize mode buttons
    for (int i = 0; i < 8; i++) {
        modeButtons[i] = nullptr;
    }

    // Initialize bypass toggles
    delayBypassToggle = nullptr;
    combBypassToggle = nullptr;

    // Initialize minimap
    minimapContainer = nullptr;
    for (int i = 0; i < 16; i++) {
        minimapButtons[i] = nullptr;
    }

    // Initialize global controls
    syncModeKnob = nullptr;
    timeDivisionKnob = nullptr;
    feedbackKnob = nullptr;
    gridKnob = nullptr;
    inputGainKnob = nullptr;
    outputGainKnob = nullptr;
    dryWetKnob = nullptr;

    // Initialize comb controls
    combSizeKnob = nullptr;
    combFeedbackKnob = nullptr;
    combPitchKnob = nullptr;
    combTapsKnob = nullptr;
    combSyncKnob = nullptr;
    combDivisionKnob = nullptr;
    routeModeKnob = nullptr;

    // Initialize labels
    syncModeLabel = nullptr;
    timeDivisionLabel = nullptr;
    feedbackLabel = nullptr;
    gridLabel = nullptr;
    inputGainLabel = nullptr;
    outputGainLabel = nullptr;
    dryWetLabel = nullptr;

    // Initialize comb labels
    combSizeLabel = nullptr;
    combFeedbackLabel = nullptr;
    combPitchLabel = nullptr;
    combTapsLabel = nullptr;
    combSyncLabel = nullptr;
    combDivisionLabel = nullptr;
    routeModeLabel = nullptr;

    // Initialize value readouts
    syncModeValue = nullptr;
    timeDivisionValue = nullptr;
    feedbackValue = nullptr;
    gridValue = nullptr;
    inputGainValue = nullptr;
    outputGainValue = nullptr;
    dryWetValue = nullptr;

    // Initialize comb value readouts
    combSizeValue = nullptr;
    combFeedbackValue = nullptr;
    combPitchValue = nullptr;
    combTapsValue = nullptr;
    combSyncValue = nullptr;
    combDivisionValue = nullptr;
    routeModeValue = nullptr;

    // Initialize bypass labels
    delayBypassLabel = nullptr;
    combBypassLabel = nullptr;
}

bool PLUGIN_API WaterStickEditor::open(void* parent, const VSTGUI::PlatformType& platformType)
{
    VSTGUI::CRect frameSize(0, 0, kEditorWidth, kEditorHeight);

    frame = new VSTGUI::CFrame(frameSize, this);
    frame->open(parent, platformType);

    // Create main container with white background
    auto container = new VSTGUI::CViewContainer(frameSize);
    container->setBackgroundColor(VSTGUI::kWhiteCColor);


    // Create tap buttons
    createTapButtons(container);

    // Create mode buttons
    createModeButtons(container);

    // Create bypass controls
    createBypassControls(container);

    // Create global controls
    createGlobalControls(container);

    // Create comb controls
    createCombControls(container);

    // Create minimap
    createMinimap(container);

    frame->addView(container);

    // Force parameter synchronization for VST3 lifecycle compliance
    forceParameterSynchronization();

    // Update value readouts with initial parameter values
    updateValueReadouts();

    // Update minimap with initial tap enable states
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
    const int totalGridWidth = (gridWidth * buttonSize) + ((gridWidth - 1) * buttonSpacing);
    const int totalGridHeight = (gridHeight * buttonSize) + ((gridHeight - 1) * buttonSpacing);

    // Position grid in left section (2/3 of window width) with margin
    const int upperTwoThirdsHeight = (kEditorHeight * 2) / 3;
    const int delaySection = (kEditorWidth * 2) / 3;  // 667px for delay section
    const int delayMargin = 30;
    const int gridLeft = delayMargin;  // Align to left margin within delay section
    const int gridTop = (upperTwoThirdsHeight - totalGridHeight) / 2;

    // Create 16 tap buttons in 2x8 grid
    for (int i = 0; i < 16; i++) {
        // Calculate grid position: top row (0-7), bottom row (8-15)
        int row = i / 8;        // 0 for taps 1-8, 1 for taps 9-16
        int col = i % 8;        // 0-7 for column position

        // Calculate button position
        int x = gridLeft + col * (buttonSize + buttonSpacing);
        int y = gridTop + row * (buttonSize + buttonSpacing);

        VSTGUI::CRect buttonRect(x, y, x + buttonSize, y + buttonSize);

        // Create custom tap button
        auto button = new TapButton(buttonRect, this, kTap1Enable + (i * 3));

        // Initialize context state
        button->setContext(TapContext::Enable);

        // Load initial values from parameters for all contexts
        auto controller = getController();
        if (controller) {
            // Load Enable context value
            int enableParamId = getTapParameterIdForContext(i, TapContext::Enable);
            auto enableValue = controller->getParamNormalized(enableParamId);
            button->setContextValue(TapContext::Enable, enableValue);

            // Load Volume context value
            int volumeParamId = getTapParameterIdForContext(i, TapContext::Volume);
            auto volumeValue = controller->getParamNormalized(volumeParamId);
            button->setContextValue(TapContext::Volume, volumeValue);

            // Load Pan context value
            int panParamId = getTapParameterIdForContext(i, TapContext::Pan);
            auto panValue = controller->getParamNormalized(panParamId);
            button->setContextValue(TapContext::Pan, panValue);

            // Load Filter Cutoff context value
            int filterCutoffParamId = getTapParameterIdForContext(i, TapContext::FilterCutoff);
            auto filterCutoffValue = controller->getParamNormalized(filterCutoffParamId);
            button->setContextValue(TapContext::FilterCutoff, filterCutoffValue);

            // Load Filter Resonance context value
            int filterResonanceParamId = getTapParameterIdForContext(i, TapContext::FilterResonance);
            auto filterResonanceValue = controller->getParamNormalized(filterResonanceParamId);
            button->setContextValue(TapContext::FilterResonance, filterResonanceValue);

            // Load Filter Type context value
            int filterTypeParamId = getTapParameterIdForContext(i, TapContext::FilterType);
            auto filterTypeValue = controller->getParamNormalized(filterTypeParamId);
            button->setContextValue(TapContext::FilterType, filterTypeValue);

            // Set initial display value (Enable context is default)
            button->setValue(enableValue);
        }

        // Store reference for easy access
        tapButtons[i] = button;

        // Add to container
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
    const int totalGridWidth = (gridWidth * buttonSize) + ((gridWidth - 1) * buttonSpacing);
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
    // Calculate knob positioning in bottom 1/3 of window
    const int bottomThirdTop = (kEditorHeight * 2) / 3;

    // Knob configuration
    const int knobSize = 53; // Same size as tap buttons
    const int buttonSize = 53; // For tap button calculations
    const int buttonSpacing = buttonSize / 2;
    const int gridWidth = 8;
    const int totalGridWidth = (gridWidth * buttonSize) + ((gridWidth - 1) * buttonSpacing);

    // Calculate tap grid positioning to align knob edges with tap edges
    const int delayMargin = 30;
    const int tapGridLeft = delayMargin;  // Match repositioned tap buttons
    const int tapGridRight = tapGridLeft + totalGridWidth;

    // Distribute 7 knobs equally across the tap grid width
    // Align leftmost and rightmost knob edges with tap grid edges
    const int availableWidth = totalGridWidth - (7 * knobSize); // Space for 6 gaps between 7 knobs
    const int knobSpacing = availableWidth / 6; // Equal spacing between knobs

    // Position knobs with proper spacing from mode buttons
    const int modeButtonSpacing = static_cast<int>(buttonSpacing * 1.5);
    const int knobY = bottomThirdTop + modeButtonSpacing;

    // Label positioning - uniform sizing based on "OUTPUT" label
    const int labelHeight = 20;
    const int labelY = knobY + knobSize + 5; // 5px gap between knob and label
    const int valueReadoutY = labelY + labelHeight + 2; // 2px gap between label and value

    // Create knobs and labels
    const char* knobLabels[] = {"SYNC", "TIME", "FEEDBACK", "GRID", "INPUT", "OUTPUT", "DRY/WET"};
    const int knobTags[] = {kTempoSyncMode, kDelayTime, kFeedback, kGrid, kInputGain, kOutputGain, kDryWet};
    KnobControl** knobPointers[] = {&syncModeKnob, &timeDivisionKnob, &feedbackKnob, &gridKnob, &inputGainKnob, &outputGainKnob, &dryWetKnob};
    VSTGUI::CTextLabel** labelPointers[] = {&syncModeLabel, &timeDivisionLabel, &feedbackLabel, &gridLabel, &inputGainLabel, &outputGainLabel, &dryWetLabel};
    VSTGUI::CTextLabel** valuePointers[] = {&syncModeValue, &timeDivisionValue, &feedbackValue, &gridValue, &inputGainValue, &outputGainValue, &dryWetValue};

    for (int i = 0; i < 7; i++) {
        // Calculate knob position (equally distributed)
        int knobX = tapGridLeft + i * (knobSize + knobSpacing);

        // Create knob (no clipping - full circle visible)
        VSTGUI::CRect knobRect(knobX, knobY, knobX + knobSize, knobY + knobSize);
        *(knobPointers[i]) = new KnobControl(knobRect, this, knobTags[i]);

        // Special handling for time/division knob
        if (i == 1) {
            (*knobPointers[i])->setIsTimeDivisionKnob(true);
        }

        // Load initial value from controller
        auto controller = getController();
        if (controller) {
            float value = controller->getParamNormalized(knobTags[i]);
            (*knobPointers[i])->setValue(value);

            // Log global control initialization with descriptive names
            std::string paramName;
            switch (knobTags[i]) {
                case kInputGain: paramName = "InputGain"; break;
                case kOutputGain: paramName = "OutputGain"; break;
                case kTempoSyncMode: paramName = "TempoSyncMode"; break;
                case kDelayTime: paramName = "DelayTime"; break;
                case kFeedback: paramName = "Feedback"; break;
                case kGrid: paramName = "Grid"; break;
                case kDryWet: paramName = "DryWet"; break;
                default: paramName = "Unknown"; break;
            }
            WS_LOG_PARAM_CONTEXT("GLOBAL-LOAD", knobTags[i], paramName, value);
        }

        container->addView(*(knobPointers[i]));

        // Calculate dynamic label width based on text content
        auto customFont = getWorkSansFont(11.0f); // Get font first for width calculation
        int labelWidth = knobSize; // Default minimum width

        // Use a simple approximation for text width since we don't have drawing context yet
        // FEEDBACK = 8 chars, others are shorter
        const char* text = knobLabels[i];
        int textLength = static_cast<int>(strlen(text));

        // Approximate width: 7.5 pixels per character for 11pt font + 8px padding
        int approximateWidth = static_cast<int>(textLength * 7.5f + 8);
        labelWidth = std::max(knobSize, approximateWidth);

        // Center the label horizontally around the knob center
        int labelLeft = knobX + (knobSize - labelWidth) / 2;
        int labelRight = labelLeft + labelWidth;

        // Create label with dynamic width
        VSTGUI::CRect labelRect(labelLeft, labelY, labelRight, labelY + labelHeight);
        *(labelPointers[i]) = new VSTGUI::CTextLabel(labelRect, knobLabels[i]);

        // Set label styling - no background, uniform font size
        auto label = *(labelPointers[i]);
        label->setHoriAlign(VSTGUI::kCenterText);
        label->setFontColor(VSTGUI::kBlackCColor);
        label->setBackColor(VSTGUI::kTransparentCColor);
        label->setFrameColor(VSTGUI::kTransparentCColor); // Remove any border
        label->setStyle(VSTGUI::CTextLabel::kNoFrame); // Explicitly no frame

        // Apply the font we already measured with
        if (customFont) {
            label->setFont(customFont);
        }

        container->addView(label);

        // Create value readout
        VSTGUI::CRect valueRect(knobX, valueReadoutY, knobX + knobSize, valueReadoutY + labelHeight);
        *(valuePointers[i]) = new VSTGUI::CTextLabel(valueRect, "");

        // Set value readout styling
        auto valueLabel = *(valuePointers[i]);
        valueLabel->setHoriAlign(VSTGUI::kCenterText);
        valueLabel->setFontColor(VSTGUI::kBlackCColor);
        valueLabel->setBackColor(VSTGUI::kTransparentCColor);
        valueLabel->setFrameColor(VSTGUI::kTransparentCColor);
        valueLabel->setStyle(VSTGUI::CTextLabel::kNoFrame);

        // Use smaller font for value readouts
        auto valueFont = getWorkSansFont(9.0f);
        if (valueFont) {
            valueLabel->setFont(valueFont);
        }

        // Set initial value text
        if (controller) {
            float value = controller->getParamNormalized(knobTags[i]);
            std::string valueText = formatParameterValue(knobTags[i], value);
            valueLabel->setText(valueText.c_str());
        }

        container->addView(valueLabel);
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

        case kDryWet:
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

        case kCombSize:
            // Format as seconds (0.0001f to 2.0f range)
            oss << std::fixed << std::setprecision(3) << (normalizedValue * 2.0f) << "s";
            return oss.str();

        case kCombFeedback:
            // Format as percentage (0-99%)
            oss << std::fixed << std::setprecision(0) << (normalizedValue * 99.0f) << "%";
            return oss.str();

        case kCombPitchCV:
        {
            // Format as volts (-5.0 to +5.0V)
            float volts = -5.0f + (normalizedValue * 10.0f);
            oss << std::fixed << std::setprecision(2) << volts << "V";
            return oss.str();
        }

        case kCombTaps:
        {
            // Format as discrete count (1 to 64)
            int taps = 1 + static_cast<int>(normalizedValue * 63);
            oss << taps << " taps";
            return oss.str();
        }

        case kCombSync:
            // Format as sync mode
            return normalizedValue > 0.5f ? "SYNC" : "FREE";

        case kCombDivision:
        {
            // Format as sync division (reuse existing sync division logic)
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

        case kRouteMode:
        {
            // Format as routing mode
            int mode = static_cast<int>(normalizedValue * 2);
            const char* modes[] = {"D>C", "C>D", "D+C"};
            if (mode >= 0 && mode <= 2) {
                return modes[mode];
            }
            return "D>C";
        }

        case kDelayBypass:
        case kCombBypass:
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
    const int knobTags[] = {kTempoSyncMode, kDelayTime, kFeedback, kGrid, kInputGain, kOutputGain, kDryWet};
    VSTGUI::CTextLabel* valueLabels[] = {syncModeValue, timeDivisionValue, feedbackValue, gridValue, inputGainValue, outputGainValue, dryWetValue};

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

    // Update comb control value readouts
    const int combTags[] = {kCombSize, kCombFeedback, kCombPitchCV, kCombTaps, kCombSync, kCombDivision, kRouteMode};
    VSTGUI::CTextLabel* combValueLabels[] = {combSizeValue, combFeedbackValue, combPitchValue, combTapsValue, combSyncValue, combDivisionValue, routeModeValue};

    for (int i = 0; i < 7; i++) {
        if (combValueLabels[i]) {
            float value = controller->getParamNormalized(combTags[i]);
            std::string valueText = formatParameterValue(combTags[i], value);
            combValueLabels[i]->setText(valueText.c_str());
            combValueLabels[i]->invalid();
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

    WS_LOG_INFO("Context switch: " + oldContextName + " -> " + newContextName);

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

    // DEBUG: Log context switching
    if (newContext == TapContext::FilterType) {
        std::cout << "[WaterStick DEBUG] Switching to FilterType context (mode 6)" << std::endl;
    }

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

                // DEBUG: Log FilterType context switching for first 4 taps
                if (i < 4 && newContext == TapContext::FilterType) {
                    std::cout << "[WaterStick DEBUG] Context switch - Tap " << (i+1)
                              << " FilterType param " << newParamId << " value: "
                              << std::fixed << std::setprecision(3) << paramValue << std::endl;
                }

                // Set the button's value and internal storage
                tapButton->setValue(paramValue);
                tapButton->setContextValue(newContext, paramValue);

                // Trigger visual update
                tapButton->invalid();
            }
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

            // DEBUG: Log what letter is being displayed
            std::cout << "[WaterStick DEBUG] TapButton draw - Tag " << getTag()
                      << " FilterType value: " << std::fixed << std::setprecision(3) << currentValue
                      << " -> Letter: '" << letter << "'" << std::endl;

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

//========================================================================
// MinimapTapButton Implementation
//========================================================================

MinimapTapButton::MinimapTapButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag)
: VSTGUI::CControl(size, listener, tag)
{
}

void MinimapTapButton::draw(VSTGUI::CDrawContext* context)
{
    // Get the actual drawing rectangle
    VSTGUI::CRect rect = getViewSize();

    // Define circle parameters - scaled down from original 53px to ~13px
    const double circleSize = 13.0; // About 1/4 scale of original 53px
    VSTGUI::CPoint center = rect.getCenter();
    const double radius = circleSize / 2.0;

    // Create circle rect centered in the button area
    VSTGUI::CRect circleRect(
        center.x - radius,
        center.y - radius,
        center.x + radius,
        center.y + radius
    );

    // Determine if tap is enabled (getValue > 0.5 means enabled)
    bool isEnabled = (getValue() > 0.5);

    // Draw circle stroke (1px width for minimap)
    context->setLineWidth(1.0);
    context->setDrawMode(VSTGUI::kAntiAliasing);
    context->setFrameColor(VSTGUI::kBlackCColor);

    // Fill based on enabled state
    if (isEnabled) {
        context->setFillColor(VSTGUI::kBlackCColor);
        context->drawEllipse(circleRect, VSTGUI::kDrawFilledAndStroked);
    } else {
        context->setFillColor(VSTGUI::kWhiteCColor);
        context->drawEllipse(circleRect, VSTGUI::kDrawFilledAndStroked);
    }

    setDirty(false);
}

//========================================================================
// Minimap Implementation
//========================================================================

void WaterStickEditor::createMinimap(VSTGUI::CViewContainer* container)
{
    // Minimap dimensions - height = 1 tap button, width = 2 tap buttons
    const int minimapButtonSize = 13; // Scaled down from 53px (about 1/4 scale)
    const int minimapButtonSpacing = minimapButtonSize / 2; // Proportional spacing
    const int minimapGridWidth = 8;
    const int minimapGridHeight = 2;

    // Calculate minimap total dimensions
    const int minimapTotalWidth = (minimapGridWidth * minimapButtonSize) + ((minimapGridWidth - 1) * minimapButtonSpacing);
    const int minimapTotalHeight = (minimapGridHeight * minimapButtonSize) + ((minimapGridHeight - 1) * minimapButtonSpacing);

    // Target dimensions: height = 1 tap element (53px), width = 2 tap elements (53+26.5+53 = 132.5px)
    const int targetHeight = 53;
    const int targetWidth = 150; // Increased to prevent clipping

    // Calculate actual tap grid positioning (matches repositioned tap buttons)
    const int buttonSize = 53;
    const int buttonSpacing = buttonSize / 2;
    const int gridWidth = 8;
    const int gridHeight = 2;
    const int totalGridWidth = (gridWidth * buttonSize) + ((gridWidth - 1) * buttonSpacing);
    const int totalGridHeight = (gridHeight * buttonSize) + ((gridHeight - 1) * buttonSpacing);
    const int upperTwoThirdsHeight = (kEditorHeight * 2) / 3;
    const int delayMargin = 30;
    const int gridLeft = delayMargin;  // Match repositioned tap buttons
    const int gridTop = (upperTwoThirdsHeight - totalGridHeight) / 2;

    // Position in upper right area of delay section (left 2/3)
    const int delaySection = (kEditorWidth * 2) / 3;  // 667px delay section
    const int margin = 30;
    const int minimapTop = margin;
    const int minimapLeft = delaySection - margin - targetWidth;  // Position within delay section

    // Create minimap container
    VSTGUI::CRect minimapRect(minimapLeft, minimapTop, minimapLeft + targetWidth, minimapTop + targetHeight);
    minimapContainer = new VSTGUI::CViewContainer(minimapRect);
    minimapContainer->setBackgroundColor(VSTGUI::kTransparentCColor);

    // Center the minimap grid within the container
    const int gridStartX = (targetWidth - minimapTotalWidth) / 2;
    const int gridStartY = (targetHeight - minimapTotalHeight) / 2;

    // Create minimap buttons in 2x8 grid
    for (int i = 0; i < 16; i++) {
        int row = i / 8;        // 0 for taps 1-8, 1 for taps 9-16
        int col = i % 8;        // 0-7 for column position

        // Calculate button position within minimap container
        int x = gridStartX + col * (minimapButtonSize + minimapButtonSpacing);
        int y = gridStartY + row * (minimapButtonSize + minimapButtonSpacing);

        VSTGUI::CRect buttonRect(x, y, x + minimapButtonSize, y + minimapButtonSize);

        // Create minimap button (non-interactive, display only)
        minimapButtons[i] = new MinimapTapButton(buttonRect, nullptr, -1);

        // Initialize with current tap enable state
        // Use proper parameter ID mapping (each tap has 3 params: Enable, Level, Pan)
        Steinberg::Vst::ParamID paramId = kTap1Enable + (i * 3);
        Steinberg::Vst::ParamValue paramValue = controller->getParamNormalized(paramId);
        minimapButtons[i]->setValue(static_cast<float>(paramValue));

        minimapContainer->addView(minimapButtons[i]);
    }

    container->addView(minimapContainer);
}

void WaterStickEditor::updateMinimapState()
{
    if (!minimapContainer) return;

    // Update minimap buttons to reflect current tap enable states
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

    std::cout << "[WaterStick DEBUG] GUI::forceParameterSynchronization() called" << std::endl;

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

            // DEBUG: Log final displayed value for filter type context
            if (i < 4 && buttonContext == TapContext::FilterType) {
                std::cout << "[WaterStick DEBUG] Tap " << (i+1) << " FilterType final display value: "
                          << std::fixed << std::setprecision(3) << currentContextValue << std::endl;
            }
        }
    }

    // Sync all global knobs with current parameter values
    const int knobTags[] = {kTempoSyncMode, kDelayTime, kFeedback, kGrid, kInputGain, kOutputGain, kDryWet};
    KnobControl* knobs[] = {syncModeKnob, timeDivisionKnob, feedbackKnob, gridKnob, inputGainKnob, outputGainKnob, dryWetKnob};

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

    // Sync all comb knobs with current parameter values
    const int combTags[] = {kCombSize, kCombFeedback, kCombPitchCV, kCombTaps, kCombSync, kCombDivision, kRouteMode};
    KnobControl* combKnobs[] = {combSizeKnob, combFeedbackKnob, combPitchKnob, combTapsKnob, combSyncKnob, combDivisionKnob, routeModeKnob};

    for (int i = 0; i < 7; i++) {
        if (combKnobs[i]) {
            float paramValue = controller->getParamNormalized(combTags[i]);
            combKnobs[i]->setValue(paramValue);
            combKnobs[i]->invalid();
        }
    }

    // Sync bypass toggles with current parameter values
    if (delayBypassToggle) {
        float delayBypassValue = controller->getParamNormalized(kDelayBypass);
        delayBypassToggle->setValue(delayBypassValue);
        delayBypassToggle->invalid();
    }

    if (combBypassToggle) {
        float combBypassValue = controller->getParamNormalized(kCombBypass);
        combBypassToggle->setValue(combBypassValue);
        combBypassToggle->invalid();
    }

    // Force visual updates
    updateValueReadouts();
    updateMinimapState();

    std::cout << "[WaterStick DEBUG] GUI::forceParameterSynchronization() completed" << std::endl;
}

void WaterStickEditor::createCombControls(VSTGUI::CViewContainer* container)
{
    // Position comb controls in right 1/3 of window (333px width)
    const int combSectionLeft = (kEditorWidth * 2) / 3;  // Start at 667px
    const int combSectionWidth = kEditorWidth / 3;       // 333px width
    const int combMargin = 25;                           // Internal margins
    const int knobSize = 53;                             // Match global knobs

    // Grid layout parameters for 2×4 arrangement (2 columns, up to 4 rows)
    const int combColumns = 2;
    const int combHSpacing = 90;    // Horizontal spacing between columns
    const int combVSpacing = 70;    // Reduced vertical spacing to fit in 600px window
    const int combStartY = 120;     // Moved up to optimize space usage

    const int labelHeight = 20;
    const int valueHeight = 18;

    // Define comb control knobs and their properties
    struct CombKnobDef {
        const char* label;
        KnobControl** knob;
        VSTGUI::CTextLabel** labelPtr;
        VSTGUI::CTextLabel** valuePtr;
        int tag;
    };

    // Map to actual VST3 parameter IDs
    CombKnobDef combKnobs[] = {
        {"SIZE", &combSizeKnob, &combSizeLabel, &combSizeValue, kCombSize},
        {"FEEDBACK", &combFeedbackKnob, &combFeedbackLabel, &combFeedbackValue, kCombFeedback},
        {"PITCH", &combPitchKnob, &combPitchLabel, &combPitchValue, kCombPitchCV},
        {"TAPS", &combTapsKnob, &combTapsLabel, &combTapsValue, kCombTaps},
        {"SYNC", &combSyncKnob, &combSyncLabel, &combSyncValue, kCombSync},
        {"DIVISION", &combDivisionKnob, &combDivisionLabel, &combDivisionValue, kCombDivision},
        {"ROUTE", &routeModeKnob, &routeModeLabel, &routeModeValue, kRouteMode}
    };

    const int numCombKnobs = sizeof(combKnobs) / sizeof(CombKnobDef);

    for (int i = 0; i < numCombKnobs; i++) {
        // Calculate grid position (2 columns)
        int col = i % combColumns;  // 0 or 1
        int row = i / combColumns;  // 0, 1, 2, or 3

        // Calculate knob position in grid
        int knobX = combSectionLeft + combMargin + (col * (knobSize + combHSpacing));
        int knobY = combStartY + (row * (knobSize + combVSpacing));

        // Create knob
        VSTGUI::CRect knobRect(knobX, knobY, knobX + knobSize, knobY + knobSize);
        *(combKnobs[i].knob) = new KnobControl(knobRect, this, combKnobs[i].tag);

        // Load initial value from controller instead of hardcoded 0.5f
        auto controller = getController();
        if (controller) {
            float value = controller->getParamNormalized(combKnobs[i].tag);
            (*(combKnobs[i].knob))->setValue(value);
        }

        container->addView(*(combKnobs[i].knob));

        // Create label with dynamic width calculation
        const char* text = combKnobs[i].label;
        int textLength = static_cast<int>(strlen(text));
        int labelWidth = std::max(knobSize, static_cast<int>(textLength * 7.5f + 8));
        int labelLeft = knobX + (knobSize - labelWidth) / 2;  // Center under knob

        VSTGUI::CRect labelRect(labelLeft, knobY + knobSize + 5, labelLeft + labelWidth, knobY + knobSize + 5 + labelHeight);
        *(combKnobs[i].labelPtr) = new VSTGUI::CTextLabel(labelRect, combKnobs[i].label);

        // Set label styling to match global controls
        auto label = *(combKnobs[i].labelPtr);
        label->setHoriAlign(VSTGUI::kCenterText);
        label->setFontColor(VSTGUI::kBlackCColor);
        label->setBackColor(VSTGUI::kTransparentCColor);
        label->setFrameColor(VSTGUI::kTransparentCColor);
        label->setStyle(VSTGUI::CTextLabel::kNoFrame);

        auto customFont = getWorkSansFont(11.0f);
        if (customFont) {
            label->setFont(customFont);
        }

        container->addView(label);

        // Create value readout centered under knob
        VSTGUI::CRect valueRect(knobX, knobY + knobSize + 5 + labelHeight + 2, knobX + knobSize, knobY + knobSize + 5 + labelHeight + 2 + valueHeight);
        *(combKnobs[i].valuePtr) = new VSTGUI::CTextLabel(valueRect, "");

        // Set initial value text from controller
        if (controller) {
            float value = controller->getParamNormalized(combKnobs[i].tag);
            std::string valueText = formatParameterValue(combKnobs[i].tag, value);
            (*(combKnobs[i].valuePtr))->setText(valueText.c_str());
        }

        // Set value readout styling to match global controls
        auto valueLabel = *(combKnobs[i].valuePtr);
        valueLabel->setHoriAlign(VSTGUI::kCenterText);
        valueLabel->setFontColor(VSTGUI::kBlackCColor);
        valueLabel->setBackColor(VSTGUI::kTransparentCColor);
        valueLabel->setFrameColor(VSTGUI::kTransparentCColor);
        valueLabel->setStyle(VSTGUI::CTextLabel::kNoFrame);

        auto valueFont = getWorkSansFont(9.0f);
        if (valueFont) {
            valueLabel->setFont(valueFont);
        }

        container->addView(valueLabel);
    }
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

    // Set stroke and drawing properties
    context->setLineWidth(2.0);
    context->setDrawMode(VSTGUI::kAntiAliasing);

    // Create rounded rectangle with 6px corner radius
    const double cornerRadius = 6.0;

    if (isBypassed) {
        // Bypassed state: black background, white text, black stroke
        context->setFillColor(VSTGUI::kBlackCColor);
        context->setFrameColor(VSTGUI::kBlackCColor);
    } else {
        // Active state: white background, black text, black stroke
        context->setFillColor(VSTGUI::kWhiteCColor);
        context->setFrameColor(VSTGUI::kBlackCColor);
    }

    // Draw rounded rectangle
    VSTGUI::CGraphicsPath* path = context->createRoundRectGraphicsPath(rect, cornerRadius);
    if (path) {
        context->drawGraphicsPath(path, VSTGUI::CDrawContext::kPathFilled);
        context->drawGraphicsPath(path, VSTGUI::CDrawContext::kPathStroked);
        path->forget();
    }

    // Draw text
    const char* text = isBypassed ? "BYP" : "OFF";

    // Set text color
    if (isBypassed) {
        context->setFontColor(VSTGUI::kWhiteCColor);  // White text on black background
    } else {
        context->setFontColor(VSTGUI::kBlackCColor);  // Black text on white background
    }

    // Set font - use WorkSans-Regular 11pt to match mode button labels
    auto editor = dynamic_cast<WaterStickEditor*>(listener);
    auto customFont = editor ? editor->getWorkSansFont(11.0f) : nullptr;
    if (customFont) {
        context->setFont(customFont);
    } else {
        auto systemFont = VSTGUI::kSystemFont;
        systemFont->setSize(11);
        context->setFont(systemFont);
    }

    // Calculate text position for centering
    auto textWidth = context->getStringWidth(text);
    VSTGUI::CPoint center = rect.getCenter();
    VSTGUI::CPoint textPos(
        center.x - textWidth / 2.0,
        center.y + 4.0  // Slight vertical adjustment for better visual centering
    );

    // Draw the text
    context->drawString(text, textPos);

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
        }

        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

void WaterStickEditor::createBypassControls(VSTGUI::CViewContainer* container)
{
    // Position bypass toggles within the delay section boundary (667px)
    const int buttonSize = 53;           // Match tap/mode button size
    const int buttonSpacing = buttonSize / 2;
    const int toggleWidth = 50;          // Smaller width for bypass toggles
    const int toggleHeight = 30;         // Smaller height for bypass toggles

    // Match mode button positioning
    const int delayMargin = 30;
    const int gridLeft = delayMargin;
    const int delaySectionWidth = (kEditorWidth * 2) / 3;  // 667px boundary

    // Calculate mode button Y position (matches createModeButtons logic)
    const int gridWidth = 8;
    const int gridHeight = 2;
    const int totalGridHeight = (gridHeight * buttonSize) + ((gridHeight - 1) * buttonSpacing);
    const int upperTwoThirdsHeight = (kEditorHeight * 2) / 3;
    const int tapGridTop = (upperTwoThirdsHeight - totalGridHeight) / 2;
    const int modeButtonY = tapGridTop + (gridHeight * buttonSize) + buttonSpacing + (buttonSpacing * 1.5);

    // Position bypass toggles WITHIN delay section - stack vertically to the right of mode buttons
    // Place them at the right edge of delay section with margin
    const int rightMargin = 30;
    const int toggleGap = 10;  // Gap between the two bypass toggles
    const int firstToggleX = delaySectionWidth - rightMargin - toggleWidth - toggleGap - toggleWidth;  // Room for both toggles
    const int toggleY = modeButtonY + (buttonSize - toggleHeight) / 2;  // Center vertically with mode buttons

    // Create delay bypass toggle
    VSTGUI::CRect delayToggleRect(firstToggleX, toggleY, firstToggleX + toggleWidth, toggleY + toggleHeight);
    delayBypassToggle = new BypassToggle(delayToggleRect, this, kDelayBypass);

    // Load initial value from controller
    auto controller = getController();
    if (controller) {
        float value = controller->getParamNormalized(kDelayBypass);
        delayBypassToggle->setValue(value);
    }

    container->addView(delayBypassToggle);

    // Create delay bypass label
    const int labelHeight = 20;
    const int labelY = modeButtonY + buttonSize + 15;  // Match mode button label positioning
    VSTGUI::CRect delayLabelRect(firstToggleX, labelY, firstToggleX + toggleWidth, labelY + labelHeight);
    delayBypassLabel = new VSTGUI::CTextLabel(delayLabelRect, "D-BYP");

    // Set label styling to match mode button labels
    delayBypassLabel->setHoriAlign(VSTGUI::kCenterText);
    delayBypassLabel->setFontColor(VSTGUI::kBlackCColor);
    delayBypassLabel->setBackColor(VSTGUI::kTransparentCColor);
    delayBypassLabel->setFrameColor(VSTGUI::kTransparentCColor);
    delayBypassLabel->setStyle(VSTGUI::CTextLabel::kNoFrame);

    auto customFont = getWorkSansFont(11.0f);
    if (customFont) {
        delayBypassLabel->setFont(customFont);
    }

    container->addView(delayBypassLabel);

    // Create comb bypass toggle (position after delay bypass)
    const int secondToggleX = firstToggleX + toggleWidth + toggleGap;  // Use consistent gap
    VSTGUI::CRect combToggleRect(secondToggleX, toggleY, secondToggleX + toggleWidth, toggleY + toggleHeight);
    combBypassToggle = new BypassToggle(combToggleRect, this, kCombBypass);

    // Load initial value from controller
    if (controller) {
        float value = controller->getParamNormalized(kCombBypass);
        combBypassToggle->setValue(value);
    }

    container->addView(combBypassToggle);

    // Create comb bypass label
    VSTGUI::CRect combLabelRect(secondToggleX, labelY, secondToggleX + toggleWidth, labelY + labelHeight);
    combBypassLabel = new VSTGUI::CTextLabel(combLabelRect, "C-BYP");

    // Set label styling to match mode button labels
    combBypassLabel->setHoriAlign(VSTGUI::kCenterText);
    combBypassLabel->setFontColor(VSTGUI::kBlackCColor);
    combBypassLabel->setBackColor(VSTGUI::kTransparentCColor);
    combBypassLabel->setFrameColor(VSTGUI::kTransparentCColor);
    combBypassLabel->setStyle(VSTGUI::CTextLabel::kNoFrame);

    if (customFont) {
        combBypassLabel->setFont(customFont);
    }

    container->addView(combBypassLabel);
}

} // namespace WaterStick