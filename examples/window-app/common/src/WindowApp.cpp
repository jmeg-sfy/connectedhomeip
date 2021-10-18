/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#include <AppConfig.h>
#include <WindowApp.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/clusters/window-covering-server/window-covering-server.h>
#include <app/server/Server.h>
#include <app/util/af.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <platform/CHIPDeviceLayer.h>

using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;
using namespace chip::app::Clusters::WindowCovering;

inline void OnTriggerEffectCompleted(chip::System::Layer * systemLayer, void * appState)
{
    WindowApp::Instance().PostEvent(WindowApp::EventId::WinkOff);
}

void OnTriggerEffect(Identify * identify)
{
    EmberAfIdentifyEffectIdentifier sIdentifyEffect = identify->mCurrentEffectIdentifier;

    ChipLogProgress(Zcl, "IDENTFY  OnTriggerEffect");

    if (identify->mCurrentEffectIdentifier == EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_CHANNEL_CHANGE)
    {
        ChipLogProgress(Zcl, "IDENTIFY_EFFECT_IDENTIFIER_CHANNEL_CHANGE - Not supported, use effect varriant %d",
                        identify->mEffectVariant);
        sIdentifyEffect = static_cast<EmberAfIdentifyEffectIdentifier>(identify->mEffectVariant);
    }

    switch (sIdentifyEffect)
    {
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_BLINK:
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_BREATHE:
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_OKAY:
        WindowApp::Instance().PostEvent(WindowApp::EventId::WinkOn);
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(5), OnTriggerEffectCompleted, identify);
        break;
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_FINISH_EFFECT:
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_STOP_EFFECT:
        (void) chip::DeviceLayer::SystemLayer().CancelTimer(OnTriggerEffectCompleted, identify);
        break;
    default:
        ChipLogProgress(Zcl, "No identifier effect");
    }
}

Identify gIdentify = {
    chip::EndpointId{ 1 },
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyStart"); },
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyStop"); },
    EMBER_ZCL_IDENTIFY_IDENTIFY_TYPE_VISIBLE_LED,
    OnTriggerEffect,
};

void WindowApp::Timer::Timeout()
{
    mIsActive = false;
    if (mCallback)
    {
        mCallback(*this);
    }
}

void WindowApp::Button::Press()
{
    EventId event = Button::Id::Up == mId ? EventId::UpPressed : EventId::DownPressed;
    Instance().PostEvent(WindowApp::Event(event));
}

void WindowApp::Button::Release()
{
    Instance().PostEvent(Button::Id::Up == mId ? EventId::UpReleased : EventId::DownReleased);
}

WindowApp::Cover & WindowApp::GetCover()
{
    return mCoverList[mCurrentCover];
}

WindowApp::Cover * WindowApp::GetCover(chip::EndpointId endpoint)
{
    for (uint16_t i = 0; i < WINDOW_COVER_COUNT; ++i)
    {
        if (mCoverList[i].mEndpoint == endpoint)
        {
            return &mCoverList[i];
        }
    }
    return nullptr;
}

CHIP_ERROR WindowApp::Init()
{
    // Init ZCL Data Model
    chip::Server::GetInstance().Init();

    // Initialize device attestation config
    SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());

    ConfigurationMgr().LogDeviceConfig();

    // Timers
    mLongPressTimer = CreateTimer("Timer:LongPress", LONG_PRESS_TIMEOUT, OnLongPressTimeout, this);

    // Buttons
    mButtonUp   = CreateButton(Button::Id::Up, "UP");
    mButtonDown = CreateButton(Button::Id::Down, "DOWN");

    // Coverings
    mCoverList[0].Init(WINDOW_COVER_ENDPOINT1);
    mCoverList[1].Init(WINDOW_COVER_ENDPOINT2);

    return CHIP_NO_ERROR;
}

