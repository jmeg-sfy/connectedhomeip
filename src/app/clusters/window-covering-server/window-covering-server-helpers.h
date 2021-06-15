/**
 *
 *    Copyright (c) 2020 Project CHIP Authors
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


/**
 *
 *    Copyright (c) 2020 Silicon Labs
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
/****************************************************************************
 * @file
 * @brief Routines for the Window Covering Server cluster
 *******************************************************************************
 ******************************************************************************/

#include <app/util/af-enums.h>
#include <app/common/gen/enums.h>

#define WC_PERCENTAGE_COEF 100
#define WC_PERCENT100THS_MAX 10000
#define WC_DEFAULT_EP 1

typedef uint16_t posPercent100ths_t;

//  ZCL_WC_PHYSICAL_CLOSED_LIMIT_LIFT_ATTRIBUTE_ID (0x0001)
//  ZCL_WC_PHYSICAL_CLOSED_LIMIT_TILT_ATTRIBUTE_ID (0x0002)

//  ZCL_WC_NUMBER_OF_ACTUATIONS_LIFT_ATTRIBUTE_ID (0x0005)
//  ZCL_WC_NUMBER_OF_ACTUATIONS_TILT_ATTRIBUTE_ID (0x0006)


//  ZCL_WC_OPERATIONAL_STATUS_ATTRIBUTE_ID (0x000A)

//  ZCL_WC_END_PRODUCT_TYPE_ATTRIBUTE_ID (0x000D)


//  ZCL_WC_VELOCITY_LIFT_ATTRIBUTE_ID (0x0014)
//  ZCL_WC_ACCELERATION_TIME_LIFT_ATTRIBUTE_ID (0x0015)
//  ZCL_WC_DECELERATION_TIME_LIFT_ATTRIBUTE_ID (0x0016)
//  ZCL_WC_MODE_ATTRIBUTE_ID (0x0017)
//  ZCL_WC_INTERMEDIATE_SETPOINTS_LIFT_ATTRIBUTE_ID (0x0018)
//  ZCL_WC_INTERMEDIATE_SETPOINTS_TILT_ATTRIBUTE_ID (0x0019)
//  ZCL_WC_SAFETY_STATUS_ATTRIBUTE_ID (0x001A)




EmberAfStatus wcWriteAttribute(chip::EndpointId ep, chip::AttributeId attributeID, uint8_t * dataPtr, EmberAfAttributeType dataType);
EmberAfStatus wcReadAttribute(chip::EndpointId ep, chip::AttributeId attributeID, uint8_t * dataPtr, uint16_t readLength);

EmberAfStatus wcSetType(chip::EndpointId ep, EmberAfWcType type);
EmberAfStatus wcGetType(chip::EndpointId ep, EmberAfWcType * p_type);

EmberAfStatus wcSetSafetyStatus(chip::EndpointId ep, uint16_t safetyStatus);
EmberAfStatus wcGetSafetyStatus(chip::EndpointId ep, uint16_t * p_safetyStatus);

EmberAfStatus wcSetConfigStatus(chip::EndpointId ep, uint8_t configStatus);
EmberAfStatus wcGetConfigStatus(chip::EndpointId ep, uint8_t * p_configStatus);

EmberAfStatus wcSetEndProductType(chip::EndpointId ep, EmberAfWcEndProductType endProduct);
EmberAfStatus wcGetEndProductType(chip::EndpointId ep, EmberAfWcEndProductType * p_endProduct);

EmberAfStatus wcSetOperationalStatus(chip::EndpointId ep, uint8_t operationalStatus);
EmberAfStatus wcGetOperationalStatus(chip::EndpointId ep, uint8_t * p_operationalStatus);

EmberAfStatus wcSetInstalledOpenLimitLift(chip::EndpointId ep, uint16_t installedOpenLimitLift);
EmberAfStatus wcGetInstalledOpenLimitLift(chip::EndpointId ep, uint16_t * p_installedOpenLimitLift);

EmberAfStatus wcSetInstalledOpenLimitTilt(chip::EndpointId ep, uint16_t installedOpenLimitTilt);
EmberAfStatus wcGetInstalledOpenLimitTilt(chip::EndpointId ep, uint16_t * p_installedOpenLimitTilt);

EmberAfStatus wcSetInstalledClosedLimitLift(chip::EndpointId ep, uint16_t installedClosedLimitLift);
EmberAfStatus wcGetInstalledClosedLimitLift(chip::EndpointId ep, uint16_t * p_installedClosedLimitLift);

EmberAfStatus wcSetInstalledClosedLimitTilt(chip::EndpointId ep, uint16_t installedClosedLimitTilt);
EmberAfStatus wcGetInstalledClosedLimitTilt(chip::EndpointId ep, uint16_t * p_installedClosedLimitTilt);

EmberAfStatus wcSetTargetPositionLift(chip::EndpointId ep, posPercent100ths_t liftPercent100ths);
EmberAfStatus wcGetTargetPositionLift(chip::EndpointId ep, posPercent100ths_t * p_liftPercent100ths);

EmberAfStatus wcSetTargetPositionTilt(chip::EndpointId ep, posPercent100ths_t tiltPercent100ths);
EmberAfStatus wcGetTargetPositionTilt(chip::EndpointId ep, posPercent100ths_t * p_tiltPercent100ths);

EmberAfStatus wcSetCurrentPositionLift(chip::EndpointId ep, posPercent100ths_t liftPercent100ths, uint16_t liftValue);
EmberAfStatus wcGetCurrentPositionLift(chip::EndpointId ep, posPercent100ths_t * p_liftPercent100ths);

EmberAfStatus wcSetCurrentPositionTilt(chip::EndpointId ep, posPercent100ths_t tiltPercent100ths, uint16_t tiltValue);
EmberAfStatus wcGetCurrentPositionTilt(chip::EndpointId ep, posPercent100ths_t * p_tiltPercent100ths);








//_______________________________________________________________________________________________________________________










