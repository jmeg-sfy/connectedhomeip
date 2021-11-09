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
    EventId event = Button::Id::Up == mId ? EventId::BtnUpPressed : EventId::BtnDownPressed;
    Instance().PostEvent(WindowApp::Event(event));
}

void WindowApp::Button::Release()
{
    Instance().PostEvent(Button::Id::Up == mId ? EventId::BtnUpReleased : EventId::BtnDownReleased);
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


void WindowApp::DispatchEventStateChange(const WindowApp::Event & event)
{
    Cover * cover = nullptr;
    cover = GetCover(event.mEndpoint);
    emberAfWindowCoveringClusterPrint("Ep[%u] DispatchEvent=%u %p \n", event.mEndpoint, (unsigned int) event.mId , cover);

    cover = &GetCover();

    switch (event.mId)
    {
    default:
        break;
    }
}


void WindowApp::DispatchEventAttributeChange(chip::EndpointId endpoint, chip::AttributeId attribute)
{
    Cover * cover = nullptr;
    cover = GetCover(endpoint);


    emberAfWindowCoveringClusterPrint("Ep[%u] DispatchEvent=%u %p \n", endpoint , (unsigned int)attribute , cover);

    //cover = &GetCover();

    switch (attribute)
    {
    /* For a device supporting Position Awareness : Changing the Target triggers motions on the real or simulated device */
    case Attributes::TargetPositionLiftPercent100ths::Id:
        if (cover) {
            cover->mLift.GoToTargetPositionAttribute(endpoint);
        }
        break;
    /* For a device supporting Position Awareness : Changing the Target triggers motions on the real or simulated device */
    case Attributes::TargetPositionTiltPercent100ths::Id:
        if (cover) {
            cover->mTilt.GoToTargetPositionAttribute(endpoint);
        }
        break;
    /* RO OperationalStatus */
    case Attributes::OperationalStatus::Id:
        emberAfWindowCoveringClusterPrint("OpState: %02X\n", (unsigned int) OperationalStatusGet(endpoint).global);
        break;
    /* RW Mode */
    case Attributes::Mode::Id:
        emberAfWindowCoveringClusterPrint("Mode set externally ignored");
        break;
    /* ### ATTRIBUTEs CHANGEs IGNORED ### */
    /* RO Type: not supposed to dynamically change -> Cycling Window Covering Demo */
    case Attributes::Type::Id:
    /* RO EndProductType: not supposed to dynamically change */
    case Attributes::EndProductType::Id:
    /* RO ConfigStatus: set by WC server */
    case Attributes::ConfigStatus::Id:
    /* RO SafetyStatus: set by WC server */
    case Attributes::SafetyStatus::Id:
    /* ============= Positions for Position Aware ============= */
    case Attributes::CurrentPositionLiftPercent100ths::Id:
    case Attributes::CurrentPositionTiltPercent100ths::Id:
    default:
        break;
    }
}

void WindowApp::ToogleControlMode(void)
{
    if (Cover::ControlMode::TiltOnly == mControlMode)
        mControlMode = Cover::ControlMode::LiftOnly;
    else
        mControlMode = Cover::ControlMode::TiltOnly;
}

void WindowApp::Cover::StepTowardUpOrOpen(ControlMode controlMode)
{
    switch (controlMode)
    {
    case ControlMode::TiltOnly:
        mTilt.StepTowardUpOrOpen();
        break;
    case ControlMode::LiftOnly:
        mLift.StepTowardUpOrOpen();
        break;
    case ControlMode::All:
        mTilt.StepTowardUpOrOpen();
        mLift.StepTowardUpOrOpen();
        break;
    }
}

void WindowApp::Cover::StepTowardDownOrClose(ControlMode controlMode)
{
    switch (controlMode)
    {
    case ControlMode::TiltOnly:
        mTilt.StepTowardDownOrClose();
        break;
    case ControlMode::LiftOnly:
        mLift.StepTowardDownOrClose();
        break;
    case ControlMode::All:
        mTilt.StepTowardDownOrClose();
        mLift.StepTowardDownOrClose();
        break;
    }
}

void WindowApp::Cover::StopMotion(ControlMode controlMode)
{

    switch (controlMode)
    {
    case ControlMode::TiltOnly:
        mTilt.StopMotion();
        break;
    case ControlMode::LiftOnly:
        mLift.StopMotion();
        break;
    case ControlMode::All:
        mTilt.StopMotion();
        mLift.StopMotion();
        break;
    }
}

void WindowApp::Cover::UpdateCurrentPositionAttribute(ControlMode controlMode)
{
    switch (controlMode)
    {
    case ControlMode::TiltOnly:
        mOperationalStatus.tilt = mTilt.mOpState;
        mTilt.UpdateCurrentPositionAttribute(mEndpoint);
        break;
    case ControlMode::LiftOnly:
        mOperationalStatus.lift = mLift.mOpState;
        mLift.UpdateCurrentPositionAttribute(mEndpoint);
        break;
    case ControlMode::All:
        mOperationalStatus.tilt = mTilt.mOpState;
        mTilt.UpdateCurrentPositionAttribute(mEndpoint);
        mOperationalStatus.lift = mLift.mOpState;
        mLift.UpdateCurrentPositionAttribute(mEndpoint);
        break;
    }
    OperationalStatusSetWithGlobalUpdated(mEndpoint, mOperationalStatus);
}


void WindowApp::DispatchEvent(const WindowApp::Event & event)
{
    Cover * cover = nullptr;
    cover = GetCover(event.mEndpoint);


    emberAfWindowCoveringClusterPrint("Ep[%u] DispatchEvent=%u %p \n", event.mEndpoint , (unsigned int) event.mId , cover);

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

    case EventId::ResetStart:
        ConfigurationMgr().InitiateFactoryReset();
        break;

    case EventId::BtnUpPressed:
        emberAfWindowCoveringClusterPrint("UpPressed");
        mButtonUp->mPressed = true;
        if (mLongPressTimer)
        {
            mLongPressTimer->Start();
        }
        break;

    case EventId::BtnUpReleased:
        emberAfWindowCoveringClusterPrint("UpReleased");
        mButtonUp->mPressed = false;
        if (mLongPressTimer)
        {
            mLongPressTimer->Stop();
        }
        if (mResetWarning)
        {
            PostEvent(EventId::ResetCanceled);
        }
        if (mButtonUp->mSuppressed)
        {
            mButtonUp->mSuppressed = false;
        }
        else if (mButtonDown->mPressed)
        {
            ToogleControlMode();
            mButtonUp->mSuppressed = mButtonDown->mSuppressed = true;
            PostEvent(EventId::BtnCycleActuator);
        }
        else
        {
            cover->StepTowardUpOrOpen(mControlMode);
        }


        break;

    case EventId::BtnDownPressed:
        mButtonDown->mPressed = true;
        if (mLongPressTimer)
        {
            mLongPressTimer->Start();
        }
        break;

    case EventId::BtnDownReleased:
        mButtonDown->mPressed = false;
        if (mLongPressTimer)
        {
            mLongPressTimer->Stop();
        }
        if (mResetWarning)
        {
            PostEvent(EventId::ResetCanceled);
        }
        if (mButtonDown->mSuppressed)
        {
            mButtonDown->mSuppressed = false;
        }
        else if (mButtonUp->mPressed)
        {
            ToogleControlMode();
            mButtonUp->mSuppressed = mButtonDown->mSuppressed = true;
            PostEvent(EventId::BtnCycleActuator);
        }
        else
            cover->StepTowardDownOrClose(mControlMode);
        break;
    case EventId::StopMotion:
        if (cover) {
            cover->StopMotion(Cover::ControlMode::All);
        }
        break;
    case EventId::ActuatorUpdateLift:
        if (cover) {
            cover->UpdateCurrentPositionAttribute(Cover::ControlMode::LiftOnly);
        }
        break;
    case EventId::ActuatorUpdateTilt:
        if (cover) {
            cover->UpdateCurrentPositionAttribute(Cover::ControlMode::TiltOnly);
        }
        break;
    case EventId::BtnCycleActuator:
        if (cover) {
            cover->mLift.GoToAbsolute(50);
            //OperationalStatusSet(event.mEndpoint, cover->mOperationalStatus);
        }
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
    if (mButtonUp->mPressed && mButtonDown->mPressed)
    {
        // Long press both buttons: Cycle between window coverings
        mButtonUp->mSuppressed = mButtonDown->mSuppressed = true;
        mCurrentCover                   = mCurrentCover < WINDOW_COVER_COUNT - 1 ? mCurrentCover + 1 : 0;
        PostEvent(EventId::BtnCycleType);
    }
    else if (mButtonUp->mPressed)
    {
        mButtonUp->mSuppressed = true;
        if (mResetWarning)
        {
            // Double long press button up: Reset now, you were warned!
            PostEvent(EventId::ResetStart);
        }
        else
        {
            // Long press button up: Reset warning!
            PostEvent(EventId::ResetWarning);
        }
    }
    else if (mButtonDown->mPressed)
    {
        // Long press button down: Cycle between covering types
        mButtonDown->mSuppressed          = true;
        EmberAfWcType cover_type = GetCover().CycleType();
        if (EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT == cover_type)
            mControlMode = Cover::ControlMode::TiltOnly;
        else
            mControlMode = Cover::ControlMode::LiftOnly;
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
    return CheckLimitState(mCurrentPosition, mAttributes.mLimits);
}



void WindowApp::Actuator::OnActuatorTimeout(WindowApp::Timer & timer)
{
    WindowApp::Actuator * actuator = static_cast<WindowApp::Actuator *>(timer.mContext);
    if (actuator) actuator->UpdatePosition();
}

void WindowApp::Actuator::Init(Features feature, uint32_t timeoutInMs, OperationalState * opState, uint16_t stepDelta)
{

   // mOpState = opState;
    mStepDelta = stepDelta;

    if (Features::Lift == feature)
    {
        mEvent = EventId::ActuatorUpdateLift;
        mAttributes = LiftAccess();
        mTimer = WindowApp::Instance().CreateTimer("Lift", timeoutInMs, OnActuatorTimeout, this);
    }
    else
    {
        mEvent = EventId::ActuatorUpdateTilt;
        mAttributes = TiltAccess();
        mTimer = WindowApp::Instance().CreateTimer("Tilt", timeoutInMs, OnActuatorTimeout, this);
    }
}

void WindowApp::Cover::Init(chip::EndpointId endpoint)
{
    mEndpoint = endpoint;

    mLift.Init(Features::Lift, COVER_LIFT_TILT_TIMEOUT, nullptr, LIFT_DELTA);
    mTilt.Init(Features::Tilt, COVER_LIFT_TILT_TIMEOUT, nullptr, TILT_DELTA);

    mLift.mAttributes.InitializeLimits(endpoint, { LIFT_OPEN_LIMIT, LIFT_CLOSED_LIMIT});
    mTilt.mAttributes.InitializeLimits(endpoint, { TILT_OPEN_LIMIT, TILT_CLOSED_LIMIT});

    // Attribute: Id  0 Type
    TypeSet(endpoint, EMBER_ZCL_WC_TYPE_TILT_BLIND_LIFT_AND_TILT);

    // Attribute: Id  7 ConfigStatus
    ConfigStatus configStatus = { .operational             = 1,
                                  .online                  = 1,
                                  .liftIsReversed          = 0,
                                  .liftIsPA                = mLift.mAttributes.IsPositionAware(endpoint),
                                  .tiltIsPA                = mTilt.mAttributes.IsPositionAware(endpoint),
                                  .liftIsEncoderControlled = 1,
                                  .tiltIsEncoderControlled = 1 };
    ConfigStatusSet(endpoint, configStatus);

    OperationalStatusSetWithGlobalUpdated(endpoint, mOperationalStatus);

    // Attribute: Id 13 EndProductType
    EndProductTypeSet(endpoint, EMBER_ZCL_WC_END_PRODUCT_TYPE_INTERIOR_BLIND);

    // Attribute: Id 24 Mode
    Mode mode = { .motorDirReversed = 0, .calibrationMode = 0, .maintenanceMode = 0, .ledDisplay = 0 };
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

    //AbsoluteLimits limits = mAttributes.AbsoluteLimitsGet(endpoint);
    AbsoluteLimits limits = mAttributes.mLimits;

    if (mStepDelta < mStepMinimum) {
        mStepDelta = mStepMinimum;
    }

    if (mCurrentPosition >= mStepDelta) {
        SetPosition(mCurrentPosition - mStepDelta);
    } else {
        SetPosition(limits.open); //Percent100ths attribute will be set to 0%.
    }
}

void WindowApp::Actuator::StepTowardDownOrClose()//endpoint
{
    emberAfWindowCoveringClusterPrint(__func__);

    //AbsoluteLimits limits = mAttributes.AbsoluteLimitsGet(endpoint);
    AbsoluteLimits limits = mAttributes.mLimits;

    if (mStepDelta < mStepMinimum) {
        mStepDelta = mStepMinimum;
    }


    //EFR32_LOG("ActuatorStepTowardClose %u %u %u", pAct->currentPosition, (pAct->stepDelta - pAct->closedLimit),pAct->closedLimit );
    if (mCurrentPosition <= (limits.closed - mStepDelta)) {
        SetPosition(mCurrentPosition + mStepDelta);
    } else {
        SetPosition(limits.closed); //Percent100ths attribute will be set to 100%.
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
    AbsoluteLimits limits = mAttributes.mLimits;

    emberAfWindowCoveringClusterPrint("T=%u C=%u Delta=%u Min=%u", mTargetPosition, mCurrentPosition, mStepDelta, mStepMinimum);
    emberAfWindowCoveringClusterPrint("[%u, %u]", limits.open, limits.closed);
}

void WindowApp::Actuator::GoToAbsolute(uint16_t value)
{
    emberAfWindowCoveringClusterPrint(__func__);

    mTargetPosition = mAttributes.WithinAbsoluteRangeCheck(value);

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

void WindowApp::Actuator::GoToTargetPositionAttribute(chip::EndpointId endpoint)
{
    /* Update Local target value from Attribute */
    /* Trigger movement */
    GoToAbsolute(mAttributes.PositionAbsoluteGet(endpoint, ActuatorAccessors::PositionAccessors::Type::Target));
}

void WindowApp::Actuator::UpdateCurrentPositionAttribute(chip::EndpointId endpoint)
{
    /* Update Current position attribute from local current */
    /* Reflect current position to remote client */
    mAttributes.PositionAbsoluteSet(endpoint, ActuatorAccessors::PositionAccessors::Type::Current, mCurrentPosition);
}




void WindowApp::Actuator::GoToRelative(chip::Percent100ths percent100ths)
{
}


void WindowApp::Actuator::SetPosition(uint16_t value)
{

    emberAfWindowCoveringClusterPrint(__func__);

    value = mAttributes.WithinAbsoluteRangeCheck(value);

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

    AbsoluteLimits limits = mAttributes.mLimits;

    uint16_t currMin = mCurrentPosition - mStepMinimum;
    if (currMin < limits.open) currMin = limits.open;

    uint16_t currMax = mCurrentPosition + mStepMinimum;
    if (currMax > limits.closed) currMax = limits.closed;

    // Detect when actuator cannot go further due to minStep
    if (((mTargetPosition <= currMax) && (mTargetPosition >= currMin)) || (mCurrentPosition == mTargetPosition))
    {
        mOpState = OperationalState::Stall;//Terminate movement
        emberAfWindowCoveringClusterPrint("Finish job min=%u med=%u max=%u T=%u", currMin, mCurrentPosition, currMax, mTargetPosition);
        mCurrentPosition = mTargetPosition;// <- succesfully achieved position is always equals to target

    }
    else //Movement must keep going
    {
        mOpState = ComputeOperationalState(mTargetPosition, mCurrentPosition);
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







