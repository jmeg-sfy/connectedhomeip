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
    mLiftTimer.Init(this, "Timer:lift", TIMER_DELAY_MS, LiftTimerCallback);
    mTiltTimer.Init(this, "Timer:tilt", TIMER_DELAY_MS, TiltTimerCallback);
}

WindowCover::WindowCover(CoverType type, uint16_t liftOpenLimit, uint16_t liftClosedLimit, uint16_t tiltOpenLimit,
                         uint16_t tiltClosedLimit) :
    mType(type),
    mLiftOpenLimit(liftOpenLimit), mLiftClosedLimit(liftClosedLimit), mLiftPosition(liftClosedLimit), mTiltOpenLimit(tiltOpenLimit),
    mTiltClosedLimit(tiltClosedLimit), mTiltPosition(tiltClosedLimit)
{}

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
    return mLiftOpenLimit;
}

uint16_t WindowCover::LiftClosedLimitGet()
{
    return mLiftClosedLimit;
}

void WindowCover::LiftSet(uint16_t lift)
{
    if (lift > mLiftClosedLimit)
    {
        lift = mLiftClosedLimit;
    }
    else if (lift < mLiftOpenLimit)
    {
        lift = mLiftOpenLimit;
    }
    if (lift != mLiftPosition)
    {
        AppEvent::EventType event = lift > mLiftPosition ? AppEvent::EventType::CoverLiftUpOrOpen : AppEvent::EventType::CoverLiftDownOrClose;
        mLiftPosition                     = lift;
        PostEvent(event);
    }
}

uint16_t WindowCover::LiftGet()
{
    return mLiftPosition;
}

void WindowCover::LiftPercentSet(uint8_t percentage)
{
    LiftSet(PercentToLift(percentage));
}

uint8_t WindowCover::LiftPercentGet()
{
    return LiftToPercent(mLiftPosition);
}

void WindowCover::LiftUp()
{
    LiftSet(mLiftPosition + LIFT_DELTA);
}

void WindowCover::LiftDown()
{
    if (mLiftPosition >= LIFT_DELTA)
    {
        LiftSet(mLiftPosition - LIFT_DELTA);
    }
    else
    {
        LiftSet(mLiftOpenLimit);
    }
}

void WindowCover::LiftGoToValue(uint16_t lift)
{
    if (lift > mLiftClosedLimit)
    {
        lift = mLiftClosedLimit;
    }
    else if (lift < mLiftOpenLimit)
    {
        lift = mLiftOpenLimit;
    }
    mLiftAction = lift > mLiftPosition ? CoverAction::LiftUp : CoverAction::LiftDown;
    mLiftTarget = lift;
    mLiftTimer.Start();
    PostEvent(AppEvent::EventType::CoverStart);
}

void WindowCover::LiftGoToAccuratePercentage(uint16_t percentage)
{
    LiftGoToValue(PercentToLift(percentage));
}

uint8_t WindowCover::LiftToPercent(uint16_t lift)
{
    return (uint8_t)(100 * (lift - mLiftOpenLimit) / (mLiftClosedLimit - mLiftOpenLimit));
}

uint16_t WindowCover::PercentToLift(uint8_t liftPercent)
{
    return mLiftOpenLimit + (mLiftClosedLimit - mLiftOpenLimit) * liftPercent / 100;
}

uint16_t WindowCover::TiltOpenLimitGet()
{
    return mTiltOpenLimit;
}

uint16_t WindowCover::TiltClosedLimitGet()
{
    return mTiltClosedLimit;
}

void WindowCover::TiltSet(uint16_t tilt)
{
    if (tilt > mTiltClosedLimit)
    {
        tilt = mTiltClosedLimit;
    }
    else if (tilt < mTiltOpenLimit)
    {
        tilt = mTiltOpenLimit;
    }
    if (tilt != mTiltPosition)
    {
        AppEvent::EventType event = tilt > mTiltPosition ? AppEvent::EventType::CoverTiltUpOrOpen : AppEvent::EventType::CoverTiltDownOrClose;
        mTiltPosition                     = tilt;
        PostEvent(event);
    }
}

uint16_t WindowCover::TiltGet()
{
    return mTiltPosition;
}

void WindowCover::TiltPercentSet(uint8_t percentage)
{
    TiltSet(PercentToTilt(percentage));
}

