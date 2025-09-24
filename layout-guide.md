# WaterStick VST3 Plugin - Final Layout Architecture Specification

## Overview

This document provides the complete technical specification for the WaterStick VST3 plugin's interface layout system. The interface has reached its final form with all positioning issues resolved, optimal spacing achieved, and professional visual hierarchy established.

## Plugin Dimensions

### Base Constants
```cpp
static constexpr int kEditorWidth = 670;   // Total plugin width
static constexpr int kEditorHeight = 480;  // Final optimized height (+40px from original)
```

### Coordinate System
- **Origin**: Top-left corner (0, 0)
- **Positive X**: Rightward
- **Positive Y**: Downward
- **Units**: Pixels (absolute positioning)
- **Layout System**: Two-phase positioning with equal margin adjustment

## Layout Architecture

### Layout Creation Sequence
The interface is constructed in a specific order from the `open()` method:
```cpp
createTapButtons(container);        // Phase 1: 16 tap buttons in 8x2 grid
createSmartHierarchy(container);    // Phase 2: 24 triangular smart hierarchy controls
createModeButtons(container);       // Phase 3: 8 mode buttons + context labels
createGlobalControls(container);    // Phase 4: Global knobs + bypass toggle
createMinimap(container);          // Phase 5: 16 minimap indicators
applyEqualMarginLayout(container); // Phase 6: Final positioning with equal margins
```

### Two-Phase Positioning System
1. **Phase 1 - Initial Positioning**: Elements positioned using absolute coordinates
2. **Phase 2 - Equal Margin Layout**: All elements repositioned for balanced margins

## Vertical Layout Structure

### Three-Section Division
```
┌─────────────────────────────────────────────────────────────────┐ ← Y=0
│ UPPER SECTION (Delay Processing Area)                          │
│ Height: (kEditorHeight * 2) / 3 = (480 * 2) / 3 = 320px       │
│                                                                 │
│ • Tap Button Grid (8x2)                                        │
│ • Triangular Smart Hierarchy (8 triangles)                     │
│ • Mode Buttons + Context Labels                                 │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤ ← Y=320
│ LOWER SECTION (Global Controls Area)                           │
│ Height: kEditorHeight / 3 = 480 / 3 = 160px                    │
│                                                                 │
│ • Global Control Knobs                                         │
│ • Bypass Toggle                                                │
│ • Labels and Value Displays                                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘ ← Y=480
```

### Universal Centering Adjustment
**Critical**: All positioning calculations apply a **-15px** upward shift for optimal visual centering:
```cpp
// Applied consistently across all layout functions
const int centeringAdjustment = -15;  // Optimized from original -23px
```

## Detailed Component Specifications

### 1. Tap Button Grid (Primary Interface)

#### Layout Constants
```cpp
const int buttonSize = 53;                    // Button diameter
const int buttonSpacing = buttonSize / 2;    // 26.5px inter-button spacing
const int gridHeight = 2;                    // 2 rows
const int delayMargin = 30;                  // Left margin alignment
```

#### Positioning Calculation
```cpp
const int upperTwoThirdsHeight = (480 * 2) / 3;     // 320px
const int totalGridHeight = (2 * 53) + (1 * 26);   // 132px total
const int gridTop = ((320 - 132) / 2) - 15;        // 78.5px → 78px (floored)
```

#### Grid Coordinate Map
```
Y=78   ┌─[T1]─┬─[T2]─┬─[T3]─┬─[T4]─┬─[T5]─┬─[T6]─┬─[T7]─┬─[T8]─┐
       │ 53px │ 53px │ 53px │ 53px │ 53px │ 53px │ 53px │ 53px │
Y=131  ├──────┼──────┼──────┼──────┼──────┼──────┼──────┼──────┤ ← 26.5px gap
       │ 53px │ 53px │ 53px │ 53px │ 53px │ 53px │ 53px │ 53px │
Y=184  └─[T9]─┴─[T10]┴─[T11]┴─[T12]┴─[T13]┴─[T14]┴─[T15]┴─[T16]┘

X:     30    109   188   267   346   425   504   583   px
```

### 2. Triangular Smart Hierarchy System

#### Design Philosophy
The Triangular Smart Hierarchy represents a major design breakthrough, organizing macro controls in intuitive triangular formations that eliminate overlap issues while creating superior visual hierarchy.

#### Triangle Specifications
```cpp
const int triangleHeight = 50;        // Total triangle height
const int triangleBaseWidth = 40;     // Triangle base width
const int macroKnobSize = 24;        // Macro knob diameter
const int actionButtonSize = 14;     // R/X button size
```

#### Space Allocation
```cpp
const int tapGridBottom = 78 + 106 + 26;                        // 210px
const int modeButtonY = 78 + 106 + 26 + (26 * 3.0);            // 288px (3.05x spacing)
const int availableSpace = 288 - 210;                           // 78px total space
const int triangleStartY = 210 + ((78 - 50) / 2);              // 224px (centered)
```

