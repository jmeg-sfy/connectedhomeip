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

#include "closure-dimension-cluster-objects.h"
#include <app/AttributeAccessInterface.h>
#include <app/AttributeAccessInterfaceRegistry.h>
#include <app/data-model/Nullable.h>
#include <app/reporting/reporting.h>
#include <app/util/attribute-storage.h>
#include <lib/support/IntrusiveList.h>
#include <type_traits>

namespace chip {
namespace app {
namespace Clusters {
namespace ClosureDimension {

namespace Detail {

struct DummyPositioningMembers
{
};

struct DummyUnitMembers
{
};

struct DummySpeedMembers
{
};

struct DummyLatchingMembers
{
};

struct DummyLatchingOnlyMembers
{
};

struct DummyLimitationMembers
{
};

struct DummyRotationMembers
{
};

struct DummyTranslationMembers
{
};

struct DummyModulationMembers
{
};

struct DummyRotationOrLatchingMembers
{
};

class PositioningMembers
{
protected:
    using Type = chip::app::DataModel::Nullable<chip::BitMask<chip::app::Clusters::Closure4thDimension::PositioningBitmap>>;

    NBitMaskPositioningBitmap mCurrentPositioning;
    NBitMaskPositioningBitmap mTargetPositioning;
    Percent100ths mResolution;
    Percent100ths mStepValue;
};

class LatchingMembers
{
protected:
    NLatchingEnum mCurrentLatching;
    NLatchingEnum mTargetLatching;
};

class LatchingOnlyMembers
{
protected:
    LatchingAxisEnum mLatchingAxis;
};

class UnitMembers
{
protected:
    UnitEnum mUnit;
    SignedValuesRangeStruct mRange;
};

class SpeedMembers
{
protected:
};

class LimitationMembers
{
protected:
    NRangePercent100thsStruct mLimitRange;
};

class RotationMembers
{
protected:
    RotationAxisEnum mRotationAxis;
};

class TranslationMembers
{
protected:
    TranslationDirectionEnum mTranslationDirection;
};

class ModulationMembers
{
protected:
    ModulationTypeEnum mModulationType;
};

class RotationOrLatchingMembers
{
protected:
    OverFlowEnum mOverFlow;
};

} // namespace Detail

/**
 * This class provides the base implementation for the server side of the Concentration Measurement cluster as well as an API for
 * setting the values of the attributes.
 *
 * @tparam FeaturePositioningEnabled whether the cluster supports numeric measurement
 * @tparam FeatureLatchingEnabled whether the cluster supports level indication
 * @tparam FeatureRotationEnabled whether the Level Indication Feature supports medium level
 * @tparam FeatureTranslationEnabled whether the Level Indication Feature supports critical level
 * @tparam FeatureUnitEnabled whether the Numeric Measurement Feature supports peak measurement
 * @tparam FeatureSpeedEnabled whether the Numeric Measurement Feature supports average measurement
 */
template <bool FeaturePositioningEnabled,
          bool FeatureLatchingEnabled,
          bool FeatureUnitEnabled,
          bool FeatureSpeedEnabled,
          bool FeatureLimitationEnabled,
          bool FeatureRotationEnabled,
          bool FeatureTranslationEnabled,
          bool FeatureModulationEnabled>
class Instance
    : public AttributeAccessInterface,
      protected std::conditional_t<FeaturePositioningEnabled, Detail::PositioningMembers, Detail::DummyPositioningMembers>,
      protected std::conditional_t<FeatureLatchingEnabled   , Detail::LatchingMembers   , Detail::DummyLatchingMembers>,
      protected std::conditional_t<FeatureUnitEnabled       , Detail::UnitMembers       , Detail::DummyUnitMembers>,
      protected std::conditional_t<FeatureSpeedEnabled      , Detail::SpeedMembers      , Detail::DummySpeedMembers>,
      protected std::conditional_t<FeatureLimitationEnabled , Detail::LimitationMembers , Detail::DummyLimitationMembers>,
      protected std::conditional_t<FeatureRotationEnabled   , Detail::RotationMembers   , Detail::DummyRotationMembers>,
      protected std::conditional_t<FeatureTranslationEnabled, Detail::TranslationMembers, Detail::DummyTranslationMembers>,
      protected std::conditional_t<FeatureModulationEnabled , Detail::ModulationMembers , Detail::DummyModulationMembers>,
      protected std::conditional_t<FeatureLatchingEnabled && !FeaturePositioningEnabled,
          Detail::LatchingOnlyMembers      , Detail::DummyLatchingOnlyMembers>,
      protected std::conditional_t<FeatureRotationEnabled || FeatureLatchingEnabled,
          Detail::RotationOrLatchingMembers, Detail::DummyRotationOrLatchingMembers>
{
private:
    static const int WINDOW_MAX = 604800;

    EndpointId mEndpointId{};
    ClusterId mClusterId{};

    Percent100ths mResolution;

    uint32_t mFeatureMap = 0;



// // This function is called by the interaction model engine when a command destined for this instance is received.
// void Instance::InvokeCommand(HandlerContext & handlerContext)
// {
//     ChipLogDetail(Zcl, "OperationalState: InvokeCommand");
//     switch (handlerContext.mRequestPath.mCommandId)
//     {

//     case Commands::Stop::Id:
//         ChipLogDetail(Zcl, "OperationalState: Entering handling Stop state");

//         HandleCommand<Commands::Stop::DecodableType>(
//             handlerContext, [this](HandlerContext & ctx, const auto & req) { HandleStopState(ctx, req); });
//         break;
//     default:
//         ChipLogDetail(Zcl, "OperationalState: Entering handling derived cluster commands");
//         InvokeDerivedClusterCommand(handlerContext);
//         break;
//     }
// }




    // AttributeAccessInterface
    CHIP_ERROR Read(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder) override
    {
        bool isAttributeSupported = false;
        switch (aPath.mAttributeId)
        {
        /* Positioning feature Attributes */
        case Attributes::CurrentPositioning::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::CurrentPositioning", GetClusterName());
            if constexpr (FeaturePositioningEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mCurrentPositioning));
                isAttributeSupported = true;
            }
            break;

        case Attributes::TargetPositioning::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::TargetPositioning", GetClusterName());
            if constexpr (FeaturePositioningEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mTargetPositioning));
                isAttributeSupported = true;
            }
            break;

