#include "closures-device.h"

#include <string>

using namespace chip::app::Clusters;

void ClosuresDevice::Init()
{
    mOperationalStateInstance.Init();
}

void ClosuresDevice::SetDeviceToStoppedState()
{
    if (mNotOperational)
    {
        mOperationalStateInstance.SetOperationalState(to_underlying(ClosureOperationalState::OperationalStateEnum::kSetupRequired));
    }
    else
    {
        mOperationalStateInstance.SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped));
    }
}


void ClosuresDevice::HandleOpStatePauseCallback(Clusters::OperationalState::GenericOperationalError & err)
{
    // This method is only called if the device is in a Pause-compatible state, i.e. `Running` or `PendingFallback`.
    mStateBeforePause = mOperationalStateInstance.GetCurrentOperationalState();
    auto error = mOperationalStateInstance.SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kPaused));
    err.Set((error == CHIP_NO_ERROR) ? to_underlying(OperationalState::ErrorStateEnum::kNoError)
                                     : to_underlying(OperationalState::ErrorStateEnum::kUnableToCompleteOperation));
}

void ClosuresDevice::HandleOpStateResumeCallback(Clusters::OperationalState::GenericOperationalError & err)
{
    if (mOperationalStateInstance.GetCurrentOperationalState() == to_underlying(OperationalState::OperationalStateEnum::kPaused))
    {
        auto error = mOperationalStateInstance.SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kPaused));
        err.Set((error == CHIP_NO_ERROR) ? to_underlying(OperationalState::ErrorStateEnum::kNoError)
                                     : to_underlying(OperationalState::ErrorStateEnum::kUnableToCompleteOperation));
    }
    else {
        // This method is only called if the device is in a resume-compatible state, i.e. 
        // `Paused`. 
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kCommandInvalidInState));
    }    
}

void ClosuresDevice::HandleOpStateStopCallback(Clusters::OperationalState::GenericOperationalError & err)
{
    switch (mOperationalStateInstance.GetCurrentOperationalState())
    {
    case to_underlying(OperationalState::OperationalStateEnum::kPaused): 
    case to_underlying(OperationalState::OperationalStateEnum::kRunning):
    case to_underlying(OperationalState::OperationalStateEnum::kError):
    case to_underlying(ClosureOperationalState::OperationalStateEnum::kCalibrating): 
    case to_underlying(ClosureOperationalState::OperationalStateEnum::kProtected): 
    case to_underlying(ClosureOperationalState::OperationalStateEnum::kDisengaded):
    case to_underlying(ClosureOperationalState::OperationalStateEnum::kPendingFallback): {

        auto error = mOperationalStateInstance.SetOperationalState(
            to_underlying(OperationalState::OperationalStateEnum::kStopped));

        err.Set((error == CHIP_NO_ERROR) ? to_underlying(OperationalState::ErrorStateEnum::kNoError)
                                         : to_underlying(OperationalState::ErrorStateEnum::kUnableToCompleteOperation));
    }
    break;
    default:
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kCommandInvalidInState));
        return;
    }   
}

void ClosuresDevice::HandleOpStateMoveToCallback(Clusters::OperationalState::GenericOperationalError & err)
{
    switch (mOperationalStateInstance.GetCurrentOperationalState())
    {
    case to_underlying(OperationalState::OperationalStateEnum::kPaused): 
    case to_underlying(OperationalState::OperationalStateEnum::kStopped):
    case to_underlying(ClosureOperationalState::OperationalStateEnum::kPendingFallback): {

        auto error = mOperationalStateInstance.SetOperationalState(
            to_underlying(OperationalState::OperationalStateEnum::kRunning));

        err.Set((error == CHIP_NO_ERROR) ? to_underlying(OperationalState::ErrorStateEnum::kNoError)
                                         : to_underlying(OperationalState::ErrorStateEnum::kUnableToCompleteOperation));
    }
    break;
    default:
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kCommandInvalidInState));
        return;
    }  
}

void ClosuresDevice::HandleCalibrationEndedMessage()
{
    if (mOperationalStateInstance.GetCurrentOperationalState() != to_underlying(ClosureOperationalState::OperationalStateEnum::kCalibrating))
    {
        ChipLogError(NotSpecified, "Closures App: The 'CalibrationEnded' command is only accepted when the device is in the 'Calibrating' state.");
        return;
    }

    mCalibrating = false;
    mOperationalStateInstance.SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped));
}

void ClosuresDevice::HandleCalibratingMessage()
{
    if (mOperationalStateInstance.GetCurrentOperationalState() != to_underlying(OperationalState::OperationalStateEnum::kStopped))
    {
        ChipLogError(NotSpecified, "Closures App: The 'Calibrate' command is only accepted when the device is in the 'Stopped' state.");
        return;
    }

    mCalibrating = true;
    mOperationalStateInstance.SetOperationalState(to_underlying(ClosureOperationalState::OperationalStateEnum::kCalibrating));
}

