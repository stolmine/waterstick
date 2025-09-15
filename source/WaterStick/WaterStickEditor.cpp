#include "WaterStickEditor.h"
#include "vstgui/lib/cviewcontainer.h"
#include "vstgui/lib/ctextlabel.h"
#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/cfont.h"

namespace WaterStick {

WaterStickEditor::WaterStickEditor(Steinberg::Vst::EditController* controller)
: VST3Editor(controller, nullptr, nullptr)
{
    Steinberg::ViewRect viewRect(0, 0, kEditorWidth, kEditorHeight);
    setRect(viewRect);
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
    frame->addView(container);

    return true;
}

} // namespace WaterStick