        case Attributes::Resolution::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::Resolution", GetClusterName());
            if constexpr (FeaturePositioningEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mResolution));
                isAttributeSupported = true;
            }
            break;

        case Attributes::StepValue::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::StepValue", GetClusterName());
            if constexpr (FeaturePositioningEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mStepValue));
                isAttributeSupported = true;
            }
            break;

        /* Unit feature Attributes */
        case Attributes::Unit::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::Unit", GetClusterName());
            if constexpr (FeatureUnitEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mUnit));
                isAttributeSupported = true;
            }
            break;

        case Attributes::Range::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::Range", GetClusterName());
            if constexpr (FeatureUnitEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mRange));
                isAttributeSupported = true;
            }
            break;

        /* Limitation feature Attributes */
        case Attributes::LimitRange::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::LimitRange", GetClusterName());
            if constexpr (FeatureLimitationEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mLimitRange));
                isAttributeSupported = true;
            }
            break;

        case Attributes::TranslationDirection::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::TranslationDirection", GetClusterName());
            if constexpr (FeatureTranslationEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mTranslationDirection));
                isAttributeSupported = true;
            }
            break;

        case Attributes::RotationAxis::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::RotationAxis", GetClusterName());
            if constexpr (FeatureRotationEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mRotationAxis));
                isAttributeSupported = true;
            }
            break;

        case Attributes::OverFlow::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::OverFlow", GetClusterName());
            if constexpr (FeatureRotationEnabled || FeatureLatchingEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mOverFlow));
                isAttributeSupported = true;
            }
            break;

        case Attributes::ModulationType::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::ModulationType", GetClusterName());
            if constexpr (FeatureModulationEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mModulationType));
                isAttributeSupported = true;
            }
            break;

        case Attributes::LatchingAxis::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::LatchingAxis", GetClusterName());
            if constexpr (FeatureLatchingEnabled && !FeaturePositioningEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mLatchingAxis));
                isAttributeSupported = true;
            }
            break;

        /* Latching feature Attributes */
        case Attributes::CurrentLatching::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::CurrentLatching", GetClusterName());
            if constexpr (FeatureLatchingEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mCurrentLatching));
                isAttributeSupported = true;
            }
            break;

        case Attributes::TargetLatching::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::TargetLatching", GetClusterName());
            if constexpr (FeatureLatchingEnabled)
            {
                ReturnErrorOnFailure(aEncoder.Encode(this->mTargetLatching));
                isAttributeSupported = true;
            }
            break;

        case Attributes::FeatureMap::Id:
            ChipLogDetail(Zcl, "%s ClDim::Read::FeatureMap", GetClusterName());
            LogFeatureMap(mFeatureMap);
            ReturnErrorOnFailure(aEncoder.Encode(mFeatureMap));
            isAttributeSupported = true;
            break;
        }

        return isAttributeSupported ? CHIP_NO_ERROR : CHIP_IM_GLOBAL_STATUS(UnsupportedAttribute);
    };

    /**
     * This checks if the clusters instance is a valid ResourceMonitoring cluster based on the AliasedClusters list.
     * @return true if the cluster is a valid ResourceMonitoring cluster.
     */
    bool IsValidAliasCluster() const
    {
        for (Info AliasedCluster : AliasedClusters)
        {
            if (mClusterId == AliasedCluster.cId)
            {
                return true;
            }
        }
        ChipLogDetail(Zcl, "INVALID CLUSTER");
        return false;
    };

    const char * GetClusterName() const
    {
        for (Info AliasedCluster : AliasedClusters)
        {
            if (mClusterId == AliasedCluster.cId)
            {
                return AliasedCluster.name;
            }
        }
        ChipLogDetail(Zcl, "INVALID CLUSTER");
        return nullptr;
    };

    /**
     * This generates a feature bitmap from the enabled features and then returns its raw value.
     * @return The raw feature bitmap.
     */
    uint32_t GenerateFeatureMap() const
    {
        BitMask<Feature, uint32_t> featureMap(0);

        if constexpr (FeaturePositioningEnabled)
        {
            featureMap.Set(Feature::kPositioning);
        }

        if constexpr (FeatureLatchingEnabled)
        {
            featureMap.Set(Feature::kLatching);
        }

        if constexpr (FeatureRotationEnabled)
        {
            featureMap.Set(Feature::kRotation);
        }

        if constexpr (FeatureTranslationEnabled)
        {
            featureMap.Set(Feature::kTranslation);
        }

        if constexpr (FeatureUnitEnabled)
        {
            featureMap.Set(Feature::kUnit);
        }

        if constexpr (FeatureSpeedEnabled)
        {
            featureMap.Set(Feature::kSpeed);
        }

        if constexpr (FeatureModulationEnabled)
        {
            featureMap.Set(Feature::kModulation);
        }

        if constexpr (FeatureLimitationEnabled)
        {
            featureMap.Set(Feature::kLimitation);
        }

        return featureMap.Raw();
    };

    /**
     * This checks if a given nullable float is within the min and max constraints or two other nullable floats.
     * @param value The value to check.
     * @param minValue The minimum value.
     * @param maxValue The maximum value.
     * @return true if the value is within the min and max constraints. If either of the pair of values being compared is null,
     * that's considered to be within the constraint.
     */
    static bool CheckConstraintMinMax(DataModel::Nullable<float> value, DataModel::Nullable<float> minValue,
                                      DataModel::Nullable<float> maxValue)
    {
        return (minValue.IsNull() || value.IsNull() || (value.Value() >= minValue.Value())) &&
            (maxValue.IsNull() || value.IsNull() || (value.Value() <= maxValue.Value()));
    };

    /**
     * This checks if a given nullable float is less than or equal to another given nullable float.
     * @param value The value to check.
     * @param valueToBeLessThanOrEqualTo The value to be less than or equal to.
     * @return true if value is less than or equal to valueToBeLessThanOrEqualTo, or if either of the values is Null.
     */
    static bool CheckConstraintsLessThanOrEqualTo(DataModel::Nullable<float> value,
                                                  DataModel::Nullable<float> valueToBeLessThanOrEqualTo)
    {
        return valueToBeLessThanOrEqualTo.IsNull() || value.IsNull() || (value.Value() <= valueToBeLessThanOrEqualTo.Value());
    };

    /**
     * This checks if a given nullable float is greater than or equal to another given nullable float.
     * @param value The value to check.
     * @param valueToBeGreaterThanOrEqualTo The value to be greater than or equal to.
     * @return true if value is greater than or equal to valueToBeGreaterThanOrEqualTo, or if either of the values is Null.
     */
    static bool CheckConstraintsGreaterThanOrEqualTo(DataModel::Nullable<float> value,
                                                     DataModel::Nullable<float> valueToBeGreaterThanOrEqualTo)
    {
        return valueToBeGreaterThanOrEqualTo.IsNull() || value.IsNull() || (value.Value() >= valueToBeGreaterThanOrEqualTo.Value());
    };

