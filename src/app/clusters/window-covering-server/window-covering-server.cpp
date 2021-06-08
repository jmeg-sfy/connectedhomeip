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

//#include "on-off-server.h"

#include <app/util/af.h>

//#include "gen/att-storage.h"
#include <app/common/gen/attribute-id.h>
//#include "gen/attribute-type.h"
//#include "gen/cluster-id.h"
#include <app/common/gen/command-id.h>
#include <app/common/gen/cluster-id.h>


#include <app/Command.h>
#include <app/reporting/reporting.h>
#include <app/util/af-event.h>
#include <app/util/af-types.h>
#include <app/util/attribute-storage.h>

#ifdef EMBER_AF_PLUGIN_SCENES
#include <app/clusters/scenes/scenes.h>
#endif // EMBER_AF_PLUGIN_SCENES

using namespace chip;



#define WC_PERCENTAGE_COEF 100


static EmberAfStatus emberAfWindowCoveringClusterSetValueCallback(EndpointId endpoint, uint8_t command);

// ZCL_WC_TYPE_ATTRIBUTE_ID (0x0000)
//  ZCL_WC_PHYSICAL_CLOSED_LIMIT_LIFT_ATTRIBUTE_ID (0x0001)
//  ZCL_WC_PHYSICAL_CLOSED_LIMIT_TILT_ATTRIBUTE_ID (0x0002)
//  ZCL_WC_CURRENT_POSITION_LIFT_ATTRIBUTE_ID (0x0003)
//  ZCL_WC_CURRENT_POSITION_TILT_ATTRIBUTE_ID (0x0004)
//  ZCL_WC_NUMBER_OF_ACTUATIONS_LIFT_ATTRIBUTE_ID (0x0005)
//  ZCL_WC_NUMBER_OF_ACTUATIONS_TILT_ATTRIBUTE_ID (0x0006)
//  ZCL_WC_CONFIG_STATUS_ATTRIBUTE_ID (0x0007)
//  ZCL_WC_CURRENT_POSITION_LIFT_PERCENTAGE_ATTRIBUTE_ID (0x0008)
//  ZCL_WC_CURRENT_POSITION_TILT_PERCENTAGE_ATTRIBUTE_ID (0x0009)
//  ZCL_WC_OPERATIONAL_STATUS_ATTRIBUTE_ID (0x000A)
//  ZCL_WC_TARGET_POSITION_LIFT_PERCENT100_THS_ATTRIBUTE_ID (0x000B)
//  ZCL_WC_TARGET_POSITION_TILT_PERCENT100_THS_ATTRIBUTE_ID (0x000C)
//  ZCL_WC_END_PRODUCT_TYPE_ATTRIBUTE_ID (0x000D)
//  ZCL_WC_CURRENT_POSITION_LIFT_PERCENT100_THS_ATTRIBUTE_ID (0x000E)
//  ZCL_WC_CURRENT_POSITION_TILT_PERCENT100_THS_ATTRIBUTE_ID (0x000F)
//  ZCL_WC_INSTALLED_OPEN_LIMIT_LIFT_ATTRIBUTE_ID (0x0010)
//  ZCL_WC_INSTALLED_CLOSED_LIMIT_LIFT_ATTRIBUTE_ID (0x0011)
//  ZCL_WC_INSTALLED_OPEN_LIMIT_TILT_ATTRIBUTE_ID (0x0012)
//  ZCL_WC_INSTALLED_CLOSED_LIMIT_TILT_ATTRIBUTE_ID (0x0013)
//  ZCL_WC_VELOCITY_LIFT_ATTRIBUTE_ID (0x0014)
//  ZCL_WC_ACCELERATION_TIME_LIFT_ATTRIBUTE_ID (0x0015)
//  ZCL_WC_DECELERATION_TIME_LIFT_ATTRIBUTE_ID (0x0016)
//  ZCL_WC_MODE_ATTRIBUTE_ID (0x0017)
//  ZCL_WC_INTERMEDIATE_SETPOINTS_LIFT_ATTRIBUTE_ID (0x0018)
//  ZCL_WC_INTERMEDIATE_SETPOINTS_TILT_ATTRIBUTE_ID (0x0019)
//  ZCL_WC_SAFETY_STATUS_ATTRIBUTE_ID (0x001A)


