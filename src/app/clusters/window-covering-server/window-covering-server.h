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
#include <app-common/zap-generated/enums.h>
#include <app/util/af-types.h>
#include <app/util/basic-types.h>

#define WC_PERCENT100THS_MIN_OPEN   0
#define WC_PERCENT100THS_MAX_CLOSED 10000


namespace chip {
namespace app {
namespace Clusters {
namespace WindowCovering {

enum class Features
{
    Lift          = 0x01,
    Tilt          = 0x02,
    PositionAware = 0x04,
    Absolute      = 0x08,
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

bool IsLiftOpen(chip::EndpointId endpoint);
bool IsLiftClosed(chip::EndpointId endpoint);

bool IsTiltOpen(chip::EndpointId endpoint);
bool IsTiltClosed(chip::EndpointId endpoint);

// Declare Position Limit Status
enum class LimitStatus : uint8_t
{
    Intermediate      = 0x00, //
    IsUpOrOpen        = 0x01, //
    IsDownOrClose     = 0x02, //
    Inverted          = 0x03, //
    IsOverUpOrOpen    = 0x04, //
    IsOverDownOrClose = 0x05, //
};
static_assert(sizeof(LimitStatus) == sizeof(uint8_t), "LimitStatus Size is not correct");

struct AbsoluteLimits
{
    uint16_t open;
    uint16_t closed;
};

enum class RelativeLimits : Percent100ths
{
    UpOrOpen  = WC_PERCENT100THS_MIN_OPEN,
    Half   = 5000,
    DownOrClose = WC_PERCENT100THS_MAX_CLOSED,
};

bool HasFeature(chip::EndpointId endpoint, Features feature);

void TypeSet(chip::EndpointId endpoint, EmberAfWcType type);
EmberAfWcType TypeGet(chip::EndpointId endpoint);

void ConfigStatusSet(chip::EndpointId endpoint, const ConfigStatus & status);
const ConfigStatus ConfigStatusGet(chip::EndpointId endpoint);

void OperationalStatusSetWithGlobalUpdated(chip::EndpointId endpoint, OperationalStatus & status);
void OperationalStatusSet(chip::EndpointId endpoint, const OperationalStatus & status);
const OperationalStatus OperationalStatusGet(chip::EndpointId endpoint);

void EndProductTypeSet(chip::EndpointId endpoint, EmberAfWcEndProductType type);
EmberAfWcEndProductType EndProductTypeGet(chip::EndpointId endpoint);

void ModeSet(chip::EndpointId endpoint, const Mode & mode);
const Mode ModeGet(chip::EndpointId endpoint);

void SafetyStatusSet(chip::EndpointId endpoint, SafetyStatus & status);
const SafetyStatus SafetyStatusGet(chip::EndpointId endpoint);

// typedef EmberAfStatus (*SetPercentage_f          )(chip::EndpointId endpoint, uint8_t relPercentage);
// typedef EmberAfStatus (*SetPercent100ths_f       )(chip::EndpointId endpoint, uint16_t relPercent100ths);
// typedef EmberAfStatus (*SetAbsolute_f            )(chip::EndpointId endpoint, uint16_t relPercent100ths);
typedef EmberAfStatus (*GetInstalledOpenLimit_f  )(chip::EndpointId endpoint, uint16_t *relPercent100ths);
typedef EmberAfStatus (*GetInstalledClosedLimit_f)(chip::EndpointId endpoint, uint16_t *relPercent100ths);


typedef EmberAfStatus (*SetAttributeU8_f )(chip::EndpointId endpoint, uint8_t   value);
typedef EmberAfStatus (*GetAttributeU8_f )(chip::EndpointId endpoint, uint8_t *pValue);
typedef EmberAfStatus (*SetAttributeU16_f)(chip::EndpointId endpoint, uint16_t   value);
typedef EmberAfStatus (*GetAttributeU16_f)(chip::EndpointId endpoint, uint16_t *pValue);

struct PositionAccessors
{
    enum class Type : uint8_t
    {
        Current,
        Target,
    };
    SetAttributeU8_f  mSetPercentageCb;    GetAttributeU8_f  mGetPercentageCb;
    SetAttributeU16_f mSetPercent100thsCb; GetAttributeU16_f mGetPercent100thsCb;
    SetAttributeU16_f mSetAbsoluteCb;      GetAttributeU16_f mGetAbsoluteCb;


