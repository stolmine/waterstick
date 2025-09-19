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
tresult PLUGIN_API WaterStickController::setComponentState(IBStream* state)
{
    WS_LOG_INFO("Controller::setComponentState() called");

    if (!state) {
        WS_LOG_INFO("No state provided - using defaults");
        setDefaultParameters();
        return kResultOk;
    }

    IBStreamer streamer(state, kLittleEndian);

    float inputGain, outputGain, delayTime, dryWet, feedback;
    bool tempoSyncMode;
    int32 syncDivision, grid;

    // Try to read state - if any read fails, fall back to defaults
    if (streamer.readFloat(inputGain) == false ||
        streamer.readFloat(outputGain) == false ||
        streamer.readFloat(delayTime) == false ||
        streamer.readFloat(dryWet) == false ||
        streamer.readFloat(feedback) == false ||
        streamer.readBool(tempoSyncMode) == false ||
        streamer.readInt32(syncDivision) == false ||
        streamer.readInt32(grid) == false) {
        // State loading failed - use defaults
        setDefaultParameters();
        return kResultOk;
    }

    setParamNormalized(kInputGain, inputGain);
    setParamNormalized(kOutputGain, outputGain);
    setParamNormalized(kDelayTime, delayTime / 2.0); // Normalize 0-2s to 0-1
    setParamNormalized(kDryWet, dryWet);
    setParamNormalized(kFeedback, feedback);
    setParamNormalized(kTempoSyncMode, tempoSyncMode ? 1.0 : 0.0);
    setParamNormalized(kSyncDivision, static_cast<Vst::ParamValue>(syncDivision) / (kNumSyncDivisions - 1));
    setParamNormalized(kGrid, static_cast<Vst::ParamValue>(grid) / (kNumGridValues - 1));

    // Load all tap parameters
    for (int i = 0; i < 16; i++) {
        bool tapEnabled;
        float tapLevel, tapPan;
        float tapFilterCutoff, tapFilterResonance;
        int32 tapFilterType;

        if (streamer.readBool(tapEnabled) == false ||
            streamer.readFloat(tapLevel) == false ||
            streamer.readFloat(tapPan) == false ||
            streamer.readFloat(tapFilterCutoff) == false ||
            streamer.readFloat(tapFilterResonance) == false ||
            streamer.readInt32(tapFilterType) == false) {
            // Tap parameter loading failed - use defaults
            setDefaultParameters();
            return kResultOk;
        }

        setParamNormalized(kTap1Enable + (i * 3), tapEnabled ? 1.0 : 0.0);
        setParamNormalized(kTap1Level + (i * 3), tapLevel);
        setParamNormalized(kTap1Pan + (i * 3), tapPan);

        // Set per-tap filter parameters
        // Inverse of logarithmic scaling: if freq = 20 * pow(1000, value), then value = log(freq/20) / log(1000)
        setParamNormalized(kTap1FilterCutoff + (i * 3), std::log(tapFilterCutoff / 20.0f) / std::log(1000.0f));

        // Inverse of resonance scaling
        float normalizedResonance;
        if (tapFilterResonance >= 0.0f) {
            // Inverse of cubic curve: value = cbrt(resonance)
            normalizedResonance = 0.5f + 0.5f * std::cbrt(tapFilterResonance);
        } else {
            // Linear mapping for negative values
            normalizedResonance = 0.5f + tapFilterResonance / 2.0f;
        }
        setParamNormalized(kTap1FilterResonance + (i * 3), normalizedResonance);
        setParamNormalized(kTap1FilterType + (i * 3), static_cast<Vst::ParamValue>(tapFilterType) / (kNumFilterTypes - 1));
    }

    // Load routing and wet/dry parameters
    int32 routeMode;
    float globalDryWet, delayDryWet;
    bool delayBypass, combBypass;

    if (streamer.readInt32(routeMode) == false ||
        streamer.readFloat(globalDryWet) == false ||
        streamer.readFloat(delayDryWet) == false ||
        streamer.readBool(delayBypass) == false ||
        streamer.readBool(combBypass) == false) {
        // Routing parameter loading failed - use defaults
        setDefaultParameters();
        return kResultOk;
    }

    setParamNormalized(kRouteMode, static_cast<Vst::ParamValue>(routeMode) / 2.0); // 0-2 routing modes
    setParamNormalized(kGlobalDryWet, globalDryWet);
    setParamNormalized(kDelayDryWet, delayDryWet);
    setParamNormalized(kDelayBypass, delayBypass ? 1.0 : 0.0);
    setParamNormalized(kCombBypass, combBypass ? 1.0 : 0.0);

    // DEBUG: Log parameter values after loading state
    std::cout << "[WaterStick DEBUG] Controller::setComponentState() completed successfully" << std::endl;
    std::cout << "[WaterStick DEBUG] Sample filter type parameter values after setComponentState:" << std::endl;
    for (int i = 0; i < 4; i++) {  // Check first 4 filter type parameters
        int paramId = kTap1FilterType + i * 3;
        Vst::ParamValue value = getParamNormalized(paramId);
        std::cout << "  Tap " << (i+1) << " FilterType (ID " << paramId << "): " << std::fixed << std::setprecision(3) << value << std::endl;
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