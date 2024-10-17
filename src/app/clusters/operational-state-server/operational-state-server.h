/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
 *    All rights reserved.
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

#pragma once

#include "operational-state-cluster-objects.h"
#include <app-common/zap-generated/cluster-objects.h>
#include <app/AttributeAccessInterface.h>
#include <app/CommandHandlerInterface.h>
#include <app/cluster-building-blocks/QuieterReporting.h>
#include <app/data-model/Nullable.h>

namespace chip {
namespace app {
namespace Clusters {
namespace OperationalState {

const uint8_t DerivedClusterNumberSpaceStart = 0x40;
const uint8_t VendorNumberSpaceStart         = 0x80;
const uint32_t kNoFeatures                   = 0x00000000;
const uint32_t kAllFeatures                  = 0xFFFFFFFF;

class Uncopyable
{
protected:
    Uncopyable() {}
    ~Uncopyable() = default;

private:
    Uncopyable(const Uncopyable &)             = delete;
    Uncopyable & operator=(const Uncopyable &) = delete;
};

class Delegate;
class Observer;

/**
 * Instance is a class that represents an instance of a derivation of the operational state cluster.
 * It implements CommandHandlerInterface so it can generically handle commands for any derivation cluster id.
 */
class Instance : public CommandHandlerInterface, public AttributeAccessInterface, public Uncopyable
{
public:
    /**
     * Creates an operational state cluster instance.
     * The Init() function needs to be called for this instance to be registered and called by the
     * interaction model at the appropriate times.
     * It is possible to set the CurrentPhase and OperationalState via the Set... methods before calling Init().
     * @param aDelegate A pointer to the delegate to be used by this server.
     * Note: the caller must ensure that the delegate lives throughout the instance's lifetime.
     * @param aEndpointId The endpoint on which this cluster exists. This must match the zap configuration.
     */
    Instance(Delegate * aDelegate, EndpointId aEndpointId);

    ~Instance() override;

    /**
     * Phase name's max length
     */
    static constexpr uint8_t kMaxPhaseNameLength = 64;

    /**
     * Initialise the operational state server instance.
     * This function must be called after defining an Instance class object.
     * @return Returns an error if the given endpoint and cluster ID have not been enabled in zap or if the
     * CommandHandler or AttributeHandler registration fails, else returns CHIP_NO_ERROR.
     */
    CHIP_ERROR Init();

    // Attribute setters
    /**
     * Set operational phase.
     * @param aPhase The operational phase that should now be the current one.
     * @return CHIP_ERROR_INVALID_ARGUMENT if aPhase is an invalid value. CHIP_NO_ERROR if set was successful.
     */
    CHIP_ERROR SetCurrentPhase(const app::DataModel::Nullable<uint8_t> & aPhase);

    /**
     * Set current operational state to aOpState and the operational error to kNoError.
     * NOTE: This method cannot be used to set the error state. The error state must be set via the
     * OnOperationalErrorDetected method.
     * @param aOpState The operational state that should now be the current one.
     * @return CHIP_ERROR_INVALID_ARGUMENT if aOpState is an invalid value. CHIP_NO_ERROR if set was successful.
     */
    CHIP_ERROR SetOperationalState(uint8_t aOpState);

    void LogOperationalState(const uint8_t & aState);
    const char * GetOperationalStateString(const uint8_t & aState);
    virtual const char * GetDerivedClusterOperationalStateString(const uint8_t & aState) { return nullptr; };

    void LogFeatureMap(const uint32_t & featureMap);
    virtual void LogDerivedClusterFeatureMap(const uint32_t & featureMap) { ChipLogDetail(Zcl, "OperationalState::FeatureMap=0x%08X", featureMap); };

    // Attribute getters
    /**
     * Get the feature map.
     * @return The feature map.
     */
    uint32_t GetFeatureMap() const;

    /**
     * Has feature
     */
    bool HasFeature(uint32_t feature);

