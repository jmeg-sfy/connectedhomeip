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
#include "closures-device.h"
#include <AppMain.h>

#include <string>


/* imgui support */
#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
#include <imgui_ui/ui.h>
#include <imgui_ui/windows/qrcode.h>
#endif /* CHIP_IMGUI_ENABLED */

#define CLOSURE_ENDPOINT 1

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

namespace {
constexpr char kChipEventFifoPathPrefix[] = "/tmp/chip_closure_fifo_";
NamedPipeCommands sChipNamedPipeCommands;
ClosuresAppCommandDelegate sClosuresAppCommandDelegate;
} // namespace

ClosuresDevice * gClosuresDevice = nullptr;

#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
    example::Ui::ImguiUi gUI;
#endif

void ApplicationInit()
{
    std::string path = kChipEventFifoPathPrefix + std::to_string(getpid());

    ChipLogDetail(NotSpecified, "ApplicationInit()");
    ChipLogDetail(NotSpecified, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! PIPE PID: %d", getpid());


    if (sChipNamedPipeCommands.Start(path, &sClosuresAppCommandDelegate) != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "Failed to start CHIP NamedPipeCommands");
        sChipNamedPipeCommands.Stop();
    }

    gClosuresDevice = new ClosuresDevice(CLOSURE_ENDPOINT);
    // Provide an Instance of ImGui
#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
    gClosuresDevice->AddImGuiInstance(&example::Ui::ImguiUi::AddWindow, &gUI);
#endif
    gClosuresDevice->Init();

    sClosuresAppCommandDelegate.SetClosuresDevice(gClosuresDevice);
    // Register the device as an observer
    gClosuresDevice->AddOperationalStateObserver(gClosuresDevice);
    


}

void ApplicationShutdown()
{
    delete gClosuresDevice;
    gClosuresDevice = nullptr;

    sChipNamedPipeCommands.Stop();
}

int main(int argc, char * argv[])
{
    if (ChipLinuxAppInit(argc, argv) != 0)
    {
        return -1;
    }

#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
    gUI.AddWindow(std::make_unique<example::Ui::Windows::QRCode>());
    ChipLinuxAppMainLoop(&gUI);
#else
    ChipLinuxAppMainLoop();
#endif

    return 0;
}
