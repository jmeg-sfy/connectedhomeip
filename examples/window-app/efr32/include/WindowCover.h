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

#define LIFT_DELTA (LCD_COVER_SIZE / 10)
#define TILT_DELTA 1

class WindowCover
{
public:



    enum class CoverType
    {
        Rollershade                  = 0, // Lift
        Rollershade_2_motor          = 1, // Lift
        Rollershade_exterior         = 2, // Lift
        Rollershade_exterior_2_motor = 3, // Lift
        Drapery                      = 4, // Lift
        Awning                       = 5, // Lift
        Shutter                      = 6, // Tilt
        Tilt_blind                   = 7, // Tilt
        Tilt_Lift_blind              = 8, // Lift, Tilt
        Projector_screen             = 9  // Tilt
    };

    enum class CoverAction
    {
        None = 0,
        MovingDownOrClose,
        MovingUpOrOpen,
    };

    typedef struct CoverActuator
    {
        /* data */
        CoverAction action;
        uint16_t openLimit;
        uint16_t closedLimit;
        uint16_t currentPosition;
        uint16_t targetPosition;
        uint16_t stepDelta;
        AppEvent::EventType eventOpening;
        AppEvent::EventType eventClosing;
        AppTimer timer;
    } CoverActuator_t;

    static const char * TypeString(const CoverType type);

    WindowCover();
    WindowCover(CoverType type, uint16_t liftOpenLimit, uint16_t liftClosedLimit, uint16_t tiltOpenLimit, uint16_t tiltClosedLimit);

    // Config Status
    void ConfigStatusSet(uint8_t status);
    uint8_t ConfigStatusGet(void);

    // Operational Status
    void OperationalStatusSet(uint8_t status);
    uint8_t OperationalStatusGet(void);

    // Config Status
    void SafetyStatusSet(uint16_t status);
    uint16_t SafetyStatusGet(void);

    // Type
    void TypeSet(CoverType type);
    void TypeCycle();
    CoverType TypeGet(void);
    // Lift
    uint16_t LiftOpenLimitGet(void);
    uint16_t LiftClosedLimitGet(void);
    void LiftSet(uint16_t lift);
    uint16_t LiftGet(void);
    // void LiftPercentSet(uint8_t percentage);
    // uint8_t LiftPercentGet(void);
    void LiftUpOrOpen();
    void LiftDownOrClose();
    void LiftGoToValue(uint16_t lift);
    void LiftGoToPercent100ths(uint16_t percent100ths);

    // Tilt
    uint16_t TiltOpenLimitGet(void);
    uint16_t TiltClosedLimitGet(void);
    void TiltSet(uint16_t tilt);
    uint16_t TiltGet(void);
    // void TiltPercentSet(uint8_t percentage);
    // uint8_t TiltPercentGet(void);
    void TiltUpOrOpen();
    void TiltDownOrClose();
    void TiltGoToValue(uint16_t tilt);
    void TiltGoToPercent100ths(uint16_t percent100ths);

    uint16_t PositionToPercent100ths(CoverActuator_t * pActuator, uint16_t position);
    uint16_t Percent100thsToPosition(CoverActuator_t * pActuator, uint16_t percent100ths);

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
    void ActuatorSet(bool mode);
    bool ActuatorGet(void);
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

    uint8_t  mConfigStatus      = 0x03; // bit0: Operational, bit1: Online;
    uint8_t  mOperationalStatus = 0x00; // 0 is no movement;
    uint16_t mSafetyStatus      = 0x00; // 0 is no issues;
    CoverType mType            = CoverType::Tilt_Lift_blind;
    bool mActuator;

    CoverActuator_t mLift = { CoverAction::None, LIFT_OPEN_LIMIT, LIFT_CLOSED_LIMIT, UINT16_MAX, UINT16_MAX, LIFT_DELTA, AppEvent::EventType::CoverLiftUpOrOpen, AppEvent::EventType::CoverLiftDownOrClose };
    CoverActuator_t mTilt = { CoverAction::None, TILT_OPEN_LIMIT, TILT_CLOSED_LIMIT, UINT16_MAX, UINT16_MAX, TILT_DELTA, AppEvent::EventType::CoverTiltUpOrOpen, AppEvent::EventType::CoverTiltDownOrClose };
};