EmberAfStatus emberAfWcWriteAttribute(chip::EndpointId ep, chip::AttributeId attributeID, uint8_t * dataPtr, EmberAfAttributeType dataType)
{
    EmberAfStatus status = emberAfWriteAttribute(ep, ZCL_WINDOW_COVERING_CLUSTER_ID, attributeID, CLUSTER_MASK_SERVER, dataPtr, dataType);

    if (status != EMBER_ZCL_STATUS_SUCCESS) {
        emberAfWindowCoveringClusterPrint("Err: WC Writing Attribute failed: %x", status);
    }
    return status;
}


EmberAfStatus emberAfWcReadAttribute(chip::EndpointId ep, chip::AttributeId attributeID, uint8_t * dataPtr, uint16_t readLength)
{
    EmberAfStatus status = emberAfReadAttribute(ep, ZCL_WINDOW_COVERING_CLUSTER_ID, attributeID, CLUSTER_MASK_SERVER, dataPtr, readLength, NULL);

    if (status != EMBER_ZCL_STATUS_SUCCESS) {
        emberAfWindowCoveringClusterPrint("Err: WC Reading Attribute failed: %x", status);
    }
    return status;
}


EmberAfStatus emberAfWcSetTargetPositionLift(EndpointId ep, uint16_t value)
{
    return emberAfWcWriteAttribute(ep, ZCL_WC_TARGET_POSITION_LIFT_PERCENT100_THS_ATTRIBUTE_ID, (uint8_t *) &value, ZCL_INT16U_ATTRIBUTE_TYPE);
}

EmberAfStatus emberAfWcSetTargetPositionTilt(EndpointId ep, uint16_t value)
{
    return emberAfWcWriteAttribute(ep, ZCL_WC_TARGET_POSITION_TILT_PERCENT100_THS_ATTRIBUTE_ID, (uint8_t *) &value, ZCL_INT16U_ATTRIBUTE_TYPE);
}

EmberAfStatus emberAfWcGetTargetPositionLift(EndpointId ep, uint8_t * dataPtr, uint16_t readLength)
{
    return emberAfWcReadAttribute(ep, ZCL_WC_TARGET_POSITION_LIFT_PERCENT100_THS_ATTRIBUTE_ID, dataPtr, readLength);
}

EmberAfStatus emberAfWcGetTargetPositionTilt(EndpointId ep, uint8_t * dataPtr, uint16_t readLength)
{
    return emberAfWcReadAttribute(ep, ZCL_WC_TARGET_POSITION_TILT_PERCENT100_THS_ATTRIBUTE_ID, dataPtr, readLength);
}

//_______________________________________________________________________________________________________________________

/**
 * @brief Window Covering Cluster UpOrOpen Command callback
 */
