#pragma once

namespace WaterStick {

// Parameter IDs
enum {
    kInputGain = 0,
    kOutputGain,
    kDelayTime,
    kFeedback,               // Global feedback parameter
    // Tempo sync parameters - added at end to maintain compatibility
    kTempoSyncMode,      // Toggle: 0=Free, 1=Synced
    kSyncDivision,       // Sync division: 1/64 to 8 bars
    // Tap distribution parameters
    kGrid,               // Grid parameter: 1, 2, 3, 4, 6, 8, 12, 16 (Taps/Beat)
    kDelayGain,          // Delay section gain (-40dB to +12dB)
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
    // Routing and Wet/Dry controls
    kRouteMode,          // Routing mode: 0=Delay>Comb, 1=Comb>Delay, 2=Delay+Comb
    kGlobalDryWet,       // Global Dry/Wet mix (affects final output)
    kDelayDryWet,        // Delay section Dry/Wet mix (delay section only)
    kCombDryWet,         // Comb section Dry/Wet mix (comb section only)
    kDelayBypass,        // Delay section bypass toggle
    kCombBypass,         // Comb section bypass toggle
    // Comb control parameters
    kCombSize,           // Comb delay size (0.0001f to 2.0f seconds)
    kCombFeedback,       // Comb feedback (0.0f to 0.99f)
    kCombPitchCV,        // Comb pitch CV (-5.0f to +5.0f volts)
    kCombTaps,           // Number of active taps (1 to 64)
    kCombSync,           // Comb sync mode (0=free, 1=tempo-synced)
    kCombDivision,       // Comb sync division (0 to 21)
    kCombPattern,        // Comb tap spacing pattern (0-15)
    kCombSlope,          // Comb envelope slope (0-3: Flat, Rising, Falling, Rise/Fall)
    kCombGain,           // Comb section gain (-40dB to +12dB)
    kCombFadeTime,       // Comb parameter fade time (0.0-1.0, maps to 1ms-500ms)
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

// Comb pattern types (16 different tap spacing patterns)
enum CombPatterns {
    kCombPattern_1 = 0,          // Pattern 1 (uniform spacing)
    kCombPattern_2,              // Pattern 2
    kCombPattern_3,              // Pattern 3
    kCombPattern_4,              // Pattern 4
    kCombPattern_5,              // Pattern 5
    kCombPattern_6,              // Pattern 6
    kCombPattern_7,              // Pattern 7
    kCombPattern_8,              // Pattern 8
    kCombPattern_9,              // Pattern 9
    kCombPattern_10,             // Pattern 10
    kCombPattern_11,             // Pattern 11
    kCombPattern_12,             // Pattern 12
    kCombPattern_13,             // Pattern 13
    kCombPattern_14,             // Pattern 14
    kCombPattern_15,             // Pattern 15
    kCombPattern_16,             // Pattern 16
    kNumCombPatterns
};

// Comb slope types (4 envelope patterns)
enum CombSlopes {
    kCombSlope_Flat = 0,         // Flat envelope
    kCombSlope_Rising,           // Rising envelope
    kCombSlope_Falling,          // Falling envelope
    kCombSlope_RiseFall,         // Rise/Fall envelope
    kNumCombSlopes
};

} // namespace WaterStick