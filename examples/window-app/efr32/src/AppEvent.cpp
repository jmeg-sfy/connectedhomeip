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

#include <AppEvent.h>

AppEvent::AppEvent(EventType type, void * context) : mType(type), mContext(context) {}

const char * AppEvent::TypeString(EventType type)
{
    switch (type)
    {
    case EventType::None:
        return "None";

    case EventType::ButtonPressed:
        return "ButtonPressed";
    case EventType::ButtonReleased:
        return "ButtonReleased";
    case EventType::CoverModeChange:
        return "Mode";
    case EventType::CoverTypeChange:
        return "Type";
    case EventType::CoverEndProductTypeChange:
        return "EndProductType";
    case EventType::CoverConfigStatusChange:
        return "ConfigStatus";
    case EventType::CoverOperationalStatusChange:
        return "OperationalStatus";
    case EventType::CoverSafetyStatusChange:
        return "SafetyStatus";
    case EventType::CoverActuatorChange:
        return "SelectedActuator";
    case EventType::CoverLiftChange:
        return "Lift";
    case EventType::CoverTiltChange:
        return "Tilt";
    case EventType::CoverOpen:
        return "Open";
    case EventType::CoverClosed:
        return "Close";
    case EventType::CoverStart:
        return "Start";
    case EventType::CoverStop:
        return "Stop";

    default:
        return "?";
    }
}

