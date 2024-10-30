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
    ChipLogDetail(NotSpecified, "CLOSURE DEVICE INIT");
    // TODO check after boot if setup required
    mReadyToRun = true;
}

ClosureOperationalState::PositioningEnum ClosuresDevice::ConvertTagToPositioning(ClosureOperationalState::TagEnum aTag)
{
    switch (aTag)
    {
    case ClosureOperationalState::TagEnum::kCloseInFull:
        return ClosureOperationalState::PositioningEnum::kFullyClosed;
    
    case ClosureOperationalState::TagEnum::kOpenInFull:
        return ClosureOperationalState::PositioningEnum::kFullyOpened;

    case ClosureOperationalState::TagEnum::kOpenOneQuarter:
        return ClosureOperationalState::PositioningEnum::kOneQuarterOpened;

    case ClosureOperationalState::TagEnum::kOpenInHalf:
        return ClosureOperationalState::PositioningEnum::kHalfOpened;

    case ClosureOperationalState::TagEnum::kOpenThreeQuarter:
        return ClosureOperationalState::PositioningEnum::kThreeQuarterOpened;

    case ClosureOperationalState::TagEnum::kPedestrian:
        return ClosureOperationalState::PositioningEnum::kOpenedForPedestrian;

    case ClosureOperationalState::TagEnum::kVentilation:
        return ClosureOperationalState::PositioningEnum::kOpenedForVentilation;

    case ClosureOperationalState::TagEnum::kSignature:
        return ClosureOperationalState::PositioningEnum::kOpenedAtSignature;

    // Handle unknown or next step cases.
    case ClosureOperationalState::TagEnum::kUnknownEnumValue:
    case ClosureOperationalState::TagEnum::kSequenceNextStep:
    case ClosureOperationalState::TagEnum::kPedestrianNextStep:
    default:
        return ClosureOperationalState::PositioningEnum::kUnknownEnumValue;
    }
}

ClosureOperationalState::PositioningEnum ClosuresDevice::GetPositioning()
{
    return mPositioning;
}

ClosureOperationalState::LatchingEnum ClosuresDevice::GetLatching()
{
    return mLatching;
}

Globals::ThreeLevelAutoEnum ClosuresDevice::GetSpeed()
{
    return mSpeed;
}

ClosureOperationalState::Structs::OverallStateStruct::Type ClosuresDevice::GetOverallState() const
{
    return mOverallState;
}

void ClosuresDevice::CheckReadiness(ReadinessCheckType aType, bool & aReady)
{
    switch (aType) {
    case ReadinessCheckType::ReadyToRun:
        aReady = mReadyToRun;
        break;
    case ReadinessCheckType::ActionNeeded:
        aReady = mActionNeeded;
        break;
    case ReadinessCheckType::SetupNeeded:
        aReady = mSetupRequired;
        break;
    case ReadinessCheckType::FallbackNeeded:
        aReady = mFallbackNeeded;
        break;
    default:
        aReady = false; // Default to not ready if unknown type
        break;
    }
}

void ClosuresDevice::AddOperationalStateObserver(OperationalState::Observer * observer) {
        mOperationalStateInstance.AddObserver(observer);
}

void ClosuresDevice::SetPositioning(ClosureOperationalState::PositioningEnum aPositioning) 
{
    mPositioning = aPositioning;
}

void ClosuresDevice::SetLatching(ClosureOperationalState::LatchingEnum aLatching) 
{
    mLatching = aLatching;
}

void ClosuresDevice::SetSpeed(Globals::ThreeLevelAutoEnum aSpeed) 
{
    mSpeed = aSpeed;
}

const char * ClosuresDevice::GetStateString(ClosureOperationalState::OperationalStateEnum aOpState) const
{
    switch (aOpState)
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
    ChipLogDetail(NotSpecified, CL_GREEN "HandleOpStatePauseCallback" CL_CLEAR);
    mPaused = true;
}

void ClosuresDevice::HandleOpStateResumeCallback(Clusters::OperationalState::GenericOperationalError & err)
{
}

