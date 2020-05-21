#ifndef ZCL_FREEPADAPP_H
#define ZCL_FREEPADAPP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include "version.h"
#include "zcl.h"


/*********************************************************************
 * CONSTANTS
 */
#define SAMPLETEMPERATURESENSOR_MAX_MEASURED_VALUE  2700  // 27.00C
#define SAMPLETEMPERATURESENSOR_MIN_MEASURED_VALUE  1700  // 17.00C

#define SAMPLE_HUMODITY_SENSOR_MAX_MEASURED_VALUE  2700  // 27.00C
#define SAMPLE_HUMODITY_SENSOR_MIN_MEASURED_VALUE  1700  // 17.00C


#define SAMPLE_PRESSURE_SENSOR_MAX_MEASURED_VALUE  2700  // 27.00C
#define SAMPLE_PRESSURE_SENSOR_MIN_MEASURED_VALUE  1700  // 17.00C


#define HAL_UNKNOWN_BUTTON HAL_KEY_CODE_NOKEY
// Application Events

#define FREEPADAPP_CONST_ONE_MINUTE_IN_MS ((uint32) 60 * (uint32) 1000)

#define FREEPADAPP_END_DEVICE_REJOIN_EVT 0x0001

#define FREEPADAPP_RESET_EVT 0x0004
#define FREEPADAPP_REPORT_EVT 0x0008
#define FREEPADAPP_HOLD_START_EVT 0x0010


#define FREEPADAPP_RESET_DELAY 10 * 1000

#define FREEPADAPP_END_DEVICE_REJOIN_MAX_DELAY ((uint32)1800000) // 30 minutes 30 * 60 * 1000
#define FREEPADAPP_END_DEVICE_REJOIN_START_DELAY 10 * 1000 // 10 seconds
#define FREEPADAPP_END_DEVICE_REJOIN_BACKOFF ((float) 1.2)
#define FREEPADAPP_END_DEVICE_REJOIN_TRIES 20




#define FREEPADAPP_REPORT_DELAY ((uint32)1800000) // 30 minutes 30 * 60 * 1000


/*********************************************************************
 * MACROS
 */

#define R ACCESS_CONTROL_READ
#define RR R | ACCESS_REPORTABLE

#define BASIC ZCL_CLUSTER_ID_GEN_BASIC
#define POWER_CFG ZCL_CLUSTER_ID_GEN_POWER_CFG

#define TEMP ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT
#define HUMIDITY ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY
#define PRESSURE ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT
#define ILLUMINANCE ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT

#define ZCL_UINT8 ZCL_DATATYPE_UINT8
#define ZCL_UINT16 ZCL_DATATYPE_INT16

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */

extern SimpleDescriptionFormat_t zclFreePadApp_FirstEP;
extern SimpleDescriptionFormat_t zclFreePadApp_SecondEP;

extern uint8 zclFreePadApp_BatteryVoltage;
extern uint8 zclFreePadApp_BatteryPercentageRemainig;
extern uint16 zclSampleTemperatureSensor_MeasuredValue;
extern uint16 zclSamplePressureSensor_MeasuredValue;
extern uint16 zclSampleHumiditySensor_MeasuredValue;
extern uint16 zclSampleTemperatureSensorDS18B20_MeasuredValue;


// attribute list
extern CONST zclAttrRec_t zclFreePadApp_AttrsFirstEP[];
extern CONST zclAttrRec_t zclFreePadApp_AttrsSecondEP[];
extern CONST uint8 zclFreePadApp_AttrsSecondEPCount;
extern CONST uint8 zclFreePadApp_AttrsFirstEPCount;


extern const uint8 zclFreePadApp_ManufacturerName[];
extern const uint8 zclFreePadApp_ModelId[];
extern const uint8 zclFreePadApp_PowerSource;

// FREEPADAPP_TODO: Declare application specific attributes here

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialization for the task
 */
extern void zclFreePadApp_Init(byte task_id);
extern byte zclFreePadApp_KeyCodeToButton(byte key);

/*
 *  Event Process for the task
 */
extern UINT16 zclFreePadApp_event_loop(byte task_id, UINT16 events);

/*********************************************************************
 *********************************************************************/




#ifdef __cplusplus
}
#endif

#endif /* ZCL_FREEPADAPP_H */
