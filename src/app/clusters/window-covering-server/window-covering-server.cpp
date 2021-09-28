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

#define WC_PERCENT100THS_MIN_OPEN   0
#define WC_PERCENT100THS_MAX_CLOSED 10000

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

static uint16_t ConvertValue(uint16_t inputLowValue, uint16_t inputHighValue, uint16_t outputLowValue, uint16_t outputHighValue, uint16_t value)
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

    if (value < inputMin)
    {
        return outputMin;
    }
    
    if (value > inputMax)
    {
        return outputMax;
    }

    if (inputRange > 0)
    {
        return static_cast<uint16_t>(outputMin + ((outputRange * (value - inputMin) / inputRange)));
    }

    return outputMax;
}


static uint16_t ValueToPercent100ths(uint16_t openLimit, uint16_t closedLimit, uint16_t value)
{
    return ConvertValue(openLimit, closedLimit, WC_PERCENT100THS_MIN_OPEN, WC_PERCENT100THS_MAX_CLOSED, value);
}

static uint16_t Percent100thsToValue(uint16_t openLimit, uint16_t closedLimit, uint16_t percent100ths)
{
    return ConvertValue(WC_PERCENT100THS_MIN_OPEN, WC_PERCENT100THS_MAX_CLOSED, openLimit, closedLimit, percent100ths);
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
namespace WindowCovering {

bool HasFeature(chip::EndpointId endpoint, Features feature)
{
    return true;
}

void PrintPercent100ths(const char * pMessage, uint16_t percent100ths)
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

uint16_t LiftToPercent100ths(chip::EndpointId endpoint, uint16_t lift)
{
    uint16_t openLimit   = 0;
    uint16_t closedLimit = 0;
    Attributes::InstalledOpenLimitLift::Get(endpoint, &openLimit);
    Attributes::InstalledClosedLimitLift::Get(endpoint, &closedLimit);
    return ValueToPercent100ths(openLimit, closedLimit, lift);
}

uint16_t Percent100thsToLift(chip::EndpointId endpoint, uint16_t percent100ths)
{
    uint16_t openLimit   = 0;
    uint16_t closedLimit = 0;
    Attributes::InstalledOpenLimitLift::Get(endpoint, &openLimit);
    Attributes::InstalledClosedLimitLift::Get(endpoint, &closedLimit);
    return Percent100thsToValue(openLimit, closedLimit, percent100ths);
}

EmberAfStatus LiftCurrentPositionSet(chip::EndpointId endpoint, uint16_t percent100ths)
{
    bool hasLift         = HasFeature(endpoint, Features::Lift);
    bool hasAbsolute     = HasFeature(endpoint, Features::Absolute);
    bool isPositionAware = HasFeature(endpoint, Features::PositionAware);

    PrintPercent100ths(__func__, percent100ths);

    if (hasLift)
    {
        if (isPositionAware)
        {
            if (IsPercent100thsValid(percent100ths))
            {
                Attributes::SetCurrentPositionLiftPercentage(endpoint, static_cast<uint8_t>(percent100ths / 100));
                Attributes::SetCurrentPositionLiftPercent100ths(endpoint, percent100ths);
                if (hasAbsolute) Attributes::SetCurrentPositionLift(endpoint, Percent100thsToLift(endpoint, percent100ths));
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
    }
    else
    {
        return EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}

uint16_t LiftCurrentPositionGet(chip::EndpointId endpoint)
{
    uint16_t percent100ths = WC_PERCENT100THS_MIN_OPEN;

    Attributes::GetTargetPositionLiftPercent100ths(endpoint, &percent100ths);

    return percent100ths;
}

EmberAfStatus LiftTargetPositionSet(chip::EndpointId endpoint, uint16_t percent100ths)
{
    bool hasLift         = HasFeature(endpoint, Features::Lift);
    bool isPositionAware = HasFeature(endpoint, Features::PositionAware);

    PrintPercent100ths(__func__, percent100ths);

    if (hasLift)
    {
        if (isPositionAware)
        {
            if (IsPercent100thsValid(percent100ths))
            {
                Attributes::SetTargetPositionLiftPercent100ths(endpoint, percent100ths);
            }
            else
            {
                return EMBER_ZCL_STATUS_INVALID_VALUE;
            }
        }
        else
        {
            /* If the server does not support the Position Aware feature,
             then a zero percentage SHOULD be treated as a DownOrClose command and a non-zero percentage SHOULD be treated as an UpOrOpen command
            */
            /* TODO rewrite this part later */
            Attributes::SetTargetPositionLiftPercent100ths(endpoint, percent100ths ? WC_PERCENT100THS_MIN_OPEN : WC_PERCENT100THS_MAX_CLOSED);
        }
    }
    else
    {
        return EMBER_ZCL_STATUS_UNSUP_COMMAND;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}

uint16_t TiltToPercent100ths(chip::EndpointId endpoint, uint16_t tilt)
{
    uint16_t openLimit   = 0;
    uint16_t closedLimit = 0;
    Attributes::InstalledOpenLimitTilt::Get(endpoint, &openLimit);
    Attributes::InstalledClosedLimitTilt::Get(endpoint, &closedLimit);
    return ValueToPercent100ths(openLimit, closedLimit, tilt);
}

uint16_t Percent100thsToTilt(chip::EndpointId endpoint, uint16_t percent100ths)
{
    uint16_t openLimit   = 0;
    uint16_t closedLimit = 0;
    Attributes::InstalledOpenLimitTilt::Get(endpoint, &openLimit);
    Attributes::InstalledClosedLimitTilt::Get(endpoint, &closedLimit);
    return Percent100thsToValue(openLimit, closedLimit, percent100ths);
}

EmberAfStatus TiltCurrentPositionSet(chip::EndpointId endpoint, uint16_t percent100ths)
{
    bool hasTilt         = HasFeature(endpoint, Features::Tilt);
    bool hasAbsolute     = HasFeature(endpoint, Features::Absolute);
    bool isPositionAware = HasFeature(endpoint, Features::PositionAware);

    PrintPercent100ths(__func__, percent100ths);

    if (hasTilt)
    {
        if (isPositionAware)
        {
            if (IsPercent100thsValid(percent100ths))
            {
                Attributes::SetCurrentPositionTiltPercentage(endpoint, static_cast<uint8_t>(percent100ths / 100));
                Attributes::SetCurrentPositionTiltPercent100ths(endpoint, percent100ths);
                if (hasAbsolute) Attributes::SetCurrentPositionTilt(endpoint, Percent100thsToTilt(endpoint, percent100ths));
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
    }
    else
    {
        return EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}

uint16_t TiltCurrentPositionGet(chip::EndpointId endpoint)
{
    uint16_t percent100ths = WC_PERCENT100THS_MIN_OPEN;

    Attributes::GetTargetPositionTiltPercent100ths(endpoint, &percent100ths);

    return percent100ths;
}

uint16_t TiltTargetPositionGet(chip::EndpointId endpoint)
{
    uint16_t percent100ths = WC_PERCENT100THS_MIN_OPEN;

    Attributes::GetCurrentPositionTiltPercent100ths(endpoint, &percent100ths);

    return percent100ths;
}

uint16_t LiftTargetPositionGet(chip::EndpointId endpoint)
{
    uint16_t percent100ths = WC_PERCENT100THS_MIN_OPEN;

    Attributes::GetCurrentPositionLiftPercent100ths(endpoint, &percent100ths);

    return percent100ths;
}



EmberAfStatus TiltTargetPositionSet(chip::EndpointId endpoint, uint16_t percent100ths)
{
    bool hasTilt         = HasFeature(endpoint, Features::Tilt);
    bool isPositionAware = HasFeature(endpoint, Features::PositionAware);

    PrintPercent100ths(__func__, percent100ths);

    if (hasTilt)
    {
        if (isPositionAware)
        {
            if (IsPercent100thsValid(percent100ths))
            {
                Attributes::SetTargetPositionTiltPercent100ths(endpoint, percent100ths);
            }
            else
            {
                return EMBER_ZCL_STATUS_INVALID_VALUE;
            }
        }
        else
        {
            /* If the server does not support the Position Aware feature,
             then a zero percentage SHOULD be treated as a DownOrClose command and a non-zero percentage SHOULD be treated as an UpOrOpen command
            */
            Attributes::SetTargetPositionTiltPercent100ths(endpoint, percent100ths ? WC_PERCENT100THS_MIN_OPEN : WC_PERCENT100THS_MAX_CLOSED);
        }
    }
    else
    {
        return EMBER_ZCL_STATUS_UNSUP_COMMAND;
    }

    return EMBER_ZCL_STATUS_SUCCESS;
}

/* PostAttributeChange is used in all-cluster-app simulation and CI testing : otherwise it is bounded to manufacturer specific implementation */
void PostAttributeChange(chip::EndpointId endpoint, chip::AttributeId attributeId)
{
    OperationalStatus opStatus = OperationalStatusGet(endpoint);
    uint16_t posTarget = 0, posCurrent = 0;

    emberAfWindowCoveringClusterPrint("WC POST ATTRIBUTE=%u OpStatus=0x%02X", attributeId, opStatus);

    switch (attributeId)
    {
    /* RO Type: Cycling Window Covering Demo */
    case ZCL_WC_TYPE_ATTRIBUTE_ID:
        break;
    /* RO ConfigStatus */
    case ZCL_WC_CONFIG_STATUS_ATTRIBUTE_ID:
        break;
    /* RO OperationalStatus */
    case ZCL_WC_OPERATIONAL_STATUS_ATTRIBUTE_ID:
        if (OperationalState::Stall != opStatus.lift)
            LiftCurrentPositionSet(endpoint, LiftTargetPositionGet(endpoint));
        if (OperationalState::Stall != opStatus.tilt)
            TiltCurrentPositionSet(endpoint, TiltTargetPositionGet(endpoint));
        break;
    /* RO EndProductType */
    case ZCL_WC_END_PRODUCT_TYPE_ATTRIBUTE_ID:
        break;
    /* RW Mode */
    case ZCL_WC_MODE_ATTRIBUTE_ID:
        break;
    /* RO SafetyStatus */
    case ZCL_WC_SAFETY_STATUS_ATTRIBUTE_ID:
        break;
    /* ============= Positions for Position Aware ============= */
    case ZCL_WC_CURRENT_POSITION_LIFT_PERCENT100_THS_ATTRIBUTE_ID:
        if (OperationalState::Stall != opStatus.lift) {
            opStatus.lift = OperationalState::Stall;
            emberAfWindowCoveringClusterPrint("Lift stop");
            OperationalStatusSetWithGlobalUpdated(endpoint, opStatus);
        }
        break;
    case ZCL_WC_CURRENT_POSITION_TILT_PERCENT100_THS_ATTRIBUTE_ID:
        if (OperationalState::Stall != opStatus.tilt) {
            opStatus.tilt = OperationalState::Stall;
            emberAfWindowCoveringClusterPrint("Tilt stop");
            OperationalStatusSetWithGlobalUpdated(endpoint, opStatus);
        }
        break;
    /* For a device supporting Position Awareness : Changing the Target triggers motions on the real or simulated device */
    case ZCL_WC_TARGET_POSITION_LIFT_PERCENT100_THS_ATTRIBUTE_ID:
        posTarget  = LiftTargetPositionGet(endpoint);
        posCurrent = LiftCurrentPositionGet(endpoint);
        emberAfWindowCoveringClusterPrint("Lift move C=%u -> T=%u", posCurrent, posTarget);
        if (posCurrent != posTarget) {
            opStatus.lift = (posCurrent < posTarget) ? OperationalState::MovingDownOrClose : OperationalState::MovingUpOrOpen;
            OperationalStatusSetWithGlobalUpdated(endpoint, opStatus);
        }
        break;
    /* For a device supporting Position Awareness : Changing the Target triggers motions on the real or simulated device */
    case ZCL_WC_TARGET_POSITION_TILT_PERCENT100_THS_ATTRIBUTE_ID:
        posTarget  = TiltTargetPositionGet(endpoint);
        posCurrent = TiltCurrentPositionGet(endpoint);
        emberAfWindowCoveringClusterPrint("Tilt move C=%u -> T=%u", posCurrent, posTarget);
        if (posCurrent != posTarget) {
            opStatus.tilt = (posCurrent < posTarget) ? OperationalState::MovingDownOrClose : OperationalState::MovingUpOrOpen;
            OperationalStatusSetWithGlobalUpdated(endpoint, opStatus);
        }
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

/** @brief Window Covering Cluster Init
 *
 * Cluster Init
 *
 * @param endpoint    Endpoint that is being initialized
 */
void emberAfWindowCoveringClusterInitCallback(chip::EndpointId endpoint)
{
    emberAfWindowCoveringClusterPrint("Window Covering Cluster init");

    /* Init at Half 50% */
    LiftCurrentPositionSet(endpoint, 5000);
    TiltCurrentPositionSet(endpoint, 5000);
}

/**
 * @brief  Cluster UpOrOpen Command callback (from client)
 */
bool emberAfWindowCoveringClusterUpOrOpenCallback(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
                                                  const Commands::UpOrOpen::DecodableType & commandData)
{
    EndpointId endpoint = commandPath.mEndpointId;

    emberAfWindowCoveringClusterPrint("UpOrOpen command received");

    EmberAfStatus tiltStatus = TiltTargetPositionSet(endpoint, WC_PERCENT100THS_MIN_OPEN);
    EmberAfStatus liftStatus = LiftTargetPositionSet(endpoint, WC_PERCENT100THS_MIN_OPEN);

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
bool emberAfWindowCoveringClusterDownOrCloseCallback(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
                                                     const Commands::DownOrClose::DecodableType & commandData)
{
    EndpointId endpoint = commandPath.mEndpointId;

    emberAfWindowCoveringClusterPrint("DownOrClose command received");

    EmberAfStatus tiltStatus = TiltTargetPositionSet(endpoint, WC_PERCENT100THS_MAX_CLOSED);
    EmberAfStatus liftStatus = LiftTargetPositionSet(endpoint, WC_PERCENT100THS_MAX_CLOSED);

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
bool emberAfWindowCoveringClusterStopMotionCallback(chip::EndpointId endpoint, chip::app::CommandHandler * commandObj)
{
    emberAfWindowCoveringClusterPrint("StopMotion command received");

    EmberAfStatus tiltStatus = TiltTargetPositionSet(endpoint, TiltCurrentPositionGet(endpoint));
    EmberAfStatus liftStatus = LiftTargetPositionSet(endpoint, LiftCurrentPositionGet(endpoint));

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
emberAfWindowCoveringClusterStopMotionCallback(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
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

/**
 * @brief  Cluster GoToLiftValue Command callback (from client)
 */
bool emberAfWindowCoveringClusterGoToLiftValueCallback(app::CommandHandler * commandObj,
                                                       const app::ConcreteCommandPath & commandPath,
                                                       const Commands::GoToLiftValue::DecodableType & commandData)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_UNSUP_COMMAND;
    bool hasAbsolute = HasFeature(endpoint, Features::Absolute);

    emberAfWindowCoveringClusterPrint("GoToLiftValue command received w/ %u", liftValue);

    if (hasAbsolute)
    {
        status = LiftTargetPositionSet(endpoint, LiftToPercent100ths(endpoint, liftValue));
    }

    emberAfSendImmediateDefaultResponse(status);

    return true;
}

/**
 * @brief  Cluster GoToLiftPercentage Command callback (from client)
 */
bool emberAfWindowCoveringClusterGoToLiftPercentageCallback(app::CommandHandler * commandObj,
                                                            const app::ConcreteCommandPath & commandPath,
                                                            const Commands::GoToLiftPercentage::DecodableType & commandData)
{
    auto & liftPercentageValue    = commandData.liftPercentageValue;
    auto & liftPercent100thsValue = commandData.liftPercent100thsValue;

    emberAfWindowCoveringClusterPrint("GoToLiftPercentage command received w/ %u, %u", liftPercentageValue, liftPercent100thsValue);

    emberAfSendImmediateDefaultResponse(LiftTargetPositionSet(endpoint, liftPercent100thsValue));

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
bool emberAfWindowCoveringClusterGoToTiltValueCallback(app::CommandHandler * commandObj,
                                                       const app::ConcreteCommandPath & commandPath,
                                                       const Commands::GoToTiltValue::DecodableType & commandData)
{
    auto & tiltValue = commandData.tiltValue;

    EndpointId endpoint = commandPath.mEndpointId;

    EmberAfStatus status = EMBER_ZCL_STATUS_UNSUP_COMMAND;
    bool hasAbsolute = HasFeature(endpoint, Features::Absolute);

    emberAfWindowCoveringClusterPrint("GoToTiltValue command received w/ %u", tiltValue);

    if (hasAbsolute)
    {
        status = TiltTargetPositionSet(endpoint, TiltToPercent100ths(endpoint, tiltValue));
    }

    emberAfSendImmediateDefaultResponse(status);

    return true;
}

/**
 * @brief  Cluster GoToTiltPercentage Command callback (from client)
 */
bool emberAfWindowCoveringClusterGoToTiltPercentageCallback(app::CommandHandler * commandObj,
                                                            const app::ConcreteCommandPath & commandPath,
                                                            const Commands::GoToTiltPercentage::DecodableType & commandData)
{
    auto & tiltPercentageValue    = commandData.tiltPercentageValue;
    auto & tiltPercent100thsValue = commandData.tiltPercent100thsValue;

    EndpointId endpoint = commandPath.mEndpointId;

    emberAfWindowCoveringClusterPrint("GoToTiltPercentage command received w/ %u, %u", tiltPercentageValue, tiltPercent100thsValue);

    emberAfSendImmediateDefaultResponse(TiltTargetPositionSet(endpoint, tiltPercent100thsValue));

    return true;
}

/**
 * @brief  Cluster Attribute Update Callback
 */
void __attribute__((weak)) emberAfWindowCoveringClusterPostAttributeChangeCallback(chip::EndpointId endpoint, chip::AttributeId attributeId, uint8_t mask,
                                        uint16_t manufacturerCode, uint8_t type, uint16_t size, uint8_t * value)
{
    PostAttributeChange(endpoint, attributeId);
}

void MatterWindowCoveringPluginServerInitCallback() {}
