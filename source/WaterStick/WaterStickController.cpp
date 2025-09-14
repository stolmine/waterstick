#include "WaterStickController.h"
#include "WaterStickCIDs.h"
#include "pluginterfaces/base/ustring.h"
#include "base/source/fstreamer.h"

using namespace Steinberg;

namespace WaterStick {

WaterStickController::WaterStickController()
{
}

WaterStickController::~WaterStickController()
{
}

tresult PLUGIN_API WaterStickController::initialize(FUnknown* context)
{
    tresult result = EditController::initialize(context);
    if (result != kResultOk)
        return result;

    // Add parameters
    parameters.addParameter(STR16("Delay Time"), STR16("s"), 0, 0.25,
                           Vst::ParameterInfo::kCanAutomate, kDelayTime);

    parameters.addParameter(STR16("Delay Feedback"), STR16("%"), 0, 0.3,
                           Vst::ParameterInfo::kCanAutomate, kDelayFeedback);

    parameters.addParameter(STR16("Delay Mix"), STR16("%"), 0, 0.5,
                           Vst::ParameterInfo::kCanAutomate, kDelayMix);

    parameters.addParameter(STR16("Comb Size"), STR16(""), 0, 0.5,
                           Vst::ParameterInfo::kCanAutomate, kCombSize);

    parameters.addParameter(STR16("Comb Feedback"), STR16("%"), 0, 0.4,
                           Vst::ParameterInfo::kCanAutomate, kCombFeedback);

    parameters.addParameter(STR16("Comb Damping"), STR16("%"), 0, 0.5,
                           Vst::ParameterInfo::kCanAutomate, kCombDamping);

    parameters.addParameter(STR16("Comb Density"), STR16(""), 0, 0.125,
                           Vst::ParameterInfo::kCanAutomate, kCombDensity);

    parameters.addParameter(STR16("Comb Mix"), STR16("%"), 0, 0.5,
                           Vst::ParameterInfo::kCanAutomate, kCombMix);

    parameters.addParameter(STR16("Input Gain"), STR16("dB"), 0, 1.0,
                           Vst::ParameterInfo::kCanAutomate, kInputGain);

    parameters.addParameter(STR16("Output Gain"), STR16("dB"), 0, 1.0,
                           Vst::ParameterInfo::kCanAutomate, kOutputGain);

    parameters.addParameter(STR16("Bypass"), STR16(""), 0, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
                           kBypass);

    return result;
}

tresult PLUGIN_API WaterStickController::terminate()
{
    return EditController::terminate();
}

tresult PLUGIN_API WaterStickController::setComponentState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    // Read parameters from state
    IBStreamer streamer(state, kLittleEndian);

    double delayTime, delayFeedback, delayMix;
    double combSize, combFeedback, combDamping, combDensity, combMix;
    double inputGain, outputGain, bypass;

    if (!streamer.readDouble(delayTime)) return kResultFalse;
    if (!streamer.readDouble(delayFeedback)) return kResultFalse;
    if (!streamer.readDouble(delayMix)) return kResultFalse;

    if (!streamer.readDouble(combSize)) return kResultFalse;
    if (!streamer.readDouble(combFeedback)) return kResultFalse;
    if (!streamer.readDouble(combDamping)) return kResultFalse;
    if (!streamer.readDouble(combDensity)) return kResultFalse;
    if (!streamer.readDouble(combMix)) return kResultFalse;
    if (!streamer.readDouble(inputGain)) return kResultFalse;
    if (!streamer.readDouble(outputGain)) return kResultFalse;
    if (!streamer.readDouble(bypass)) return kResultFalse;

    // Set parameter values
    setParamNormalized(kDelayTime, delayTime);
    setParamNormalized(kDelayFeedback, delayFeedback);
    setParamNormalized(kDelayMix, delayMix);
    setParamNormalized(kCombSize, combSize);
    setParamNormalized(kCombFeedback, combFeedback);
    setParamNormalized(kCombDamping, combDamping);
    setParamNormalized(kCombDensity, combDensity);
    setParamNormalized(kCombMix, combMix);
    setParamNormalized(kInputGain, inputGain);
    setParamNormalized(kOutputGain, outputGain);
    setParamNormalized(kBypass, bypass);

    return kResultOk;
}

IPlugView* PLUGIN_API WaterStickController::createView(FIDString name)
{
    // Return nullptr for now - we'll add GUI later
    return nullptr;
}

} // namespace WaterStick