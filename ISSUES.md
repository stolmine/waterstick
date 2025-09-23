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

### 🎯 Primary Focus: Advanced Feature Development
**Priority**: High
**Component**: DSP/Audio Processing

**Feature-Driven Development Priorities:**

#### 1. Granular Pitch Shifting ✅ COMPLETED
**Priority**: Completed
**Component**: DSP/Audio Processing
**Description**: Implemented real-time pitch shifting for each delay tap
**Completed Goals**:
- ✅ Per-tap pitch control with semitone precision (-12 to +12 semitones)
- ✅ Real-time pitch shifting without artifacts using 4-grain overlap system
- ✅ Integration with existing filter and level systems
- ✅ Musical pitch relationships with ER-301-quality smoothness
- ✅ Automatic dry/wet bypass (zero pitch = bypass, any shift = 100% wet)
- ✅ 16 new VST3 parameters with full automation support

**Status**: Core DSP implementation complete. GUI integration planned for future branch.

#### 2. Randomization System
**Priority**: High
**Component**: Parameter Control/Modulation
**Description**: Comprehensive randomization system for tap parameters
**Goals**:
- Intelligent parameter randomization with musical constraints
- Per-context randomization (volume, pan, filter, timing)
- Probability-based parameter variation
- Undo/redo for randomization operations

#### 3. Macro Control System
**Priority**: High
**Component**: Parameter Control/Automation
**Description**: Advanced macro control system for complex parameter relationships
**Goals**:
- Multi-parameter macro assignments
- Morphing between parameter states
- Performance-oriented macro controls
- Integration with existing parameter automation

#### 4. Tap Selection for Feedback Routing
**Priority**: Medium
**Component**: DSP/Signal Processing
**Description**: Selective feedback routing from specific delay taps
**Goals**:
- Individual tap feedback send controls
- Selectable feedback source taps (which taps feed back to input)
- Variable feedback amounts per tap
- Complex feedback routing patterns for enhanced delay textures

### 📊 Global DSP Enhancement
**Priority**: Medium
**Component**: Audio Processing/Performance

**Tasks:**
1. Investigate advanced tap distribution algorithms
2. Implement intelligent time scaling with multiple division strategies
3. Optimize parameter smoothing across all global controls
4. Add enhanced modulation capabilities for tap parameters
5. Develop CPU-efficient processing techniques

**Expected Outcome**: Advanced delay processing with minimal computational overhead

### 🌟 Next Development Priorities
1. Advanced tap distribution pattern implementation
2. Enhanced visual feedback for delay parameters
3. Performance optimization for high tap count scenarios
4. Develop advanced parameter automation features
5. Implement professional preset management system

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
- Phase 4.0: Granular Pitch Shifting Implementation
  - Semitone-snapped pitch shifting (-12 to +12 semitones)
  - 4-grain overlap system with ER-301-quality smoothness
  - Automatic dry/wet bypass for zero CPU overhead
  - 16 new VST3 parameters with full automation support
- Phase 3.12: GUI Finalization and Interaction Enhancements
  - Contextual Minimap Filter Type Display
  - Double-Click to Default Functionality
- Phase 1: C-GAIN Parameter Implementation
- Phase 1.5: D-GAIN Parameter Implementation
- Phase 4: Hierarchical Dry/Wet Controls
- Phase 4.5: Delay Parameter Propagation
- Minimap Layout Implementation
- Uniform Spacing Corrections
- Code Signing and AMFI Validation Fixes
- Plugin Loading Issues Resolution
- Clean Installation Process Establishment

See PROGRESS.md Phase 4.0 for complete technical details.

### Code Signing & Distribution Challenges
**Priority**: High
**Component**: Build/Distribution

**Key Challenges**:
1. AMFI (Apple Mobile File Integrity) Validation Failures
2. CMS Blob Missing Errors
3. Adhoc Signing Rejection (Error -423)

**Implemented Solutions**:
- Custom entitlements for audio plugins
- Refined CMake configuration for code signing
- Improved build process for macOS compatibility
- Established plugin cache clearing procedures for DAW testing

---

*This tracker focuses on immediate, actionable issues. Completed work and development phases belong in PROGRESS.md.*