void ClosuresDevice::HandleOpStateStopCallback(Clusters::OperationalState::GenericOperationalError & err)
{
    // Cancel the motion simulation if it is running.
    mMotionSimulator.Cancel();

    // Optionally, notify the server that the operation was stopped.
    mOperationalStateInstance.OnClosureOperationCompletionDetected(1,OperationalState::OperationalStateEnum::kStopped, GetOverallState());
    ChipLogDetail(NotSpecified, "ClosuresDevice: Movement stopped, notified server.");

    // Set the error status for the stop operation.
    err.Set(to_underlying(Clusters::OperationalState::ErrorStateEnum::kNoError));
}

void ClosuresDevice::HandleOpStateMoveToCallback(OperationalState::GenericOperationalError & err, const chip::Optional<ClosureOperationalState::TagEnum> tag, 
                                                            const chip::Optional<Globals::ThreeLevelAutoEnum> speed, 
                                                            const chip::Optional<ClosureOperationalState::LatchingEnum> latch)
{
    ChipLogDetail(NotSpecified, CL_GREEN "HandleOpStateMoveToCallback" CL_CLEAR);
    err.Set(to_underlying(Clusters::OperationalState::ErrorStateEnum::kNoError));
    // Define the duration for the motion (5 seconds)
    mMotionSimulator.SetMoveDuration(System::Clock::Milliseconds32(5000));

    if (mReadyToRun)
    {
        mMotionSimulator.StartMotion(
        [this,tag, speed, latch]() 
        {
            if (tag.HasValue())
            {
                ClosureOperationalState::PositioningEnum newPositioning = ConvertTagToPositioning(tag.Value());
                mOverallState.positioning.SetValue(newPositioning);
                ChipLogDetail(NotSpecified, "ClosureDevice - Positioning value set to: %d", static_cast<int>(latch.Value()));
            }
            if (speed.HasValue())
            {
                mOverallState.speed.SetValue(speed.Value());
                ChipLogDetail(NotSpecified, "ClosureDevice - Speed value set to: %d", static_cast<int>(latch.Value()));
            }
            if (latch.HasValue())
            {
                mOverallState.latching.SetValue(latch.Value());
                ChipLogDetail(NotSpecified, "ClosureDevice - Latching value set to: %d", static_cast<int>(latch.Value()));
            }
            // Callback when the motion is completed
            mOperationalStateInstance.OnClosureOperationCompletionDetected(0,OperationalState::OperationalStateEnum::kStopped, GetOverallState());
            ChipLogDetail(NotSpecified, "ClosuresDevice: Movement complete, server notified.");
        },
        [this](const char * progressMessage) 
        {
            // Progress callback for motion simulation
            ChipLogDetail(NotSpecified, "ClosuresDevice: %s", progressMessage);
        });
    }
    else 
    {
        err.Set(to_underlying(Clusters::OperationalState::ErrorStateEnum::kCommandInvalidInState));
        // TODO check for specific errors
    }    
}

void ClosuresDevice::HandleOpStateCalibrateCallback(OperationalState::GenericOperationalError & err)
{
    ChipLogDetail(NotSpecified, CL_GREEN "HandleOpStateCalibrateCallback" CL_CLEAR);
    err.Set(to_underlying(Clusters::OperationalState::ErrorStateEnum::kNoError));
    // Define the duration for the motion (5 seconds)
    mMotionSimulator.SetCalibrationDuration(System::Clock::Milliseconds32(3000));

    mMotionSimulator.StartCalibration(
        [this]() 
        {
            // Completion callback for calibration simulation
            ChipLogDetail(NotSpecified, "ClosuresDevice: Calibration complete.");
            // Callback when the motion is completed
            mOperationalStateInstance.OnClosureOperationCompletionDetected(0,OperationalState::OperationalStateEnum::kStopped, GetOverallState());
        },
        [this](const char * progressMessage) 
        {
            // Progress callback for calibration simulation
            ChipLogDetail(NotSpecified, "ClosuresDevice: %s", progressMessage);
        });
}

