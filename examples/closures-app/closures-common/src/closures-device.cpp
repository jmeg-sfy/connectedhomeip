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

void ClosuresDevice::HandleStopStimuli()
{
    ChipLogDetail(Zcl, CL_GREEN "ClosuresDevice: Stop Stimuli..." CL_CLEAR);
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


void ClosuresDevice::HandleMoveToStimuli(std::optional<uint8_t> tag, std::optional<uint8_t> speed, std::optional<uint8_t> latch)
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


void ClosuresDevice::HandleCalibrateStimuli()
{
    ChipLogDetail(Zcl, CL_GREEN "ClosuresDevice: Calibrate Stimuli..." CL_CLEAR);
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

void ClosuresDevice::HandleConfigureFallbackStimuli(std::optional<uint8_t> restingProcedure, std::optional<uint8_t> triggerCondition, 
                                                    std::optional<uint8_t> triggerPosition, std::optional<uint16_t> waitingDelay)
{
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

void ClosuresDevice::HandleProtectedStimuli()
{
    
}

void ClosuresDevice::HandleUnprotectedStimuli()
{
    
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
