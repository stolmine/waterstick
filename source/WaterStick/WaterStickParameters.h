#pragma once

namespace WaterStick {

// Parameter IDs
enum {
    kInputGain = 0,
    kOutputGain,
    kDelayTime,
    kFeedback,               // Global feedback parameter
    kFeedbackDamping,        // High-frequency damping amount (0.0-1.0)
    kFeedbackDampingCutoff,  // Damping filter cutoff frequency (0.0-1.0, mapped to 20Hz-20kHz)
    kFeedbackPreEffects,     // Feedback routing: 0=post-effects, 1=pre-effects
    kFeedbackPolarityInvert, // Feedback polarity: 0=normal, 1=inverted
    // Tempo sync parameters - added at end to maintain compatibility
    kTempoSyncMode,      // Toggle: 0=Free, 1=Synced
    kSyncDivision,       // Sync division: 1/64 to 8 bars
    // Tap distribution parameters
    kGrid,               // Grid parameter: 1, 2, 3, 4, 6, 8, 12, 16 (Taps/Beat)
    // Per-tap parameters (16 taps)
    kTap1Enable,         // Tap 1 enable/disable
    kTap1Level,          // Tap 1 level
    kTap1Pan,            // Tap 1 pan position
    kTap2Enable,         // Tap 2 enable/disable
    kTap2Level,          // Tap 2 level
    kTap2Pan,            // Tap 2 pan position
    kTap3Enable,         // Tap 3 enable/disable
    kTap3Level,          // Tap 3 level
    kTap3Pan,            // Tap 3 pan position
    kTap4Enable,         // Tap 4 enable/disable
    kTap4Level,          // Tap 4 level
    kTap4Pan,            // Tap 4 pan position
    kTap5Enable,         // Tap 5 enable/disable
    kTap5Level,          // Tap 5 level
    kTap5Pan,            // Tap 5 pan position
    kTap6Enable,         // Tap 6 enable/disable
    kTap6Level,          // Tap 6 level
    kTap6Pan,            // Tap 6 pan position
    kTap7Enable,         // Tap 7 enable/disable
    kTap7Level,          // Tap 7 level
    kTap7Pan,            // Tap 7 pan position
    kTap8Enable,         // Tap 8 enable/disable
    kTap8Level,          // Tap 8 level
    kTap8Pan,            // Tap 8 pan position
    kTap9Enable,         // Tap 9 enable/disable
    kTap9Level,          // Tap 9 level
    kTap9Pan,            // Tap 9 pan position
    kTap10Enable,        // Tap 10 enable/disable
    kTap10Level,         // Tap 10 level
    kTap10Pan,           // Tap 10 pan position
    kTap11Enable,        // Tap 11 enable/disable
    kTap11Level,         // Tap 11 level
    kTap11Pan,           // Tap 11 pan position
    kTap12Enable,        // Tap 12 enable/disable
    kTap12Level,         // Tap 12 level
    kTap12Pan,           // Tap 12 pan position
    kTap13Enable,        // Tap 13 enable/disable
    kTap13Level,         // Tap 13 level
    kTap13Pan,           // Tap 13 pan position
    kTap14Enable,        // Tap 14 enable/disable
    kTap14Level,         // Tap 14 level
    kTap14Pan,           // Tap 14 pan position
    kTap15Enable,        // Tap 15 enable/disable
    kTap15Level,         // Tap 15 level
    kTap15Pan,           // Tap 15 pan position
    kTap16Enable,        // Tap 16 enable/disable
    kTap16Level,         // Tap 16 level
    kTap16Pan,           // Tap 16 pan position
    // Per-tap filter parameters (16 taps × 3 parameters each)
    kTap1FilterCutoff,   // Tap 1 filter cutoff frequency
    kTap1FilterResonance, // Tap 1 filter resonance (-1.0 to +1.0)
    kTap1FilterType,     // Tap 1 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap2FilterCutoff,   // Tap 2 filter cutoff frequency
    kTap2FilterResonance, // Tap 2 filter resonance (-1.0 to +1.0)
    kTap2FilterType,     // Tap 2 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap3FilterCutoff,   // Tap 3 filter cutoff frequency
    kTap3FilterResonance, // Tap 3 filter resonance (-1.0 to +1.0)
    kTap3FilterType,     // Tap 3 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap4FilterCutoff,   // Tap 4 filter cutoff frequency
    kTap4FilterResonance, // Tap 4 filter resonance (-1.0 to +1.0)
    kTap4FilterType,     // Tap 4 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap5FilterCutoff,   // Tap 5 filter cutoff frequency
    kTap5FilterResonance, // Tap 5 filter resonance (-1.0 to +1.0)
    kTap5FilterType,     // Tap 5 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap6FilterCutoff,   // Tap 6 filter cutoff frequency
    kTap6FilterResonance, // Tap 6 filter resonance (-1.0 to +1.0)
    kTap6FilterType,     // Tap 6 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap7FilterCutoff,   // Tap 7 filter cutoff frequency
    kTap7FilterResonance, // Tap 7 filter resonance (-1.0 to +1.0)
    kTap7FilterType,     // Tap 7 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap8FilterCutoff,   // Tap 8 filter cutoff frequency
    kTap8FilterResonance, // Tap 8 filter resonance (-1.0 to +1.0)
    kTap8FilterType,     // Tap 8 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap9FilterCutoff,   // Tap 9 filter cutoff frequency
    kTap9FilterResonance, // Tap 9 filter resonance (-1.0 to +1.0)
    kTap9FilterType,     // Tap 9 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap10FilterCutoff,  // Tap 10 filter cutoff frequency
    kTap10FilterResonance, // Tap 10 filter resonance (-1.0 to +1.0)
    kTap10FilterType,    // Tap 10 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap11FilterCutoff,  // Tap 11 filter cutoff frequency
    kTap11FilterResonance, // Tap 11 filter resonance (-1.0 to +1.0)
    kTap11FilterType,    // Tap 11 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap12FilterCutoff,  // Tap 12 filter cutoff frequency
    kTap12FilterResonance, // Tap 12 filter resonance (-1.0 to +1.0)
    kTap12FilterType,    // Tap 12 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap13FilterCutoff,  // Tap 13 filter cutoff frequency
    kTap13FilterResonance, // Tap 13 filter resonance (-1.0 to +1.0)
    kTap13FilterType,    // Tap 13 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap14FilterCutoff,  // Tap 14 filter cutoff frequency
    kTap14FilterResonance, // Tap 14 filter resonance (-1.0 to +1.0)
    kTap14FilterType,    // Tap 14 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap15FilterCutoff,  // Tap 15 filter cutoff frequency
    kTap15FilterResonance, // Tap 15 filter resonance (-1.0 to +1.0)
    kTap15FilterType,    // Tap 15 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    kTap16FilterCutoff,  // Tap 16 filter cutoff frequency
    kTap16FilterResonance, // Tap 16 filter resonance (-1.0 to +1.0)
    kTap16FilterType,    // Tap 16 filter type (0=LP, 1=HP, 2=BP, 3=Notch)
    // Per-tap pitch shift parameters (16 taps)
    kTap1PitchShift,     // Tap 1 pitch shift in semitones (-12 to +12)
    kTap2PitchShift,     // Tap 2 pitch shift in semitones (-12 to +12)
    kTap3PitchShift,     // Tap 3 pitch shift in semitones (-12 to +12)
    kTap4PitchShift,     // Tap 4 pitch shift in semitones (-12 to +12)
    kTap5PitchShift,     // Tap 5 pitch shift in semitones (-12 to +12)
    kTap6PitchShift,     // Tap 6 pitch shift in semitones (-12 to +12)
    kTap7PitchShift,     // Tap 7 pitch shift in semitones (-12 to +12)
    kTap8PitchShift,     // Tap 8 pitch shift in semitones (-12 to +12)
    kTap9PitchShift,     // Tap 9 pitch shift in semitones (-12 to +12)
    kTap10PitchShift,    // Tap 10 pitch shift in semitones (-12 to +12)
    kTap11PitchShift,    // Tap 11 pitch shift in semitones (-12 to +12)
    kTap12PitchShift,    // Tap 12 pitch shift in semitones (-12 to +12)
    kTap13PitchShift,    // Tap 13 pitch shift in semitones (-12 to +12)
    kTap14PitchShift,    // Tap 14 pitch shift in semitones (-12 to +12)
    kTap15PitchShift,    // Tap 15 pitch shift in semitones (-12 to +12)
    kTap16PitchShift,    // Tap 16 pitch shift in semitones (-12 to +12)
    // Per-tap feedback send parameters (16 taps)
    kTap1FeedbackSend,   // Tap 1 feedback send level (0.0 to 1.0)
    kTap2FeedbackSend,   // Tap 2 feedback send level (0.0 to 1.0)
    kTap3FeedbackSend,   // Tap 3 feedback send level (0.0 to 1.0)
    kTap4FeedbackSend,   // Tap 4 feedback send level (0.0 to 1.0)
    kTap5FeedbackSend,   // Tap 5 feedback send level (0.0 to 1.0)
    kTap6FeedbackSend,   // Tap 6 feedback send level (0.0 to 1.0)
    kTap7FeedbackSend,   // Tap 7 feedback send level (0.0 to 1.0)
    kTap8FeedbackSend,   // Tap 8 feedback send level (0.0 to 1.0)
    kTap9FeedbackSend,   // Tap 9 feedback send level (0.0 to 1.0)
    kTap10FeedbackSend,  // Tap 10 feedback send level (0.0 to 1.0)
    kTap11FeedbackSend,  // Tap 11 feedback send level (0.0 to 1.0)
    kTap12FeedbackSend,  // Tap 12 feedback send level (0.0 to 1.0)
    kTap13FeedbackSend,  // Tap 13 feedback send level (0.0 to 1.0)
    kTap14FeedbackSend,  // Tap 14 feedback send level (0.0 to 1.0)
    kTap15FeedbackSend,  // Tap 15 feedback send level (0.0 to 1.0)
    kTap16FeedbackSend,  // Tap 16 feedback send level (0.0 to 1.0)
    // Global controls
    kGlobalDryWet,       // Global Dry/Wet mix (affects final output)
    kDelayBypass,        // Delay section bypass toggle
    // Discrete control system parameters (32 parameters)
    kDiscrete1,          // Discrete control 1 (0.0-1.0)
    kDiscrete2,          // Discrete control 2 (0.0-1.0)
    kDiscrete3,          // Discrete control 3 (0.0-1.0)
    kDiscrete4,          // Discrete control 4 (0.0-1.0)
    kDiscrete5,          // Discrete control 5 (0.0-1.0)
    kDiscrete6,          // Discrete control 6 (0.0-1.0)
    kDiscrete7,          // Discrete control 7 (0.0-1.0)
    kDiscrete8,          // Discrete control 8 (0.0-1.0)
    kDiscrete9,          // Discrete control 9 (0.0-1.0)
    kDiscrete10,         // Discrete control 10 (0.0-1.0)
    kDiscrete11,         // Discrete control 11 (0.0-1.0)
    kDiscrete12,         // Discrete control 12 (0.0-1.0)
    kDiscrete13,         // Discrete control 13 (0.0-1.0)
    kDiscrete14,         // Discrete control 14 (0.0-1.0)
    kDiscrete15,         // Discrete control 15 (0.0-1.0)
    kDiscrete16,         // Discrete control 16 (0.0-1.0)
    kDiscrete17,         // Discrete control 17 (0.0-1.0)
    kDiscrete18,         // Discrete control 18 (0.0-1.0)
    kDiscrete19,         // Discrete control 19 (0.0-1.0)
    kDiscrete20,         // Discrete control 20 (0.0-1.0)
    kDiscrete21,         // Discrete control 21 (0.0-1.0)
    kDiscrete22,         // Discrete control 22 (0.0-1.0)
    kDiscrete23,         // Discrete control 23 (0.0-1.0)
    kDiscrete24,         // Discrete control 24 (0.0-1.0)
    // Macro curve system parameters
    kMacroCurve1Type,    // Curve type 1 (0-7)
    kMacroCurve2Type,    // Curve type 2 (0-7)
    kMacroCurve3Type,    // Curve type 3 (0-7)
    kMacroCurve4Type,    // Curve type 4 (0-7)
    // Macro knob parameters
    kMacroKnob1,         // Macro knob 1 (0.0-1.0)
    kMacroKnob2,         // Macro knob 2 (0.0-1.0)
    kMacroKnob3,         // Macro knob 3 (0.0-1.0)
    kMacroKnob4,         // Macro knob 4 (0.0-1.0)
    kMacroKnob5,         // Macro knob 5 (0.0-1.0)
    kMacroKnob6,         // Macro knob 6 (0.0-1.0)
    kMacroKnob7,         // Macro knob 7 (0.0-1.0)
    kMacroKnob8,         // Macro knob 8 (0.0-1.0)
    // Randomization and reset controls
    kRandomizeSeed,      // Randomization seed value
    kRandomizeAmount,    // Randomization intensity (0.0-1.0)
    kRandomizeTrigger,   // Trigger randomization (0=idle, 1=trigger)
    kResetTrigger,       // Trigger reset to defaults (0=idle, 1=trigger)
    kNumParams
};

// Tempo sync divisions
enum TempoSyncDivisions {
    kSync_1_64 = 0,      // 1/64 note
    kSync_1_32T,         // 1/32 triplet
    kSync_1_64D,         // 1/64 dotted
    kSync_1_32,          // 1/32 note
    kSync_1_16T,         // 1/16 triplet
    kSync_1_32D,         // 1/32 dotted
    kSync_1_16,          // 1/16 note
    kSync_1_8T,          // 1/8 triplet
    kSync_1_16D,         // 1/16 dotted
    kSync_1_8,           // 1/8 note
    kSync_1_4T,          // 1/4 triplet
    kSync_1_8D,          // 1/8 dotted
    kSync_1_4,           // 1/4 note
    kSync_1_2T,          // 1/2 triplet
    kSync_1_4D,          // 1/4 dotted
    kSync_1_2,           // 1/2 note
    kSync_1_1T,          // 1 bar triplet
    kSync_1_2D,          // 1/2 dotted
    kSync_1_1,           // 1 bar
    kSync_2_1,           // 2 bars
    kSync_4_1,           // 4 bars
    kSync_8_1,           // 8 bars
    kNumSyncDivisions
};

// Grid values (Taps/Beat)
enum GridValues {
    kGrid_1 = 0,         // 1 Tap/Beat
    kGrid_2,             // 2 Taps/Beat
    kGrid_3,             // 3 Taps/Beat
    kGrid_4,             // 4 Taps/Beat
    kGrid_6,             // 6 Taps/Beat
    kGrid_8,             // 8 Taps/Beat
    kGrid_12,            // 12 Taps/Beat
    kGrid_16,            // 16 Taps/Beat
    kNumGridValues
};

// Filter types
enum FilterTypes {
    kFilterType_Bypass = 0,      // Bypass (no filtering)
    kFilterType_LowPass,         // Low pass filter
    kFilterType_HighPass,        // High pass filter
    kFilterType_BandPass,        // Band pass filter
    kFilterType_Notch,           // Notch filter
    kNumFilterTypes
};

// Macro curve types (from Rainmaker manual)
enum MacroCurveTypes {
    kCurveType_Linear = 0,       // Linear curve (y = x)
    kCurveType_Exponential,      // Exponential curve (y = x^2)
    kCurveType_InverseExp,       // Inverse exponential curve (y = 1 - (1-x)^2)
    kCurveType_Logarithmic,      // Logarithmic curve (y = log(1 + x*9)/log(10))
    kCurveType_InverseLog,       // Inverse logarithmic curve (y = (10^x - 1)/9)
    kCurveType_SCurve,          // S-curve (y = 0.5 * (1 + tanh(4*(x-0.5))))
    kCurveType_InverseSCurve,   // Inverse S-curve (y = 0.5 + 0.25*atan(4*(2*x-1))/atan(4))
    kCurveType_Quantized,       // Quantized/stepped curve (8 steps)
    kNumCurveTypes
};

// Randomization constraints for different parameter types
enum RandomizationConstraints {
    kRandomConstraint_Full = 0,      // Full 0.0-1.0 randomization
    kRandomConstraint_Musical,       // Musical intervals and scales
    kRandomConstraint_Conservative,  // Limited range for stability
    kRandomConstraint_Bipolar,      // Bipolar around center (0.5)
    kNumRandomConstraints
};


} // namespace WaterStick