    /**
     * Get current phase.
     * @return The current phase.
     */
    app::DataModel::Nullable<uint8_t> GetCurrentPhase() const;

    /**
     * Get the current operational state.
     * @return The current operational state value.
     */
    uint8_t GetCurrentOperationalState() const;

    /**
     * Get current operational error.
     * @param error The GenericOperationalError to fill with the current operational error value
     */
    void GetCurrentOperationalError(GenericOperationalError & error) const;

    /**
     * @brief Whenever application delegate wants to possibly report a new updated time,
     *        call this method. The `GetCountdownTime()` method will be called on the delegate.
     */
    void UpdateCountdownTimeFromDelegate() { UpdateCountdownTime(/* fromDelegate = */ true); }

    // Event triggers
    /**
     * @brief Called when the Node detects a OperationalError has been raised.
     * Note: This function also sets the OperationalState attribute to Error.
     * @param aError OperationalError which detects
     */
    void OnOperationalErrorDetected(const Structs::ErrorStateStruct::Type & aError);

    /**
     * @brief Called when the Node detects a OperationCompletion has been raised.
     * @param aCompletionErrorCode CompletionErrorCode
     * @param aTotalOperationalTime TotalOperationalTime
     * @param aPausedTime PausedTime
     */
    void OnOperationCompletionDetected(uint8_t aCompletionErrorCode,
                                       const Optional<DataModel::Nullable<uint32_t>> & aTotalOperationalTime = NullOptional,
                                       const Optional<DataModel::Nullable<uint32_t>> & aPausedTime           = NullOptional);

    // List change reporting
    /**
     * Reports that the contents of the operational state list has changed.
     * The device SHALL call this method whenever it changes the operational state list.
     */
    void ReportOperationalStateListChange();

    /**
     * Reports that the contents of the phase list has changed.
     * The device SHALL call this method whenever it changes the phase list.
     */
    void ReportPhaseListChange();

    /**
     * This function returns true if the phase value given exists in the PhaseList attribute, otherwise it returns false.
     */
    bool IsSupportedPhase(uint8_t aPhase);

    /**
     * This function returns true if the operational state value given exists in the OperationalStateList attribute,
     * otherwise it returns false.
     */
    bool IsSupportedOperationalState(uint8_t aState);

protected:
    /**
     * Creates an operational state cluster instance for a given cluster ID.
     * The Init() function needs to be called for this instance to be registered and called by the
     * interaction model at the appropriate times.
     * It is possible to set the CurrentPhase and OperationalState via the Set... methods before calling Init().
     * @param aDelegate A pointer to the delegate to be used by this server.
     * Note: the caller must ensure that the delegate lives throughout the instance's lifetime.
     * @param aEndpointId The endpoint on which this cluster exists. This must match the zap configuration.
     * @param aClusterId The ID of the operational state derived cluster to be instantiated.
     */
    Instance(Delegate * aDelegate, EndpointId aEndpointId, ClusterId aClusterId, uint32_t aFeature);
    Instance(Delegate * aDelegate, EndpointId aEndpointId, ClusterId aClusterId);

    /**
     * Given a state in the derived cluster number-space (from 0x40 to 0x7f), this method checks if the state is pause-compatible.
     * Note: if a state outside the derived cluster number-space is given, this method returns false.
     * @param aState The state to check.
     * @return true if aState is pause-compatible, false otherwise.
     */
    virtual bool IsDerivedClusterStatePauseCompatible(uint8_t aState) { return false; }

    /**
     * Given a state in the derived cluster number-space (from 0x40 to 0x7f), this method checks if the state is stop-compatible.
     * Note: if a state outside the derived cluster number-space is given, this method returns false.
     * @param aState The state to check.
     * @return true if aState is stop-compatible, false otherwise.
     */
    virtual bool IsDerivedClusterStateStopCompatible(uint8_t aState) { return false; };

