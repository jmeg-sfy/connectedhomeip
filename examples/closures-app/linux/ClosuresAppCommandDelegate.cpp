/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
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

#include "ClosuresAppCommandDelegate.h"
#include <app/data-model/Nullable.h>
#include <platform/PlatformManager.h>

#include "closures-device.h"
#include <string>
#include <utility>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

ClosuresAppCommandHandler * ClosuresAppCommandHandler::FromJSON(const char * json)
{
    Json::Reader reader;
    Json::Value value;

    if (!reader.parse(json, value))
    {
        ChipLogError(NotSpecified, "Closures App: Error parsing JSON with error %s:", reader.getFormattedErrorMessages().c_str());
        return nullptr;
    }

    if (value.empty() || !value.isObject())
    {
        ChipLogError(NotSpecified, "Closures App: Invalid JSON command received");
        return nullptr;
    }

    if (!value.isMember("Name") || !value["Name"].isString())
    {
        ChipLogError(NotSpecified, "Closures App: Invalid JSON command received: command name is missing");
        return nullptr;
    }

    return Platform::New<ClosuresAppCommandHandler>(std::move(value));
}

void ClosuresAppCommandHandler::HandleCommand(intptr_t context)
{
    auto * self      = reinterpret_cast<ClosuresAppCommandHandler *>(context);
    std::string name = self->mJsonValue["Name"].asString();

    VerifyOrExit(!self->mJsonValue.empty(), ChipLogError(NotSpecified, "Invalid JSON event command received"));

    if (name == "GoRunning")
    {
        self->GoRunningStimuli();
    }
    if (name == "GoStopped")
    {
        self->GoStoppedStimuli();
    }
    if (name == "DownClose")
    {
        self->OnDownCloseHandler();
    }
    if (name == "CalibrationEnded")
    {
        self->OnCalibrationEndedHandler();
    }
    else if (name == "Engaged")
    {
        self->OnEngagedHandler();
    }
    else if (name == "Disengaged")
    {
        self->OnDisengagedHandler();
    }
    else if (name == "ProtectionRised")
    {
        self->OnProtectionRisedHandler();
    }
    else if (name == "ProtectionDropped")
    {
        self->OnProtectionDroppedHandler();
    }
    else if (name == "MovementComplete")
    {
        self->OnMovementCompleteHandler();
    }
    else if (name == "ErrorEvent")
    {
        std::string error = self->mJsonValue["Error"].asString();
        self->OnErrorEventHandler(error);
    }
    else if (name == "ClearError")
    {
        self->OnClearErrorHandler();
    }
    else if (name == "Reset")
    {
        self->OnResetHandler();
    }
    else
    {
        ChipLogError(NotSpecified, "Unhandled command: Should never happens");
    }

exit:
    Platform::Delete(self);
}

void ClosuresAppCommandHandler::SetClosuresDevice(chip::app::Clusters::ClosuresDevice * aClosuresDevice)
{
    mClosuresDevice = aClosuresDevice;
}

void ClosuresAppCommandDelegate::SetClosuresDevice(chip::app::Clusters::ClosuresDevice * aClosuresDevice)
{
    mClosuresDevice = aClosuresDevice;
}

void ClosuresAppCommandHandler::OnCalibrationEndedHandler()
{
    mClosuresDevice->HandleCalibrationEndedMessage();
}

void ClosuresAppCommandHandler::OnEngagedHandler()
{
    mClosuresDevice->HandleEngagedMessage();
}

void ClosuresAppCommandHandler::OnDisengagedHandler()
{
    mClosuresDevice->HandleDisengagedMessage();
}

void ClosuresAppCommandHandler::OnProtectionRisedHandler()
{
    mClosuresDevice->HandleProtectionRisedMessage();
}

void ClosuresAppCommandHandler::OnProtectionDroppedHandler()
{
    mClosuresDevice->HandleProtectionDroppedMessage();
}

void ClosuresAppCommandHandler::GoRunningStimuli()
{
    mClosuresDevice->HandleGoRunning();
}

void ClosuresAppCommandHandler::GoStoppedStimuli()
{
    mClosuresDevice->HandleGoStopped();
}

void ClosuresAppCommandHandler::OnDownCloseHandler()
{
    mClosuresDevice->HandleDownCloseMessage();
}

void ClosuresAppCommandHandler::OnMovementCompleteHandler()
{
    mClosuresDevice->HandleMovementCompleteEvent();
}

void ClosuresAppCommandHandler::OnErrorEventHandler(const std::string & error)
{
    mClosuresDevice->HandleErrorEvent(error);
}

void ClosuresAppCommandHandler::OnClearErrorHandler()
{
    mClosuresDevice->HandleClearErrorMessage();
}

void ClosuresAppCommandHandler::OnResetHandler()
{
    mClosuresDevice->HandleResetMessage();
}

void ClosuresAppCommandDelegate::OnEventCommandReceived(const char * json)
{
    auto handler = ClosuresAppCommandHandler::FromJSON(json);
    if (nullptr == handler)
    {
        ChipLogError(NotSpecified, "Closures App: Unable to instantiate a command handler");
        return;
    }

    handler->SetClosuresDevice(mClosuresDevice);
    chip::DeviceLayer::PlatformMgr().ScheduleWork(ClosuresAppCommandHandler::HandleCommand, reinterpret_cast<intptr_t>(handler));
}
