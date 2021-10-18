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

/* Conversions might be needed for device supporting ABS flags attribute */

static AbsoluteLimits AbsoluteLimitsGet(chip::EndpointId endpoint, PositionAccessors * access)
{
    AbsoluteLimits limits = { .open = WC_PERCENT100THS_MIN_OPEN, .closed = WC_PERCENT100THS_MAX_CLOSED };// default is 1:1 conversion

    if (HasFeature(endpoint, Features::Absolute))
    {
        access->GetInstalledOpenLimit(endpoint, &limits.open);
        access->GetInstalledClosedLimit(endpoint, &limits.closed);
    }

    return limits;
}



/* Conversions might be needed for device supporting ABS flags attribute */

static AbsoluteLimits LiftAbsoluteLimitsGet(chip::EndpointId endpoint)
{
    AbsoluteLimits limits = { .open = WC_PERCENT100THS_MIN_OPEN, .closed = WC_PERCENT100THS_MAX_CLOSED };// default is 1:1 conversion

    if (HasFeature(endpoint, Features::Absolute))
    {
        Attributes::InstalledOpenLimitLift::Get(endpoint, &limits.open);
        Attributes::InstalledClosedLimitLift::Get(endpoint, &limits.closed);
    }

    return limits;
}

    //Attributes::InstalledOpenLimitTilt::Set(endpoint, mTilt.mLimits.open);
    //Attributes::InstalledClosedLimitTilt::Set(endpoint, mTilt.mLimits.closed);


static AbsoluteLimits TiltAbsoluteLimitsGet(chip::EndpointId endpoint)
{
    AbsoluteLimits limits = { .open = WC_PERCENT100THS_MIN_OPEN, .closed = WC_PERCENT100THS_MAX_CLOSED };// default is 1:1 conversion

    if (HasFeature(endpoint, Features::Absolute))
    {
        Attributes::InstalledOpenLimitTilt::Get(endpoint, &limits.open);
        Attributes::InstalledClosedLimitTilt::Get(endpoint, &limits.closed);
    }

    return limits;
}


PositionAccessors mLiftAccess = { 0 };
PositionAccessors mTiltAccess = { 0 };

PositionAccessors * LiftAccess(void) { return &mLiftAccess; }
PositionAccessors * TiltAccess(void) { return &mTiltAccess; }

inline Percent100ths AbsoluteToRelative(chip::EndpointId endpoint, PositionAccessors * access, uint16_t absolute) { return ValueToPercent100ths(AbsoluteLimitsGet(endpoint, access), absolute); }
inline Percent100ths LiftAbsoluteToRelative(chip::EndpointId endpoint, uint16_t absolute) { return ValueToPercent100ths(LiftAbsoluteLimitsGet(endpoint), absolute); }
inline uint16_t LiftRelativeToAbsolute(chip::EndpointId endpoint, Percent100ths relative) { return Percent100thsToValue(LiftAbsoluteLimitsGet(endpoint), relative); }

inline Percent100ths TiltAbsoluteToRelative(chip::EndpointId endpoint, uint16_t absolute) { return ValueToPercent100ths(TiltAbsoluteLimitsGet(endpoint), absolute); }
inline uint16_t TiltRelativeToAbsolute(chip::EndpointId endpoint, Percent100ths relative) { return Percent100thsToValue(TiltAbsoluteLimitsGet(endpoint), relative); }

//EmberAfStatus LiftCurrentPositionAbsoluteSet(chip::EndpointId endpoint, Percent100ths relative) { return LiftCurrentPositionRelativeSet(endpoint, LiftAbsoluteToRelative(endpoint, relative)); }
EmberAfStatus CurrentPositionAbsoluteSet(chip::EndpointId endpoint, PositionAccessors * access, Percent100ths relative) { return CurrentPositionRelativeSet(endpoint, access, AbsoluteToRelative(endpoint, access, relative)); }
EmberAfStatus TiltCurrentPositionAbsoluteSet(chip::EndpointId endpoint, Percent100ths relative) { return TiltCurrentPositionRelativeSet(endpoint, TiltAbsoluteToRelative(endpoint, relative)); }

inline EmberAfStatus LiftTargetPositionAbsoluteSet(chip::EndpointId endpoint, Percent100ths relative)  { return LiftTargetPositionRelativeSet(endpoint, LiftAbsoluteToRelative(endpoint, relative)); }
inline EmberAfStatus TiltTargetPositionAbsoluteSet(chip::EndpointId endpoint, Percent100ths relative)  { return TiltTargetPositionRelativeSet(endpoint, TiltAbsoluteToRelative(endpoint, relative)); }

