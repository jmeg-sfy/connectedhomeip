/*
 *
 *    Copyright (c) 2019 Google LLC.
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

#include <AppConfig.h>
#include <AppEvent.h>
#include <AppTimer.h>
#include <memory>
#include <stdint.h>
#include <vector>
#include <map>


#include <app/clusters/window-covering-server/window-covering-common.h>

#define LIFT_DELTA (LCD_COVER_SIZE / 10)
#define TILT_DELTA 1

#define SELECT_LIFT false
#define SELECT_TILT true

class WindowCover
{
public:



    typedef struct CoverActuator
    {
        /* data */
        OperationalState_e state;
        uint16_t openLimit;                // Attribute: Id 16, 18 InstalledOpenLimit          PA
        uint16_t closedLimit;              // Attribute: Id 17, 19 InstalledClosedLimit        PA
        uint16_t currentPosition;          // Attribute: Id 3, 4, 8, 9, 14, 15 CurrentPosition PA
        uint16_t targetPosition;           // Attribute: Id 11, 12              TargetPosition PA
        uint16_t stepDelta;                // Attribute: Id
        uint16_t physicalClosedLimit;      // Attribute: Id 1, 2           PhysicalClosedLimit [PA]
        uint16_t numberOfActuations;       // Attribute: Id 5, 6           NumberOfActuations  O
        AppEvent::EventType event;
        AppTimer timer;
    } CoverActuator_t;

    static const char * TypeGetString(const EmberAfWcType type);
    static const char * EndProductTypeGetString(const EmberAfWcEndProductType endProduct);

    WindowCover();
    WindowCover(EmberAfWcType type, EmberAfWcEndProductType endProductType, uint16_t liftopenLimit, uint16_t liftclosedLimit, uint16_t tiltopenLimit, uint16_t tiltclosedLimit);

    // MANDATORY Attributes -- Setter/Getter Internal Variables equivalent

    // Attribute: Id  0 Type
    void TypeSet(EmberAfWcType type);
    EmberAfWcType TypeGet(void);

    // Attribute: Id  7 ConfigStatus
    void ConfigStatusSet(ConfigStatus_t status);
    ConfigStatus_t * ConfigStatusGet(void);

    // Attribute: Id 10 OperationalStatus
    void OperationalStatusSet(OperationalStatus_t status);
    OperationalStatus_t OperationalStatusGet(void);

    // Attribute: Id 13 EndProductType
    void EndProductTypeSet(EmberAfWcEndProductType endProduct);
    EmberAfWcEndProductType EndProductTypeGet(void);

    // Attribute: Id 24 Mode
    void ModeSet(Mode_t mode);
    Mode_t ModeGet(void);

    // OPTIONAL Attributes -- Setter/Getter Internal Variables equivalent
    // Attribute: Id 27 SafetyStatus (Optional)
    void SafetyStatusSet(uint16_t status);
    uint16_t SafetyStatusGet(void);

    void TypeCycle();

    void InitCommon(void);
    void SynchronizeCluster();

    // Lift
    uint16_t LiftOpenLimitGet(void);
    uint16_t LiftClosedLimitGet(void);

    uint16_t LiftCurrentValueGet(void);
    posPercent100ths_t LiftCurrentPercent100thsGet(void);
    posPercent100ths_t LiftTargetPercent100thsGet(void);

    void LiftStepTowardOpen();
    void LiftStepTowardClose();
    void LiftGoToValue(uint16_t lift);
    void LiftGoToPercent100ths(uint16_t percent100ths);

    // Tilt
    uint16_t TiltOpenLimitGet(void);
    uint16_t TiltClosedLimitGet(void);

    uint16_t TiltCurrentValueGet(void);
    posPercent100ths_t TiltCurrentPercent100thsGet(void);
    posPercent100ths_t TiltTargetPercent100thsGet(void);

    void TiltStepTowardOpen();
    void TiltStepTowardClose();
    void TiltGoToValue(uint16_t tilt);
    void TiltGoToPercent100ths(uint16_t percent100ths);

    posPercent100ths_t PositionToPercent100ths(CoverActuator_t * pActuator, uint16_t position);
    uint16_t Percent100thsToPosition(CoverActuator_t * pActuator, posPercent100ths_t percent100ths);

    void ActuatorStepTowardOpen(CoverActuator_t * pActuator);
    void ActuatorStepTowardClose(CoverActuator_t * pActuator);
    void ActuatorSetPosition(CoverActuator_t * pActuator, uint16_t value);

    void PrintActuators(void);
    void PrintStatus(void);

    // Commands
    void Open();
    void Close();
    void Stop();
    // Other
    void SelectedActuatorSet(bool mode);
    bool SelectedActuatorGet(void);
    CoverActuator_t * ActuatorGetLift(void);
    CoverActuator_t * ActuatorGetTilt(void);
    void ActuatorGoToValue(CoverActuator_t * pAct, uint16_t value);
    void ActuatorGoToPercent100ths(CoverActuator_t * pAct, uint16_t accuratePercent100ths);
    void ToggleActuator();
    void StepUpOrOpen();
    void StepDownOrClose();
    bool IsOpen(void);
    bool IsClosed(void);
    bool IsMoving(void);
    // Events
    void PostEvent(AppEvent::EventType event);

private:
    static void LiftTimerCallback(AppTimer & timer, void * context);
    static void TiltTimerCallback(AppTimer & timer, void * context);
    static void ActuatorTimerCallback(AppTimer & timer, WindowCover * pCover, CoverActuator_t * pAct);

    void PrintActuator(const char * pName, CoverActuator_t * pAct);

    // Attribute: Id  0 Type
    EmberAfWcType mType                     = EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT;

    // Attribute: Id  7 ConfigStatus
    ConfigStatus_t mConfigStatus = { .operational = 1, .online = 1, .liftIsReversed = 0, .liftIsPA = 1, .tiltIsPA = 1, .liftIsEncoderControlled = 1, .tiltIsEncoderControlled = 1 };

    // Attribute: Id 10 OperationalStatus
    OperationalStatus_t mOperationalStatus = { .global = Stall, .lift = Stall, .tilt = Stall };

    // Attribute: Id 13 EndProductType
    EmberAfWcEndProductType mEndProductType = EMBER_ZCL_WC_END_PRODUCT_TYPE_INTERIOR_BLIND;

    // Attribute: Id 24 Mode
    Mode_t mMode = { .motorDirReversed = 0, .calibrationMode = 1, .maintenanceMode = 1, .ledDisplay = 1 };

    // Attribute: Id 27 SafetyStatus (Optional)
    uint16_t mSafetyStatus      = 0x00; // 0 is no issues;

    bool mSelectedActuator;

    // TODO: Store feature map once it is supported

    CoverActuator_t mLift = { Stall, LIFT_OPEN_LIMIT, LIFT_CLOSED_LIMIT, UINT16_MAX, UINT16_MAX, LIFT_DELTA, 0, 0, AppEvent::EventType::CoverLiftChange };
    CoverActuator_t mTilt = { Stall, TILT_OPEN_LIMIT, TILT_CLOSED_LIMIT, UINT16_MAX, UINT16_MAX, TILT_DELTA, 0, 0, AppEvent::EventType::CoverTiltChange };
};
