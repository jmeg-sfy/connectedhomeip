/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

#include <AppTask.h>
#include <WindowCover.h>


#define TIMER_DELAY_MS 500

WindowCover::WindowCover()
{
    mLift.timer.Init(this, "Timer:lift", TIMER_DELAY_MS, LiftTimerCallback);
    mTilt.timer.Init(this, "Timer:tilt", TIMER_DELAY_MS, TiltTimerCallback);

    /* Target Position is initial state reflect the current position */
    mLift.currentPosition = mLift.openLimit;
    mTilt.currentPosition = mTilt.openLimit;

    mLift.targetPosition = mLift.currentPosition;
    mTilt.targetPosition = mTilt.currentPosition;
}

WindowCover::WindowCover(CoverType type, uint16_t liftopenLimit, uint16_t liftclosedLimit, uint16_t tiltopenLimit, uint16_t tiltclosedLimit) :
    mType(type)
{
    mLift.openLimit = liftopenLimit, mLift.closedLimit = liftclosedLimit;
    mTilt.openLimit = tiltopenLimit, mTilt.closedLimit = tiltclosedLimit;


    /* Target Position is initial state reflect the current position */
    mLift.currentPosition = liftopenLimit,
    mTilt.currentPosition = tiltopenLimit,

    mLift.targetPosition = mLift.currentPosition;
    mTilt.targetPosition = mTilt.currentPosition;
}

void WindowCover::ConfigStatusSet(uint8_t status)
{
    if (status != mConfigStatus)
    {
        mConfigStatus = status;
        PostEvent(AppEvent::EventType::CoverConfigStatusChange);
    }
}

uint8_t WindowCover::ConfigStatusGet(void)
{
    return mConfigStatus;
}

void WindowCover::OperationalStatusSet(uint8_t status)
{
    if (status != mOperationalStatus)
    {
        mOperationalStatus = status;
        PostEvent(AppEvent::EventType::CoverOperationalStatusChange);
    }
}

uint8_t WindowCover::OperationalStatusGet(void)
{
    return mOperationalStatus;
}

void WindowCover::SafetyStatusSet(uint16_t status)
{
    if (status != mSafetyStatus)
    {
        mSafetyStatus = status;
        PostEvent(AppEvent::EventType::CoverSafetyStatusChange);
    }
}

uint16_t WindowCover::SafetyStatusGet(void)
{
    return mSafetyStatus;
}

void WindowCover::TypeSet(CoverType type)
{
    if (type != mType)
    {
        mType = type;
        switch (mType)
        {
        // Lift only
        case CoverType::Rollershade:
        case CoverType::Rollershade_2_motor:
        case CoverType::Rollershade_exterior_2_motor:
        case CoverType::Drapery:
        case CoverType::Awning:
            ActuatorSet(false);
            break;
        // Tilt only
        case CoverType::Shutter:
        case CoverType::Tilt_blind:
        case CoverType::Projector_screen:
            ActuatorSet(true);
            break;
        // Lift & Tilt
        case CoverType::Tilt_Lift_blind:
        default:
            break;
        }
        PostEvent(AppEvent::EventType::CoverTypeChange);
    }
}

void WindowCover::TypeCycle()
{
    switch (mType)
    {
    case CoverType::Rollershade:
        TypeSet(CoverType::Drapery);
        break;
    case CoverType::Drapery:
        TypeSet(CoverType::Tilt_Lift_blind);
        break;
    case CoverType::Tilt_Lift_blind:
        TypeSet(CoverType::Rollershade);
        break;
    default:
        TypeSet(CoverType::Tilt_Lift_blind);
        break;
    }
}

WindowCover::CoverType WindowCover::TypeGet()
{
    return mType;
}

uint16_t WindowCover::LiftOpenLimitGet()
{
    return mLift.openLimit;
}

uint16_t WindowCover::LiftClosedLimitGet()
{
    return mLift.closedLimit;
}





