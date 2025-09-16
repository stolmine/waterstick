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

## Complete Feature Implementation Requirements

### Core Architecture
- **Dual Processing Engine**: Independent Rhythm Delay and Comb sections with flexible routing
- **High Quality Processing**: 32-bit internal processing, oversampling capabilities
- **Low Latency**: Optimized for real-time performance in DAW environments

### Rhythm Delay Section (16-Tap Spectral Delay)

#### Delay Line Features
- **16 Independent Taps**: Each with individual delay time, granular pitch shifting, level, and stereo positioning
- **Maximum Delay Time**: Up to 20+ seconds per tap
- **Tap Distribution**: Multiple rhythm patterns and custom tap spacing
- **Feedback Control**: Independent feedback amount and routing options

#### Per-Tap Processing
- **State Variable Filters**: 2nd-order filters per tap (Low Pass, High Pass, Band Pass, Notch)
- **Granular Pitch Shifting**: Real-time pitch manipulation with grain controls
- **Level Control**: Independent amplitude control per tap
- **Pan Position**: Stereo field positioning for each tap
- **Mute/Solo**: Individual tap bypass and isolation

#### Groove System
- **Rhythm Templates**: Pre-programmed groove patterns
- **Swing Amount**: Adjustable timing deviation for humanization
- **Accent Control**: Dynamic emphasis on specific taps
- **Custom Patterns**: User-definable tap timing relationships

### Comb Resonator Section (64-Tap Karplus-Strong)

#### Comb Architecture
- **64 Variable Taps**: Dense comb filtering with adjustable tap density, size, and feedback control
- **String Synthesis**: Karplus-Strong algorithm implementation
- **Multiple Modes**: Guitar, Sitar, Clarinet, and Raw synthesis modes
- **Pitch Control**: Fundamental frequency and harmonic content adjustment

#### Tap Distribution Patterns
- **Uniform**: Even spacing across the delay line
- **Fibonacci**: Mathematical sequence-based tap placement
- **Early**: Emphasis on early reflections
- **Late**: Emphasis on late reflections
- **Custom**: User-definable tap positioning

#### Resonance Controls
- **Damping**: High-frequency attenuation simulation
- **Dispersion**: All-pass filter chains for realistic string behavior
- **Nonlinearity**: Saturation and compression effects
- **Excitation**: Initial impulse shaping and character

### Granular Processing Engine

#### Grain Parameters
- **Grain Size**: Variable from 5ms to 671ms
- **Overlap Control**: Grain density and overlap management
- **Window Functions**: Multiple grain envelope shapes
- **Pitch Shifting**: Real-time transpose without time stretching

#### Quality Controls
- **Anti-Aliasing**: Oversampling for pitch shift operations
- **Interpolation**: High-quality fractional delay algorithms
- **Buffer Management**: Efficient memory usage for long delays

### Global Controls and Routing

#### Audio Routing Matrix
- **Delay → Comb**: Serial processing chain
- **Comb → Delay**: Reverse serial chain
- **Parallel**: Independent dual processing

#### Master Controls
- **Input Gain**: Pre-processing level adjustment
- **Output Level**: Master output control
- **Mix Control**: Dry/wet balance
- **Bypass**: True bypass implementation

#### Modulation System
- **Parameter Automation**: Full DAW automation support
- **MIDI Control**: Assignable MIDI CC mapping
- **Preset Management**: Save/recall functionality
- **Real-time Control**: Smooth parameter interpolation

### Tempo and Timing

#### Clock Sources
- **Host Sync**: DAW tempo synchronization
- **Internal Clock**: Independent timing reference
- **Tap Tempo**: Manual tempo input
- **External Sync**: MIDI clock input

#### Timing Resolution
- **Note Values**: From 1/64 notes to whole notes
- **Triplets**: Full triplet subdivision support
- **Dotted Notes**: Extended note value options
- **Free Running**: Non-quantized delay times

### Advanced Features

#### Preset System
- **Factory Presets**: Recreation of original module presets
- **User Presets**: Custom setting storage and recall
- **Preset Morphing**: Smooth transitions between settings
- **Import/Export**: Preset sharing capabilities

#### Performance Features
- **Low CPU Usage**: Optimized algorithms for efficiency
- **Variable Buffer Sizes**: Adaptable to different audio interfaces
- **Multi-Channel**: Stereo and mono operation modes
- **Freeze Function**: Infinite sustain capability

