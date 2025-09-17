#include "WaterStickController.h"
#include "WaterStickEditor.h"
#include "WaterStickCIDs.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/vsttypes.h"
#include <cmath>

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

    // Grid parameter
    parameters.addParameter(STR16("Grid"), nullptr, kNumGridValues - 1,
                           static_cast<Vst::ParamValue>(kGrid_4) / (kNumGridValues - 1),
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kGrid, 0,
                           STR16("Tap"));

    // Tap parameters (16 taps) - need to use individual strings for STR16 macro
    parameters.addParameter(STR16("Tap 1 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap1Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 1 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap1Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 1 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap1Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 2 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap2Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 2 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap2Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 2 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap2Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 3 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap3Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 3 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap3Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 3 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap3Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 4 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap4Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 4 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap4Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 4 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap4Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 5 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap5Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 5 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap5Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 5 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap5Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 6 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap6Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 6 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap6Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 6 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap6Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 7 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap7Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 7 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap7Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 7 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap7Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 8 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap8Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 8 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap8Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 8 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap8Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 9 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap9Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 9 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap9Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 9 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap9Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 10 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap10Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 10 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap10Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 10 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap10Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 11 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap11Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 11 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap11Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 11 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap11Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 12 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap12Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 12 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap12Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 12 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap12Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 13 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap13Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 13 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap13Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 13 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap13Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 14 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap14Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 14 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap14Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 14 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap14Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 15 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap15Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 15 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap15Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 15 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap15Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 16 Enable"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap16Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 16 Level"), STR16("%"), 0, 1.0, Vst::ParameterInfo::kCanAutomate, kTap16Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 16 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap16Pan, 0, STR16("Tap"));

    // Global filter parameters
    parameters.addParameter(STR16("Filter Cutoff"), STR16("Hz"), 0, 0.5,
                           Vst::ParameterInfo::kCanAutomate, kFilterCutoff, 0,
                           STR16("Filter"));

    parameters.addParameter(STR16("Filter Resonance"), STR16("%"), 0, 0.5,
                           Vst::ParameterInfo::kCanAutomate, kFilterResonance, 0,
                           STR16("Filter"));

    parameters.addParameter(STR16("Filter Type"), nullptr, kNumFilterTypes - 1, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kFilterType, 0,
                           STR16("Filter"));

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
    int32 syncDivision, grid;

    if (streamer.readFloat(inputGain) == false) return kResultFalse;
    if (streamer.readFloat(outputGain) == false) return kResultFalse;
    if (streamer.readFloat(delayTime) == false) return kResultFalse;
    if (streamer.readFloat(dryWet) == false) return kResultFalse;
    if (streamer.readBool(tempoSyncMode) == false) return kResultFalse;
    if (streamer.readInt32(syncDivision) == false) return kResultFalse;
    if (streamer.readInt32(grid) == false) return kResultFalse;

    setParamNormalized(kInputGain, inputGain);
    setParamNormalized(kOutputGain, outputGain);
    setParamNormalized(kDelayTime, delayTime / 2.0); // Normalize 0-2s to 0-1
    setParamNormalized(kDryWet, dryWet);
    setParamNormalized(kTempoSyncMode, tempoSyncMode ? 1.0 : 0.0);
    setParamNormalized(kSyncDivision, static_cast<Vst::ParamValue>(syncDivision) / (kNumSyncDivisions - 1));
    setParamNormalized(kGrid, static_cast<Vst::ParamValue>(grid) / (kNumGridValues - 1));

    // Load all tap parameters
    for (int i = 0; i < 16; i++) {
        bool tapEnabled;
        float tapLevel, tapPan;

        if (streamer.readBool(tapEnabled) == false) return kResultFalse;
        if (streamer.readFloat(tapLevel) == false) return kResultFalse;
        if (streamer.readFloat(tapPan) == false) return kResultFalse;

        setParamNormalized(kTap1Enable + (i * 3), tapEnabled ? 1.0 : 0.0);
        setParamNormalized(kTap1Level + (i * 3), tapLevel);
        setParamNormalized(kTap1Pan + (i * 3), tapPan);
    }

    // Load filter parameters
    float filterCutoff, filterResonance;
    int32 filterType;

    if (streamer.readFloat(filterCutoff) == false) return kResultFalse;
    if (streamer.readFloat(filterResonance) == false) return kResultFalse;
    if (streamer.readInt32(filterType) == false) return kResultFalse;

    // Inverse of logarithmic scaling: if freq = 20 * pow(1000, value), then value = log(freq/20) / log(1000)
    setParamNormalized(kFilterCutoff, std::log(filterCutoff / 20.0f) / std::log(1000.0f));

    // Inverse of resonance scaling
    float normalizedResonance;
    if (filterResonance >= 0.0f) {
        // Inverse of cubic curve: value = cbrt(resonance)
        normalizedResonance = 0.5f + 0.5f * std::cbrt(filterResonance);
    } else {
        // Linear mapping for negative values
        normalizedResonance = 0.5f + filterResonance / 2.0f;
    }
    setParamNormalized(kFilterResonance, normalizedResonance);
    setParamNormalized(kFilterType, static_cast<Vst::ParamValue>(filterType) / (kNumFilterTypes - 1));

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
        case kGrid:
        {
            int grid = static_cast<int>(valueNormalized * (kNumGridValues - 1) + 0.5);
            if (grid >= 0 && grid < kNumGridValues) {
                // Grid text lookup table (matches TapDistribution class)
                static const char* gridTexts[kNumGridValues] = {
                    "1", "2", "3", "4", "6", "8", "12", "16"
                };
                Steinberg::UString(string, 128).fromAscii(gridTexts[grid]);
                return kResultTrue;
            }
            break;
        }
        case kFilterCutoff:
        {
            // Convert normalized value to frequency using same logarithmic scale as processor
            float frequency = 20.0f * std::pow(1000.0f, valueNormalized); // 20Hz to 20kHz logarithmic

            char freqText[128];
            if (frequency < 1000.0f) {
                snprintf(freqText, sizeof(freqText), "%.1f Hz", frequency);
            } else {
                snprintf(freqText, sizeof(freqText), "%.2f kHz", frequency / 1000.0f);
            }
            Steinberg::UString(string, 128).fromAscii(freqText);
            return kResultTrue;
        }
        case kFilterResonance:
        {
            // Convert normalized value to actual resonance using same scaling as processor
            float resonance;
            if (valueNormalized >= 0.5f) {
                // Positive resonance with cubic curve
                float positiveValue = (valueNormalized - 0.5f) * 2.0f;
                resonance = positiveValue * positiveValue * positiveValue;
            } else {
                // Negative resonance with linear mapping
                resonance = (valueNormalized - 0.5f) * 2.0f;
            }

            char resText[128];
            snprintf(resText, sizeof(resText), "%.2f", resonance);
            Steinberg::UString(string, 128).fromAscii(resText);
            return kResultTrue;
        }
        case kFilterType:
        {
            int filterType = static_cast<int>(valueNormalized * (kNumFilterTypes - 1) + 0.5);
            if (filterType >= 0 && filterType < kNumFilterTypes) {
                static const char* filterTypeTexts[kNumFilterTypes] = {
                    "Low Pass", "High Pass", "Band Pass", "Notch"
                };
                Steinberg::UString(string, 128).fromAscii(filterTypeTexts[filterType]);
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

//------------------------------------------------------------------------
IPlugView* PLUGIN_API WaterStickController::createView(FIDString name)
{
    if (FIDStringsEqual(name, Vst::ViewType::kEditor))
    {
        return new WaterStickEditor(this);
    }
    return nullptr;
}

} // namespace WaterStick