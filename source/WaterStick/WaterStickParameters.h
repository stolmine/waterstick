#pragma once

namespace WaterStick {

// Parameter IDs
enum {
    kInputGain = 0,
    kOutputGain,
    kDelayTime,
    kDryWet,
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

} // namespace WaterStick