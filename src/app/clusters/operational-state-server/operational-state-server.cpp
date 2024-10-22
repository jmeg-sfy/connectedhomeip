/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/****************************************************************************'
 * @file
 * @brief Implementation for the Operational State Server Cluster
 ***************************************************************************/
#include "operational-state-server.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/callback.h>
#include <app/AttributeAccessInterfaceRegistry.h>
#include <app/CommandHandlerInterfaceRegistry.h>
#include <app/EventLogging.h>
#include <app/InteractionModelEngine.h>
#include <app/reporting/reporting.h>
#include <app/util/attribute-storage.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::OperationalState;
using namespace chip::app::Clusters::OperationalState::Attributes;
using Feature = ClosureOperationalState::Feature;
using TagEnum = chip::app::Clusters::ClosureOperationalState::TagEnum;
using LatchingEnum = chip::app::Clusters::ClosureOperationalState::LatchingEnum;
using Status = Protocols::InteractionModel::Status;


/* ANSI Colored escape code */
#define CL_CLEAR  "\x1b[0m"
#define CL_RED    "\u001b[31m"
#define CL_GREEN  "\u001b[32m"
#define CL_YELLOW "\u001b[33m"

static constexpr char strLogY[] = CL_GREEN "Y" CL_CLEAR;
static constexpr char strLogN[] = CL_RED "N" CL_CLEAR;

const char * StrYN(bool isTrue)
{
    return isTrue ? strLogY : strLogN;
}

#define IsYN(TEST)  StrYN(TEST)

//Protocols::InteractionModel::Status;
template <class U>
Status VerifyOptionalEnumRange(const chip::Optional<U> & item, const char * name) //const Optional<U> & other)
{
    if (item.HasValue())
    {
        //NOTE Could be replaced with EnsureKnownEnumValue(ClosureOperationalState::MyEnum val) w/o logging
        if (item.Value() >= U::kUnknownEnumValue)
        {
            ChipLogDetail(Zcl, "%s enum " CL_RED "overflowed" CL_CLEAR " >= 0x%02u", name, static_cast<uint8_t>(U::kUnknownEnumValue));
            return Status::ConstraintError;
        }
    }
    return Status::Success;
}

template <class U>
void ChipLogOptionalValue(const chip::Optional<U> & item, const char * message, const char * name) //const Optional<U> & other)
{
    if (item.HasValue())
    {
        ChipLogDetail(Zcl, "%s %s 0x%02u", message, name, static_cast<uint16_t>(item.Value()));
    }
    else
    {
        ChipLogDetail(Zcl, "%s %s " CL_YELLOW "NotPresent" CL_CLEAR, message, name);
    }
}

Instance::Instance(Delegate * aDelegate, EndpointId aEndpointId, ClusterId aClusterId) :
    CommandHandlerInterface(MakeOptional(aEndpointId), aClusterId), AttributeAccessInterface(MakeOptional(aEndpointId), aClusterId),
    mDelegate(aDelegate), mEndpointId(aEndpointId), mClusterId(aClusterId)
{
    ChipLogDetail(Zcl, "OperationalStateServer Instance.Constructor() w/o Features: ID=0x%02lX EP=%u", long(mClusterId), mEndpointId);
    mFeatureMap = kNoFeatures;
    LogFeatureMap(mFeatureMap);

    mDelegate->SetInstance(this);
    mCountdownTime.policy()
        .Set(QuieterReportingPolicyEnum::kMarkDirtyOnIncrement)
        .Set(QuieterReportingPolicyEnum::kMarkDirtyOnChangeToFromZero);
}

Instance::Instance(Delegate * aDelegate, EndpointId aEndpointId, ClusterId aClusterId, uint32_t aFeatureMap) :
    CommandHandlerInterface(MakeOptional(aEndpointId), aClusterId), AttributeAccessInterface(MakeOptional(aEndpointId), aClusterId),
    mDelegate(aDelegate), mEndpointId(aEndpointId), mClusterId(aClusterId), mFeatureMap(aFeatureMap)
{
    ChipLogDetail(Zcl, "OperationalStateServer Instance.Constructor() w Features: ID=0x%02lX EP=%u", long(mClusterId), mEndpointId);
    LogFeatureMap(mFeatureMap);

    mDelegate->SetInstance(this);
    mCountdownTime.policy()
        .Set(QuieterReportingPolicyEnum::kMarkDirtyOnIncrement)
        .Set(QuieterReportingPolicyEnum::kMarkDirtyOnChangeToFromZero);
}

Instance::Instance(Delegate * aDelegate, EndpointId aEndpointId) : Instance(aDelegate, aEndpointId, OperationalState::Id, kAllFeatures) {}

Instance::~Instance()
{
    ChipLogDetail(Zcl, "OperationalStateServer Instance.Destructor(): ID=0x%02lX EP=%u", long(mClusterId), mEndpointId);
    CommandHandlerInterfaceRegistry::Instance().UnregisterCommandHandler(this);
    AttributeAccessInterfaceRegistry::Instance().Unregister(this);
}

