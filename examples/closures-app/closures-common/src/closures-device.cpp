#include "closures-device.h"
#include <string>
#include <app/clusters/closure-dimension-server/closure-dimension-server.h>

/* imgui support */
#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
#include <imgui_ui/windows/imgui-closure-dimension.h>
#include <imgui_ui/windows/imgui-closure-opstate.h>
#endif /* CHIP_IMGUI_ENABLED */


#define CLOSURES_DEVICE_LOW_SPEED       2000
#define CLOSURES_DEVICE_MEDIUM_SPEED    5000
#define CLOSURES_DEVICE_HIGH_SPEED      10000

using namespace chip::app::Clusters;

/* ANSI Colored escape code */
#define CL_CLEAR  "\x1b[0m"
#define CL_RED    "\u001b[31m"
#define CL_GREEN  "\u001b[32m"
#define CL_YELLOW "\u001b[33m"

static constexpr char strLogY[] = CL_GREEN "Y" CL_CLEAR;
static constexpr char strLogN[] = CL_RED "N" CL_CLEAR;

// Template feature Ordering: Latching, Speed, Limitation
// -> <bool FeatureLatchingEnabled, bool FeatureSpeedEnabled, bool FeatureLimitationEnabled>

using namespace chip::app::Clusters::ClosureDimension;
static Instance gLatchOnlyInstance    = ClosureLatchOnlyInstance(                       chip::EndpointId(1), Closure1stDimension::Id, LatchingAxisEnum::kVerticalTranslation);
static Instance gModulationInstance   = ClosureModulationInstance<false, false, false> (chip::EndpointId(1), Closure2ndDimension::Id, ModulationTypeEnum::kStripsAlignment);
static Instance gRotationInstance     = ClosureRotationInstance<true, false, false>    (chip::EndpointId(1), Closure3rdDimension::Id, RotationAxisEnum::kLeft, OverFlowEnum::kNoOverFlow);
static Instance gTranslationInstance  = ClosureTranslationInstance<false, false, false>(chip::EndpointId(1), Closure4thDimension::Id, TranslationDirectionEnum::kHorizontalSymmetry);
static Instance gAllFeatureTrInstance = ClosureTranslationInstance<true, true, true>   (chip::EndpointId(1), Closure5thDimension::Id, TranslationDirectionEnum::kDepthMask);

void MatterClosure1stDimensionPluginServerInitCallback() {
    ChipLogDetail(NotSpecified, "MatterClosure1stDimensionPluginServerInitCallback begin");
}

// ImGui addition
#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
void ClosuresDevice::AddImGuiClosureDimensionInstance(chip::EndpointId aEp, const char * aName, uint32_t aFeature)
{
    if (mImguiInstance && mImguiCallback)
    {
        (mImguiInstance->*mImguiCallback)(std::make_unique<example::Ui::Windows::ImGuiClosureDimension>(aEp, aName, aFeature));
    }
    else
    {
        ChipLogError(NotSpecified, "ImGuiClosureDimension cannot add window");
    }
}

void ClosuresDevice::AddImGuiClosureOpStateInstance(chip::EndpointId aEp, const char * aName, uint32_t aFeature)
{
    if (mImguiInstance && mImguiCallback)
    {
        (mImguiInstance->*mImguiCallback)(std::make_unique<example::Ui::Windows::ImGuiClosureOpState>(aEp, aName, aFeature));
    }
    else
    {
        ChipLogError(NotSpecified, "ImGuiClosureDimension cannot add window");
    }
}
#endif