    /**
     * Given a state in the derived cluster number-space (from 0x40 to 0x7f), this method checks if the state is start-compatible.
     * Note: if a state outside the derived cluster number-space is given, this method returns false.
     * @param aState The state to check.
     * @return true if aState is start-compatible, false otherwise.
     */
    virtual bool IsDerivedClusterStateStartCompatible(uint8_t aState) { return false; };

    /**
     * Given a state in the derived cluster number-space (from 0x40 to 0x7f), this method checks if the state is resume-compatible.
     * Note: if a state outside the derived cluster number-space is given, this method returns false.
     * @param aState The state to check.
     * @return true if aState is resume-compatible, false otherwise.
     */
    virtual bool IsDerivedClusterStateResumeCompatible(uint8_t aState) { return false; };

    /**
     * Handles the invocation of derived cluster commands.
     * If a derived cluster defines its own commands, this method SHALL be implemented by the derived cluster's class
     * to handle the derived cluster's specific commands.
     * @param handlerContext The command handler context containing information about the received command.
     */
    virtual void InvokeDerivedClusterCommand(HandlerContext & handlerContext) { return; };

    /**
     * Handles the reading of derived cluster commands.
     * If a derived cluster defines its own attributes, this method SHALL be implemented by the derived cluster's class
     * to handle the derived cluster's specific attributes read.
     * @param aPath The constant path indicating the attributId.
     * @param aEncoder Encoder to copy the Attribute value.
     * IM-level implementation of read
     * @return appropriately mapped CHIP_ERROR if applicable (may return CHIP_IM_GLOBAL_STATUS errors)
     */
    virtual CHIP_ERROR ReadDerivedClusterAttribute(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder) { return CHIP_NO_ERROR; };


    virtual bool HasFeatureDerivedCluster(uint32_t feature) { return false; };
    /**
     * Causes reporting/udpating of CountdownTime attribute from driver if sufficient changes have
     * occurred (based on Q quality definition for operational state). Calls the Delegate::GetCountdownTime() method.
     *
     * @param fromDelegate true if the change notice was triggered by the delegate, false if internal to cluster logic.
     */
    void UpdateCountdownTime(bool fromDelegate);

    /**
     * @brief Whenever the cluster logic thinks time should be updated, call this.
     */
    void UpdateCountdownTimeFromClusterLogic() { UpdateCountdownTime(/* fromDelegate=*/false); }

private:
    Delegate * mDelegate;

    const EndpointId mEndpointId;
    const ClusterId mClusterId;

    // Attribute Data Store
    app::DataModel::Nullable<uint8_t> mCurrentPhase;
    uint8_t mOperationalState                 = to_underlying(OperationalStateEnum::kStopped);
    GenericOperationalError mOperationalError = to_underlying(ErrorStateEnum::kNoError);
    app::QuieterReportingAttribute<uint32_t> mCountdownTime{ DataModel::NullNullable };
    uint32_t mFeatureMap;

    /**
     * This method is inherited from CommandHandlerInterface.
     * This reimplementation does not check that the cluster ID in the HandlerContext (the cluster the command relates to)
     * matches the cluster ID of the RequestT type.
     * These cluster IDs may be different in the case where a command defined in the base cluster is intended for a
     * derived cluster.
     */
    template <typename RequestT, typename FuncT>
    void HandleCommand(HandlerContext & handlerContext, FuncT func);

    // Inherited from CommandHandlerInterface
    void InvokeCommand(HandlerContext & ctx) override;

    /**
     * IM-level implementation of read
     * @return appropriately mapped CHIP_ERROR if applicable (may return CHIP_IM_GLOBAL_STATUS errors)
     */
    CHIP_ERROR Read(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder) override;

    /**
     * Handle Command: Pause.
     * If the current state is not pause-compatible, this method responds with an ErrorStateId of CommandInvalidInState.
     * If the current state is paused, this method responds with an ErrorStateId of NoError but takes no action.
     * Otherwise, this method calls the delegate's HandlePauseStateCallback.
     */
    void HandlePauseState(HandlerContext & ctx, const Commands::Pause::DecodableType & req);