uint8_t WindowCover::TiltPercentGet()
{
    return TiltToPercent(mTiltPosition);
}

void WindowCover::TiltUp()
{
    TiltSet(mTiltPosition + TILT_DELTA);
}

void WindowCover::TiltDown()
{
    if (mTiltPosition >= TILT_DELTA)
    {
        TiltSet(mTiltPosition - TILT_DELTA);
    }
    else
    {
        TiltSet(mTiltOpenLimit);
    }
}

void WindowCover::TiltGoToValue(uint16_t tilt)
{
    if (tilt > mTiltClosedLimit)
    {
        tilt = mTiltClosedLimit;
    }
    else if (tilt < mTiltOpenLimit)
    {
        tilt = mTiltOpenLimit;
    }
    mTiltAction = tilt > mTiltPosition ? CoverAction::TiltUp : CoverAction::TiltDown;
    mTiltTarget = tilt;
    mTiltTimer.Start();
    PostEvent(AppEvent::EventType::CoverStart);
}

void WindowCover::TiltGoToAccuratePercentage(uint16_t percentage)
{
    TiltGoToValue(PercentToTilt(percentage));
}

uint8_t WindowCover::TiltToPercent(uint16_t tilt)
{
    return (uint8_t)(100 * (tilt - mTiltOpenLimit) / (mTiltClosedLimit - mTiltOpenLimit));
}

uint16_t WindowCover::PercentToTilt(uint8_t tiltPercent)
{
    return mTiltOpenLimit + (mTiltClosedLimit - mTiltOpenLimit) * tiltPercent / 100;
}

void WindowCover::Open()
{
    LiftGoToValue(mLiftOpenLimit);
    TiltGoToValue(mTiltOpenLimit);
}

void WindowCover::Close()
{
    LiftGoToValue(mLiftClosedLimit);
    TiltGoToValue(mTiltClosedLimit);
}

void WindowCover::Stop()
{
    mLiftTimer.Stop();
    mTiltTimer.Stop();
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
        return mLiftPosition <= mLiftOpenLimit;
    // Tilt only
    case CoverType::Shutter:
    case CoverType::Tilt_blind:
    case CoverType::Projector_screen:
        return mTiltPosition <= mTiltOpenLimit;
    // Lift & Tilt
    case CoverType::Tilt_Lift_blind:
    default:
        return mLiftPosition <= mLiftOpenLimit && mTiltPosition <= mTiltOpenLimit;
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
        return mLiftPosition >= mLiftClosedLimit;
    // Tilt only
    case CoverType::Shutter:
    case CoverType::Tilt_blind:
    case CoverType::Projector_screen:
        return mTiltPosition >= mTiltClosedLimit;
    // Lift & Tilt
    case CoverType::Tilt_Lift_blind:
    default:
        return mLiftPosition >= mLiftClosedLimit && mTiltPosition >= mTiltClosedLimit;
    }
    return false;
}

bool WindowCover::IsMoving(void)
{
    return mLiftTimer.IsActive() || mTiltTimer.IsActive();
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
    switch (cover->mLiftAction)
    {
    case CoverAction::LiftDown:
        if (cover->mLiftPosition > cover->mLiftTarget)
        {
            cover->LiftDown();
            timer.Start();
        }
        break;
    case CoverAction::LiftUp:
        if (cover->mLiftPosition < cover->mLiftTarget)
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
        cover->mLiftAction = CoverAction::None;
        cover->PostEvent(AppEvent::EventType::CoverStop);
    }
}

void WindowCover::TiltTimerCallback(AppTimer & timer, void * context)
{
    WindowCover * cover = (WindowCover *) context;
    switch (cover->mTiltAction)
    {
    case CoverAction::TiltDown:
        if (cover->mTiltPosition > cover->mTiltTarget)
        {
            cover->TiltDown();
            timer.Start();
        }
        break;
    case CoverAction::TiltUp:
        if (cover->mTiltPosition < cover->mTiltTarget)
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
        cover->mTiltAction = CoverAction::None;
        cover->PostEvent(AppEvent::EventType::CoverStop);
    }
}

void WindowCover::PostEvent(AppEvent::EventType event)
{
    AppTask::Instance().PostEvent(AppEvent(event, this));
}