void ClosuresDevice::ClosureDimensionsSetup(chip::EndpointId endpoint)
{
    ChipLogDetail(NotSpecified, "Dimensions setup start");

    // Initialize all Clusters Dimensions

    /* Latching Only */
    gLatchOnlyInstance.Init();
    gLatchOnlyInstance.SetLatchingAxis(LatchingAxisEnum::kDepthTranslation);
    chip::Optional<LatchingEnum> aLatching;
    gLatchOnlyInstance.SetTargetLatching(aLatching);
    aLatching.SetValue(LatchingEnum::kLatchedButNotSecured);
    gLatchOnlyInstance.SetCurrentLatching(aLatching);

#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
    AddImGuiClosureDimensionInstance(gLatchOnlyInstance.GetEndpoint(), gLatchOnlyInstance.GetClusterName(), gLatchOnlyInstance.GetFeatureMap());
#endif

    /* Modulation */
    gModulationInstance.Init();
    chip::Optional<chip::Percent100ths> aPositioning;
    chip::Optional<Globals::ThreeLevelAutoEnum> aSpeed;
    aPositioning.SetValue(2);
    aSpeed.SetValue(Globals::ThreeLevelAutoEnum::kMedium);
    gModulationInstance.SetUnitAndRange(UnitEnum::kDegree, 2, 5405);
    gModulationInstance.SetResolutionAndStepValue(5000, 3);
    gModulationInstance.SetTargetPositioning(aPositioning, aSpeed);
    gModulationInstance.SetCurrentPositioning(aPositioning, aSpeed);
    gModulationInstance.SetModulationType(ModulationTypeEnum::kSlatsOpenwork);

#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
    AddImGuiClosureDimensionInstance(gModulationInstance.GetEndpoint(), gModulationInstance.GetClusterName(), gModulationInstance.GetFeatureMap());
#endif

    /* Translation */
    gTranslationInstance.Init();
    gTranslationInstance.SetTranslationDirection(TranslationDirectionEnum::kCeilingSymmetry);

#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
    AddImGuiClosureDimensionInstance(gTranslationInstance.GetEndpoint(), gTranslationInstance.GetClusterName(), gTranslationInstance.GetFeatureMap());
#endif

    /* Rotation */
    gRotationInstance.Init();
    gRotationInstance.SetRotationAxis(RotationAxisEnum::kCenteredHorizontal);
    gRotationInstance.SetOverFlow(OverFlowEnum::kLeftInside);

#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
    AddImGuiClosureDimensionInstance(gRotationInstance.GetEndpoint(), gRotationInstance.GetClusterName(), gRotationInstance.GetFeatureMap());
#endif

    /* All Features Translation */
    gAllFeatureTrInstance.Init();
    chip::Optional<chip::Percent100ths> aMin;
    chip::Optional<chip::Percent100ths> aMax;
    aMax.SetValue(8000);
    gAllFeatureTrInstance.SetLimitRange(aMin, aMax);

#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
    AddImGuiClosureDimensionInstance(gAllFeatureTrInstance.GetEndpoint(), gAllFeatureTrInstance.GetClusterName(), gAllFeatureTrInstance.GetFeatureMap());
#endif

    ChipLogDetail(NotSpecified, "Dimensions setup done");
}

void ClosuresDevice::Init()
{
    mOperationalStateInstance.Init();
    ChipLogDetail(NotSpecified, "CLOSURE DEVICE INIT");

    ClosureDimensionsSetup(mOperationalStateInstance.GetEndpointId());
    // TODO check after boot if setup required
    mReadyToRun = true;

#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
    AddImGuiClosureOpStateInstance(mOperationalStateInstance.GetEndpointId(), "OpState main", mOperationalStateInstance.GetFeatureMap());
#endif
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

#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
void ClosuresDevice::AddImGuiInstance(AddWindow aCallback, example::Ui::ImguiUi * aGuiInstance)
{
    mImguiCallback = aCallback;
    mImguiInstance = aGuiInstance;
}
#endif

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
    mStopped=true;
    // Optionally, notify the server that the operation was stopped.
    mOperationalStateInstance.OnClosureOperationCompletionDetected(1, static_cast<uint8_t>(OperationalState::OperationalStateEnum::kStopped), GetOverallState());
    ChipLogDetail(NotSpecified, "ClosuresDevice: Movement stopped, notified server.");

    // Set the error status for the stop operation.
    err.Set(to_underlying(Clusters::OperationalState::ErrorStateEnum::kNoError));
}

