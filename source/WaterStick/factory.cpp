#include "public.sdk/source/main/pluginfactory.h"
#include "WaterStickProcessor.h"
#include "WaterStickController.h"
#include "WaterStickCIDs.h"
#include "version.h"

using namespace Steinberg;

BEGIN_FACTORY_DEF (stringCompanyName, stringCompanyWeb, stringCompanyEmail)

    DEF_CLASS2 (INLINE_UID_FROM_FUID(WaterStick::kWaterStickProcessorUID),
                PClassInfo::kManyInstances,
                kVstAudioEffectClass,
                stringPluginName,
                Vst::kDistributable,
                WaterStickVST3Category,
                FULL_VERSION_STR,
                kVstVersionString,
                WaterStick::WaterStickProcessor::createInstance)

    DEF_CLASS2 (INLINE_UID_FROM_FUID(WaterStick::kWaterStickControllerUID),
                PClassInfo::kManyInstances,
                kVstComponentControllerClass,
                stringPluginName "Controller",
                0,
                "",
                FULL_VERSION_STR,
                kVstVersionString,
                WaterStick::WaterStickController::createInstance)

END_FACTORY