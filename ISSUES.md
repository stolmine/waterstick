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

**Active Issues Requiring Implementation**:
1. **Buffer State Issues**: Routing mode changes don't clear processor buffers, causing audio artifacts
2. **Comb Parameter Smoothing**: Parameters cause clicks/pops in comb section

**Implementation Recommendations**:
1. Implement section-specific feedback routing
2. Add buffer clearing to routing mode transitions
3. Introduce dynamic scaling for parallel processing
4. Add user-controllable parallel mix balance
5. Fine-tune routing transition timing (current 10ms)
6. Implement comb parameter smoothing with 5-10ms time constants

**Status**: Investigation Complete ✓

## Active Implementation Plan

### ✅ Completed Phases (Moved to PROGRESS.md)
- Phase 1: C-GAIN Parameter ✅
- Phase 1.5: D-GAIN Parameter ✅
- Phase 4: Hierarchical Dry/Wet Controls ✅
- Phase 4.5: Delay Parameter Propagation ✅

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


### 🔄 Phase 4.6: Comb Parameter Smoothing (HIGH PRIORITY)
**Priority**: High
**Component**: DSP/Audio Processing
**Estimated Time**: 4-6 hours

**Issues Identified:**
1. **Abrupt Parameter Updates**: Comb parameters change instantly without smoothing, causing clicks/pops
2. **Control Rate Updates**: Parameters only updated when VST changes occur, not at audio rate
3. **Missing Professional Standards**: No 5-10ms parameter smoothing for real-time automation
4. **Separate from Delay Issue**: Comb section needs its own parameter smoothing implementation

**Tasks:**
1. Implement sample-rate parameter smoothing for all comb controls (SIZE, PITCH, FEEDBACK, TAPS)
2. Add exponential interpolation with 5-10ms time constants
3. Update parameters at audio rate in processStereo() method
4. Add crossfading for large parameter changes to prevent buffer index jumps
5. Enhanced tanh limiting for parameter change transients

**Expected Outcome**: Smooth, click-free comb parameter automation with professional audio standards

### 📊 Active Implementation Summary
**Remaining Estimated Time**: 16-26 hours
**Current Progress**: 4 major phases completed, moved to PROGRESS.md
**Risk Level**: Medium (DSP architecture changes require careful testing)
**Testing Strategy**: VST3 validation + professional audio testing after each phase

**Current Priority**: Phase 4.6 - Comb Parameter Smoothing (4-6 hours estimated)
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

## Resolved Issues ✅
**Location**: All completed issues and phases have been moved to PROGRESS.md for historical tracking.

**Recent Completions**:
- Phase 1: C-GAIN Parameter Implementation
- Phase 1.5: D-GAIN Parameter Implementation
- Phase 4: Hierarchical Dry/Wet Controls
- Phase 4.5: Delay Parameter Propagation

See PROGRESS.md Phase 3.10 for complete technical details.

---

*This tracker focuses on immediate, actionable issues. Completed work and development phases belong in PROGRESS.md.*