void ClosuresDevice::HandleOpStateMoveToCallback(OperationalState::GenericOperationalError & err, const Optional<ClosureOperationalState::TagEnum> tag, 
                                                            const Optional<Globals::ThreeLevelAutoEnum> speed, 
                                                            const Optional<ClosureOperationalState::LatchingEnum> latch)
{
    ChipLogDetail(NotSpecified, CL_GREEN "HandleOpStateMoveToCallback" CL_CLEAR);
    err.Set(to_underlying(Clusters::OperationalState::ErrorStateEnum::kNoError));

    // Set the move duration based on the speed parameter
    System::Clock::Milliseconds32 moveDuration = System::Clock::Milliseconds32(CLOSURES_DEVICE_MEDIUM_SPEED); // Default value (medium speed)
    if (speed.HasValue())
    {
        switch (speed.Value())
        {
        case Globals::ThreeLevelAutoEnum::kLow:
            moveDuration = System::Clock::Milliseconds32(CLOSURES_DEVICE_LOW_SPEED); 
            break;
        case Globals::ThreeLevelAutoEnum::kMedium:
            moveDuration = System::Clock::Milliseconds32(CLOSURES_DEVICE_MEDIUM_SPEED);  
            break;
        case Globals::ThreeLevelAutoEnum::kHigh:
            moveDuration = System::Clock::Milliseconds32(CLOSURES_DEVICE_HIGH_SPEED); 
            break;
        case Globals::ThreeLevelAutoEnum::kAutomatic:
            break;
        default:
            ChipLogDetail(NotSpecified, "ClosureDevice - Unrecognized speed value, using default duration.");
            break;
        }
    }

    // Set the determined move duration
    mMotionSimulator.SetMoveDuration(moveDuration);

    if (mReadyToRun)
    {
        mRunning=true;
        mMotionSimulator.StartMotion(
        [this,tag, speed, latch]() 
        {
            if (tag.HasValue())
            {
                ClosureOperationalState::PositioningEnum newPositioning = ConvertTagToPositioning(tag.Value());
                mOverallState.positioning.SetValue(newPositioning);
                ChipLogDetail(NotSpecified, "ClosureDevice - OverallState Positioning value set to: %d", static_cast<int>(latch.Value()));
            }
            if (speed.HasValue())
            {
                mOverallState.speed.SetValue(speed.Value());
                ChipLogDetail(NotSpecified, "ClosureDevice - OverallState Speed value set to: %d", static_cast<int>(latch.Value()));
            }
            if (latch.HasValue())
            {
                mOverallState.latching.SetValue(latch.Value());
                ChipLogDetail(NotSpecified, "ClosureDevice - OverallState Latching value set to: %d", static_cast<int>(latch.Value()));
            }
            mStopped=false;
            // Callback when the motion is completed
            // TODO overallstate attribute value shall be updated before the movement?
            mOperationalStateInstance.OnClosureOperationCompletionDetected(0,static_cast<uint8_t>(OperationalState::OperationalStateEnum::kStopped), GetOverallState());
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

void ClosuresDevice::HandleOpStateConfigureFallbackCallback(
    OperationalState::GenericOperationalError & err, const Optional<ClosureOperationalState::RestingProcedureEnum> restingProcedure,
    const Optional<ClosureOperationalState::TriggerConditionEnum> triggerCondition,
    const Optional<ClosureOperationalState::TriggerPositionEnum> triggerPosition, const Optional<uint16_t> waitingDelay)
{
    if (restingProcedure.HasValue())
    {
        mRestingProcedure = static_cast<ClosureOperationalState::RestingProcedureEnum>(restingProcedure.Value());
        ChipLogDetail(NotSpecified, "RestingProcedure attribute updated to %d", static_cast<uint8_t>(mRestingProcedure));
    }

    if (triggerCondition.HasValue())
    {
        mTriggerCondition = static_cast<ClosureOperationalState::TriggerConditionEnum>(triggerCondition.Value());
        ChipLogDetail(NotSpecified, "TriggerCondition attribute updated to %d", static_cast<uint8_t>(mTriggerCondition));
    }

    if (triggerPosition.HasValue())
    {
        mTriggerPosition = static_cast<ClosureOperationalState::TriggerPositionEnum>(triggerPosition.Value());
        ChipLogDetail(NotSpecified, "TriggerPosition attribute updated to %d", static_cast<uint8_t>(mTriggerPosition));
    }

    if (waitingDelay.HasValue())
    {
        mWaitingDelay = waitingDelay.Value();
        ChipLogDetail(NotSpecified, "WaitingDelay attribute updated to %d", mWaitingDelay);
    }
    // TODO simulation
}

void chip::app::Clusters::ClosuresDevice::HandleOpStateCancelFallbackCallback(OperationalState::GenericOperationalError & err)
{
    ChipLogDetail(NotSpecified, "Cancelling current fallback and setting KickoffTimer to Zero.");
    mKickoffTimer = 0;
    mOperationalStateInstance.OnClosureOperationCompletionDetected(0,static_cast<uint8_t>(OperationalState::OperationalStateEnum::kStopped), GetOverallState());
}

void ClosuresDevice::HandleOpStateCalibrateCallback(OperationalState::GenericOperationalError & err)
{
    ChipLogDetail(NotSpecified, CL_GREEN "HandleOpStateCalibrateCallback" CL_CLEAR);
    err.Set(to_underlying(Clusters::OperationalState::ErrorStateEnum::kNoError));
    // Define the duration for the motion (5 seconds)
    System::Clock::Milliseconds32 calibrationDuration = System::Clock::Milliseconds32(5000);
    mMotionSimulator.SetCalibrationDuration(calibrationDuration);

    mCalibrating=true;
    mMotionSimulator.StartCalibration(
        [this]() 
        {
            // Completion callback for calibration simulation
            ChipLogDetail(NotSpecified, "ClosuresDevice: Calibration complete.");
            mStopped=true;
            // Callback when the motion is completed
            mOperationalStateInstance.OnClosureOperationCompletionDetected(0,static_cast<uint8_t>(OperationalState::OperationalStateEnum::kStopped), GetOverallState());
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

bool emberAfClosure1stDimensionClusterStepsCallback(chip::app::CommandHandler*, chip::app::ConcreteCommandPath const&, Commands::Steps::DecodableType const&)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}
bool emberAfClosure1stDimensionClusterLatchCallback(chip::app::CommandHandler*, chip::app::ConcreteCommandPath const&, Commands::Latch::DecodableType const&)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}

bool emberAfClosure1stDimensionClusterUnLatchCallback(chip::app::CommandHandler*, chip::app::ConcreteCommandPath const&, Commands::UnLatch::DecodableType const&)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}

using toto = chip::app::Clusters::Closure5thDimension::Commands::UnLatch::DecodableType;

bool emberAfClosure2ndDimensionClusterStepsCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, Closure2ndDimension::Commands::Steps::DecodableType const& c)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}
bool emberAfClosure2ndDimensionClusterLatchCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, Closure2ndDimension::Commands::Latch::DecodableType const& c)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}
bool emberAfClosure2ndDimensionClusterUnLatchCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, Closure2ndDimension::Commands::UnLatch::DecodableType const& c)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}
// 3
bool emberAfClosure3rdDimensionClusterStepsCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, Closure3rdDimension::Commands::Steps::DecodableType const& c)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}
bool emberAfClosure3rdDimensionClusterLatchCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, Closure3rdDimension::Commands::Latch::DecodableType const& c)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}
bool emberAfClosure3rdDimensionClusterUnLatchCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, Closure3rdDimension::Commands::UnLatch::DecodableType const& c)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}
// 4
bool emberAfClosure4thDimensionClusterStepsCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, Closure4thDimension::Commands::Steps::DecodableType const& c)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}
bool emberAfClosure4thDimensionClusterLatchCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, Closure4thDimension::Commands::Latch::DecodableType const& c)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}
bool emberAfClosure4thDimensionClusterUnLatchCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, Closure4thDimension::Commands::UnLatch::DecodableType const& c)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}
// 5
bool emberAfClosure5thDimensionClusterStepsCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, Closure5thDimension::Commands::Steps::DecodableType const& c)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}
bool emberAfClosure5thDimensionClusterLatchCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, Closure5thDimension::Commands::Latch::DecodableType const& c)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
}
bool emberAfClosure5thDimensionClusterUnLatchCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, Closure5thDimension::Commands::UnLatch::DecodableType const& c)
{
    ChipLogDetail(Zcl, "!!!!!!!!!!!!! emberAfClosure Callback neutralize me need to be removed !!!!!!!");
    return true;
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
    mOperationalStateInstance.OnClosureOperationCompletionDetected(0,static_cast<uint8_t>(ClosureOperationalState::OperationalStateEnum::kSetupRequired), GetOverallState());
}

