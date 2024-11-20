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
#include <closures-operational-state-delegate.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ClosureOperationalState;

CHIP_ERROR ClosuresOperationalStateDelegate::GetOperationalStateAtIndex(size_t index,
                                                                   OperationalState::GenericOperationalState & operationalState)
{
    if (index >= ArraySize(mOperationalStateList))
    {
        return CHIP_ERROR_NOT_FOUND;
    }
    operationalState = mOperationalStateList[index];
    return CHIP_NO_ERROR;
}

CHIP_ERROR ClosuresOperationalStateDelegate::GetOperationalPhaseAtIndex(size_t index, MutableCharSpan & operationalPhase)
{
    if (index >= mOperationalPhaseList.size())
    {
        return CHIP_ERROR_NOT_FOUND;
    }
    return CopyCharSpanToMutableCharSpan(mOperationalPhaseList[index], operationalPhase);
}

void ClosuresOperationalStateDelegate::HandlePauseStateCallback(OperationalState::GenericOperationalError & err)
{
    (mPauseClosuresDeviceInstance->*mPauseCallback)(err);
}

void ClosuresOperationalStateDelegate::HandleResumeStateCallback(OperationalState::GenericOperationalError & err)
{
    (mResumeClosuresDeviceInstance->*mResumeCallback)(err);
}

void ClosuresOperationalStateDelegate::HandleStopStateCallback(OperationalState::GenericOperationalError & err)
{
    (mStopClosuresDeviceInstance->*mStopCallback)(err);
}

void ClosuresOperationalStateDelegate::HandleCalibrateCommandCallback(OperationalState::GenericOperationalError & err)
{
    (mCalibrateClosuresDeviceInstance->*mCalibrateCallback)(err);
}

void ClosuresOperationalStateDelegate::HandleMoveToCommandCallback(OperationalState::GenericOperationalError & err, const chip::Optional<ClosureOperationalState::TagEnum> tag, 
                                                            const chip::Optional<Globals::ThreeLevelAutoEnum> speed, 
                                                            const chip::Optional<ClosureOperationalState::LatchingEnum> latch)
{
    (mMoveToClosuresDeviceInstance->*mMoveToCallback)(err, tag, speed, latch);
}

void ClosuresOperationalStateDelegate::HandleConfigureFallbackCommandCallback(
    OperationalState::GenericOperationalError & err, const Optional<ClosureOperationalState::RestingProcedureEnum> restingProcedure,
    const Optional<ClosureOperationalState::TriggerConditionEnum> triggerCondition,
    const Optional<ClosureOperationalState::TriggerPositionEnum> triggerPosition, const Optional<uint16_t> waitingDelay)
{
    (mConfigureFallbackClosuresDeviceInstance->*mConfigureFallbackCallback)(err, restingProcedure, triggerCondition, triggerPosition, waitingDelay);
}

void ClosuresOperationalStateDelegate::HandleCancelFallbackCommandCallback(Clusters::OperationalState::GenericOperationalError & err)
{
    (mCancelFallbackClosuresDeviceInstance->*mCancelFallbackCallback)(err);
}

void ClosureOperationalState::ClosuresOperationalStateDelegate::CheckReadinessCallback(ReadinessCheckType aType, bool & aReady)
{
    (mCheckReadinessInstance->*mCheckReadinessCallback)(aType, aReady);
}

void ClosuresOperationalStateDelegate::SetDeviceNotReadyCallback(std::function<void()> callback)
{
    mDeviceNotReadyCallback = std::move(callback);
}

void ClosuresOperationalStateDelegate::NotifyDeviceNotReady()
{
    if (mDeviceNotReadyCallback) {
        mDeviceNotReadyCallback();
    }
}