void WindowCover::PrintActuator(const char * pName, WindowCover::CoverActuator_t * pAct)
{
    if (!pName || !pAct) return;

    uint16_t currentAccuratePercentage = PositionToAccuratePercentage(pAct, pAct->currentPosition);
    uint16_t  targetAccuratePercentage = PositionToAccuratePercentage(pAct, pAct->targetPosition);
    uint8_t currentPercentage = currentAccuratePercentage / 100;
    uint8_t  targetPercentage =  targetAccuratePercentage / 100;

    EFR32_LOG("%10s Abs:[ %7u - %7u ] Current %7u, Target %7u", pName,
        pAct->openLimit, pAct->closedLimit, pAct->currentPosition, pAct->targetPosition);
    EFR32_LOG("%10s Rel:[   0.00%% - 100.00%% ] Current %3u.%02u%%, Target %3u.%02u%%", pName,
        currentPercentage, (currentAccuratePercentage - (currentPercentage * 100)),
         targetPercentage, ( targetAccuratePercentage - ( targetPercentage * 100)));

}

void WindowCover::PrintActuators(void)
{
    PrintActuator("Lift", &mLift);
    PrintActuator("Tilt", &mTilt);
    PrintStatus();
}

void WindowCover::PrintStatus(void)
{
    EFR32_LOG("Config: 0x%02X, Operational: 0x%02X, Safety: 0x%04X", mConfigStatus, mOperationalStatus, mSafetyStatus);
}

// void WindowCover::LiftPercentSet(uint8_t percentage)
// {
//     LiftSet(PercentToLift(percentage));
// }

// uint8_t WindowCover::LiftPercentGet()
// {
//     return LiftToPercent(mLift.currentPosition);
// }

void WindowCover::LiftUpOrOpen()    { ActuatorStepTowardOpen(&mLift); }
void WindowCover::TiltUpOrOpen()    { ActuatorStepTowardOpen(&mTilt); }
void WindowCover::LiftDownOrClose() { ActuatorStepTowardClose(&mLift); }
void WindowCover::TiltDownOrClose() { ActuatorStepTowardClose(&mTilt); }
uint16_t WindowCover::LiftGet()
{
    return mLift.currentPosition;
}
void WindowCover::ActuatorStepTowardOpen(CoverActuator_t * pAct)
{
    if (!pAct) return;

    if (pAct->currentPosition >= pAct->stepDelta) {
        ActuatorSetPosition(pAct, pAct->currentPosition - pAct->stepDelta);
    } else {
        ActuatorSetPosition(pAct, pAct->openLimit);//Percentage attribute will be set to 0%.
    }
}

void WindowCover::ActuatorStepTowardClose(CoverActuator_t * pAct)
{
    if (!pAct) return;

    //EFR32_LOG("ActuatorStepTowardClose %u %u %u", pAct->currentPosition, (pAct->stepDelta - pAct->closedLimit),pAct->closedLimit );
    if (pAct->currentPosition <= (pAct->closedLimit - pAct->stepDelta)) {
        ActuatorSetPosition(pAct, pAct->currentPosition + pAct->stepDelta);
    } else {
        ActuatorSetPosition(pAct, pAct->closedLimit);//Percentage attribute will be set to 100%.
    }
}




void WindowCover::LiftGoToAccuratePercentage(uint16_t accuratePercentage)
{
    ActuatorGoToAccuratePercentage(&mLift, accuratePercentage);
}

void WindowCover::LiftGoToValue(uint16_t value)
{
    ActuatorGoToValue(&mLift, value);
}

uint16_t WindowCover::TiltOpenLimitGet()
{
    return mTilt.openLimit;
}

uint16_t WindowCover::TiltClosedLimitGet()
{
    return mTilt.closedLimit;
}



uint16_t WindowCover::TiltGet()
{
    return mTilt.currentPosition;
}

// void WindowCover::TiltPercentSet(uint8_t percentage)
// {
//     TiltSet(PercentToTilt(percentage));
// }

// uint8_t WindowCover::TiltPercentGet()
// {
//     return TiltToPercent(mTilt.currentPosition);
// }