void ClosuresDevice::HandleErrorEvent(const std::string & error)
{
    detail::Structs::ErrorStateStruct::Type err;
    ChipLogDetail(NotSpecified, CL_RED"ClosuresDevice: ERROR stimuli received: %s" CL_CLEAR, error.c_str());

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

    mError=true;
    mOperationalStateInstance.OnOperationalErrorDetected(err);
}

void ClosuresDevice::HandleClearErrorMessage()
{
    detail::Structs::ErrorStateStruct::Type err;

    if (mOperationalStateInstance.GetCurrentOperationalState() != to_underlying(OperationalState::OperationalStateEnum::kError))
    {
        ChipLogError(NotSpecified, "Closures App: The 'ClearError' command is only excepted when the device is in the 'Error' state.");
        return;
    }
    else
    {
        err.errorStateID = 0;
        mError=false;
        mOperationalStateInstance.OnOperationalErrorDetected(err);
        mOperationalStateInstance.OnClosureOperationCompletionDetected(0,static_cast<uint8_t>(OperationalState::OperationalStateEnum::kStopped), GetOverallState());
    }
}

void ClosuresDevice::HandleResetStimuli()
{
    ChipLogDetail(NotSpecified, CL_GREEN "ClosuresDevice: Reset Stimuli" CL_CLEAR);
    mStopped = true;
    mOperationalStateInstance.OnClosureOperationCompletionDetected(0,static_cast<uint8_t>(OperationalState::OperationalStateEnum::kStopped), GetOverallState());
}
