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

    // Initialize mode button
    modeButton1 = nullptr;
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

        // Set initial state based on parameter value
        auto controller = getController();
        if (controller) {
            auto paramValue = controller->getParamNormalized(button->getTag());
            button->setValue(paramValue);
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

    // Position under column 1 (leftmost column)
    const int modeButton1X = gridLeft;

    // Calculate expanded view bounds to accommodate the rectangle
    // Circle size: 30px - 5px stroke = 25px
    // Rectangle size: 25px * 1.5 = 37.5px
    // Expansion needed: (37.5px - 30px) / 2 = 3.75px per side
    const int expansionNeeded = 4; // Round up to 4px for safety

    VSTGUI::CRect modeButtonRect(
        modeButton1X - expansionNeeded,
        modeButtonY - expansionNeeded,
        modeButton1X + buttonSize + expansionNeeded,
        modeButtonY + buttonSize + expansionNeeded
    );

    // Create mode button with temporary tag (-1 for now)
    modeButton1 = new ModeButton(modeButtonRect, this, -1);

    // Add to container
    container->addView(modeButton1);
}

void WaterStickEditor::valueChanged(VSTGUI::CControl* control)
{
    if (control && control->getTag() != -1) {
        // Update parameter in controller
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
    bool isEnabled = (getValue() > 0.5);

    // Set stroke width to 5px
    context->setLineWidth(5.0);
    context->setFrameColor(VSTGUI::kBlackCColor);

    // Create drawing rect that accounts for stroke width
    // Inset by half the stroke width to prevent clipping
    VSTGUI::CRect drawRect = rect;
    const double strokeInset = 2.5; // Half of 5px stroke
    drawRect.inset(strokeInset, strokeInset);

    if (isEnabled) {
        // Enabled state: black fill with black stroke
        context->setFillColor(VSTGUI::kBlackCColor);
        context->drawEllipse(drawRect, VSTGUI::kDrawFilled);
        context->drawEllipse(drawRect, VSTGUI::kDrawStroked);
    } else {
        // Disabled state: no fill, black stroke only
        context->drawEllipse(drawRect, VSTGUI::kDrawStroked);
    }

    setDirty(false);
}

VSTGUI::CMouseEventResult TapButton::onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (buttons & VSTGUI::kLButton) {
        // Start drag operation
        dragMode = true;

        // Clear the affected buttons set for this new drag operation
        resetDragAffectedSet();

        // Toggle this button (flip its current state)
        setValue(getValue() > 0.5 ? 0.0 : 1.0);
        invalid();  // Trigger redraw

        // Mark this button as affected in this drag operation
        markButtonAsAffected(this);

        // Notify listener
        if (listener) {
            listener->valueChanged(this);
        }

        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

VSTGUI::CMouseEventResult TapButton::onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (dragMode && (buttons & VSTGUI::kLButton)) {
        // Convert local coordinates to frame coordinates for hit testing
        VSTGUI::CPoint framePoint = where;
        localToFrame(framePoint);

        // Get editor to find button at this point
        auto editor = dynamic_cast<WaterStickEditor*>(listener);
        if (editor) {
            auto targetButton = editor->getTapButtonAtPoint(framePoint);
            if (targetButton && !isButtonAlreadyAffected(targetButton)) {
                // Toggle the target button's current state (flip it)
                double newValue = targetButton->getValue() > 0.5 ? 0.0 : 1.0;
                targetButton->setValue(newValue);
                targetButton->invalid();
                editor->valueChanged(targetButton);

                // Mark this button as affected so it won't be toggled again
                markButtonAsAffected(targetButton);
            }
        }

        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

VSTGUI::CMouseEventResult TapButton::onMouseUp(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (dragMode) {
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
        // Toggle button state
        setValue(getValue() > 0.5 ? 0.0 : 1.0);
        invalid();  // Trigger redraw

        // Notify listener
        if (listener) {
            listener->valueChanged(this);
        }

        return VSTGUI::kMouseEventHandled;
    }
    return VSTGUI::kMouseEventNotHandled;
}

} // namespace WaterStick