    /**
     * Handle Command: Stop.
    **/
    virtual void HandleStopState(HandlerContext & ctx, const Commands::Stop::DecodableType & req);

    /**
    * Handle Command: Stop.
    **/
    virtual void HandleStopState();

    /**
     * Handle Command: Start.
     */
    void HandleStartState(HandlerContext & ctx, const Commands::Start::DecodableType & req);

    /**
     * Handle Command: Resume.
     * If the current state is not resume-compatible, this method responds with an ErrorStateId of CommandInvalidInState.
     * Otherwise, this method calls the delegate's HandleResumeStateCallback.
     */
    void HandleResumeState(HandlerContext & ctx, const Commands::Resume::DecodableType & req);
};

/**
 * A delegate to handle application logic of the Operational State aliased Cluster.
 * The delegate API assumes there will be separate delegate objects for each cluster instance.
 * (i.e. each separate operational state cluster derivation, on each separate endpoint),
 * since the delegate methods are not handed the cluster id or endpoint.
 */
class Delegate
{
public:
    Delegate() = default;

    virtual ~Delegate() = default;

    /**
     * Get the countdown time. This will get called on many edges such as
     * commands to change operational state, or when the delegate deals with
     * changes. Make sure it becomes null whenever it is appropriate.
     *
     * @return The current countdown time.
     */
    virtual app::DataModel::Nullable<uint32_t> GetCountdownTime() = 0;

    /**
     * Fills in the provided GenericOperationalState with the state at index `index` if there is one,
     * or returns CHIP_ERROR_NOT_FOUND if the index is out of range for the list of states.
     * Note: This is used by the SDK to populate the operational state list attribute. If the contents of this list changes,
     * the device SHALL call the Instance's ReportOperationalStateListChange method to report that this attribute has changed.
     * @param index The index of the state, with 0 representing the first state.
     * @param operationalState  The GenericOperationalState is filled.
     */
    virtual CHIP_ERROR GetOperationalStateAtIndex(size_t index, GenericOperationalState & operationalState) = 0;

    /**
     * Fills in the provided MutableCharSpan with the phase at index `index` if there is one,
     * or returns CHIP_ERROR_NOT_FOUND if the index is out of range for the list of phases.
     *
     * If CHIP_ERROR_NOT_FOUND is returned for index 0, that indicates that the PhaseList attribute is null
     * (there are no phases defined at all).
     *
     * Note: This is used by the SDK to populate the phase list attribute. If the contents of this list changes, the
     * device SHALL call the Instance's ReportPhaseListChange method to report that this attribute has changed.
     * @param index The index of the phase, with 0 representing the first phase.
     * @param operationalPhase  The MutableCharSpan is filled.
     */
    virtual CHIP_ERROR GetOperationalPhaseAtIndex(size_t index, MutableCharSpan & operationalPhase) = 0;

    // command callback
    /**
     * Handle Command Callback in application: Pause
     * @param[out] err operational error after callback.
     */
    virtual void HandlePauseStateCallback(GenericOperationalError & err) = 0;

    /**
     * Handle Command Callback in application: Resume
     * @param[out] err operational error after callback.
     */
    virtual void HandleResumeStateCallback(GenericOperationalError & err) = 0;

    /**
     * Handle Command Callback in application: Start
     * @param[out] err operational error after callback.
     */
    virtual void HandleStartStateCallback(GenericOperationalError & err) = 0;

    /**
     * Handle Command Callback in application: Stop
     * @param[out] err operational error after callback.
     */
    virtual void HandleStopStateCallback(GenericOperationalError & err) = 0;

private:
    friend class Instance;

    Instance * mInstance = nullptr;

