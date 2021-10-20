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

#include "window-covering-server.h"

#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <app-common/zap-generated/command-id.h>
#include <app/CommandHandler.h>
#include <app/ConcreteCommandPath.h>
#include <app/reporting/reporting.h>
#include <app/util/af-event.h>
#include <app/util/af-types.h>
#include <app/util/af.h>
#include <app/util/attribute-storage.h>
#include <lib/support/TypeTraits.h>
#include <string.h>

#ifdef EMBER_AF_PLUGIN_SCENES
#include <app/clusters/scenes/scenes.h>
#endif // EMBER_AF_PLUGIN_SCENES

using namespace chip;
using namespace chip::app::Clusters::WindowCovering;


#define CHECK_BOUNDS_INVALID(MIN, VAL, MAX) ((VAL < MIN) || (VAL > MAX))
#define CHECK_BOUNDS_VALID(MIN, VAL, MAX)   (!CHECK_BOUNDS_INVALID(MIN, VAL, MAX))

static bool HasFeature(chip::EndpointId endpoint, WcFeature feature)
{
    uint32_t FeatureMap = 0;
    if (EMBER_ZCL_STATUS_SUCCESS ==
        emberAfReadServerAttribute(endpoint, chip::app::Clusters::WindowCovering::Id,
                                   chip::app::Clusters::WindowCovering::Attributes::FeatureMap::Id,
                                   reinterpret_cast<uint8_t *>(&FeatureMap), sizeof(FeatureMap)))
    {
        return (FeatureMap & chip::to_underlying(feature)) != 0;
    }

    return false;
}

static bool HasFeaturePaLift(chip::EndpointId endpoint)
{
    return (HasFeature(endpoint, WcFeature::kLift) && HasFeature(endpoint, WcFeature::kPositionAwareLift));
}

static bool HasFeaturePaTilt(chip::EndpointId endpoint)
{
    return (HasFeature(endpoint, WcFeature::kTilt) && HasFeature(endpoint, WcFeature::kPositionAwareTilt));
}

static uint16_t ConvertValue(uint16_t inputLowValue, uint16_t inputHighValue, uint16_t outputLowValue, uint16_t outputHighValue, uint16_t value, bool offset)
{
    uint16_t inputMin = inputLowValue, inputMax = inputHighValue, inputRange = UINT16_MAX;
    uint16_t outputMin = outputLowValue, outputMax = outputHighValue, outputRange = UINT16_MAX;

    if (inputLowValue > inputHighValue)
    {
        inputMin = inputHighValue;
        inputMax = inputLowValue;
    }

    if (outputLowValue > outputHighValue)
    {
        outputMin = outputHighValue;
        outputMax = outputLowValue;
    }

    inputRange = static_cast<uint16_t>(inputMax - inputMin);
    outputRange = static_cast<uint16_t>(outputMax - outputMin);

    emberAfWindowCoveringClusterPrint("ValueIn=%u", value);
    emberAfWindowCoveringClusterPrint("I range=%u min=%u max=%u",  inputRange,  inputMin, inputMax);
    emberAfWindowCoveringClusterPrint("O range=%u min=%u max=%u", outputRange, outputMin, outputMax);

    if (offset)
    {
        if (value < inputMin)
        {
            return outputMin;
        }

        if (value > inputMax)
        {
            return outputMax;
        }


        emberAfWindowCoveringClusterPrint("ValueIn converted");
        if (inputRange > 0)
        {
            return static_cast<uint16_t>(outputMin + ((outputRange * (value - inputMin) / inputRange)));
        }
    }
    else
    {
        if (inputRange > 0)
        {
            return static_cast<uint16_t>((outputRange * (value) / inputRange));
        }
    }

    return outputMax;

    return outputMax;
}

// typedef struct AbsoluteLimits
// {
//     uint16_t open;
//     uint16_t closed;
// } AbsoluteLimits;

static Percent100ths ValueToPercent100ths(AbsoluteLimits limits, uint16_t absolute)
{
    return ConvertValue(limits.open, limits.closed, WC_PERCENT100THS_MIN_OPEN, WC_PERCENT100THS_MAX_CLOSED, absolute, true);
}

static uint16_t Percent100thsToValue(AbsoluteLimits limits, Percent100ths relative)
{
    return ConvertValue(WC_PERCENT100THS_MIN_OPEN, WC_PERCENT100THS_MAX_CLOSED, limits.open, limits.closed, relative, true);
}

static bool IsPercent100thsValid(uint16_t percent100ths)
{
    if (CHECK_BOUNDS_VALID(WC_PERCENT100THS_MIN_OPEN, percent100ths, WC_PERCENT100THS_MAX_CLOSED))
        return true;

    return false;
}

static OperationalState ValueToOperationalState(uint8_t value)
{
    switch (value)
    {
    case 0x00:
        return OperationalState::Stall;
    case 0x01:
        return OperationalState::MovingUpOrOpen;
    case 0x02:
        return OperationalState::MovingDownOrClose;
    case 0x03:
    default:
        return OperationalState::Reserved;
    }
}
static uint8_t OperationalStateToValue(const OperationalState & state)
{
    switch (state)
    {
    case OperationalState::Stall:
        return 0x00;
    case OperationalState::MovingUpOrOpen:
        return 0x01;
    case OperationalState::MovingDownOrClose:
        return 0x02;
    case OperationalState::Reserved:
    default:
        return 0x03;
    }
}