public:
    /**
     * Creates a mode base cluster instance. The Init() function needs to be called for this instance to be registered and
     * called by the interaction model at the appropriate times.
     * This constructor should be used when not using the kNumericMeasurement feature.
     * @param aEndpointId The endpoint on which this cluster exists. This must match the zap configuration.
     * @param aClusterId The ID of the ModeBase aliased cluster to be instantiated.
     * @param aResolution The measurement medium.
     */
    Instance(EndpointId aEndpointId, ClusterId aClusterId, Percent100ths aResolution) :
        AttributeAccessInterface(Optional<EndpointId>(aEndpointId), aClusterId), mEndpointId(aEndpointId), mClusterId(aClusterId),
        mResolution(aResolution){};

    /**
     * Creates a mode base cluster instance. The Init() function needs to be called for this instance to be registered and
     * called by the interaction model at the appropriate times.
     * This constructor should be used when using the kNumericMeasurement feature.
     * @param aEndpointId The endpoint on which this cluster exists. This must match the zap configuration.
     * @param aClusterId The ID of the ModeBase aliased cluster to be instantiated.
     * @param aResolution The measurement medium.
     * @param aUnit The measurement unit.
     */
    Instance(EndpointId aEndpointId, ClusterId aClusterId, Percent100ths aResolution,
             UnitEnum aUnit) :
        AttributeAccessInterface(Optional<EndpointId>(aEndpointId), aClusterId),
        mEndpointId(aEndpointId), mClusterId(aClusterId), mResolution(aResolution)
    {
        //this->mUnit = aUnit;
        ChipLogDetail(Zcl, "ClDim Instance.Constructor() w Features: ID=0x%02lX EP=%u", long(mClusterId), mEndpointId);
        LogFeatureMap(GenerateFeatureMap());
    };

    ~Instance() override { AttributeAccessInterfaceRegistry::Instance().Unregister(this); };