CHIP_ERROR Instance::Init()
{
    // Check if the cluster has been selected in zap
	ChipLogDetail(Zcl, "OperationalStateServer Instance.Init(): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    if (!emberAfContainsServer(mEndpointId, mClusterId))
    {
        ChipLogError(Zcl, "Operational State: The cluster with ID 0x%02lX, %lu was not enabled in zap.", long(mClusterId), long(mClusterId));
        return CHIP_ERROR_INVALID_ARGUMENT;
    } else {
        ChipLogDetail(Zcl, "OperationalStateServer: ID=0x%02lX EP=%u", long(mClusterId), mEndpointId);
    }

    ReturnErrorOnFailure(CommandHandlerInterfaceRegistry::Instance().RegisterCommandHandler(this));

    VerifyOrReturnError(AttributeAccessInterfaceRegistry::Instance().Register(this), CHIP_ERROR_INCORRECT_STATE);

    return CHIP_NO_ERROR;
}

CHIP_ERROR Instance::SetCurrentPhase(const DataModel::Nullable<uint8_t> & aPhase)
{
    if (!aPhase.IsNull())
    {
        if (!IsSupportedPhase(aPhase.Value()))
        {
            return CHIP_ERROR_INVALID_ARGUMENT;
        }
    }

    DataModel::Nullable<uint8_t> oldPhase = mCurrentPhase;
    mCurrentPhase                         = aPhase;
    if (mCurrentPhase != oldPhase)
    {
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::CurrentPhase::Id);
        UpdateCountdownTimeFromClusterLogic();
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR Instance::SetOperationalState(uint8_t aOpState)
{
    // Error is only allowed to be set by OnOperationalErrorDetected.
    if (aOpState == to_underlying(OperationalStateEnum::kError) || !IsSupportedOperationalState(aOpState))
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    bool countdownTimeUpdateNeeded = false;
    // Always clear Error when changing state
    if (mOperationalError.errorStateID != to_underlying(ErrorStateEnum::kNoError))
    {
        mOperationalError.Set(to_underlying(ErrorStateEnum::kNoError));
        countdownTimeUpdateNeeded = true;
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::OperationalError::Id);
    }

    uint8_t oldState  = mOperationalState;
    mOperationalState = aOpState;
    
    if (mOperationalState != oldState)
    {
        countdownTimeUpdateNeeded = true;
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::OperationalState::Id);
    }

    // If either OpError or OpState is changed then update CountdownTime
    if (countdownTimeUpdateNeeded)
    {
        UpdateCountdownTimeFromClusterLogic();
    }
    
    ChipLogDetail(Zcl, CL_GREEN "OperationalStateServer SetOperationalState(): old=%s(%u) -> new=%s(%u) !!" CL_CLEAR, GetOperationalStateString(oldState), oldState, GetOperationalStateString(aOpState), aOpState);

    return CHIP_NO_ERROR;
}

uint32_t Instance::GetFeatureMap() const
{
    return mFeatureMap;
}

bool Instance::HasFeature(uint32_t feature)
{
    const BitMask<Feature> value = GetFeatureMap();
    if (value.Has(static_cast<Feature>(feature)))
    {
        return true;
    }

    return HasFeatureDerivedCluster(feature);
}

DataModel::Nullable<uint8_t> Instance::GetCurrentPhase() const
{
    return mCurrentPhase;
}

uint8_t Instance::GetCurrentOperationalState() const
{
    return mOperationalState;
}

void Instance::GetCurrentOperationalError(GenericOperationalError & error) const
{
    error.Set(mOperationalError.errorStateID, mOperationalError.errorStateLabel, mOperationalError.errorStateDetails);
}

void Instance::OnOperationalErrorDetected(const Structs::ErrorStateStruct::Type & aError)
{
    ChipLogDetail(Zcl, "OperationalStateServer: OnOperationalErrorDetected");
    // Set the OperationalState attribute to Error
    if (mOperationalState != to_underlying(OperationalStateEnum::kError))
    {
        mOperationalState = to_underlying(OperationalStateEnum::kError);
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::OperationalState::Id);
    }

    // Set the OperationalError attribute
    if (!mOperationalError.IsEqual(aError))
    {
        mOperationalError.Set(aError.errorStateID, aError.errorStateLabel, aError.errorStateDetails);
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::OperationalError::Id);
    }

    UpdateCountdownTimeFromClusterLogic();

    // Generate an ErrorDetected event
    GenericErrorEvent event(mClusterId, aError);
    EventNumber eventNumber;
    CHIP_ERROR error = LogEvent(event, mEndpointId, eventNumber);

    if (error != CHIP_NO_ERROR)
    {
        ChipLogError(Zcl, "OperationalStateServer: Failed to record OperationalError event: %" CHIP_ERROR_FORMAT, error.Format());
    }
}

void Instance::OnOperationCompletionDetected(uint8_t aCompletionErrorCode,
                                             const Optional<DataModel::Nullable<uint32_t>> & aTotalOperationalTime,
                                             const Optional<DataModel::Nullable<uint32_t>> & aPausedTime)
{
    ChipLogDetail(Zcl, "OperationalStateServer: OnOperationCompletionDetected");

    GenericOperationCompletionEvent event(mClusterId, aCompletionErrorCode, aTotalOperationalTime, aPausedTime);
    EventNumber eventNumber;
    CHIP_ERROR error = LogEvent(event, mEndpointId, eventNumber);

    if (error != CHIP_NO_ERROR)
    {
        ChipLogError(Zcl, "OperationalStateServer: Failed to record OperationCompletion event: %" CHIP_ERROR_FORMAT,
                     error.Format());
    }

    UpdateCountdownTimeFromClusterLogic();
}

void Instance::ReportOperationalStateListChange()
{
    MatterReportingAttributeChangeCallback(ConcreteAttributePath(mEndpointId, mClusterId, Attributes::OperationalStateList::Id));
}

void Instance::ReportPhaseListChange()
{
    MatterReportingAttributeChangeCallback(ConcreteAttributePath(mEndpointId, mClusterId, Attributes::PhaseList::Id));
    UpdateCountdownTimeFromClusterLogic();
}

void Instance::UpdateCountdownTime(bool fromDelegate)
{
    app::DataModel::Nullable<uint32_t> newCountdownTime = mDelegate->GetCountdownTime();
    auto now                                            = System::SystemClock().GetMonotonicTimestamp();

    bool markDirty = false;

    if (fromDelegate)
    {
        // Updates from delegate are reduce-reported to every 10s max (choice of this implementation), in addition
        // to default change-from-null, change-from-zero and increment policy.
        auto predicate = [](const decltype(mCountdownTime)::SufficientChangePredicateCandidate & candidate) -> bool {
            if (candidate.lastDirtyValue.IsNull() || candidate.newValue.IsNull())
            {
                return false;
            }

            uint32_t lastDirtyValue           = candidate.lastDirtyValue.Value();
            uint32_t newValue                 = candidate.newValue.Value();
            uint32_t kNumSecondsDeltaToReport = 10;
            return (newValue < lastDirtyValue) && ((lastDirtyValue - newValue) > kNumSecondsDeltaToReport);
        };
        markDirty = (mCountdownTime.SetValue(newCountdownTime, now, predicate) == AttributeDirtyState::kMustReport);
    }
    else
    {
        auto predicate = [](const decltype(mCountdownTime)::SufficientChangePredicateCandidate &) -> bool { return true; };
        markDirty      = (mCountdownTime.SetValue(newCountdownTime, now, predicate) == AttributeDirtyState::kMustReport);
    }

    if (markDirty)
    {
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::CountdownTime::Id);
    }
}