    /**
     * This method is used by the SDK to set the instance pointer. This is done during the instantiation of a Instance object.
     * @param aInstance A pointer to the Instance object related to this delegate object.
     */
    void SetInstance(Instance * aInstance) { mInstance = aInstance; }

protected:
    Instance * GetInstance() const { return mInstance; }
};

class Observer
{
public:
    Observer() = default;
    virtual ~Observer() = default;

    // Called when the server changes its Operational state
    virtual void OnStateChanged(ClosureOperationalState::OperationalStateEnum state) = 0;
    virtual void OnErrorOccurred(const std::string & error) {};
};

} // namespace OperationalState

// #############################################################################################

namespace RvcOperationalState {

class Delegate : public OperationalState::Delegate
{
public:
    /**
     * Handle Command Callback in application: GoHome
     * @param[out] err operational error after callback.
     */
    virtual void HandleGoHomeCommandCallback(OperationalState::GenericOperationalError & err)
    {
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kUnknownEnumValue));
    };

    /**
     * The start command is not supported by the RvcOperationalState cluster hence this method should never be called.
     * This is a dummy implementation of the handler method so the consumer of this class does not need to define it.
     */
    void HandleStartStateCallback(OperationalState::GenericOperationalError & err) override
    {
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kUnknownEnumValue));
    };

    /**
     * The stop command is not supported by the RvcOperationalState cluster hence this method should never be called.
     * This is a dummy implementation of the handler method so the consumer of this class does not need to define it.
     */
    void HandleStopStateCallback(OperationalState::GenericOperationalError & err) override
    {
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kUnknownEnumValue));
    };
};

class Instance : public OperationalState::Instance
{
public:
    /**
     * Creates an RVC operational state cluster instance.
     * The Init() function needs to be called for this instance to be registered and called by the
     * interaction model at the appropriate times.
     * It is possible to set the CurrentPhase and OperationalState via the Set... methods before calling Init().
     * @param aDelegate A pointer to the delegate to be used by this server.
     * Note: the caller must ensure that the delegate lives throughout the instance's lifetime.
     * @param aEndpointId The endpoint on which this cluster exists. This must match the zap configuration.
     */
    Instance(Delegate * aDelegate, EndpointId aEndpointId) :
        OperationalState::Instance(aDelegate, aEndpointId, Id), mDelegate(aDelegate)
    {}

protected:
    /**
     * Given a state in the derived cluster number-space (from 0x40 to 0x7f), this method checks if the state is pause-compatible.
     * Note: if a state outside the derived cluster number-space is given, this method returns false.
     * @param aState The state to check.
     * @return true if aState is pause-compatible, false otherwise.
     */
    bool IsDerivedClusterStatePauseCompatible(uint8_t aState) override;

    /**
     * Given a state in the derived cluster number-space (from 0x40 to 0x7f), this method checks if the state is resume-compatible.
     * Note: if a state outside the derived cluster number-space is given, this method returns false.
     * @param aState The state to check.
     * @return true if aState is pause-compatible, false otherwise.
     */
    bool IsDerivedClusterStateResumeCompatible(uint8_t aState) override;

    /**
     * Handles the invocation of RvcOperationalState specific commands
     * @param handlerContext The command handler context containing information about the received command.
     */
    void InvokeDerivedClusterCommand(HandlerContext & handlerContext) override;

private:
    Delegate * mDelegate;

    /**
     * Handle Command: GoHome
     */
    void HandleGoHomeCommand(HandlerContext & ctx, const Commands::GoHome::DecodableType & req);
};

} // namespace RvcOperationalState

// #############################################################################################

namespace OvenCavityOperationalState {

class Instance : public OperationalState::Instance
{
public:
    /**
     * Creates an oven cavity operational state cluster instance.
     * The Init() function needs to be called for this instance to be registered and called by the
     * interaction model at the appropriate times.
     * It is possible to set the CurrentPhase and OperationalState via the Set... methods before calling Init().
     * @param aDelegate A pointer to the delegate to be used by this server.
     * Note: the caller must ensure that the delegate lives throughout the instance's lifetime.
     * @param aEndpointId The endpoint on which this cluster exists. This must match the zap configuration.
     */
    Instance(OperationalState::Delegate * aDelegate, EndpointId aEndpointId) :
        OperationalState::Instance(aDelegate, aEndpointId, Id)
    {}
};

} // namespace OvenCavityOperationalState