void ClosuresDevice::HandleEngagedMessage()
{
    if (mOperationalStateInstance.GetCurrentOperationalState() != to_underlying(OperationalState::OperationalStateEnum::kStopped))
    {
        ChipLogError(NotSpecified, "Closures App: The 'Engaged' command is only accepted when the device is in the 'Stopped' state.");
        return;
    }

    mCalibrating = true;
    mOperationalStateInstance.SetOperationalState(to_underlying(ClosureOperationalState::OperationalStateEnum::kDisengaded));
}

void ClosuresDevice::HandleDisengagedMessage()
{
    if (mOperationalStateInstance.GetCurrentOperationalState() != to_underlying(ClosureOperationalState::OperationalStateEnum::kDisengaded))
    {
        ChipLogError(NotSpecified, "Closures App: The 'Disengaged' command is only accepted when the device is in the 'Disengaged' state.");
        return;
    }

    mCalibrating = true;
    mOperationalStateInstance.SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped));
}

void ClosuresDevice::HandleProtectionRisedMessage()
{
    if (mOperationalStateInstance.GetCurrentOperationalState() != to_underlying(OperationalState::OperationalStateEnum::kStopped))
    {
        ChipLogError(NotSpecified, "Closures App: The 'ProtectionRised' command is only accepted when the device is in the 'Stopped' state.");
        return;
    }

    mCalibrating = true;
    mOperationalStateInstance.SetOperationalState(to_underlying(ClosureOperationalState::OperationalStateEnum::kProtected));
}

void ClosuresDevice::HandleProtectionDroppedMessage()
{
    if (mOperationalStateInstance.GetCurrentOperationalState() != to_underlying(ClosureOperationalState::OperationalStateEnum::kProtected))
    {
        ChipLogError(NotSpecified, "Closures App: The 'ProtectionDropped' command is only accepted when the device is in the 'Protected' state.");
        return;
    }

    mCalibrating = true;
    mOperationalStateInstance.SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped));
}

void ClosuresDevice::HandleMovementCompleteEvent()
{
    if (mOperationalStateInstance.GetCurrentOperationalState() != to_underlying(OperationalState::OperationalStateEnum::kRunning))
    {
        ChipLogError(NotSpecified,
                     "Closures App: The 'MovementComplete' command is only accepted when the device is in the 'Running' state.");
        return;
    }
    mOperationalStateInstance.SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped));
}

void ClosuresDevice::HandleErrorEvent(const std::string & error)
{
    detail::Structs::ErrorStateStruct::Type err;

    if (error == "UnableToStartOrResume")
    {
        err.errorStateID = to_underlying(OperationalState::ErrorStateEnum::kUnableToStartOrResume);
    }
    else if (error == "UnableToCompleteOperation")
    {
        err.errorStateID = to_underlying(OperationalState::ErrorStateEnum::kUnableToCompleteOperation);
    }
    else if (error == "CommandInvalidInState")
    {
        err.errorStateID = to_underlying(OperationalState::ErrorStateEnum::kCommandInvalidInState);
    }
    else if (error == "Blocked")
    {
        err.errorStateID = to_underlying(ClosureOperationalState::ErrorStateEnum::kBlocked);
    }
    else if (error == "IntegratedElement")
    {
        err.errorStateID = to_underlying(ClosureOperationalState::ErrorStateEnum::kIntegratedElement);
    }
    else if (error == "MaintenanceRequired")
    {
        err.errorStateID = to_underlying(ClosureOperationalState::ErrorStateEnum::kMaintenanceRequired);
    }
    else if (error == "ThermalProtected")
    {
        err.errorStateID = to_underlying(ClosureOperationalState::ErrorStateEnum::kThermalProtected);
    }
    else
    {
        ChipLogError(NotSpecified, "Unhandled command: The 'Error' key of the 'ErrorEvent' message is not valid.");
        return;
    }

    mOperationalStateInstance.OnOperationalErrorDetected(err);
}

void ClosuresDevice::HandleClearErrorMessage()
{
    if (mOperationalStateInstance.GetCurrentOperationalState() != to_underlying(OperationalState::OperationalStateEnum::kError))
    {
        ChipLogError(NotSpecified, "Closures App: The 'ClearError' command is only excepted when the device is in the 'Error' state.");
        return;
    }

    SetDeviceToStoppedState();
}

void ClosuresDevice::HandleResetMessage()
{
    mOperationalStateInstance.SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped));
}