bool Instance::IsSupportedPhase(uint8_t aPhase)
{
    char buffer[kMaxPhaseNameLength];
    MutableCharSpan phase(buffer);
    if (mDelegate->GetOperationalPhaseAtIndex(aPhase, phase) != CHIP_ERROR_NOT_FOUND)
    {
        return true;
    }
    return false;
}

bool Instance::IsSupportedOperationalState(uint8_t aState)
{
    GenericOperationalState opState;
    for (uint8_t i = 0; mDelegate->GetOperationalStateAtIndex(i, opState) != CHIP_ERROR_NOT_FOUND; i++)
    {
        if (opState.operationalStateID == aState)
        {
            return true;
        }
    }
    ChipLogDetail(Zcl, "Cannot find an operational state with value %u", aState);
    return false;
}

// private

template <typename RequestT, typename FuncT>
void Instance::HandleCommand(HandlerContext & handlerContext, FuncT func)
{
    if (!handlerContext.mCommandHandled && (handlerContext.mRequestPath.mCommandId == RequestT::GetCommandId()))
    {
        RequestT requestPayload;

        // If the command matches what the caller is looking for, let's mark this as being handled
        // even if errors happen after this. This ensures that we don't execute any fall-back strategies
        // to handle this command since at this point, the caller is taking responsibility for handling
        // the command in its entirety, warts and all.
        //
        handlerContext.SetCommandHandled();
        if (DataModel::Decode(handlerContext.mPayload, requestPayload) != CHIP_NO_ERROR)
        {
            
            handlerContext.mCommandHandler.AddStatus(handlerContext.mRequestPath,
                                                     Protocols::InteractionModel::Status::InvalidCommand);
            return;
        }
        func(handlerContext, requestPayload);
    }
}

// This function is called by the interaction model engine when a command destined for this instance is received.
void Instance::InvokeCommand(HandlerContext & handlerContext)
{
    ChipLogDetail(Zcl, "OperationalState: InvokeCommand");
    switch (handlerContext.mRequestPath.mCommandId)
    {
    case Commands::Pause::Id:
        ChipLogDetail(Zcl, "OperationalState: Entering handling Pause state");

        HandleCommand<Commands::Pause::DecodableType>(
            handlerContext, [this](HandlerContext & ctx, const auto & req) { HandlePauseState(ctx, req); });
        break;

    case Commands::Resume::Id:
        ChipLogDetail(Zcl, "OperationalState: Entering handling Resume state");

        HandleCommand<Commands::Resume::DecodableType>(
            handlerContext, [this](HandlerContext & ctx, const auto & req) { HandleResumeState(ctx, req); });
        break;

    case Commands::Start::Id:
        ChipLogDetail(Zcl, "OperationalState: Entering handling Start state");

        HandleCommand<Commands::Start::DecodableType>(
            handlerContext, [this](HandlerContext & ctx, const auto & req) { HandleStartState(ctx, req); });
        break;

    case Commands::Stop::Id:
        ChipLogDetail(Zcl, "OperationalState: Entering handling Stop state");

        HandleCommand<Commands::Stop::DecodableType>(
            handlerContext, [this](HandlerContext & ctx, const auto & req) { HandleStopState(ctx, req); });
        break;
    default:
        ChipLogDetail(Zcl, "OperationalState: Entering handling derived cluster commands");
        InvokeDerivedClusterCommand(handlerContext);
        break;
    }
}

const char * Instance::GetOperationalStateString(const uint8_t & aState)
{
    switch (aState)
    {
    case to_underlying(OperationalState::OperationalStateEnum::kError): {
        return "Error";
    }

    case to_underlying(OperationalState::OperationalStateEnum::kPaused): {
        return "Paused";
    }

    case to_underlying(OperationalState::OperationalStateEnum::kRunning): {
        return "Running";
    }

    case to_underlying(OperationalState::OperationalStateEnum::kStopped): {
        return "Stopped";
    }

    default: {
        return GetDerivedClusterOperationalStateString(aState);
    }
    }
}

void Instance::LogOperationalState(const uint8_t & aState)
{
    ChipLogDetail(Zcl, "OperationalState: %s", GetOperationalStateString(aState));
}


CHIP_ERROR Instance::Read(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder)
{
    ChipLogError(Zcl, "OperationalState: Reading");
    switch (aPath.mAttributeId)
    {

    case OperationalState::Attributes::FeatureMap::Id: {
        LogFeatureMap(GetFeatureMap());
        ReturnErrorOnFailure(aEncoder.Encode(GetFeatureMap()));
        break;
    }

    case OperationalState::Attributes::OperationalStateList::Id: {
        return aEncoder.EncodeList([delegate = mDelegate](const auto & encoder) -> CHIP_ERROR {
            GenericOperationalState opState;
            size_t index   = 0;
            CHIP_ERROR err = CHIP_NO_ERROR;
            while ((err = delegate->GetOperationalStateAtIndex(index, opState)) == CHIP_NO_ERROR)
            {
                ReturnErrorOnFailure(encoder.Encode(opState));
                index++;
            }
            if (err == CHIP_ERROR_NOT_FOUND)
            {
                return CHIP_NO_ERROR;
            }
            return err;
        });
        break;
    }

    case OperationalState::Attributes::OperationalState::Id: {
        uint8_t opState = GetCurrentOperationalState();
        LogOperationalState(opState);
        ReturnErrorOnFailure(aEncoder.Encode(opState));
        break;
    }

    case OperationalState::Attributes::OperationalError::Id: {
        ReturnErrorOnFailure(aEncoder.Encode(mOperationalError));
        break;
    }

    case OperationalState::Attributes::PhaseList::Id: {

        char buffer[kMaxPhaseNameLength];
        MutableCharSpan phase(buffer);
        size_t index = 0;

        if (mDelegate->GetOperationalPhaseAtIndex(index, phase) == CHIP_ERROR_NOT_FOUND)
        {
            return aEncoder.EncodeNull();
        }

        return aEncoder.EncodeList([delegate = mDelegate](const auto & encoder) -> CHIP_ERROR {
            for (uint8_t i = 0; true; i++)
            {
                char buffer2[kMaxPhaseNameLength];
                MutableCharSpan phase2(buffer2);
                auto err = delegate->GetOperationalPhaseAtIndex(i, phase2);
                if (err == CHIP_ERROR_NOT_FOUND)
                {
                    return CHIP_NO_ERROR;
                }
                ReturnErrorOnFailure(err);
                ReturnErrorOnFailure(encoder.Encode(phase2));
            }
        });
        break;
    }

    case OperationalState::Attributes::CurrentPhase::Id: {
        ReturnErrorOnFailure(aEncoder.Encode(GetCurrentPhase()));
        break;
    }

    case OperationalState::Attributes::CountdownTime::Id: {
        // Read through to get value closest to reality.
        ReturnErrorOnFailure(aEncoder.Encode(mDelegate->GetCountdownTime()));
        break;
    }
    default: {
        ChipLogDetail(Zcl, "OperationalState: Entering handling derived cluster attributes");
        //         TLV::TLVReader reader;
        // reader.Init(tlvBuffer);

		// tlvReader = ReadEncodedValue(value);AttributeValueEncoder & aEncoder
		//    DataModelLogger::LogAttribute(aPath, )
        return ReadDerivedClusterAttribute(aPath, aEncoder);
        break;
    }
    }
    return CHIP_NO_ERROR;
}


