/**
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

/**
 *
 *    Copyright (c) 2020 Silicon Labs
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

#include "window-covering-server.h"

#include <app/util/af.h>

#include <app/common/gen/attribute-id.h>
#include <app/common/gen/cluster-id.h>
#include <app/common/gen/command-id.h>

#include <app/CommandHandler.h>
#include <app/common/gen/attributes/Accessors.h>
#include <app/reporting/reporting.h>
#include <app/util/af-event.h>
#include <app/util/af-types.h>
#include <app/util/attribute-storage.h>
#include <string.h>

#ifdef EMBER_AF_PLUGIN_SCENES
#include <app/clusters/scenes/scenes.h>
#endif // EMBER_AF_PLUGIN_SCENES

using namespace chip::app::Clusters;

#define CHECK_BOUNDS_INVALID(MIN, VAL, MAX) ((VAL < MIN) || (VAL > MAX))
#define CHECK_BOUNDS_VALID(MIN, VAL, MAX) (!CHECK_BOUNDS_INVALID(MIN, VAL, MAX))

#define WC_PERCENTAGE_COEF 100
#define WC_PERCENT100THS_MAX 10000

//------------------------------------------------------------------------------
// WindowCover
//------------------------------------------------------------------------------

WindowCover WindowCover::sInstance;

WindowCover & WindowCover::Instance()
{
    return sInstance;
}

WindowCover::WindowCover() {}

void WindowCover::TypeSet(EmberAfWcType type)
{
    WindowCovering::Attributes::SetType(mEndPoint, static_cast<uint8_t>(type));
}

EmberAfWcType WindowCover::TypeGet(void)
{
    uint8_t type;
    WindowCovering::Attributes::GetType(mEndPoint, &type);
    return static_cast<EmberAfWcType>(type);
}

void WindowCover::ConfigStatusSet(WindowCover::ConfigStatus status)
{
    uint8_t value;
    memcpy(&value, &status, 1);
    WindowCovering::Attributes::SetConfigStatus(mEndPoint, value);
}

const WindowCover::ConfigStatus WindowCover::ConfigStatusGet(void)
{
    ConfigStatus status;
    uint8_t value;
    WindowCovering::Attributes::GetConfigStatus(mEndPoint, &value);
    memcpy(&status, &value, 1);
    return status;
}

void WindowCover::OperationalStatusSet(WindowCover::OperationalStatus status)
{
    uint8_t value;
    memcpy(&value, &status, 1);
    WindowCovering::Attributes::SetOperationalStatus(mEndPoint, value);
}

const WindowCover::OperationalStatus WindowCover::OperationalStatusGet(void)
{
    OperationalStatus status;
    uint8_t value;
    WindowCovering::Attributes::GetOperationalStatus(mEndPoint, &value);
    memcpy(&status, &value, 1);
    return status;
}

void WindowCover::EndProductTypeSet(EmberAfWcEndProductType type)
{
    WindowCovering::Attributes::SetEndProductType(mEndPoint, static_cast<uint8_t>(type));
}

EmberAfWcEndProductType WindowCover::EndProductTypeGet(void)
{
    uint8_t type;
    WindowCovering::Attributes::GetEndProductType(mEndPoint, &type);
    return static_cast<EmberAfWcEndProductType>(type);
}

void WindowCover::ModeSet(WindowCover::Mode mode)
{
    uint8_t value;
    memcpy(&value, &mode, 1);
    WindowCovering::Attributes::SetMode(mEndPoint, value);
}

const WindowCover::Mode WindowCover::ModeGet(void)
{
    Mode mode;
    uint8_t value;
    WindowCovering::Attributes::GetMode(mEndPoint, &value);
    memcpy(&mode, &value, 1);
    return mode;
}

void WindowCover::SafetyStatusSet(WindowCover::SafetyStatus status)
{
    uint16_t value;
    memcpy(&value, &status, 1);
    WindowCovering::Attributes::SetSafetyStatus(mEndPoint, value);
}

const WindowCover::SafetyStatus WindowCover::SafetyStatusGet(void)
{
    SafetyStatus status;
    uint16_t value;
    WindowCovering::Attributes::GetSafetyStatus(mEndPoint, &value);
    memcpy(&status, &value, 1);
    return status;
}

bool WindowCover::IsOpen()
{
    uint16_t liftPosition = mLift.PositionGet();
    uint16_t tiltPosition = mTilt.PositionGet();
    return liftPosition == mLift.OpenLimitGet() && tiltPosition == mTilt.OpenLimitGet();
}

bool WindowCover::IsClosed()
{
    uint16_t liftPosition = mLift.PositionGet();
    uint16_t tiltPosition = mTilt.PositionGet();
    return liftPosition == mLift.ClosedLimitGet() && tiltPosition == mTilt.ClosedLimitGet();
}

//------------------------------------------------------------------------------
// Actuator
//------------------------------------------------------------------------------

uint16_t WindowCover::Actuator::PositionValueGet()
{
    uint16_t percent100ths = PositionGet();
    return Percent100thsToValue(percent100ths);
}

void WindowCover::Actuator::TargetValueSet(uint16_t value)
{
    uint16_t percent100ths = ValueToPercent100ths(value);
    TargetSet(percent100ths);
}

uint16_t WindowCover::Actuator::TargetValueGet()
{
    uint16_t percent100ths = TargetGet();
    return Percent100thsToValue(percent100ths);
}

uint16_t WindowCover::Actuator::ValueToPercent100ths(uint16_t value)
{
    uint16_t openLimit   = OpenLimitGet();
    uint16_t closedLimit = ClosedLimitGet();
    uint16_t minimum = 0, range = UINT16_MAX;

    if (openLimit > closedLimit)
    {
        minimum = closedLimit;
        range   = static_cast<uint16_t>(openLimit - minimum);
    }
    else
    {
        minimum = openLimit;
        range   = static_cast<uint16_t>(closedLimit - minimum);
    }

    if (value < minimum)
    {
        return 0;
    }

    if (range > 0)
    {
        return static_cast<uint16_t>(WC_PERCENT100THS_MAX * (value - minimum) / range);
    }

    return WC_PERCENT100THS_MAX;
}

uint16_t WindowCover::Actuator::Percent100thsToValue(uint16_t percent100ths)
{
    uint16_t openLimit   = OpenLimitGet();
    uint16_t closedLimit = ClosedLimitGet();
    uint16_t minimum = 0, maximum = UINT16_MAX, range = UINT16_MAX;

    if (openLimit > closedLimit)
    {
        minimum = closedLimit;
        maximum = openLimit;
    }
    else
    {
        minimum = openLimit;
        maximum = closedLimit;
    }

    range = static_cast<uint16_t>(maximum - minimum);

    if (percent100ths > WC_PERCENT100THS_MAX)
        return maximum;

    return static_cast<uint16_t>(minimum + ((range * percent100ths) / WC_PERCENT100THS_MAX));
}

//------------------------------------------------------------------------------
// LiftActuator
//------------------------------------------------------------------------------

void WindowCover::LiftActuator::OpenLimitSet(uint16_t limit)
{
    WindowCovering::Attributes::SetInstalledOpenLimitLift(mEndPoint, limit);
}

uint16_t WindowCover::LiftActuator::OpenLimitGet(void)
{
    uint16_t limit = 0;
    WindowCovering::Attributes::GetInstalledOpenLimitLift(mEndPoint, &limit);
    return limit;
}

void WindowCover::LiftActuator::ClosedLimitSet(uint16_t limit)
{
    WindowCovering::Attributes::SetInstalledClosedLimitLift(mEndPoint, limit);
}

uint16_t WindowCover::LiftActuator::ClosedLimitGet(void)
{
    uint16_t limit = 0;
    WindowCovering::Attributes::GetInstalledClosedLimitLift(mEndPoint, &limit);
    return limit;
}

void WindowCover::LiftActuator::PositionSet(uint16_t percent100ths)
{
    uint8_t percent = static_cast<uint8_t>(percent100ths / 100);
    uint16_t value  = Percent100thsToValue(percent100ths);
    WindowCovering::Attributes::SetCurrentPositionLift(mEndPoint, value);
    WindowCovering::Attributes::SetCurrentPositionLiftPercentage(mEndPoint, percent);
    EmberAfStatus err = WindowCovering::Attributes::SetCurrentPositionLiftPercent100ths(mEndPoint, percent100ths);
    emberAfWindowCoveringClusterPrint("### LiftActuator::PositionSet(%u: %u, %u%%, +%u%%) err:%04x", mEndPoint, value, percent,
                                      percent100ths, err);
}

uint16_t WindowCover::LiftActuator::PositionGet()
{
    uint16_t percent100ths = 0;
    WindowCovering::Attributes::GetTargetPositionLiftPercent100ths(mEndPoint, &percent100ths);
    emberAfWindowCoveringClusterPrint("### LiftActuator::PositionGet(%u: +%u%%)", mEndPoint, percent100ths);
    return percent100ths;
}

void WindowCover::LiftActuator::TargetSet(uint16_t percent100ths)
{
    WindowCovering::Attributes::SetTargetPositionLiftPercent100ths(mEndPoint, percent100ths);
}

uint16_t WindowCover::LiftActuator::TargetGet()
{
    uint16_t percent100ths = 0;
    WindowCovering::Attributes::GetTargetPositionLiftPercent100ths(mEndPoint, &percent100ths);
    return percent100ths;
}

void WindowCover::LiftActuator::NumberOfActuationsIncrement()
{
    uint16_t count = static_cast<uint16_t>(NumberOfActuationsGet() + 1);
    WindowCovering::Attributes::SetNumberOfActuationsLift(mEndPoint, count);
}

uint16_t WindowCover::LiftActuator::NumberOfActuationsGet(void)
{
    uint16_t count = 0;
    WindowCovering::Attributes::GetNumberOfActuationsLift(mEndPoint, &count);
    return count;
}

//------------------------------------------------------------------------------
// TiltActuator
//------------------------------------------------------------------------------

void WindowCover::TiltActuator::OpenLimitSet(uint16_t limit)
{
    WindowCovering::Attributes::SetInstalledOpenLimitTilt(mEndPoint, limit);
}

uint16_t WindowCover::TiltActuator::OpenLimitGet(void)
{
    uint16_t limit = 0;
    WindowCovering::Attributes::GetInstalledOpenLimitTilt(mEndPoint, &limit);
    return limit;
}

void WindowCover::TiltActuator::ClosedLimitSet(uint16_t limit)
{
    WindowCovering::Attributes::SetInstalledClosedLimitTilt(mEndPoint, limit);
}

uint16_t WindowCover::TiltActuator::ClosedLimitGet(void)
{
    uint16_t limit = 0;
    WindowCovering::Attributes::GetInstalledClosedLimitTilt(mEndPoint, &limit);
    return limit;
}

void WindowCover::TiltActuator::PositionSet(uint16_t percent100ths)
{
    uint8_t percent = static_cast<uint8_t>(percent100ths / 100);
    uint16_t value  = Percent100thsToValue(percent100ths);
    WindowCovering::Attributes::SetCurrentPositionTilt(mEndPoint, value);
    WindowCovering::Attributes::SetCurrentPositionTiltPercentage(mEndPoint, percent);
    EmberAfStatus err = WindowCovering::Attributes::SetCurrentPositionTiltPercent100ths(mEndPoint, percent100ths);
    emberAfWindowCoveringClusterPrint("### TiltActuator::PositionSet(%u: %u, %u%%, +%u%%) err:%04x", mEndPoint, value, percent,
                                      percent100ths, err);
}

uint16_t WindowCover::TiltActuator::PositionGet()
{
    uint16_t percent100ths = 0;
    WindowCovering::Attributes::GetTargetPositionTiltPercent100ths(mEndPoint, &percent100ths);
    emberAfWindowCoveringClusterPrint("### TiltActuator::PositionGet(%u: +%u%%)", mEndPoint, percent100ths);
    return percent100ths;
}

void WindowCover::TiltActuator::TargetSet(uint16_t percent100ths)
{
    WindowCovering::Attributes::SetTargetPositionTiltPercent100ths(mEndPoint, percent100ths);
}

uint16_t WindowCover::TiltActuator::TargetGet()
{
    uint16_t percent100ths = 0;
    WindowCovering::Attributes::GetTargetPositionTiltPercent100ths(mEndPoint, &percent100ths);
    return percent100ths;
}

void WindowCover::TiltActuator::NumberOfActuationsIncrement()
{
    uint16_t count = static_cast<uint16_t>(NumberOfActuationsGet() + 1);
    WindowCovering::Attributes::SetNumberOfActuationsTilt(mEndPoint, count);
}

uint16_t WindowCover::TiltActuator::NumberOfActuationsGet(void)
{
    uint16_t count = 0;
    WindowCovering::Attributes::GetNumberOfActuationsTilt(mEndPoint, &count);
    return count;
}

//------------------------------------------------------------------------------
// Callbacks
//------------------------------------------------------------------------------

/** @brief Window Covering Cluster Init
 *
 * Cluster Init
 *
 * @param endpoint    Endpoint that is being initialized
 */
