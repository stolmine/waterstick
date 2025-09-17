# Claude.md - Rainmaker VST3 Clone Development Instructions

## Project Overview
This project is a VST3 clone of the Intellijel Rainmaker Eurorack module - a sophisticated dual-processor audio effects unit combining 16-tap spectral delay with 64-tap comb resonator. The goal is to faithfully recreate both the functionality and visual design of the hardware module.

## Version Control
- **ALWAYS COMMIT BEFORE MAJOR REFACTORING**: Do a git checkin before doing any major refactor or rewrite
- Commit working changes incrementally before attempting large changes
- This ensures we can always roll back if something goes wrong

## File Naming Convention
- **ONLY USE ONE FILENAME**: `WaterStick.vst3`
- Do NOT create multiple versions with different names (no _v2, _minimal, _test, etc.)
- Always overwrite the existing file when making updates
- This keeps the project clean and prevents confusion

## Implementation Guidelines
- **NO FALLBACK IMPLEMENTATIONS**: Build the actual functionality, not simplified versions
- When implementing features, create the real DSP processing, not placeholder objects
- If a feature requires complex DSP (like pitch shifting), implement it properly using Max/MSP objects
- Don't create "mock" or "simplified" versions for testing
- Always ensure we are in the repo's proper directory when running git commands, e.g.: '/Users/why/repos/waterstick'

- **USE SAMPLE-LEVEL PROCESSING**: Prefer sample-by-sample processing for smooth modulation
  - Enable fractional delays with smooth interpolation for zipper-free modulation
  - Process in small chunks (4 samples) for SIMD optimization when beneficial
  - Prioritize audio quality over computational efficiency for modulation smoothness

## Project Structure
- Main device file: `WaterStick.vst3`
- Documentation: `todo.md` (task tracking)
- Project instructions: `CLAUDE.md` (this file)

## UI Design Requirements

### Visual Approach
- **Skeuomorphic Design**: The interface should closely replicate the physical Rainmaker module's appearance
- **Panel Layout**: Black UI elements on white background with grey highlights, I'd like a clean and minimal aesthetic
- **Hardware Fidelity**: Knobs, switches, and LED indicators should visually match the original module
- **Future-Proofing**: While current design prioritizes hardware accuracy, code should be structured to allow easy UI refactoring in future versions

### Layout Specifications
- **Panel Dimensions**: Maintain the 36hp Eurorack module proportions
- **Section Organization**: Clear visual separation between Rhythm Delay and Comb sections
- **Control Spacing**: Match the physical spacing and grouping of the original module
- **Typography**: Use clean, technical font matching Eurorack module labeling standards

### Control Elements
- **Knobs**: Implement SIFAM-style knobs with appropriate scaling and visual feedback
- **Buttons**: Buttons should match the original's 
- **LEDs**: Bi-color LEDs for tempo sync, bypass status, and parameter feedback
- **Input/Output**: Visual representations of audio I/O connections
- **Parameter Labels**: All original parameter names and ranges preserved

## Feature Overview

### Core Capabilities
- 16-tap delay with independent control of each tap
- High-quality, sample-accurate processing
- Full VST3 parameter automation support
- Professional audio quality with low-latency performance

### Current Features
- Per-tap enable, volume, pan, and filter controls
- Multiple parameter navigation modes
- Global controls for sync, timing, input/output levels
- Sophisticated DSP with Three Sisters-quality filtering

### Planned Future Developments
- 64-tap Comb Resonator section
- Granular pitch shifting per tap
- Enhanced visual feedback and performance optimizations
- Preset system with morphing capabilities

## Technical Implementation Notes

### DSP Requirements
- Implement delay lines using circular buffers with interpolation
- Use state-variable filters for per-tap filtering
- Optimize granular synthesis for real-time performance
- Ensure sample-accurate parameter automation

### Code Organization
- Separate DSP processing from UI rendering
- Implement proper threading for real-time audio
- Use efficient memory management patterns
- Structure code for future UI refactoring flexibility

