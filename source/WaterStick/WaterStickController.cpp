#include "WaterStickController.h"
#include "WaterStickCIDs.h"

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
    return kResultOk;
}

} // namespace WaterStick