bool emberAfWindowCoveringClusterUpOrOpenCallback(chip::app::Command * commandObj)
{
    emberAfWindowCoveringClusterPrint("UpOrOpen command received");

    EmberAfStatus status = emberAfWindowCoveringClusterSetValueCallback(emberAfCurrentEndpoint(), ZCL_UP_OR_OPEN_COMMAND_ID);
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/**
 * @brief Window Covering Cluster DownOrClose Command callback
 */
bool emberAfWindowCoveringClusterDownOrCloseCallback(chip::app::Command *commandObj)
{
    emberAfWindowCoveringClusterPrint("DownOrClose command received");

    EmberAfStatus status = emberAfWindowCoveringClusterSetValueCallback(emberAfCurrentEndpoint(), ZCL_DOWN_OR_CLOSE_COMMAND_ID);
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/**
 * @brief Window Covering Cluster WindowCoveringStop Command callback
 */
bool emberAfWindowCoveringClusterStopMotionCallback(chip::app::Command *)
{
    emberAfWindowCoveringClusterPrint("StopMotion command received");

    EmberAfStatus status = emberAfWindowCoveringClusterSetValueCallback(emberAfCurrentEndpoint(), ZCL_STOP_COMMAND_ID);
    emberAfSendImmediateDefaultResponse(status);

    return true;
}

// /**
//  * @brief Window Covering Cluster WindowCoveringGoToLiftAccuratePercentage Command callback
//  * @param accuratePercentageLiftValue
//  */

// bool emberAfWindowCoveringClusterGoToLiftAccuratePercentageCallback(chip::app::Command *, uint16_t accuratePercentageLiftValue)
// {
//     EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
//     emberAfWindowCoveringClusterPrint("Window Go To Lift Accurate Percentage command received");
//     //AppTask::Instance().Cover().LiftGoToAccuratePercentage(accuratePercentageLiftValue);
//     emberAfSendImmediateDefaultResponse(status);
//     return true;
// }

/**
 * @brief Window Covering Cluster WindowCoveringGoToLiftPercentage Command callback
 * @param percentageLiftValue
 */

bool emberAfWindowCoveringClusterGoToLiftPercentageCallback(chip::app::Command *, uint8_t liftPercentageValue)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
    emberAfWindowCoveringClusterPrint("GoToLiftPercentage Percentage command received");
    //AppTask::Instance().Cover().LiftGoToAccuratePercentage(percentageLiftValue * WC_PERCENTAGE_COEF);
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

bool emberAfWindowCoveringClusterGoToLiftPercentageCallback(chip::app::Command *, uint8_t liftPercentageValue, uint16_t liftPercent100thsValue)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
    emberAfWindowCoveringClusterPrint("GoToLiftPercentage Percentage command received");
    //AppTask::Instance().Cover().LiftGoToAccuratePercentage(percentageLiftValue * WC_PERCENTAGE_COEF);
    emberAfSendImmediateDefaultResponse(status);
    return true;
}


/**
 * @brief Window Covering Cluster WindowCoveringGoToLiftValue Command callback
 * @param liftValue
 */

bool emberAfWindowCoveringClusterGoToLiftValueCallback(chip::app::Command *, uint16_t liftValue)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
    emberAfWindowCoveringClusterPrint("GoToLiftValue Value command received");
    //AppTask::Instance().Cover().LiftGoToValue(liftValue);
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

// /**
//  * @brief Window Covering Cluster WindowCoveringGoToTiltAccuratePPercentage Command callback
//  * @param accuratePercentageTiltValue
//  */

// bool emberAfWindowCoveringClusterGoToTiltAccuratePercentageCallback(chip::app::Command *, uint16_t accuratePercentageTiltValue)
// {
//     EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
//     emberAfWindowCoveringClusterPrint("Window Go To Tilt Accurate Percentage command received");
//     //AppTask::Instance().Cover().TiltGoToAccuratePercentage(accuratePercentageTiltValue);
//     emberAfSendImmediateDefaultResponse(status);
//     return true;
// }

/**
 * @brief Window Covering Cluster WindowCoveringGoToTiltPercentage Command callback
 * @param percentageTiltValue
 */

bool emberAfWindowCoveringClusterGoToTiltPercentageCallback(chip::app::Command *, uint8_t tiltPercentageValue)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
    emberAfWindowCoveringClusterPrint("GoToTiltPercentage command received");
    //AppTask::Instance().Cover().TiltGoToAccuratePercentage(percentageTiltValue * WC_PERCENTAGE_COEF);
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

bool emberAfWindowCoveringClusterGoToTiltPercentageCallback(chip::app::Command *, uint8_t tiltPercentageValue, uint16_t tiltPercent100thsValue)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
    emberAfWindowCoveringClusterPrint("GoToTiltPercentage command received");
    //AppTask::Instance().Cover().TiltGoToAccuratePercentage(percentageTiltValue * WC_PERCENTAGE_COEF);
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

/**
 * @brief Window Covering Cluster WindowCoveringGoToTiltValue Command callback
 * @param tiltValue
 */

bool emberAfWindowCoveringClusterGoToTiltValueCallback(chip::app::Command *, uint16_t tiltValue)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
    emberAfWindowCoveringClusterPrint("GoToTiltValue command received");
    //AppTask::Instance().Cover().TiltGoToValue(tiltValue);
    emberAfSendImmediateDefaultResponse(status);
    return true;
}