### Testing and Validation
- Comprehensive unit testing for DSP components
- Integration testing with multiple DAWs
- Performance profiling and optimization
- User acceptance testing with original hardware comparison

### Housekeeping
- After any build succeeds execute the following bash to move build to library: 'cp -R /Users/why/repos/waterstick/build/VST3/Release/WaterStick.vst3 /Library/Audio/Plug-Ins/VST3'

This specification ensures the VST3 clone captures both the sonic character and visual appeal of the original Rainmaker module while meeting professional plugin development standards.

## Development Phases

### Completed Phases
- **Phase 1**: Core VST3 Architecture & Timing System
  - High-quality delay line implementation
  - Tempo synchronization with 22 musical divisions

- **Phase 2**: Multi-Tap Distribution Engine
  - 16 independent tap positioning
  - Per-tap volume, pan, and filter controls
  - Professional audio quality with smooth parameter changes

### Future Phases
- **Phase 3**: Comb Resonator Section
  - 64-tap Karplus-Strong delay
  - Advanced string synthesis modes

- **Phase 4**: Advanced Features
  - Granular pitch shifting
  - Audio routing matrix
  - Comprehensive preset system

## GUI Development Roadmap

### Phase 1: Tap Control Interface ✅ COMPLETED
**Implementation Priority:**

1. **✅ Tap Enable/Disable Buttons**
   - Visual: 16 individual buttons in 2x8 grid with custom styling
   - Functionality: Toggle tap enable parameters (Tap 1-16 Enable)
   - Layout: Clean circular buttons with black stroke and fill states
   - Advanced Features: Click-drag toggle functionality for efficient pattern editing
   - Why first: Essential for basic tap control and user workflow

### Phase 1.6: Tap Menu System ✅ COMPLETED
**Contextual Parameter Navigation:**

1. **✅ Mode Button Foundation**
   - Visual: Custom ModeButton class with center dot styling
   - Position: Under column 1 with proper 1.5x spacing from tap grid
   - States: Unselected (black dot) and selected (white dot on black rounded rectangle)
   - Rectangle: 1.5x circle size (37.5px) with 8px rounded corners, black fill only
   - Color Inversion: Complete white circle stroke and white dot on black background
   - Technical: Expanded view bounds (38x38px) to prevent rectangle clipping
   - Foundation for contextual tap parameter control modes

2. **Global Control Interface** (NEXT PRIORITY)
   - Target Controls: Input Gain, Output Gain, Delay Time, Dry/Wet, Sync Mode, Sync Division, Grid
   - Exclude for now: Individual tap Level/Pan controls (more complex layout)
   - Visual: Knobs and switches matching Eurorack aesthetic
   - Functionality: Real-time parameter control with host automation
   - Why next: Provides complete basic functionality without overwhelming layout

### Phase 2: Tap Menu System Expansion ✅ COMPLETED
**Multi-Mode Parameter Navigation:**

1. **✅ Additional Mode Buttons**
   - Added mode buttons under columns 2-8 for different parameter contexts
   - Implemented mutual exclusion (only one mode selected at a time)
   - Applied visual consistency with first mode button design
   - Enhanced ModeButton class with radio button behavior

2. **✅ Volume Context Implementation**
   - Complete continuous volume control for all 16 taps
   - Professional circular fill visualization representing volume levels
   - Three interaction modes: click (absolute positioning), vertical drag (relative adjustment), horizontal drag (cross-tap painting)
   - Intelligent drag direction detection with 3-pixel threshold
   - Full VST parameter integration with tap Level parameters (kTap1Level-kTap16Level)
   - Real-time visual feedback with smooth circular fill rendering
   - Context-aware state management with automatic parameter save/load

