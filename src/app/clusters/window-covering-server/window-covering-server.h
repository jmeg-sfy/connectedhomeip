/*
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

#pragma once

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <app-common/zap-generated/enums.h>
#include <app/util/af-types.h>
#include <app/util/basic-types.h>
#include <map>

#include <app/data-model/Nullable.h>

#define WC_PERCENT100THS_MIN_OPEN 0
#define WC_PERCENT100THS_MAX_CLOSED 10000

namespace chip {
namespace app {
namespace Clusters {
namespace WindowCovering {

typedef DataModel::Nullable<Percent> NPercent;
typedef DataModel::Nullable<Percent100ths> NPercent100ths;
typedef DataModel::Nullable<uint16_t> NAbsolute;

static const std::map<WcFeature, const char *> mFeatureId = {
    { WcFeature::kLift             , "Lift"   },
    { WcFeature::kTilt             , "Tilt"   },
    { WcFeature::kPositionAwareLift, "LiftPa" },
    { WcFeature::kAbsolutePosition , "ABS"    },
    { WcFeature::kPositionAwareTilt, "TiltPa" },
};

struct Mode
{
    uint8_t motorDirReversed : 1; // bit 0
    uint8_t calibrationMode : 1;  // bit 1
    uint8_t maintenanceMode : 1;  // bit 2
    uint8_t ledDisplay : 1;       // bit 3
};
static_assert(sizeof(Mode) == sizeof(uint8_t), "Mode Size is not correct");

struct ConfigStatus
{
    uint8_t operational : 1;             // bit 0 M
    uint8_t online : 1;                  // bit 1 M
    uint8_t liftIsReversed : 1;          // bit 2 LF
    uint8_t liftIsPA : 1;                // bit 3 LF & PA
    uint8_t tiltIsPA : 1;                // bit 4 TL & PA
    uint8_t liftIsEncoderControlled : 1; // bit 5 LF & PA
    uint8_t tiltIsEncoderControlled : 1; // bit 6 LF & PA
};
static_assert(sizeof(ConfigStatus) == sizeof(uint8_t), "ConfigStatus Size is not correct");

// Match directly with OperationalStatus 2 bits Fields
enum class OperationalState : uint8_t
{
    Stall             = 0x00, // currently not moving
    MovingUpOrOpen    = 0x01, // is currently opening
    MovingDownOrClose = 0x02, // is currently closing
    Reserved          = 0x03, // dont use
};
static_assert(sizeof(OperationalState) == sizeof(uint8_t), "OperationalState Size is not correct");

struct OperationalStatus
{
    OperationalState global : 2; // bit 0-1 M
    OperationalState lift : 2;   // bit 2-3 LF
    OperationalState tilt : 2;   // bit 4-5 TL
};
static_assert(sizeof(OperationalStatus) == sizeof(uint8_t), "OperationalStatus Size is not correct");

struct SafetyStatus
{
    uint8_t remoteLockout : 1;       // bit 0
    uint8_t tamperDetection : 1;     // bit 1
    uint8_t failedCommunication : 1; // bit 2
    uint8_t positionFailure : 1;     // bit 3
    uint8_t thermalProtection : 1;   // bit 4
    uint8_t obstacleDetected : 1;    // bit 5
    uint8_t powerIssue : 1;          // bit 6
    uint8_t stopInput : 1;           // bit 7
    uint8_t motorJammed : 1;         // bit 8
    uint8_t hardwareFailure : 1;     // bit 9
    uint8_t manualOperation : 1;     // bit 10
};
static_assert(sizeof(SafetyStatus) == sizeof(uint16_t), "SafetyStatus Size is not correct");

// Declare Position Limit Status
enum class LimitStatus : uint8_t
{
    Intermediate      = 0x00,
    IsUpOrOpen        = 0x01,
    IsDownOrClose     = 0x02,
    Inverted          = 0x03,
    IsPostUpOrOpen    = 0x04,
    IsPostDownOrClose = 0x05,
};
static_assert(sizeof(LimitStatus) == sizeof(uint8_t), "LimitStatus Size is not correct");

struct AbsoluteLimits
{
    uint16_t open;
    uint16_t closed;
};

typedef app::DataModel::Nullable<Percent> NPercent;
typedef app::DataModel::Nullable<Percent100ths> NPercent100ths;
typedef app::DataModel::Nullable<uint16_t> NAbsolute;

typedef EmberAfStatus (*SetAttributeU8_f )           (EndpointId endpoint, uint8_t   value);
typedef EmberAfStatus (*GetAttributeU8_f )           (EndpointId endpoint, uint8_t *pValue);
typedef EmberAfStatus (*SetAttributeU16_f)           (EndpointId endpoint, uint16_t   value);
typedef EmberAfStatus (*GetAttributeU16_f)           (EndpointId endpoint, uint16_t *pValue);
typedef EmberAfStatus (*SetAttributePercent_f )      (EndpointId endpoint, const NPercent&);
typedef EmberAfStatus (*GetAttributePercent_f )      (EndpointId endpoint, NPercent&);
typedef EmberAfStatus (*SetAttributePercent100ths_f) (EndpointId endpoint, const NPercent100ths&);
typedef EmberAfStatus (*GetAttributePercent100ths_f) (EndpointId endpoint, NPercent100ths&);
typedef EmberAfStatus (*SetAttributeNullableU16_f)   (EndpointId endpoint, const NAbsolute&);
typedef EmberAfStatus (*GetAttributeNullableU16_f)   (EndpointId endpoint, NAbsolute&);

/* Represents the Actuator part of the WindowCovering and helps to handle attributes complexity between RELATIVE and ABSOLUTE attribute */
struct ActuatorAccessors
{
    /* Represents the Positioning part and helps to handle attributes complexity between RELATIVE and ABSOLUTE attribute */
    struct PositionAccessors
    {
        enum class Type : uint8_t
        {
            Current,
            Target,
        };
        SetAttributePercent_f  mSetPercentageCb;    GetAttributePercent_f  mGetPercentageCb;
        SetAttributePercent100ths_f mSetPercent100thsCb; GetAttributePercent100ths_f mGetPercent100thsCb;
        SetAttributeNullableU16_f mSetAbsoluteCb;      GetAttributeNullableU16_f mGetAbsoluteCb;