inline uint16_t LiftCurrentPositionAbsoluteGet(chip::EndpointId endpoint) { return LiftRelativeToAbsolute(endpoint, LiftCurrentPositionRelativeGet(endpoint)); }
inline uint16_t TiltCurrentPositionAbsoluteGet(chip::EndpointId endpoint) { return TiltRelativeToAbsolute(endpoint, TiltCurrentPositionRelativeGet(endpoint)); }

uint16_t LiftTargetPositionAbsoluteGet(chip::EndpointId endpoint) { return LiftRelativeToAbsolute(endpoint, LiftTargetPositionRelativeGet(endpoint)); }
uint16_t TiltTargetPositionAbsoluteGet(chip::EndpointId endpoint) { return TiltRelativeToAbsolute(endpoint, TiltTargetPositionRelativeGet(endpoint)); }

// EmberAfStatus LiftTargetPositionRelativeSet(chip::EndpointId endpoint, Percent100ths relative)
// {
//     bool hasLift         = HasFeature(endpoint, Features::Lift);
//     bool isPositionAware = HasFeature(endpoint, Features::PositionAware);

//     PrintPercent100ths(__func__, relative);

//     if (hasLift && isPositionAware)
//     {
//         /* Attribute is mandatory in that case */
//         if (IsPercent100thsValid(relative))
//         {
//             Attributes::TargetPositionLiftPercent100ths::Set(endpoint, relative);
//         }
//         else
//         {
//             return EMBER_ZCL_STATUS_INVALID_VALUE;
//         }
//     }
//     else
//     {
//         return EMBER_ZCL_STATUS_UNSUP_COMMAND;
//     }

//     return EMBER_ZCL_STATUS_SUCCESS;
// }