namespace chip {
namespace app {
namespace Clusters {
namespace WindowCovering { //update to server

EmberEventControl wc_eventControls[EMBER_AF_WINDOW_COVERING_CLUSTER_SERVER_ENDPOINT_COUNT];

bool HasFeature(chip::EndpointId endpoint, Features feature)
{
    return true;
}

void PrintPercent100ths(const char * pMessage, Percent100ths percent100ths)
{
    if (!pMessage) return;

    uint16_t percentage_int = percent100ths / 100;
    uint16_t percentage_dec = static_cast<uint16_t>(percent100ths - ( percentage_int * 100 ));

    emberAfWindowCoveringClusterPrint("%.32s %3u.%02u%%", pMessage, percentage_int, percentage_dec);
}

// void WindowCover::PrintActuators(void)
// {
//     PrintActuator("Lift", &mLift);
//     PrintActuator("Tilt", &mTilt);
//     PrintStatus();
// }

// void WindowCover::PrintStatus(void)
// {
//     EFR32_LOG("Config: 0x%02X, Operational: 0x%02X, Safety: 0x%04X, Mode: 0x%02X", mConfigStatus, mOperationalStatus, mSafetyStatus, mMode);
// }



bool IsLiftOpen(chip::EndpointId endpoint)
{
    EmberAfStatus status;
    app::DataModel::Nullable<Percent100ths> position;

    status = Attributes::TargetPositionLiftPercent100ths::Get(endpoint, position);

    if ((status != EMBER_ZCL_STATUS_SUCCESS) || position.IsNull())
        return false;

    return ((position.Value() == WC_PERCENT100THS_MIN));
}

bool IsTiltOpen(chip::EndpointId endpoint)
{
    EmberAfStatus status;
    app::DataModel::Nullable<Percent100ths> position;

    status = Attributes::TargetPositionTiltPercent100ths::Get(endpoint, position);

    if ((status != EMBER_ZCL_STATUS_SUCCESS) || position.IsNull())
        return false;

    return ((position.Value() == WC_PERCENT100THS_MIN));
}

bool IsLiftClosed(chip::EndpointId endpoint)
{
    EmberAfStatus status;
    app::DataModel::Nullable<Percent100ths> position;

    status = Attributes::TargetPositionLiftPercent100ths::Get(endpoint, position);

    if ((status != EMBER_ZCL_STATUS_SUCCESS) || position.IsNull())
        return false;

    return ((position.Value() == WC_PERCENT100THS_MAX));
}

bool IsTiltClosed(chip::EndpointId endpoint)
{
    EmberAfStatus status;
    app::DataModel::Nullable<Percent100ths> position;

    status = Attributes::TargetPositionTiltPercent100ths::Get(endpoint, position);

    if ((status != EMBER_ZCL_STATUS_SUCCESS) || position.IsNull())
        return false;

    return ((position.Value() == WC_PERCENT100THS_MAX));
}

void TypeSet(chip::EndpointId endpoint, EmberAfWcType type)
{
    Attributes::Type::Set(endpoint, chip::to_underlying(type));
}

EmberAfWcType TypeGet(chip::EndpointId endpoint)
{
    std::underlying_type<EmberAfWcType>::type value;
    Attributes::Type::Get(endpoint, &value);
    return static_cast<EmberAfWcType>(value);
}

void ConfigStatusSet(chip::EndpointId endpoint, const ConfigStatus & status)
{
    /* clang-format off */
    uint8_t value = (status.operational ? 0x01 : 0)
                    | (status.online ? 0x02 : 0)
                    | (status.liftIsReversed ? 0x04 : 0)
                    | (status.liftIsPA ? 0x08 : 0)
                    | (status.tiltIsPA ? 0x10 : 0)
                    | (status.liftIsEncoderControlled ? 0x20 : 0)
                    | (status.tiltIsEncoderControlled ? 0x40 : 0);
    /* clang-format on */
    Attributes::ConfigStatus::Set(endpoint, value);
}

const ConfigStatus ConfigStatusGet(chip::EndpointId endpoint)
{
    uint8_t value = 0;
    ConfigStatus status;

    Attributes::ConfigStatus::Get(endpoint, &value);
    status.operational             = (value & 0x01) ? 1 : 0;
    status.online                  = (value & 0x02) ? 1 : 0;
    status.liftIsReversed          = (value & 0x04) ? 1 : 0;
    status.liftIsPA                = (value & 0x08) ? 1 : 0;
    status.tiltIsPA                = (value & 0x10) ? 1 : 0;
    status.liftIsEncoderControlled = (value & 0x20) ? 1 : 0;
    status.tiltIsEncoderControlled = (value & 0x40) ? 1 : 0;
    return status;
}


void OperationalStatusSetWithGlobalUpdated(chip::EndpointId endpoint, OperationalStatus & status)
{
    /* Global Always follow Lift by priority and then fallback to Tilt */
    if (OperationalState::Stall != status.lift) {
        status.global = status.lift;
    } else {
        status.global = status.tilt;
    }

    OperationalStatusSet(endpoint, status);
}


void OperationalStatusSet(chip::EndpointId endpoint, const OperationalStatus & status)
{
    uint8_t global = OperationalStateToValue(status.global);
    uint8_t lift   = OperationalStateToValue(status.lift);
    uint8_t tilt   = OperationalStateToValue(status.tilt);

    uint8_t value  = (global & 0x03) | static_cast<uint8_t>((lift & 0x03) << 2) | static_cast<uint8_t>((tilt & 0x03) << 4);
    Attributes::OperationalStatus::Set(endpoint, value);
}

const OperationalStatus OperationalStatusGet(chip::EndpointId endpoint)
{
    uint8_t value = 0;
    OperationalStatus status;

    Attributes::OperationalStatus::Get(endpoint, &value);
    status.global = ValueToOperationalState(value & 0x03);
    status.lift   = ValueToOperationalState((value >> 2) & 0x03);
    status.tilt   = ValueToOperationalState((value >> 4) & 0x03);
    return status;
}

void EndProductTypeSet(chip::EndpointId endpoint, EmberAfWcEndProductType type)
{
    Attributes::EndProductType::Set(endpoint, chip::to_underlying(type));
}

EmberAfWcEndProductType EndProductTypeGet(chip::EndpointId endpoint)
{
    std::underlying_type<EmberAfWcType>::type value;
    Attributes::EndProductType::Get(endpoint, &value);
    return static_cast<EmberAfWcEndProductType>(value);
}

void ModeSet(chip::EndpointId endpoint, const Mode & mode)
{
    uint8_t value = (mode.motorDirReversed ? 0x01 : 0) | (mode.calibrationMode ? 0x02 : 0) | (mode.maintenanceMode ? 0x04 : 0) |
        (mode.ledDisplay ? 0x08 : 0);
    Attributes::Mode::Set(endpoint, value);
}

const Mode ModeGet(chip::EndpointId endpoint)
{
    uint8_t value = 0;
    Mode mode;

    Attributes::Mode::Get(endpoint, &value);
    mode.motorDirReversed = (value & 0x01) ? 1 : 0;
    mode.calibrationMode  = (value & 0x02) ? 1 : 0;
    mode.maintenanceMode  = (value & 0x04) ? 1 : 0;
    mode.ledDisplay       = (value & 0x08) ? 1 : 0;
    return mode;
}

void SafetyStatusSet(chip::EndpointId endpoint, SafetyStatus & status)
{
    /* clang-format off */
    uint16_t value = (status.remoteLockout ? 0x0001 : 0)
                     | (status.tamperDetection ? 0x0002 : 0)
                     | (status.failedCommunication ? 0x0004 : 0)
                     | (status.positionFailure ? 0x0008 : 0)
                     | (status.thermalProtection ? 0x0010 : 0)
                     | (status.obstacleDetected ? 0x0020 : 0)
                     | (status.powerIssue ? 0x0040 : 0)
                     | (status.stopInput ? 0x0080 : 0);
    value |= (uint16_t) (status.motorJammed ? 0x0100 : 0)
             | (uint16_t) (status.hardwareFailure ? 0x0200 : 0)
             | (uint16_t) (status.manualOperation ? 0x0400 : 0);
    /* clang-format on */
    Attributes::SafetyStatus::Set(endpoint, value);
}

const SafetyStatus SafetyStatusGet(chip::EndpointId endpoint)
{
    uint16_t value = 0;
    SafetyStatus status;

    Attributes::SafetyStatus::Get(endpoint, &value);
    status.remoteLockout       = (value & 0x0001) ? 1 : 0;
    status.tamperDetection     = (value & 0x0002) ? 1 : 0;
    status.failedCommunication = (value & 0x0004) ? 1 : 0;
    status.positionFailure     = (value & 0x0008) ? 1 : 0;
    status.thermalProtection   = (value & 0x0010) ? 1 : 0;
    status.obstacleDetected    = (value & 0x0020) ? 1 : 0;
    status.powerIssue          = (value & 0x0040) ? 1 : 0;
    status.stopInput           = (value & 0x0080) ? 1 : 0;
    status.motorJammed         = (value & 0x0100) ? 1 : 0;
    status.hardwareFailure     = (value & 0x0200) ? 1 : 0;
    status.manualOperation     = (value & 0x0400) ? 1 : 0;
    return status;
}


