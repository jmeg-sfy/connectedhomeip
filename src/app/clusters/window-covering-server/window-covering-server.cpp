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
#include <map>

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

}

static Percent100ths ValueToPercent100ths(AbsoluteLimits limits, uint16_t absolute)
{
    return ConvertValue(limits.open, limits.closed, WC_PERCENT100THS_MIN_OPEN, WC_PERCENT100THS_MAX_CLOSED, absolute, true);
}

static uint16_t Percent100thsToValue(AbsoluteLimits limits, Percent100ths relative)
{
    emberAfWindowCoveringClusterPrint("Percent100thsToValue");
    return ConvertValue(WC_PERCENT100THS_MIN_OPEN, WC_PERCENT100THS_MAX_CLOSED, limits.open, limits.closed, relative, true);
}

static bool IsPercent100thsValid(Percent100ths percent100ths)
{
    if (CHECK_BOUNDS_VALID(WC_PERCENT100THS_MIN_OPEN, percent100ths, WC_PERCENT100THS_MAX_CLOSED))
        return true;

    return false;
}

static bool IsPercent100thsValid(NPercent100ths percent100ths)
{
    if (!percent100ths.IsNull())
    {
        return IsPercent100thsValid(percent100ths.Value());
    }

    return true;
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

static ActuatorAccessors mLiftAccess = { 0 };
static ActuatorAccessors mTiltAccess = { 0 };

ActuatorAccessors & LiftAccess(void) { return mLiftAccess; }
ActuatorAccessors & TiltAccess(void) { return mTiltAccess; }


{
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



{


}

{




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
    emberAfWindowCoveringClusterPrint("OperationalStatusSet %u 0x%02X global=0x%02X lift=0x%02X tilt=0x%02X", value, value, global, lift, tilt);
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

EmberAfStatus ActuatorAccessors::PositionAccessors::SetAttributeRelativePosition(chip::EndpointId endpoint, const NPercent100ths& relPercent100ths)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;

    NPercent relPercent;

    /* This part is always the mandatory one */
    if (this->mSetPercent100thsCb)
    {
        status = this->mSetPercent100thsCb(endpoint, relPercent100ths);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("SetPercent100thsCb undef");
    }

    if (this->mSetPercentageCb && (EMBER_ZCL_STATUS_SUCCESS == status))
    {
        if (relPercent100ths.IsNull())
        {
            relPercent.SetNull();
        }
        else
        {
            relPercent.Value() = static_cast<Percent>(relPercent100ths.Value() / 100);
        }

        status = this->mSetPercentageCb(endpoint, relPercent);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("SetPercentageCb undef");
    }

    return status;
}

EmberAfStatus ActuatorAccessors::PositionAccessors::GetAttributeRelativePosition(chip::EndpointId endpoint, NPercent100ths& relative)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;

    if (this->mGetPercent100thsCb)
    {
        status = this->mGetPercent100thsCb(endpoint, relative);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("GetPercent100ths undef");
    }

    return status;
}


EmberAfStatus ActuatorAccessors::PositionAccessors::SetAttributeAbsolutePosition(chip::EndpointId endpoint, const NAbsolute& absolute)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;

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
    EmberAfStatus status = EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;

    if (mSetOpenLimitCb)
    {
        status = this->mSetOpenLimitCb(endpoint, limits.open);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("SetOpenLimitCb undef");
    }

    if (this->mSetClosedLimitCb && (EMBER_ZCL_STATUS_SUCCESS == status))
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
    EmberAfStatus status = EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;

    if (this->mGetOpenLimitCb && p_limits)
    {
        status = this->mGetOpenLimitCb(endpoint, &p_limits->open);
    }
    else
    {
        emberAfWindowCoveringClusterPrint("GetOpenLimitCb undef");
    }

    if (this->mGetClosedLimitCb && (EMBER_ZCL_STATUS_SUCCESS == status))
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
    AbsoluteLimits limits = mLimits;

    if (value > limits.closed) {
        value = limits.closed;
    } else if (value < limits.open) {
        value = limits.open;
    }

    return value;
}

EmberAfStatus ActuatorAccessors::PositionAbsoluteSet(chip::EndpointId endpoint, ActuatorAccessors::PositionAccessors::Type type, const NAbsolute& absolute)
{
    NPercent100ths relative;

    AbsoluteToRelative(endpoint, absolute, relative);

    return PositionRelativeSet(endpoint, type, relative);
}

