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
- Documentation: `PROGRESS.md` (development progress tracking)
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

## Audio Quality Standards

### Professional Audio Processing
- Sample-accurate parameter automation with smooth transitions
- Exponential fade curves for tap enable/disable to prevent audio artifacts
- Automatic buffer clearing for clean state management
- High-quality interpolation for fractional delay lines

### DSP Performance
- Prioritize audio quality over computational efficiency for modulation
- Use sample-level processing for smooth parameter changes
- Implement proper anti-aliasing and filtering techniques
- Maintain low-latency performance suitable for real-time use

### Filter Implementation
- Per-tap Three Sisters-quality filtering with individual control
- Support for multiple filter types (LP, HP, BP, Notch, Bypass)
- Logarithmic frequency scaling (20Hz-20kHz) for musical control
- Cubic curve resonance scaling for smooth control characteristics

### Feedback System
- Global feedback parameter with cubic scaling curve
- Tanh saturation limiting to prevent runaway feedback
- Sample-accurate feedback routing with dedicated buffers
- Professional control range with precision at lower values

## GUI Development Standards

### Contextual Interface Design
- 6-mode tap control system (Enable, Volume, Pan, Filter Cutoff/Resonance/Type)
- Mouse interaction paradigms: click (absolute), vertical drag (relative), horizontal drag (cross-tap painting)
- 3-pixel threshold for intelligent drag detection
- Visual feedback with appropriate scaling and representation

### Layout and Spacing
- Dynamic sizing for labels and controls to prevent clipping
- Equal spacing and edge alignment for professional appearance
- Center alignment around control elements regardless of dynamic sizing
- Consistent visual hierarchy and grouping

### Global Controls
- Professional knob design with dot indicators
- Real-time value display with formatted readouts
- Context-sensitive displays (e.g., time vs division based on sync mode)
- Complete VST parameter integration with automation support

## Development Workflow

### Code Quality
- Follow existing code patterns and conventions
- Maintain modular, extensible design for future enhancements
- Implement proper error handling and validation
- Use meaningful variable and function names

### Testing Protocol
- Build and validate after each significant change
- Run VST3 validator to ensure compliance (target: 47/47 tests passing)
- Test in multiple DAW environments
- Verify parameter automation and state persistence

### Documentation
- Update PROGRESS.md after completing significant features or phases
- Document any deviations from original design specifications
- Record performance optimizations and their impact
- Maintain clear commit messages with feature descriptions

This document establishes the behavioral conventions and technical standards for developing the WaterStick VST3 plugin while ensuring professional quality and maintainable code architecture.
- loud output style dsp-subagent, then read claude.md and progress.md