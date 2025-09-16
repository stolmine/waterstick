#include "WaterStickEditor.h"
#include "WaterStickParameters.h"
#include "vstgui/lib/cviewcontainer.h"
#include "vstgui/lib/controls/ctextlabel.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/cframe.h"
#include <cmath>

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

    frame->addView(container);

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
    // Button grid configuration
    const int buttonSize = 30;           // Button diameter
    const int buttonSpacing = buttonSize / 2;  // Half diameter spacing
    const int gridWidth = 8;            // 8 columns
    const int gridHeight = 2;           // 2 rows

    // Calculate total grid dimensions
    const int totalGridWidth = (gridWidth * buttonSize) + ((gridWidth - 1) * buttonSpacing);
    const int totalGridHeight = (gridHeight * buttonSize) + ((gridHeight - 1) * buttonSpacing);

    // Center the grid in the window (occupying ~30% of space)
    const int gridLeft = (kEditorWidth - totalGridWidth) / 2;
    const int gridTop = (kEditorHeight - totalGridHeight) / 2;

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
    // Button grid configuration (matching tap buttons)
    const int buttonSize = 30;           // Button diameter
    const int buttonSpacing = buttonSize / 2;  // Half diameter spacing
    const int gridWidth = 8;            // 8 columns
    const int gridHeight = 2;           // 2 rows

    // Calculate total grid dimensions
    const int totalGridWidth = (gridWidth * buttonSize) + ((gridWidth - 1) * buttonSpacing);

    // Center the grid in the window (matching tap buttons)
    const int gridLeft = (kEditorWidth - totalGridWidth) / 2;
    const int tapGridTop = (kEditorHeight - (gridHeight * buttonSize + buttonSpacing)) / 2;

    // Calculate mode button position
    // Place 1.5x button spacing below the tap button grid
    const int modeButtonY = tapGridTop + (gridHeight * buttonSize) + buttonSpacing + (buttonSpacing * 1.5);

    // Calculate expanded view bounds to accommodate the rectangle
    // Circle size: 30px - 5px stroke = 25px
    // Rectangle size: 25px * 1.5 = 37.5px
    // Expansion needed: (37.5px - 30px) / 2 = 3.75px per side
    const int expansionNeeded = 4; // Round up to 4px for safety

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
            }
        }
    }
    else if (control && control->getTag() != -1) {
        // Handle other controls (non-tap, non-mode buttons)
        auto controller = getController();
        if (controller) {
            controller->setParamNormalized(control->getTag(), control->getValue());
            controller->performEdit(control->getTag(), control->getValue());
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

int WaterStickEditor::getTapParameterIdForContext(int tapButtonIndex, TapContext context) const
{
    // Convert tap button index to tap number (1-16)
    // The grid layout: taps 1-8 are top row (indices 0-7), taps 9-16 are bottom row (indices 8-15)
    int tapNumber = tapButtonIndex + 1; // Convert from 0-15 to 1-16

    // Calculate base parameter offset for this tap (each tap has 3 params: Enable, Level, Pan)
    int baseOffset = (tapNumber - 1) * 3;

    switch (context) {
        case TapContext::Enable:
            return kTap1Enable + baseOffset;  // Enable parameter
        case TapContext::Volume:
            return kTap1Level + baseOffset;   // Level parameter
        case TapContext::Pan:
            return kTap1Pan + baseOffset;     // Pan parameter (future)
        default:
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
        if (currentContext == TapContext::Volume) {
            // Volume context: Handle continuous control
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
        // Volume context: Handle continuous dragging
        double deltaX = where.x - initialClickPoint.x;
        double deltaY = initialClickPoint.y - where.y;  // Negative deltaY = drag up = increase volume

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

        if (currentDragDirection == DragDirection::Horizontal) {
            // Horizontal drag: Set volume on different taps based on mouse position
            VSTGUI::CPoint framePoint = where;
            localToFrame(framePoint);

            auto editor = dynamic_cast<WaterStickEditor*>(listener);
            if (editor) {
                auto targetButton = editor->getTapButtonAtPoint(framePoint);
                if (targetButton) {
                    // Calculate volume based on vertical position within the target button
                    const VSTGUI::CRect& targetRect = targetButton->getViewSize();
                    VSTGUI::CRect targetDrawRect = targetRect;
                    const double strokeInset = 2.5; // Half of 5px stroke
                    targetDrawRect.inset(strokeInset, strokeInset);

                    // Convert frame coordinates to target button local coordinates
                    VSTGUI::CPoint localPoint = framePoint;
                    targetButton->frameToLocal(localPoint);

                    // Calculate volume based on Y position within target button
                    double relativeY = (targetDrawRect.bottom - localPoint.y) / targetDrawRect.getHeight();
                    relativeY = std::max(0.0, std::min(1.0, relativeY)); // Clamp to [0.0, 1.0]

                    // Update the target button's volume
                    targetButton->setValue(relativeY);
                    targetButton->invalid();
                    editor->valueChanged(targetButton);
                }
            }
        }
        else if (currentDragDirection == DragDirection::Vertical) {
            // Vertical drag: Relative volume adjustment on this button
            double sensitivity = 1.0 / 30.0;  // 30 pixels = full range (0.0 to 1.0)
            double volumeChange = deltaY * sensitivity;

            double newVolume = initialVolumeValue + volumeChange;
            newVolume = std::max(0.0, std::min(1.0, newVolume));  // Clamp to [0.0, 1.0]

            setValue(newVolume);
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
        // Volume context: Check if this was a click or drag
        if (currentDragDirection == DragDirection::None) {
            // This was a click - set volume based on absolute position within circle
            const VSTGUI::CRect& rect = getViewSize();
            VSTGUI::CRect drawRect = rect;
            const double strokeInset = 2.5; // Half of 5px stroke
            drawRect.inset(strokeInset, strokeInset);

            // Calculate position within the circle (0.0 = bottom, 1.0 = top)
            double relativeY = (drawRect.bottom - where.y) / drawRect.getHeight();
            relativeY = std::max(0.0, std::min(1.0, relativeY)); // Clamp to [0.0, 1.0]

            // Set the volume value
            setValue(relativeY);
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

    // Define the logical button area (30x30px) centered in the expanded view
    const double buttonSize = 30.0;
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
        const double strokeInset = 2.5; // Half of 5px stroke width
        const double circleSize = buttonSize - (strokeInset * 2); // 30px - 5px = 25px
        const double rectangleSize = circleSize * 1.5; // 25px * 1.5 = 37.5px

        // Center rectangle on the button's center
        const double halfRectSize = rectangleSize / 2.0;

        VSTGUI::CRect backgroundRect(
            viewCenter.x - halfRectSize,
            viewCenter.y - halfRectSize,
            viewCenter.x + halfRectSize,
            viewCenter.y + halfRectSize
        );

        // Draw black rounded rectangle background (no stroke, fill only)
        const double cornerRadius = 8.0; // Circular corner radius
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

    // Draw center dot (7px diameter)
    const double centerDotRadius = 3.5; // 7px diameter = 3.5px radius
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

} // namespace WaterStick