/* ANSI Colored escape code */
#define CL_CLEAR  "\x1b[0m"
#define CL_RED    "\u001b[31m"
#define CL_GREEN  "\u001b[32m"
#define CL_YELLOW "\u001b[33m"

static constexpr char strLogY[] = CL_GREEN "Y" CL_CLEAR;
static constexpr char strLogN[] = CL_RED "N" CL_CLEAR;

const char * StrYN(bool isTrue)
{
    return isTrue ? strLogY : strLogN;
}

#define IsYN(TEST)  StrYN(TEST)

const char * GetFeatureMapString(Feature aFeature)
{
    switch (aFeature)
    {
    case Feature::kPositioning:
        return "Positioning";
    case Feature::kLatching:
        return "Latching";
    case Feature::kUnit:
        return "Unit";
    case Feature::kSpeed:
        return "Speed";
    case Feature::kLimitation:
        return "Limitation";
    case Feature::kRotation:
        return "Rotation";
    case Feature::kTranslation:
        return "Translation";
    case Feature::kModulation:
        return "Modulation";
    default:
        return "Unknown";
    }
}


    inline void LogIsFeatureSupported(const uint32_t & featureMap, Feature aFeature)
    {
        const BitMask<Feature> value = featureMap;
        ChipLogDetail(NotSpecified, " %-20s [%s]", GetFeatureMapString(aFeature), IsYN(value.Has(aFeature)));
    }

    void LogFeatureMap(const uint32_t & featureMap)
    {
        const BitMask<Feature> value = featureMap;

        ChipLogDetail(NotSpecified, "%s ClDim::FeatureMap=0x%08X (%u)", GetClusterName(), value.Raw(), value.Raw());

        LogIsFeatureSupported(featureMap, Feature::kPositioning);
        LogIsFeatureSupported(featureMap, Feature::kLatching);
        LogIsFeatureSupported(featureMap, Feature::kUnit);
        LogIsFeatureSupported(featureMap, Feature::kSpeed);
        LogIsFeatureSupported(featureMap, Feature::kUnit);
        LogIsFeatureSupported(featureMap, Feature::kLimitation);
        LogIsFeatureSupported(featureMap, Feature::kRotation);
        LogIsFeatureSupported(featureMap, Feature::kTranslation);
        LogIsFeatureSupported(featureMap, Feature::kModulation);
    }

    void LogPositioning(const BitMaskPositioningBitmap & opStatus)
    {
        ChipLogProgress(Zcl, "PositioningBitmap raw=0x%02X position=%u speed=%u", opStatus.Raw(),
                    opStatus.GetField(PositioningBitmap::kPosition),
                    opStatus.GetField(PositioningBitmap::kSpeed));
    }


    CHIP_ERROR Init()
    {
        // Conformance for Feature prechecked

        ChipLogDetail(NotSpecified, "ClDim INIT");

        static_assert( FeaturePositioningEnabled ||  FeatureLatchingEnabled   , "Feature: At least one of Positioning and/or Latching should be enabled");
        static_assert( FeaturePositioningEnabled || !FeatureUnitEnabled       , "Feature: Unit        requires Positioning to be true");
        static_assert( FeaturePositioningEnabled || !FeatureSpeedEnabled      , "Feature: Speed       requires Positioning to be true");
        static_assert( FeaturePositioningEnabled || !FeatureLimitationEnabled , "Feature: Limitation  requires Positioning to be true");
        static_assert( FeaturePositioningEnabled || !FeatureRotationEnabled   , "Feature: Rotation    requires Positioning to be true");
        static_assert( FeaturePositioningEnabled || !FeatureTranslationEnabled, "Feature: Translation requires Positioning to be true");
        static_assert( FeaturePositioningEnabled || !FeatureModulationEnabled , "Feature: Modulation  requires Positioning to be true");
        static_assert( FeatureModulationEnabled  || !FeatureRotationEnabled   || !FeatureTranslationEnabled, "Features: RO vs TR, exclusivity unmet");
        static_assert( FeatureTranslationEnabled || !FeatureRotationEnabled   || !FeatureModulationEnabled , "Features: RO vs MD, exclusivity unmet");
        static_assert( FeatureRotationEnabled    || !FeatureModulationEnabled || !FeatureTranslationEnabled, "Features: MD vs TR, exclusivity unmet");
        static_assert(!FeatureTranslationEnabled ||  FeatureRotationEnabled   ||  FeatureModulationEnabled || FeatureTranslationEnabled, "Features: at least one among MD/RO/TR");
        static_assert(!FeatureRotationEnabled    || !FeatureModulationEnabled || !FeatureTranslationEnabled, "Features: MD vs TR vs RO, exclusivity unmet");

        VerifyOrReturnError(IsValidAliasCluster(), CHIP_ERROR_INCORRECT_STATE);
        ChipLogDetail(NotSpecified, "ClDim INIT B");

        // Check if the cluster has been selected in zap
        VerifyOrReturnError(emberAfContainsServer(mEndpointId, mClusterId), CHIP_ERROR_INCORRECT_STATE);
        ChipLogDetail(NotSpecified, "ClDim INIT C");

        // Register the object as attribute provider
        VerifyOrReturnError(AttributeAccessInterfaceRegistry::Instance().Register(this), CHIP_ERROR_INCORRECT_STATE);
        ChipLogDetail(NotSpecified, "ClDim INIT D");

        mFeatureMap = GenerateFeatureMap();
        LogFeatureMap(mFeatureMap);

        return CHIP_NO_ERROR;
    };


    // Positioning setters
    template <bool Enabled = FeaturePositioningEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetPositioning(Optional<Percent100ths> aPositioning, Optional<Globals::ThreeLevelAutoEnum> aSpeed, bool isTarget)
    {
        NBitMaskPositioningBitmap & positioning = isTarget ? this->mTargetPositioning : this->mCurrentPositioning;
        NBitMaskPositioningBitmap oldPositioning = positioning;
    
        if (aPositioning.HasValue())
        {
            if (aPositioning.Value() > 10000)
            {
                return CHIP_ERROR_INVALID_ARGUMENT;
            }

            BitMaskPositioningBitmap temp;

            if (positioning.IsNull())
            {
                ChipLogDetail(NotSpecified, "Position No VALUE");
            }
            else
            {
                ChipLogDetail(NotSpecified, "Position Had VALUE");
                BitMaskPositioningBitmap prev = positioning.Value();
                uint16_t speed = prev.GetField(PositioningBitmap::kSpeed);
                temp.SetField(PositioningBitmap::kSpeed, static_cast<uint16_t>(speed));
            }

            temp.SetField(PositioningBitmap::kPosition, static_cast<uint16_t>(aPositioning.Value()));
            positioning.SetNonNull(temp);
            LogPositioning(temp);
        }

        if (aSpeed.HasValue())
        {
            if (aSpeed.Value() >= Globals::ThreeLevelAutoEnum::kUnknownEnumValue)
            {
                return CHIP_ERROR_INVALID_ARGUMENT;
            }

            BitMaskPositioningBitmap temp;
    
            if (positioning.IsNull())
            {
                ChipLogDetail(NotSpecified, "Speed No VALUE");
            }
            else
            {
                ChipLogDetail(NotSpecified, "Speed Had VALUE");
                BitMaskPositioningBitmap prev = positioning.Value();
                uint16_t position = prev.GetField(PositioningBitmap::kPosition);
                temp.SetField(PositioningBitmap::kPosition, static_cast<uint16_t>(position));
            }

            temp.SetField(PositioningBitmap::kSpeed   , static_cast<uint16_t>(aSpeed.Value()));
            positioning.SetNonNull(temp);
            LogPositioning(temp);
        }

        if (!aSpeed.HasValue() && !aPositioning.HasValue())
        {
            positioning.SetNull();
            ChipLogDetail(NotSpecified, "set Nullable");
        }

        // Check to see if a change has ocurred
        VerifyOrReturnError(positioning != oldPositioning, CHIP_NO_ERROR);

        ChipLogDetail(NotSpecified, "fire update attribute report");
        const AttributeId aId = isTarget ? Attributes::TargetPositioning::Id : Attributes::CurrentPositioning::Id;
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, aId);

        return CHIP_NO_ERROR;
    };

    template <bool Enabled = FeaturePositioningEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetCurrentPositioning(Optional<Percent100ths> aPositioning, Optional<Globals::ThreeLevelAutoEnum> aSpeed)
    {
        return SetPositioning(aPositioning, aSpeed, false);
    };

    template <bool Enabled = FeaturePositioningEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetTargetPositioning(Optional<Percent100ths> aPositioning, Optional<Globals::ThreeLevelAutoEnum> aSpeed)
    {
        return SetPositioning(aPositioning, aSpeed, true);
    };

    template <bool Enabled = FeaturePositioningEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetResolutionAndStepValue(Percent100ths aResolutionPercent100ths, uint8_t aStepCoef)
    {
        if (0 == aResolutionPercent100ths)
        {
            aResolutionPercent100ths = 1;
        }

        if (aResolutionPercent100ths > 10000)
        {
            this->mResolution = 10000;
        }
        else
        {
            this->mResolution = aResolutionPercent100ths;
        }

        if (0 == aStepCoef)
        {
            this->mStepValue = this->mResolution;
        }
        else
        {
            this->mStepValue = this->mResolution * aStepCoef;
        }

        if (this->mStepValue > 10000)
        {
            this->mStepValue = 10000;
        }

        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::Resolution::Id);
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::StepValue::Id);

        ChipLogDetail(NotSpecified, "Resolution=%u, StepValue=%u", this->mResolution, this->mStepValue);        

        return CHIP_NO_ERROR;
    };

    template <bool Enabled = FeatureUnitEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetUnitAndRange(UnitEnum aUnit, int16_t aMin, int16_t aMax)
    {
        VerifyOrReturnError(aMax > aMin, CHIP_ERROR_INVALID_ARGUMENT);
        VerifyOrReturnError(EnsureKnownEnumValue(aUnit) != UnitEnum::kUnknownEnumValue, CHIP_ERROR_INVALID_ARGUMENT);

        if (this->mUnit != aUnit)
        {
            this->mUnit = aUnit;
            MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::Unit::Id);
            ChipLogDetail(NotSpecified, "Unit=%u", to_underlying(this->mUnit));
        }

        // Check to see if a change has ocurred
        VerifyOrReturnError(this->mRange.min != aMin || this->mRange.max != aMax , CHIP_NO_ERROR);
        this->mRange.min = aMin;
        this->mRange.max = aMax;
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::Range::Id);

        ChipLogDetail(NotSpecified, "Range.min=%i Range.max=%i", this->mRange.min, this->mRange.max);

        return CHIP_NO_ERROR;
    };

    // Limitation setters
    template <bool Enabled = FeatureLimitationEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetLimitRange(Optional<Percent100ths> aMin, Optional<Percent100ths> aMax)
    {
        //DataModel::Nullable<LatchingEnum> oldLatching = nullableLatching;
        NRangePercent100thsStruct limitRange = this->mLimitRange;
        RangePercent100thsStruct limit = { .min = 0, .max = 10000 };

        if (!aMin.HasValue() && !aMax.HasValue())
        {
            limitRange.SetNull();
            ChipLogDetail(NotSpecified, "set limit range Nullable");
        }

        if (aMin.HasValue() && aMax.HasValue())
        {
            VerifyOrReturnError(aMax.Value() > aMin.Value(), CHIP_ERROR_INVALID_ARGUMENT);
        }

        if (aMin.HasValue())
        {
            limit.min = aMin.Value();
            limitRange.SetNonNull(limit);
            ChipLogDetail(NotSpecified, "set limit range min value");
        }

        if (aMax.HasValue())
        {
            limit.max = aMax.Value();
            limitRange.SetNonNull(limit);
            ChipLogDetail(NotSpecified, "set limit range max value");
        }

        this->mLimitRange = limitRange;

        if (this->mLimitRange.IsNull())
        {
            ChipLogDetail(NotSpecified, "Limit Null")
        }
        else
        {
            ChipLogDetail(NotSpecified, "Limit.min=%i Limit.max=%i", this->mLimitRange.Value().min, this->mLimitRange.Value().max);
        }

        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::LimitRange::Id);

        return CHIP_NO_ERROR;
    };

    template <bool Enabled = FeatureTranslationEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetTranslationDirection(TranslationDirectionEnum aTranslationDirection)
    {
        VerifyOrReturnError(EnsureKnownEnumValue(aTranslationDirection) != TranslationDirectionEnum::kUnknownEnumValue, CHIP_ERROR_INVALID_ARGUMENT);
        // Check to see if a change has ocurred
        VerifyOrReturnError(this->mTranslationDirection != aTranslationDirection, CHIP_NO_ERROR);
        this->mTranslationDirection = aTranslationDirection;
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::TranslationDirection::Id);

        return CHIP_NO_ERROR;
    };

    template <bool Enabled = FeatureRotationEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetRotationAxis(RotationAxisEnum aRotationAxis)
    {
        VerifyOrReturnError(EnsureKnownEnumValue(aRotationAxis) != RotationAxisEnum::kUnknownEnumValue, CHIP_ERROR_INVALID_ARGUMENT);
        // Check to see if a change has ocurred
        VerifyOrReturnError(this->mRotationAxis != aRotationAxis, CHIP_NO_ERROR);
        this->mRotationAxis = aRotationAxis;
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::RotationAxis::Id);

        return CHIP_NO_ERROR;
    };

    template <bool Enabled = FeatureRotationEnabled || FeatureLatchingEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetOverFlow(OverFlowEnum aOverFlow)
    {
        ChipLogDetail(NotSpecified, "set OverFlow A");
        VerifyOrReturnError(EnsureKnownEnumValue(aOverFlow) != OverFlowEnum::kUnknownEnumValue, CHIP_ERROR_INVALID_ARGUMENT);
        // Check to see if a change has ocurred
        VerifyOrReturnError(this->mOverFlow != aOverFlow, CHIP_NO_ERROR);
        this->mOverFlow = aOverFlow;
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::OverFlow::Id);
        ChipLogDetail(NotSpecified, "set OverFlow");
        return CHIP_NO_ERROR;
    };

    template <bool Enabled = FeatureModulationEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetModulationType(ModulationTypeEnum aModulationType)
    {
        VerifyOrReturnError(EnsureKnownEnumValue(aModulationType) != ModulationTypeEnum::kUnknownEnumValue, CHIP_ERROR_INVALID_ARGUMENT);
        // Check to see if a change has ocurred
        VerifyOrReturnError(this->mModulationType != aModulationType, CHIP_NO_ERROR);
        this->mModulationType = aModulationType;
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::ModulationType::Id);

        return CHIP_NO_ERROR;
    };

    // Latching setters
    template <bool Enabled = FeatureLatchingEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetLatching(NLatchingEnum & nullableLatching, AttributeId aId, Optional<LatchingEnum> aLatching)
    {
        NLatchingEnum oldLatching = nullableLatching;

        if (aLatching.HasValue())
        {
            nullableLatching.SetNonNull(aLatching.Value());
            ChipLogDetail(NotSpecified, "set latch w/ value");
        }
        else
        {
            nullableLatching.SetNull();
            ChipLogDetail(NotSpecified, "set latch to null");
        }


        // Check to see if a change has ocurred
        VerifyOrReturnError(oldLatching != nullableLatching, CHIP_NO_ERROR);

        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, aId);

        return CHIP_NO_ERROR;
    };

    template <bool Enabled = FeatureLatchingEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetCurrentLatching(Optional<LatchingEnum> aLatching)
    {
        return SetLatching(this->mCurrentLatching, Attributes::CurrentLatching::Id, aLatching);
    };

    template <bool Enabled = FeatureLatchingEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetTargetLatching(Optional<LatchingEnum> aLatching)
    {
        return SetLatching(this->mTargetLatching, Attributes::TargetLatching::Id, aLatching);
    };

    template <bool Enabled = FeatureLatchingEnabled && !FeaturePositioningEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    CHIP_ERROR SetLatchingAxis(LatchingAxisEnum aLatchingAxis)
    {
        VerifyOrReturnError(EnsureKnownEnumValue(aLatchingAxis) != LatchingAxisEnum::kUnknownEnumValue, CHIP_ERROR_INVALID_ARGUMENT);
        // Check to see if a change has ocurred
        VerifyOrReturnError(this->mLatchingAxis != aLatchingAxis, CHIP_NO_ERROR);
        this->mLatchingAxis = aLatchingAxis;
        MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::LatchingAxis::Id);

        return CHIP_NO_ERROR;
    };

    // template <bool Enabled = FeatureLatchingEnabled, typename = std::enable_if_t<Enabled, CHIP_ERROR>>
    // CHIP_ERROR SetStepValue(Percent100ths aLevel)
    // {
    //     if constexpr (!FeatureRotationEnabled)
    //     {
    //         // if (aLevel == StepValueEnum::kMedium)
    //         // {
    //         //     return CHIP_ERROR_INVALID_ARGUMENT;
    //         // }
    //     }

    //     if constexpr (!FeatureTranslationEnabled)
    //     {
    //         // if (aLevel == StepValueEnum::kCritical)
    //         // {
    //         //     return CHIP_ERROR_INVALID_ARGUMENT;
    //         // }
    //     }

    //     // Check to see if a change has ocurred
    //     VerifyOrReturnError(this->mLevel != aLevel, CHIP_NO_ERROR);
    //     this->mLevel = aLevel;
    //     MatterReportingAttributeChangeCallback(mEndpointId, mClusterId, Attributes::StepValue::Id);

    //     return CHIP_NO_ERROR;
    // };


};


