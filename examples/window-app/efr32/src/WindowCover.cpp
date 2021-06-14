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

WindowCover::WindowCover(EmberAfWcType type, EmberAfWcEndProductType endProductType, uint16_t liftopenLimit, uint16_t liftclosedLimit, uint16_t tiltopenLimit, uint16_t tiltclosedLimit) :
    mType(type), mEndProductType(endProductType)
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

void WindowCover::TypeSet(EmberAfWcType type)
{
    if (type != mType)
    {
        mType = type;
        switch (mType)
        {
        // Lift only
        case EMBER_ZCL_WC_TYPE_ROLLERSHADE:
        case EMBER_ZCL_WC_TYPE_ROLLERSHADE2_MOTOR:
        case EMBER_ZCL_WC_TYPE_ROLLERSHADE_EXTERIOR2_MOTOR:
        case EMBER_ZCL_WC_TYPE_DRAPERY:
        case EMBER_ZCL_WC_TYPE_AWNING:
            ActuatorSet(false);
            break;
        // Tilt only
        case EMBER_ZCL_WC_TYPE_SHUTTER:
        case EMBER_ZCL_WC_TYPE_TILT_BLIND_TILT_ONLY:
        case EMBER_ZCL_WC_TYPE_PROJECTOR_SCREEN:
            ActuatorSet(true);
            break;
        // Lift & Tilt
        case EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT:
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
    case EMBER_ZCL_WC_TYPE_ROLLERSHADE:
        TypeSet(EMBER_ZCL_WC_TYPE_DRAPERY);
        break;
    case EMBER_ZCL_WC_TYPE_DRAPERY:
        TypeSet(EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT);
        break;
    case EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT:
        TypeSet(EMBER_ZCL_WC_TYPE_ROLLERSHADE);
        break;
    default:
        TypeSet(EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT);
        break;
    }
}

EmberAfWcType WindowCover::TypeGet()
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

    uint16_t currentPercent100ths = PositionToPercent100ths(pAct, pAct->currentPosition);
    uint16_t  targetPercent100ths = PositionToPercent100ths(pAct, pAct->targetPosition);
    uint8_t currentPercentage = currentPercent100ths / 100;
    uint8_t  targetPercentage =  targetPercent100ths / 100;

    EFR32_LOG("%10s Abs:[ %7u - %7u ] Current %7u, Target %7u", pName,
        pAct->openLimit, pAct->closedLimit, pAct->currentPosition, pAct->targetPosition);
    EFR32_LOG("%10s Rel:[   0.00%% - 100.00%% ] Current %3u.%02u%%, Target %3u.%02u%%", pName,
        currentPercentage, (currentPercent100ths - (currentPercentage * 100)),
         targetPercentage, ( targetPercent100ths - ( targetPercentage * 100)));

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
        ActuatorSetPosition(pAct, pAct->openLimit);//Percent100ths attribute will be set to 0%.
    }
}

void WindowCover::ActuatorStepTowardClose(CoverActuator_t * pAct)
{
    if (!pAct) return;

    //EFR32_LOG("ActuatorStepTowardClose %u %u %u", pAct->currentPosition, (pAct->stepDelta - pAct->closedLimit),pAct->closedLimit );
    if (pAct->currentPosition <= (pAct->closedLimit - pAct->stepDelta)) {
        ActuatorSetPosition(pAct, pAct->currentPosition + pAct->stepDelta);
    } else {
        ActuatorSetPosition(pAct, pAct->closedLimit);//Percent100ths attribute will be set to 100%.
    }
}




void WindowCover::LiftGoToPercent100ths(uint16_t percent100ths)
{
    ActuatorGoToPercent100ths(&mLift, percent100ths);
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

void WindowCover::ActuatorGoToPercent100ths(CoverActuator_t * pAct, uint16_t percent100ths)
{
    ActuatorGoToValue(pAct, Percent100thsToPosition(pAct, percent100ths));
}

void WindowCover::TiltGoToPercent100ths(uint16_t percent100ths)
{
    ActuatorGoToPercent100ths(&mTilt, percent100ths);
}

void WindowCover::TiltGoToValue(uint16_t value)
{
    ActuatorGoToValue(&mTilt, value);
}

uint16_t WindowCover::PositionToPercent100ths(CoverActuator_t * pActuator, uint16_t position)
{
    if (!pActuator) return UINT16_MAX;

    uint16_t range = pActuator->closedLimit - pActuator->openLimit;

    if (range > 0) {
        return (uint16_t) (10000 * (position - pActuator->openLimit) / range);
    }

    return UINT16_MAX;
}

uint16_t WindowCover::Percent100thsToPosition(CoverActuator_t * pActuator, uint16_t percent100ths)
{
    if (!pActuator) return UINT16_MAX;

    uint16_t range = pActuator->closedLimit - pActuator->openLimit;
    return pActuator->openLimit + (range * percent100ths) / 10000;
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
    case EMBER_ZCL_WC_TYPE_ROLLERSHADE:
    case EMBER_ZCL_WC_TYPE_ROLLERSHADE2_MOTOR:
    case EMBER_ZCL_WC_TYPE_ROLLERSHADE_EXTERIOR2_MOTOR:
    case EMBER_ZCL_WC_TYPE_DRAPERY:
    case EMBER_ZCL_WC_TYPE_AWNING:
        return mLift.currentPosition <= mLift.openLimit;
    // Tilt only
    case EMBER_ZCL_WC_TYPE_SHUTTER:
    case EMBER_ZCL_WC_TYPE_TILT_BLIND_TILT_ONLY:
    case EMBER_ZCL_WC_TYPE_PROJECTOR_SCREEN:
        return mTilt.currentPosition <= mTilt.openLimit;
    // Lift & Tilt
    case EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT:
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
    case EMBER_ZCL_WC_TYPE_ROLLERSHADE:
    case EMBER_ZCL_WC_TYPE_ROLLERSHADE2_MOTOR:
    case EMBER_ZCL_WC_TYPE_ROLLERSHADE_EXTERIOR2_MOTOR:
    case EMBER_ZCL_WC_TYPE_DRAPERY:
    case EMBER_ZCL_WC_TYPE_AWNING:
        return mLift.currentPosition >= mLift.closedLimit;
    // Tilt only
    case EMBER_ZCL_WC_TYPE_SHUTTER:
    case EMBER_ZCL_WC_TYPE_TILT_BLIND_TILT_ONLY:
    case EMBER_ZCL_WC_TYPE_PROJECTOR_SCREEN:
        return mTilt.currentPosition >= mTilt.closedLimit;
    // Lift & Tilt
    case EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT:
    default:
        return mLift.currentPosition >= mLift.closedLimit && mTilt.currentPosition >= mTilt.closedLimit;
    }
    return false;
}