#### Triangle Positioning
```cpp
// Y-coordinates for triangular arrangement
const int macroKnobY = 224;                    // Triangle apex
const int actionButtonsY = 224 + 50 - 14;     // Triangle base (260px)

// X-coordinates per column (8 triangles)
for (int i = 0; i < 8; i++) {
    int columnX = 30 + i * (53 + 26);         // Base column position

    // Macro knob at triangle top center
    int macroKnobX = columnX + (53 - 24) / 2; // Centered in column

    // R button at triangle bottom left
    int triangleLeftOffset = (53 - 40) / 2;   // 6.5px offset
    int rButtonX = columnX + triangleLeftOffset;

    // X button at triangle bottom right
    int xButtonX = columnX + triangleLeftOffset + 40 - 14; // 32.5px from left
}
```

#### Triangular Layout Visual Specification
```
     ▲ M1 ▲     ← Macro Knob (24x24px) at Y=224
    ╱       ╲
   ╱  50px   ╲   ← Triangle Height
  ╱  Height   ╲
 ╱             ╲
R1 ←─ 40px ─→ X1  ← R/X Buttons (14x14px) at Y=260
```

#### 8-Column Triangular Array
```
Y=210  ┌─────────────── Tap Grid Bottom ─────────────────┐
Y=224  │  ▲M1▲   ▲M2▲   ▲M3▲   ▲M4▲   ▲M5▲   ▲M6▲   ▲M7▲   ▲M8▲  │ ← Macro Knobs
       │  ╱ ╲    ╱ ╲    ╱ ╲    ╱ ╲    ╱ ╲    ╱ ╲    ╱ ╲    ╱ ╲   │
       │ ╱   ╲  ╱   ╲  ╱   ╲  ╱   ╲  ╱   ╲  ╱   ╲  ╱   ╲  ╱   ╲  │
Y=260  │ R1  X1  R2  X2  R3  X3  R4  X4  R5  X5  R6  X6  R7  X7  │ ← Action Buttons
Y=274  │                                                         │ ← Triangle end
       │                    14px safety gap                      │
Y=288  └─────────────── Mode Button Y ──────────────────────────┘
```

### 3. Mode Buttons and Context Labels

#### Positioning Calculation
```cpp
const int modeButtonY = tapGridTop + (gridHeight * buttonSize) + buttonSpacing + (buttonSpacing * 3.0);
// 78 + (2 * 53) + 26 + (26 * 3.0) = 78 + 106 + 26 + 78 = 288px

const int labelY = modeButtonY + buttonSize + 12; // 288 + 53 + 12 = 353px
```

#### Mode Button Layout
```
Y=288  ┌─[M1]─┬─[M2]─┬─[M3]─┬─[M4]─┬─[M5]─┬─[M6]─┬─[M7]─┬─[M8]─┐
       │ 53px │ 53px │ 53px │ 53px │ 53px │ 53px │ 53px │ 53px │
Y=341  └──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┘
Y=353  │Mutes │Level │ Pan  │Cutoff│ Res  │Type  │Pitch │FBSend│ ← Context Labels
Y=373  └──────────────────── Context Labels End ─────────────────┘
```

### 4. Global Controls Section

#### Positioning Calculation
```cpp
const int bottomThirdTop = ((480 * 2) / 3);                    // 320px (before centering)
const int modeButtonSpacing = static_cast<int>(buttonSpacing * 3.05); // 79.3px (refined spacing)
const int knobY = bottomThirdTop + modeButtonSpacing - 10;      // 320 + 79.3 - 10 = 389.3px
```

#### Global Layout Structure
```
Y=373  ┌─────────── Context Labels End ────────────┐
       │              16px safety gap              │
Y=389  │ [BYP][K1] [K2] [K3] [K4] [K5] [K6] [K7]  │ ← Global Knobs (53px)
Y=442  │ D-BYP SYNC TIME FDBK GRID  IN  OUT G-MIX │ ← Labels (20px)
Y=462  │  OFF  1/16 127ms 45%  ON  0dB +3dB  67%  │ ← Values (18px)
Y=480  └─────────────────────────────────────────────┘
```

### 5. Minimap Integration System

#### Positioning Strategy
The minimap uses **synchronized positioning** with tap buttons to ensure perfect alignment:
```cpp
// CRITICAL: Must match tap button positioning exactly
const int gridTop = ((upperTwoThirdsHeight - totalGridHeight) / 2) - 15; // SYNCHRONIZED
```

#### Minimap Coordinate Calculation
```cpp
// Position minimap circles relative to corresponding tap buttons
for (int i = 0; i < 16; i++) {
    int row = i / 8;
    int col = i % 8;

    // X-position: Center of corresponding tap button column minus half circle
    double minimapX = gridLeft + col * (buttonSize + buttonSpacing) + (buttonSize / 2.0) - 6.5;

    // Y-position: Relative to tap button rows
    double minimapY;
    if (row == 0) {
        // Row 1: 19.75px above tap buttons
        minimapY = gridTop - 19.75; // Y = 58.25px
    } else {
        // Row 2: Center of 26.5px gap between rows
        minimapY = gridTop + 53 + 6.75; // Y = 137.75px
    }
}
```

