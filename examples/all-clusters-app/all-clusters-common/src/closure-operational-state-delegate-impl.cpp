/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
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
#include <closure-operational-state-delegate-impl.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ClosureOperationalState;

CHIP_ERROR ClosureOperationalStateDelegate::GetOperationalStateAtIndex(size_t index,
                                                                   OperationalState::GenericOperationalState & operationalState)
{
    if (index >= mOperationalStateList.size())
    {
        return CHIP_ERROR_NOT_FOUND;
    }
    operationalState = mOperationalStateList[index];
    return CHIP_NO_ERROR;
}

CHIP_ERROR ClosureOperationalStateDelegate::GetOperationalPhaseAtIndex(size_t index, MutableCharSpan & operationalPhase)
{
    if (index >= mOperationalPhaseList.size())
    {
        return CHIP_ERROR_NOT_FOUND;
    }
    return CopyCharSpanToMutableCharSpan(mOperationalPhaseList[index], operationalPhase);
}

void ClosureOperationalStateDelegate::HandlePauseStateCallback(OperationalState::GenericOperationalError & err)
{
    // placeholder implementation
    auto error = GetInstance()->SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kPaused));
    if (error == CHIP_NO_ERROR)
    {
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kNoError));
    }
    else
    {
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kUnableToCompleteOperation));
    }
}

void ClosureOperationalStateDelegate::HandleResumeStateCallback(OperationalState::GenericOperationalError & err)
{
    // placeholder implementation
    auto error = GetInstance()->SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kRunning));
    if (error == CHIP_NO_ERROR)
    {
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kNoError));
    }
    else
    {
        err.Set(to_underlying(OperationalState::ErrorStateEnum::kUnableToCompleteOperation));
    }
}

void ClosureOperationalStateDelegate::HandleCalibrateCommandCallback(OperationalState::GenericOperationalError & err)
{
    // placeholder implementation
}

void ClosureOperationalStateDelegate::HandleMoveToCommandCallback(OperationalState::GenericOperationalError & err)
{
    // placeholder implementation
}

void ClosureOperationalStateDelegate::HandleConfigureFallbackCommandCallback(OperationalState::GenericOperationalError & err)
{
    // placeholder implementation
}

static ClosureOperationalState::Instance * gClosureOperationalStateInstance = nullptr;
static ClosureOperationalStateDelegate * gClosureOperationalStateDelegate   = nullptr;

ClosureOperationalState::Instance * ClosureOperationalState::GetClosureOperationalStateInstance()
{
    return gClosureOperationalStateInstance;
}

void ClosureOperationalState::Shutdown()
{
    if (gClosureOperationalStateInstance != nullptr)
    {
        delete gClosureOperationalStateInstance;
        gClosureOperationalStateInstance = nullptr;
    }
    if (gClosureOperationalStateDelegate != nullptr)
    {
        delete gClosureOperationalStateDelegate;
        gClosureOperationalStateDelegate = nullptr;
    }
}

void emberAfClosureOperationalStateClusterInitCallback(chip::EndpointId endpointId)
{
    VerifyOrDie(endpointId == 1); // this cluster is only enabled for endpoint 1.
    VerifyOrDie(gClosureOperationalStateInstance == nullptr && gClosureOperationalStateDelegate == nullptr);

    gClosureOperationalStateDelegate        = new ClosureOperationalStateDelegate;
    EndpointId operationalStateEndpoint = 0x01;
    gClosureOperationalStateInstance        = new ClosureOperationalState::Instance(gClosureOperationalStateDelegate, operationalStateEndpoint);

    gClosureOperationalStateInstance->SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped));

    gClosureOperationalStateInstance->Init();
}
