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


inline bool operator!=(const OperationalStatus_t& lhs, const OperationalStatus_t& rhs){ return !memcmp(&lhs, &rhs, sizeof(OperationalStatus_t)) == 0; }
inline bool operator!=(const ConfigStatus_t& lhs, const ConfigStatus_t& rhs){ return !memcmp(&lhs, &rhs, sizeof(ConfigStatus_t)) == 0; }
inline bool operator!=(const Mode_t& lhs, const Mode_t& rhs){ return !memcmp(&lhs, &rhs, sizeof(Mode_t)) == 0; }

// Non-Fixed Status ConfigStatus
void WindowCover::ConfigStatusSet(ConfigStatus_t status)
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

// Non-Fixed Status Mode
void WindowCover::ModeSet(Mode_t mode)
{
    if (mode != mMode)
    {
        mMode = mode;
        PostEvent(AppEvent::EventType::CoverModeChange);
    }
}

Mode_t WindowCover::ModeGet(void)
{
    return mMode;
}


// Reported State OperationalStatus
void WindowCover::OperationalStatusSet(OperationalStatus_t status)
{
    if (status != mOperationalStatus)
    {
        mOperationalStatus = status;
        PostEvent(AppEvent::EventType::CoverOperationalStatusChange);
    }
}

OperationalStatus_t WindowCover::OperationalStatusGet(void)
{
    return mOperationalStatus;
}
// Reported State SafetyStatus
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

// Product-Fixed Type
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
            SelectedActuatorSet(SELECT_LIFT);
            break;
        // Tilt only
        case EMBER_ZCL_WC_TYPE_SHUTTER:
        case EMBER_ZCL_WC_TYPE_TILT_BLIND_TILT_ONLY:
        case EMBER_ZCL_WC_TYPE_PROJECTOR_SCREEN:
            SelectedActuatorSet(SELECT_TILT);
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

// Product-Fixed EndProductType
void WindowCover::EndProductTypeSet(EmberAfWcEndProductType endProductType)
{
    if (endProductType != mEndProductType)
    {
        mEndProductType = endProductType;
        PostEvent(AppEvent::EventType::CoverEndProductTypeChange);
    }
}

EmberAfWcEndProductType WindowCover::EndProductTypeGet()
{
    return mEndProductType;
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

void WindowCover::LiftStepTowardOpen()  { ActuatorStepTowardOpen(&mLift); }
void WindowCover::TiltStepTowardOpen()  { ActuatorStepTowardOpen(&mTilt); }
void WindowCover::LiftStepTowardClose() { ActuatorStepTowardClose(&mLift); }
void WindowCover::TiltStepTowardClose() { ActuatorStepTowardClose(&mTilt); }

void WindowCover::StepUpOrOpen()        { mSelectedActuator ? TiltStepTowardOpen()  : LiftStepTowardOpen();  }
void WindowCover::StepDownOrClose()     { mSelectedActuator ? TiltStepTowardClose() : LiftStepTowardClose(); }

uint16_t WindowCover::LiftCurrentValueGet(void)                    { return mLift.currentPosition; }
uint16_t WindowCover::TiltCurrentValueGet(void)                    { return mTilt.currentPosition; }

posPercent100ths_t WindowCover::LiftCurrentPercent100thsGet(void)  { return PositionToPercent100ths(&mLift, mLift.currentPosition); }
posPercent100ths_t WindowCover::TiltCurrentPercent100thsGet(void)  { return PositionToPercent100ths(&mTilt, mTilt.currentPosition); }

posPercent100ths_t WindowCover::LiftTargetPercent100thsGet(void)   { return PositionToPercent100ths(&mLift, mLift.targetPosition); }
posPercent100ths_t WindowCover::TiltTargetPercent100thsGet(void)   { return PositionToPercent100ths(&mTilt, mTilt.targetPosition); }

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
        pAct->state = (value > pAct->currentPosition) ? MovingDownOrClose : MovingUpOrOpen;
        pAct->currentPosition = value;

        // Trick here If direct command set Target to go directly to position
        if (!pAct->timer.IsActive()) pAct->targetPosition = pAct->currentPosition;

        PostEvent(pAct->event);
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
        pAct->state = (value > pAct->currentPosition) ? MovingDownOrClose : MovingUpOrOpen;
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

posPercent100ths_t WindowCover::PositionToPercent100ths(CoverActuator_t * pActuator, uint16_t position)
{
    if (!pActuator) return UINT16_MAX;

    return wcAbsPositionToRelPercent100ths(pActuator->openLimit, pActuator->closedLimit, position);
}

uint16_t WindowCover::Percent100thsToPosition(CoverActuator_t * pActuator, posPercent100ths_t percent100ths)
{
    if (!pActuator) return UINT16_MAX;

    return wcRelPercent100thsToAbsPosition(pActuator->openLimit, pActuator->closedLimit, percent100ths);
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

void WindowCover::SelectedActuatorSet(bool mode)
{
    if (mode != mSelectedActuator)
    {
        mSelectedActuator = mode;
        PostEvent(AppEvent::EventType::CoverActuatorChange);
    }
}

bool WindowCover::SelectedActuatorGet()
{
    return mSelectedActuator;
}

WindowCover::CoverActuator_t * WindowCover::ActuatorGetLift(void) { return &mLift; }
WindowCover::CoverActuator_t * WindowCover::ActuatorGetTilt(void) { return &mTilt; }

void WindowCover::ToggleActuator()
{
    SelectedActuatorSet(!SelectedActuatorGet());
}



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

    switch (pAct->state)
    {
    case MovingDownOrClose:
        if (pAct->currentPosition < pAct->targetPosition)
        {
            pCover->ActuatorStepTowardClose(pAct);
            timer.Start();
        }
        break;
    case MovingUpOrOpen:
        if (pAct->currentPosition > pAct->targetPosition)
        {
            pCover->ActuatorStepTowardOpen(pAct);
            timer.Start();
        }
        break;
    case Stall:
    default:
        timer.Stop();
        break;
    }

    if (!timer.IsActive())
    {
        pAct->state = Stall;
        pCover->PostEvent(AppEvent::EventType::CoverStop);
    }
}

void WindowCover::PostEvent(AppEvent::EventType event)
{
    AppTask::Instance().PostEvent(AppEvent(event, this));
}
