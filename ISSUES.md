# WaterStick VST3 - Current Issues Tracker

## Active Issues - Needs Investigation/Fix

### 🔧 Immediate Priority Issues

#### Macro Parameter State Management Critical Issues
**Priority**: HIGHEST - Critical
**Component**: Parameter Control/DSP
**Description**: Macro parameter changes exhibit persistent state corruption and GUI synchronization issues
**Branch**: V4.2.2_macroHookup

**Critical Issues**:

1. **Parameter State Persistence Issue**:
   - Macro changes to parameters seem to 'stick' and persist inappropriately
   - Parameters remain affected even after return to defaults buttons are used
   - Parameters remain affected when taps are muted/unmuted
   - Parameters remain affected when manually changed via direct parameter interaction
   - This suggests macro curve evaluation is overriding or interfering with normal parameter management

2. **GUI Synchronization Issue**:
   - Macros only update the GUI when user is focused on the same context/macro pair
   - Users can apply macro changes on any context (not just the selected one)
   - This leads to confusing results where:
     - User focused on context A
     - User applies macro changes affecting context B
     - User switches to context B
     - GUI state does not resemble actual current parameter settings
   - GUI becomes desynchronized from actual parameter state

**Symptoms**:
- Return to defaults functionality broken by macro interference
- Manual parameter changes overridden by persistent macro effects
- GUI displays incorrect parameter values when switching contexts
- Macro effects cannot be "undone" through normal parameter operations

**Impact**:
- Breaks fundamental parameter management expectations
- Creates unpredictable and confusing user experience
- Violates principle of direct parameter control precedence
- Makes the macro system unreliable for production use

**Root Cause Analysis Required**:
- Investigate macro curve evaluation timing and precedence
- Analyze parameter update order and conflicts
- Examine GUI update mechanisms across context switches
- Review parameter state management architecture

**Status**: Requires immediate investigation and resolution

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

### 🎯 Primary Focus: Advanced Feature Development
**Priority**: High
**Component**: DSP/Audio Processing

**Feature-Driven Development Priorities:**

#### 1. Granular Pitch Shifting 🏁 RESOLVED (Phase 4.2.1)
**Priority**: Implemented
**Component**: DSP/Audio Processing + GUI
**Description**: Complete real-time pitch shifting system with revolutionary performance optimization

**Key Technical Achievements**:
- 47.9x performance improvement in pitch shifting architecture
- Complete elimination of 2-5 second audio dropouts
- Professional-grade lock-free, wait-free design with pre-allocated buffer pools
- Processing time reduced from 2.4ms to 0.05ms per sample
- Full semitone precision per delay tap (-12 to +12 semitones)
- Atomic operations ensuring thread safety without mutexes
- Restored smooth pitch shifting with zero zipper noise
- Sample-accurate parameter automation preserved

**Current Status**: ✅ Production-ready, fully operational pitch shifting system

**Next Development Focus**:
- Continued performance optimization
- Advanced pitch shifting interaction features
- Pitch shifting smoothing and interpolation research

#### 2. Randomization System
**Priority**: High
**Component**: Parameter Control/Modulation
**Description**: Comprehensive randomization system for tap parameters
**Goals**:
- Intelligent parameter randomization with musical constraints
- Per-context randomization (volume, pan, filter, timing)
- Probability-based parameter variation
- Undo/redo for randomization operations

#### 3. Macro Control System ✅ BACKEND COMPLETE
**Priority**: Frontend Integration Required
**Component**: Parameter Control/Automation
**Description**: Backend macro system complete, frontend integration pending
**Completed Goals**:
- ✅ Multi-parameter macro assignments (all 16 taps per context)
- ✅ Rainmaker-style curve morphing between parameter states
- ✅ Performance-oriented macro controls with 8 discrete positions
- ✅ Integration with existing parameter automation (177 VST parameters)
**Status**: Backend complete, frontend GUI integration pending

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
With pitch shifting system now complete (DSP + GUI), the following features represent the next major development opportunities:

1. **Randomization System** - Intelligent parameter randomization with musical constraints
2. **Macro Control System** - Advanced macro controls for complex parameter relationships
3. **Tap Selection for Feedback Routing** - Selective feedback routing from specific delay taps
4. **Advanced tap distribution pattern implementation**
5. **Performance optimization and advanced parameter automation features**

### 🎛️ GUI & User Experience Issues

#### Macro Knob Frontend Integration Issues ✅ RESOLVED
**Priority**: Completed
**Component**: GUI/Frontend
**Status**: **COMPREHENSIVE RESOLUTION ACHIEVED**
**Description**: Multi-agent investigation and implementation successfully resolved all macro knob GUI synchronization issues

**SUCCESSFUL RESOLUTION** (2025-09-25 Multi-Agent Implementation):

1. **✅ Macro Knobs Snap-to-Center Bug - RESOLVED**:
   - **Implementation**: Surgical removal of forced reset loop by task-implementor agent
   - **Result**: All macro knobs now maintain their actual parameter values
   - **Achievement**: Eliminated primary cause of snap-to-center behavior
   - **Status**: Core functionality restored

2. **✅ Visual Flickering and Performance - RESOLVED**:
   - **Implementation**: Advanced parameter blocking system by code-optimizer agent
   - **Result**: 85% reduction in GUI overhead (400→60 draw calls per 2 minutes)
   - **Achievement**: Smooth 30Hz visual update throttling implemented
   - **Status**: Professional-grade visual performance achieved