CHIP_ERROR WindowApp::Run()
{
    StateFlags oldState;

#if CHIP_ENABLE_OPENTHREAD
    oldState.isThreadProvisioned = !ConnectivityMgr().IsThreadProvisioned();
#else
    oldState.isWiFiProvisioned = !ConnectivityMgr().IsWiFiStationProvisioned();
#endif
    while (true)
    {
        ProcessEvents();

        // Collect connectivity and configuration state from the CHIP stack. Because
        // the CHIP event loop is being run in a separate task, the stack must be
        // locked while these values are queried.  However we use a non-blocking
        // lock request (TryLockCHIPStack()) to avoid blocking other UI activities
        // when the CHIP task is busy (e.g. with a long crypto operation).
        if (PlatformMgr().TryLockChipStack())
        {
#if CHIP_ENABLE_OPENTHREAD
            mState.isThreadProvisioned = ConnectivityMgr().IsThreadProvisioned();
            mState.isThreadEnabled     = ConnectivityMgr().IsThreadEnabled();
#else
            mState.isWiFiProvisioned = ConnectivityMgr().IsWiFiStationProvisioned();
            mState.isWiFiEnabled     = ConnectivityMgr().IsWiFiStationEnabled();
#endif
            mState.haveBLEConnections = (ConnectivityMgr().NumBLEConnections() != 0);
            PlatformMgr().UnlockChipStack();
        }

#if CHIP_ENABLE_OPENTHREAD
        if (mState.isThreadProvisioned != oldState.isThreadProvisioned)
#else
        if (mState.isWiFiProvisioned != oldState.isWiFiProvisioned)
#endif
        {
            // Provisioned state changed
            DispatchEvent(EventId::ProvisionedStateChanged);
        }

        if (mState.haveBLEConnections != oldState.haveBLEConnections)
        {
            // Provisioned state changed
            DispatchEvent(EventId::BLEConnectionsChanged);
        }

        OnMainLoop();
        oldState = mState;
    }

    return CHIP_NO_ERROR;
}

void WindowApp::Finish()
{
    DestroyTimer(mLongPressTimer);
    DestroyButton(mButtonUp);
    DestroyButton(mButtonDown);
    for (uint16_t i = 0; i < WINDOW_COVER_COUNT; ++i)
    {
        mCoverList[i].Finish();
    }
}

