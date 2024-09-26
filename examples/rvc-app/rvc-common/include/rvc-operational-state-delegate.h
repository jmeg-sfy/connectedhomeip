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

#include <app-common/zap-generated/cluster-objects.h>
#include <app/clusters/operational-state-server/operational-state-server.h>

#include <protocols/interaction_model/StatusCode.h>

namespace chip {
namespace app {
namespace Clusters {

class RvcDevice;

typedef void (RvcDevice::*HandleOpStateCommand)(Clusters::OperationalState::GenericOperationalError & err);

namespace ClosureOperationalState {

// This is an application level delegate to handle operational state commands according to the specific business logic.
class ClosureOperationalStateDelegate : public ClosureOperationalState::Delegate
{
private:
    const Clusters::OperationalState::GenericOperationalState mOperationalStateList[8] = {
        OperationalState::GenericOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped)),
        OperationalState::GenericOperationalState(to_underlying(OperationalState::OperationalStateEnum::kRunning)),
        OperationalState::GenericOperationalState(to_underlying(OperationalState::OperationalStateEnum::kPaused)),
        OperationalState::GenericOperationalState(to_underlying(OperationalState::OperationalStateEnum::kError)),
        OperationalState::GenericOperationalState(to_underlying(ClosureOperationalState::OperationalStateEnum::kCalibrating)),
        OperationalState::GenericOperationalState(to_underlying(ClosureOperationalState::OperationalStateEnum::kProtected)),
        OperationalState::GenericOperationalState(to_underlying(ClosureOperationalState::OperationalStateEnum::kDisengaded)),
        OperationalState::GenericOperationalState(to_underlying(ClosureOperationalState::OperationalStateEnum::kSetupRequired)),
    };
    const Span<const CharSpan> mOperationalPhaseList;

    // Base OperationalState Callbacks
    RvcDevice * mPauseRvcDeviceInstance;
    HandleOpStateCommand mPauseCallback;
    RvcDevice * mStopRvcDeviceInstance;
    HandleOpStateCommand mStopCallback;
    RvcDevice * mResumeRvcDeviceInstance;
    HandleOpStateCommand mResumeCallback;
    // Derived ClosureOperationalState Callbacks
    RvcDevice * mCalibrateRvcDeviceInstance;
    HandleOpStateCommand mCalibrateCallback;
    RvcDevice * mMoveToRvcDeviceInstance;
    HandleOpStateCommand mMoveToCallback;
    RvcDevice * mConfigureFallbackRvcDeviceInstance;
    HandleOpStateCommand mConfigureFallbackCallback;

public:
    /**
     * Get the countdown time. This attribute is not supported in our example RVC app.
     * @return Null.
     */
    DataModel::Nullable<uint32_t> GetCountdownTime() override { return {}; };

    /**
     * Fills in the provided GenericOperationalState with the state at index `index` if there is one,
     * or returns CHIP_ERROR_NOT_FOUND if the index is out of range for the list of states.
     * Note: This is used by the SDK to populate the operational state list attribute. If the contents of this list changes,
     * the device SHALL call the Instance's ReportOperationalStateListChange method to report that this attribute has changed.
     * @param index The index of the state, with 0 representing the first state.
     * @param operationalState  The GenericOperationalState is filled.
     */
    CHIP_ERROR GetOperationalStateAtIndex(size_t index,
                                          Clusters::OperationalState::GenericOperationalState & operationalState) override;

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
    CHIP_ERROR GetOperationalPhaseAtIndex(size_t index, MutableCharSpan & operationalPhase) override;

    // command callback
    /**
     * Handle Command Callback in application: Pause
     * @param[out] get operational error after callback.
     */
    void HandlePauseStateCallback(Clusters::OperationalState::GenericOperationalError & err) override;

    // command callback
    /**
     * Handle Command Callback in application: Stop
     * @param[out] get operational error after callback.
     */
    void HandleStopStateCallback(Clusters::OperationalState::GenericOperationalError & err) override;

    // /**
    //  * Handle Command Callback in application: Resume
    //  * @param[out] get operational error after callback.
    //  */
    // void HandleResumeStateCallback(Clusters::OperationalState::GenericOperationalError & err) override;

    /**
     * Handle Command Callback in application: Calibrate
     * @param[out] get operational error after callback.
     */
    void HandleCalibrateCommandCallback(Clusters::OperationalState::GenericOperationalError & err) override;

    /**
     * Handle Command Callback in application: MoveTo
     * @param[out] get operational error after callback.
     */
    void HandleMoveToCommandCallback(Clusters::OperationalState::GenericOperationalError & err) override;

    /**
     * Handle Command Callback in application: ConfigureFallback
     * @param[out] get operational error after callback.
     */
    void HandleConfigureFallbackCommandCallback(Clusters::OperationalState::GenericOperationalError & err) override;

    void SetPauseCallback(HandleOpStateCommand aCallback, RvcDevice * aInstance)
    {
        mPauseCallback          = aCallback;
        mPauseRvcDeviceInstance = aInstance;
    };

    void SetStopCallback(HandleOpStateCommand aCallback, RvcDevice * aInstance)
    {
        mStopCallback          = aCallback;
        mStopRvcDeviceInstance = aInstance;
    };

    void SetResumeCallback(HandleOpStateCommand aCallback, RvcDevice * aInstance)
    {
        mResumeCallback          = aCallback;
        mResumeRvcDeviceInstance = aInstance;
    };

    void SetCalibrateCallback(HandleOpStateCommand aCallback, RvcDevice * aInstance)
    {
        mCalibrateCallback          = aCallback;
        mCalibrateRvcDeviceInstance = aInstance;
    };

    void SetMoveToCallback(HandleOpStateCommand aCallback, RvcDevice * aInstance)
    {
        mMoveToCallback          = aCallback;
        mMoveToRvcDeviceInstance = aInstance;
    };

    void SetConfigureFallbackCallback(HandleOpStateCommand aCallback, RvcDevice * aInstance)
    {
        mConfigureFallbackCallback          = aCallback;
        mConfigureFallbackRvcDeviceInstance = aInstance;
    };
};

void Shutdown();

} // namespace ClosureOperationalState
} // namespace Clusters
} // namespace app
} // namespace chip
