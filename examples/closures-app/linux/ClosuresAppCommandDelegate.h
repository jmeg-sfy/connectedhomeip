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

#pragma once

#include "NamedPipeCommands.h"
#include "closures-device.h"
#include <json/json.h>
#include <platform/DiagnosticDataProvider.h>

#include <string>

namespace chip {
namespace app {
namespace Clusters {

class ClosuresAppCommandHandler
{
public:
    static ClosuresAppCommandHandler * FromJSON(const char * json);

    static void HandleCommand(intptr_t context);

    ClosuresAppCommandHandler(Json::Value && jasonValue) : mJsonValue(std::move(jasonValue)) {}

    void SetClosuresDevice(chip::app::Clusters::ClosuresDevice * aClosuresDevice);

private:
    Json::Value mJsonValue;
    ClosuresDevice * mClosuresDevice;

    /**
     * Should be called to notify that the device has finished calibration.
     */
    void OnCalibrationEndedHandler();

    void OnEngagedHandler();

    void OnDisengagedHandler();

    void OnProtectionRisedHandler();

    void OnProtectionDroppedHandler();

    void OnMovementCompleteHandler();

    void MoveToStimuli();

    void StopStimuli();

    void OnDownCloseHandler();

    void OnErrorEventHandler(const std::string & error);

    void OnClearErrorHandler();

    void OnResetHandler();
};

class ClosuresAppCommandDelegate : public NamedPipeCommandDelegate
{
private:
    ClosuresDevice * mClosuresDevice;

public:
    void SetClosuresDevice(ClosuresDevice * aClosuresDevice);
    void OnEventCommandReceived(const char * json) override;
};

} // namespace Clusters
} // namespace app
} // namespace chip
