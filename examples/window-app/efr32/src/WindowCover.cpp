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

#define LIFT_DELTA (LCD_COVER_SIZE / 10)
#define TILT_DELTA 1
#define TIMER_DELAY_MS 500

WindowCover::WindowCover()
{
    mLift.timer.Init(this, "Timer:lift", TIMER_DELAY_MS, LiftTimerCallback);
    mTilt.timer.Init(this, "Timer:tilt", TIMER_DELAY_MS, TiltTimerCallback);
}

WindowCover::WindowCover(CoverType type, uint16_t liftopenLimit, uint16_t liftclosedLimit, uint16_t tiltopenLimit, uint16_t tiltclosedLimit) :
    mType(type)
{
    mLift.openLimit = liftopenLimit, mLift.closedLimit = liftclosedLimit;
    mTilt.openLimit = tiltopenLimit, mTilt.closedLimit = tiltclosedLimit;
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

void WindowCover::LiftSet(uint16_t lift)
{
    if (lift > mLift.closedLimit)
    {
        lift = mLift.closedLimit;
    }
    else if (lift < mLift.openLimit)
    {
        lift = mLift.openLimit;
    }
    if (lift != mLift.currentPosition)
    {
        AppEvent::EventType event = lift > mLift.currentPosition ? AppEvent::EventType::CoverLiftUpOrOpen : AppEvent::EventType::CoverLiftDownOrClose;
        mLift.currentPosition                     = lift;
        PostEvent(event);
    }
}

uint16_t WindowCover::LiftGet()
{
    return mLift.currentPosition;
}

void WindowCover::PrintActuator(const char * pName, WindowCover::CoverActuator_t * pAct)
{
    if (!pName || !pAct) return;

    uint16_t currentAccuratePercentage = PositionToAccuratePercentage(pAct, pAct->currentPosition);
    uint16_t  targetAccuratePercentage = PositionToAccuratePercentage(pAct, pAct->targetPosition);
    uint8_t currentPercentage = currentAccuratePercentage / 100;
    uint8_t  targetPercentage =  targetAccuratePercentage / 100;

    EFR32_LOG("%10s Abs:[%4u - %4u] Current %4u, Target %4u\n", pName,
        pAct->openLimit, pAct->closedLimit, pAct->currentPosition, pAct->targetPosition);
    EFR32_LOG("%10s Rel:[0.00% - 100.00%] Current %3u.%02u, Target %3u.%02u\n", pName,
        currentPercentage, (currentAccuratePercentage - (currentPercentage * 100)),
         targetPercentage, ( targetAccuratePercentage - ( targetPercentage * 100)));

}

void WindowCover::PrintActuators(void)
{
    PrintActuator("Lift", &mLift);
    PrintActuator("Tilt", &mTilt);
}

// void WindowCover::LiftPercentSet(uint8_t percentage)
// {
//     LiftSet(PercentToLift(percentage));
// }

// uint8_t WindowCover::LiftPercentGet()
// {
//     return LiftToPercent(mLift.currentPosition);
// }

void WindowCover::LiftUp()
{
    LiftSet(mLift.currentPosition + LIFT_DELTA);
}

void WindowCover::LiftDown()
{
    if (mLift.currentPosition >= LIFT_DELTA)
    {
        LiftSet(mLift.currentPosition - LIFT_DELTA);
    }
    else
    {
        LiftSet(mLift.openLimit);
    }
}

void WindowCover::LiftGoToValue(uint16_t lift)
{
    if (lift > mLift.closedLimit)
    {
        lift = mLift.closedLimit;
    }
    else if (lift < mLift.openLimit)
    {
        lift = mLift.openLimit;
    }
    mLift.action = lift > mLift.currentPosition ? CoverAction::LiftUp : CoverAction::LiftDown;
    mLift.targetPosition = lift;
    mLift.timer.Start();
    PostEvent(AppEvent::EventType::CoverStart);
}

void WindowCover::LiftGoToAccuratePercentage(uint16_t accuratePercentage)
{
    LiftGoToValue(AccuratePercentageToPosition(&mLift, accuratePercentage));
}

uint16_t WindowCover::TiltOpenLimitGet()
{
    return mTilt.openLimit;
}

uint16_t WindowCover::TiltClosedLimitGet()
{
    return mTilt.closedLimit;
}

void WindowCover::TiltSet(uint16_t tilt)
{
    if (tilt > mTilt.closedLimit)
    {
        tilt = mTilt.closedLimit;
    }
    else if (tilt < mTilt.openLimit)
    {
        tilt = mTilt.openLimit;
    }
    if (tilt != mTilt.currentPosition)
    {
        AppEvent::EventType event = tilt > mTilt.currentPosition ? AppEvent::EventType::CoverTiltUpOrOpen : AppEvent::EventType::CoverTiltDownOrClose;
        mTilt.currentPosition                     = tilt;
        PostEvent(event);
    }
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

void WindowCover::TiltUp()
{
    TiltSet(mTilt.currentPosition + TILT_DELTA);
}

void WindowCover::TiltDown()
{
    if (mTilt.currentPosition >= TILT_DELTA)
    {
        TiltSet(mTilt.currentPosition - TILT_DELTA);
    }
    else
    {
        TiltSet(mTilt.openLimit);
    }
}

void WindowCover::TiltGoToValue(uint16_t tilt)
{
    if (tilt > mTilt.closedLimit)
    {
        tilt = mTilt.closedLimit;
    }
    else if (tilt < mTilt.openLimit)
    {
        tilt = mTilt.openLimit;
    }
    mTilt.action = tilt > mTilt.currentPosition ? CoverAction::TiltUp : CoverAction::TiltDown;
    mTilt.targetPosition = tilt;
    mTilt.timer.Start();
    PostEvent(AppEvent::EventType::CoverStart);
}

void WindowCover::TiltGoToAccuratePercentage(uint16_t accuratePercentage)
{
    TiltGoToValue(AccuratePercentageToPosition(&mTilt, accuratePercentage));
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
    mLift.timer.Stop();
    mTilt.timer.Stop();
    PostEvent(AppEvent::EventType::CoverStop);
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

void WindowCover::ToggleActuator()
{
    mActuator = !mActuator;
    PostEvent(AppEvent::EventType::CoverActuatorChange);
}

void WindowCover::StepUpOrOpen()
{
    if (mActuator)
    {
        TiltUp();
    }
    else
    {
        LiftUp();
    }
}

void WindowCover::StepDownOrClose()
{
    if (mActuator)
    {
        TiltDown();
    }
    else
    {
        LiftDown();
    }
}

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

void WindowCover::LiftTimerCallback(AppTimer & timer, void * context)
{
    WindowCover * cover = (WindowCover *) context;
    switch (cover->mLift.action)
    {
    case CoverAction::LiftDown:
        if (cover->mLift.currentPosition > cover->mLift.targetPosition)
        {
            cover->LiftDown();
            timer.Start();
        }
        break;
    case CoverAction::LiftUp:
        if (cover->mLift.currentPosition < cover->mLift.targetPosition)
        {
            cover->LiftUp();
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
        cover->mLift.action = CoverAction::None;
        cover->PostEvent(AppEvent::EventType::CoverStop);
    }
}

void WindowCover::TiltTimerCallback(AppTimer & timer, void * context)
{
    WindowCover * cover = (WindowCover *) context;
    switch (cover->mTilt.action)
    {
    case CoverAction::TiltDown:
        if (cover->mTilt.currentPosition > cover->mTilt.targetPosition)
        {
            cover->TiltDown();
            timer.Start();
        }
        break;
    case CoverAction::TiltUp:
        if (cover->mTilt.currentPosition < cover->mTilt.targetPosition)
        {
            cover->TiltUp();
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
        cover->mTilt.action = CoverAction::None;
        cover->PostEvent(AppEvent::EventType::CoverStop);
    }
}

void WindowCover::PostEvent(AppEvent::EventType event)
{
    AppTask::Instance().PostEvent(AppEvent(event, this));
}
