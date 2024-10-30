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

class ClosuresDevice;

typedef void (ClosuresDevice::*HandleOpSubState)(ReadinessCheckType type, bool & ready);
typedef void (ClosuresDevice::*HandleOpStateCommand)(Clusters::OperationalState::GenericOperationalError & err);
typedef void (ClosuresDevice::*HandleClosureOpStateCommand)(OperationalState::GenericOperationalError & err, const chip::Optional<ClosureOperationalState::TagEnum> tag, 
                                                            const chip::Optional<Globals::ThreeLevelAutoEnum> speed, 
                                                            const chip::Optional<ClosureOperationalState::LatchingEnum> latch) ;
namespace ClosureOperationalState {

// This is an application level delegate to handle operational state commands according to the specific business logic.
class ClosuresOperationalStateDelegate : public ClosureOperationalState::Delegate
{
private:
    const Clusters::OperationalState::GenericOperationalState mOperationalStateList[9] = {
        OperationalState::GenericOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped)),
        OperationalState::GenericOperationalState(to_underlying(OperationalState::OperationalStateEnum::kRunning)),
        OperationalState::GenericOperationalState(to_underlying(OperationalState::OperationalStateEnum::kPaused)),
        OperationalState::GenericOperationalState(to_underlying(OperationalState::OperationalStateEnum::kError)),
        OperationalState::GenericOperationalState(to_underlying(Clusters::ClosureOperationalState::OperationalStateEnum::kCalibrating)),
        OperationalState::GenericOperationalState(to_underlying(Clusters::ClosureOperationalState::OperationalStateEnum::kDisengaded)),
        OperationalState::GenericOperationalState(to_underlying(Clusters::ClosureOperationalState::OperationalStateEnum::kPendingFallback)),
        OperationalState::GenericOperationalState(to_underlying(Clusters::ClosureOperationalState::OperationalStateEnum::kProtected)),
        OperationalState::GenericOperationalState(to_underlying(Clusters::ClosureOperationalState::OperationalStateEnum::kSetupRequired)),

    };
    const Span<const CharSpan> mOperationalPhaseList;

    ClosuresDevice * mPauseClosuresDeviceInstance;
    HandleOpStateCommand mPauseCallback;
    ClosuresDevice * mResumeClosuresDeviceInstance;
    HandleOpStateCommand mResumeCallback;
    ClosuresDevice * mStopClosuresDeviceInstance;
    HandleOpStateCommand mStopCallback;
    ClosuresDevice * mMoveToClosuresDeviceInstance;
    HandleClosureOpStateCommand mMoveToCallback;
    ClosuresDevice * mCalibrateClosuresDeviceInstance;
    HandleOpStateCommand mCalibrateCallback;
    ClosuresDevice * mCheckReadinessInstance;
    HandleOpSubState mCheckReadinessCallback;

    std::function<void()> mDeviceNotReadyCallback;

public:
    /**
     * Get the countdown time. This attribute is not supported in our example Closures app.
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

    /**
     * Handle Command Callback in application: Resume
     * @param[out] get operational error after callback.
     */
    void HandleResumeStateCallback(Clusters::OperationalState::GenericOperationalError & err) override;

    /**
     * Handle Command Callback in application: Stop
     * @param[out] get operational error after callback.
     */
    void HandleStopStateCallback(Clusters::OperationalState::GenericOperationalError & err) override;

    /**
     * Handle Command Callback in application: Stop
     * @param[out] get operational error after callback.
     */
    void HandleCalibrateCommandCallback(Clusters::OperationalState::GenericOperationalError & err) override;

    /**
     * Handle Command Callback in application: MoveTo
     * @param[out] get operational error after callback.
     */
    void HandleMoveToCommandCallback(OperationalState::GenericOperationalError & err, const chip::Optional<ClosureOperationalState::TagEnum> tag, 
                                                            const chip::Optional<Globals::ThreeLevelAutoEnum> speed, 
                                                            const chip::Optional<ClosureOperationalState::LatchingEnum> latch) override;

    void CheckReadinessCallback(ReadinessCheckType aType, bool & aReady) override;

    void SetPauseCallback(HandleOpStateCommand aCallback, ClosuresDevice * aInstance)
    {
        mPauseCallback          = aCallback;
        mPauseClosuresDeviceInstance = aInstance;
    };

    void SetResumeCallback(HandleOpStateCommand aCallback, ClosuresDevice * aInstance)
    {
        mResumeCallback          = aCallback;
        mResumeClosuresDeviceInstance = aInstance;
    };

    void SetStopCallback(HandleOpStateCommand aCallback, ClosuresDevice * aInstance)
    {
        mStopCallback          = aCallback;
        mStopClosuresDeviceInstance = aInstance;
    };

    void SetMoveToCallback(HandleClosureOpStateCommand aCallback, ClosuresDevice * aInstance)
    {
        mMoveToCallback          = aCallback;
        mMoveToClosuresDeviceInstance = aInstance;
    };

    void SetCalibrateCallback(HandleOpStateCommand aCallback, ClosuresDevice * aInstance)
    {
        mCalibrateCallback          = aCallback;
        mCalibrateClosuresDeviceInstance = aInstance;
    };

    void SetCheckReadinessCallback(HandleOpSubState aCallback, ClosuresDevice * aInstance)
    {
        mCheckReadinessCallback = aCallback;
        mCheckReadinessInstance = aInstance;
    };

    void SetDeviceNotReadyCallback(std::function<void()> callback);
    void NotifyDeviceNotReady();
};

void Shutdown();

} // namespace ClosureOperationalState
} // namespace Clusters
} // namespace app
} // namespace chip
