#include "WaterStickController.h"
#include "WaterStickCIDs.h"
#include "base/source/fstreamer.h"

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
    if (streamer.readFloat(inputGain) == false) return kResultFalse;
    if (streamer.readFloat(outputGain) == false) return kResultFalse;
    if (streamer.readFloat(delayTime) == false) return kResultFalse;
    if (streamer.readFloat(dryWet) == false) return kResultFalse;

    setParamNormalized(kInputGain, inputGain);
    setParamNormalized(kOutputGain, outputGain);
    setParamNormalized(kDelayTime, delayTime / 2.0); // Normalize 0-2s to 0-1
    setParamNormalized(kDryWet, dryWet);

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

} // namespace WaterStick