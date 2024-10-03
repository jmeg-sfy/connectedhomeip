#pragma once

#include "closures-operational-state-delegate.h"
#include <app/clusters/mode-base-server/mode-base-server.h>
#include <app/clusters/operational-state-server/operational-state-server.h>

#include <string>

namespace chip {
namespace app {
namespace Clusters {

class ClosuresDevice
{
private:

    ClosureOperationalState::ClosuresOperationalStateDelegate mOperationalStateDelegate;
    ClosureOperationalState::Instance mOperationalStateInstance;

    bool mCalibrating   = false;
    bool mProtected = false;
    bool mDisengaged = false;
    bool mNotOperational = false;

    uint8_t mStateBeforePause = 0;

public:
    /**
     * This class is responsible for initialising all the Closures clusters and manging the interactions between them as required by
     * the specific "business logic". See the state machine diagram.
     * @param aClosuresClustersEndpoint The endpoint ID where all the Closures clusters exist.
     */
    explicit ClosuresDevice(EndpointId aClosuresClustersEndpoint) :
        mOperationalStateInstance(&mOperationalStateDelegate, aClosuresClustersEndpoint)
    {

        SetDeviceToStoppedState();

        // set callback functions
        mOperationalStateDelegate.SetPauseCallback(&ClosuresDevice::HandleOpStatePauseCallback, this);
        mOperationalStateDelegate.SetResumeCallback(&ClosuresDevice::HandleOpStateResumeCallback, this);
        mOperationalStateDelegate.SetStopCallback(&ClosuresDevice::HandleOpStateStopCallback, this);
        mOperationalStateDelegate.SetMoveToCallback(&ClosuresDevice::HandleOpStateMoveToCallback, this);
    }

    /**
     * Init all the clusters used by this device.
     */
    void Init();

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
    void HandleOpStateMoveToCallback(Clusters::OperationalState::GenericOperationalError & err);

    /**
     * Updates the state machine when the device ends calibration
     */

    void HandleCalibrationEndedMessage();

    /**
     * Handles the Calibrate command.
     */
    void HandleCalibratingMessage();

    /**
     * Handles the Engaged command.
     */
    void HandleEngagedMessage();

    /**
     * Handles the Disengaged command.
     */
    void HandleDisengagedMessage();

    /**
     * Handles the ProtectionRised command.
     */
    void HandleProtectionRisedMessage();

    /**
     * Handles the ProtectionDropped command.
     */
    void HandleProtectionDroppedMessage();

    void HandleMovementCompleteEvent();

    /**
     * Sets the device to an error state with the error state ID matching the error name given.
     * @param error The error name. 
     */
    void HandleErrorEvent(const std::string & error);

    void HandleClearErrorMessage();

    void HandleResetMessage();

};

} // namespace Clusters
} // namespace app
} // namespace chip
