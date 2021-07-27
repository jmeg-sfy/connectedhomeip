/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

#include <app/common/gen/attribute-id.h>
#include <app/common/gen/enums.h>
#include <app/util/af-enums.h>

class WindowCover
{
public:

    #if BYTE_ORDER==LITTLE_ENDIAN
    #elif BYTE_ORDER==BIG_ENDIAN
        #error "machine is big endian"
    #elif BYTE_ORDER==PDP_ENDIAN
        #error "jeez, machine is PDP!"
    #else
        #error "What kind of hardware is this?!"
    #endif

    enum class Feature
    {
        Lift          = 0x01,
        Tilt          = 0x02,
        PositionAware = 0x04
    };

    struct Mode
    {
    #if BYTE_ORDER==LITTLE_ENDIAN
        uint8_t motorDirReversed : 1; // bit 0
        uint8_t calibrationMode : 1;  // bit 1
        uint8_t maintenanceMode : 1;  // bit 2
        uint8_t ledDisplay : 1;       // bit 3
    #else /* BYTE_ORDER==BIG_ENDIAN */
        uint8_t ledDisplay : 1;       // bit 3
        uint8_t maintenanceMode : 1;  // bit 2
        uint8_t calibrationMode : 1;  // bit 1
        uint8_t motorDirReversed : 1; // bit 0
    #endif
    };

    static_assert(sizeof(Mode) == sizeof(uint8_t), "Mode Size is not correct");

    struct ConfigStatus
    {
        uint8_t operational : 1;             // bit 0 M
        uint8_t online : 1;                  // bit 1 M
        uint8_t liftIsReversed : 1;          // bit 2 LF
        uint8_t liftIsPA : 1;                // bit 3 LF & PA
        uint8_t tiltIsPA : 1;                // bit 4 TL & PA
        uint8_t liftIsEncoderControlled : 1; // bit 5 LF & PA
        uint8_t tiltIsEncoderControlled : 1; // bit 6 LF & PA
    };
    static_assert(sizeof(ConfigStatus) == sizeof(uint8_t), "ConfigStatus Size is not correct");

    // Match directly with OperationalStatus 2 bits Fields
    enum class OperationalState : uint8_t
    {
        Stall             = 0x00, // currently not moving
        MovingUpOrOpen    = 0x01, // is currently opening
        MovingDownOrClose = 0x02, // is currently closing
        Reserved          = 0x03, // dont use
    };
    static_assert(sizeof(OperationalState) == sizeof(uint8_t), "OperationalState Size is not correct");

    struct OperationalStatus
    {
        OperationalState global : 2; // bit 0-1 M
        OperationalState lift : 2;   // bit 2-3 LF
        OperationalState tilt : 2;   // bit 4-5 TL
    };
    static_assert(sizeof(OperationalStatus) == sizeof(uint8_t), "OperationalStatus Size is not correct");

    struct SafetyStatus
    {
        uint16_t remoteLockout : 1;       // bit 0
        uint16_t tamperDetection : 1;     // bit 1
        uint16_t failedCommunication : 1; // bit 2
        uint16_t positionFailure : 1;     // bit 3
        uint16_t thermalProtection : 1;   // bit 4
        uint16_t obstacleDetected : 1;    // bit 5
        uint16_t powerIssue : 1;          // bit 6
        uint16_t stopInput : 1;           // bit 7
        uint16_t motorJammed : 1;         // bit 8
        uint16_t hardwareFailure : 1;     // bit 9
        uint16_t manualOperation : 1;     // bit 10
    };
    static_assert(sizeof(SafetyStatus) == sizeof(uint16_t), "SafetyStatus Size is not correct");

    class Actuator
    {
    public:
        Actuator()          = default;
        virtual ~Actuator() = default;

        virtual void OpenLimitSet(uint16_t limit) = 0;
        virtual uint16_t OpenLimitGet()           = 0;

        virtual void ClosedLimitSet(uint16_t limit) = 0;
        virtual uint16_t ClosedLimitGet()           = 0;

        virtual void PositionSet(uint16_t percent100ths) = 0;
        virtual uint16_t PositionGet()                   = 0;
        uint16_t PositionValueGet();

        virtual void TargetSet(uint16_t percent100ths) = 0;
        virtual uint16_t TargetGet()                   = 0;

        void TargetValueSet(uint16_t value);
        uint16_t TargetValueGet();

        virtual void NumberOfActuationsIncrement() = 0;
        virtual uint16_t NumberOfActuationsGet()   = 0;

        void Stop(void){};

    protected:
        uint16_t ValueToPercent100ths(uint16_t position);
        uint16_t Percent100thsToValue(uint16_t percent100ths);
    };

    class LiftActuator : public Actuator
    {
    public:
        void OpenLimitSet(uint16_t limit) override;
        uint16_t OpenLimitGet() override;
        void ClosedLimitSet(uint16_t limit) override;
        uint16_t ClosedLimitGet() override;
        void PositionSet(uint16_t percent100ths) override;
        uint16_t PositionGet() override;
        void TargetSet(uint16_t percent100ths) override;
        uint16_t TargetGet() override;
        void NumberOfActuationsIncrement() override;
        uint16_t NumberOfActuationsGet() override;
    };

    class TiltActuator : public Actuator
    {
    public:
        void OpenLimitSet(uint16_t limit) override;
        uint16_t OpenLimitGet() override;
        void ClosedLimitSet(uint16_t limit) override;
        uint16_t ClosedLimitGet() override;
        void PositionSet(uint16_t percent100ths) override;
        uint16_t PositionGet() override;
        void TargetSet(uint16_t percent100ths) override;
        uint16_t TargetGet() override;
        void NumberOfActuationsIncrement() override;
        uint16_t NumberOfActuationsGet() override;
    };

    static WindowCover & Instance();
    static constexpr uint8_t mEndPoint = 1;

    bool hasFeature(Feature feat) { return (mFeatures & (uint8_t) feat) > 0; }

    LiftActuator & Lift() { return mLift; }
    TiltActuator & Tilt() { return mTilt; }

    // Attribute: Id  0 Type
    void TypeSet(EmberAfWcType type);
    EmberAfWcType TypeGet();

    // Attribute: Id  7 ConfigStatus
    void ConfigStatusSet(ConfigStatus status);
    const ConfigStatus ConfigStatusGet();

    // Attribute: Id 10 OperationalStatus
    void OperationalStatusSet(OperationalStatus status);
    const OperationalStatus OperationalStatusGet();

    // Attribute: Id 13 EndProductType
    void EndProductTypeSet(EmberAfWcEndProductType endProduct);
    EmberAfWcEndProductType EndProductTypeGet();

    // Attribute: Id 24 Mode
    void ModeSet(Mode mode);
    const Mode ModeGet();

    // OPTIONAL Attributes -- Setter/Getter Internal Variables equivalent
    // Attribute: Id 27 SafetyStatus (Optional)
    void SafetyStatusSet(SafetyStatus status);
    const SafetyStatus SafetyStatusGet();

    bool IsOpen();
    bool IsClosed();

private:
    static WindowCover sInstance;

    WindowCover();

    uint8_t mFeatures = ((uint8_t) Feature::Lift | (uint8_t) Feature::Tilt | (uint8_t) Feature::PositionAware);
    LiftActuator mLift;
    TiltActuator mTilt;
};
