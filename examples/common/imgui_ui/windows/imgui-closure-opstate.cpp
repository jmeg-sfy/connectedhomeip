/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
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

#include "imgui-closure-opstate.h"

#include <imgui.h>

#include <math.h>

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-enums.h>

//#include <app/clusters/level-control/level-control.h>
//#include <app/clusters/closure-dimension-server/closure-dimension-server.h>
// #include <app/clusters/level-control/level-control.h>
//#include <app/clusters/on-off-server/on-off-server.h>//
#include <app/clusters/operational-state-server/operational-state-server.h>
//#include <app/clusters/closure-dimension-server/closure-dimension-server.h>
//MatterDiagnosticLogsPluginServerInitCallback


namespace example {
namespace Ui {
namespace Windows {
namespace {

using namespace chip::app::Clusters;
using chip::app::DataModel::Nullable;

} // namespace

// static const ImColor kRed   = ImColor(255, 0, 0, 255);
// static const ImColor kGreen = ImColor(0, 255, 0, 255);
// static const ImColor kBlue  = ImColor(0, 0, 255, 255);

// /* ANSI Colored escape code */
// #define CL_CLEAR  "\x1b[0m"
// #define CL_RED    "\u001b[31m"
// #define CL_GREEN  "\u001b[32m"
// #define CL_YELLOW "\u001b[33m"

// static constexpr char strLogY[] = CL_GREEN "Y" CL_CLEAR;
// static constexpr char strLogN[] = CL_RED "N" CL_CLEAR;

// const char * StrYN(bool isTrue)
// {
//     return isTrue ? strLogY : strLogN;
// }

// #define IsYN(TEST)  StrYN(TEST)

// static void DisplayColoredYesNoText(bool isEnabled, const char * text)
// {
//     ImGui::PushTextWrapPos( 0 );
//     ImGui::Text(" %-20s [", text);
//     ImGui::SameLine();
//     isEnabled ? ImGui::TextColored(kGreen, "Y" ) : ImGui::TextColored(kRed, "N");
//     ImGui::SameLine();
//     ImGui::TextUnformatted("]");
//     ImGui::PopTextWrapPos();
// }

// // static void DisplayFeature(const uint32_t & featureMap, Operation::Feature aFeature)
// // {
// //     const chip::BitMask<ClosureDimension::Feature> value = featureMap;
// //     DisplayColoredYesNoText(value.Has(aFeature), ClosureDimension::GetFeatureName(aFeature));
// // }

// static void DisplayFeatures(const uint32_t & featureMap)
// {
//     //const chip::BitMask<ClosureDimension::Feature> value = featureMap;

//     //ImGui::Text("FeatureMap=0x%08X (%u)", value.Raw(), value.Raw());

//     //DisplayFeature(featureMap, ClosureDimension::Feature::kPositioning);
//     //DisplayFeature(featureMap, ClosureDimension::Feature::kLatching);
//     //DisplayFeature(featureMap, ClosureDimension::Feature::kUnit);
//     //DisplayFeature(featureMap, ClosureDimension::Feature::kLimitation);
//     //DisplayFeature(featureMap, ClosureDimension::Feature::kSpeed);
//     //DisplayFeature(featureMap, ClosureDimension::Feature::kTranslation);
//     //DisplayFeature(featureMap, ClosureDimension::Feature::kRotation);
//     //DisplayFeature(featureMap, ClosureDimension::Feature::kModulation);
// }

void ImGuiClosureOpState::Render()
{
    ImGui::Begin("Closure OpState Cluster");
    ImGui::Separator();
    ImGui::Text("Closure OpState Cluster Ep[%d] ", mEndpointId);

    ImGui::Indent();
    {
       //DisplayFeatures(mFeature);
    }
    ImGui::Unindent();

    ImGui::End();
}

void ImGuiClosureOpState::UpdateState()
{
    // Fill update of attributes here
    // Get / Set
}

} // namespace Windows
} // namespace Ui
} // namespace example