#### Quality Assurance
- **Plugin Validation**: Pass all standard plugin tests
- **DAW Compatibility**: Tested across major DAWs (Ableton Live, Pro Tools, Logic, etc.)
- **Cross-Platform**: Windows, macOS, and Linux support
- **Format Support**: VST3 primary, with potential AU and AAX versions

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

## Implementation Progress & Development Roadmap

### Phase 1: Foundation & Timing System ✅ COMPLETED
- **✅ Basic VST3 Architecture**: Core plugin framework with parameter system
- **✅ High-Quality Delay Line**: STK DelayA implementation with Thiran allpass interpolation
- **✅ Zipper-Free Modulation**: Dual crossfading delay lines for smooth parameter changes
- **✅ Tempo Synchronization**: Complete host tempo sync system with 22 musical divisions
  - Free-running and synced modes with binary toggle
  - Divisions from 1/64 to 8 bars including triplets and dotted notes
  - Real-time host tempo integration with continuous operation
  - Custom parameter display formatting for musical divisions

### Phase 2: Multi-Tap Distribution Engine ✅ COMPLETED
**Recommended Implementation Order:**

1. **✅ Tap Distribution Engine**
   - Why first: Defines where the 16 taps sit in time
   - Implement rhythm patterns (uniform, swing, custom)
   - Calculate tap delay times from tempo and pattern
   - This turns one delay into 16 positioned delays

2. **✅ Multi-Tap Architecture**
   - Why second: Scales existing delay to 16 independent taps
   - Extend DualDelayLine to support multiple read heads
   - Each tap reads from same buffer at different positions
   - Maintains excellent crossfading system

3. **Per-Tap Processing Chain** (IN PROGRESS)
   - Why third: Adds spectral processing per tap
   - State-variable filters (utilize Three Sisters knowledge)
   - Level and pan controls
   - Individual mute/solo states

4. **Granular Pitch Shifting** (FUTURE)
   - Why last: Most complex, builds on everything else
   - Add grain-based pitch shifting per tap
   - Can start with simple pitch shift, refine later

### Phase 3: Comb Resonator Section (FUTURE)
- 64-tap Karplus-Strong implementation
- String synthesis modes and resonance controls
- Tap distribution patterns (uniform, fibonacci, early/late)

### Phase 4: Advanced Features (FUTURE)
- Audio routing matrix between delay and comb sections
- Preset system and factory presets
- Enhanced modulation capabilities
- Performance optimizations

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

3. **Contextual Tap Indicators** (FUTURE)
   - Pan mode: Tap buttons show stereo positioning
   - Filter mode: Tap buttons show filter frequency/resonance
   - Pitch mode: Tap buttons show granular pitch shift values
   - Each mode changes tap button appearance and interaction

4. **Mode Labels and Identification** (FUTURE)
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
- **Robust timing system** ready for multi-tap distribution
- **High-quality delay infrastructure** that can be extended to multiple taps
- **Parameter safety** with backward-compatible enum extensions
- **Professional plugin standards** with full VST3 validation
- **Crash-free GUI framework** using VSTGUIEditor for programmatic interface
- **Professional audio behavior** with smooth fade transitions and clean buffer management
- **Complete tap menu system** with 8 mode buttons and mutual exclusion for contextual parameter navigation
- **Professional volume context** with continuous control, circular fill visualization, and full VST parameter integration

## Stretch Goals (Future Iterations)

These are advanced features for much later development phases:

### Audio Quality Refinements
1. **Fine-tune fade-in/out times on tap enable/disable**
   - Current implementation works well, but could be optimized further
   - Consider user-adjustable fade curves or adaptive timing
   - Potentially add different fade algorithms for different musical contexts

### Advanced GUI Features
2. **Sophisticated parameter automation curves**
   - Bezier curve automation for smooth parameter changes
   - Musical timing-aware automation (quantized to beat divisions)

3. **Visual feedback enhancements**
   - Real-time audio waveform display
   - Tap activity meters with peak hold
   - Visual representation of delay times and tap positions

### DSP Enhancements
4. **Advanced spectral processing per tap**
   - Individual EQ sections per tap
   - Granular pitch shifting implementation
   - Advanced filtering options

5. **Performance optimizations**
   - SIMD optimization for multi-tap processing
   - Efficient memory management for long delay times
   - Multi-threading for complex processing chains