   //Attributes::InstalledOpenLimitTilt::Set(endpoint, mTilt.mLimits.open);
   //Attributes::InstalledClosedLimitTilt::Set(endpoint, mTilt.mLimits.closed);
   //PositionAbsoluteSet(endpoint, mTilt.mLimits.closed);

/* Conversions might be needed for device supporting ABS flags attribute */

// static AbsoluteLimits LiftAbsoluteLimitsGet(chip::EndpointId endpoint)
// {
//     AbsoluteLimits limits = { .open = WC_PERCENT100THS_MIN_OPEN, .closed = WC_PERCENT100THS_MAX_CLOSED };// default is 1:1 conversion

//     if (HasFeature(endpoint, Features::Absolute))
//     {
//         Attributes::InstalledOpenLimitLift::Get(endpoint, &limits.open);
//         Attributes::InstalledClosedLimitLift::Get(endpoint, &limits.closed);
//     }

//     return limits;
// }

    //Attributes::InstalledOpenLimitTilt::Set(endpoint, mTilt.mLimits.open);
    //Attributes::InstalledClosedLimitTilt::Set(endpoint, mTilt.mLimits.closed);


// static AbsoluteLimits TiltAbsoluteLimitsGet(chip::EndpointId endpoint)
// {
//     AbsoluteLimits limits = { .open = WC_PERCENT100THS_MIN_OPEN, .closed = WC_PERCENT100THS_MAX_CLOSED };// default is 1:1 conversion

//     if (HasFeature(endpoint, Features::Absolute))
//     {
//         Attributes::InstalledOpenLimitTilt::Get(endpoint, &limits.open);
//         Attributes::InstalledClosedLimitTilt::Get(endpoint, &limits.closed);
//     }

//     return limits;
// }


ActuatorAccessors mLiftAccess = { 0 };
ActuatorAccessors mTiltAccess = { 0 };

ActuatorAccessors * LiftAccess(void) { return &mLiftAccess; }
ActuatorAccessors * TiltAccess(void) { return &mTiltAccess; }

EmberAfStatus PositionAccessors::SetAttributeRelativePosition(chip::EndpointId endpoint, Percent100ths relative)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;

    if (this->mSetPercentageCb)
    {
        status = this->mSetPercentageCb(endpoint, static_cast<uint8_t>(relative / 100));
    }
    else
    {
        emberAfWindowCoveringClusterPrint("SetPercentage undef");
    }

    if ((EMBER_ZCL_STATUS_SUCCESS == status) && this->mSetPercent100thsCb)
    {
        status = this->mSetPercent100thsCb(endpoint, relative);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("SetPercent100ths undef");
    }

    return status;
}


EmberAfStatus PositionAccessors::GetAttributeRelativePosition(chip::EndpointId endpoint, Percent100ths * p_relative)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;

    if (this->mGetPercent100thsCb && p_relative)
    {
        status = this->mGetPercent100thsCb(endpoint, p_relative);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("GetPercent100ths undef");
    }

    return status;
}


EmberAfStatus PositionAccessors::SetAttributeAbsolutePosition(chip::EndpointId endpoint, uint16_t absolute)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    if (this->mSetAbsoluteCb)
    {
        status = this->mSetAbsoluteCb(endpoint, absolute);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("SetAbsolute undef");
    }

    return status;
}

EmberAfStatus ActuatorAccessors::SetAttributeAbsoluteLimits(chip::EndpointId endpoint, AbsoluteLimits limits)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    if (this->mSetOpenLimitCb)
    {
        status = this->mSetOpenLimitCb(endpoint, limits.open);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("SetOpenLimitCb undef");
    }

