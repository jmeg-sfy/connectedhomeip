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
#pragma once

#include "window.h"

#include <stdint.h>

#include <app/data-model/Nullable.h>
#include <lib/core/DataModelTypes.h>
#include <lib/core/Optional.h>

#include <app-common/zap-generated/cluster-enums.h>

//typedef const char * (GetFeatureMapString(Feature aFeature))
//typedef void (ClosuresDevice::*HandleOpStateCommand)(Clusters::OperationalState::GenericOperationalError & err);

namespace example {
namespace Ui {
namespace Windows {

/**
 * Assumes ClosureDimension on a given endpoint with FULL support (on/off, level and color)
 */
class ImGuiClosureDimension : public Window
{
public:
    ImGuiClosureDimension(chip::EndpointId aEndpointId, const char * aName, uint32_t aFeature) : 
    mEndpointId(aEndpointId), mName(aName), mFeature(aFeature) {}

    void UpdateState() override;
    void Render() override;

private:
    const chip::EndpointId mEndpointId;
    const char * mName;
    uint32_t mFeature;
};

} // namespace Windows
} // namespace Ui
} // namespace example