void WindowCover::ActuatorSetPosition(CoverActuator_t * pAct, uint16_t value)
{
    if (!pAct) return;

    if (value > pAct->closedLimit) {
        value = pAct->closedLimit;
    } else if (value < pAct->openLimit) {
        value = pAct->openLimit;
    }

    if (value != pAct->currentPosition)
    {
        AppEvent::EventType event = (value > pAct->currentPosition) ? pAct->eventClosing : pAct->eventOpening;
        pAct->currentPosition = value;

        // Trick here If direct command set Target to go directly to position
        if (!pAct->timer.IsActive()) pAct->targetPosition = pAct->currentPosition;

        PostEvent(event);
    }
}

void WindowCover::ActuatorGoToValue(CoverActuator_t * pAct, uint16_t value)
{
    if (!pAct) return;

    if (value > pAct->closedLimit) {
        value = pAct->closedLimit;
    } else if (value < pAct->openLimit) {
        value = pAct->openLimit;
    }

    if (value != pAct->currentPosition) {
        pAct->action = (value > pAct->currentPosition) ? CoverAction::MovingDownOrClose : CoverAction::MovingUpOrOpen;
        pAct->targetPosition = value;
        pAct->timer.Start();
        PostEvent(AppEvent::EventType::CoverStart);
    } else {
        pAct->targetPosition = pAct->currentPosition;
        pAct->timer.Stop();
        PostEvent(AppEvent::EventType::CoverStop);
    }
}

void WindowCover::ActuatorGoToAccuratePercentage(CoverActuator_t * pAct, uint16_t accuratePercentage)
{
    ActuatorGoToValue(pAct, AccuratePercentageToPosition(pAct, accuratePercentage));
}

void WindowCover::TiltGoToAccuratePercentage(uint16_t accuratePercentage)
{
    ActuatorGoToAccuratePercentage(&mTilt, accuratePercentage);
}

void WindowCover::TiltGoToValue(uint16_t value)
{
    ActuatorGoToValue(&mTilt, value);
}

uint16_t WindowCover::PositionToAccuratePercentage(CoverActuator_t * pActuator, uint16_t position)
{
    if (!pActuator) return UINT16_MAX;

    uint16_t range = pActuator->closedLimit - pActuator->openLimit;

    if (range > 0) {
        return (uint16_t) (10000 * (position - pActuator->openLimit) / range);
    }

    return UINT16_MAX;
}

uint16_t WindowCover::AccuratePercentageToPosition(CoverActuator_t * pActuator, uint16_t accuratePercentage)
{
    if (!pActuator) return UINT16_MAX;

    uint16_t range = pActuator->closedLimit - pActuator->openLimit;
    return pActuator->openLimit + (range * accuratePercentage) / 10000;
}

void WindowCover::Open()
{
    LiftGoToValue(mLift.openLimit);
    TiltGoToValue(mTilt.openLimit);
}

void WindowCover::Close()
{
    LiftGoToValue(mLift.closedLimit);
    TiltGoToValue(mTilt.closedLimit);
}

void WindowCover::Stop()
{
    LiftGoToValue(mLift.currentPosition);
    TiltGoToValue(mTilt.currentPosition);
}

void WindowCover::ActuatorSet(bool mode)
{
    if (mode != mActuator)
    {
        mActuator = mode;
        PostEvent(AppEvent::EventType::CoverActuatorChange);
    }
}

bool WindowCover::ActuatorGet()
{
    return mActuator;
}

WindowCover::CoverActuator_t * WindowCover::ActuatorGetLift(void) { return &mLift; }
WindowCover::CoverActuator_t * WindowCover::ActuatorGetTilt(void) { return &mTilt; }

void WindowCover::ToggleActuator()
{
    mActuator = !mActuator;
    PostEvent(AppEvent::EventType::CoverActuatorChange);
}

void WindowCover::StepUpOrOpen()    { mActuator ? TiltUpOrOpen()    : LiftUpOrOpen();    }
void WindowCover::StepDownOrClose() { mActuator ? TiltDownOrClose() : LiftDownOrClose(); }

