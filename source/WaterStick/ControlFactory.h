#pragma once

#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/controls/ctextlabel.h"
#include "vstgui/lib/cviewcontainer.h"
#include "vstgui/lib/cfont.h"
#include "WaterStickEditor.h"

namespace WaterStick {

struct KnobSet {
    KnobControl* knob;
    VSTGUI::CTextLabel* label;
    VSTGUI::CTextLabel* valueLabel;
};

struct KnobDefinition {
    const char* label;
    int tag;
    KnobControl** knobPtr;
    VSTGUI::CTextLabel** labelPtr;
    VSTGUI::CTextLabel** valuePtr;
    bool isTimeDivision = false;
};

class ControlFactory {
public:
    ControlFactory(WaterStickEditor* editor, VSTGUI::CViewContainer* container);

    KnobSet createKnob(const VSTGUI::CRect& knobRect, int tag, const char* labelText,
                       bool isTimeDivision = false);

    void createKnobWithLayout(int x, int y, int knobSize, const KnobDefinition& def);

    void createGlobalKnobsHorizontal(int startX, int y, int knobSize, int spacing,
                                   const KnobDefinition* defs, int count);

    void createCombKnobsGrid(int startX, int startY, int knobSize, int hSpacing, int vSpacing,
                           int columns, const KnobDefinition* defs, int count);

private:
    WaterStickEditor* editor;
    VSTGUI::CViewContainer* container;

    VSTGUI::CTextLabel* createLabel(const VSTGUI::CRect& rect, const char* text,
                                  float fontSize = 11.0f, bool isValueLabel = false);
    int calculateLabelWidth(const char* text, int minWidth = 0);
    void styleLabel(VSTGUI::CTextLabel* label, float fontSize, bool isValueLabel = false);
    VSTGUI::SharedPointer<VSTGUI::CFontDesc> getWorkSansFont(float size) const;
};

} // namespace WaterStick