bool WindowCover::IsMoving(void)
{
    return mLift.timer.IsActive() || mTilt.timer.IsActive();
}

const char * WindowCover::TypeGetString(const EmberAfWcType type)
{
    switch (type)
    {
    case EMBER_ZCL_WC_TYPE_ROLLERSHADE:
        return "Rollershade";
    case EMBER_ZCL_WC_TYPE_ROLLERSHADE2_MOTOR:
        return "Rollershade_2_motor";
    case EMBER_ZCL_WC_TYPE_ROLLERSHADE_EXTERIOR2_MOTOR:
        return "Rollershade_exterior_2_motor";
    case EMBER_ZCL_WC_TYPE_DRAPERY:
        return "Drapery";
    case EMBER_ZCL_WC_TYPE_AWNING:
        return "Awning";
    case EMBER_ZCL_WC_TYPE_SHUTTER:
        return "Shutter";
    case EMBER_ZCL_WC_TYPE_TILT_BLIND_TILT_ONLY:
        return "Tilt_blind";
    case EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT:
        return "Tilt_Lift_blind";
    case EMBER_ZCL_WC_TYPE_PROJECTOR_SCREEN:
        return "Projector_screen";
    default:
        return "?";
    }
}

const char * WindowCover::EndProductTypeGetString(const EmberAfWcEndProductType endProduct)
{
    switch (endProduct)
    {
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_ROLLER_SHADE                 : return "ROLLER_SHADE";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_ROMAN_SHADE                  : return "ROMAN_SHADE";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_BALLOON_SHADE                : return "BALLOON_SHADE";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_WOVEN_WOOD                   : return "WOVEN_WOOD";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_PLEATED_SHADE                : return "PLEATED_SHADE";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_CELLULAR_SHADE               : return "CELLULAR_SHADE";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_LAYERED_SHADE                : return "LAYERED_SHADE";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_LAYERED_SHADE2_D             : return "LAYERED_SHADE2_D";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_SHEER_SHADE                  : return "SHEER_SHADE";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_TILT_ONLY_INTERIOR_BLIND     : return "TILT_ONLY_INTERIOR_BLIND";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_INTERIOR_BLIND               : return "INTERIOR_BLIND";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_VERTICAL_BLIND_STRIP_CURTAIN : return "VERTICAL_BLIND_STRIP_CURTAIN";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_INTERIOR_VENETIAN_BLIND      : return "INTERIOR_VENETIAN_BLIND";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_EXTERIOR_VENETIAN_BLIND      : return "EXTERIOR_VENETIAN_BLIND";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_LATERAL_LEFT_CURTAIN         : return "LATERAL_LEFT_CURTAIN";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_LATERAL_RIGHT_CURTAIN        : return "LATERAL_RIGHT_CURTAIN";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_CENTRAL_CURTAIN              : return "CENTRAL_CURTAIN";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_ROLLER_SHUTTER               : return "ROLLER_SHUTTER";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_EXTERIOR_VERTICAL_SCREEN     : return "EXTERIOR_VERTICAL_SCREEN";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_AWNING_TERRACE_PATIO         : return "AWNING_TERRACE_PATIO";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_AWNING_VERTICAL_SCREEN       : return "AWNING_VERTICAL_SCREEN";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_TILT_ONLY_PERGOLA            : return "TILT_ONLY_PERGOLA";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_SWINGING_SHUTTER             : return "SWINGING_SHUTTER";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_SLIDING_SHUTTER              : return "SLIDING_SHUTTER";
    case EMBER_ZCL_WC_END_PRODUCT_TYPE_UNKNOWN                      : return "UNKNOWN";
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