/* ====================================================================================================== */
/* ====================================================================================================== */
/* ====================================================================================================== */
/* ====================================================================================================== */
/* ====================================================================================================== */
/* ====================================================================================================== */

// template <bool FeaturePositioningEnabled,
//           bool FeatureLatchingEnabled,
//           bool FeatureUnitEnabled,
//           bool FeatureSpeedEnabled,
//           bool FeatureLimitationEnabled,
//           bool FeatureRotationEnabled,
//           bool FeatureTranslationEnabled,
//           bool FeatureModulationEnabled>

/**
 * A factory function to create a new instance of a Concentration Measurement Cluster with only the NumericMeasurement feature
 * enabled.
 *
 * @tparam FeatureUnitEnabled Whether the PeakMeasurement feature is enabled.
 * @tparam FeatureSpeedEnabled Whether the AverageMeasurement feature is enabled.
 * @param endpoint Endpoint that the cluster is on.
 * @param clusterId Cluster that the cluster is on.
 * @param aResolution The measurement medium.
 * @param aUnit The measurement unit.
 * @return A new instance of Concentration Measurement Cluster.
 */

template <bool FeatureSpeedEnabled, bool FeatureLimitationEnabled, bool FeatureLatchingEnabled>
Instance<true, FeatureLatchingEnabled, false, FeatureSpeedEnabled, FeatureLimitationEnabled, true, false, false>
ClosureRotationInstance(EndpointId endpoint, ClusterId clusterId, Percent100ths aResolution, UnitEnum aUnit)
{
    return Instance<
        true, // position
        FeatureLatchingEnabled,
        false, // unit
        FeatureSpeedEnabled,
        FeatureLimitationEnabled,
        true, // rotation
        false, // translation
        false>( // modulation
        endpoint, clusterId, aResolution, aUnit);
}