// Notify all registered observers about the state change
void ClosureOperationalState::Instance::NotifyObservers()
{
    for (OperationalState::Observer* observer : mObservers)
    {
        if (observer)
        {
            observer->OnStateChanged(mCurrentState);
        }
    }
}

// Private method to set state and notify all observers
void ClosureOperationalState::Instance::SetState(ClosureOperationalState::OperationalStateEnum newState)
{
    if (mCurrentState != newState)
    {
        mCurrentState = newState;
        //SetOperationalState(to_underlying(newState));

        // Notify all registered observers
        NotifyObservers();
    }
}


void Instance::LogFeatureMap(const uint32_t & featureMap)
{
    LogDerivedClusterFeatureMap(featureMap);
}

void Instance::HandlePauseState(HandlerContext & ctx, const Commands::Pause::DecodableType & req)
{
    ChipLogDetail(Zcl, "OperationalState: HandlePauseState");

    GenericOperationalError err(to_underlying(ErrorStateEnum::kNoError));
    uint8_t opState = GetCurrentOperationalState();

    // Handle Operational State Pause-incompatible states.
    if (opState == to_underlying(OperationalStateEnum::kStopped) || opState == to_underlying(OperationalStateEnum::kError))
    {
        err.Set(to_underlying(ErrorStateEnum::kCommandInvalidInState));
    }

    // Handle Pause-incompatible states for derived clusters.
    if (opState >= DerivedClusterNumberSpaceStart && opState < VendorNumberSpaceStart)
    {
        if (!IsDerivedClusterStatePauseCompatible(opState))
        {
            err.Set(to_underlying(ErrorStateEnum::kCommandInvalidInState));
        }
    }

    // If the error is still NoError, we can call the delegate's handle function.
    // If the current state is Paused we can skip this call.
    if (err.errorStateID == 0 && opState != to_underlying(OperationalStateEnum::kPaused))
    {
        mDelegate->HandlePauseStateCallback(err);
    }

    Commands::OperationalCommandResponse::Type response;
    response.commandResponseState = err;

    ctx.mCommandHandler.AddResponse(ctx.mRequestPath, response);
}

void Instance::HandleStopState(HandlerContext & ctx, const Commands::Stop::DecodableType & req)
{
    ChipLogDetail(Zcl, "OperationalState: HandleStopState");

    GenericOperationalError err(to_underlying(ErrorStateEnum::kNoError));
    uint8_t opState = GetCurrentOperationalState();

    if (opState != to_underlying(OperationalStateEnum::kStopped))
    {
        mDelegate->HandleStopStateCallback(err);
    }

    Commands::OperationalCommandResponse::Type response;
    response.commandResponseState = err;

    ctx.mCommandHandler.AddResponse(ctx.mRequestPath, response);
}

void Instance::HandleStopState()
{
    ChipLogDetail(Zcl, "OperationalState: HandleStopState");
}

void Instance::HandleStartState(HandlerContext & ctx, const Commands::Start::DecodableType & req)
{
    ChipLogDetail(Zcl, "OperationalState: HandleStartState");

    GenericOperationalError err(to_underlying(ErrorStateEnum::kNoError));
    uint8_t opState = GetCurrentOperationalState();

    if (opState != to_underlying(OperationalStateEnum::kRunning))
    {
        mDelegate->HandleStartStateCallback(err);
    }

    Commands::OperationalCommandResponse::Type response;
    response.commandResponseState = err;

    ctx.mCommandHandler.AddResponse(ctx.mRequestPath, response);
}

void Instance::HandleResumeState(HandlerContext & ctx, const Commands::Resume::DecodableType & req)
{
    ChipLogDetail(Zcl, "OperationalState: HandleResumeState");

    GenericOperationalError err(to_underlying(ErrorStateEnum::kNoError));
    uint8_t opState = GetCurrentOperationalState();

    // Handle Operational State Resume-incompatible states.
    if (opState == to_underlying(OperationalStateEnum::kStopped) || opState == to_underlying(OperationalStateEnum::kError))
    {
        err.Set(to_underlying(ErrorStateEnum::kCommandInvalidInState));
    }

    // Handle Resume-incompatible states for derived clusters.
    if (opState >= DerivedClusterNumberSpaceStart && opState < VendorNumberSpaceStart)
    {
        if (!IsDerivedClusterStateResumeCompatible(opState))
        {
            err.Set(to_underlying(ErrorStateEnum::kCommandInvalidInState));
        }
    }

    // If the error is still NoError, we can call the delegate's handle function.
    // If the current state is Running we can skip this call.
    if (err.errorStateID == 0 && opState != to_underlying(OperationalStateEnum::kRunning))
    {
        mDelegate->HandleResumeStateCallback(err);
    }

    Commands::OperationalCommandResponse::Type response;
    response.commandResponseState = err;

    ctx.mCommandHandler.AddResponse(ctx.mRequestPath, response);
}

