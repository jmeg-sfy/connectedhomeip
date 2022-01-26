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

#pragma once

#include <app-common/zap-generated/enums.h>
#include <app/util/af-types.h>
#include <lib/core/CHIPError.h>
#include <app/clusters/window-covering-server/window-covering-server.h>


using namespace chip::app::Clusters::WindowCovering;

class WindowApp
{
public:
    struct Timer
    {
        typedef void (*Callback)(Timer & timer);

        Timer(const char * name, uint32_t timeoutInMs, Callback callback, void * context) :
            mName(name), mTimeoutInMs(timeoutInMs), mCallback(callback), mContext(context)
        {}
        virtual ~Timer()     = default;
        virtual void Start() = 0;
        virtual void Stop()  = 0;
        void Timeout();

        const char * mName    = nullptr;
        uint32_t mTimeoutInMs = 0;
        Callback mCallback    = nullptr;
        void * mContext       = nullptr;
        bool mIsActive        = false;
    };

    struct Button
    {
        enum class Id
        {
            Up   = 0,
            Down = 1
        };

        Button(Id id, const char * name) : mId(id), mName(name) {}
        virtual ~Button() = default;
        void Press();
        void Release();

        Id mId;
        const char * mName = nullptr;
        bool mPressed      = false;
        bool mSuppressed   = false;
    };

    enum class EventId
    {
        None = 0,

        // Reset events
        ResetStart,
        ResetPressed,
        ResetWarning,
        ResetCanceled,

        // Button events
        BtnUpPressed,
        BtnUpReleased,
        BtnDownPressed,
        BtnDownReleased,
        BtnCycleEndpoint,
        BtnCycleActuator,

        // Actuator Update Change
        ActuatorUpdateLift,
        ActuatorUpdateTilt,

        // Cover Attribute update events
        AttributeChange,

        // Provisioning events
        ProvisionedStateChanged,
        ConnectivityStateChanged,
        BLEConnectionsChanged,
        WinkOff,
        WinkOn,
    };


    struct Event
    {
        Event(EventId id) : mId(id), mEndpoint(1) {}
        Event(EventId id, chip::EndpointId endpoint) : mId(id), mEndpoint(endpoint) {}
        Event(EventId id, chip::EndpointId endpoint, chip::AttributeId attributeId) : mId(id), mEndpoint(endpoint), mAttributeId(attributeId) {}

        EventId mId;
        chip::EndpointId mEndpoint;
        chip::AttributeId mAttributeId;
    };

    struct Actuator
    {
        ActuatorAccessors & mAttributes = LiftAccess(); //defaulted to Lift by default

        uint16_t mCurrentPosition    = 0;//LimitStatus::IsUpOrOpen;//WC_PERCENT100THS_DEF;
        uint16_t mTargetPosition     = 0;//OperationalState::MovingDownOrClose;//WC_PERCENT100THS_DEF;


        void SetPosition(uint16_t value);
        void StepTowardUpOrOpen();//chip::EndpointId endpoint);
        void StepTowardDownOrClose();//chip::EndpointId endpoint);//not sure yet

        void GoToTargetPositionAttribute(chip::EndpointId endpoint);
        void UpdateCurrentPositionAttribute(chip::EndpointId endpoint);

        void GoToAbsolute(uint16_t absolute);
        void GoToRelative(chip::Percent100ths relative);
        void StopMotion();
        void UpdatePosition();
        void PostUpdateAttributes();
        void Print();

        void TimerStart();
        void TimerStop();
        bool IsActive();
        LimitStatus GetLimitState();

        static void OnActuatorTimeout(Timer & timer);
        void Init(WcFeature feature, uint32_t timeoutInMs, OperationalState * opState, uint16_t stepDelta);

        Timer *          mTimer   = nullptr;
        OperationalState mOpState = OperationalState::Stall;

        private:
        uint16_t mStepDelta          = 1;
        uint16_t mStepMinimum        = 1;
        uint16_t mNumberOfActuations = 0; //Number of commands sent to the actuators
    };


    struct Cover
    {
        enum ControlMode
        {
            TiltOnly = 0,
            LiftOnly,
            All,    //Tilt and Lift
        };

        void Init(chip::EndpointId endpoint);
        void Finish();
        void StopMotion(ControlMode controlMode);
        void StepTowardDownOrClose(ControlMode controlMode);
        void StepTowardUpOrOpen(ControlMode controlMode);
        void UpdateCurrentPositionAttribute(ControlMode controlMode);

        EmberAfWcType CycleType();

        chip::EndpointId mEndpoint = 0;

        // Attribute: Id 10 OperationalStatus
        OperationalStatus mOperationalStatus = { .global = OperationalState::Stall,
                                                 .lift   = OperationalState::Stall,
                                                 .tilt   = OperationalState::Stall };

        //protected:
        Actuator mLift, mTilt;

    };

    static WindowApp & Instance();

    virtual ~WindowApp() = default;
    virtual CHIP_ERROR Init();
    virtual CHIP_ERROR Start() = 0;
    virtual CHIP_ERROR Run();
    virtual void Finish();
    virtual void PostEvent(const Event & event) = 0;
    virtual void PostAttributeChange(chip::EndpointId endpoint, chip::AttributeId attributeId) = 0;
    void ToogleControlMode(void);

protected:
    struct StateFlags
    {
#if CHIP_ENABLE_OPENTHREAD
        bool isThreadProvisioned = false;
        bool isThreadEnabled     = false;
#else
        bool isWiFiProvisioned = false;
        bool isWiFiEnabled     = false;
#endif
        bool haveBLEConnections = false;
        bool isWinking          = false;
    };

    Cover & GetCover();
    Cover * GetCover(chip::EndpointId endpoint);

    virtual Button * CreateButton(Button::Id id, const char * name) = 0;
    virtual void DestroyButton(Button * b);
    virtual Timer * CreateTimer(const char * name, uint32_t timeoutInMs, Timer::Callback callback, void * context) = 0;
    virtual void DestroyTimer(Timer * timer);

    virtual void ProcessEvents() = 0;
    virtual void DispatchEvent(const Event & event);
    virtual void OnMainLoop() = 0;
    static void OnLongPressTimeout(Timer & timer);

    Timer * mLongPressTimer = nullptr;
    Button * mButtonUp      = nullptr;
    Button * mButtonDown    = nullptr;
    Cover::ControlMode mControlMode;

    StateFlags mState;



    bool mResetWarning   = false;

private:
    void HandleLongPress();
    void DispatchEventAttributeChange(chip::EndpointId endpoint, chip::AttributeId attribute);

    Cover mCoverList[WINDOW_COVER_COUNT];

    uint8_t mCurrentCover = 0;
};
