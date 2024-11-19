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

#include <app-common/zap-generated/cluster-objects.h>
#include <app/CommandHandlerInterface.h>
#include <app/util/util.h>
#include <utility>

namespace chip {
namespace app {
namespace Clusters {
namespace ClosureDimension {

struct Info {
    ClusterId cId;
    const char * name;
};

static constexpr std::array<Info, 5> const AliasedClusters = {{ 
                                                            { Closure1stDimension::Id, "1st" },
                                                            { Closure2ndDimension::Id, "2nd" },
                                                            { Closure3rdDimension::Id, "3rd" },
                                                            { Closure4thDimension::Id, "4th" },
                                                            { Closure5thDimension::Id, "5th" },
                                                       }};

// Redeclare alias toward the first Cluster dimension for common DataType
/* Enum, BitMap, Structure */
using UnitEnum = Closure1stDimension::UnitEnum; //--> Clusters::detail::UnitEnum;
using LatchingEnum = Closure1stDimension::LatchingEnum;
using NLatchingEnum = Closure1stDimension::Attributes::CurrentLatching::TypeInfo::Type; // Nullable
using TranslationDirectionEnum = Closure1stDimension::TranslationDirectionEnum;
using RotationAxisEnum = Closure1stDimension::RotationAxisEnum;
using ModulationTypeEnum = Closure1stDimension::ModulationTypeEnum;
using LatchingAxisEnum = Closure1stDimension::LatchingAxisEnum;
using OverFlowEnum = Closure1stDimension::OverFlowEnum;
using SignedValuesRangeStruct = Closure1stDimension::Structs::SignedValuesRangeStruct::Type;
using RangePercent100thsStruct = Closure1stDimension::Structs::RangePercent100thsStruct::Type;
using NRangePercent100thsStruct = Closure1stDimension::Attributes::LimitRange::TypeInfo::Type; // Nullable
using PositioningBitmap = chip::app::Clusters::Closure1stDimension::PositioningBitmap;
using BitMaskPositioningBitmap = chip::BitMask<chip::app::Clusters::Closure1stDimension::PositioningBitmap>;
using NBitMaskPositioningBitmap = Closure1stDimension::Attributes::CurrentPositioning::TypeInfo::Type; // Nullable

using Feature = Closure1stDimension::Feature;
using TXXT = chip::app::DataModel::Nullable<chip::BitMask<chip::app::Clusters::Closure1stDimension::PositioningBitmap>>; ///using PositioningBitmap = Clusters::detail::PositioningBitmap;
//chip::app::DataModel::Nullable<chip::BitMask<chip::app::Clusters::Closure4thDimension::PositioningBitmap>>;
//using SignedValuesRangeStruct = Closure1stDimension::Structs::SignedValuesRangeStruct;
// enum class Feature : uint32_t
// {
//     kNumericMeasurement = 0x1,
//     kLevelIndication    = 0x2,
//     kMediumLevel        = 0x4,
//     kCriticalLevel      = 0x8,
//     kPeakMeasurement    = 0x10,
//     kAverageMeasurement = 0x20,
// };

/* Attributes::Id */
namespace Attributes {

namespace CurrentPositioning {
static constexpr AttributeId Id = Closure1stDimension::Attributes::CurrentPositioning::Id;
} // namespace CurrentPositioning

namespace TargetPositioning {
static constexpr AttributeId Id = Closure1stDimension::Attributes::TargetPositioning::Id;
} // namespace TargetPositioning

namespace Resolution {
static constexpr AttributeId Id = Closure1stDimension::Attributes::Resolution::Id;
} // namespace Resolution

namespace StepValue {
static constexpr AttributeId Id = Closure1stDimension::Attributes::StepValue::Id;
} // namespace StepValue

namespace Unit {
static constexpr AttributeId Id = Closure1stDimension::Attributes::Unit::Id;
} // namespace Unit

namespace UnitRange {
static constexpr AttributeId Id = Closure1stDimension::Attributes::UnitRange::Id;
} // namespace UnitRange

namespace LimitRange {
static constexpr AttributeId Id = Closure1stDimension::Attributes::LimitRange::Id;
} // namespace LimitRange

namespace TranslationDirection {
static constexpr AttributeId Id = Closure1stDimension::Attributes::TranslationDirection::Id;
} // namespace TranslationDirection

namespace RotationAxis {
static constexpr AttributeId Id = Closure1stDimension::Attributes::RotationAxis::Id;
} // namespace RotationAxis

namespace OverFlow {
static constexpr AttributeId Id = Closure1stDimension::Attributes::OverFlow::Id;
} // namespace OverFlow

namespace ModulationType {
static constexpr AttributeId Id = Closure1stDimension::Attributes::ModulationType::Id;
} // namespace ModulationType

namespace CurrentLatching {
static constexpr AttributeId Id = Closure1stDimension::Attributes::CurrentLatching::Id;
} // namespace CurrentLatching

namespace TargetLatching {
static constexpr AttributeId Id = Closure1stDimension::Attributes::TargetLatching::Id;
} // namespace TargetLatching

namespace LatchingAxis {
static constexpr AttributeId Id = Closure1stDimension::Attributes::LatchingAxis::Id;
} // namespace LatchingAxis

namespace GeneratedCommandList {
static constexpr AttributeId Id = Globals::Attributes::GeneratedCommandList::Id;
} // namespace GeneratedCommandList

namespace AcceptedCommandList {
static constexpr AttributeId Id = Globals::Attributes::AcceptedCommandList::Id;
} // namespace AcceptedCommandList

namespace AttributeList {
static constexpr AttributeId Id = Globals::Attributes::AttributeList::Id;
} // namespace AttributeList

namespace FeatureMap {
static constexpr AttributeId Id = Globals::Attributes::FeatureMap::Id;
} // namespace FeatureMap

namespace ClusterRevision {
static constexpr AttributeId Id = Globals::Attributes::ClusterRevision::Id;
} // namespace ClusterRevision

} // namespace Attributes

/* Commands::Id */
namespace Commands {

namespace Steps {
static constexpr CommandId Id = Closure1stDimension::Commands::Steps::Id;
using DecodableType = Closure1stDimension::Commands::Steps::DecodableType;
} // namespace Steps

namespace Latch {
static constexpr CommandId Id = Closure1stDimension::Commands::Latch::Id;
using DecodableType = Closure1stDimension::Commands::Latch::DecodableType;
} // namespace Latch

namespace UnLatch {
static constexpr CommandId Id = Closure1stDimension::Commands::UnLatch::Id;
using DecodableType = Closure1stDimension::Commands::UnLatch::DecodableType;
} // namespace UnLatch

} // namespace Commands

} // namespace ClosureDimension
} // namespace Clusters
} // namespace app
} // namespace chip