    void RegisterCallbacksPercentage   (SetAttributeU8_f  set_cb, GetAttributeU8_f  get_cb);
    void RegisterCallbacksPercent100ths(SetAttributeU16_f set_cb, GetAttributeU16_f get_cb);
    void RegisterCallbacksAbsolute     (SetAttributeU16_f set_cb, GetAttributeU16_f get_cb);

    //private:
    EmberAfStatus SetAttributeRelativePosition(chip::EndpointId endpoint, Percent100ths     relative);
    EmberAfStatus GetAttributeRelativePosition(chip::EndpointId endpoint, Percent100ths * p_relative);
    EmberAfStatus SetAttributeAbsolutePosition(chip::EndpointId endpoint, uint16_t absolute);
};

//     struct Actuator
//     {
//         AbsoluteLimits mLimits = { .open = WC_PERCENT100THS_MIN_OPEN, .closed = WC_PERCENT100THS_MAX_CLOSED };// default is 1:1 conversion

//         uint16_t mCurrentPosition    = 0;//LimitStatus::IsUpOrOpen;//WC_PERCENT100THS_DEF;
//         uint16_t mTargetPosition     = 0;//OperationalState::MovingDownOrClose;//WC_PERCENT100THS_DEF;

//         uint16_t mStepDelta          = 1;
//         uint16_t mStepMinimum        = 1;



//         uint16_t mNumberOfActuations = 0; //Number of commands sent to the actuators

//         void SetPosition(uint16_t value);
//         void StepTowardUpOrOpen();
//         void StepTowardDownOrClose();

//         void GoToAbsolute(uint16_t absolute);
//         void GoToRelative(chip::Percent100ths relative);
//         void StopMotion();
//         void UpdatePosition();
//         void Print();

//         void TimerStart();
//         void TimerStop();
//         bool IsActive();
//         LimitStatus GetLimitState();

//         static void OnActuatorTimeout(Timer & timer);
//         void Init(const char * name, uint32_t timeoutInMs, OperationalState * opState, EventId event);

// //struct OperationalStatus ff;
//       //  OperationalState * mOpState;


//         Timer *          mTimer   = nullptr;
//         EventId          mEvent  = EventId::None;
//         OperationalState mOpState = OperationalState::Stall;
//     };


struct ActuatorAccessors
{
    SetAttributeU16_f mSetOpenLimitCb;       GetAttributeU16_f mGetOpenLimitCb;
    SetAttributeU16_f mSetClosedLimitCb;     GetAttributeU16_f mGetClosedLimitCb;

    void RegisterCallbacksOpenLimit    (SetAttributeU16_f set_cb, GetAttributeU16_f get_cb);
    void RegisterCallbacksClosedLimit  (SetAttributeU16_f set_cb, GetAttributeU16_f get_cb);



    PositionAccessors mCurrent;
    PositionAccessors mTarget;

    AbsoluteLimits mLimits;
    Features mFeatureTag; //non-endpoint dependant

    void Init(chip::EndpointId endpoint, Features tag, AbsoluteLimits limits);

    bool IsPositionAware(chip::EndpointId endpoint);

    EmberAfStatus GoToUpOrOpen   (chip::EndpointId endpoint);
    EmberAfStatus GoToDownOrClose(chip::EndpointId endpoint);
    EmberAfStatus GoToCurrent    (chip::EndpointId endpoint);

    OperationalState OperationalStateGet(chip::EndpointId endpoint);

    EmberAfStatus PositionRelativeSet(chip::EndpointId endpoint, PositionAccessors::Type position, Percent100ths relative);
    EmberAfStatus PositionAbsoluteSet(chip::EndpointId endpoint, PositionAccessors::Type position, uint16_t absolute);

    Percent100ths PositionRelativeGet(chip::EndpointId endpoint, PositionAccessors::Type position);
    uint16_t      PositionAbsoluteGet(chip::EndpointId endpoint, PositionAccessors::Type position);

    Percent100ths AbsoluteToRelative(chip::EndpointId endpoint, uint16_t absolute);
    uint16_t      RelativeToAbsolute(chip::EndpointId endpoint, Percent100ths relative);

    AbsoluteLimits AbsoluteLimitsGet(chip::EndpointId endpoint);
    void           AbsoluteLimitsSet(chip::EndpointId endpoint, AbsoluteLimits limits);

    uint16_t WithinAbsoluteRangeCheck(uint16_t value);