EmberAfStatus CurrentPositionRelativeSet(chip::EndpointId endpoint, PositionAccessors * position,  Percent100ths relative)
{
    bool hasFeature      = HasFeature(endpoint, position->feature);
   // bool hasAbsolute     = HasFeature(endpoint, Features::Absolute);
    bool isPositionAware = HasFeature(endpoint, Features::PositionAware);

    PrintPercent100ths(__func__, relative);

     if (hasFeature && isPositionAware)
     {
         if (IsPercent100thsValid(relative))
         {
            if (position->SetPercentage)                position->SetPercentage(endpoint, static_cast<uint8_t>(relative / 100));
            if (position->SetPercent100ths)             position->SetPercent100ths(endpoint, relative);
           // if (hasAbsolute && position->SetAbsolute && position->RelativeToAbsolute) position->SetAbsolute(endpoint, position->RelativeToAbsolute(endpoint, relative));
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

     return EMBER_ZCL_STATUS_SUCCESS;
}

// EmberAfStatus LiftCurrentPositionRelativeSet(chip::EndpointId endpoint, Percent100ths relative)
// {
//     bool hasLift         = HasFeature(endpoint, Features::Lift);
//     bool hasAbsolute     = HasFeature(endpoint, Features::Absolute);
//     bool isPositionAware = HasFeature(endpoint, Features::PositionAware);

//     PrintPercent100ths(__func__, relative);

//     if (hasLift && isPositionAware)
//     {
//         if (IsPercent100thsValid(relative))
//         {
//             Attributes::CurrentPositionLiftPercentage::Set(endpoint, static_cast<uint8_t>(relative / 100));
//             Attributes::CurrentPositionLiftPercent100ths::Set(endpoint, relative);
//             if (hasAbsolute) Attributes::CurrentPositionLift::Set(endpoint, LiftRelativeToAbsolute(endpoint, relative));
//         }
//         else
//         {
//             return EMBER_ZCL_STATUS_INVALID_VALUE;
//         }
//     }
//     else
//     {
//         return EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
//     }

//     return EMBER_ZCL_STATUS_SUCCESS;
// }

EmberAfStatus TiltCurrentPositionRelativeSet(chip::EndpointId endpoint, Percent100ths relative)
{
    bool hasTilt         = HasFeature(endpoint, Features::Tilt);
    bool hasAbsolute     = HasFeature(endpoint, Features::Absolute);
    bool isPositionAware = HasFeature(endpoint, Features::PositionAware);

    PrintPercent100ths(__func__, relative);

    if (hasTilt && isPositionAware)
    {
        if (IsPercent100thsValid(relative))
        {
            Attributes::CurrentPositionTiltPercentage::Set(endpoint, static_cast<uint8_t>(relative / 100));
            Attributes::CurrentPositionTiltPercent100ths::Set(endpoint, relative);
            if (hasAbsolute) Attributes::CurrentPositionTilt::Set(endpoint, TiltRelativeToAbsolute(endpoint, relative));
        }
        else
        {
            return EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
        }
    }
    else
    {
        return EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}

Percent100ths LiftCurrentPositionRelativeGet(chip::EndpointId endpoint)
{
    Percent100ths relative = WC_PERCENT100THS_MIN_OPEN;

    Attributes::CurrentPositionLiftPercent100ths::Get(endpoint, &relative);

    return relative;
}

Percent100ths TiltCurrentPositionRelativeGet(chip::EndpointId endpoint)
{
    Percent100ths relative = WC_PERCENT100THS_MIN_OPEN;

    Attributes::CurrentPositionTiltPercent100ths::Get(endpoint, &relative);

    return relative;
}

Percent100ths LiftTargetPositionRelativeGet(chip::EndpointId endpoint)
{
    Percent100ths relative = WC_PERCENT100THS_MIN_OPEN;

    Attributes::TargetPositionLiftPercent100ths::Get(endpoint, &relative);

    return relative;
}

Percent100ths TiltTargetPositionRelativeGet(chip::EndpointId endpoint)
{
    Percent100ths relative = WC_PERCENT100THS_MIN_OPEN;

    Attributes::TargetPositionTiltPercent100ths::Get(endpoint, &relative);

    return relative;
}


EmberAfStatus LiftTargetPositionRelativeSet(chip::EndpointId endpoint, Percent100ths relative)
{
    bool hasLift         = HasFeature(endpoint, Features::Lift);
    bool isPositionAware = HasFeature(endpoint, Features::PositionAware);

    PrintPercent100ths(__func__, relative);

    if (hasLift && isPositionAware)
    {
        /* Attribute is mandatory in that case */
        if (IsPercent100thsValid(relative))
        {
            Attributes::TargetPositionLiftPercent100ths::Set(endpoint, relative);
        }
        else
        {
            return EMBER_ZCL_STATUS_INVALID_VALUE;
        }
    }
    else
    {
        return EMBER_ZCL_STATUS_UNSUP_COMMAND;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}

EmberAfStatus TiltTargetPositionRelativeSet(chip::EndpointId endpoint, Percent100ths relative)
{
    bool hasTilt         = HasFeature(endpoint, Features::Tilt);
    bool isPositionAware = HasFeature(endpoint, Features::PositionAware);

    PrintPercent100ths(__func__, relative);

    if (hasTilt && isPositionAware)
    {
        /* Attribute is mandatory in that case */
        if (IsPercent100thsValid(relative))
        {
            Attributes::TargetPositionTiltPercent100ths::Set(endpoint, relative);
        }
        else
        {
            return EMBER_ZCL_STATUS_INVALID_VALUE;
        }
    }
    else
    {
        /* Presence of this attribute cannot be optional */
        return EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}






void emberAfPluginWindowCoveringEventHandler(EndpointId endpoint)
{
    OperationalStatus opStatus = OperationalStatusGet(endpoint);
    emberAfWindowCoveringClusterPrint("WC DELAYED CALLBACK 100ms w/ OpStatus=0x%02X", opStatus);

    /* Update position to simulate movement to pass the CI */
    if (OperationalState::Stall != opStatus.lift)
        CurrentPositionRelativeSet(endpoint, LiftAccess(), LiftTargetPositionRelativeGet(endpoint));

    if (OperationalState::Stall != opStatus.tilt)
        TiltCurrentPositionRelativeSet(endpoint, TiltTargetPositionRelativeGet(endpoint));
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


OperationalState ComputeOperationalState(Percent100ths target, Percent100ths current, const char* name)
{
    OperationalState opState = OperationalState::Stall;

    /* Compute Operational State from Relative Target and Current */
    emberAfWindowCoveringClusterPrint("%.5s move C=%u -> T=%u", name, current, target);
    if (current != target) {
        opState = (current < target) ? OperationalState::MovingDownOrClose : OperationalState::MovingUpOrOpen;
    }
    return opState;
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
        opStatus.lift = ComputeOperationalState(LiftTargetPositionRelativeGet(endpoint), LiftCurrentPositionRelativeGet(endpoint), "Lift");
        OperationalStatusSetWithGlobalUpdated(endpoint, opStatus);
        break;
    /* For a device supporting Position Awareness : Changing the Target triggers motions on the real or simulated device */
    case Attributes::TargetPositionTiltPercent100ths::Id:
        opStatus.tilt = ComputeOperationalState(TiltTargetPositionRelativeGet(endpoint), TiltCurrentPositionRelativeGet(endpoint), "Tilt");
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

    mSuperLift.getPos = Attributes::TargetPositionTiltPercent100ths::Get;
    mSuperLift.setPos = Attributes::TargetPositionTiltPercent100ths::Set;

    LiftAccess()->SetPercentage      = Attributes::CurrentPositionLiftPercentage::Set;
    LiftAccess()->SetPercent100ths   = Attributes::CurrentPositionLiftPercent100ths::Set;
    LiftAccess()->SetAbsolute        = Attributes::CurrentPositionLift::Set;
    LiftAccess()->feature = Features::Lift;
   // mPosRel.RelativeToAbsolute = Attributes::TargetPositionTiltPercent100ths::Set

         //   (endpoint, static_cast<uint8_t>(relative / 100));
//             (endpoint, relative);
//             if (hasAbsolute) (endpoint, LiftRelativeToAbsolute(endpoint, relative));
    mSuperLift.setPos(endpoint, 5032);

    uint16_t tte;
    mSuperLift.getPos(endpoint, &tte);

    emberAfWindowCoveringClusterPrint("Window Covering Cluster init = %u", tte);

    /* Init at Half 50% */
    CurrentPositionRelativeSet(endpoint, LiftAccess(), WC_PERCENT100THS_MAX_CLOSED / 2 );
    //TiltCurrentPositionRelativeSet(endpoint, WC_PERCENT100THS_MAX_CLOSED / 2);
}

/* #################### all the time : MANDATORY COMMANDs #################### */

/**
 * @brief  Cluster UpOrOpen Command callback (from client)
 */
bool emberAfWindowCoveringClusterUpOrOpenCallback(app::CommandHandler * cmdObj, const app::ConcreteCommandPath & cmdPath,
                                                  const Commands::UpOrOpen::DecodableType & cmdData)
{
    emberAfWindowCoveringClusterPrint("UpOrOpen command received OpStatus=0x%02X", OperationalStatusGet(cmdPath.mEndpointId));

    EmberAfStatus tiltStatus = TiltTargetPositionRelativeSet(cmdPath.mEndpointId, 0/*hip::app::Clusters::WindowCovering::RelativeLimits::UpOrOpen*/);
    EmberAfStatus liftStatus = LiftTargetPositionRelativeSet(cmdPath.mEndpointId, 0/*hip::app::Clusters::WindowCovering::RelativeLimits::UpOrOpen*/);

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

    EmberAfStatus tiltStatus = TiltTargetPositionRelativeSet(cmdPath.mEndpointId, 0/*RelativeLimits::DownOrClose*/);
    EmberAfStatus liftStatus = LiftTargetPositionRelativeSet(cmdPath.mEndpointId, 0/*RelativeLimits::DownOrClose*/);

    /* By the specification definition we need to support Tilt and/or Lift -> so to simplify only one can be successfull */
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

    EmberAfStatus tiltStatus = TiltTargetPositionRelativeSet(cmdPath.mEndpointId, TiltCurrentPositionRelativeGet(cmdPath.mEndpointId));
    EmberAfStatus liftStatus = LiftTargetPositionRelativeSet(cmdPath.mEndpointId, LiftCurrentPositionRelativeGet(cmdPath.mEndpointId));

    /* By the specification definition we need to support Tilt and/or Lift -> so to simplify only one can be successfull */
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
    emberAfSendImmediateDefaultResponse(LiftTargetPositionAbsoluteSet(cmdPath.mEndpointId, cmdData.liftValue));

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
    emberAfSendImmediateDefaultResponse(LiftTargetPositionRelativeSet(cmdPath.mEndpointId, cmdData.liftPercent100thsValue));

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
    emberAfSendImmediateDefaultResponse(TiltTargetPositionAbsoluteSet(cmdPath.mEndpointId, cmdData.tiltValue));

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
    emberAfSendImmediateDefaultResponse(TiltTargetPositionRelativeSet(cmdPath.mEndpointId, cmdData.tiltPercent100thsValue));

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
