#include "closures-device.h"

#include <string>

using namespace chip::app::Clusters;

/* ANSI Colored escape code */
#define CL_CLEAR  "\x1b[0m"
#define CL_RED    "\u001b[31m"
#define CL_GREEN  "\u001b[32m"
#define CL_YELLOW "\u001b[33m"

static constexpr char strLogY[] = CL_GREEN "Y" CL_CLEAR;
static constexpr char strLogN[] = CL_RED "N" CL_CLEAR;

void ClosuresDevice::Init()
{
    mOperationalStateInstance.Init();
    ChipLogDetail(NotSpecified, CL_YELLOW "!!!!! CLOSURE DEVICE INIT!!!!!!!!!" CL_CLEAR);
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

void ClosuresDevice::AddOperationalStateObserver(OperationalState::Observer * observer) {
        mOperationalStateInstance.AddObserver(observer);
}

const char* ClosuresDevice::GetStateString(ClosureOperationalState::OperationalStateEnum state) const
{
    switch (state)
    {
    case ClosureOperationalState::OperationalStateEnum::kCalibrating:
        return "Running";
    case ClosureOperationalState::OperationalStateEnum::kDisengaded:
        return "Stopped";
    case ClosureOperationalState::OperationalStateEnum::kPendingFallback:
        return "Protected";
    case ClosureOperationalState::OperationalStateEnum::kProtected:
        return "Paused";
    default:
        return "Unknown";
    }
}

// Called when the state changes, updating the state in closure device
void ClosuresDevice::OnStateChanged(ClosureOperationalState::OperationalStateEnum state)
{
    const char* state_string = GetStateString(state);
    ChipLogDetail(NotSpecified, CL_YELLOW "The new Closure Operational State value: %s" CL_CLEAR, state_string);
}

void ClosuresDevice::HandleOpStatePauseCallback(Clusters::OperationalState::GenericOperationalError & err)
{
}

void ClosuresDevice::HandleOpStateResumeCallback(Clusters::OperationalState::GenericOperationalError & err)
{
}

void ClosuresDevice::HandleOpStateStopCallback(Clusters::OperationalState::GenericOperationalError & err)
{
}

void ClosuresDevice::HandleOpStateMoveToCallback(Clusters::OperationalState::GenericOperationalError & err)
{
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

void ClosuresDevice::HandleMoveToStimuli()
{
    ChipLogDetail(Zcl, CL_GREEN "ClosuresDevice: MoveTo Stimuli..." CL_CLEAR);
    mOperationalStateInstance.HandleMoveToCommand();
}

void ClosuresDevice::HandleStopStimuli()
{
    ChipLogDetail(Zcl, CL_GREEN "ClosuresDevice: Stop Stimuli.." CL_CLEAR);
    mOperationalStateInstance.HandleStopState();
}

void ClosuresDevice::HandleDownCloseMessage()
{
    if (mOperationalStateInstance.GetCurrentOperationalState() != to_underlying(OperationalState::OperationalStateEnum::kStopped))
    {
        ChipLogError(NotSpecified,
                     "Closures App: The 'DownClose' command is only accepted when the device is in the 'Running' state.");
        return;
    }
    mOperationalStateInstance.SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kRunning));
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
