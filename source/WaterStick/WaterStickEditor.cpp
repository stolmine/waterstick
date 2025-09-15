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
}

bool PLUGIN_API WaterStickEditor::open(void* parent, const VSTGUI::PlatformType& platformType)
{
    VSTGUI::CRect frameSize(0, 0, kEditorWidth, kEditorHeight);

    frame = new VSTGUI::CFrame(frameSize, this);
    frame->open(parent, platformType);

    // Create main container with white background
    auto container = new VSTGUI::CViewContainer(frameSize);
    container->setBackgroundColor(VSTGUI::kWhiteCColor);

    // Create "WaterStick" text label
    VSTGUI::CRect textRect(0, 0, kEditorWidth, 50);
    textRect.centerInside(frameSize);

    auto textLabel = new VSTGUI::CTextLabel(textRect);
    textLabel->setText("WaterStick");
    textLabel->setFont(VSTGUI::kSystemFont);
    textLabel->setFontColor(VSTGUI::kBlackCColor);
    textLabel->setBackColor(VSTGUI::kTransparentCColor);
    textLabel->setHoriAlign(VSTGUI::kCenterText);
    textLabel->setStyle(VSTGUI::kBoldFace);

    container->addView(textLabel);

    // Create tap buttons
    createTapButtons(container);

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

//------------------------------------------------------------------------
// TapButton Implementation
//------------------------------------------------------------------------
TapButton::TapButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag)
: VSTGUI::CControl(size, listener, tag)
{
    setMax(1.0);  // Binary on/off button
    setMin(0.0);
}

void TapButton::draw(VSTGUI::CDrawContext* context)
{
    const VSTGUI::CRect& rect = getViewSize();
    bool isEnabled = (getValue() > 0.5);

    context->setLineWidth(2.0);
    context->setFrameColor(VSTGUI::kBlackCColor);

    if (isEnabled) {
        // Enabled state: black fill with black stroke
        context->setFillColor(VSTGUI::kBlackCColor);
        context->drawEllipse(rect, VSTGUI::kDrawFilled);
        context->drawEllipse(rect, VSTGUI::kDrawStroked);
    } else {
        // Disabled state: no fill, black stroke only
        context->drawEllipse(rect, VSTGUI::kDrawStroked);
    }

    setDirty(false);
}

VSTGUI::CMouseEventResult TapButton::onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons)
{
    if (buttons & VSTGUI::kLButton) {
        // Toggle value
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