//_______________________________________________________________________________________________________________________

// void emberAfPostAttributeChangeCallback(EndpointId endpoint, ClusterId clusterId, AttributeId attributeId, uint8_t mask,
//                                         uint16_t manufacturerCode, uint8_t type, uint16_t size, uint8_t * value)
// {
//     if (clusterId != ZCL_WINDOW_COVERING_CLUSTER_ID)
//     {
//         emberAfWindowCoveringClusterPrint("Unknown cluster ID: %d", clusterId);
//     }
// }

/** @brief Window Covering Cluster Init
 *
 * Cluster Init
 *
 * @param endpoint    Endpoint that is being initialized
 */
void emberAfWindowCoveringClusterInitCallback(chip::EndpointId endpoint)
{
    emberAfWindowCoveringClusterPrint("Window Covering Cluster init");
}



///EmberAfStatus emberAfWindowCoveringClusterSetValueCallback(EndpointId endpoint, uint8_t command, bool initiatedByLevelChange)
static EmberAfStatus emberAfWindowCoveringClusterSetValueCallback(EndpointId endpoint, uint8_t command)
{
    //EmberAfStatus status;
    //bool currentValue, newValue;

/*     switch(command) {
    case ZCL_WC_UP_OR_OPEN_COMMAND_ID:
        break;
    case ZCL_WC_DOWN_OR_CLOSE_COMMAND_ID:
        break;
    case ZCL_WC_STOP_COMMAND_ID:
        break;
    default:
    } */



//     // read current on/off value
//     status = emberAfReadAttribute(endpoint, ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
//                                   (uint8_t *) &currentValue, sizeof(currentValue),
//                                   NULL); // data type
//     if (status != EMBER_ZCL_STATUS_SUCCESS)
//     {
//         emberAfWindowCoveringClusterPrintln("ERR: reading on/off %x", status);
//         return status;
//     }

//     // if the value is already what we want to set it to then do nothing
//     if ((!currentValue && command == ZCL_OFF_COMMAND_ID) || (currentValue && command == ZCL_ON_COMMAND_ID))
//     {
//         emberAfWindowCoveringClusterPrintln("On/off already set to new value");
//         return EMBER_ZCL_STATUS_SUCCESS;
//     }

//     // we either got a toggle, or an on when off, or an off when on,
//     // so we need to swap the value
//     newValue = !currentValue;
//     emberAfWindowCoveringClusterPrintln("Toggle on/off from %x to %x", currentValue, newValue);

//     // the sequence of updating on/off attribute and kick off level change effect should
//     // be depend on whether we are turning on or off. If we are turning on the light, we
//     // should update the on/off attribute before kicking off level change, if we are
//     // turning off the light, we should do the opposite, that is kick off level change
//     // before updating the on/off attribute.
//     if (newValue)
//     {
//         // write the new on/off value
//         status = emberAfWriteAttribute(endpoint, ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
//                                        (uint8_t *) &newValue, ZCL_BOOLEAN_ATTRIBUTE_TYPE);
//         if (status != EMBER_ZCL_STATUS_SUCCESS)
//         {
//             emberAfWindowCoveringClusterPrintln("ERR: writing on/off %x", status);
//             return status;
//         }

// #ifdef EMBER_AF_PLUGIN_LEVEL_CONTROL
//         // If initiatedByLevelChange is false, then we assume that the level change
//         // ZCL stuff has not happened and we do it here
//         if (!initiatedByLevelChange && emberAfContainsServer(endpoint, ZCL_LEVEL_CONTROL_CLUSTER_ID))
//         {
//             emberAfWindowCoveringClusterLevelControlEffectCallback(endpoint, newValue);
//         }
// #endif // EMBER_AF_PLUGIN_LEVEL_CONTROL
//     }
//     else
//     {
// #ifdef EMBER_AF_PLUGIN_LEVEL_CONTROL
//         // If initiatedByLevelChange is false, then we assume that the level change
//         // ZCL stuff has not happened and we do it here
//         if (!initiatedByLevelChange && emberAfContainsServer(endpoint, ZCL_LEVEL_CONTROL_CLUSTER_ID))
//         {
//             emberAfWindowCoveringClusterLevelControlEffectCallback(endpoint, newValue);
//         }
// #endif // EMBER_AF_PLUGIN_LEVEL_CONTROL

//         // write the new on/off value
//         status = emberAfWriteAttribute(endpoint, ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
//                                        (uint8_t *) &newValue, ZCL_BOOLEAN_ATTRIBUTE_TYPE);
//         if (status != EMBER_ZCL_STATUS_SUCCESS)
//         {
//             emberAfWindowCoveringClusterPrintln("ERR: writing on/off %x", status);
//             return status;
//         }
//     }



// #ifdef EMBER_AF_PLUGIN_SCENES
//     // the scene has been changed (the value of on/off has changed) so
//     // the current scene as descibed in the attribute table is invalid,
//     // so mark it as invalid (just writes the valid/invalid attribute)
//     if (emberAfContainsServer(endpoint, ZCL_SCENES_CLUSTER_ID))
//     {
//         emberAfScenesClusterMakeInvalidCallback(endpoint);
//     }
// #endif // EMBER_AF_PLUGIN_SCENES

//     // The returned status is based solely on the On/Off cluster.  Errors in the
//     // Level Control and/or Scenes cluster are ignored.
//     return EMBER_ZCL_STATUS_SUCCESS;
// }

//     EmberAfStatus status;
//     bool currentValue, newValue;

//     emberAfWindowCoveringClusterPrintln("On/Off set value: %x %x", endpoint, command);



//     }



// #ifdef EMBER_AF_PLUGIN_SCENES
//     // the scene has been changed (the value of on/off has changed) so
//     // the current scene as descibed in the attribute table is invalid,
//     // so mark it as invalid (just writes the valid/invalid attribute)
//     if (emberAfContainsServer(endpoint, ZCL_SCENES_CLUSTER_ID))
//     {
//         emberAfScenesClusterMakeInvalidCallback(endpoint);
//     }
// #endif // EMBER_AF_PLUGIN_SCENES

//     // The returned status is based solely on the On/Off cluster.  Errors in the
//     // Level Control and/or Scenes cluster are ignored.
    return EMBER_ZCL_STATUS_SUCCESS;
}



