EmberAfStatus ActuatorAccessors::PositionAbsoluteGet(chip::EndpointId endpoint, ActuatorAccessors::PositionAccessors::Type type, NAbsolute& absolute)
{
    NPercent100ths relative;

    EmberAfStatus status = PositionRelativeGet(endpoint, type, relative);

    if (EMBER_ZCL_STATUS_SUCCESS == status)
    {
        RelativeToAbsolute(endpoint, relative, absolute);
    }

    return status;
}

EmberAfStatus ActuatorAccessors::PositionRelativeGet(chip::EndpointId endpoint, ActuatorAccessors::PositionAccessors::Type type, NPercent100ths& relative)
{
    PositionAccessors & posAccessors = (PositionAccessors::Type::Target == type) ? mTarget : mCurrent;
    /* Position Attribute Getter should never fail since Percent100ths are mandatory for all WindowCovering */
    return posAccessors.GetAttributeRelativePosition(endpoint, relative);
}

EmberAfStatus ActuatorAccessors::PositionRelativeSet(chip::EndpointId endpoint, ActuatorAccessors::PositionAccessors::Type type, const NPercent100ths& relative)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    emberAfWindowCoveringClusterPrint("ep[%u] %s %s", endpoint, __func__, (WcFeature::kLift == mFeatureTag) ? "Lift" : "Tilt");
    PrintPercent100ths((PositionAccessors::Type::Target == type) ? "Target" : "Current", relative);

    PositionAccessors & posAccessors = (PositionAccessors::Type::Target == type) ? mTarget : mCurrent;

    /* Position Attribute is mandatory in that case */
    if (IsPositionAware(endpoint))
    {
        /* Attribute is mandatory in that case */
        if (IsPercent100thsValid(relative))
        {
            /* Relative 100ths are always mandatory */
            status = posAccessors.SetAttributeRelativePosition(endpoint, relative);
            if ((EMBER_ZCL_STATUS_SUCCESS == status) && HasFeature(endpoint, WcFeature::kAbsolutePosition))
            {
                NAbsolute absolute;
                RelativeToAbsolute(endpoint, relative, absolute);
                status = posAccessors.SetAttributeAbsolutePosition(endpoint, absolute);
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

EmberAfStatus ActuatorAccessors::PositionRelativeSet(chip::EndpointId endpoint, ActuatorAccessors::PositionAccessors::Type type, Percent100ths relative)
{
    NPercent100ths temp;
    temp.Value(relative);

    return PositionRelativeSet(endpoint, type, temp);
}

EmberAfStatus ActuatorAccessors::PositionAbsoluteSet(chip::EndpointId endpoint, ActuatorAccessors::PositionAccessors::Type type, uint16_t absolute)
{
    NAbsolute temp;
    temp.Value(absolute);

    return PositionAbsoluteSet(endpoint, type, temp);
}

/* Conversions might be needed for device supporting ABS flags attribute */

AbsoluteLimits ActuatorAccessors::AbsoluteLimitsGet(chip::EndpointId endpoint)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;

    /* Following the specs: Since a device can declare or not its Absolute limits on a actuator we try to get if present */
    AbsoluteLimits limits = { .open = WC_PERCENT100THS_MIN_OPEN, .closed = WC_PERCENT100THS_MAX_CLOSED }; // default is 1:1 conversion
    if (HasFeature(endpoint, WcFeature::kAbsolutePosition))
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

    if (HasFeature(endpoint, WcFeature::kAbsolutePosition))
    {
        SetAttributeAbsoluteLimits(endpoint, limits);
    }
}

OperationalState ActuatorAccessors::OperationalStateGet(chip::EndpointId endpoint)
{
    NPercent100ths targetPos, currentPos;

    PositionRelativeGet(endpoint, ActuatorAccessors::PositionAccessors::Type::Target ,  targetPos);
    PositionRelativeGet(endpoint, ActuatorAccessors::PositionAccessors::Type::Current, currentPos);

    /* Compute Operational State from Relative Target and Current */

    return ComputeOperationalState(targetPos, currentPos);
}

void ActuatorAccessors::AbsoluteToRelative(chip::EndpointId endpoint, const NAbsolute& absolute, NPercent100ths& relative)
{
    if (absolute.IsNull())
    {
        relative.SetNull();
    }
    else
    {
        Percent100ths temp = ValueToPercent100ths(AbsoluteLimitsGet(endpoint), absolute.Value());
        relative.Value(temp);
    }
}

void ActuatorAccessors::RelativeToAbsolute(chip::EndpointId endpoint, const NPercent100ths& relative, NAbsolute& absolute)
{
    if (relative.IsNull())
    {
        absolute.SetNull();
    }
    else
    {
        uint16_t temp = Percent100thsToValue(AbsoluteLimitsGet(endpoint), relative.Value());
        absolute.Value(temp);
    }
}

EmberAfStatus ActuatorAccessors::GoToUpOrOpen(chip::EndpointId endpoint)
{
    return PositionRelativeSet(endpoint, ActuatorAccessors::PositionAccessors::Type::Target, WC_PERCENT100THS_MIN_OPEN);
}

EmberAfStatus ActuatorAccessors::GoToDownOrClose(chip::EndpointId endpoint)
{
    return PositionRelativeSet(endpoint, ActuatorAccessors::PositionAccessors::Type::Target, WC_PERCENT100THS_MAX_CLOSED);
}

EmberAfStatus ActuatorAccessors::GoToCurrent(chip::EndpointId endpoint)
{
    NPercent100ths currentPos;

    EmberAfStatus status = PositionRelativeGet(endpoint, ActuatorAccessors::PositionAccessors::Type::Current, currentPos);

    if (EMBER_ZCL_STATUS_SUCCESS != status)
    {
        return status;
    }

    if (currentPos.IsNull())
    {
        return EMBER_ZCL_STATUS_CONSTRAINT_ERROR;
    }

    return PositionRelativeSet(endpoint, ActuatorAccessors::PositionAccessors::Type::Target, currentPos);
}

EmberAfStatus ActuatorAccessors::SetCurrentAtTarget(chip::EndpointId endpoint)
{
    NPercent100ths targetPos;

    EmberAfStatus status = PositionRelativeGet(endpoint, ActuatorAccessors::PositionAccessors::Type::Target, targetPos);

    if (EMBER_ZCL_STATUS_SUCCESS != status)
    {
        return status;
    }

    if (targetPos.IsNull())
    {
        return EMBER_ZCL_STATUS_CONSTRAINT_ERROR;
    }

    return PositionRelativeSet(endpoint, ActuatorAccessors::PositionAccessors::Type::Current, targetPos);
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
void ActuatorAccessors::PositionAccessors::RegisterCallbacksPercentage   (SetAttributePercent_f  set_cb, GetAttributePercent_f  get_cb)
{
    this->mSetPercentageCb = set_cb;
    this->mGetPercentageCb = get_cb;
}
void ActuatorAccessors::PositionAccessors::RegisterCallbacksPercent100ths(SetAttributePercent100ths_f set_cb, GetAttributePercent100ths_f get_cb)
{
    this->mSetPercent100thsCb = set_cb;
    this->mGetPercent100thsCb = get_cb;
}
void ActuatorAccessors::PositionAccessors::RegisterCallbacksAbsolute     (SetAttributeNullableU16_f set_cb, GetAttributeNullableU16_f get_cb)
{
    this->mSetAbsoluteCb = set_cb;
    this->mGetAbsoluteCb = get_cb;
}



void emberAfPluginWindowCoveringFinalizeFakeMotionEventHandler(EndpointId endpoint)
{
    OperationalStatus opStatus = OperationalStatusGet(endpoint);
    emberAfWindowCoveringClusterPrint("WC DELAYED CALLBACK 100ms w/ OpStatus=0x%02X", (unsigned char) opStatus.global);

    /* Update position to simulate movement to pass the CI */
    if (OperationalState::Stall != opStatus.lift)
    {
        LiftAccess().SetCurrentAtTarget(endpoint);
    }

    /* Update position to simulate movement to pass the CI */
    if (OperationalState::Stall != opStatus.tilt)
    {
        TiltAccess().SetCurrentAtTarget(endpoint);
    }
}

EmberEventControl wc_eventControls[EMBER_AF_WINDOW_COVERING_CLUSTER_SERVER_ENDPOINT_COUNT];

EmberEventControl * getEventControl(EndpointId endpoint)
{
    uint16_t index = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_WINDOW_COVERING_CLUSTER_ID);
    return &wc_eventControls[index];
}

EmberEventControl * FinalizeFakeMotionEventControl(EndpointId endpoint)
{
    EmberEventControl * controller = getEventControl(endpoint);

    controller->endpoint = endpoint;
    controller->callback = &emberAfPluginWindowCoveringFinalizeFakeMotionEventHandler;

    return controller;
}

OperationalState ComputeOperationalState(uint16_t target, uint16_t current)
{
    OperationalState opState = OperationalState::Stall;

    if (current != target)
    {
        opState = (current < target) ? OperationalState::MovingDownOrClose : OperationalState::MovingUpOrOpen;
    }
    return opState;
}

OperationalState ComputeOperationalState(NPercent100ths target, NPercent100ths current)
{
    if (!current.IsNull() && !target.IsNull())
    {
        return ComputeOperationalState(target.Value(), current.Value());
    }
    return OperationalState::Stall;
}


void ActuatorAccessors::InitializeCallbacks(chip::EndpointId endpoint, WcFeature tag)
{
    mFeatureTag = tag;

    // Preset all the Attribute Accessors needed for the actuator
    if (mFeatureTag == WcFeature::kLift)
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
}

void ActuatorAccessors::InitializeLimits(chip::EndpointId endpoint)
{
    // If the attributes are present we need to get their values from the default ZAP configuration
    mLimits = AbsoluteLimitsGet(endpoint);

    InitializeLimits(endpoint, mLimits);
}

void ActuatorAccessors::InitializeLimits(chip::EndpointId endpoint, AbsoluteLimits limits)
{
    // Even if the attribute are not present we need the limits for a position in order to do conversion between relative and absolute
    AbsoluteLimitsSet(endpoint, limits);

    PositionAbsoluteSet(endpoint, ActuatorAccessors::PositionAccessors::Type::Current, mLimits.open);

    /* Equalize Target and Current */
    GoToCurrent(endpoint);
}

bool ActuatorAccessors::IsPositionAware(chip::EndpointId endpoint)
{
    bool isPositionAware = false;

    if (WcFeature::kLift == mFeatureTag)
    {
        isPositionAware = (HasFeature(endpoint, WcFeature::kLift) && HasFeature(endpoint, WcFeature::kPositionAwareLift));
    }
    else
    {
        isPositionAware = (HasFeature(endpoint, WcFeature::kTilt) && HasFeature(endpoint, WcFeature::kPositionAwareTilt));
    }

    return isPositionAware;
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
    OperationalStatus prevOpStatus = OperationalStatusGet(endpoint);
    OperationalStatus opStatus = prevOpStatus;

    emberAfWindowCoveringClusterPrint("WC POST ATTRIBUTE=%u OpStatus global=0x%02X lift=0x%02X tilt=0x%02X", (unsigned int) attributeId, (unsigned int) opStatus.global, (unsigned int) opStatus.lift, (unsigned int) opStatus.tilt);

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
        if (OperationalState::Stall != opStatus.global) {
            // Finish the fake motion attribute update:
            emberEventControlSetDelayMS(FinalizeFakeMotionEventControl(endpoint), 6000);
        }
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
        }
        break;
    case Attributes::CurrentPositionTiltPercent100ths::Id:
        if (OperationalState::Stall != opStatus.tilt) {
            opStatus.tilt = OperationalState::Stall;
            emberAfWindowCoveringClusterPrint("Tilt stop");
        }
        break;
    /* For a device supporting Position Awareness : Changing the Target triggers motions on the real or simulated device */
    case Attributes::TargetPositionLiftPercent100ths::Id:
        opStatus.lift = LiftAccess().OperationalStateGet(endpoint);
        break;
    /* For a device supporting Position Awareness : Changing the Target triggers motions on the real or simulated device */
    case Attributes::TargetPositionTiltPercent100ths::Id:
        opStatus.tilt = TiltAccess().OperationalStateGet(endpoint);
        break;
    default:
        break;
    }

    /* This decides and triggers fake motion for the selected endpoint */
    if ((opStatus.lift != prevOpStatus.lift) || (opStatus.tilt != prevOpStatus.tilt))
        OperationalStatusSetWithGlobalUpdated(endpoint, opStatus);
}

} // namespace WindowCovering
} // namespace Clusters
} // namespace app
} // namespace chip

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

    LiftAccess().InitializeCallbacks(endpoint, WcFeature::kLift);
    TiltAccess().InitializeCallbacks(endpoint, WcFeature::kTilt);

    LiftAccess().InitializeLimits(endpoint);
    TiltAccess().InitializeLimits(endpoint);

    /* Init at Half 50% */
    LiftAccess().PositionRelativeSet(endpoint, ActuatorAccessors::PositionAccessors::Type::Current, WC_PERCENT100THS_MAX_CLOSED / 2);
    TiltAccess().PositionRelativeSet(endpoint, ActuatorAccessors::PositionAccessors::Type::Current, WC_PERCENT100THS_MAX_CLOSED / 2);
}

