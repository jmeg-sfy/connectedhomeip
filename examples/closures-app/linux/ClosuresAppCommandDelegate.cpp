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

namespace {
    enum class CommandType {
        MoveTo,
        Stop,
        Calibrate,
        ConfigureFallback,
        Protected,
        Unprotected,
        SetReadyToRun,
        SetActionNeeded,
        SetFallbackNeeded,
        SetSetupRequired,
        ErrorEvent,
        ClearError,
        Reset,
        Unknown
    };

    CommandType GetCommandType(const std::string & name)
    {
        if (name == "MoveTo") return CommandType::MoveTo;
        if (name == "Stop") return CommandType::Stop;
        if (name == "Calibrate") return CommandType::Calibrate;
        if (name == "ConfigureFallback") return CommandType::ConfigureFallback;
        if (name == "Protected") return CommandType::Protected;
        if (name == "UnProtected") return CommandType::Unprotected;
        if (name == "SetReadyToRun") return CommandType::SetReadyToRun;
        if (name == "SetActionNeeded") return CommandType::SetActionNeeded;
        if (name == "SetFallbackNeeded") return CommandType::SetFallbackNeeded;
        if (name == "SetSetupRequired") return CommandType::SetSetupRequired;
        if (name == "ErrorEvent") return CommandType::ErrorEvent;
        if (name == "ClearError") return CommandType::ClearError;
        if (name == "Reset") return CommandType::Reset;
        return CommandType::Unknown;
    }
}


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
    CommandType commandType = GetCommandType(name);

    VerifyOrReturn(!self->mJsonValue.empty(), {
        ChipLogError(NotSpecified, "Invalid JSON event command received");
        Platform::Delete(self);
    });

    switch (commandType)
    {
        case CommandType::MoveTo:
            self->MoveToStimuli(
                self->mJsonValue.isMember("Tag") ? std::make_optional(static_cast<uint8_t>(self->mJsonValue["Tag"].asUInt())) : std::nullopt,
                self->mJsonValue.isMember("Speed") ? std::make_optional(static_cast<uint8_t>(self->mJsonValue["Speed"].asUInt())) : std::nullopt,
                self->mJsonValue.isMember("Latch") ? std::make_optional(static_cast<uint8_t>(self->mJsonValue["Latch"].asUInt())) : std::nullopt);
            break;

        case CommandType::Stop:
            self->StopStimuli();
            break;

        case CommandType::Calibrate:
            self->CalibrateStimuli();
            break;

        case CommandType::ConfigureFallback:
            self->ConfigureFallbackStimuli(
                self->mJsonValue.isMember("RestingProcedure") ? std::make_optional(static_cast<uint8_t>(self->mJsonValue["RestingProcedure"].asUInt())) : std::nullopt,
                self->mJsonValue.isMember("TriggerCondition") ? std::make_optional(static_cast<uint8_t>(self->mJsonValue["TriggerCondition"].asUInt())) : std::nullopt,
                self->mJsonValue.isMember("TriggerPosition") ? std::make_optional(static_cast<uint8_t>(self->mJsonValue["TriggerPosition"].asUInt())) : std::nullopt,
                self->mJsonValue.isMember("WaitingDelay") ? std::make_optional(static_cast<uint16_t>(self->mJsonValue["WaitingDelay"].asUInt())) : std::nullopt);
            break;

        case CommandType::Protected:
            self->ProtectedStimuli();
            break;

        case CommandType::Unprotected:
            self->UnprotectedStimuli();
            break;

        case CommandType::SetReadyToRun:
        {
            bool readyToRun = self->mJsonValue["ReadyToRun"].asBool();
            self->ReadyToRunStimuli(readyToRun);
            break;
        }

        case CommandType::SetActionNeeded:
        {
            bool actionNeeded = self->mJsonValue["ActionNeeded"].asBool();
            self->ActionNeededStimuli(actionNeeded);
            break;
        }

        case CommandType::SetFallbackNeeded:
        {
            bool fallbackNeeded = self->mJsonValue["FallbackNeeded"].asBool();
            self->FallbackNeededStimuli(fallbackNeeded);
            break;
        }

        case CommandType::SetSetupRequired:
        {
            bool setupRequired = self->mJsonValue["SetupRequired"].asBool();
            self->SetupRequiredStimuli(setupRequired);
            break;
        }

        case CommandType::ErrorEvent:
            self->OnErrorEventStimuli(self->mJsonValue["Error"].asString());
            break;

        case CommandType::ClearError:
            self->OnClearErrorStimuli();
            break;

        case CommandType::Reset:
            self->OnResetHandler();
            break;

        default:
            ChipLogError(NotSpecified, "ClosuresAppDelegate: Unhandled command: %s", name.c_str());
            break;
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

void ClosuresAppCommandHandler::MoveToStimuli(std::optional<uint8_t> tag, std::optional<uint8_t> speed,
                                              std::optional<uint8_t> latch)
{
    mClosuresDevice->ClosuresMoveToStimuli(tag, latch, speed);
}

void ClosuresAppCommandHandler::StopStimuli()
{
    mClosuresDevice->ClosuresStopStimuli();
}

void ClosuresAppCommandHandler::CalibrateStimuli()
{
    mClosuresDevice->ClosuresCalibrateStimuli();
}

void ClosuresAppCommandHandler::ConfigureFallbackStimuli(std::optional<uint8_t> restingProcedure, std::optional<uint8_t> triggerCondition, std::optional<uint8_t> triggerPosition, std::optional<uint16_t> waitingDelay)
{
    mClosuresDevice->ClosuresConfigureFallbackStimuli(restingProcedure, triggerCondition, triggerPosition, waitingDelay);
}

void ClosuresAppCommandHandler::ProtectedStimuli()
{
    mClosuresDevice->ClosuresProtectedStimuli();
}

void ClosuresAppCommandHandler::UnprotectedStimuli()
{
    mClosuresDevice->ClosuresUnprotectedStimuli();
}

void ClosuresAppCommandHandler::ReadyToRunStimuli(bool aReady)
{
    mClosuresDevice->ClosuresReadyToRunStimuli(aReady);
}

void ClosuresAppCommandHandler::ActionNeededStimuli(bool aActionNeeded)
{
    mClosuresDevice->ClosuresActionNeededStimuli(aActionNeeded);
}

void ClosuresAppCommandHandler::FallbackNeededStimuli(bool aFallbackNeeded)
{
    mClosuresDevice->ClosuresFallbackNeededStimuli(aFallbackNeeded);
}

void ClosuresAppCommandHandler::SetupRequiredStimuli(bool aSetupRequired)
{
    mClosuresDevice->ClosuresSetupRequiredStimuli(aSetupRequired);
}

void ClosuresAppCommandHandler::OnErrorEventStimuli(const std::string & error)
{
    mClosuresDevice->HandleErrorEvent(error);
}

void ClosuresAppCommandHandler::OnClearErrorStimuli()
{
    mClosuresDevice->HandleClearErrorMessage();
}

void ClosuresAppCommandHandler::OnResetHandler()
{
    mClosuresDevice->HandleResetStimuli();
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
    DeviceLayer::PlatformMgr().ScheduleWork(ClosuresAppCommandHandler::HandleCommand, reinterpret_cast<intptr_t>(handler));
}