3. **✅ Pan Context Implementation**
   - Complete continuous pan control for all 16 taps with 5px baseline visualization
   - Professional full-width rectangle with circular clipping and center reference
   - Three interaction modes: click (absolute positioning), vertical drag (relative adjustment), horizontal drag (cross-tap painting)
   - 5px minimum baseline height for all pan positions (center = baseline only, left/right expand beyond baseline)
   - Full VST parameter integration with tap Pan parameters (kTap1Pan-kTap16Pan)
   - Visual behavior: bottom half = left pan, top half = right pan, always maintaining center baseline
   - Context-aware state management with automatic parameter save/load

4. **Contextual Tap Indicators** (FUTURE)
   - Filter mode: Tap buttons show filter frequency/resonance
   - Pitch mode: Tap buttons show granular pitch shift values
   - Each mode changes tap button appearance and interaction

5. **Mode Labels and Identification** (FUTURE)
   - Text labels below mode buttons for clear identification
   - Icons or symbols for quick mode recognition
   - Extend rectangle design to accommodate labels

### Phase 3: Enhanced Tap Controls (FUTURE)
- Advanced per-tap parameter editing interfaces
- Real-time visual meters for tap activity
- Sophisticated parameter coupling and grouping

### Phase 4: Advanced GUI Features (FUTURE)
- Preset browser interface
- Real-time waveform display
- Enhanced visual feedback and animations

### Phase 2.1: Volume Context Implementation ✅ COMPLETED
**Professional Tap Volume Control:**

1. **✅ Contextual State Management Foundation**
   - TapContext enum system with Enable, Volume, Pan, Filter modes
   - Context-aware parameter ID resolution with getTapParameterIdForContext()
   - Automatic state save/load between VST parameters and button contexts
   - Complete backward compatibility with existing Enable context functionality

2. **✅ Continuous Volume Control**
   - Replace binary toggle with continuous 0.0-1.0 volume range
   - Professional circular fill visualization representing volume levels
   - Intelligent scaling curve preventing visual 100% until truly at maximum
   - Real-time visual feedback with smooth parameter updates

3. **✅ Advanced Mouse Interaction**
   - 3-pixel threshold for intelligent click vs drag detection
   - Click mode: Absolute positioning based on vertical position within circle
   - Vertical drag: Relative adjustment from starting point with 30-pixel sensitivity
   - Horizontal drag: Cross-tap volume painting with real-time target tracking

4. **✅ VST Parameter Integration**
   - Full integration with kTap1Level through kTap16Level parameters
   - Proper 2x8 grid mapping (taps 1-8 top row, taps 9-16 bottom row)
   - Context-aware valueChanged() routing to correct parameter IDs
   - Complete DAW automation support and project save/restore functionality

### Phase 2.2: Pan Context Implementation ✅ COMPLETED
**Professional Tap Pan Control:**

1. **✅ Continuous Pan Control with 5px Baseline**
   - Replace three-position toggle with continuous 0.0-1.0 pan range
   - Professional full-width rectangle visualization spanning circle diameter
   - 5px minimum baseline height for all pan positions as visual reference
   - Center position (0.5) shows exactly 5px baseline rectangle
   - Left/right positions expand beyond baseline toward respective halves

2. **✅ Advanced Visual Design**
   - Full-width rectangle clipped to circle bounds using x² + y² = r² equation
   - Visual mapping: bottom half = left pan (0.0), top half = right pan (1.0)
   - Always maintains 5px center baseline for intuitive reference
   - Smooth transitions with proper circular clipping throughout range

3. **✅ Volume-Style Mouse Interaction**
   - 3-pixel threshold for intelligent click vs drag detection
   - Click mode: Absolute positioning based on vertical position within circle
   - Vertical drag: Relative adjustment from starting point with 30-pixel sensitivity
   - Horizontal drag: Cross-tap pan painting with real-time target tracking

4. **✅ Complete VST Parameter Integration**
   - Full integration with kTap1Pan through kTap16Pan parameters
   - Proper 2x8 grid mapping consistent with Volume context
   - Context-aware valueChanged() routing to Pan parameter IDs
   - Complete DAW automation support and project save/restore functionality