        void RegisterCallbacksPercentage   (SetAttributePercent_f  set_cb, GetAttributePercent_f  get_cb);
        void RegisterCallbacksPercent100ths(SetAttributePercent100ths_f set_cb, GetAttributePercent100ths_f get_cb);
        void RegisterCallbacksAbsolute     (SetAttributeNullableU16_f set_cb, GetAttributeNullableU16_f get_cb);

        //private:
        EmberAfStatus SetAttributeRelativePosition(chip::EndpointId endpoint, const NPercent100ths& relative);
        EmberAfStatus GetAttributeRelativePosition(chip::EndpointId endpoint, NPercent100ths& relative);
        EmberAfStatus SetAttributeAbsolutePosition(chip::EndpointId endpoint, const NAbsolute& absolute);
    }; // struct PositionAccessors

    SetAttributeU16_f mSetOpenLimitCb;       GetAttributeU16_f mGetOpenLimitCb;
    SetAttributeU16_f mSetClosedLimitCb;     GetAttributeU16_f mGetClosedLimitCb;

    void RegisterCallbacksOpenLimit    (SetAttributeU16_f set_cb, GetAttributeU16_f get_cb);
    void RegisterCallbacksClosedLimit  (SetAttributeU16_f set_cb, GetAttributeU16_f get_cb);

    PositionAccessors mCurrent;
    PositionAccessors mTarget;

    AbsoluteLimits mLimits = { .open = WC_PERCENT100THS_MIN_OPEN, .closed = WC_PERCENT100THS_MAX_CLOSED }; // default is 1:1 conversion
    WcFeature mFeatureTag; //non-endpoint dependant

    void InitializeCallbacks(chip::EndpointId endpoint, WcFeature tag);
    void InitializeLimits(chip::EndpointId endpoint);
    void InitializeLimits(chip::EndpointId endpoint, AbsoluteLimits limits);

    bool IsPositionAware(chip::EndpointId endpoint);

    /* Compute the operational state of an actuator from current and target */
    OperationalState OperationalStateGet(chip::EndpointId endpoint);