    if (this->mSetClosedLimitCb)
    {
        status = this->mSetClosedLimitCb(endpoint, limits.closed);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("SetClosedLimitCb undef");
    }

    return status;
}

EmberAfStatus ActuatorAccessors::GetAttributeAbsoluteLimits(chip::EndpointId endpoint, AbsoluteLimits * p_limits)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    if (this->mGetOpenLimitCb)
    {
        status = this->mGetOpenLimitCb(endpoint, &p_limits->open);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("GetOpenLimitCb undef");
    }

    if (this->mGetClosedLimitCb)
    {
        status = this->mGetClosedLimitCb(endpoint, &p_limits->closed);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("GetClosedLimitCb undef");
    }

    return status;
}

uint16_t ActuatorAccessors::WithinAbsoluteRangeCheck(uint16_t value)
{
    AbsoluteLimits limits = mLimits;//mAttributes.AbsoluteLimitsGet();

    if (value > limits.closed) {
        value = limits.closed;
    } else if (value < limits.open) {
        value = limits.open;
    }

    return value;
}

EmberAfStatus ActuatorAccessors::PositionAbsoluteSet(chip::EndpointId endpoint, PositionAccessors::Type type, uint16_t absolute)
{
    return PositionRelativeSet(endpoint, type, AbsoluteToRelative(endpoint, absolute));
}

uint16_t      ActuatorAccessors::PositionAbsoluteGet(chip::EndpointId endpoint, PositionAccessors::Type type)
{
    return RelativeToAbsolute(endpoint, PositionRelativeGet(endpoint, type));
}

Percent100ths ActuatorAccessors::PositionRelativeGet(chip::EndpointId endpoint, PositionAccessors::Type type)
{
    Percent100ths relative = WC_PERCENT100THS_MIN_OPEN;

    PositionAccessors * p_position = (PositionAccessors::Type::Target == type) ? &mTarget : &mCurrent;
    /* Position Attribute Getter should never fail since Percent100ths are mandatory for all WindowCovering */
    p_position->GetAttributeRelativePosition(endpoint, &relative);

    return relative;
}

EmberAfStatus ActuatorAccessors::PositionRelativeSet(chip::EndpointId endpoint, PositionAccessors::Type type, Percent100ths relative)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    bool hasFeature      = HasFeature(endpoint, mFeatureTag);
    bool hasAbsolute     = HasFeature(endpoint, Features::Absolute);
    bool isPositionAware = HasFeature(endpoint, Features::PositionAware);

    PrintPercent100ths(__func__, relative);

    PositionAccessors * p_position = (PositionAccessors::Type::Target == type) ? &mTarget : &mCurrent;

    /* Position Attribute is mandatory in that case */
    if (hasFeature && isPositionAware)
    {
        /* Attribute is mandatory in that case */
        if (IsPercent100thsValid(relative))
        {
            /* Relative 100ths are always mandatory */
            status = p_position->SetAttributeRelativePosition(endpoint, relative);
            if ((EMBER_ZCL_STATUS_SUCCESS == status) && hasAbsolute)
            {
                status = p_position->SetAttributeAbsolutePosition(endpoint, RelativeToAbsolute(endpoint, relative));
            }
        }
        else
        {
            return EMBER_ZCL_STATUS_INVALID_VALUE;
        }
    }
    else
    {
        return EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
    }

    return status;
}


/* Conversions might be needed for device supporting ABS flags attribute */

AbsoluteLimits ActuatorAccessors::AbsoluteLimitsGet(chip::EndpointId endpoint)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;

    /* Following the specs: Since a device can declare or not its Absolute limits on a actuator we try to get if present */
    AbsoluteLimits limits = { .open = WC_PERCENT100THS_MIN_OPEN, .closed = WC_PERCENT100THS_MAX_CLOSED };// default is 1:1 conversion
    if (HasFeature(endpoint, Features::Absolute))
    {
        status = GetAttributeAbsoluteLimits(endpoint, &limits);
    }

    /* If no callbacks or attribute available fallback on Actuator internal limits */
    if (EMBER_ZCL_STATUS_SUCCESS != status)
    {
        limits = mLimits;
    }

    return limits;
}

void ActuatorAccessors::AbsoluteLimitsSet(chip::EndpointId endpoint, AbsoluteLimits limits)
{
    /* Following the specs: Since a device can declare or not its Absolute limits on a actuator we try to set if present */
    mLimits = limits;

    if (HasFeature(endpoint, Features::Absolute))
    {
        SetAttributeAbsoluteLimits(endpoint, limits);
    }
}


OperationalState ActuatorAccessors::OperationalStateGet(chip::EndpointId endpoint)
{
    uint16_t  targetPos = PositionAbsoluteGet(endpoint, PositionAccessors::Type::Target);
    uint16_t currentPos = PositionAbsoluteGet(endpoint, PositionAccessors::Type::Current);

    /* Compute Operational State from Relative Target and Current */
    emberAfWindowCoveringClusterPrint("%.5s[%2u] Cur=%5u <> Tar=%5u", (Features::Lift == mFeatureTag) ? "Lift" : "Tilt", endpoint, currentPos, targetPos);

    return ComputeOperationalState(targetPos, currentPos);
}

Percent100ths ActuatorAccessors::AbsoluteToRelative(chip::EndpointId endpoint, uint16_t absolute)
{
    return ValueToPercent100ths(AbsoluteLimitsGet(endpoint), absolute);
}

uint16_t      ActuatorAccessors::RelativeToAbsolute(chip::EndpointId endpoint, Percent100ths relative)
{
    return Percent100thsToValue(AbsoluteLimitsGet(endpoint), relative);
}

EmberAfStatus ActuatorAccessors::GoToUpOrOpen(chip::EndpointId endpoint)
{

    return PositionAbsoluteSet(endpoint, PositionAccessors::Type::Target, mLimits.open);
}

