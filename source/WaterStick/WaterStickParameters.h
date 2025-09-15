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

} // namespace WaterStick