    /* Setter/Getter Helpers to simplify interaction with the positioning attributes */
    EmberAfStatus GoToUpOrOpen      (chip::EndpointId endpoint);
    EmberAfStatus GoToDownOrClose   (chip::EndpointId endpoint);
    EmberAfStatus GoToCurrent       (chip::EndpointId endpoint);
    EmberAfStatus SetCurrentAtTarget(chip::EndpointId endpoint);

    EmberAfStatus PositionRelativeSet(chip::EndpointId endpoint, PositionAccessors::Type type, const NPercent100ths& relative);
    EmberAfStatus PositionRelativeSet(chip::EndpointId endpoint, PositionAccessors::Type type, Percent100ths relative);
    EmberAfStatus PositionAbsoluteSet(chip::EndpointId endpoint, PositionAccessors::Type type, const NAbsolute& absolute);
    EmberAfStatus PositionAbsoluteSet(chip::EndpointId endpoint, PositionAccessors::Type type, uint16_t absolute);

    EmberAfStatus PositionRelativeGet(chip::EndpointId endpoint, PositionAccessors::Type type, NPercent100ths& relative);
    EmberAfStatus PositionAbsoluteGet(chip::EndpointId endpoint, PositionAccessors::Type type, NAbsolute& absolute);

    /* Units Conversion from Absolute to Relative */
    void AbsoluteToRelative(chip::EndpointId endpoint, const NAbsolute& absolute, NPercent100ths& relative);
    /* Units Conversion from Relative to Absolute */
    void RelativeToAbsolute(chip::EndpointId endpoint, const NPercent100ths& relative, NAbsolute& absolute);

    uint16_t WithinAbsoluteRangeCheck(uint16_t value);

    /* Absolute Limits are exposed by device using the ABS flag in the FeatureMap */
    AbsoluteLimits AbsoluteLimitsGet(chip::EndpointId endpoint);
    void           AbsoluteLimitsSet(chip::EndpointId endpoint, AbsoluteLimits limits);

    EmberAfStatus SetAttributeAbsoluteLimits(chip::EndpointId endpoint, AbsoluteLimits     limits);
    EmberAfStatus GetAttributeAbsoluteLimits(chip::EndpointId endpoint, AbsoluteLimits * p_limits);
}; // struct ActuatorAccessors

ActuatorAccessors & LiftAccess(void);
ActuatorAccessors & TiltAccess(void);

bool HasFeature(chip::EndpointId endpoint, WcFeature feature);

void TypeSet(chip::EndpointId endpoint, EmberAfWcType type);
EmberAfWcType TypeGet(chip::EndpointId endpoint);

void ConfigStatusSet(chip::EndpointId endpoint, const ConfigStatus & status);
const ConfigStatus ConfigStatusGet(chip::EndpointId endpoint);

void OperationalStatusSet(chip::EndpointId endpoint, const OperationalStatus & status);
void OperationalStatusSetWithGlobalUpdated(chip::EndpointId endpoint, OperationalStatus & status);
const OperationalStatus OperationalStatusGet(chip::EndpointId endpoint);

OperationalState ComputeOperationalState(uint16_t target, uint16_t current);
OperationalState ComputeOperationalState(NPercent100ths target, NPercent100ths current);

void EndProductTypeSet(chip::EndpointId endpoint, EmberAfWcEndProductType type);
EmberAfWcEndProductType EndProductTypeGet(chip::EndpointId endpoint);

void ModeSet(chip::EndpointId endpoint, const Mode & mode);
const Mode ModeGet(chip::EndpointId endpoint);

void SafetyStatusSet(chip::EndpointId endpoint, SafetyStatus & status);
const SafetyStatus SafetyStatusGet(chip::EndpointId endpoint);

LimitStatus CheckLimitState(uint16_t position, AbsoluteLimits limits);

bool IsPercent100thsValid(Percent100ths percent100ths);
bool IsPercent100thsValid(NPercent100ths npercent100ths);

void PostAttributeChange(chip::EndpointId endpoint, chip::AttributeId attributeId);

} // namespace WindowCovering
} // namespace Clusters
} // namespace app
} // namespace chip

void WindowCoveringPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & attributePath, uint8_t mask, uint8_t type, uint16_t size, uint8_t * value);