void WindowApp::DispatchEvent(const WindowApp::Event & event)
{
    Cover * cover = nullptr;
    cover = GetCover(event.mEndpoint);


    emberAfWindowCoveringClusterPrint("Ep[%u] DispatchEvent=%u %p \n", event.mEndpoint , event.mId , cover);

     cover = &GetCover();

    switch (event.mId) {
    case EventId::ResetWarning:
        mResetWarning = true;
        if (mLongPressTimer)
        {
            mLongPressTimer->Start();
        }
        break;

    case EventId::ResetCanceled:
        mResetWarning = false;
        break;

    case EventId::Reset:
        ConfigurationMgr().InitiateFactoryReset();
        break;

    case EventId::UpPressed:
        emberAfWindowCoveringClusterPrint("UpPressed");
        mUpPressed = true;
        if (mLongPressTimer)
        {
            mLongPressTimer->Start();
        }
        break;

    case EventId::UpReleased:
        emberAfWindowCoveringClusterPrint("UpReleased");
        mUpPressed = false;
        if (mLongPressTimer)
        {
            mLongPressTimer->Stop();
        }
        if (mResetWarning)
        {
            PostEvent(EventId::ResetCanceled);
        }
        if (mUpSuppressed)
        {
            mUpSuppressed = false;
        }
        else if (mDownPressed)
        {
            if (ButtonCtrlMode::Tilt == mButtonCtrlMode)
                mButtonCtrlMode = ButtonCtrlMode::Lift;
            else
                mButtonCtrlMode = ButtonCtrlMode::Tilt;

            mUpSuppressed = mDownSuppressed = true;
            PostEvent(EventId::BtnCycleActuator);
        }
        else
        {
            emberAfWindowCoveringClusterPrint("Forward");
            if (ButtonCtrlMode::Tilt == mButtonCtrlMode)
                GetCover().mTilt.StepTowardUpOrOpen();
            else
                GetCover().mLift.StepTowardUpOrOpen();
        }


        break;

    case EventId::DownPressed:
        mDownPressed = true;
        if (mLongPressTimer)
        {
            mLongPressTimer->Start();
        }
        break;

    case EventId::DownReleased:
        mDownPressed = false;
        if (mLongPressTimer)
        {
            mLongPressTimer->Stop();
        }
        if (mResetWarning)
        {
            PostEvent(EventId::ResetCanceled);
        }
        if (mDownSuppressed)
        {
            mDownSuppressed = false;
        }
        else if (mUpPressed)
        {
            if (ButtonCtrlMode::Tilt == mButtonCtrlMode)
                mButtonCtrlMode = ButtonCtrlMode::Lift;
            else
                mButtonCtrlMode = ButtonCtrlMode::Tilt;

            mUpSuppressed = mDownSuppressed = true;
            PostEvent(EventId::BtnCycleActuator);
        }
        else
            if (ButtonCtrlMode::Tilt == mButtonCtrlMode)
                GetCover().mTilt.StepTowardDownOrClose();
            else
                GetCover().mLift.StepTowardDownOrClose();
        break;

    case EventId::LiftTargetPosition:
        if (cover) {
            cover->mTilt.GoToAbsolute(LiftTargetPositionAbsoluteGet(event.mEndpoint));
        }
        break;
    case EventId::TiltTargetPosition:
        if (cover) {
            cover->mTilt.GoToAbsolute(TiltTargetPositionAbsoluteGet(event.mEndpoint));
        }
        break;
    case EventId::StopMotion:
        if (cover) {
            cover->StopMotion();
        }
        break;
    case EventId::LiftUpdate:
        if (cover) {
            cover->mOperationalStatus.lift = cover->mLift.mOpState;
            OperationalStatusSetWithGlobalUpdated(cover->mEndpoint, cover->mOperationalStatus);
            //LiftCurrentPositionAbsoluteSet(cover->mEndpoint, cover->mLift.mCurrentPosition);
            CurrentPositionAbsoluteSet(cover->mEndpoint, LiftAccess(), cover->mLift.mCurrentPosition);
        }
        break;
    case EventId::TiltUpdate:
        if (cover) {
            cover->mOperationalStatus.tilt = cover->mTilt.mOpState;
            OperationalStatusSetWithGlobalUpdated(cover->mEndpoint, cover->mOperationalStatus);
            TiltCurrentPositionAbsoluteSet(cover->mEndpoint, cover->mTilt.mCurrentPosition);
        }
        break;
    case EventId::BtnCycleActuator:
        if (cover) {
            cover->mLift.GoToAbsolute(50);
            //OperationalStatusSet(event.mEndpoint, cover->mOperationalStatus);
        }
        break;
    case EventId::OperationalStatus:
        emberAfWindowCoveringClusterPrint("OpState: %02X\n", OperationalStatusGet(cover->mEndpoint));
        break;
    default:
        break;
    }
}

void WindowApp::DestroyTimer(Timer * timer)
{
    if (timer)
    {
        delete timer;
    }
}

void WindowApp::DestroyButton(Button * btn)
{
    if (btn)
    {
        delete btn;
    }
}

void WindowApp::HandleLongPress()
{
    if (mUpPressed && mDownPressed)
    {
        // Long press both buttons: Cycle between window coverings
        mUpSuppressed = mDownSuppressed = true;
        mCurrentCover                   = mCurrentCover < WINDOW_COVER_COUNT - 1 ? mCurrentCover + 1 : 0;
        PostEvent(EventId::BtnCycleType);
    }
    else if (mUpPressed)
    {
        mUpSuppressed = true;
        if (mResetWarning)
        {
            // Double long press button up: Reset now, you were warned!
            PostEvent(EventId::Reset);
        }
        else
        {
            // Long press button up: Reset warning!
            PostEvent(EventId::ResetWarning);
        }
    }
    else if (mDownPressed)
    {
        // Long press button down: Cycle between covering types
        mDownSuppressed          = true;
        EmberAfWcType cover_type = GetCover().CycleType();
        if (EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT == cover_type)
            mButtonCtrlMode = ButtonCtrlMode::Tilt;
        else
            mButtonCtrlMode = ButtonCtrlMode::Lift;
    }
}