// ------------------------------------------------------------------------
// ****** callback section *******

/* void emberAfPluginTemperatureMeasurementServerInitCallback(void)
{
    EmberAfStatus status;
    // FIXME Use real values for the temperature sensor polling the sensor using the
    //       EMBER_AF_PLUGIN_TEMPERATURE_MEASUREMENT_SERVER_MAX_MEASUREMENT_FREQUENCY_S macro
    EndpointId endpointId = 1; // Hardcoded to 1 for now
    int16_t newValue      = 0x1234;

    status = emberAfWriteAttribute(endpointId, ZCL_TEMP_MEASUREMENT_CLUSTER_ID, ZCL_CURRENT_TEMPERATURE_ATTRIBUTE_ID,
                                   CLUSTER_MASK_SERVER, (uint8_t *) &newValue, ZCL_INT16S_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS)
    {
        emberAfTempMeasurementClusterPrint("Err: writing temperature: %x", status);
        return;
    }
} */
void emberAfWindowCoveringClusterServerInitCallback(EndpointId endpoint)
{
#ifdef ZCL_USING_ON_OFF_CLUSTER_START_UP_ON_OFF_ATTRIBUTE
    // StartUp behavior relies on WindowCovering and StartUpWindowCovering attributes being tokenized.
    if (areStartUpWindowCoveringServerAttributesTokenized(endpoint))
    {
        // Read the StartUpWindowCovering attribute and set the WindowCovering attribute as per
        // following from zcl 7 14-0127-20i-zcl-ch-3-general.doc.
        // 3.8.2.2.5	StartUpWindowCovering Attribute
        // The StartUpWindowCovering attribute SHALL define the desired startup behavior of a
        // lamp device when it is supplied with power and this state SHALL be
        // reflected in the WindowCovering attribute.  The values of the StartUpWindowCovering
        // attribute are listed below.
        // Table 3 46. Values of the StartUpWindowCovering Attribute
        // Value      Action on power up
        // 0x00       Set the WindowCovering attribute to 0 (off).
        // 0x01       Set the WindowCovering attribute to 1 (on).
        // 0x02       If the previous value of the WindowCovering attribute is equal to 0,
        //            set the WindowCovering attribute to 1.If the previous value of the WindowCovering
        //            attribute is equal to 1, set the WindowCovering attribute to 0 (toggle).
        // 0x03-0xfe  These values are reserved.  No action.
        // 0xff       Set the WindowCovering attribute to its previous value.

        // Initialize startUpWindowCovering to No action value 0xFE
        uint8_t startUpWindowCovering = 0xFE;
        EmberAfStatus status = emberAfReadAttribute(endpoint, ZCL_ON_OFF_CLUSTER_ID, ZCL_START_UP_ON_OFF_ATTRIBUTE_ID,
                                                    CLUSTER_MASK_SERVER, (uint8_t *) &startUpWindowCovering, sizeof(startUpWindowCovering), NULL);
        if (status == EMBER_ZCL_STATUS_SUCCESS)
        {
            // Initialise updated value to 0
            bool updatedWindowCovering = 0;
            status            = emberAfReadAttribute(endpoint, ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                                          (uint8_t *) &updatedWindowCovering, sizeof(updatedWindowCovering), NULL);
            if (status == EMBER_ZCL_STATUS_SUCCESS)
            {
                switch (startUpWindowCovering)
                {
                case EMBER_ZCL_START_UP_ON_OFF_VALUE_SET_TO_OFF:
                    updatedWindowCovering = 0; // Off
                    break;
                case EMBER_ZCL_START_UP_ON_OFF_VALUE_SET_TO_ON:
                    updatedWindowCovering = 1; // On
                    break;
                case EMBER_ZCL_START_UP_ON_OFF_VALUE_SET_TO_TOGGLE:
                    updatedWindowCovering = !updatedWindowCovering;
                    break;
                case EMBER_ZCL_START_UP_ON_OFF_VALUE_SET_TO_PREVIOUS:
                default:
                    // All other values 0x03- 0xFE are reserved - no action.
                    // When value is 0xFF - update with last value - that is as good as
                    // no action.
                    break;
                }
                status = emberAfWriteAttribute(endpoint, ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                                               (uint8_t *) &updatedWindowCovering, ZCL_BOOLEAN_ATTRIBUTE_TYPE);
            }
        }
    }
#endif

}

#ifdef ZCL_USING_ON_OFF_CLUSTER_START_UP_ON_OFF_ATTRIBUTE
static bool areStartUpWindowCoveringServerAttributesTokenized(EndpointId endpoint)
{
    EmberAfAttributeMetadata * metadata;

    metadata = emberAfLocateAttributeMetadata(endpoint, ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                                              EMBER_AF_NULL_MANUFACTURER_CODE);
    if (!emberAfAttributeIsTokenized(metadata))
    {
        return false;
    :

    return true;
}
#endif