## Horizontal Grid System

### 8-Column Layout
```cpp
const int delayMargin = 30;           // Left edge alignment
const int columnSpacing = 79.5;       // Center-to-center spacing (53 + 26.5)

Column X-coordinates:
Col 0: 56.5px    (30 + 26.5)         Col 4: 373px
Col 1: 136px     (56.5 + 79.5)       Col 5: 452.5px
Col 2: 215.5px   (136 + 79.5)        Col 6: 532px
Col 3: 295px     (215.5 + 79.5)      Col 7: 611.5px
```

### Element Alignment
All interface elements align to the 8-column grid:
- **Tap buttons**: Center-aligned to grid columns
- **Triangular hierarchy**: Macro knobs centered, action buttons within triangle base
- **Mode buttons**: Center-aligned to grid columns
- **Global controls**: Bypass in column 0, knobs in columns 1-7
- **Minimap**: Circles centered above corresponding tap button columns

## Critical Y-Coordinate Reference

### Layout Hierarchy (Top to Bottom)
```
Y=58   ← Minimap Row 1 (13px circles)
Y=78   ← Tap Button Grid Top
Y=131  ← Tap Button Row 2 Start
Y=138  ← Minimap Row 2 (centered in gap)
Y=184  ← Tap Button Grid Bottom
Y=210  ← Available Space Start (Smart Hierarchy)
Y=224  ← Triangle Macro Knobs (apex)
Y=260  ← Triangle Action Buttons (base)
Y=274  ← Triangle End
Y=288  ← Mode Buttons Start
Y=341  ← Mode Buttons End
Y=353  ← Context Labels Start
Y=373  ← Context Labels End
Y=389  ← Global Knobs Start (16px safety gap)
Y=442  ← Global Knobs End / Labels Start
Y=462  ← Labels End / Values Start
Y=480  ← Plugin Bottom Boundary
```

## Equal Margin Layout System

### Operation Overview
The `applyEqualMarginLayout()` function performs the final positioning adjustment:

1. **Calculate Content Bounds**: Determines true bounding box of all visual elements
2. **Calculate Desired Position**: Centers content with equal margins
3. **Apply Offset**: Moves all elements by calculated X/Y offset

### Content Inclusion
All elements are included in both bounding box calculation AND repositioning:
- Tap buttons ✓
- Smart Hierarchy controls ✓
- Mode buttons and labels ✓
- Global controls and labels ✓
- Minimap buttons ✓

### Margin Calculation
```cpp
const int desiredMargin = 30;  // Target margin on all sides
// Final position ensures equal margins while respecting aspect ratio
```

## Layout Constants Summary

### Core Measurements
| Constant | Value | Purpose |
|----------|-------|---------|
| **kEditorWidth** | 670px | Total plugin width |
| **kEditorHeight** | 480px | Final optimized height |
| **buttonSize** | 53px | Standard element diameter |
| **buttonSpacing** | 26.5px | Inter-element spacing |
| **delayMargin** | 30px | Base content margin |
| **centeringAdjustment** | -15px | Universal vertical centering |

### Spacing Multipliers
| Context | Multiplier | Result | Purpose |
|---------|------------|--------|---------|
| Triangle spacing | 3.0x | 78px | Mode button clearance |
| Global control gap | 3.05x | 79.3px | Context-to-global spacing |
| Safety margins | 1.0x | 26.5px | Element separation |

### Element Dimensions
| Element | Width | Height | Notes |
|---------|-------|--------|-------|
| Tap buttons | 53px | 53px | Primary interface |
| Mode buttons | 53px | 53px | Context selection |
| Global knobs | 53px | 53px | Parameter control |
| Macro knobs | 24px | 24px | Smart hierarchy apex |
| Action buttons | 14px | 14px | R/X triangle base |
| Minimap circles | 13px | 13px | Status indicators |

## Design Principles

### Visual Hierarchy
1. **Top-Down Flow**: Tap grid → Smart controls → Context selection → Global parameters
2. **Functional Grouping**: Related controls spatially organized
3. **Progressive Disclosure**: Context-sensitive interface reveals relevant parameters
4. **Spatial Relationships**: Minimap provides spatial reference to main grid

### Layout Stability
1. **Consistent Alignment**: 8-column grid system throughout
2. **Proportional Spacing**: Mathematical relationships between elements
3. **Collision Prevention**: Adequate safety margins between sections
4. **Scalable Foundation**: Triangle and grid systems can accommodate future features

### Professional Standards
1. **Equal Margins**: Balanced appearance through systematic positioning
2. **Pixel Precision**: Integer coordinates for sharp rendering
3. **Hardware Fidelity**: Maintains Eurorack module proportions and aesthetics
4. **Usability**: Touch-friendly sizing with clear visual feedback

This specification represents the final, optimized layout architecture for the WaterStick VST3 plugin, with all positioning issues resolved and professional visual hierarchy established. The 480px height provides adequate spacing for all elements while maintaining the intended hardware-inspired aesthetic.