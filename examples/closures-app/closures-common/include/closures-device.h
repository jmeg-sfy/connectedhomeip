#pragma once

#include "closures-operational-state-delegate.h"
#include <app/clusters/mode-base-server/mode-base-server.h>
#include <app/clusters/operational-state-server/operational-state-server.h>
#include "../../linux/MotionSimulator.h"

#include <string>

namespace chip {
namespace app {
namespace Clusters {

class ClosuresDevice : public OperationalState::Observer 
{
private:

    ClosureOperationalState::ClosuresOperationalStateDelegate mOperationalStateDelegate;
    ClosureOperationalState::Instance mOperationalStateInstance;
    MotionSimulator mMotionSimulator;

    bool mCalibrating   = false;
    bool mProtected = false;
    bool mDisengaged = false;
    bool mNotOperational = false;
    bool mStopped = false;
    bool mRunning = false;
    bool mPaused = false;

    bool mReadyToRun = false;
    bool mActionNeeded = false;
    bool mSetupNeeded = false;
    bool mFallbackNeeded = false;

    ClosureOperationalState::Structs::OverallStateStruct::Type mOverallState;

    ClosureOperationalState::PositioningEnum mPositioning;
    ClosureOperationalState::LatchingEnum mLatching;
    Globals::ThreeLevelAutoEnum mSpeed;

    uint8_t mStateBeforePause = 0;

    void SetPositioning(ClosureOperationalState::PositioningEnum aPositioning);
    void SetLatching(ClosureOperationalState::LatchingEnum aLatching);
    void SetSpeed(Globals::ThreeLevelAutoEnum aSpeed);
    const char* GetStateString(ClosureOperationalState::OperationalStateEnum aOpState) const;
    ClosureOperationalState::PositioningEnum ConvertTagToPositioning(ClosureOperationalState::TagEnum aTag);

public:
    /**
     * This class is responsible for initialising all the Closures clusters and manging the interactions between them as required by
     * the specific "business logic". See the state machine diagram.
     * @param aClosuresClustersEndpoint The endpoint ID where all the Closures clusters exist.
     */
    using Feature = ClosureOperationalState::Feature;
    explicit ClosuresDevice(EndpointId aClosuresClustersEndpoint) :
        mOperationalStateInstance(&mOperationalStateDelegate, aClosuresClustersEndpoint,
              BitMask<Feature>(
                Feature::kPositioning,
                Feature::kIntermediatePositioning,
                Feature::kSpeed,
                Feature::kVentilation,
                Feature::kManuallyOperable,
                Feature::kFallback))
    {

        SetDeviceToStoppedState();

        // Initialize mOverallState with default values
        mOverallState = mOperationalStateInstance.GetCurrentOverallState();

        // set callback functions
        mOperationalStateDelegate.SetPauseCallback(&ClosuresDevice::HandleOpStatePauseCallback, this);
        mOperationalStateDelegate.SetResumeCallback(&ClosuresDevice::HandleOpStateResumeCallback, this);
        mOperationalStateDelegate.SetStopCallback(&ClosuresDevice::HandleOpStateStopCallback, this);
        mOperationalStateDelegate.SetMoveToCallback(&ClosuresDevice::HandleOpStateMoveToCallback, this);
        mOperationalStateDelegate.SetIsReadyToRunCallback(&ClosuresDevice::IsReadyToRun, this);
        mOperationalStateDelegate.SetActionNeededCallback(&ClosuresDevice::ActionNeeded, this);
        mOperationalStateDelegate.SetSetupNeededCallback(&ClosuresDevice::SetupNeeded, this);
        mOperationalStateDelegate.SetFallbackNeededCallback(&ClosuresDevice::FallbackNeeded, this);
    }

    // Add a public method to allow adding observers
    void AddOperationalStateObserver(OperationalState::Observer * observer);

    /**
     * Init all the clusters used by this device.
     */
    void Init();

    ClosureOperationalState::PositioningEnum GetPositioning();
    ClosureOperationalState::LatchingEnum GetLatching();
    Globals::ThreeLevelAutoEnum GetSpeed();
    ClosureOperationalState::Structs::OverallStateStruct::Type GetOverallState() const;