// #############################################################################################


// #############################################################################################
// ClosureOperationalState

namespace ClosureOperationalState {

const chip::DurationS kMaxDurationS = 64800;
const chip::DurationS kDefaultDurationS = 30;

using Status = Protocols::InteractionModel::Status;

// Commands in state verification
bool IsConfigureFallbackInvalidInState(const uint8_t & aOpState);
bool IsMoveToInvalidInState(const uint8_t & aOpState);
bool IsCalibrateInvalidInState(const uint8_t & aOpState);
bool IsStopInvalidInState(const uint8_t & aOpState);

// MoveTo Command fields verification
Status VerifyFieldLatch(const chip::Optional<LatchingEnum> & item);
Status VerifyFieldTag(const chip::Optional<TagEnum> & item, const uint32_t & featureMap);
Status VerifyFieldSpeed(const chip::Optional<chip::app::Clusters::Globals::ThreeLevelAutoEnum> & item);

// ConfigureFallback Command fields verification
Status VerifyFieldTriggerCondition(const chip::Optional<TriggerConditionEnum> & item);
Status VerifyFieldTriggerPosition(const chip::Optional<TriggerPositionEnum> & item);
Status VerifyFieldRestingProcedure(const chip::Optional<RestingProcedureEnum>  & item);
Status VerifyFieldWaitingDelay(const chip::Optional<chip::DurationS> & item);

void LogOverallStateStruct(const Structs::OverallStateStruct::Type & aOverallState);
void LogMoveToRequest(const Commands::MoveTo::DecodableType & req);
void LogConfigureFallbackRequest(const Commands::ConfigureFallback::DecodableType & req);

const char * GetFeatureMapString(Feature aFeature);
void LogIsFeatureSupported(const uint32_t & featureMap, Feature aFeature);

class Delegate : public OperationalState::Delegate
{
public:

	Delegate() {};

    /**
     * Get the KickoffTimer time. This will get called on many edges such as
     * commands to change operational state, or when the delegate deals with
     * changes.
     *
     * @return The current kickoff timer value.
     */
    virtual DurationS GetKickoffTimer()
    {
        ChipLogDetail(Zcl, "ClosureOperationalState:Delegate GetKickoffTimer dummy");
        return 0;
    };

    /**
     * Handle Command Callback in application: Calibrate
     * @param[out] err operational error after callback.
     */
    virtual void HandleCalibrateCommandCallback(OperationalState::GenericOperationalError & err)
    {
        ChipLogDetail(Zcl, "ClosureOperationalState:Delegate HandleCalibrateCommandCallback dummy");
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kUnknownEnumValue));
    };

    /**
     * Handle Command Callback in application: MoveTo
     * @param[out] err operational error after callback.
     */
    virtual bool IsReadyToRunCallback(void)
    {
        ChipLogDetail(Zcl, "ClosureOperationalState:Delegate IsReadyToRunCallback dummy");
        return true;
    };

    /**
     * Handle Command Callback in application: MoveTo
     * @param[out] err operational error after callback.
     */
    virtual void HandleMoveToCommandCallback(OperationalState::GenericOperationalError & err)
    {
        ChipLogDetail(Zcl, "ClosureOperationalState:Delegate HandleMoveToCommandCallback dummy");
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kUnknownEnumValue));
    };

    /**
     * Handle Command Callback in application: ConfigureFallback
     * @param[out] err operational error after callback.
     */
    virtual void HandleConfigureFallbackCommandCallback(OperationalState::GenericOperationalError & err)
    {
        ChipLogDetail(Zcl, "ClosureOperationalState:Delegate HandleConfigureFallbackCommandCallback dummy");
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kUnknownEnumValue));
    };

    /**
     * The start command is not supported by the ClosureOperationalState cluster hence this method should never be called.
     * This is a dummy implementation of the handler method so the consumer of this class does not need to define it.
     */
    void HandleStartStateCallback(OperationalState::GenericOperationalError & err) override
    {
        ChipLogDetail(Zcl, "ClosureOperationalState:Delegate HandleStartStateCallback dummy");
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kUnknownEnumValue));
    };

    /**
     * The stop command is not supported by the ClosureOperationalState cluster hence this method should never be called.
     * This is a dummy implementation of the handler method so the consumer of this class does not need to define it.
     */
    void HandleResumeStateCallback(OperationalState::GenericOperationalError & err) override
    {
        ChipLogDetail(Zcl, "ClosureOperationalState:Delegate HandleResumeStateCallback dummy");
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kUnknownEnumValue));
    };

    /**
     * The stop command is not supported by the ClosureOperationalState cluster hence this method should never be called.
     * This is a dummy implementation of the handler method so the consumer of this class does not need to define it.
     */
    void HandleStopStateCallback(OperationalState::GenericOperationalError & err) override
    {
        ChipLogDetail(Zcl, "ClosureOperationalState:Delegate HandleStopStateCallback dummy");
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kUnknownEnumValue));
    };
};