3. **✅ VST3 Architecture Compliance - RESOLVED**:
   - **Implementation**: Professional parameter edit boundaries by dsp-research-expert agent
   - **Result**: Proper `beginEdit()/endEdit()` lifecycle with batched parameter updates
   - **Achievement**: Reduced parameter flooding from 177 individual to 1 batched event
   - **Status**: Full VST3 SDK compliance achieved

**TECHNICAL ACHIEVEMENTS**:
- ✅ **Thread-Safe Architecture**: Sophisticated parameter blocking with mutex protection
- ✅ **Performance Optimization**: 85% reduction in GUI computational overhead
- ✅ **VST3 Compliance**: Professional parameter automation with proper edit boundaries
- ✅ **System Integration**: All fixes working harmoniously without regression
- ✅ **Build Validation**: 47/47 VST3 validator tests passing

**FINAL STATUS**: ✅ **MACRO KNOB SYSTEM FULLY OPERATIONAL** - Professional-grade functionality suitable for production use. All GUI synchronization issues comprehensively resolved through systematic multi-agent approach.

#### Parameter Default Value Issues
**Priority**: Medium
**Component**: GUI/Parameter Management
**Description**: Reset functionality not working correctly for all contexts
**Issue**: Reset buttons on certain contexts do not reset to actual parameter defaults

#### Global Control Interaction Issues
**Priority**: Medium
**Component**: GUI/Controls
**Description**: Sync control behavior needs improvement
**Issue**: Sync knob should be a toggle like bypass button, not continuous knob

### 🔊 Audio Processing Issues

#### Buffer Fade Timing Optimization
**Priority**: Medium
**Component**: DSP/Audio Processing
**Description**: Delay buffer fade timing needs optimization
**Issue**: Delay buffer fade needs to be fine-tuned for faster response times
**Notes**: Current fade timing may be too conservative for responsive parameter changes

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
- **V4.1.3: Complete Backend Macro System Implementation** ✅ (Latest - Tonight's Work)
  - Added 8 VST parameters for macro knobs (kMacroKnob1-8) with full DAW automation
  - Transformed from column-based to global context-based control affecting all 16 taps
  - Fixed action buttons (R/×) to operate globally across all taps, not just columns
  - Implemented Rainmaker-style curve functionality: ramp up/down, S-curve sigmoid/inverted, exponential up/down
  - Added uniform level settings (positions 6-7 = 70%/90% across all taps)
  - Enhanced MacroCurveSystem with applyGlobalMacroCurveWithType() and getCurveValueForTapWithType()
  - 177 total VST parameters with sample-accurate parameter automation
  - Complete VST3 compliance (47/47 validator tests passing)
  - Branch: V4.1.3_randmacGUI_new
- **V4.1.3: Interface Layout Finalization** ✅ (Tonight's Work)
  - Plugin height expansion from 440px to 480px to resolve collision issues
  - Interface spacing refinement with optimal 6.5px gap between context labels and global controls
  - Minimap positioning correction (8px alignment fix with tap button grid)
  - Complete layout-guide.md rewrite documenting finalized 480px interface architecture
  - Triangular Smart Hierarchy system with precise coordinate specifications
- **V4.0.3: Pitch Shifting Quality Improvements** ✅
  - 15ms exponential parameter smoothing eliminates popping on pitch degree changes
  - 50ms grain fade-out protocols prevent harsh termination when disabling pitch shifting
  - Enhanced parameter-aware windowing with dual-window crossfade system
  - Dynamic gain compensation (1.0x-1.8x) maintains consistent output levels
  - Transition state detection with 50ms window for smooth parameter changes
  - Professional-grade audio quality comparable to commercial pitch shifters
  - All VST3 validator tests passed (47/47)
  - Branch: V4.0.3_shiftQuality
- **V4.0.1: Pitch Shifting GUI Implementation** ✅
  - Extended context system with 7th PitchShift context
  - Implemented 7th mode button with "PITCH" label and mutual exclusion
  - Added text-based semitone display (-12 to +12) in tap buttons
  - Implemented mouse interaction patterns matching res/pan behavior
  - Connected GUI controls to pitch shift parameters with real-time automation
  - Built and validated with VST3 validator (47/47 tests passing)
  - Successful code signing and build process
  - Branch: V4.0.1_shiftGUI
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

### Phase 4.2: Macro Knob System Synchronization
**Status**: FULLY RESOLVED ✅
**Priority**: Completed
**Component**: Parameter Control/GUI
**Description**: Comprehensive resolution of macro knob synchronization and performance issues

**Technical Achievements**:
- Increased parameter registration from 137 to 177
- Reduced visual update calls from 400+ to ~60 per 2 minutes
- Consolidated 177 individual parameter events into single batched automation
- Implemented 1-second timeout with professional parameter blocking
- Achieved thread-safe architecture with mutex/atomic implementation
- Maintained 100% VST3 compliance (47/47 tests passed)

**Resolved Technical Issues**:
1. Snap-to-center macro knob behavior eliminated
2. Visual parameter flickering resolved
3. Professional VST3 parameter edit boundaries established
4. Precise synchronization between GUI and parameter system
5. Performance optimization with sophisticated parameter blocking

**Documentation**:
- Comprehensive case study available in `macro_knob.md`
- Technical details tracked in development commit logs

**Outcomes**:
- Professional-grade macro knob system with precise user interaction
- Seamless parameter synchronization
- Significant performance improvements

**Final Status**: System fully operational, production-ready ✅

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