EmberAfStatus ActuatorAccessors::GoToDownOrClose(chip::EndpointId endpoint)
{
    return PositionAbsoluteSet(endpoint, PositionAccessors::Type::Target, mLimits.closed);
}

EmberAfStatus ActuatorAccessors::GoToCurrent(chip::EndpointId endpoint)
{
    Percent100ths currentPos = PositionRelativeGet(endpoint, PositionAccessors::Type::Current);

    return PositionRelativeSet(endpoint, PositionAccessors::Type::Target, currentPos);
}


void ActuatorAccessors::RegisterCallbacksOpenLimit    (SetAttributeU16_f set_cb, GetAttributeU16_f get_cb)
{
    this->mSetOpenLimitCb = set_cb;
    this->mGetOpenLimitCb = get_cb;
}
void ActuatorAccessors::RegisterCallbacksClosedLimit  (SetAttributeU16_f set_cb, GetAttributeU16_f get_cb)
{
    this->mSetClosedLimitCb = set_cb;
    this->mGetClosedLimitCb = get_cb;
}
void PositionAccessors::RegisterCallbacksPercentage   (SetAttributeU8_f  set_cb, GetAttributeU8_f  get_cb)
{
    this->mSetPercentageCb = set_cb;
    this->mGetPercentageCb = get_cb;
}
void PositionAccessors::RegisterCallbacksPercent100ths(SetAttributeU16_f set_cb, GetAttributeU16_f get_cb)
{
    this->mSetPercent100thsCb = set_cb;
    this->mGetPercent100thsCb = get_cb;
}
void PositionAccessors::RegisterCallbacksAbsolute     (SetAttributeU16_f set_cb, GetAttributeU16_f get_cb)
{
    this->mSetAbsoluteCb = set_cb;
    this->mGetAbsoluteCb = get_cb;
}





//inline Percent100ths LiftAbsoluteToRelative(chip::EndpointId endpoint, uint16_t absolute) { return ValueToPercent100ths(LiftAbsoluteLimitsGet(endpoint), absolute); }
//inline Percent100ths LiftAbsoluteToRelative(chip::EndpointId endpoint, uint16_t absolute) { return ValueToPercent100ths(LiftAbsoluteLimitsGet(endpoint), absolute); }
//inline uint16_t LiftRelativeToAbsolute(chip::EndpointId endpoint, Percent100ths relative) { return Percent100thsToValue(LiftAbsoluteLimitsGet(endpoint), relative); }

//inline Percent100ths TiltAbsoluteToRelative(chip::EndpointId endpoint, uint16_t absolute) { return ValueToPercent100ths(TiltAbsoluteLimitsGet(endpoint), absolute); }
//inline uint16_t TiltRelativeToAbsolute(chip::EndpointId endpoint, Percent100ths relative) { return Percent100thsToValue(TiltAbsoluteLimitsGet(endpoint), relative); }

//EmberAfStatus LiftCurrentPositionAbsoluteSet(chip::EndpointId endpoint, Percent100ths relative) { return LiftCurrentPositionRelativeSet(endpoint, LiftAbsoluteToRelative(endpoint, relative)); }

//EmberAfStatus PositionAbsoluteSet(chip::EndpointId endpoint, PositionAccessors * access, uint16_t absolute) { return PositionRelativeSet(endpoint, access, AbsoluteToRelative(endpoint, access, absolute)); }
//EmberAfStatus PositionAbsoluteSet(chip::EndpointId endpoint, uint16_t absolute) { return PositionRelativeSet(endpoint, TiltAbsoluteToRelative(endpoint, absolute)); }



//inline EmberAfStatus LiftTargetPositionAbsoluteSet(chip::EndpointId endpoint, Percent100ths relative)  { return LiftTargetPositionRelativeSet(endpoint, LiftAbsoluteToRelative(endpoint, relative)); }
//inline EmberAfStatus PositionAbsoluteSet(chip::EndpointId endpoint, Percent100ths relative)  { return PositionRelativeSet(endpoint, TiltAbsoluteToRelative(endpoint, relative)); }


//inline uint16_t LiftCurrentPositionAbsoluteGet(chip::EndpointId endpoint) { return LiftRelativeToAbsolute(endpoint, LiftCurrentPositionRelativeGet(endpoint)); }
//inline uint16_t PositionAbsoluteGet(chip::EndpointId endpoint) { return TiltRelativeToAbsolute(endpoint, PositionRelativeGet(endpoint)); }

//uint16_t TargetPositionAbsoluteGet(chip::EndpointId endpoint, PositionAccessors * access) { return RelativeToAbsolute(endpoint, access, TargetPositionRelativeGet(endpoint, access)); }
//uint16_t LiftTargetPositionAbsoluteGet(chip::EndpointId endpoint) { return LiftRelativeToAbsolute(endpoint, LiftTargetPositionRelativeGet(endpoint)); }
//uint16_t PositionAbsoluteGet(chip::EndpointId endpoint) { return TiltRelativeToAbsolute(endpoint, PositionRelativeGet(endpoint)); }












void emberAfPluginWindowCoveringEventHandler(EndpointId endpoint)
{
    OperationalStatus opStatus = OperationalStatusGet(endpoint);
    emberAfWindowCoveringClusterPrint("WC DELAYED CALLBACK 100ms w/ OpStatus=0x%02X", opStatus);

    /* Update position to simulate movement to pass the CI */
    if (OperationalState::Stall != opStatus.lift)
        LiftAccess()->GoToCurrent(endpoint);

    if (OperationalState::Stall != opStatus.tilt)
        TiltAccess()->GoToCurrent(endpoint);
}

EmberEventControl * getEventControl(EndpointId endpoint)
{
    uint16_t index = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_WINDOW_COVERING_CLUSTER_ID);
    return &wc_eventControls[index];
}

EmberEventControl * configureXYEventControl(EndpointId endpoint)
{
    EmberEventControl * controller = getEventControl(endpoint);

    controller->endpoint = endpoint;
    controller->callback = &emberAfPluginWindowCoveringEventHandler;

    return controller;
}