### Phase 1.5: Audio Quality Enhancements ✅ COMPLETED
**Professional Tap Behavior:**

1. **✅ Automatic Buffer Clearing**
   - Clean buffer clearing when taps transition from enabled to disabled
   - Prevents residual audio from playing when taps are re-enabled
   - Tracks previous tap states for intelligent clearing logic

2. **✅ Exponential Fade-Out on Disable**
   - Proportional fade length: 1% of delay time (64-2048 samples)
   - Exponential curve: `exp(-6.0f * progress)` for natural ~60dB fade
   - Eliminates popping artifacts when disabling taps
   - GUI remains instantly responsive while audio fades smoothly

3. **✅ Exponential Fade-In on Enable**
   - Ultra-short fade length: 0.25% of delay time (16-512 samples)
   - Inverse exponential curve: `1.0 - exp(-6.0f * progress)`
   - Minimal audible delay (0.3ms-11.6ms) for immediate response feel
   - Prevents clicks and jarring engagement artifacts

### Current Architecture Status
The foundation provides:
- **Robust timing system** with complete multi-tap distribution
- **High-quality delay infrastructure** with 16 independent taps and crossfading
- **Per-tap filter system** with 32 Three Sisters-quality filters (16 taps × 2 channels)
- **Professional plugin standards** with full VST3 validation (103 parameters)
- **Crash-free GUI framework** using VSTGUIEditor for programmatic interface
- **Professional audio behavior** with smooth fade transitions and clean buffer management
- **Complete tap menu system** with 8 mode buttons and mutual exclusion for contextual parameter navigation
- **Professional volume context** with continuous control, circular fill visualization, and full VST parameter integration
- **Professional pan context** with 5px baseline, continuous stereo positioning, and Volume-style mouse interaction
- **Three-context tap control system** providing Enable, Volume, and Pan control for all 16 taps
- **Sophisticated spectral processing** with individual filter control per tap

## Future Vision

### Long-Term Development Goals
- Advanced parameter automation with musical timing awareness
- Real-time visual feedback and waveform display
- Enhanced spectral processing and tap interactions
- Performance optimizations through SIMD and multi-threading

### Ongoing Quality Improvements
- Continuous refinement of tap enable/disable behaviors
- Memory efficiency and processing speed enhancements
- Expanded modulation and routing capabilities

## Current Development Status (Updated: Phase 2.5 Global Controls Complete)

### ✅ Completed Core Features
The WaterStick VST3 plugin now provides a professional, production-ready foundation with:

#### **Multi-Tap Delay Engine**
- 16 independent delay taps with high-quality STK DelayA implementation
- Dual crossfading delay lines for zipper-free modulation
- Complete tempo synchronization with 22 musical divisions
- Tap distribution engine with rhythm patterns

#### **Professional GUI Interface**
- **Enable Context (Mode 1)**: Binary tap on/off control with drag functionality
- **Volume Context (Mode 2)**: Continuous level control with circular fill visualization
- **Pan Context (Mode 3)**: Continuous stereo positioning with 5px baseline reference
- **Filter Cutoff Context (Mode 4)**: Per-tap filter frequency control with logarithmic scaling
- **Filter Resonance Context (Mode 5)**: Per-tap filter resonance control with cubic curve scaling
- **Five-Mode Mouse Interaction**: Click (absolute), vertical drag (relative), horizontal drag (cross-tap)
- **Complete VST Integration**: Full DAW automation and project save/restore

#### **Audio Quality Standards**
- Professional fade-in/out curves preventing audio artifacts
- Automatic buffer clearing for clean tap transitions
- Sample-accurate parameter automation
- VST3 validation compliance (103 parameters, 47 tests passed)

### Phase 2.3: Per-Tap Filter Implementation ✅ COMPLETED
**Transition from Global to Per-Tap Filtering:**