void ClosuresDevice::ClosuresStopStimuli()
{
    ChipLogDetail(NotSpecified, CL_GREEN "ClosuresDevice: Stop Stimuli..." CL_CLEAR);
    uint16_t endpoint = 1;

    // Create a shared pointer for MockCommandHandler
    std::shared_ptr<MockCommandHandler> commandHandler = std::make_shared<MockCommandHandler>();

    // Create a ConcreteCommandPath with a unique name
    chip::app::ConcreteCommandPath localCommandPath(
        endpoint,
        chip::app::Clusters::ClosureOperationalState::Id,
        chip::app::Clusters::ClosureOperationalState::Commands::Stop::Id
    );

    // Create a buffer and TLVWriter to encode the command payload
    uint8_t buffer[128]; 
    chip::TLV::TLVWriter writer;
    writer.Init(buffer, sizeof(buffer));

    // Start a structure using AnonymousTag (better for outer containers)
    chip::TLV::TLVType containerType;
    CHIP_ERROR err = writer.StartContainer(chip::TLV::AnonymousTag(), chip::TLV::kTLVType_Structure, containerType);
    if (err != CHIP_NO_ERROR) 
    {
        ChipLogError(Zcl, "Failed to start TLV container: %s", chip::ErrorStr(err));
        return;
    }

    // End the structure
    err = writer.EndContainer(containerType);
    if (err != CHIP_NO_ERROR) {
        ChipLogError(Zcl, "Failed to end TLV container: %s", chip::ErrorStr(err));
        return;
    }

    // Initialize the reader with the written buffer and the exact length of data written.
    chip::TLV::TLVReader reader;
    reader.Init(buffer, writer.GetLengthWritten());

    // Move the reader to the first element
    err = reader.Next();
    if (err != CHIP_NO_ERROR) {
        ChipLogError(Zcl, "Failed to advance TLV reader: %s", chip::ErrorStr(err));
        return;
    }

    // Initialize HandlerContext with the appropriate constructor
    chip::app::CommandHandlerInterface::HandlerContext handlerContext(*commandHandler, localCommandPath, reader);
    mOperationalStateInstance.InvokeCommand(handlerContext);    
}


void ClosuresDevice::ClosuresMoveToStimuli(std::optional<uint8_t> tag, std::optional<uint8_t> speed, std::optional<uint8_t> latch)
{
    uint16_t endpoint = 1;

    // Create a shared pointer for MockCommandHandler
    std::shared_ptr<MockCommandHandler> commandHandler = std::make_shared<MockCommandHandler>();

    // Create a ConcreteCommandPath with a unique name
    chip::app::ConcreteCommandPath localCommandPath(
        endpoint,
        chip::app::Clusters::ClosureOperationalState::Id,
        chip::app::Clusters::ClosureOperationalState::Commands::MoveTo::Id
    );

    // Create a buffer and TLVWriter to encode the command payload
    uint8_t buffer[128]; 
    chip::TLV::TLVWriter writer;
    writer.Init(buffer, sizeof(buffer));

    // Start a structure using AnonymousTag (better for outer containers)
    chip::TLV::TLVType containerType;
    CHIP_ERROR err = writer.StartContainer(chip::TLV::AnonymousTag(), chip::TLV::kTLVType_Structure, containerType);
    if (err != CHIP_NO_ERROR) 
    {
        ChipLogError(Zcl, "Failed to start TLV container: %s", chip::ErrorStr(err));
        return;
    }

    if (tag.has_value())
    {
        // Write a field within the structure
        err = writer.Put(chip::TLV::ContextTag(0), static_cast<ClosureOperationalState::TagEnum>(*tag));
        if (err != CHIP_NO_ERROR) 
        {
            ChipLogError(Zcl, "Failed to write TLV data: %s", chip::ErrorStr(err));
            return;
        }
    }

    if (speed.has_value())
    {
        // Write a field within the structure
        err = writer.Put(chip::TLV::ContextTag(1), static_cast<Globals::ThreeLevelAutoEnum>(*speed));
        if (err != CHIP_NO_ERROR) 
        {
            ChipLogError(Zcl, "Failed to write TLV data: %s", chip::ErrorStr(err));
            return;
        }
    }

    if (latch.has_value())
    {
        // Write a field within the structure
        err = writer.Put(chip::TLV::ContextTag(2), static_cast<ClosureOperationalState::LatchingEnum>(*latch));
        if (err != CHIP_NO_ERROR) 
        {
            ChipLogError(Zcl, "Failed to write TLV data: %s", chip::ErrorStr(err));
            return;
        }
    }

    // End the structure
    err = writer.EndContainer(containerType);
    if (err != CHIP_NO_ERROR) {
        ChipLogError(Zcl, "Failed to end TLV container: %s", chip::ErrorStr(err));
        return;
    }

    // Finalize the writer
    err = writer.Finalize();
    if (err != CHIP_NO_ERROR) {
        ChipLogError(Zcl, "Failed to finalize TLV writer: %s", chip::ErrorStr(err));
        return;
    }

    // Initialize the reader with the written buffer and the exact length of data written.
    chip::TLV::TLVReader reader;
    reader.Init(buffer, writer.GetLengthWritten());

    // Move the reader to the first element
    err = reader.Next();
    if (err != CHIP_NO_ERROR) {
        ChipLogError(Zcl, "Failed to advance TLV reader: %s", chip::ErrorStr(err));
        return;
    }

    // Initialize HandlerContext with the appropriate constructor
    chip::app::CommandHandlerInterface::HandlerContext handlerContext(*commandHandler, localCommandPath, reader);
    mOperationalStateInstance.InvokeCommand(handlerContext);    
}