OperationalState ComputeOperationalState(uint16_t target, uint16_t current)
{
    OperationalState opState = OperationalState::Stall;

    if (current != target) {
        opState = (current < target) ? OperationalState::MovingDownOrClose : OperationalState::MovingUpOrOpen;
    }
    return opState;
}



void ActuatorAccessors::Init(chip::EndpointId endpoint, Features tag, AbsoluteLimits limits)
{
    mFeatureTag = tag;

    // Preset all the Attribute Accessors needed for the actuator
    if (mFeatureTag == Features::Lift)
    {
        RegisterCallbacksOpenLimit    (Attributes::InstalledOpenLimitLift::Set  , Attributes::InstalledOpenLimitLift::Get);
        RegisterCallbacksClosedLimit  (Attributes::InstalledClosedLimitLift::Set, Attributes::InstalledClosedLimitLift::Get);

        mCurrent.RegisterCallbacksAbsolute     (Attributes::CurrentPositionLift::Set             , Attributes::CurrentPositionLift::Get);
        mCurrent.RegisterCallbacksPercentage   (Attributes::CurrentPositionLiftPercentage::Set   , Attributes::CurrentPositionLiftPercentage::Get);
        mCurrent.RegisterCallbacksPercent100ths(Attributes::CurrentPositionLiftPercent100ths::Set, Attributes::CurrentPositionLiftPercent100ths::Get);

        mTarget.RegisterCallbacksPercent100ths (Attributes::TargetPositionLiftPercent100ths::Set , Attributes::TargetPositionLiftPercent100ths::Get);
    }
    else /* Tilt */
    {
        RegisterCallbacksOpenLimit    (Attributes::InstalledOpenLimitTilt::Set  , Attributes::InstalledOpenLimitTilt::Get);
        RegisterCallbacksClosedLimit  (Attributes::InstalledClosedLimitTilt::Set, Attributes::InstalledClosedLimitTilt::Get);

        mCurrent.RegisterCallbacksAbsolute     (Attributes::CurrentPositionTilt::Set             , Attributes::CurrentPositionTilt::Get);
        mCurrent.RegisterCallbacksPercentage   (Attributes::CurrentPositionTiltPercentage::Set   , Attributes::CurrentPositionTiltPercentage::Get);
        mCurrent.RegisterCallbacksPercent100ths(Attributes::CurrentPositionTiltPercent100ths::Set, Attributes::CurrentPositionTiltPercent100ths::Get);

        mTarget.RegisterCallbacksPercent100ths (Attributes::TargetPositionTiltPercent100ths::Set , Attributes::TargetPositionTiltPercent100ths::Get);
    }


    //even if the attribute are not present we need the limits for a position in order to do conversion between relative and absolute
    AbsoluteLimitsSet(endpoint, limits);

    PositionAbsoluteSet(endpoint, PositionAccessors::Type::Current, limits.open);
    /* Equalize Target and Current */
    GoToCurrent(endpoint);
}

bool ActuatorAccessors::IsPositionAware(chip::EndpointId endpoint)
{
    return (HasFeature(endpoint, mFeatureTag) && HasFeature(endpoint, Features::PositionAware));
}


LimitStatus CheckLimitState(uint16_t position, AbsoluteLimits limits)
{

    if (limits.open > limits.closed)
        return LimitStatus::Inverted;

    if (position == limits.open)
        return LimitStatus::IsUpOrOpen;

    if (position == limits.closed)
        return LimitStatus::IsDownOrClose;

    if ((limits.open   > 0) && (position < limits.open  ))
        return LimitStatus::IsOverUpOrOpen;

    if ((limits.closed > 0) && (position > limits.closed))
        return LimitStatus::IsOverDownOrClose;

    return LimitStatus::Intermediate;
}

/* PostAttributeChange is used in all-cluster-app simulation and CI testing : otherwise it is bounded to manufacturer specific implementation */
void PostAttributeChange(chip::EndpointId endpoint, chip::AttributeId attributeId)
{
    OperationalStatus opStatus = OperationalStatusGet(endpoint);

    emberAfWindowCoveringClusterPrint("WC POST ATTRIBUTE=%u OpStatus=0x%02X", attributeId, opStatus);

    switch (attributeId)
    {
    /* RO Type: Cycling Window Covering Demo */
    case Attributes::Type::Id:
        break;
    /* RO ConfigStatus */
    case Attributes::ConfigStatus::Id:
        break;
    /* RO OperationalStatus */
    case Attributes::OperationalStatus::Id:
        //if ((OperationalState::Stall != opStatus.lift) || (OperationalState::Stall != opStatus.tilt)) {
            // kick off the state machine:
            emberEventControlSetDelayMS(configureXYEventControl(endpoint), 3000);
        //}
        break;
    /* RO EndProductType */
    case Attributes::EndProductType::Id:
        break;
    /* RW Mode */
    case Attributes::Mode::Id:
        break;
    /* RO SafetyStatus */
    case Attributes::SafetyStatus::Id:
        break;
    /* ============= Positions for Position Aware ============= */
    case Attributes::CurrentPositionLiftPercent100ths::Id:
        if (OperationalState::Stall != opStatus.lift) {
            opStatus.lift = OperationalState::Stall;
            emberAfWindowCoveringClusterPrint("Lift stop");
            OperationalStatusSetWithGlobalUpdated(endpoint, opStatus);
        }
        break;
    case Attributes::CurrentPositionTiltPercent100ths::Id:
        if (OperationalState::Stall != opStatus.tilt) {
            opStatus.tilt = OperationalState::Stall;
            emberAfWindowCoveringClusterPrint("Tilt stop");
            OperationalStatusSetWithGlobalUpdated(endpoint, opStatus);
        }
        break;
    /* For a device supporting Position Awareness : Changing the Target triggers motions on the real or simulated device */
    case Attributes::TargetPositionLiftPercent100ths::Id:
        opStatus.lift = LiftAccess()->OperationalStateGet(endpoint);
        OperationalStatusSetWithGlobalUpdated(endpoint, opStatus);
        break;
    /* For a device supporting Position Awareness : Changing the Target triggers motions on the real or simulated device */
    case Attributes::TargetPositionTiltPercent100ths::Id:
        //opStatus.tilt = ComputeOperationalState(PositionRelativeGet(endpoint), PositionRelativeGet(endpoint), "Tilt");
        opStatus.tilt = TiltAccess()->OperationalStateGet(endpoint);
        OperationalStatusSetWithGlobalUpdated(endpoint, opStatus);
        break;
    default:
        break;
    }
}

} // namespace WindowCovering
} // namespace Clusters
} // namespace app
} // namespace chip