template <bool FeatureSpeedEnabled, bool FeatureLimitationEnabled, bool FeatureLatchingEnabled>
Instance<true, FeatureLatchingEnabled, false, FeatureSpeedEnabled, FeatureLimitationEnabled, false, true, false>
ClosureTranslationInstance(EndpointId endpoint, ClusterId clusterId, Percent100ths aResolution, UnitEnum aUnit)
{
    return Instance<
        true, // position
        FeatureLatchingEnabled,
        false, // unit
        FeatureSpeedEnabled,
        FeatureLimitationEnabled,
        false, // rotation
        true, // translation
        false>( // modulation
        endpoint, clusterId, aResolution, aUnit);
}

template <bool FeatureSpeedEnabled, bool FeatureLimitationEnabled, bool FeatureLatchingEnabled>
Instance<true, FeatureLatchingEnabled, true, FeatureSpeedEnabled, FeatureLimitationEnabled, false, false, true>
ClosureModulationInstance(EndpointId endpoint, ClusterId clusterId, Percent100ths aResolution, UnitEnum aUnit)
{
    return Instance<
        true, // position
        FeatureLatchingEnabled,
        true, // unit
        FeatureSpeedEnabled,
        FeatureLimitationEnabled,
        false, // rotation
        false, // translation
        true>( // modulation
        endpoint, clusterId, aResolution, aUnit);
}

Instance<false, true, false, false, false, false, false, false>
ClosureLatchOnlyInstance(EndpointId endpoint, ClusterId clusterId, Percent100ths aResolution, UnitEnum aUnit)
{
    return Instance<
        false, // position
        true, // latch
        false, // unit
        false, // speed
        false, // limitation
        false, // rotation
        false, // translation
        false>( // modulation
        endpoint, clusterId, aResolution, aUnit);
}

} // namespace ClosureDimension
} // namespace Clusters
} // namespace app
} // namespace chip