void ClosuresDevice::ClosuresCalibrateStimuli()
{
    ChipLogDetail(NotSpecified, CL_GREEN "ClosuresDevice: Calibrate Stimuli..." CL_CLEAR);
    uint16_t endpoint = 1;

    // Create a shared pointer for MockCommandHandler
    std::shared_ptr<MockCommandHandler> commandHandler = std::make_shared<MockCommandHandler>();

    // Create a ConcreteCommandPath with a unique name
    chip::app::ConcreteCommandPath localCommandPath(
        endpoint,
        chip::app::Clusters::ClosureOperationalState::Id,
        chip::app::Clusters::ClosureOperationalState::Commands::Calibrate::Id
    );

    // Create a buffer and TLVWriter to encode the command payload
    uint8_t buffer[128]; 
    chip::TLV::TLVWriter writer;
    writer.Init(buffer, sizeof(buffer));

    // Start a structure using AnonymousTag (better for outer containers)
    chip::TLV::TLVType containerType;
    CHIP_ERROR err = writer.StartContainer(chip::TLV::AnonymousTag(), chip::TLV::kTLVType_Structure, containerType);
    if (err != CHIP_NO_ERROR) 
    {
        ChipLogError(Zcl, "Failed to start TLV container: %s", chip::ErrorStr(err));
        return;
    }

    // End the structure
    err = writer.EndContainer(containerType);
    if (err != CHIP_NO_ERROR) {
        ChipLogError(Zcl, "Failed to end TLV container: %s", chip::ErrorStr(err));
        return;
    }

    // Initialize the reader with the written buffer and the exact length of data written.
    chip::TLV::TLVReader reader;
    reader.Init(buffer, writer.GetLengthWritten());

    // Move the reader to the first element
    err = reader.Next();
    if (err != CHIP_NO_ERROR) {
        ChipLogError(Zcl, "Failed to advance TLV reader: %s", chip::ErrorStr(err));
        return;
    }

    // Initialize HandlerContext with the appropriate constructor
    chip::app::CommandHandlerInterface::HandlerContext handlerContext(*commandHandler, localCommandPath, reader);
    mOperationalStateInstance.InvokeCommand(handlerContext);    
}