// ###################################################
// RvcOperationalState

bool RvcOperationalState::Instance::IsDerivedClusterStatePauseCompatible(uint8_t aState)
{
    return aState == to_underlying(RvcOperationalState::OperationalStateEnum::kSeekingCharger);
}

bool RvcOperationalState::Instance::IsDerivedClusterStateResumeCompatible(uint8_t aState)
{
    return (aState == to_underlying(RvcOperationalState::OperationalStateEnum::kCharging) ||
            aState == to_underlying(RvcOperationalState::OperationalStateEnum::kDocked));
}

// This function is called by the base operational state cluster when a command in the derived cluster number-space is received.
void RvcOperationalState::Instance::InvokeDerivedClusterCommand(chip::app::CommandHandlerInterface::HandlerContext & handlerContext)
{
    ChipLogDetail(Zcl, "RvcOperationalState: InvokeDerivedClusterCommand");
    switch (handlerContext.mRequestPath.mCommandId)
    {
    case RvcOperationalState::Commands::GoHome::Id:
        ChipLogDetail(Zcl, "RvcOperationalState: Entering handling GoHome command");

        CommandHandlerInterface::HandleCommand<Commands::GoHome::DecodableType>(
            handlerContext, [this](HandlerContext & ctx, const auto & req) { HandleGoHomeCommand(ctx, req); });
        break;
    }
}

void RvcOperationalState::Instance::HandleGoHomeCommand(HandlerContext & ctx, const Commands::GoHome::DecodableType & req)
{
    ChipLogDetail(Zcl, "RvcOperationalState: HandleGoHomeCommand");

    GenericOperationalError err(to_underlying(OperationalState::ErrorStateEnum::kNoError));
    uint8_t opState = GetCurrentOperationalState();

    // Handle the case of the device being in an invalid state
    if (opState == to_underlying(OperationalStateEnum::kCharging) || opState == to_underlying(OperationalStateEnum::kDocked))
    {
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kCommandInvalidInState));
    }

    if (err.errorStateID == 0 && opState != to_underlying(OperationalStateEnum::kSeekingCharger))
    {
        mDelegate->HandleGoHomeCommandCallback(err);
    }

    Commands::OperationalCommandResponse::Type response;
    response.commandResponseState = err;

    ctx.mCommandHandler.AddResponse(ctx.mRequestPath, response);
}

// ###################################################
// ClosureOperationalState

ClosureOperationalState::Instance::~Instance()
{
    ChipLogDetail(Zcl, "ClosureOperationalStateServer Instance.Destructor()");
}

CHIP_ERROR ClosureOperationalState::Instance::UpdateActionState(void)
{
    if (mDelegate->IsReadyToRunCallback())
    {
        SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kRunning));
    }
    else
    {
        SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kPaused));
    }
    return CHIP_NO_ERROR;
}

ClosureOperationalState::Structs::OverallStateStruct::Type ClosureOperationalState::Instance::GetCurrentOverallState() const
{
    return mOverallState;
}

chip::DurationS ClosureOperationalState::Instance::GetCurrentWaitingDelay() const
{
    return mWaitingDelay;
}

bool ClosureOperationalState::Instance::IsDerivedClusterStatePauseCompatible(uint8_t aState)
{
    return aState == to_underlying(ClosureOperationalState::OperationalStateEnum::kDisengaded);
}

bool ClosureOperationalState::Instance::IsDerivedClusterStateResumeCompatible(uint8_t aState)
{
    return (aState == to_underlying(ClosureOperationalState::OperationalStateEnum::kProtected) ||
            aState == to_underlying(ClosureOperationalState::OperationalStateEnum::kSetupRequired));
}

// This function is called by the base operational state cluster when a command in the derived cluster number-space is received.
void ClosureOperationalState::Instance::InvokeDerivedClusterCommand(chip::app::CommandHandlerInterface::HandlerContext & handlerContext)
{
    ChipLogDetail(Zcl, "ClosureOperationalState: InvokeDerivedClusterCommand");
    auto commandId = handlerContext.mRequestPath.mCommandId;
    switch (commandId)
    {
    case ClosureOperationalState::Commands::Calibrate::Id:
        ChipLogDetail(Zcl, "ClosureOperationalState: Entering handling Calibrate command");

        CommandHandlerInterface::HandleCommand<Commands::Calibrate::DecodableType>(
            handlerContext, [this](HandlerContext & ctx, const auto & req) { HandleCalibrateCommand(ctx, req); });
        break;
    case ClosureOperationalState::Commands::MoveTo::Id:
        ChipLogDetail(Zcl, "ClosureOperationalState: Entering handling MoveTo command");

        CommandHandlerInterface::HandleCommand<Commands::MoveTo::DecodableType>(
            handlerContext, [this](HandlerContext & ctx, const auto & req) { HandleMoveToCommand(ctx, req); });
        break;
    case ClosureOperationalState::Commands::ConfigureFallback::Id:
        ChipLogDetail(Zcl, "ClosureOperationalState: Entering handling ConfigureFallback command");

        CommandHandlerInterface::HandleCommand<Commands::ConfigureFallback::DecodableType>(
            handlerContext, [this](HandlerContext & ctx, const auto & req) { HandleConfigureFallbackCommand(ctx, req); });
        break;
    default:
        ChipLogProgress(Zcl, "ClosureOperationalState: WARNING CommandId=0x%02X Invoke is not handled !", commandId);
        handlerContext.mCommandHandler.AddStatus(handlerContext.mRequestPath, Status::InvalidCommand);
    }
}

void ClosureOperationalState::LogOverallStateStruct(const Structs::OverallStateStruct::Type & aOverallState)
{
    ChipLogOptionalValue(aOverallState.positioning, "    -", "Positioning");
    ChipLogOptionalValue(aOverallState.speed      , "    -", "Speed");
    ChipLogOptionalValue(aOverallState.latching   , "    -", "Latching");
}