1. **✅ Per-Tap Filter Architecture**
   - 32 ThreeSistersFilter instances (16 taps × 2 channels)
   - Individual control of cutoff, resonance, and filter type per tap
   - Cascaded dual SVF architecture maintained for each tap
   - Complete removal of global filter processing for cleaner signal path

2. **✅ Expanded Parameter System**
   - 48 new filter parameters (16 taps × 3 parameters each)
   - Same logarithmic frequency scaling (20Hz-20kHz) per tap
   - Same cubic curve resonance scaling per tap
   - Individual filter type selection per tap (Low Pass, High Pass, Band Pass, Notch)

3. **✅ Audio Processing Integration**
   - Per-tap filters applied after delay and level but before panning
   - Maintains Three Sisters quality for each individual tap
   - Sample-accurate parameter automation for all tap filters
   - Professional fade transitions preserved during tap state changes

4. **✅ VST3 Architecture Updates**
   - Dynamic parameter handling for 48 new filter parameters
   - Efficient state save/load for per-tap filter settings
   - Full VST3 validation compliance (103 total parameters)
   - Backward-compatible parameter indexing maintained

### Phase 2.4: Filter Context Implementation ✅ COMPLETED
**Professional Per-Tap Filter Control:**

1. **✅ Filter Cutoff Context (Mode 4)**
   - Complete per-tap filter cutoff frequency control using volume GUI visualization
   - Professional circular fill visualization representing cutoff frequency levels
   - Logarithmic frequency scaling (20Hz-20kHz) matching commit e5eb25c parameters
   - Full VST parameter integration with kTap1FilterCutoff through kTap16FilterCutoff
   - Three interaction modes: click (absolute positioning), vertical drag (relative adjustment), horizontal drag (cross-tap painting)

2. **✅ Filter Resonance Context (Mode 5)**
   - Complete per-tap filter resonance control using pan GUI visualization
   - Professional rectangle with 5px baseline representing resonance levels
   - Resonance scaling (-1.0 to +1.0) with cubic curve for positive values matching commit e5eb25c
   - Full VST parameter integration with kTap1FilterResonance through kTap16FilterResonance
   - Visual mapping: bottom half = low resonance, center = moderate, top half = high resonance

3. **✅ Context System Enhancement**
   - Extended TapContext enum with FilterCutoff and FilterResonance contexts
   - Updated getTapParameterIdForContext() to route to proper per-tap filter parameters
   - Enhanced createTapButtons() to load filter context values automatically
   - Complete context switching with parameter save/load functionality

4. **✅ Mouse Interaction Integration**
   - Extended all mouse event handlers (onMouseDown, onMouseMoved, onMouseUp) for filter contexts
   - Maintained consistent interaction paradigms with existing Volume and Pan contexts
   - Professional drag direction detection and cross-tap painting support
   - Sample-accurate parameter automation for all filter controls

### 🔄 Next Development Priorities
1. **Expand Filter Capabilities**
   - Filter Type Context (Mode 6): GUI interface for per-tap filter type selection
   - Advanced per-tap filter routing and chaining
2. **Enhanced Visual Feedback**
   - Real-time tap activity meters
   - Visualization of delay times and tap positions
3. **Performance Optimizations**
   - SIMD optimization for multi-tap processing
   - Memory efficiency improvements

### 📊 Development Progress
- **Phase 1-2.5**: Core VST3 Foundation ✅ 100% Complete
  - Full tap distribution engine
  - Per-tap enable, volume, pan, and filter controls
  - Global control interface
  - VST3 validation passed (47/47 tests)
- **Phase 3**: Comb Resonator Section 🔄 In Planning
- **Phase 4**: Advanced Features 🔄 In Planning

The WaterStick VST3 plugin now provides a professional, production-ready delay effect with comprehensive per-tap control, full automation support, and a modular, extensible design. Global controls for sync, time, grid, input, output, and dry/wet are fully implemented, maintaining a clean and intuitive user experience.