class Instance : public OperationalState::Instance
{
public:
    /**
     * Creates an Closure operational state cluster instance.
     * The Init() function needs to be called for this instance to be registered and called by the
     * interaction model at the appropriate times.
     * It is possible to set the CurrentPhase and OperationalState via the Set... methods before calling Init().
     * @param aDelegate A pointer to the delegate to be used by this server.
     * Note: the caller must ensure that the delegate lives throughout the instance's lifetime.
     * @param aEndpointId The endpoint on which this cluster exists. This must match the zap configuration.
     */
    // TODO : Note Cluster ID is passed here
    Instance(Delegate * aDelegate, EndpointId aEndpointId, BitMask<Feature> aFeatureBitMask) :
        OperationalState::Instance(aDelegate, aEndpointId, Id, aFeatureBitMask.Raw()), mDelegate(aDelegate)
    {
        // Preset OverallState
        if (HasFeature(to_underlying(Feature::kPositioning)))
        {
            mOverallState.positioning.SetValue(PositioningEnum::kPartiallyOpened);
        }
        else
        {
            mOverallState.positioning.ClearValue();
        }

        if (HasFeature(to_underlying(Feature::kLatching)))
        {
            mOverallState.latching.SetValue(LatchingEnum::kLatchedButNotSecured);
        }
        else
        {
            mOverallState.latching.ClearValue();
        }

        if (HasFeature(to_underlying(Feature::kSpeed)))
        {
            mOverallState.speed.SetValue(Globals::ThreeLevelAutoEnum::kAutomatic);
        }
        else
        {
            mOverallState.speed.ClearValue();
        }
        mWaitingDelay = kDefaultDurationS;
        LogFeatureMap(GetFeatureMap());
    }

    void AddObserver(OperationalState::Observer* observer);
    void RemoveObserver(OperationalState::Observer* observer);

    ~Instance() override;