void ClosuresDevice::ClosuresConfigureFallbackStimuli(std::optional<uint8_t> restingProcedure, std::optional<uint8_t> triggerCondition, 
                                                    std::optional<uint8_t> triggerPosition, std::optional<uint16_t> waitingDelay)
{
    ChipLogDetail(NotSpecified, CL_GREEN "ClosuresDevice: Configure Fallback Stimuli..." CL_CLEAR);

    uint16_t endpoint = 1;

    // Create a shared pointer for MockCommandHandler
    std::shared_ptr<MockCommandHandler> commandHandler = std::make_shared<MockCommandHandler>();

    // Create a ConcreteCommandPath with a unique name
    chip::app::ConcreteCommandPath localCommandPath(
        endpoint,
        chip::app::Clusters::ClosureOperationalState::Id,
        chip::app::Clusters::ClosureOperationalState::Commands::ConfigureFallback::Id
    );

    // Create a buffer and TLVWriter to encode the command payload
    uint8_t buffer[128]; 
    chip::TLV::TLVWriter writer;
    writer.Init(buffer, sizeof(buffer));

    // Start a structure using AnonymousTag (better for outer containers)
    chip::TLV::TLVType containerType;
    CHIP_ERROR err = writer.StartContainer(chip::TLV::AnonymousTag(), chip::TLV::kTLVType_Structure, containerType);
    if (err != CHIP_NO_ERROR) 
    {
        ChipLogError(Zcl, "Failed to start TLV container: %s", chip::ErrorStr(err));
        return;
    }

    if (restingProcedure.has_value())
    {
        // Write a field within the structure
        err = writer.Put(chip::TLV::ContextTag(0), static_cast<ClosureOperationalState::TagEnum>(*restingProcedure));
        if (err != CHIP_NO_ERROR) 
        {
            ChipLogError(Zcl, "Failed to write TLV data: %s", chip::ErrorStr(err));
            return;
        }
    }

    if (triggerCondition.has_value())
    {
        // Write a field within the structure
        err = writer.Put(chip::TLV::ContextTag(1), static_cast<Globals::ThreeLevelAutoEnum>(*triggerCondition));
        if (err != CHIP_NO_ERROR) 
        {
            ChipLogError(Zcl, "Failed to write TLV data: %s", chip::ErrorStr(err));
            return;
        }
    }

    if (triggerPosition.has_value())
    {
        // Write a field within the structure
        err = writer.Put(chip::TLV::ContextTag(2), static_cast<ClosureOperationalState::LatchingEnum>(*triggerPosition));
        if (err != CHIP_NO_ERROR) 
        {
            ChipLogError(Zcl, "Failed to write TLV data: %s", chip::ErrorStr(err));
            return;
        }
    }

    if (waitingDelay.has_value())
    {
        // Write a field within the structure
        err = writer.Put(chip::TLV::ContextTag(3), static_cast<ClosureOperationalState::LatchingEnum>(*waitingDelay));
        if (err != CHIP_NO_ERROR) 
        {
            ChipLogError(Zcl, "Failed to write TLV data: %s", chip::ErrorStr(err));
            return;
        }
    }

    // End the structure
    err = writer.EndContainer(containerType);
    if (err != CHIP_NO_ERROR) {
        ChipLogError(Zcl, "Failed to end TLV container: %s", chip::ErrorStr(err));
        return;
    }

    // Finalize the writer
    err = writer.Finalize();
    if (err != CHIP_NO_ERROR) {
        ChipLogError(Zcl, "Failed to finalize TLV writer: %s", chip::ErrorStr(err));
        return;
    }

    // Initialize the reader with the written buffer and the exact length of data written.
    chip::TLV::TLVReader reader;
    reader.Init(buffer, writer.GetLengthWritten());

    // Move the reader to the first element
    err = reader.Next();
    if (err != CHIP_NO_ERROR) {
        ChipLogError(Zcl, "Failed to advance TLV reader: %s", chip::ErrorStr(err));
        return;
    }

    // Initialize HandlerContext with the appropriate constructor
    chip::app::CommandHandlerInterface::HandlerContext handlerContext(*commandHandler, localCommandPath, reader);
    mOperationalStateInstance.InvokeCommand(handlerContext);
}

void ClosuresDevice::ClosuresProtectedStimuli()
{
    
}

void ClosuresDevice::ClosuresUnprotectedStimuli()
{
}

void ClosuresDevice::ClosuresReadyToRunStimuli(bool aReady)
{
    ChipLogDetail(NotSpecified, CL_GREEN "ClosuresDevice: Ready to Run Stimuli to %d" CL_CLEAR, aReady);
    mReadyToRun = aReady;
}

void ClosuresDevice::ClosuresActionNeededStimuli(bool aActionNeeded)
{
    ChipLogDetail(NotSpecified, CL_GREEN "ClosuresDevice: Action Needed Stimuli to %d" CL_CLEAR, aActionNeeded);
    mActionNeeded = aActionNeeded;
}

void ClosuresDevice::ClosuresFallbackNeededStimuli(bool aFallbackNeeded)
{
    ChipLogDetail(NotSpecified, CL_GREEN "ClosuresDevice: Fallback Needed Stimuli to %d" CL_CLEAR, aFallbackNeeded);
    mFallbackNeeded = aFallbackNeeded;
}

void ClosuresDevice::ClosuresSetupRequiredStimuli(bool aSetupRequired)
{
    ChipLogDetail(NotSpecified, CL_GREEN "ClosuresDevice: Setup Required Stimuli to %d" CL_CLEAR, aSetupRequired);
    mSetupRequired = aSetupRequired;
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
}

void ClosuresDevice::HandleResetMessage()
{
    // TODO
}