/* #################### all the time : MANDATORY COMMANDs #################### */

/**
 * @brief  Cluster UpOrOpen Command callback (from client)
 */
bool emberAfWindowCoveringClusterUpOrOpenCallback(app::CommandHandler * cmdObj, const app::ConcreteCommandPath & cmdPath,
                                                  const Commands::UpOrOpen::DecodableType & cmdData)
{
    emberAfWindowCoveringClusterPrint("UpOrOpen command received OpStatus=0x%02X", (unsigned int) OperationalStatusGet(cmdPath.mEndpointId).global);

    EmberAfStatus tiltStatus = LiftAccess().GoToUpOrOpen(cmdPath.mEndpointId);
    EmberAfStatus liftStatus = TiltAccess().GoToUpOrOpen(cmdPath.mEndpointId);

    /* By the specification definition we need to support Tilt and/or Lift -> so to simplify only one can be successfull */
    if ((EMBER_ZCL_STATUS_SUCCESS == liftStatus) || (EMBER_ZCL_STATUS_SUCCESS == tiltStatus))
        emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    else
        emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_UNSUP_COMMAND);

    return true;
}

/**
 * @brief  Cluster DownOrClose Command callback (from client)
 */
bool emberAfWindowCoveringClusterDownOrCloseCallback(app::CommandHandler * cmdObj, const app::ConcreteCommandPath & cmdPath,
                                                     const Commands::DownOrClose::DecodableType & cmdData)
{
    emberAfWindowCoveringClusterPrint("DownOrClose command received OpStatus=0x%02X", (unsigned int) OperationalStatusGet(cmdPath.mEndpointId).global);

    EmberAfStatus liftStatus = LiftAccess().GoToDownOrClose(cmdPath.mEndpointId);
    EmberAfStatus tiltStatus = TiltAccess().GoToDownOrClose(cmdPath.mEndpointId);

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
bool emberAfWindowCoveringClusterStopMotionCallback(app::CommandHandler * cmdObj, const app::ConcreteCommandPath & cmdPath,
                                               const Commands::StopMotion::DecodableType & fields)
{
    emberAfWindowCoveringClusterPrint("StopMotion command received OpStatus=0x%02X", (unsigned int) OperationalStatusGet(cmdPath.mEndpointId).global);

    EmberAfStatus liftStatus = LiftAccess().GoToCurrent(cmdPath.mEndpointId);
    EmberAfStatus tiltStatus = TiltAccess().GoToCurrent(cmdPath.mEndpointId);

    /* By the specification definition we need to support Tilt and/or Lift -> so to simplify only one need to be successfull */
    if ((EMBER_ZCL_STATUS_SUCCESS == liftStatus) || (EMBER_ZCL_STATUS_SUCCESS == tiltStatus))
        emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
    else
        emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_UNSUP_COMMAND);

    return true;
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
    EmberAfStatus status = LiftAccess().PositionAbsoluteSet(cmdPath.mEndpointId, ActuatorAccessors::PositionAccessors::Type::Target, cmdData.liftValue);

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
    EmberAfStatus status = LiftAccess().PositionRelativeSet(cmdPath.mEndpointId, ActuatorAccessors::PositionAccessors::Type::Target, cmdData.liftPercent100thsValue);

    emberAfSendImmediateDefaultResponse(status);

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
    EmberAfStatus status = TiltAccess().PositionAbsoluteSet(cmdPath.mEndpointId, ActuatorAccessors::PositionAccessors::Type::Target, cmdData.tiltValue);

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
    EmberAfStatus status = TiltAccess().PositionRelativeSet(cmdPath.mEndpointId, ActuatorAccessors::PositionAccessors::Type::Target, cmdData.tiltPercent100thsValue);

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

void MatterWindowCoveringPluginServerInitCallback()
{
    emberAfWindowCoveringClusterPrint("Window Covering Cluster Plugin Init");
}