//------------------------------------------------------------------------------
// Callbacks
//------------------------------------------------------------------------------

typedef EmberAfStatus (*GetPos_f)(chip::EndpointId endpoint, uint16_t * relPercent100ths)   ; // int16u
typedef EmberAfStatus (*SetPos_f)(chip::EndpointId endpoint, uint16_t relPercent100ths)     ;




typedef struct
{
    GetPos_f getPos;
    SetPos_f setPos;
} super_t;

super_t mSuperLift;





/** @brief Window Covering Cluster Init
 *
 * Cluster Init
 *
 * @param endpoint    Endpoint that is being initialized
 */
void emberAfWindowCoveringClusterInitCallback(chip::EndpointId endpoint)
{
    emberAfWindowCoveringClusterPrint("Window Covering Cluster init");


    //LiftAccess()->current.RegisterPercentageCb   (Attributes::CurrentPositionLiftPercentage::Set, Attributes::CurrentPositionLiftPercentage::Get);
    //LiftAccess()->current.RegisterPercent100thsCb(Attributes::CurrentPositionLiftPercent100ths::Set, Attributes::CurrentPositionLiftPercent100ths::Get);
    //LiftAccess()->current.RegisterAbsoluteCb     (Attributes::CurrentPositionLift::Set, Attributes::CurrentPositionLift::Get);
    //LiftAccess()->current.feature = Features::Lift;
   // mPosRel.RelativeToAbsolute = Attributes::TargetPositionTiltPercent100ths::Set

         //   (endpoint, static_cast<uint8_t>(relative / 100));
//             (endpoint, relative);
//             if (hasAbsolute) (endpoint, LiftRelativeToAbsolute(endpoint, relative));
    mSuperLift.setPos(endpoint, 5032);

    uint16_t tte;
    mSuperLift.getPos(endpoint, &tte);

    emberAfWindowCoveringClusterPrint("Window Covering Cluster init = %u", tte);

    /* Init at Half 50% */
    LiftAccess()->PositionRelativeSet(endpoint, PositionAccessors::Type::Current, WC_PERCENT100THS_MAX_CLOSED / 2);
    TiltAccess()->PositionRelativeSet(endpoint, PositionAccessors::Type::Current, WC_PERCENT100THS_MAX_CLOSED / 2);
}

/* #################### all the time : MANDATORY COMMANDs #################### */

/**
 * @brief  Cluster UpOrOpen Command callback (from client)
 */
bool emberAfWindowCoveringClusterUpOrOpenCallback(app::CommandHandler * cmdObj, const app::ConcreteCommandPath & cmdPath,
                                                  const Commands::UpOrOpen::DecodableType & cmdData)
{
    emberAfWindowCoveringClusterPrint("UpOrOpen command received OpStatus=0x%02X", OperationalStatusGet(cmdPath.mEndpointId));

    EmberAfStatus tiltStatus = LiftAccess()->GoToUpOrOpen(cmdPath.mEndpointId);
    EmberAfStatus liftStatus = TiltAccess()->GoToUpOrOpen(cmdPath.mEndpointId);

    /* By the specification definition we need to support Tilt and/or Lift -> so to simplify only one can be successfull */
    if ((EMBER_ZCL_STATUS_SUCCESS == liftStatus) || (EMBER_ZCL_STATUS_SUCCESS == tiltStatus))
        emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    else
        emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_UNSUP_COMMAND);


    if (HasFeature(endpoint, WcFeature::kLift))
    {
        Attributes::TargetPositionLiftPercent100ths::Set(endpoint, WC_PERCENT100THS_MIN);
    }
    if (HasFeature(endpoint, WcFeature::kTilt))
    {
        Attributes::TargetPositionTiltPercent100ths::Set(endpoint, WC_PERCENT100THS_MIN);
    }
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    return true;
}

/**
 * @brief  Cluster DownOrClose Command callback (from client)
 */
bool emberAfWindowCoveringClusterDownOrCloseCallback(app::CommandHandler * cmdObj, const app::ConcreteCommandPath & cmdPath,
                                                     const Commands::DownOrClose::DecodableType & cmdData)
{
    emberAfWindowCoveringClusterPrint("DownOrClose command received OpStatus=0x%02X", OperationalStatusGet(cmdPath.mEndpointId));

    EmberAfStatus liftStatus = LiftAccess()->GoToDownOrClose(cmdPath.mEndpointId);
    EmberAfStatus tiltStatus = TiltAccess()->GoToDownOrClose(cmdPath.mEndpointId);

    /* By the specification definition we need to support Tilt and/or Lift -> so to simplify only one need to be successfull */
    if ((EMBER_ZCL_STATUS_SUCCESS == liftStatus) || (EMBER_ZCL_STATUS_SUCCESS == tiltStatus))
        emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    else
        emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_UNSUP_COMMAND);

    return true;
}

/**
 * @brief  Cluster StopMotion Command callback (from client)
 */
bool emberAfWindowCoveringClusterStopMotionCallback(chip::app::CommandHandler * cmdObj,
                                                    const chip::app::ConcreteCommandPath & cmdPath, chip::EndpointId endpoint,
                                                    Commands::StopMotion::DecodableType & cmdData)
{
    emberAfWindowCoveringClusterPrint("StopMotion command received OpStatus=0x%02X", OperationalStatusGet(endpoint));

    EmberAfStatus liftStatus = LiftAccess()->GoToCurrent(cmdPath.mEndpointId);
    EmberAfStatus tiltStatus = TiltAccess()->GoToCurrent(cmdPath.mEndpointId);

    /* By the specification definition we need to support Tilt and/or Lift -> so to simplify only one need to be successfull */
    if ((EMBER_ZCL_STATUS_SUCCESS == liftStatus) || (EMBER_ZCL_STATUS_SUCCESS == tiltStatus))
        emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    else
        emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_UNSUP_COMMAND);

    return true;
}

