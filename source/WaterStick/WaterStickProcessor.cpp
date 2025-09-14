#include "WaterStickProcessor.h"
#include "WaterStickCIDs.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

using namespace Steinberg;

namespace WaterStick {

//------------------------------------------------------------------------
WaterStickProcessor::WaterStickProcessor()
{
    setControllerClass(kWaterStickControllerUID);
}

//------------------------------------------------------------------------
WaterStickProcessor::~WaterStickProcessor()
{
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickProcessor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk)
    {
        return result;
    }

    // Add audio input/output busses
    addAudioInput(STR16("Stereo In"), Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickProcessor::terminate()
{
    return AudioEffect::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
{
    return AudioEffect::setupProcessing(newSetup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickProcessor::process(Vst::ProcessData& data)
{
    // Basic passthrough processing
    if (data.numInputs == 0 || data.numOutputs == 0)
    {
        return kResultOk;
    }

    // Get input and output buses
    Vst::AudioBusBuffers* input = data.inputs;
    Vst::AudioBusBuffers* output = data.outputs;

    // Simple passthrough
    for (int32 channel = 0; channel < input->numChannels && channel < output->numChannels; channel++)
    {
        if (input->channelBuffers32 && output->channelBuffers32)
        {
            memcpy(output->channelBuffers32[channel], input->channelBuffers32[channel],
                   data.numSamples * sizeof(Vst::Sample32));
        }
    }

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickProcessor::getState(IBStream* state)
{
    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickProcessor::setState(IBStream* state)
{
    return kResultOk;
}

} // namespace WaterStick