void emberAfWindowCoveringClusterInitCallback(chip::EndpointId endpoint)
{
    emberAfWindowCoveringClusterPrint("Window Covering Cluster init");
}

/**
 * @brief Window Covering Cluster UpOrOpen Command callback
 */

bool emberAfWindowCoveringClusterUpOrOpenCallback(chip::app::CommandHandler * cmmd)
{
    WindowCover & cover  = WindowCover::Instance();
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    emberAfWindowCoveringClusterPrint("UpOrOpen command received");
    if (cover.hasFeature(WindowCover::Feature::Lift))
    {
        cover.Lift().TargetSet(0);
    }
    if (cover.hasFeature(WindowCover::Feature::Tilt))
    {
        cover.Tilt().TargetSet(0);
    }
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/**
 * @brief Window Covering Cluster DownOrClose Command callback
 */
bool emberAfWindowCoveringClusterDownOrCloseCallback(chip::app::CommandHandler * commandObj)
{
    WindowCover & cover  = WindowCover::Instance();
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    emberAfWindowCoveringClusterPrint("DownOrClose command received");
    if (cover.hasFeature(WindowCover::Feature::Lift))
    {
        cover.Lift().TargetSet(WC_PERCENT100THS_MAX);
    }
    if (cover.hasFeature(WindowCover::Feature::Tilt))
    {
        cover.Tilt().TargetSet(WC_PERCENT100THS_MAX);
    }
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/**
 * @brief Window Covering Cluster Stop Motion Command callback
 */
bool emberAfWindowCoveringClusterStopMotionCallback(chip::app::CommandHandler *)
{
    WindowCover & cover  = WindowCover::Instance();
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    emberAfWindowCoveringClusterPrint("StopMotion command received");
    cover.Lift().Stop();
    cover.Tilt().Stop();
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/**
 * @brief Window Covering Cluster Go To Lift Value Command callback
 * @param liftValue
 */

bool emberAfWindowCoveringClusterGoToLiftValueCallback(chip::app::CommandHandler * cmmd, uint16_t value)
{
    WindowCover & cover  = WindowCover::Instance();
    bool hasLift         = cover.hasFeature(WindowCover::Feature::Lift);
    bool isPositionAware = cover.hasFeature(WindowCover::Feature::PositionAware);
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    emberAfWindowCoveringClusterPrint("GoToLiftValue Value command received");
    if (hasLift && isPositionAware)
    {
        cover.Lift().TargetValueSet(value);
    }
    else
    {
        status = EMBER_ZCL_STATUS_ACTION_DENIED;
        emberAfWindowCoveringClusterPrint("Err Device is not PA=%u or LF=%u", isPositionAware, hasLift);
    }
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/**
 * @brief Window Covering Cluster Go To Lift Percentage Command callback
 * @param percentLiftValue
 */

bool emberAfWindowCoveringClusterGoToLiftPercentageCallback(chip::app::CommandHandler * cmmd, uint8_t percent)
{
    WindowCover & cover  = WindowCover::Instance();
    bool hasLift         = cover.hasFeature(WindowCover::Feature::Lift);
    bool isPositionAware = cover.hasFeature(WindowCover::Feature::PositionAware);
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    emberAfWindowCoveringClusterPrint("GoToLiftPercentage Percentage command received");
    if (hasLift && isPositionAware)
    {
        cover.Lift().TargetSet(static_cast<uint16_t> (percent * WC_PERCENTAGE_COEF));
    }
    else
    {
        status = EMBER_ZCL_STATUS_ACTION_DENIED;
        emberAfWindowCoveringClusterPrint("Err Device is not PA=%u or LF=%u", isPositionAware, hasLift);
    }
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

bool emberAfWindowCoveringClusterGoToLiftPercentageCallback(chip::app::CommandHandler * cmmd, uint8_t percent,
                                                            uint16_t percent100ths)
{
    WindowCover & cover  = WindowCover::Instance();
    bool hasLift         = cover.hasFeature(WindowCover::Feature::Lift);
    bool isPositionAware = cover.hasFeature(WindowCover::Feature::PositionAware);
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    emberAfWindowCoveringClusterPrint("GoToLiftPercentage Percentage command received");
    if (hasLift && isPositionAware)
    {
        cover.Lift().TargetSet(percent100ths);
    }
    else
    {
        status = EMBER_ZCL_STATUS_ACTION_DENIED;
        emberAfWindowCoveringClusterPrint("Err Device is not PA=%u or LF=%u", isPositionAware, hasLift);
    }
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/**
 * @brief Window Covering Cluster Go To Tilt Value Command callback
 * @param tiltValue
 */

bool emberAfWindowCoveringClusterGoToTiltValueCallback(chip::app::CommandHandler * cmmd, uint16_t value)
{
    WindowCover & cover  = WindowCover::Instance();
    bool hasTilt         = cover.hasFeature(WindowCover::Feature::Tilt);
    bool isPositionAware = cover.hasFeature(WindowCover::Feature::PositionAware);
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    emberAfWindowCoveringClusterPrint("GoToTiltValue command received");
    if (hasTilt && isPositionAware)
    {
        cover.Tilt().TargetValueSet(value);
    }
    else
    {
        status = EMBER_ZCL_STATUS_ACTION_DENIED;
        emberAfWindowCoveringClusterPrint("Err Device is not PA=%u or TL=%u", isPositionAware, hasTilt);
    }
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/**
 * @brief Window Covering Cluster Go To Tilt Percentage Command callback
 * @param percentTiltValue
 */

bool emberAfWindowCoveringClusterGoToTiltPercentageCallback(chip::app::CommandHandler * cmmd, uint8_t percent)
{
    WindowCover & cover  = WindowCover::Instance();
    bool hasTilt         = cover.hasFeature(WindowCover::Feature::Tilt);
    bool isPositionAware = cover.hasFeature(WindowCover::Feature::PositionAware);
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    emberAfWindowCoveringClusterPrint("GoToTiltPercentage command received");
    if (hasTilt && isPositionAware)
    {
        cover.Tilt().TargetSet(static_cast<uint16_t> (percent * WC_PERCENTAGE_COEF));
    }
    else
    {
        status = EMBER_ZCL_STATUS_ACTION_DENIED;
        emberAfWindowCoveringClusterPrint("Err Device is not PA=%u or TL=%u", isPositionAware, hasTilt);
    }
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

bool emberAfWindowCoveringClusterGoToTiltPercentageCallback(chip::app::CommandHandler * cmmd, uint8_t percent,
                                                            uint16_t percent100ths)
{
    WindowCover & cover  = WindowCover::Instance();
    bool hasTilt         = cover.hasFeature(WindowCover::Feature::Tilt);
    bool isPositionAware = cover.hasFeature(WindowCover::Feature::PositionAware);
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    emberAfWindowCoveringClusterPrint("GoToTiltPercentage command received");
    if (hasTilt && isPositionAware)
    {
        cover.Tilt().TargetSet(percent100ths);
    }
    else
    {
        status = EMBER_ZCL_STATUS_ACTION_DENIED;
        emberAfWindowCoveringClusterPrint("Err Device is not PA=%u or TL=%u", isPositionAware, hasTilt);
    }
    emberAfSendImmediateDefaultResponse(status);
    return true;
}