    void IsReadyToRun(bool & ready);
    void ActionNeeded(bool & ready);
    void SetupNeeded(bool & ready);
    void FallbackNeeded(bool & ready);

    // Call to observer to be notified of a state change
    void OnStateChanged(ClosureOperationalState::OperationalStateEnum state);

    /**
     * 
     */
    void SetDeviceToStoppedState();

    /**
     * Handles the ClosureOperationalState pause command.
     */
    void HandleOpStatePauseCallback(Clusters::OperationalState::GenericOperationalError & err);

    /**
     * Handles the ClosureOperationalState resume command.
     */
    void HandleOpStateResumeCallback(Clusters::OperationalState::GenericOperationalError & err);

    /**
     * Handles the ClosureOperationalState stop command.
     */
    void HandleOpStateStopCallback(Clusters::OperationalState::GenericOperationalError & err);

    /**
     * Handles the ClosureOperationalState MoveTo command.
     */
    void HandleOpStateMoveToCallback(OperationalState::GenericOperationalError & err, const chip::Optional<ClosureOperationalState::TagEnum> tag, 
                                                            const chip::Optional<Globals::ThreeLevelAutoEnum> speed, 
                                                            const chip::Optional<ClosureOperationalState::LatchingEnum> latch);

    /**
     * Handles the MoveTo command stimuli from app.
     */
    void HandleMoveToStimuli(std::optional<uint8_t> tag, std::optional<uint8_t> speed, std::optional<uint8_t> latch);

    /**
     * Handles the Stop command stimuli from app.
     */
    void HandleStopStimuli();

    /**
     * Handles the Calibrate command stimuli from app.
     */
    void HandleCalibrateStimuli();

    /**
     * Handles the ConfigureFallback command stimuli from app.
     */
    void HandleConfigureFallbackStimuli(std::optional<uint8_t> restingProcedure, std::optional<uint8_t> triggerCondition, 
                                        std::optional<uint8_t> triggerPosition, std::optional<uint16_t> waitingDelay);

    /**
     * Handles the Protected stimuli from app.
     */
    void HandleProtectedStimuli();

    /**
     * Handles the Unprotected stimuli from app.
     */
    void HandleUnprotectedStimuli();

    /**
     * Sets the device to an error state with the error state ID matching the error name given.
     * @param error The error name. 
     */
    void HandleErrorEvent(const std::string & error);

    void HandleClearErrorMessage();

    void HandleResetMessage();

};


class MockCommandHandler : public chip::app::CommandHandler {
public:
    MockCommandHandler() : chip::app::CommandHandler() {}

    // Implement any pure virtual methods here
    void AddStatus(const ConcreteCommandPath & aRequestCommandPath,
                           const Protocols::InteractionModel::ClusterStatusCode & aStatus, const char * context = nullptr) override 
    {
    }

    FabricIndex GetAccessingFabricIndex() const override
    {
        FabricIndex test = 1;
        return test;
    }

    bool IsTimedInvoke() const override 
    {
        bool test = true;
        return test;
    }

    virtual void FlushAcksRightAwayOnSlowCommand() override {}
    Access::SubjectDescriptor GetSubjectDescriptor() const override 
    {
        Access::SubjectDescriptor test;
        return test;
    }

    virtual CHIP_ERROR FallibleAddStatus(const ConcreteCommandPath & aRequestCommandPath,
                                         const Protocols::InteractionModel::ClusterStatusCode & aStatus,
                                         const char * context = nullptr) override 
                                         {
                                            return CHIP_NO_ERROR;
                                         }

    CHIP_ERROR AddResponseData(const ConcreteCommandPath & aRequestCommandPath, CommandId aResponseCommandId,
                                       const DataModel::EncodableToTLV & aEncodable) override
                                       {
                                        return CHIP_NO_ERROR;
                                       }
    void AddResponse(const ConcreteCommandPath & aRequestCommandPath, CommandId aResponseCommandId,
                             const DataModel::EncodableToTLV & aEncodable) override {}

    Messaging::ExchangeContext * GetExchangeContext() const override
    {
        return nullptr;
    }
};

} // namespace Clusters
} // namespace app
} // namespace chip
