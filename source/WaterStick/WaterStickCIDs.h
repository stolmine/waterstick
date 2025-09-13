#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace WaterStick {

// Processor UUID - generated unique identifier
static const Steinberg::FUID kWaterStickProcessorUID(0x12345678, 0x12345678, 0x12345678, 0x12345678);

// Controller UUID - generated unique identifier
static const Steinberg::FUID kWaterStickControllerUID(0x87654321, 0x87654321, 0x87654321, 0x87654321);

#define WaterStickVST3Category "Fx|Delay"

} // namespace WaterStick