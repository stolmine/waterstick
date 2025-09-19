#include "WaterStickController.h"
#include "WaterStickEditor.h"
#include "WaterStickCIDs.h"
#include "WaterStickLogger.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/vsttypes.h"
#include <cmath>
#include <iostream>
#include <iomanip>

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
    // Start fresh logging session for debugging
    WS_LOG_SESSION_START();

    tresult result = EditControllerEx1::initialize(context);
    if (result != kResultOk)
    {
        return result;
    }

    // Add parameters
    parameters.addParameter(STR16("Input Gain"), STR16("dB"), 0, 40.0/52.0,
                           Vst::ParameterInfo::kCanAutomate, kInputGain, 0,
                           STR16("Input"));

    parameters.addParameter(STR16("Output Gain"), STR16("dB"), 0, 40.0/52.0,
                           Vst::ParameterInfo::kCanAutomate, kOutputGain, 0,
                           STR16("Output"));

    parameters.addParameter(STR16("Delay Time"), STR16("s"), 0, 0.05,
                           Vst::ParameterInfo::kCanAutomate, kDelayTime, 0,
                           STR16("Delay"));

    parameters.addParameter(STR16("Dry/Wet"), STR16("%"), 0, 0.5,
                           Vst::ParameterInfo::kCanAutomate, kDryWet, 0,
                           STR16("Mix"));

    parameters.addParameter(STR16("Feedback"), STR16("%"), 0, 0.0,
                           Vst::ParameterInfo::kCanAutomate, kFeedback, 0,
                           STR16("Global"));

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
    parameters.addParameter(STR16("Tap 1 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap1Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 1 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap1Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 1 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap1Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 2 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap2Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 2 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap2Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 2 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap2Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 3 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap3Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 3 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap3Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 3 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap3Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 4 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap4Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 4 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap4Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 4 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap4Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 5 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap5Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 5 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap5Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 5 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap5Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 6 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap6Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 6 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap6Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 6 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap6Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 7 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap7Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 7 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap7Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 7 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap7Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 8 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap8Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 8 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap8Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 8 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap8Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 9 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap9Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 9 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap9Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 9 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap9Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 10 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap10Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 10 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap10Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 10 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap10Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 11 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap11Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 11 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap11Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 11 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap11Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 12 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap12Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 12 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap12Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 12 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap12Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 13 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap13Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 13 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap13Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 13 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap13Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 14 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap14Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 14 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap14Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 14 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap14Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 15 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap15Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 15 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap15Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 15 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap15Pan, 0, STR16("Tap"));

    parameters.addParameter(STR16("Tap 16 Enable"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap16Enable, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 16 Level"), STR16("%"), 0, 0.8, Vst::ParameterInfo::kCanAutomate, kTap16Level, 0, STR16("Tap"));
    parameters.addParameter(STR16("Tap 16 Pan"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap16Pan, 0, STR16("Tap"));

    // Per-tap filter parameters (16 taps × 3 parameters each)
    parameters.addParameter(STR16("Tap 1 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap1FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 1 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap1FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 1 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap1FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 2 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap2FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 2 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap2FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 2 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap2FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 3 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap3FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 3 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap3FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 3 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap3FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 4 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap4FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 4 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap4FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 4 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap4FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 5 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap5FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 5 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap5FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 5 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap5FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 6 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap6FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 6 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap6FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 6 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap6FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 7 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap7FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 7 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap7FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 7 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap7FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 8 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap8FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 8 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap8FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 8 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap8FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 9 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap9FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 9 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap9FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 9 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap9FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 10 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap10FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 10 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap10FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 10 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap10FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 11 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap11FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 11 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap11FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 11 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap11FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 12 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap12FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 12 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap12FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 12 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap12FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 13 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap13FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 13 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap13FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 13 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap13FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 14 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap14FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 14 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap14FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 14 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap14FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 15 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap15FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 15 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap15FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 15 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap15FilterType, 0, STR16("Filter"));

    parameters.addParameter(STR16("Tap 16 Filter Cutoff"), STR16("Hz"), 0, 0.566323334778673, Vst::ParameterInfo::kCanAutomate, kTap16FilterCutoff, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 16 Filter Resonance"), STR16("%"), 0, 0.5, Vst::ParameterInfo::kCanAutomate, kTap16FilterResonance, 0, STR16("Filter"));
    parameters.addParameter(STR16("Tap 16 Filter Type"), nullptr, kNumFilterTypes - 1, 0.0, Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kTap16FilterType, 0, STR16("Filter"));

    // Routing and Wet/Dry controls
    parameters.addParameter(STR16("Routing Mode"), nullptr, 2, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kRouteMode, 0,
                           STR16("Routing"));

    parameters.addParameter(STR16("Global Dry/Wet"), STR16("%"), 0, 0.5,
                           Vst::ParameterInfo::kCanAutomate, kGlobalDryWet, 0,
                           STR16("Mix"));

    parameters.addParameter(STR16("Delay Dry/Wet"), STR16("%"), 0, 1.0,
                           Vst::ParameterInfo::kCanAutomate, kDelayDryWet, 0,
                           STR16("Mix"));

    parameters.addParameter(STR16("Delay Bypass"), nullptr, 1, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kDelayBypass, 0,
                           STR16("Control"));

    parameters.addParameter(STR16("Comb Bypass"), nullptr, 1, 0.0,
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kCombBypass, 0,
                           STR16("Control"));

    // Comb control parameters
    parameters.addParameter(STR16("Comb Size"), STR16("s"), 0, 0.05,  // Default 0.1s (0.05 normalized for log scale)
                           Vst::ParameterInfo::kCanAutomate, kCombSize, 0,
                           STR16("Comb"));

    parameters.addParameter(STR16("Comb Feedback"), STR16("%"), 0, 0.0,  // Default 0.0
                           Vst::ParameterInfo::kCanAutomate, kCombFeedback, 0,
                           STR16("Comb"));

    parameters.addParameter(STR16("Comb Pitch CV"), STR16("V"), 0, 0.5,  // Default 0.0V (0.5 normalized for -5 to +5V range)
                           Vst::ParameterInfo::kCanAutomate, kCombPitchCV, 0,
                           STR16("Comb"));

    parameters.addParameter(STR16("Comb Taps"), nullptr, 63, 15.0/63.0,  // Default 16 taps (15/63 normalized for 1-64 range)
                           Vst::ParameterInfo::kCanAutomate, kCombTaps, 0,
                           STR16("Comb"));

    parameters.addParameter(STR16("Comb Sync"), nullptr, 1, 0.0,  // Default free mode
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kCombSync, 0,
                           STR16("Comb"));

    parameters.addParameter(STR16("Comb Division"), nullptr, kNumSyncDivisions - 1,
                           static_cast<Vst::ParamValue>(kSync_1_4) / (kNumSyncDivisions - 1),  // Default 1/4 note
                           Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList, kCombDivision, 0,
                           STR16("Comb"));

    // Initialize all parameters to their default values
    // This ensures proper display even if setComponentState is never called
    setDefaultParameters();

    WS_LOG_INFO("Controller::initialize() completed successfully");

    // Log all problematic parameter values after initialization
    WS_LOG_INFO("=== POST-INITIALIZE PARAMETER VALUES ===");

    // Log global parameters
    WS_LOG_PARAM_CONTEXT("INIT", kInputGain, "InputGain", getParamNormalized(kInputGain));
    WS_LOG_PARAM_CONTEXT("INIT", kOutputGain, "OutputGain", getParamNormalized(kOutputGain));

    // Log all 16 tap filter types (should be 0.0 for bypass)
    for (int i = 0; i < 16; i++) {
        int paramId = kTap1FilterType + (i * 3);
        std::ostringstream paramName;
        paramName << "Tap" << (i+1) << "FilterType";
        WS_LOG_PARAM_CONTEXT("INIT", paramId, paramName.str(), getParamNormalized(paramId));
    }

    // Log all 16 tap levels (should be 0.8)
    for (int i = 0; i < 16; i++) {
        int paramId = kTap1Level + (i * 3);
        std::ostringstream paramName;
        paramName << "Tap" << (i+1) << "Level";
        WS_LOG_PARAM_CONTEXT("INIT", paramId, paramName.str(), getParamNormalized(paramId));
    }

    // Log all 16 tap pans (should be 0.5)
    for (int i = 0; i < 16; i++) {
        int paramId = kTap1Pan + (i * 3);
        std::ostringstream paramName;
        paramName << "Tap" << (i+1) << "Pan";
        WS_LOG_PARAM_CONTEXT("INIT", paramId, paramName.str(), getParamNormalized(paramId));
    }

    // Log filter cutoffs (should be 0.566323 for 1kHz)
    for (int i = 0; i < 16; i++) {
        int paramId = kTap1FilterCutoff + (i * 3);
        std::ostringstream paramName;
        paramName << "Tap" << (i+1) << "FilterCutoff";
        WS_LOG_PARAM_CONTEXT("INIT", paramId, paramName.str(), getParamNormalized(paramId));
    }

    WS_LOG_INFO("=== END POST-INITIALIZE PARAMETER VALUES ===");

    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::terminate()
{
    return EditControllerEx1::terminate();
}

//------------------------------------------------------------------------
void WaterStickController::setDefaultParameters()
{
    // Set global parameters to their design defaults
    setParamNormalized(kInputGain, 40.0/52.0);   // 0dB
    setParamNormalized(kOutputGain, 40.0/52.0);  // 0dB
    setParamNormalized(kDelayTime, 0.05);        // 50ms
    setParamNormalized(kDryWet, 0.5);            // 50%
    setParamNormalized(kFeedback, 0.0);          // 0%
    setParamNormalized(kTempoSyncMode, 0.0);     // Free mode
    setParamNormalized(kSyncDivision, static_cast<Vst::ParamValue>(kSync_1_4) / (kNumSyncDivisions - 1)); // 1/4 note
    setParamNormalized(kGrid, static_cast<Vst::ParamValue>(kGrid_4) / (kNumGridValues - 1)); // 4 grid

    // Initialize all tap parameters to their defaults
    for (int i = 0; i < 16; i++) {
        setParamNormalized(kTap1Enable + (i * 3), 0.0);   // Disabled
        setParamNormalized(kTap1Level + (i * 3), 0.8);    // 80% level
        setParamNormalized(kTap1Pan + (i * 3), 0.5);      // Center pan
        setParamNormalized(kTap1FilterCutoff + (i * 3), 0.566323334778673);  // 1kHz
        setParamNormalized(kTap1FilterResonance + (i * 3), 0.5);  // Moderate resonance
        setParamNormalized(kTap1FilterType + (i * 3), 0.0);       // Bypass filter
    }

    // Set routing and mix parameters to defaults
    setParamNormalized(kRouteMode, 0.0);         // Delay>Comb
    setParamNormalized(kGlobalDryWet, 0.5);      // 50%
    setParamNormalized(kDelayDryWet, 1.0);       // 100% wet
    setParamNormalized(kDelayBypass, 0.0);       // Active
    setParamNormalized(kCombBypass, 0.0);        // Active
}

//------------------------------------------------------------------------
bool WaterStickController::isValidParameterValue(Vst::ParamID id, float value)
{
    // Check if parameter value is within expected range
    if (std::isnan(value) || std::isinf(value)) {
        return false;
    }

    // Most normalized parameters should be 0.0-1.0
    if (value < 0.0f || value > 1.0f) {
        return false;
    }

    // Additional specific validation for certain parameter types
    if (id >= kTap1Level && id <= kTap16Level && ((id - kTap1Level) % 3 == 0)) {
        // Tap levels: 0.0-1.0
        return true;  // Already validated above
    }

    if (id >= kTap1Pan && id <= kTap16Pan && ((id - kTap1Pan) % 3 == 0)) {
        // Tap pans: 0.0-1.0
        return true;  // Already validated above
    }

    if (id >= kTap1FilterCutoff && id <= kTap16FilterCutoff && ((id - kTap1FilterCutoff) % 3 == 0)) {
        // Filter cutoffs: 0.0-1.0
        return true;  // Already validated above
    }

    if (id >= kTap1FilterType && id <= kTap16FilterType && ((id - kTap1FilterType) % 3 == 2)) {
        // Filter types: 0.0-1.0 (but should normalize to valid enum values)
        return true;  // Already validated above
    }

    if (id == kInputGain || id == kOutputGain) {
        // Input/Output gains: 0.0-1.0
        return true;  // Already validated above
    }

    return true;  // Default case: basic 0.0-1.0 range is valid
}

//------------------------------------------------------------------------
float WaterStickController::getDefaultParameterValue(Vst::ParamID id)
{
    // Return appropriate default for specific parameter types
    if (id >= kTap1Level && id <= kTap16Level && ((id - kTap1Level) % 3 == 0)) {
        return 0.8f;  // Tap levels default to 80%
    }

    if (id >= kTap1Pan && id <= kTap16Pan && ((id - kTap1Pan) % 3 == 0)) {
        return 0.5f;  // Tap pans default to center
    }

    if (id >= kTap1FilterCutoff && id <= kTap16FilterCutoff && ((id - kTap1FilterCutoff) % 3 == 0)) {
        return 0.566323334778673f;  // Filter cutoffs default to 1kHz
    }

    if (id >= kTap1FilterType && id <= kTap16FilterType && ((id - kTap1FilterType) % 3 == 2)) {
        return 0.0f;  // Filter types default to Bypass
    }

    if (id == kInputGain || id == kOutputGain) {
        return 40.0f/52.0f;  // Input/Output gains default to 0dB
    }

    if (id >= kTap1FilterResonance && id <= kTap16FilterResonance && ((id - kTap1FilterResonance) % 3 == 1)) {
        return 0.5f;  // Filter resonance default to moderate
    }

    // Global parameters
    if (id == kDelayTime) return 0.05f;
    if (id == kDryWet) return 0.5f;
    if (id == kFeedback) return 0.0f;
    if (id == kTempoSyncMode) return 0.0f;
    if (id == kSyncDivision) return static_cast<float>(kSync_1_4) / (kNumSyncDivisions - 1);
    if (id == kGrid) return static_cast<float>(kGrid_4) / (kNumGridValues - 1);
    if (id >= kTap1Enable && id <= kTap16Enable && ((id - kTap1Enable) % 3 == 0)) return 0.0f;
    if (id == kRouteMode) return 0.0f;
    if (id == kGlobalDryWet) return 0.5f;
    if (id == kDelayDryWet) return 1.0f;
    if (id == kDelayBypass) return 0.0f;
    if (id == kCombBypass) return 0.0f;

    return 0.0f;  // Safe default
}

//------------------------------------------------------------------------
bool WaterStickController::isSemanticallySuspiciousState()
{
    // Check for patterns that indicate corrupted/old cache state

    // Pattern 1: All tap levels are 0.0 (likely corrupted)
    bool allLevelsZero = true;
    for (int i = 0; i < 16; i++) {
        float level = getParamNormalized(kTap1Level + (i * 3));
        if (level > 0.001f) {  // Use small epsilon for floating point comparison
            allLevelsZero = false;
            break;
        }
    }

    if (allLevelsZero) {
        WS_LOG_INFO("Semantic validation: All tap levels are 0.0 - suspicious");
        return true;
    }

    // Pattern 2: All gains are exactly 1.0 (likely from old development state)
    float inputGain = getParamNormalized(kInputGain);
    float outputGain = getParamNormalized(kOutputGain);
    if (std::abs(inputGain - 1.0f) < 0.001f && std::abs(outputGain - 1.0f) < 0.001f) {
        WS_LOG_INFO("Semantic validation: Both gains are 1.0 - suspicious");
        return true;
    }

    // Pattern 3: No taps enabled (likely fresh instance)
    bool anyTapEnabled = false;
    for (int i = 0; i < 16; i++) {
        float enabled = getParamNormalized(kTap1Enable + (i * 3));
        if (enabled > 0.5f) {
            anyTapEnabled = true;
            break;
        }
    }

    if (!anyTapEnabled) {
        WS_LOG_INFO("Semantic validation: No taps enabled - likely fresh instance");
        return true;
    }

    // Pattern 4: All filter types are not bypass (unusual for default state)
    bool allFiltersNonBypass = true;
    for (int i = 0; i < 16; i++) {
        float filterType = getParamNormalized(kTap1FilterType + (i * 3));
        if (filterType < 0.001f) {  // 0.0 is bypass
            allFiltersNonBypass = false;
            break;
        }
    }

    if (allFiltersNonBypass) {
        WS_LOG_INFO("Semantic validation: All filters non-bypass - suspicious");
        return true;
    }

    return false;  // State appears semantically valid
}

//------------------------------------------------------------------------
bool WaterStickController::hasValidStateSignature(IBStream* state, bool& hasSignature)
{
    hasSignature = false;

    if (!state) {
        return false;
    }

    // Save current position
    int64 originalPos;
    state->tell(&originalPos);

    // Go to beginning
    state->seek(0, IBStream::kIBSeekSet, nullptr);

    IBStreamer streamer(state, kLittleEndian);

    // Try to read version first
    Steinberg::int32 version;
    if (!streamer.readInt32(version)) {
        // Reset position and return
        state->seek(originalPos, IBStream::kIBSeekSet, nullptr);
        return false;
    }

    // Check if version is reasonable
    if (version < kStateVersionLegacy || version > kStateVersionCurrent) {
        // This is probably not a versioned state
        state->seek(originalPos, IBStream::kIBSeekSet, nullptr);
        return false;
    }

    // Try to read signature
    Steinberg::int32 signature;
    if (!streamer.readInt32(signature)) {
        // No signature present (old format)
        state->seek(originalPos, IBStream::kIBSeekSet, nullptr);
        return false;
    }

    hasSignature = true;

    // Check if signature matches
    bool isValid = (signature == kStateMagicNumber);

    // Reset position
    state->seek(originalPos, IBStream::kIBSeekSet, nullptr);

    if (isValid) {
        WS_LOG_INFO("Valid state signature detected: 0x" + std::to_string(signature));
    } else {
        WS_LOG_INFO("Invalid state signature detected: 0x" + std::to_string(signature) + " (expected 0x" + std::to_string(kStateMagicNumber) + ")");
    }

    return isValid;
}

//------------------------------------------------------------------------
tresult PLUGIN_API WaterStickController::setComponentState(IBStream* state)
{
    WS_LOG_INFO("Controller::setComponentState() called with enhanced freshness detection");

    if (!state) {
        WS_LOG_INFO("No state provided - using defaults");
        setDefaultParameters();
        return kResultOk;
    }

    // Check for valid state signature first
    bool hasSignature = false;
    bool validSignature = hasValidStateSignature(state, hasSignature);

    if (hasSignature && !validSignature) {
        WS_LOG_INFO("Invalid state signature detected - treating as corrupted cache, using defaults");
        setDefaultParameters();
        return kResultOk;
    }

    if (!hasSignature) {
        WS_LOG_INFO("No state signature found - checking if legacy or fresh state");
    }

    // Try to read state version first
    Steinberg::int32 stateVersion;
    tresult readResult;

    if (tryReadStateVersion(state, stateVersion)) {
        WS_LOG_INFO("State version detected: " + std::to_string(stateVersion));
        readResult = readVersionedState(state, stateVersion);
    } else {
        WS_LOG_INFO("No version header found - treating as legacy state");
        // Reset stream position for legacy reading
        state->seek(0, IBStream::kIBSeekSet, nullptr);
        readResult = readLegacyState(state);
    }

    // After loading state, perform semantic validation
    if (readResult == kResultOk) {
        if (isSemanticallySuspiciousState()) {
            WS_LOG_INFO("Semantic validation failed - state appears corrupted/cached, using defaults");
            setDefaultParameters();
            return kResultOk;
        } else {
            WS_LOG_INFO("Semantic validation passed - state appears to be valid user data");
        }
    }

    return readResult;
}

//------------------------------------------------------------------------
bool WaterStickController::tryReadStateVersion(IBStream* state, Steinberg::int32& version)
{
    IBStreamer streamer(state, kLittleEndian);

    // Try to read version header
    if (!streamer.readInt32(version)) {
        return false;
    }

    // Validate version is reasonable
    if (version < kStateVersionLegacy || version > kStateVersionCurrent) {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------
tresult WaterStickController::readLegacyState(IBStream* state)
{
    WS_LOG_INFO("Reading legacy (unversioned) state with enhanced validation");

    IBStreamer streamer(state, kLittleEndian);
    int invalidParameterCount = 0;
    int validParameterCount = 0;

    // Read and validate global parameters with individual fallback
    float inputGain, outputGain, delayTime, dryWet, feedback;
    bool tempoSyncMode;
    int32 syncDivision, grid;

    // Input Gain
    if (streamer.readFloat(inputGain) && isValidParameterValue(kInputGain, inputGain)) {
        setParamNormalized(kInputGain, inputGain);
        validParameterCount++;
        WS_LOG_INFO("Loaded valid InputGain: " + std::to_string(inputGain));
    } else {
        float defaultValue = getDefaultParameterValue(kInputGain);
        setParamNormalized(kInputGain, defaultValue);
        invalidParameterCount++;
        WS_LOG_INFO("Invalid InputGain, using default: " + std::to_string(defaultValue));
    }

    // Output Gain
    if (streamer.readFloat(outputGain) && isValidParameterValue(kOutputGain, outputGain)) {
        setParamNormalized(kOutputGain, outputGain);
        validParameterCount++;
        WS_LOG_INFO("Loaded valid OutputGain: " + std::to_string(outputGain));
    } else {
        float defaultValue = getDefaultParameterValue(kOutputGain);
        setParamNormalized(kOutputGain, defaultValue);
        invalidParameterCount++;
        WS_LOG_INFO("Invalid OutputGain, using default: " + std::to_string(defaultValue));
    }

    // Delay Time
    if (streamer.readFloat(delayTime) && delayTime >= 0.0f && delayTime <= 2.0f) {
        setParamNormalized(kDelayTime, delayTime / 2.0f); // Normalize 0-2s to 0-1
        validParameterCount++;
    } else {
        setParamNormalized(kDelayTime, getDefaultParameterValue(kDelayTime));
        invalidParameterCount++;
    }

    // Dry/Wet
    if (streamer.readFloat(dryWet) && isValidParameterValue(kDryWet, dryWet)) {
        setParamNormalized(kDryWet, dryWet);
        validParameterCount++;
    } else {
        setParamNormalized(kDryWet, getDefaultParameterValue(kDryWet));
        invalidParameterCount++;
    }

    // Feedback
    if (streamer.readFloat(feedback) && isValidParameterValue(kFeedback, feedback)) {
        setParamNormalized(kFeedback, feedback);
        validParameterCount++;
    } else {
        setParamNormalized(kFeedback, getDefaultParameterValue(kFeedback));
        invalidParameterCount++;
    }

    // Tempo Sync Mode
    if (streamer.readBool(tempoSyncMode)) {
        setParamNormalized(kTempoSyncMode, tempoSyncMode ? 1.0 : 0.0);
        validParameterCount++;
    } else {
        setParamNormalized(kTempoSyncMode, getDefaultParameterValue(kTempoSyncMode));
        invalidParameterCount++;
    }

    // Sync Division
    if (streamer.readInt32(syncDivision) && syncDivision >= 0 && syncDivision < kNumSyncDivisions) {
        setParamNormalized(kSyncDivision, static_cast<Vst::ParamValue>(syncDivision) / (kNumSyncDivisions - 1));
        validParameterCount++;
    } else {
        setParamNormalized(kSyncDivision, getDefaultParameterValue(kSyncDivision));
        invalidParameterCount++;
    }

    // Grid
    if (streamer.readInt32(grid) && grid >= 0 && grid < kNumGridValues) {
        setParamNormalized(kGrid, static_cast<Vst::ParamValue>(grid) / (kNumGridValues - 1));
        validParameterCount++;
    } else {
        setParamNormalized(kGrid, getDefaultParameterValue(kGrid));
        invalidParameterCount++;
    }

    // Load all tap parameters with individual validation
    for (int i = 0; i < 16; i++) {
        bool tapEnabled;
        float tapLevel, tapPan;
        float tapFilterCutoff, tapFilterResonance;
        int32 tapFilterType;

        // Tap Enable
        if (streamer.readBool(tapEnabled)) {
            setParamNormalized(kTap1Enable + (i * 3), tapEnabled ? 1.0 : 0.0);
            validParameterCount++;
        } else {
            setParamNormalized(kTap1Enable + (i * 3), getDefaultParameterValue(kTap1Enable + (i * 3)));
            invalidParameterCount++;
        }

        // Tap Level
        if (streamer.readFloat(tapLevel) && isValidParameterValue(kTap1Level + (i * 3), tapLevel)) {
            setParamNormalized(kTap1Level + (i * 3), tapLevel);
            validParameterCount++;
        } else {
            float defaultValue = getDefaultParameterValue(kTap1Level + (i * 3));
            setParamNormalized(kTap1Level + (i * 3), defaultValue);
            invalidParameterCount++;
            WS_LOG_INFO("Invalid Tap" + std::to_string(i+1) + "Level, using default: " + std::to_string(defaultValue));
        }

        // Tap Pan
        if (streamer.readFloat(tapPan) && isValidParameterValue(kTap1Pan + (i * 3), tapPan)) {
            setParamNormalized(kTap1Pan + (i * 3), tapPan);
            validParameterCount++;
        } else {
            float defaultValue = getDefaultParameterValue(kTap1Pan + (i * 3));
            setParamNormalized(kTap1Pan + (i * 3), defaultValue);
            invalidParameterCount++;
            WS_LOG_INFO("Invalid Tap" + std::to_string(i+1) + "Pan, using default: " + std::to_string(defaultValue));
        }

        // Tap Filter Cutoff
        if (streamer.readFloat(tapFilterCutoff) && tapFilterCutoff >= 20.0f && tapFilterCutoff <= 20000.0f) {
            // Convert frequency to normalized value
            float normalizedCutoff = std::log(tapFilterCutoff / 20.0f) / std::log(1000.0f);
            if (isValidParameterValue(kTap1FilterCutoff + (i * 3), normalizedCutoff)) {
                setParamNormalized(kTap1FilterCutoff + (i * 3), normalizedCutoff);
                validParameterCount++;
            } else {
                float defaultValue = getDefaultParameterValue(kTap1FilterCutoff + (i * 3));
                setParamNormalized(kTap1FilterCutoff + (i * 3), defaultValue);
                invalidParameterCount++;
                WS_LOG_INFO("Invalid Tap" + std::to_string(i+1) + "FilterCutoff, using default");
            }
        } else {
            float defaultValue = getDefaultParameterValue(kTap1FilterCutoff + (i * 3));
            setParamNormalized(kTap1FilterCutoff + (i * 3), defaultValue);
            invalidParameterCount++;
            WS_LOG_INFO("Invalid Tap" + std::to_string(i+1) + "FilterCutoff frequency, using default");
        }

        // Tap Filter Resonance
        if (streamer.readFloat(tapFilterResonance) && tapFilterResonance >= -1.0f && tapFilterResonance <= 1.0f) {
            // Convert resonance to normalized value
            float normalizedResonance;
            if (tapFilterResonance >= 0.0f) {
                normalizedResonance = 0.5f + 0.5f * std::cbrt(tapFilterResonance);
            } else {
                normalizedResonance = 0.5f + tapFilterResonance / 2.0f;
            }

            if (isValidParameterValue(kTap1FilterResonance + (i * 3), normalizedResonance)) {
                setParamNormalized(kTap1FilterResonance + (i * 3), normalizedResonance);
                validParameterCount++;
            } else {
                float defaultValue = getDefaultParameterValue(kTap1FilterResonance + (i * 3));
                setParamNormalized(kTap1FilterResonance + (i * 3), defaultValue);
                invalidParameterCount++;
            }
        } else {
            float defaultValue = getDefaultParameterValue(kTap1FilterResonance + (i * 3));
            setParamNormalized(kTap1FilterResonance + (i * 3), defaultValue);
            invalidParameterCount++;
        }

        // Tap Filter Type
        if (streamer.readInt32(tapFilterType) && tapFilterType >= 0 && tapFilterType < kNumFilterTypes) {
            float normalizedType = static_cast<Vst::ParamValue>(tapFilterType) / (kNumFilterTypes - 1);
            if (isValidParameterValue(kTap1FilterType + (i * 3), normalizedType)) {
                setParamNormalized(kTap1FilterType + (i * 3), normalizedType);
                validParameterCount++;
            } else {
                float defaultValue = getDefaultParameterValue(kTap1FilterType + (i * 3));
                setParamNormalized(kTap1FilterType + (i * 3), defaultValue);
                invalidParameterCount++;
                WS_LOG_INFO("Invalid Tap" + std::to_string(i+1) + "FilterType, using default: " + std::to_string(defaultValue));
            }
        } else {
            float defaultValue = getDefaultParameterValue(kTap1FilterType + (i * 3));
            setParamNormalized(kTap1FilterType + (i * 3), defaultValue);
            invalidParameterCount++;
            WS_LOG_INFO("Invalid Tap" + std::to_string(i+1) + "FilterType value, using default: " + std::to_string(defaultValue));
        }
    }

    // Load routing and wet/dry parameters with individual validation
    int32 routeMode;
    float globalDryWet, delayDryWet;
    bool delayBypass, combBypass;

    // Route Mode
    if (streamer.readInt32(routeMode) && routeMode >= 0 && routeMode <= 2) {
        setParamNormalized(kRouteMode, static_cast<Vst::ParamValue>(routeMode) / 2.0);
        validParameterCount++;
    } else {
        setParamNormalized(kRouteMode, getDefaultParameterValue(kRouteMode));
        invalidParameterCount++;
    }

    // Global Dry/Wet
    if (streamer.readFloat(globalDryWet) && isValidParameterValue(kGlobalDryWet, globalDryWet)) {
        setParamNormalized(kGlobalDryWet, globalDryWet);
        validParameterCount++;
    } else {
        setParamNormalized(kGlobalDryWet, getDefaultParameterValue(kGlobalDryWet));
        invalidParameterCount++;
    }

    // Delay Dry/Wet
    if (streamer.readFloat(delayDryWet) && isValidParameterValue(kDelayDryWet, delayDryWet)) {
        setParamNormalized(kDelayDryWet, delayDryWet);
        validParameterCount++;
    } else {
        setParamNormalized(kDelayDryWet, getDefaultParameterValue(kDelayDryWet));
        invalidParameterCount++;
    }

    // Delay Bypass
    if (streamer.readBool(delayBypass)) {
        setParamNormalized(kDelayBypass, delayBypass ? 1.0 : 0.0);
        validParameterCount++;
    } else {
        setParamNormalized(kDelayBypass, getDefaultParameterValue(kDelayBypass));
        invalidParameterCount++;
    }

    // Comb Bypass
    if (streamer.readBool(combBypass)) {
        setParamNormalized(kCombBypass, combBypass ? 1.0 : 0.0);
        validParameterCount++;
    } else {
        setParamNormalized(kCombBypass, getDefaultParameterValue(kCombBypass));
        invalidParameterCount++;
    }

    // Log validation results
    WS_LOG_INFO("Parameter validation complete: " + std::to_string(validParameterCount) + " valid, " + std::to_string(invalidParameterCount) + " invalid (using defaults)");

    // Log key parameter values after enhanced validation
    WS_LOG_INFO("=== POST-VALIDATION PARAMETER VALUES ===");

    // Log input/output gains
    WS_LOG_PARAM_CONTEXT("POST-VAL", kInputGain, "InputGain", getParamNormalized(kInputGain));
    WS_LOG_PARAM_CONTEXT("POST-VAL", kOutputGain, "OutputGain", getParamNormalized(kOutputGain));

    // Log first few tap levels and filter types as examples
    for (int i = 0; i < 4; i++) {
        int levelId = kTap1Level + (i * 3);
        int typeId = kTap1FilterType + (i * 3);
        int panId = kTap1Pan + (i * 3);

        std::ostringstream levelName, typeName, panName;
        levelName << "Tap" << (i+1) << "Level";
        typeName << "Tap" << (i+1) << "FilterType";
        panName << "Tap" << (i+1) << "Pan";

        WS_LOG_PARAM_CONTEXT("POST-VAL", levelId, levelName.str(), getParamNormalized(levelId));
        WS_LOG_PARAM_CONTEXT("POST-VAL", typeId, typeName.str(), getParamNormalized(typeId));
        WS_LOG_PARAM_CONTEXT("POST-VAL", panId, panName.str(), getParamNormalized(panId));
    }

    WS_LOG_INFO("=== END POST-VALIDATION PARAMETER VALUES ===");

    return kResultOk;
}

//------------------------------------------------------------------------
tresult WaterStickController::readCurrentVersionState(IBStream* state)
{
    WS_LOG_INFO("Reading current version (v1) state with enhanced validation");

    IBStreamer streamer(state, kLittleEndian);
    int invalidParameterCount = 0;
    int validParameterCount = 0;

    // Read and validate signature (version was already read)
    Steinberg::int32 signature;
    if (streamer.readInt32(signature)) {
        if (signature == kStateMagicNumber) {
            WS_LOG_INFO("Valid signature found in versioned state: 0x" + std::to_string(signature));
        } else {
            WS_LOG_INFO("Invalid signature in versioned state: 0x" + std::to_string(signature) + " (expected 0x" + std::to_string(kStateMagicNumber) + ")");
            // Continue reading but this is suspicious
        }
    } else {
        WS_LOG_INFO("No signature found in versioned state - old format");
        // Reset stream position and continue without signature
        state->seek(-4, IBStream::kIBSeekCur, nullptr);
    }

    // Read and validate global parameters with individual fallback
    float inputGain, outputGain, delayTime, dryWet, feedback;
    bool tempoSyncMode;
    int32 syncDivision, grid;

    // Input Gain
    if (streamer.readFloat(inputGain) && isValidParameterValue(kInputGain, inputGain)) {
        setParamNormalized(kInputGain, inputGain);
        validParameterCount++;
        WS_LOG_INFO("Loaded valid InputGain: " + std::to_string(inputGain));
    } else {
        float defaultValue = getDefaultParameterValue(kInputGain);
        setParamNormalized(kInputGain, defaultValue);
        invalidParameterCount++;
        WS_LOG_INFO("Invalid InputGain, using default: " + std::to_string(defaultValue));
    }

    // Output Gain
    if (streamer.readFloat(outputGain) && isValidParameterValue(kOutputGain, outputGain)) {
        setParamNormalized(kOutputGain, outputGain);
        validParameterCount++;
        WS_LOG_INFO("Loaded valid OutputGain: " + std::to_string(outputGain));
    } else {
        float defaultValue = getDefaultParameterValue(kOutputGain);
        setParamNormalized(kOutputGain, defaultValue);
        invalidParameterCount++;
        WS_LOG_INFO("Invalid OutputGain, using default: " + std::to_string(defaultValue));
    }

    // Delay Time
    if (streamer.readFloat(delayTime) && delayTime >= 0.0f && delayTime <= 2.0f) {
        setParamNormalized(kDelayTime, delayTime / 2.0f); // Normalize 0-2s to 0-1
        validParameterCount++;
    } else {
        setParamNormalized(kDelayTime, getDefaultParameterValue(kDelayTime));
        invalidParameterCount++;
    }

    // Dry/Wet
    if (streamer.readFloat(dryWet) && isValidParameterValue(kDryWet, dryWet)) {
        setParamNormalized(kDryWet, dryWet);
        validParameterCount++;
    } else {
        setParamNormalized(kDryWet, getDefaultParameterValue(kDryWet));
        invalidParameterCount++;
    }

    // Feedback
    if (streamer.readFloat(feedback) && isValidParameterValue(kFeedback, feedback)) {
        setParamNormalized(kFeedback, feedback);
        validParameterCount++;
    } else {
        setParamNormalized(kFeedback, getDefaultParameterValue(kFeedback));
        invalidParameterCount++;
    }

    // Tempo Sync Mode
    if (streamer.readBool(tempoSyncMode)) {
        setParamNormalized(kTempoSyncMode, tempoSyncMode ? 1.0 : 0.0);
        validParameterCount++;
    } else {
        setParamNormalized(kTempoSyncMode, getDefaultParameterValue(kTempoSyncMode));
        invalidParameterCount++;
    }

    // Sync Division
    if (streamer.readInt32(syncDivision) && syncDivision >= 0 && syncDivision < kNumSyncDivisions) {
        setParamNormalized(kSyncDivision, static_cast<Vst::ParamValue>(syncDivision) / (kNumSyncDivisions - 1));
        validParameterCount++;
    } else {
        setParamNormalized(kSyncDivision, getDefaultParameterValue(kSyncDivision));
        invalidParameterCount++;
    }

    // Grid
    if (streamer.readInt32(grid) && grid >= 0 && grid < kNumGridValues) {
        setParamNormalized(kGrid, static_cast<Vst::ParamValue>(grid) / (kNumGridValues - 1));
        validParameterCount++;
    } else {
        setParamNormalized(kGrid, getDefaultParameterValue(kGrid));
        invalidParameterCount++;
    }

    // Load all tap parameters with individual validation
    for (int i = 0; i < 16; i++) {
        bool tapEnabled;
        float tapLevel, tapPan;
        float tapFilterCutoff, tapFilterResonance;
        int32 tapFilterType;

        // Tap Enable
        if (streamer.readBool(tapEnabled)) {
            setParamNormalized(kTap1Enable + (i * 3), tapEnabled ? 1.0 : 0.0);
            validParameterCount++;
        } else {
            setParamNormalized(kTap1Enable + (i * 3), getDefaultParameterValue(kTap1Enable + (i * 3)));
            invalidParameterCount++;
        }

        // Tap Level
        if (streamer.readFloat(tapLevel) && isValidParameterValue(kTap1Level + (i * 3), tapLevel)) {
            setParamNormalized(kTap1Level + (i * 3), tapLevel);
            validParameterCount++;
        } else {
            float defaultValue = getDefaultParameterValue(kTap1Level + (i * 3));
            setParamNormalized(kTap1Level + (i * 3), defaultValue);
            invalidParameterCount++;
            WS_LOG_INFO("Invalid Tap" + std::to_string(i+1) + "Level, using default: " + std::to_string(defaultValue));
        }

        // Tap Pan
        if (streamer.readFloat(tapPan) && isValidParameterValue(kTap1Pan + (i * 3), tapPan)) {
            setParamNormalized(kTap1Pan + (i * 3), tapPan);
            validParameterCount++;
        } else {
            float defaultValue = getDefaultParameterValue(kTap1Pan + (i * 3));
            setParamNormalized(kTap1Pan + (i * 3), defaultValue);
            invalidParameterCount++;
            WS_LOG_INFO("Invalid Tap" + std::to_string(i+1) + "Pan, using default: " + std::to_string(defaultValue));
        }

        // Tap Filter Cutoff
        if (streamer.readFloat(tapFilterCutoff) && tapFilterCutoff >= 20.0f && tapFilterCutoff <= 20000.0f) {
            // Convert frequency to normalized value
            float normalizedCutoff = std::log(tapFilterCutoff / 20.0f) / std::log(1000.0f);
            if (isValidParameterValue(kTap1FilterCutoff + (i * 3), normalizedCutoff)) {
                setParamNormalized(kTap1FilterCutoff + (i * 3), normalizedCutoff);
                validParameterCount++;
            } else {
                float defaultValue = getDefaultParameterValue(kTap1FilterCutoff + (i * 3));
                setParamNormalized(kTap1FilterCutoff + (i * 3), defaultValue);
                invalidParameterCount++;
                WS_LOG_INFO("Invalid Tap" + std::to_string(i+1) + "FilterCutoff, using default");
            }
        } else {
            float defaultValue = getDefaultParameterValue(kTap1FilterCutoff + (i * 3));
            setParamNormalized(kTap1FilterCutoff + (i * 3), defaultValue);
            invalidParameterCount++;
            WS_LOG_INFO("Invalid Tap" + std::to_string(i+1) + "FilterCutoff frequency, using default");
        }

        // Tap Filter Resonance
        if (streamer.readFloat(tapFilterResonance) && tapFilterResonance >= -1.0f && tapFilterResonance <= 1.0f) {
            // Convert resonance to normalized value
            float normalizedResonance;
            if (tapFilterResonance >= 0.0f) {
                normalizedResonance = 0.5f + 0.5f * std::cbrt(tapFilterResonance);
            } else {
                normalizedResonance = 0.5f + tapFilterResonance / 2.0f;
            }

            if (isValidParameterValue(kTap1FilterResonance + (i * 3), normalizedResonance)) {
                setParamNormalized(kTap1FilterResonance + (i * 3), normalizedResonance);
                validParameterCount++;
            } else {
                float defaultValue = getDefaultParameterValue(kTap1FilterResonance + (i * 3));
                setParamNormalized(kTap1FilterResonance + (i * 3), defaultValue);
                invalidParameterCount++;
            }
        } else {
            float defaultValue = getDefaultParameterValue(kTap1FilterResonance + (i * 3));
            setParamNormalized(kTap1FilterResonance + (i * 3), defaultValue);
            invalidParameterCount++;
        }

        // Tap Filter Type
        if (streamer.readInt32(tapFilterType) && tapFilterType >= 0 && tapFilterType < kNumFilterTypes) {
            float normalizedType = static_cast<Vst::ParamValue>(tapFilterType) / (kNumFilterTypes - 1);
            if (isValidParameterValue(kTap1FilterType + (i * 3), normalizedType)) {
                setParamNormalized(kTap1FilterType + (i * 3), normalizedType);
                validParameterCount++;
            } else {
                float defaultValue = getDefaultParameterValue(kTap1FilterType + (i * 3));
                setParamNormalized(kTap1FilterType + (i * 3), defaultValue);
                invalidParameterCount++;
                WS_LOG_INFO("Invalid Tap" + std::to_string(i+1) + "FilterType, using default: " + std::to_string(defaultValue));
            }
        } else {
            float defaultValue = getDefaultParameterValue(kTap1FilterType + (i * 3));
            setParamNormalized(kTap1FilterType + (i * 3), defaultValue);
            invalidParameterCount++;
            WS_LOG_INFO("Invalid Tap" + std::to_string(i+1) + "FilterType value, using default: " + std::to_string(defaultValue));
        }
    }

    // Load routing and wet/dry parameters with individual validation
    int32 routeMode;
    float globalDryWet, delayDryWet;
    bool delayBypass, combBypass;

    // Route Mode
    if (streamer.readInt32(routeMode) && routeMode >= 0 && routeMode <= 2) {
        setParamNormalized(kRouteMode, static_cast<Vst::ParamValue>(routeMode) / 2.0);
        validParameterCount++;
    } else {
        setParamNormalized(kRouteMode, getDefaultParameterValue(kRouteMode));
        invalidParameterCount++;
    }

    // Global Dry/Wet
    if (streamer.readFloat(globalDryWet) && isValidParameterValue(kGlobalDryWet, globalDryWet)) {
        setParamNormalized(kGlobalDryWet, globalDryWet);
        validParameterCount++;
    } else {
        setParamNormalized(kGlobalDryWet, getDefaultParameterValue(kGlobalDryWet));
        invalidParameterCount++;
    }

    // Delay Dry/Wet
    if (streamer.readFloat(delayDryWet) && isValidParameterValue(kDelayDryWet, delayDryWet)) {
        setParamNormalized(kDelayDryWet, delayDryWet);
        validParameterCount++;
    } else {
        setParamNormalized(kDelayDryWet, getDefaultParameterValue(kDelayDryWet));
        invalidParameterCount++;
    }

    // Delay Bypass
    if (streamer.readBool(delayBypass)) {
        setParamNormalized(kDelayBypass, delayBypass ? 1.0 : 0.0);
        validParameterCount++;
    } else {
        setParamNormalized(kDelayBypass, getDefaultParameterValue(kDelayBypass));
        invalidParameterCount++;
    }

    // Comb Bypass
    if (streamer.readBool(combBypass)) {
        setParamNormalized(kCombBypass, combBypass ? 1.0 : 0.0);
        validParameterCount++;
    } else {
        setParamNormalized(kCombBypass, getDefaultParameterValue(kCombBypass));
        invalidParameterCount++;
    }

    // Log validation results
    WS_LOG_INFO("Version 1 parameter validation complete: " + std::to_string(validParameterCount) + " valid, " + std::to_string(invalidParameterCount) + " invalid (using defaults)");

    // Log key parameter values after enhanced validation
    WS_LOG_INFO("=== POST-V1-VALIDATION PARAMETER VALUES ===");

    // Log input/output gains
    WS_LOG_PARAM_CONTEXT("POST-V1-VAL", kInputGain, "InputGain", getParamNormalized(kInputGain));
    WS_LOG_PARAM_CONTEXT("POST-V1-VAL", kOutputGain, "OutputGain", getParamNormalized(kOutputGain));

    // Log first few tap levels and filter types as examples
    for (int i = 0; i < 4; i++) {
        int levelId = kTap1Level + (i * 3);
        int typeId = kTap1FilterType + (i * 3);
        int panId = kTap1Pan + (i * 3);

        std::ostringstream levelName, typeName, panName;
        levelName << "Tap" << (i+1) << "Level";
        typeName << "Tap" << (i+1) << "FilterType";
        panName << "Tap" << (i+1) << "Pan";

        WS_LOG_PARAM_CONTEXT("POST-V1-VAL", levelId, levelName.str(), getParamNormalized(levelId));
        WS_LOG_PARAM_CONTEXT("POST-V1-VAL", typeId, typeName.str(), getParamNormalized(typeId));
        WS_LOG_PARAM_CONTEXT("POST-V1-VAL", panId, panName.str(), getParamNormalized(panId));
    }

    WS_LOG_INFO("=== END POST-V1-VALIDATION PARAMETER VALUES ===");

    return kResultOk;
}

//------------------------------------------------------------------------
tresult WaterStickController::readVersionedState(IBStream* state, Steinberg::int32 version)
{
    WS_LOG_INFO("Reading versioned state (version " + std::to_string(version) + ")");

    switch (version) {
        case kStateVersionCurrent:
            // Version 1 state format - use proper version 1 reading with enhanced validation
            return readCurrentVersionState(state);

        default:
            WS_LOG_INFO("Unknown state version: " + std::to_string(version) + " - falling back to defaults");
            setDefaultParameters();
            return kResultOk;
    }

    return kResultOk;
}

//------------------------------------------------------------------------
Vst::ParamValue PLUGIN_API WaterStickController::getParamNormalized(Vst::ParamID id)
{
    Vst::ParamValue value = EditControllerEx1::getParamNormalized(id);

    // Log critical parameter queries for debugging
    bool shouldLog = false;
    std::string paramName;

    // Check if this is a problematic parameter we want to track
    if (id >= kTap1FilterType && id <= kTap16FilterType && ((id - kTap1FilterType) % 3 == 2)) {
        // Filter type parameter
        int tapIndex = (id - kTap1FilterType) / 3;
        paramName = "Tap" + std::to_string(tapIndex + 1) + "FilterType";
        shouldLog = true;
    } else if (id >= kTap1Level && id <= kTap16Level && ((id - kTap1Level) % 3 == 0)) {
        // Level parameter
        int tapIndex = (id - kTap1Level) / 3;
        paramName = "Tap" + std::to_string(tapIndex + 1) + "Level";
        shouldLog = true;
    } else if (id >= kTap1Pan && id <= kTap16Pan && ((id - kTap1Pan) % 3 == 0)) {
        // Pan parameter
        int tapIndex = (id - kTap1Pan) / 3;
        paramName = "Tap" + std::to_string(tapIndex + 1) + "Pan";
        shouldLog = true;
    } else if (id >= kTap1FilterCutoff && id <= kTap16FilterCutoff && ((id - kTap1FilterCutoff) % 3 == 0)) {
        // Filter cutoff parameter
        int tapIndex = (id - kTap1FilterCutoff) / 3;
        paramName = "Tap" + std::to_string(tapIndex + 1) + "FilterCutoff";
        shouldLog = true;
    } else if (id == kInputGain || id == kOutputGain) {
        paramName = (id == kInputGain) ? "InputGain" : "OutputGain";
        shouldLog = true;
    }

    if (shouldLog) {
        WS_LOG_PARAM_CONTEXT("GET", id, paramName, value);
    }

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
        case kFeedback:
        {
            // Convert normalized value to percentage for display
            float percentage = valueNormalized * 100.0f;
            char percentText[128];
            snprintf(percentText, sizeof(percentText), "%.1f%%", percentage);
            Steinberg::UString(string, 128).fromAscii(percentText);
            return kResultTrue;
        }
        case kRouteMode:
        {
            int routeMode = static_cast<int>(valueNormalized * 2.0 + 0.5);
            if (routeMode >= 0 && routeMode <= 2) {
                static const char* routeModeTexts[3] = {
                    "Delay>Comb", "Comb>Delay", "Delay+Comb"
                };
                Steinberg::UString(string, 128).fromAscii(routeModeTexts[routeMode]);
                return kResultTrue;
            }
            break;
        }
        case kGlobalDryWet:
        case kDelayDryWet:
        {
            // Convert normalized value to percentage for display
            float percentage = valueNormalized * 100.0f;
            char percentText[128];
            snprintf(percentText, sizeof(percentText), "%.1f%%", percentage);
            Steinberg::UString(string, 128).fromAscii(percentText);
            return kResultTrue;
        }
        case kDelayBypass:
        case kCombBypass:
        {
            const char* text = (valueNormalized > 0.5) ? "Bypassed" : "Active";
            Steinberg::UString(string, 128).fromAscii(text);
            return kResultTrue;
        }
        default:
        {
            // Handle per-tap filter parameters
            if (id >= kTap1FilterCutoff && id <= kTap16FilterType) {
                int paramOffset = id - kTap1FilterCutoff;
                int paramType = paramOffset % 3; // 0=cutoff, 1=resonance, 2=type

                switch (paramType) {
                    case 0: // Filter Cutoff
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
                    case 1: // Filter Resonance
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
                    case 2: // Filter Type
                    {
                        int filterType = static_cast<int>(valueNormalized * (kNumFilterTypes - 1) + 0.5);
                        if (filterType >= 0 && filterType < kNumFilterTypes) {
                            static const char* filterTypeTexts[kNumFilterTypes] = {
                                "Bypass", "Low Pass", "High Pass", "Band Pass", "Notch"
                            };
                            Steinberg::UString(string, 128).fromAscii(filterTypeTexts[filterType]);
                            return kResultTrue;
                        }
                        break;
                    }
                }
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
    WS_LOG_INFO("Controller::createView() called");

    if (FIDStringsEqual(name, Vst::ViewType::kEditor))
    {
        WS_LOG_INFO("=== PRE-GUI PARAMETER VALUES ===");

        // Log all problematic parameter values before GUI creation
        for (int i = 0; i < 16; i++) {
            int filterTypeId = kTap1FilterType + (i * 3);
            int levelId = kTap1Level + (i * 3);
            int panId = kTap1Pan + (i * 3);
            int cutoffId = kTap1FilterCutoff + (i * 3);

            std::ostringstream filterTypeName, levelName, panName, cutoffName;
            filterTypeName << "Tap" << (i+1) << "FilterType";
            levelName << "Tap" << (i+1) << "Level";
            panName << "Tap" << (i+1) << "Pan";
            cutoffName << "Tap" << (i+1) << "FilterCutoff";

            WS_LOG_PARAM_CONTEXT("PRE-GUI", filterTypeId, filterTypeName.str(), getParamNormalized(filterTypeId));
            WS_LOG_PARAM_CONTEXT("PRE-GUI", levelId, levelName.str(), getParamNormalized(levelId));
            WS_LOG_PARAM_CONTEXT("PRE-GUI", panId, panName.str(), getParamNormalized(panId));
            WS_LOG_PARAM_CONTEXT("PRE-GUI", cutoffId, cutoffName.str(), getParamNormalized(cutoffId));
        }

        // Log global parameters
        WS_LOG_PARAM_CONTEXT("PRE-GUI", kInputGain, "InputGain", getParamNormalized(kInputGain));
        WS_LOG_PARAM_CONTEXT("PRE-GUI", kOutputGain, "OutputGain", getParamNormalized(kOutputGain));

        WS_LOG_INFO("=== END PRE-GUI PARAMETER VALUES ===");

        auto* editor = new WaterStickEditor(this);
        WS_LOG_INFO("GUI editor created successfully");
        return editor;
    }
    return nullptr;
}

} // namespace WaterStick