void ClosureOperationalState::LogMoveToRequest(const Commands::MoveTo::DecodableType & req)
{
    ChipLogOptionalValue(req.tag  , "    -", "Tag");
    ChipLogOptionalValue(req.latch, "    -", "Latch");
    ChipLogOptionalValue(req.speed, "    -", "Speed");
}

void ClosureOperationalState::LogConfigureFallbackRequest(const Commands::ConfigureFallback::DecodableType & req)
{
    ChipLogOptionalValue(req.restingProcedure, "    -", "RestingProcedure");
    ChipLogOptionalValue(req.triggerPosition , "    -", "TriggerPosition");
    ChipLogOptionalValue(req.triggerCondition, "    -", "TriggerCondition");
    ChipLogOptionalValue(req.waitingDelay    , "    -", "WaitingDelay");
}

void ClosureOperationalState::Instance::LogDerivedClusterFeatureMap(const uint32_t & featureMap)
{
    const BitMask<Feature> value = featureMap;

    ChipLogDetail(NotSpecified, "ClosureOperationalState::FeatureMap=0x%08X (%u)", value.Raw(), value.Raw());

    LogIsFeatureSupported(featureMap, Feature::kPositioning);
    LogIsFeatureSupported(featureMap, Feature::kLatching);
    LogIsFeatureSupported(featureMap, Feature::kIntermediatePositioning);
    LogIsFeatureSupported(featureMap, Feature::kSpeed);
    LogIsFeatureSupported(featureMap, Feature::kVentilation);
    LogIsFeatureSupported(featureMap, Feature::kPedestrian);
    LogIsFeatureSupported(featureMap, Feature::kCalibration);
    LogIsFeatureSupported(featureMap, Feature::kProtection);
    LogIsFeatureSupported(featureMap, Feature::kManuallyOperable);
    LogIsFeatureSupported(featureMap, Feature::kFallback);
}

inline void ClosureOperationalState::LogIsFeatureSupported(const uint32_t & featureMap, Feature aFeature)
{
    const BitMask<Feature> value = featureMap;
    ChipLogDetail(NotSpecified, " %-20s [%s]", GetFeatureMapString(aFeature), IsYN(value.Has(aFeature)));
}

const char * ClosureOperationalState::GetFeatureMapString(Feature aFeature)
{
    switch (aFeature)
    {
    case Feature::kPositioning:
        return "Positioning";
    case Feature::kLatching:
        return "Latching";
    case Feature::kIntermediatePositioning:
        return "IntermediatePos";
    case Feature::kSpeed:
        return "Speed";
    case Feature::kVentilation:
        return "Ventilation";
    case Feature::kPedestrian:
        return "Pedestrian";
    case Feature::kCalibration:
        return "Calibration";
    case Feature::kProtection:
        return "Protection";
    case Feature::kManuallyOperable:
        return "ManuallyOperable";
    case Feature::kFallback:
        return "Fallback";
    default:
        return "Unknown";
    }
}

const char * ClosureOperationalState::Instance::GetDerivedClusterOperationalStateString(const uint8_t & aState)
{
    switch (aState)
    {
    case to_underlying(ClosureOperationalState::OperationalStateEnum::kCalibrating): {
        return "Calibrating";
    }

    case to_underlying(ClosureOperationalState::OperationalStateEnum::kDisengaded): {
        return "Disengaded";
    }

    case to_underlying(ClosureOperationalState::OperationalStateEnum::kPendingFallback): {
        return "PendingFallback";
    }

    case to_underlying(ClosureOperationalState::OperationalStateEnum::kProtected): {
        return "Protected";
    }

    case to_underlying(ClosureOperationalState::OperationalStateEnum::kSetupRequired): {
        return "SetupRequired";
    }

    default: {
        return "Unknown";
    }
    }
}

bool ClosureOperationalState::Instance::HasFeatureDerivedCluster(uint32_t feature)
{
    const BitMask<Feature> value = GetFeatureMap();
    return value.Has(static_cast<Feature>(feature));
}

// This function is called by the base operational state cluster when a command in the derived cluster number-space is received.
CHIP_ERROR ClosureOperationalState::Instance::ReadDerivedClusterAttribute(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder)
{
    ChipLogDetail(Zcl, "ClosureOperationalState: ReadDerivedClusterAttribute");

    switch (aPath.mAttributeId)
    {
    case ClosureOperationalState::Attributes::OverallState::Id: {
        LogOverallStateStruct(GetCurrentOverallState());
        ReturnErrorOnFailure(aEncoder.Encode(GetCurrentOverallState()));
        break;
    }

    case ClosureOperationalState::Attributes::RestingProcedure::Id: {
        ChipLogDetail(Zcl, "ClosureOperationalState: Reading RestingProcedure");
        // Read through to get value closest to reality.
        ReturnErrorOnFailure(aEncoder.Encode(mDelegate->GetCountdownTime()));
        break;
    }

    case ClosureOperationalState::Attributes::TriggerPosition::Id: {
        ChipLogDetail(Zcl, "ClosureOperationalState: Reading TriggerPosition");
        ReturnErrorOnFailure(aEncoder.Encode(GetCurrentPhase()));
        break;
    }

    case ClosureOperationalState::Attributes::TriggerCondition::Id: {
        ChipLogDetail(Zcl, "ClosureOperationalState: Reading TriggerCondition");
        ReturnErrorOnFailure(aEncoder.Encode(GetCurrentPhase()));
        break;
    }

    case ClosureOperationalState::Attributes::WaitingDelay::Id: {
        ChipLogDetail(Zcl, "ClosureOperationalState: Reading WaitingDelay");
        // Feature is Enable use the Delegate Callback to update the front-attributes
        if (HasFeature(to_underlying(Feature::kFallback)))
        {
            ReturnErrorOnFailure(aEncoder.Encode(GetCurrentWaitingDelay()));
        }
        else
        {
            return CHIP_IM_GLOBAL_STATUS(UnsupportedAttribute);
        }
        break;
    }

    case ClosureOperationalState::Attributes::KickoffTimer::Id: {
        ChipLogDetail(Zcl, "ClosureOperationalState: Reading KickoffTimer");
        // Read through to get value closest to reality.
        ReturnErrorOnFailure(aEncoder.Encode(mDelegate->GetKickoffTimer()));
        break;
    }

    default: {
        ChipLogProgress(Zcl, "ClosureOperationalState: WARNING Attribute=0x%02X Read is not handled !", aPath.mAttributeId);
    }
    }
    return CHIP_NO_ERROR;
}