bool WindowCover::IsOpen(void)
{
    switch (mType)
    {
    // Lift only
    case CoverType::Rollershade:
    case CoverType::Rollershade_2_motor:
    case CoverType::Rollershade_exterior_2_motor:
    case CoverType::Drapery:
    case CoverType::Awning:
        return mLift.currentPosition <= mLift.openLimit;
    // Tilt only
    case CoverType::Shutter:
    case CoverType::Tilt_blind:
    case CoverType::Projector_screen:
        return mTilt.currentPosition <= mTilt.openLimit;
    // Lift & Tilt
    case CoverType::Tilt_Lift_blind:
    default:
        return mLift.currentPosition <= mLift.openLimit && mTilt.currentPosition <= mTilt.openLimit;
    }
    return false;
}

bool WindowCover::IsClosed(void)
{
    switch (mType)
    {
    // Lift only
    case CoverType::Rollershade:
    case CoverType::Rollershade_2_motor:
    case CoverType::Rollershade_exterior_2_motor:
    case CoverType::Drapery:
    case CoverType::Awning:
        return mLift.currentPosition >= mLift.closedLimit;
    // Tilt only
    case CoverType::Shutter:
    case CoverType::Tilt_blind:
    case CoverType::Projector_screen:
        return mTilt.currentPosition >= mTilt.closedLimit;
    // Lift & Tilt
    case CoverType::Tilt_Lift_blind:
    default:
        return mLift.currentPosition >= mLift.closedLimit && mTilt.currentPosition >= mTilt.closedLimit;
    }
    return false;
}

bool WindowCover::IsMoving(void)
{
    return mLift.timer.IsActive() || mTilt.timer.IsActive();
}

const char * WindowCover::TypeString(const WindowCover::CoverType type)
{
    switch (type)
    {
    case CoverType::Rollershade:
        return "Rollershade";
    case CoverType::Rollershade_2_motor:
        return "Rollershade_2_motor";
    case CoverType::Rollershade_exterior_2_motor:
        return "Rollershade_exterior_2_motor";
    case CoverType::Drapery:
        return "Drapery";
    case CoverType::Awning:
        return "Awning";
    case CoverType::Shutter:
        return "Shutter";
    case CoverType::Tilt_blind:
        return "Tilt_blind";
    case CoverType::Tilt_Lift_blind:
        return "Tilt_Lift_blind";
    case CoverType::Projector_screen:
        return "Projector_screen";
    default:
        return "?";
    }
}

// TIMERs
void WindowCover::LiftTimerCallback(AppTimer & timer, void * context)
{ 
    if (!context) return;

    WindowCover * cover = (WindowCover *) context;
    ActuatorTimerCallback(timer, cover, cover->ActuatorGetLift());
}

void WindowCover::TiltTimerCallback(AppTimer & timer, void * context)
{ 
    if (!context) return;

    WindowCover * cover = (WindowCover *) context;
    ActuatorTimerCallback(timer, cover, cover->ActuatorGetTilt());
}

void WindowCover::ActuatorTimerCallback(AppTimer & timer, WindowCover * pCover, CoverActuator_t * pAct)
{
    if (!pCover || !pAct) return;

    switch (pAct->action)
    {
    case CoverAction::MovingDownOrClose:
        if (pAct->currentPosition < pAct->targetPosition)
        {
            pCover->ActuatorStepTowardClose(pAct);
            timer.Start();
        }
        break;
    case CoverAction::MovingUpOrOpen:
        if (pAct->currentPosition > pAct->targetPosition)
        {
            pCover->ActuatorStepTowardOpen(pAct);
            timer.Start();
        }
        break;
    case CoverAction::None:
    default:
        timer.Stop();
        break;
    }

    if (!timer.IsActive())
    {
        pAct->action = CoverAction::None;
        pCover->PostEvent(AppEvent::EventType::CoverStop);
    }
}

void WindowCover::PostEvent(AppEvent::EventType event)
{
    AppTask::Instance().PostEvent(AppEvent(event, this));
}