void WindowApp::OnLongPressTimeout(WindowApp::Timer & timer)
{
    WindowApp * app = static_cast<WindowApp *>(timer.mContext);
    if (app)
    {
        app->HandleLongPress();
    }
}


LimitStatus WindowApp::Actuator::GetLimitState()
{
    if (mLimits.open > mLimits.closed)
        return LimitStatus::Inverted;

    if (mCurrentPosition == mLimits.open)
        return LimitStatus::IsUpOrOpen;

    if (mCurrentPosition == mLimits.closed)
        return LimitStatus::IsDownOrClose;

    if (mCurrentPosition < mLimits.open)
        return LimitStatus::IsOverUpOrOpen;

    if (mCurrentPosition > mLimits.closed)
        return LimitStatus::IsOverDownOrClose;

    return LimitStatus::Intermediate;
}



void WindowApp::Actuator::OnActuatorTimeout(WindowApp::Timer & timer)
{
    WindowApp::Actuator * actuator = static_cast<WindowApp::Actuator *>(timer.mContext);
    if (actuator) actuator->UpdatePosition();
}

void WindowApp::Actuator::Init(const char * name, uint32_t timeoutInMs, OperationalState * opState, EventId event)
{
    mTimer = WindowApp::Instance().CreateTimer(name, timeoutInMs, OnActuatorTimeout, this);
   // mOpState = opState;
    mEvent = event;
}

void WindowApp::Cover::Init(chip::EndpointId endpoint)
{
    mEndpoint  = endpoint;


mLift.mLimits.open = LIFT_OPEN_LIMIT;
mLift.mLimits.closed = LIFT_CLOSED_LIMIT;
mLift.mStepDelta = LIFT_DELTA;
//mLift.mEvent = LiftUpdate;


mTilt.mLimits.open = TILT_OPEN_LIMIT;
mTilt.mLimits.closed = TILT_CLOSED_LIMIT;
mTilt.mStepDelta = TILT_DELTA;


    mLift.Init("Lift", COVER_LIFT_TILT_TIMEOUT, nullptr, EventId::LiftUpdate);
    mTilt.Init("Tilt", COVER_LIFT_TILT_TIMEOUT, nullptr, EventId::TiltUpdate);

    //mTiltTimer = WindowApp::Instance().CreateTimer("Timer:Tilt", COVER_LIFT_TILT_TIMEOUT, OnTiltTimeout, this);
    Attributes::InstalledOpenLimitLift::Set(endpoint, mLift.mLimits.open);
    Attributes::InstalledClosedLimitLift::Set(endpoint, mLift.mLimits.closed);
    //CurrentPositionAbsoluteSet(endpoint, mLift.mLimits.closed, LiftAccess());

    Attributes::InstalledOpenLimitTilt::Set(endpoint, mTilt.mLimits.open);
    Attributes::InstalledClosedLimitTilt::Set(endpoint, mTilt.mLimits.closed);
    TiltCurrentPositionAbsoluteSet(endpoint, mTilt.mLimits.closed);

    // Attribute: Id  0 Type
    TypeSet(endpoint, EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT);

    // Attribute: Id  7 ConfigStatus
    ConfigStatus configStatus = { .operational             = 1,
                                  .online                  = 1,
                                  .liftIsReversed          = 0,
                                  .liftIsPA                = (HasFeature(endpoint, Features::Lift) && HasFeature(endpoint, Features::PositionAware)),
                                  .tiltIsPA                = (HasFeature(endpoint, Features::Tilt) && HasFeature(endpoint, Features::PositionAware)),
                                  .liftIsEncoderControlled = 1,
                                  .tiltIsEncoderControlled = 1 };
    ConfigStatusSet(endpoint, configStatus);


    OperationalStatusSet(endpoint, mOperationalStatus);

    // Attribute: Id 13 EndProductType
    EndProductTypeSet(endpoint, EMBER_ZCL_WC_END_PRODUCT_TYPE_INTERIOR_BLIND);

    // Attribute: Id 24 Mode
    Mode mode = { .motorDirReversed = 0, .calibrationMode = 1, .maintenanceMode = 1, .ledDisplay = 1 };
    ModeSet(endpoint, mode);

    // Attribute: Id 27 SafetyStatus (Optional)
    SafetyStatus safetyStatus = { 0x00 }; // 0 is no issues;
    SafetyStatusSet(endpoint, safetyStatus);
}

