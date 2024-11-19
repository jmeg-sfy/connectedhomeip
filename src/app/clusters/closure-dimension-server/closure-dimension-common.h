/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
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

#include "closure-dimension-cluster-objects.h"

// #include <app/AttributeAccessInterface.h>
// #include <app/AttributeAccessInterfaceRegistry.h>
// #include <app/CommandHandlerInterfaceRegistry.h>
// #include <app/data-model/Nullable.h>
// #include <app/reporting/reporting.h>
// #include <app/util/attribute-storage.h>
// #include <lib/support/IntrusiveList.h>
// #include <type_traits>

namespace chip {
namespace app {
namespace Clusters {
namespace ClosureDimension {

const char * GetFeatureName(Feature aFeature);

// Instance::Instance(EndpointId aEndpointId, ClusterId aClusterId,
//         RotationAxisEnum aRotationAxis,
//         OverFlowEnum aOverFlow,
//         ModulationTypeEnum aModulation,
//         LatchingAxisEnum aLatchingAxis,
//         TranslationDirectionEnum aTranslationDirection);


} // namespace ClosureDimension
} // namespace Clusters
} // namespace app
} // namespace chip