Status ClosureOperationalState::VerifyFieldTag(const chip::Optional<TagEnum> & item, const uint32_t & featureMap)
{
    const BitMask<Feature> featureBitMask = featureMap;

    Status status;
    if ((status = VerifyOptionalEnumRange(item, "Tag")) != Status::Success)
    {
        return status;
    }

    if (item.HasValue())
    {
        if (!featureBitMask.Has(Feature::kIntermediatePositioning))
        {
            if (item.Value() == TagEnum::kOpenOneQuarter ||
                item.Value() == TagEnum::kOpenInHalf ||
                item.Value() == TagEnum::kOpenThreeQuarter)
            {
                LogIsFeatureSupported(featureBitMask.Raw(), Feature::kIntermediatePositioning);
                return Status::NotFound;
            }
        }

        if (!featureBitMask.Has(Feature::kPedestrian))
        {
            if (item.Value() == TagEnum::kPedestrian || item.Value() == TagEnum::kPedestrianNextStep)
            {
                LogIsFeatureSupported(featureBitMask.Raw(), Feature::kPedestrian);
                return Status::NotFound;
            }
        }

        if (!featureBitMask.Has(Feature::kVentilation))
        {
            if (item.Value() == TagEnum::kVentilation)
            {
                LogIsFeatureSupported(featureBitMask.Raw(), Feature::kVentilation);
                return Status::NotFound;
            }
        }
    }

    return Status::Success;
}

Status ClosureOperationalState::VerifyFieldLatch(const chip::Optional<LatchingEnum> & item)
{
    Status status;

    if ((status = VerifyOptionalEnumRange(item, "Latch")) != Status::Success)
    {
        return status;
    }

    if (item.HasValue())
    {
        if (item.Value() == LatchingEnum::kLatchedButNotSecured)
        {
            ChipLogDetail(NotSpecified, "LatchedButNotSecured " CL_RED "disallowed" CL_CLEAR);
            return Status::ConstraintError;
        }
    }

    return Status::Success;
}

Status ClosureOperationalState::VerifyFieldSpeed(const chip::Optional<chip::app::Clusters::Globals::ThreeLevelAutoEnum> & item)
{
    Status status;

    if ((status = VerifyOptionalEnumRange(item, "Speed")) != Status::Success)
    {
        return status;
    }

    return Status::Success;
}

Status ClosureOperationalState::VerifyFieldRestingProcedure(const chip::Optional<RestingProcedureEnum>  & item)
{
    Status status;

    if ((status = VerifyOptionalEnumRange(item, "RestingProcedure")) != Status::Success)
    {
        return status;
    }

    return Status::Success;
}

Status ClosureOperationalState::VerifyFieldTriggerPosition(const chip::Optional<TriggerPositionEnum> & item)
{
    Status status;

    if ((status = VerifyOptionalEnumRange(item, "TriggerPosition")) != Status::Success)
    {
        return status;
    }

    return Status::Success;
}

Status ClosureOperationalState::VerifyFieldTriggerCondition(const chip::Optional<TriggerConditionEnum> & item)
{
    Status status;

    if ((status = VerifyOptionalEnumRange(item, "TriggerCondition")) != Status::Success)
    {
        return status;
    }

    return Status::Success;
}

Status ClosureOperationalState::VerifyFieldWaitingDelay(const chip::Optional<chip::DurationS> & item)
{
    if (item.HasValue())
    {
        if (item.Value() > kMaxDurationS)
        {
            ChipLogDetail(NotSpecified, "WaitingDelay " CL_RED "> %u" CL_CLEAR, kMaxDurationS);
            return Status::ConstraintError;
        }
    }

    return Status::Success;
}

bool ClosureOperationalState::IsStopInvalidInState(const uint8_t & aOpState)
{
    switch (aOpState)
    {
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kStopped):
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kPaused):
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kRunning):
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kError):
        case to_underlying(OperationalStateEnum::kCalibrating):
        case to_underlying(OperationalStateEnum::kPendingFallback):
        return false;
        break;
    default:
        break;
    }

    return true;
}

bool ClosureOperationalState::IsCalibrateInvalidInState(const uint8_t & aOpState)
{
    switch (aOpState)
    {
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kStopped):
        case to_underlying(OperationalStateEnum::kCalibrating):
        return false;
        break;
    default:
        break;
    }

    return true;
}

bool ClosureOperationalState::IsMoveToInvalidInState(const uint8_t & aOpState)
{
    switch (aOpState)
    {
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kStopped):
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kRunning):
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kPaused):
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kError):
        case to_underlying(OperationalStateEnum::kPendingFallback):
        return false;
        break;
    default:
        break;
    }

    return true;
}

bool ClosureOperationalState::IsConfigureFallbackInvalidInState(const uint8_t & aOpState)
{
    switch (aOpState)
    {
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kStopped):
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kRunning):
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kPaused):
        case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kError):
        case to_underlying(OperationalStateEnum::kCalibrating):
        case to_underlying(OperationalStateEnum::kProtected):
        case to_underlying(OperationalStateEnum::kDisengaded):
        return false;
        break;
    default:
        break;
    }

    return true;
}

void ClosureOperationalState::Instance::HandleStopState(HandlerContext & ctx, const OperationalState::Commands::Stop::DecodableType & req)
{
    ChipLogDetail(Zcl, "ClosureOperationalState: HandleStopState");

    GenericOperationalError err(to_underlying(OperationalState::ErrorStateEnum::kNoError));
    uint8_t newState = to_underlying(OperationalState::OperationalStateEnum::kStopped);

    /* NOTE: Compare to the Base::Stop Method -> ClosureOperationalState::Stop Method doesn't answer with a InvokeResponse but with a regular Status */
    // Handle the case of the device being in an invalid state
    if (IsStopInvalidInState(GetCurrentOperationalState()))
    {
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kCommandInvalidInState));
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Status::InvalidInState);
        return;
    }

    SetOperationalState(newState);
    // Command is Sane for delegation
    mDelegate->HandleStopStateCallback(err);
    if (err.errorStateID == to_underlying(OperationalState::ErrorStateEnum::kNoError))
    {
        ChipLogDetail(Zcl, "OnStop");
    }

    ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Status::Success);
}