/**
 * @brief  Cluster StopMotion Command callback (from client)
 */
bool __attribute__((weak))
emberAfWindowCoveringClusterStopMotionCallback(app::CommandHandler * cmdObj, const app::ConcreteCommandPath & cmdPath,
                                               const Commands::StopMotion::DecodableType & fields)
{
    emberAfWindowCoveringClusterPrint("StopMotion command received");
    uint16_t current          = 0;
    chip::EndpointId endpoint = commandPath.mEndpointId;
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    return true;
    if (HasFeaturePaLift(endpoint))
    {
        (void) Attributes::CurrentPositionLiftPercent100ths::Get(endpoint, current);
        (void) Attributes::TargetPositionLiftPercent100ths::Set(endpoint, current);
    }

    if (HasFeaturePaTilt(endpoint))
    {
        (void) Attributes::CurrentPositionTiltPercent100ths::Get(endpoint, current);
        (void) Attributes::TargetPositionTiltPercent100ths::Set(endpoint, current);
    }

    return EMBER_SUCCESS == emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
}

/* #################### conditionally: MANDATORY COMMANDs #################### */

/**
 * @brief  Cluster GoToLiftValue Command callback (from client)
 */
bool emberAfWindowCoveringClusterGoToLiftValueCallback(app::CommandHandler * cmdObj,
                                                       const app::ConcreteCommandPath & cmdPath,
                                                       const Commands::GoToLiftValue::DecodableType & cmdData)
{
    emberAfWindowCoveringClusterPrint("GoToLiftValue command received w/ %u", cmdData.liftValue);

    /* Absolute works only with devices defining the ABSOLUTE flag on the feature */
    EmberAfStatus status = LiftAccess()->PositionAbsoluteSet(cmdPath.mEndpointId, PositionAccessors::Type::Target, cmdData.liftValue);

    emberAfSendImmediateDefaultResponse(status);

    return true;
}

/**
 * @brief  Cluster GoToLiftPercentage Command callback (from client)
 */
bool emberAfWindowCoveringClusterGoToLiftPercentageCallback(app::CommandHandler * cmdObj,
                                                            const app::ConcreteCommandPath & cmdPath,
                                                            const Commands::GoToLiftPercentage::DecodableType & cmdData)
{
    emberAfWindowCoveringClusterPrint("GoToLiftPercentage command received w/ %u, %u", cmdData.liftPercentageValue, cmdData.liftPercent100thsValue);

    /*
      -> TODO: GoTo Percentage ZCL backward compatibility from the specs
       If the server does not support the Position Aware feature, then a:
      -1/ zero     0 percentage SHOULD be treated as a  DownOrClose command
      -2/ non-zero 1 percentage SHOULD be treated as an UpOrOpen    command
    */
    EmberAfStatus status = LiftAccess()->PositionRelativeSet(cmdPath.mEndpointId, PositionAccessors::Type::Target, cmdData.liftPercent100thsValue);

    emberAfSendImmediateDefaultResponse(status);

    emberAfWindowCoveringClusterPrint("GoToLiftPercentage Percentage command received");
    if (HasFeaturePaLift(endpoint))
    {
        Attributes::TargetPositionLiftPercent100ths::Set(
            endpoint, static_cast<uint16_t>(liftPercentageValue > 100 ? liftPercent100thsValue : liftPercentageValue * 100));
        emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("Err Device is not PA LF");
        emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_ACTION_DENIED);
    }
    return true;
}

/**
 * @brief  Cluster GoToTiltValue Command callback (from client)
 */
bool emberAfWindowCoveringClusterGoToTiltValueCallback(app::CommandHandler * cmdObj,
                                                       const app::ConcreteCommandPath & cmdPath,
                                                       const Commands::GoToTiltValue::DecodableType & cmdData)
{
    emberAfWindowCoveringClusterPrint("GoToTiltValue command received w/ %u", cmdData.tiltValue);

    /* Absolute works only with devices defining the ABSOLUTE flag on the feature */
    EmberAfStatus status = TiltAccess()->PositionAbsoluteSet(cmdPath.mEndpointId, PositionAccessors::Type::Target, cmdData.tiltValue);

    emberAfSendImmediateDefaultResponse(status);

    return true;
}

/**
 * @brief  Cluster GoToTiltPercentage Command callback (from client)
 */
bool emberAfWindowCoveringClusterGoToTiltPercentageCallback(app::CommandHandler * cmdObj,
                                                            const app::ConcreteCommandPath & cmdPath,
                                                            const Commands::GoToTiltPercentage::DecodableType & cmdData)
{
    emberAfWindowCoveringClusterPrint("GoToTiltPercentage command received w/ %u, %u", cmdData.tiltPercentageValue, cmdData.tiltPercent100thsValue);

    /*
      -> TODO: GoTo Percentage ZCL backward compatibility from the specs
       If the server does not support the Position Aware feature, then a:
      -1/ zero     0 percentage SHOULD be treated as a  DownOrClose command
      -2/ non-zero 1 percentage SHOULD be treated as an UpOrOpen    command
    */
    EmberAfStatus status = TiltAccess()->PositionRelativeSet(cmdPath.mEndpointId, PositionAccessors::Type::Target, cmdData.tiltPercent100thsValue);

    emberAfSendImmediateDefaultResponse(status);

    return true;
}

/**
 * @brief  Cluster Attribute Update Callback
 */
void __attribute__((weak)) WindowCoveringPostAttributeChangeCallback(const app::ConcreteAttributePath & attributePath, uint8_t mask, uint8_t type, uint16_t size, uint8_t * value)
{
    PostAttributeChange(attributePath.mEndpointId, attributePath.mAttributeId);
}

void MatterWindowCoveringPluginServerInitCallback() {}