    private:
    EmberAfStatus SetAttributeAbsoluteLimits(chip::EndpointId endpoint, AbsoluteLimits     limits);
    EmberAfStatus GetAttributeAbsoluteLimits(chip::EndpointId endpoint, AbsoluteLimits * p_limits);
} ;

ActuatorAccessors * LiftAccess(void);
ActuatorAccessors * TiltAccess(void);

LimitStatus CheckLimitState(uint16_t position, AbsoluteLimits limits);

/* Setter/Getter Helpers to simplify interaction with the positioning attributes */
//EmberAfStatus PositionRelativeSet(chip::EndpointId endpoint, PositionAccessors * access, Percent100ths relative);
//EmberAfStatus PositionAbsoluteSet(chip::EndpointId endpoint, PositionAccessors * access, uint16_t absolute);//cover->mEndpoint, LiftAccess(), cover->mLift.mCurrentPosition);

//Percent100ths PositionRelativeGet(chip::EndpointId endpoint, PositionAccessors * access);
//uint16_t      PositionAbsoluteGet(chip::EndpointId endpoint, PositionAccessors * access);




// EmberAfStatus  TargetPositionRelativeSet(chip::EndpointId endpoint, PositionAccessors * access, Percent100ths relative);
// EmberAfStatus  TargetPositionAbsoluteSet(chip::EndpointId endpoint, PositionAccessors * access, uint16_t absolute);
// EmberAfStatus  TargetPositionRelativeSet(chip::EndpointId endpoint, PositionAccessors * access, Percent100ths relative);
// Percent100ths  TargetPositionRelativeGet(chip::EndpointId endpoint, PositionAccessors * access);

//EmberAfStatus LiftCurrentPositionRelativeSet(chip::EndpointId endpoint, Percent100ths relative);
//EmberAfStatus LiftCurrentPositionAbsoluteSet(chip::EndpointId endpoint, uint16_t absolute);
//Percent100ths LiftCurrentPositionRelativeGet(chip::EndpointId endpoint);
//uint16_t      LiftCurrentPositionAbsoluteGet(chip::EndpointId endpoint);

//EmberAfStatus LiftTargetPositionRelativeSet(chip::EndpointId endpoint, Percent100ths relative);
//EmberAfStatus LiftTargetPositionAbsoluteSet(chip::EndpointId endpoint, uint16_t absolute);
//Percent100ths LiftTargetPositionRelativeGet(chip::EndpointId endpoint);
//uint16_t      LiftTargetPositionAbsoluteGet(chip::EndpointId endpoint);

//EmberAfStatus TiltCurrentPositionRelativeSet(chip::EndpointId endpoint, Percent100ths relative);
//EmberAfStatus TiltCurrentPositionAbsoluteSet(chip::EndpointId endpoint, uint16_t absolute);
//Percent100ths TiltCurrentPositionRelativeGet(chip::EndpointId endpoint);
//uint16_t      TiltCurrentPositionAbsoluteGet(chip::EndpointId endpoint);

//EmberAfStatus TiltTargetPositionRelativeSet(chip::EndpointId endpoint, Percent100ths relative);
//EmberAfStatus TiltTargetPositionAbsoluteSet(chip::EndpointId endpoint, uint16_t absolute);
//Percent100ths TiltTargetPositionRelativeGet(chip::EndpointId endpoint);
//uint16_t      TiltTargetPositionAbsoluteGet(chip::EndpointId endpoint);

LimitStatus LiftLimitStatusGet(chip::EndpointId endpoint);
LimitStatus TiltLimitStatusGet(chip::EndpointId endpoint);


/* Units Conversion from Absolute to Relative */
// Percent100ths LiftToPercent100ths(chip::EndpointId endpoint, uint16_t absoluteValue);
// Percent100ths TiltToPercent100ths(chip::EndpointId endpoint, uint16_t absoluteValue);

// /* Units Conversion from Relative to Absolute */
// uint16_t Percent100thsToTilt(chip::EndpointId endpoint, Percent100ths percent100ths);
// uint16_t Percent100thsToLift(chip::EndpointId endpoint, Percent100ths percent100ths);

OperationalState ComputeOperationalState(uint16_t target, uint16_t current);
void PostAttributeChange(chip::EndpointId endpoint, chip::AttributeId attributeId);

} // namespace WindowCovering
} // namespace Clusters
} // namespace app
} // namespace chip

void WindowCoveringPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & attributePath, uint8_t mask, uint8_t type, uint16_t size, uint8_t * value);
