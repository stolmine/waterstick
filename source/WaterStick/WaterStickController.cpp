#include "WaterStickController.h"
#include "WaterStickCIDs.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"

using namespace Steinberg;

namespace WaterStick {

//------------------------------------------------------------------------
WaterStickController::WaterStickController()
{
}

//------------------------------------------------------------------------
WaterStickController::~WaterStickController()
{
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::initialize(FUnknown* context)
{
    tresult result = EditControllerEx1::initialize(context);
    if (result != kResultOk)
    {
        return result;
    }

    // Add parameters
    parameters.addParameter(STR16("Input Gain"), STR16("dB"), 0, 1.0,
                           Vst::ParameterInfo::kCanAutomate, kInputGain, 0,
                           STR16("Input"));

    parameters.addParameter(STR16("Output Gain"), STR16("dB"), 0, 1.0,
                           Vst::ParameterInfo::kCanAutomate, kOutputGain, 0,
                           STR16("Output"));

    parameters.addParameter(STR16("Delay Time"), STR16("s"), 0, 0.05,
                           Vst::ParameterInfo::kCanAutomate, kDelayTime, 0,
                           STR16("Delay"));

    parameters.addParameter(STR16("Dry/Wet"), STR16("%"), 0, 0.5,
                           Vst::ParameterInfo::kCanAutomate, kDryWet, 0,
                           STR16("Mix"));

    // Tempo sync parameters
    parameters.addParameter(STR16("Sync Mode"), nullptr, 1, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTempoSyncMode, 0,
                           STR16("Sync"));

    parameters.addParameter(STR16("Sync Division"), nullptr, kNumSyncDivisions - 1,
                           static_cast<Vst::ParamValue>(kSync_1_4) / (kNumSyncDivisions - 1),
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kSyncDivision, 0,
                           STR16("Sync"));

    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::terminate()
{
    return EditControllerEx1::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::setComponentState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    IBStreamer streamer(state, kLittleEndian);

    float inputGain, outputGain, delayTime, dryWet;
    bool tempoSyncMode;
    int32 syncDivision;

    if (streamer.readFloat(inputGain) == false) return kResultFalse;
    if (streamer.readFloat(outputGain) == false) return kResultFalse;
    if (streamer.readFloat(delayTime) == false) return kResultFalse;
    if (streamer.readFloat(dryWet) == false) return kResultFalse;
    if (streamer.readBool(tempoSyncMode) == false) return kResultFalse;
    if (streamer.readInt32(syncDivision) == false) return kResultFalse;

    setParamNormalized(kInputGain, inputGain);
    setParamNormalized(kOutputGain, outputGain);
    setParamNormalized(kDelayTime, delayTime / 2.0); // Normalize 0-2s to 0-1
    setParamNormalized(kDryWet, dryWet);
    setParamNormalized(kTempoSyncMode, tempoSyncMode ? 1.0 : 0.0);
    setParamNormalized(kSyncDivision, static_cast<Vst::ParamValue>(syncDivision) / (kNumSyncDivisions - 1));

    return kResultOk;
}

//------------------------------------------------------------------------
Vst::ParamValue PLUGIN_API WaterStickController::getParamNormalized(Vst::ParamID id)
{
    Vst::ParamValue value = EditControllerEx1::getParamNormalized(id);
    return value;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::setParamNormalized(Vst::ParamID id, Vst::ParamValue value)
{
    tresult result = EditControllerEx1::setParamNormalized(id, value);
    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::getParamStringByValue(Vst::ParamID id, Vst::ParamValue valueNormalized, Vst::String128 string)
{
    switch (id)
    {
        case kTempoSyncMode:
        {
            const char* text = (valueNormalized > 0.5) ? "Synced" : "Free";
            Steinberg::UString(string, 128).fromAscii(text);
            return kResultTrue;
        }
        case kSyncDivision:
        {
            int division = static_cast<int>(valueNormalized * (kNumSyncDivisions - 1) + 0.5);
            if (division >= 0 && division < kNumSyncDivisions) {
                // Division text lookup table (matches TempoSync class)
                static const char* divisionTexts[kNumSyncDivisions] = {
                    "1/64", "1/32T", "1/64.", "1/32", "1/16T", "1/32.", "1/16",
                    "1/8T", "1/16.", "1/8", "1/4T", "1/8.", "1/4",
                    "1/2T", "1/4.", "1/2", "1T", "1/2.", "1", "2", "4", "8"
                };
                Steinberg::UString(string, 128).fromAscii(divisionTexts[division]);
                return kResultTrue;
            }
            break;
        }
    }
    return EditControllerEx1::getParamStringByValue(id, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::getParamValueByString(Vst::ParamID id, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
    switch (id)
    {
        case kTempoSyncMode:
        {
            Steinberg::UString wrapper(string, tstrlen(string));
            Steinberg::UString syncedString(USTRING("Synced"));
            Steinberg::UString freeString(USTRING("Free"));
            if (wrapper == syncedString) {
                valueNormalized = 1.0;
                return kResultTrue;
            }
            if (wrapper == freeString) {
                valueNormalized = 0.0;
                return kResultTrue;
            }
            break;
        }
        case kSyncDivision:
        {
            // This could be implemented to parse division strings, but typically not needed
            // for VST3 hosts - they usually use the getParamStringByValue direction
            break;
        }
    }
    return EditControllerEx1::getParamValueByString(id, string, valueNormalized);
}

} // namespace WaterStick