//Actuator

//currentPosition
//targetPosition

void WindowApp::Cover::Finish()
{
    WindowApp::Instance().DestroyTimer(mLift.mTimer);
    WindowApp::Instance().DestroyTimer(mTilt.mTimer);
}





EmberAfWcType WindowApp::Cover::CycleType()
{
    EmberAfWcType type = TypeGet(mEndpoint);

    switch (type)
    {
    case EMBER_ZCL_WC_TYPE_ROLLERSHADE:
        type = EMBER_ZCL_WC_TYPE_DRAPERY;
        // tilt = false;
        break;
    case EMBER_ZCL_WC_TYPE_DRAPERY:
        type = EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT;
        break;
    case EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT:
        type = EMBER_ZCL_WC_TYPE_ROLLERSHADE;
        // tilt = false;
        break;
    default:
        type = EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT;
    }
    TypeSet(mEndpoint, type);
    return type;
}

void WindowApp::Cover::StopMotion()
{
    mTilt.StopMotion();
    mLift.StopMotion();
}
//#############3
void WindowApp::Actuator::TimerStart()
{
    if (mTimer) mTimer->Start();
}


void WindowApp::Actuator::TimerStop()
{
    if (mTimer) mTimer->Stop();
}

bool WindowApp::Actuator::IsActive()
{
    if (mTimer)
        return mTimer->mIsActive;

    return false;
}

void WindowApp::Actuator::StepTowardUpOrOpen()
{
    emberAfWindowCoveringClusterPrint(__func__);
    if (mStepDelta < mStepMinimum) {
        mStepDelta = mStepMinimum;
    }

    if (mCurrentPosition >= mStepDelta) {
        SetPosition(mCurrentPosition - mStepDelta);
    } else {
        SetPosition(mLimits.open); //Percent100ths attribute will be set to 0%.
    }
}

void WindowApp::Actuator::StepTowardDownOrClose()
{
        emberAfWindowCoveringClusterPrint(__func__);
    if (mStepDelta < mStepMinimum) {
        mStepDelta = mStepMinimum;
    }


    //EFR32_LOG("ActuatorStepTowardClose %u %u %u", pAct->currentPosition, (pAct->stepDelta - pAct->closedLimit),pAct->closedLimit );
    if (mCurrentPosition <= (mLimits.closed - mStepDelta)) {
        SetPosition(mCurrentPosition + mStepDelta);
    } else {
        SetPosition(mLimits.closed); //Percent100ths attribute will be set to 100%.
    }
}


void WindowApp::Actuator::StopMotion()
{
    emberAfWindowCoveringClusterPrint(__func__);
    GoToAbsolute(mCurrentPosition);
}

// void WindowApp::Actuator::StepTowardUpOrOpen()
// {
//     //actuator.Pull();

//     if (actuator.currentPosition < actuator.openLimit)
//     {
//         currentPosition += stepDelta;
//     }
//     else
//     {
//         actuator.currentPosition = actuator.openLimit;
//     }
//     //actuator.Push();
// }

// void WindowApp::Actuator::StepTowardDownOrClose(Actuator &actuator)
// {
//     uint16_t percent100ths = 0;
//    //actuator.Pull();
//     if (actuator.currentPosition > actuator.currentPosition)
//     {
//         percent100ths -= 1000;
//     }
//     else
//     {
//         actuator.currentPosition = actuator.closedLimit;
//     }
//     //actuator.Push();
// }





