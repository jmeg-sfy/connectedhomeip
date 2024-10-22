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

    VerifyOrReturn(!self->mJsonValue.empty(), {
        ChipLogError(NotSpecified, "Invalid JSON event command received");
        Platform::Delete(self);
    });

    if (name == "MoveTo")
    {
        std::optional<uint8_t> tag = self->mJsonValue.isMember("Tag") ? 
                                    std::make_optional(static_cast<uint8_t>(self->mJsonValue["Tag"].asUInt())) : std::nullopt;

        std::optional<uint8_t> speed = self->mJsonValue.isMember("Speed") ? 
                                    std::make_optional(static_cast<uint8_t>(self->mJsonValue["Speed"].asUInt())) : std::nullopt;

        std::optional<uint8_t> latch = self->mJsonValue.isMember("Latch") ? 
                                    std::make_optional(static_cast<uint8_t>(self->mJsonValue["Latch"].asUInt())) : std::nullopt;

        self->MoveToStimuli(tag, speed, latch);
    }
    if (name == "Stop")
    {
        self->StopStimuli();
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
        ChipLogError(NotSpecified, "ClosuresAppDelegate: Unhandled command: Should never happens");
    }

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

void ClosuresAppCommandHandler::MoveToStimuli(std::optional<uint8_t> tag, 
                                              std::optional<uint8_t> speed, 
                                              std::optional<uint8_t> latch)
{
    mClosuresDevice->HandleMoveToStimuli(tag, latch, speed);
}

void ClosuresAppCommandHandler::StopStimuli()
{
    mClosuresDevice->HandleStopStimuli();
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