void ClosureOperationalState::Instance::HandleCalibrateCommand(HandlerContext & ctx, const Commands::Calibrate::DecodableType & req)
{
    ChipLogDetail(Zcl, "ClosureOperationalState: HandleCalibrateCommand");

    GenericOperationalError err(to_underlying(OperationalState::ErrorStateEnum::kNoError));
    uint8_t newState = to_underlying(ClosureOperationalState::OperationalStateEnum::kCalibrating);

    // Handle the case of the device being in an invalid state
    if (IsCalibrateInvalidInState(GetCurrentOperationalState()))
    {
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kCommandInvalidInState));
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Status::InvalidInState);
        return;
    }


    // Feature is Enable use the Delegate Callback to update the front-attributes
    if (HasFeature(to_underlying(Feature::kCalibration)))
    {
		SetOperationalState(newState);
        // Command is Sane for delegation
        mDelegate->HandleCalibrateCommandCallback(err);
        if (err.errorStateID == to_underlying(OperationalState::ErrorStateEnum::kNoError))
        {
            ChipLogDetail(Zcl, "NEED TO IMPLEMENT attribute update");
        }
    }
    else
    {
        //LogIsFeatureSupported(fakeFeature.Raw(), Feature::kCalibration);
    }
    ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Status::Success);
}

void ClosureOperationalState::Instance::HandleMoveToCommand(HandlerContext & ctx, const Commands::MoveTo::DecodableType & req)
{
    ChipLogDetail(Zcl, "ClosureOperationalState: HandleMoveToCommand");

    GenericOperationalError err(to_underlying(OperationalState::ErrorStateEnum::kNoError));
    uint8_t newState = to_underlying(OperationalState::OperationalStateEnum::kRunning);

    ChipLogDetail(Zcl, "ClosureOperationalState: HandleMoveToCommand Fields:");
    LogMoveToRequest(req);

    Status status;
    if ((status = VerifyFieldTag(req.tag, GetFeatureMap())) != Status::Success)
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, status);
        return;
    }

    if ((status = VerifyFieldLatch(req.latch)) != Status::Success)
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, status);
        return;
    }

    if ((status = VerifyFieldSpeed(req.speed)) != Status::Success)
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, status);
        return;
    }

    // At least one field SHALL be available
    if (!(req.tag.HasValue() || req.latch.HasValue() || req.speed.HasValue()))
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Status::InvalidCommand);
        return;
    }

    // Handle the case of the device being in an invalid state
    if (IsMoveToInvalidInState(GetCurrentOperationalState()))
    {
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kCommandInvalidInState));
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Status::InvalidInState);
        return;
    }

    SetOperationalState(newState);
    // Command is Sane for delegation
    mDelegate->HandleMoveToCommandCallback(err);

    if (err.errorStateID == to_underlying(OperationalState::ErrorStateEnum::kNoError))
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Status::Success);
    } else {
        ChipLogDetail(Zcl, "ClosureOperationalState: HandleMoveToCommand Error NO ANSWER %u", err.errorStateID);
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Status::Success);
    }
}


void ClosureOperationalState::Instance::HandleConfigureFallbackCommand(HandlerContext & ctx, const Commands::ConfigureFallback::DecodableType & req)
{
    ChipLogDetail(Zcl, "ClosureOperationalState: HandleConfigureFallbackCommand");

    GenericOperationalError err(to_underlying(OperationalState::ErrorStateEnum::kNoError));
    uint8_t newState = to_underlying(ClosureOperationalState::OperationalStateEnum::kPendingFallback);

    ChipLogDetail(Zcl, "ClosureOperationalState: HandleConfigureFallbackCommand Fields:");
    LogConfigureFallbackRequest(req);

    Status status;
    if ((status = VerifyFieldRestingProcedure(req.restingProcedure)) != Status::Success)
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, status);
        return;
    }

    if ((status = VerifyFieldTriggerPosition(req.triggerPosition)) != Status::Success)
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, status);
        return;
    }

    if ((status = VerifyFieldTriggerCondition(req.triggerCondition)) != Status::Success)
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, status);
        return;
    }

    if ((status = VerifyFieldWaitingDelay(req.waitingDelay)) != Status::Success)
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, status);
        return;
    }

    // At least one field SHALL be available
    if (!(req.restingProcedure.HasValue() || req.triggerPosition.HasValue() || req.triggerCondition.HasValue() || req.waitingDelay.HasValue()))
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Status::InvalidCommand);
        return;
    }

    // Handle the case of the device being in an invalid state
    if (IsConfigureFallbackInvalidInState(GetCurrentOperationalState()))
    {
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kCommandInvalidInState));
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Status::InvalidInState);
        return;
    }

    // Feature is Enable use the Delegate Callback to update the front-attributes
    if (HasFeature(to_underlying(Feature::kFallback)))
    {
        SetOperationalState(newState);
        // Command is Sane for delegation
        mDelegate->HandleConfigureFallbackCommandCallback(err);
        if (err.errorStateID == to_underlying(OperationalState::ErrorStateEnum::kNoError))
        {
            ChipLogDetail(Zcl, "NEED TO IMPLEMENT attribute update");
        }
    }
    else
    {
        ChipLogDetail(NotSpecified, "Fallback feature " CL_YELLOW "NOT SUPPORTED " CL_CLEAR " ConfigureFallback ignored");
    }
    ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Status::Success);
}

// Add an observer to the list
void ClosureOperationalState::Instance::AddObserver(OperationalState::Observer* observer)
{
    mObservers.push_back(observer);
}

// Remove an observer from the list
void ClosureOperationalState::Instance::RemoveObserver(OperationalState::Observer* observer)
{
    mObservers.erase(std::remove(mObservers.begin(), mObservers.end(), observer), mObservers.end());
}