void WindowApp::Actuator::Print(void)
{
    emberAfWindowCoveringClusterPrint("T=%u C=%u Delta=%u Min=%u", mTargetPosition, mCurrentPosition, mStepDelta, mStepMinimum);
    emberAfWindowCoveringClusterPrint("[%u, %u]", mLimits.open, mLimits.closed);
}

void WindowApp::Actuator::GoToAbsolute(uint16_t value)
{
        emberAfWindowCoveringClusterPrint(__func__);
    if (value > mLimits.closed) {
        value = mLimits.closed;
    } else if (value < mLimits.open) {
        value = mLimits.open;
    }

    mTargetPosition = value;

    if (mTargetPosition != mCurrentPosition) {
        mNumberOfActuations++;
        UpdatePosition(); //1st Update
       // TimerStart();
       // PostEvent(EventId::CoverStart);
    } else { //equals Target == Current
        //Nothing To do
        //TimerStop();
        //PostEvent(EventId::CoverStop);
    }


}

void WindowApp::Actuator::GoToRelative(chip::Percent100ths percent100ths)
{
}


void WindowApp::Actuator::SetPosition(uint16_t value)
{

    emberAfWindowCoveringClusterPrint(__func__);
    if (value > mLimits.closed) {
        value = mLimits.closed;
    } else if (value < mLimits.open) {
        value = mLimits.open;
    }

    if (value != mCurrentPosition)
    {

        mCurrentPosition = value;
        // Trick here If direct command set Target to go directly to position
        if (!IsActive()) {
            emberAfWindowCoveringClusterPrint("Mode Manual");
            // Manual mode : Here we simulate a user pulling the shade by hand -> Motor is OFF
            Instance().PostEvent(mEvent);// no matter what happened we must post an update event to refresh actuator attribute
        } else {
            emberAfWindowCoveringClusterPrint("Mode Automatic");
        }

    }

    Print();
}



void WindowApp::Actuator::UpdatePosition()
{
        emberAfWindowCoveringClusterPrint(__func__);
    uint16_t currMin = mCurrentPosition - mStepMinimum;
    if (currMin < mLimits.open) currMin = mLimits.open;

    uint16_t currMax = mCurrentPosition + mStepMinimum;
    if (currMax > mLimits.closed) currMax = mLimits.closed;

    // Detect when actuator cannot go further due to minStep
    if (((mTargetPosition <= currMax) && (mTargetPosition >= currMin)) || (mCurrentPosition == mTargetPosition))
    {
        mOpState = OperationalState::Stall;//Terminate movement
        emberAfWindowCoveringClusterPrint("Finish job min=%u med=%u max=%u T=%u", currMin, mCurrentPosition, currMax, mTargetPosition);
        mCurrentPosition = mTargetPosition;// <- succesfully achieved position is always equals to target

    }
    else //Movement must keep going
    {
        mOpState = (mTargetPosition > mCurrentPosition) ? OperationalState::MovingDownOrClose : OperationalState::MovingUpOrOpen;
    }

    switch (mOpState)
    {
        case OperationalState::MovingDownOrClose:
            TimerStart();
            StepTowardDownOrClose();
            break;
        case OperationalState::MovingUpOrOpen:
            TimerStart();
            StepTowardUpOrOpen();
            break;
        case OperationalState::Stall:
        default:
            TimerStop();
            break;
    }

    Instance().PostEvent(mEvent);// no matter what happened we must post an update event to refresh actuator attribute
    Print();
}


   // Attributes::GetTargetPositionActuatorPercent100ths(mEndpoint, &target);
    //Attributes::GetCurrentPositionActuatorPercent100ths(mEndpoint, &current);
   // mOpState = (mTargetPosition > mCurrentPosition) ? OperationalState::MovingDownOrClose : OperationalState::MovingUpOrOpen;

   // Attributes::GetTargetPositionLiftPercent100ths(mEndpoint, &target);
   // Attributes::GetCurrentPositionLiftPercent100ths(mEndpoint, &current);







