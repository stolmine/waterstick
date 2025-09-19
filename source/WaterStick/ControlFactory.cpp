#include "ControlFactory.h"
#include "WaterStickParameters.h"
#include "vstgui/lib/ccolor.h"
#include <algorithm>
#include <cstring>

namespace WaterStick {

ControlFactory::ControlFactory(WaterStickEditor* editor, VSTGUI::CViewContainer* container)
    : editor(editor), container(container)
{
}

KnobSet ControlFactory::createKnob(const VSTGUI::CRect& knobRect, int tag, const char* labelText, bool isTimeDivision)
{
    KnobSet knobSet = {};

    knobSet.knob = new KnobControl(knobRect, editor, tag);
    if (isTimeDivision) {
        knobSet.knob->setIsTimeDivisionKnob(true);
    }

    auto controller = editor->getController();
    if (controller) {
        float value = controller->getParamNormalized(tag);
        knobSet.knob->setValue(value);
    }

    container->addView(knobSet.knob);

    int knobSize = static_cast<int>(knobRect.getWidth());
    int labelWidth = calculateLabelWidth(labelText, knobSize);
    int labelLeft = static_cast<int>(knobRect.left + (knobSize - labelWidth) / 2);

    VSTGUI::CRect labelRect(labelLeft, knobRect.bottom + 5, labelLeft + labelWidth, knobRect.bottom + 25);
    knobSet.label = createLabel(labelRect, labelText, 11.0f, false);
    container->addView(knobSet.label);

    VSTGUI::CRect valueRect(knobRect.left, labelRect.bottom + 2, knobRect.right, labelRect.bottom + 20);
    knobSet.valueLabel = createLabel(valueRect, "", 9.0f, true);

    if (controller) {
        float value = controller->getParamNormalized(tag);
        std::string valueText = editor->formatParameterValue(tag, value);
        knobSet.valueLabel->setText(valueText.c_str());
    }

    container->addView(knobSet.valueLabel);

    return knobSet;
}

void ControlFactory::createKnobWithLayout(int x, int y, int knobSize, const KnobDefinition& def)
{
    VSTGUI::CRect knobRect(x, y, x + knobSize, y + knobSize);
    KnobSet knobSet = createKnob(knobRect, def.tag, def.label, def.isTimeDivision);

    *(def.knobPtr) = knobSet.knob;
    *(def.labelPtr) = knobSet.label;
    *(def.valuePtr) = knobSet.valueLabel;
}

void ControlFactory::createGlobalKnobsHorizontal(int startX, int y, int knobSize, int spacing,
                                               const KnobDefinition* defs, int count)
{
    for (int i = 0; i < count; i++) {
        int knobX = startX + i * (knobSize + spacing);
        createKnobWithLayout(knobX, y, knobSize, defs[i]);
    }
}

void ControlFactory::createCombKnobsGrid(int startX, int startY, int knobSize, int hSpacing, int vSpacing,
                                       int columns, const KnobDefinition* defs, int count)
{
    for (int i = 0; i < count; i++) {
        int col = i % columns;
        int row = i / columns;

        int knobX = startX + col * (knobSize + hSpacing);
        int knobY = startY + row * (knobSize + vSpacing);

        createKnobWithLayout(knobX, knobY, knobSize, defs[i]);
    }
}

VSTGUI::CTextLabel* ControlFactory::createLabel(const VSTGUI::CRect& rect, const char* text,
                                              float fontSize, bool isValueLabel)
{
    auto label = new VSTGUI::CTextLabel(rect, text);
    styleLabel(label, fontSize, isValueLabel);
    return label;
}

int ControlFactory::calculateLabelWidth(const char* text, int minWidth)
{
    int textLength = static_cast<int>(strlen(text));
    int approximateWidth = static_cast<int>(textLength * 7.5f + 8);
    return std::max(minWidth, approximateWidth);
}

void ControlFactory::styleLabel(VSTGUI::CTextLabel* label, float fontSize, bool isValueLabel)
{
    label->setHoriAlign(VSTGUI::kCenterText);
    label->setFontColor(VSTGUI::kBlackCColor);
    label->setBackColor(VSTGUI::kTransparentCColor);
    label->setFrameColor(VSTGUI::kTransparentCColor);
    label->setStyle(VSTGUI::CTextLabel::kNoFrame);

    auto font = getWorkSansFont(fontSize);
    if (font) {
        label->setFont(font);
    }
}

VSTGUI::SharedPointer<VSTGUI::CFontDesc> ControlFactory::getWorkSansFont(float size) const
{
    return editor->getWorkSansFont(size);
}

} // namespace WaterStick