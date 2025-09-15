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

### Phase 2: Multi-Tap Distribution Engine (NEXT)
**Recommended Implementation Order:**

1. **Tap Distribution Engine**
   - Why first: Defines where the 16 taps sit in time
   - Implement rhythm patterns (uniform, swing, custom)
   - Calculate tap delay times from tempo and pattern
   - This turns one delay into 16 positioned delays

2. **Multi-Tap Architecture**
   - Why second: Scales existing delay to 16 independent taps
   - Extend DualDelayLine to support multiple read heads
   - Each tap reads from same buffer at different positions
   - Maintains excellent crossfading system

3. **Per-Tap Processing Chain**
   - Why third: Adds spectral processing per tap
   - State-variable filters (utilize Three Sisters knowledge)
   - Level and pan controls
   - Individual mute/solo states

4. **Granular Pitch Shifting**
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

### Current Architecture Status
The foundation provides:
- **Robust timing system** ready for multi-tap distribution
- **High-quality delay infrastructure** that can be extended to multiple taps
- **Parameter safety** with backward-compatible enum extensions
- **Professional plugin standards** with full VST3 validation