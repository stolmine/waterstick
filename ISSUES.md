# WaterStick VST3 - Current Issues Tracker

## Active Issues - Needs Investigation/Fix

### 🔧 Immediate Priority Issues

#### Signal Routing & Bypass Controls Investigation
**Priority**: Completed
**Component**: DSP/Signal Processing
**Description**: Comprehensive analysis of signal routing architecture and control mechanisms
**Branch**: V3.5.0_routingResearch

**Key Findings**:
1. **Bypass Controls**:
   - D-BYP and C-BYP controls implement sophisticated click-free signal bypass
   - Exponential fade curves: 10ms fade-out, 5ms fade-in for smooth transitions
   - Independent section control allowing granular signal management

2. **Signal Routing Architecture**:
   - Three routing modes: DelayToComb, CombToDelay, DelayPlusComb
   - RoutingManager with 10ms smooth transitions between modes
   - Professional signal flow design with minimal latency

3. **Dry/Wet Processing**:
   - Hierarchical mixing system with multi-stage controls
   - Global dry/wet (kGlobalDryWet) parameter
   - Delay-specific dry/wet control (kDelayDryWet)
   - Comb section always 100% wet by design

**Improvement Areas Identified**:
1. **Feedback Routing**: Currently global rather than section-specific
2. **Parallel Mode**: Fixed 0.5f scaling, no user control over delay/comb balance
3. **Transition Timing**: 10ms may be insufficient for very long delays
4. **Buffer Management**: Routing mode changes should clear all buffers to prevent artifacts

**User Experience Issues Identified**:
5. **Missing Gain Controls**: ✅ **COMPLETED** - Both Delay and Comb gain controls implemented
6. **Dry/Wet UI Confusion**: ✅ **COMPLETED** - Clear G-MIX, D-MIX, C-MIX hierarchy implemented
7. **UI Parameter Mismatch**: ✅ **COMPLETED** - All three dry/wet parameters properly exposed
8. **Mix Control Non-Responsiveness**: ✅ **COMPLETED** - Fixed across all routing modes
9. **Serial Routing Signal Leakage**: ✅ **COMPLETED** - Eliminated unexpected signal paths
10. **Buffer State Issues**: Routing mode changes don't clear processor buffers, causing audio artifacts

**Implementation Recommendations**:
1. Implement section-specific feedback routing
2. ✅ **COMPLETED** - Individual Delay and Comb gain controls now available in DSP and GUI
3. ✅ **COMPLETED** - Hierarchical G-MIX, D-MIX, C-MIX controls implemented
4. ✅ **COMPLETED** - Mix control responsiveness fixed across all routing modes
5. ✅ **COMPLETED** - Serial routing signal leakage eliminated
6. Add buffer clearing to routing mode transitions
7. Introduce dynamic scaling for parallel processing
8. Add user-controllable parallel mix balance
9. Fine-tune routing transition timing (current 10ms)

**Status**: Investigation Complete ✓

## Implementation Plan - Signal Routing Improvements

### ✅ Phase 1: C-GAIN Parameter (COMPLETE)
**Status**: Complete ✓
**Time**: 4 hours
**Branch**: V3.1_combGUI

**Completed:**
- Added kCombGain parameter with professional -40dB to +12dB range
- DSP integration with proper dB scaling in CombProcessor
- GUI layout updated to accommodate 10 comb parameters
- VST3 validation: 47/47 tests passed

### ✅ Phase 1.5: D-GAIN Parameter (COMPLETE)
**Status**: Complete ✅
**Time**: 2 hours
**Branch**: V3.5.0_routingResearch
**Commit**: dfbdcd1

**Completed:**
- Added kDelayGain parameter with professional -40dB to +12dB range (0dB default)
- DSP integration: applies gain to wet signal only in processDelaySection()
- GUI expanded from 7 to 8-knob layout with optimal spacing (26.5px)
- D-GAIN knob positioned between GRID and INPUT for logical grouping
- Full VST3 automation support with dB value display formatting
- Build successful, ready for DAW testing

### 🔄 Phase 2: Intelligent Comb SIZE/DIV Knob
**Priority**: High
**Component**: GUI/DSP
**Estimated Time**: 6-8 hours

**Tasks:**
1. Remove kCombDivision parameter from WaterStickParameters.h
2. Enhance CombProcessor with intelligent parameter handling:
   - Add backup value storage (mStoredTimeValue, mStoredDivisionValue)
   - Implement setCombSizeParameter() with mode-aware conversion
   - Add logarithmic time scaling (100μs-2.0s) and division mapping
3. Update GUI logic for intelligent knob behavior:
   - Modify valueChanged() handler for context-sensitive display
   - Implement time/division format switching
4. Reduce comb layout back to 3×3 grid (9 parameters)

**Expected Outcome**: Single SIZE/DIV knob that adapts behavior based on SYNC mode, matching delay section pattern