    // Method to check if it's possible to change the state to Running
    bool CanTransitionToRunning() const {

        switch (GetCurrentOperationalState())
        {
            case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kStopped):
            case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kRunning):
            case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kPaused):
            case to_underlying(chip::app::Clusters::OperationalState::OperationalStateEnum::kError):
            case to_underlying(OperationalStateEnum::kPendingFallback):
            return true;
            break;
        default:
            break;
        }

        return false;
    }

    void MoveToStimuli();
	
	// // Method to transition to Running state
    // void TransitionToStopped() {
    //     if (CanTransitionToStopped()) {
    //         // Change the state to Running
    //         SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped));
    //         //SetState(ClosureOperationalState::OperationalStateEnum::kRunning);
    //     } else {
    //         ChipLogDetail(Zcl, "State transition to Running is not allowed.");
    //     }
    // }
	
    CHIP_ERROR UpdateActionState(void);
    /**
     * Get the current operational state.
     * @return The current operational state value.
     */
    Structs::OverallStateStruct::Type GetCurrentOverallState() const;
    chip::DurationS GetCurrentWaitingDelay() const;

    const char * GetDerivedClusterOperationalStateString(const uint8_t & aState) override;
	
	void SetState(ClosureOperationalState::OperationalStateEnum newState);

    /**
    * Handle Command: Stop.
    */
    void HandleStopState() override;

        /**
    * Handle Command: Stop.
    */
    void HandleMoveToCommand(/*const Commands::MoveTo::DecodableType & req*/);

protected:
    /**
     * Given a state in the derived cluster number-space (from 0x40 to 0x7f), this method checks if the state is pause-compatible.
     * Note: if a state outside the derived cluster number-space is given, this method returns false.
     * @param aState The state to check.
     * @return true if aState is pause-compatible, false otherwise.
     */
    bool IsDerivedClusterStatePauseCompatible(uint8_t aState) override;

    /**
     * Given a state in the derived cluster number-space (from 0x40 to 0x7f), this method checks if the state is resume-compatible.
     * Note: if a state outside the derived cluster number-space is given, this method returns false.
     * @param aState The state to check.
     * @return true if aState is pause-compatible, false otherwise.
     */
    bool IsDerivedClusterStateResumeCompatible(uint8_t aState) override;

    /**
     * Handles the invocation of ClosureOperationalState specific commands
     * @param handlerContext The command handler context containing information about the received command.
     */
    void InvokeDerivedClusterCommand(HandlerContext & handlerContext) override;

    /**
     * Handles the reading of derived cluster commands.
     * If a derived cluster defines its own attributes, this method SHALL be implemented by the derived cluster's class
     * to handle the derived cluster's specific attributes read.
     * @param aPath The constant path indicating the attributId.
     * @param aEncoder Encoder to copy the Attribute value.
     * IM-level implementation of read
     * @return appropriately mapped CHIP_ERROR if applicable (may return CHIP_IM_GLOBAL_STATUS errors)
     */
    CHIP_ERROR ReadDerivedClusterAttribute(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder) override;

    /**
     * Handle Command: Stop.
     */
    void HandleStopState(HandlerContext & ctx, const OperationalState::Commands::Stop::DecodableType & req) override;

    /**
     * Log the feature map of the derived cluster.
     * @param featureMap The feature map to log.
     */
    void LogDerivedClusterFeatureMap(const uint32_t & featureMap) override;

    /**
     * Derived feature verifier against Cluster FeatureMap
     * @param aFeature
     */
    bool HasFeatureDerivedCluster(uint32_t feature) override;

	
	void NotifyObservers();

private:
    Delegate * mDelegate;
    ClosureOperationalState::OperationalStateEnum mCurrentState;
    std::vector<OperationalState::Observer*> mObservers;

    // Attribute Data Store
    Structs::OverallStateStruct::Type mOverallState;
    chip::DurationS mWaitingDelay;

    /**
     * Handle Command: MoveTo
     */
    void HandleMoveToCommand(HandlerContext & ctx, const Commands::MoveTo::DecodableType & req);
	
	/**
     * Handle Command: Stop
     */
    void HandleStopCommand(HandlerContext & ctx, const Commands::MoveTo::DecodableType & req);

    /**
     * Handle Command: Calibrate
     */
    void HandleCalibrateCommand(HandlerContext & ctx, const Commands::Calibrate::DecodableType & req);

    /**
     * Handle Command: ConfigureFallback
     */
    void HandleConfigureFallbackCommand(HandlerContext & ctx, const Commands::ConfigureFallback::DecodableType & req);

};

} // namespace ClosureOperationalState

} // namespace Clusters
} // namespace app
} // namespace chip