### 🔄 Phase 3: Section-Specific Feedback Routing
**Priority**: High
**Component**: DSP/Audio Processing
**Estimated Time**: 8-10 hours

**Tasks:**
1. Add separate feedback parameters: kDelayFeedback, enhance kCombFeedback routing
2. Implement independent feedback buffers in WaterStickProcessor
3. Add feedback routing logic based on routing mode (DelayToComb, CombToDelay, DelayPlusComb)
4. Apply tanh limiting to prevent runaway feedback
5. Update parameter system and GUI controls

**Expected Outcome**: Independent feedback control for delay and comb sections with proper routing

### ✅ Phase 4: Hierarchical Dry/Wet Controls (COMPLETE)
**Status**: Complete ✅
**Priority**: Medium
**Component**: GUI/User Experience
**Time**: 8 hours
**Branch**: V3.5.0_routingResearch
**Commits**: afb3c8b, e087314, d4dc125

**Completed:**
1. ✅ Implemented complete hierarchical dry/wet system (G-MIX, D-MIX, C-MIX)
2. ✅ Added kCombDryWet parameter for comb section control
3. ✅ GUI redesign with clear visual hierarchy:
   - G-MIX: 63px global control, visually elevated
   - D-MIX: 53px delay section control in global row
   - C-MIX: 53px comb section control in comb grid
4. ✅ Professional equal-power crossfading throughout signal chain
5. ✅ Fixed mix control non-responsiveness across routing modes
6. ✅ Serial-aware processing: 100% wet + no processing = silence
7. ✅ Eliminated signal leakage in C-to-D and D-to-C modes

**Technical Achievements:**
- Professional plugin signal flow standards implemented
- Sample-accurate parameter automation maintained
- Equal-power mixing applied appropriately (parallel mode only)
- Serial routing modes use unity gain to prevent volume loss
- No-processing detection for proper silence behavior

**VST3 Validation**: 47/47 tests passed ✅

### 🔄 Phase 5: Dynamic Parallel Processing
**Priority**: Medium
**Component**: DSP/GUI
**Estimated Time**: 4-6 hours

**Tasks:**
1. Add kParallelBalance parameter (0-100%)
2. Implement equal-power crossfading in DelayPlusComb mode
3. Add BALANCE knob with contextual activation (only active in D+C mode)
4. Update routing manager for user-controllable mixing
5. Replace fixed 0.5f scaling with dynamic user control

**Expected Outcome**: User-controllable balance between delay and comb in parallel mode

### 🔄 Phase 6: Buffer Clearing & Transition Optimization
**Priority**: Low
**Component**: DSP/Performance
**Estimated Time**: 4-6 hours

**Tasks:**
1. Implement 64-sample buffer clearing during routing transitions
2. Add adaptive transition timing (5-50ms based on delay lengths)
3. Update RoutingManager with enhanced transition logic
4. Add kTransitionTime parameter for user control
5. Comprehensive buffer state management

**Expected Outcome**: Artifact-free routing transitions with optimized timing

### 📊 Implementation Summary
**Total Estimated Time**: 28-38 hours (Phases 1, 1.5, 4 complete: 14 hours completed, 14-24 hours remaining)
**Current Progress**: Phase 1 ✅, Phase 1.5 ✅, Phase 4 ✅, Phase 2 ready to start
**Risk Level**: Medium (DSP architecture changes require careful testing)
**Testing Strategy**: VST3 validation + professional audio testing after each phase

**Major Achievements This Session**:
- ✅ **Hierarchical Dry/Wet System**: Complete G-MIX, D-MIX, C-MIX implementation
- ✅ **Mix Control Responsiveness**: Fixed non-responsive controls across routing modes
- ✅ **Serial Routing Signal Leakage**: Eliminated unexpected signal paths
- ✅ **Professional Audio Standards**: Industry-standard signal flow behavior
- ✅ **Code Signing Integrity**: Maintained throughout development process

**Next Priority**: Phase 2 - Intelligent Comb SIZE/DIV Knob (6-8 hours estimated)

### 🎛️ GUI & User Experience Issues
*Additional UX issues to be added as discovered*

### 🔊 Audio Processing Issues
*Additional audio processing issues to be added as discovered*

### 📊 Performance & Optimization Issues
*Additional performance issues to be added as discovered*

---

## Issue Template

When adding new issues, use this format:

```markdown
### Issue Title
**Priority**: High/Medium/Low
**Component**: GUI/DSP/VST3/Build
**Description**: Brief description of the problem
**Steps to Reproduce**:
1. Step one
2. Step two
**Expected Behavior**: What should happen
**Actual Behavior**: What actually happens
**Notes**: Additional context or technical details
```

---

## Resolved Issues (Moved to PROGRESS.md)
*All resolved issues have been documented in PROGRESS.md. This section is kept for reference to the proper documentation location.*

---

*This tracker focuses on immediate, actionable issues. Completed work and development phases belong in PROGRESS.md.*