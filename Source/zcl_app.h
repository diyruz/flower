#ifndef ZCL_APP_H
#define ZCL_APP_H

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
#define HAL_UNKNOWN_BUTTON HAL_KEY_CODE_NOKEY
// Application Events

#define APP_CONST_ONE_MINUTE_IN_MS ((uint32) 60 * (uint32) 1000)

#define APP_END_DEVICE_REJOIN_EVT 0x0001
#define APP_REPORT_EVT 0x0002
#define APP_RESET_EVT 0x0004
#define APP_READ_SENSORS_EVT 0x0008


#define APP_RESET_DELAY 10 * 1000

#define APP_END_DEVICE_REJOIN_MAX_DELAY ((uint32)1800000) // 30 minutes 30 * 60 * 1000
#define APP_END_DEVICE_REJOIN_START_DELAY 10 * 1000 // 10 seconds
#define APP_END_DEVICE_REJOIN_BACKOFF ((float) 1.2)
#define APP_END_DEVICE_REJOIN_TRIES 20


#define APP_REPORT_DELAY APP_CONST_ONE_MINUTE_IN_MS //(30 * APP_CONST_ONE_MINUTE_IN_MS) // APP_CONST_ONE_MINUTE_IN_MS //(30 * APP_CONST_ONE_MINUTE_IN_MS)


/*********************************************************************
 * MACROS
 */

#define R           ACCESS_CONTROL_READ
#define RR          (R | ACCESS_REPORTABLE)

#define BASIC       ZCL_CLUSTER_ID_GEN_BASIC
#define POWER_CFG   ZCL_CLUSTER_ID_GEN_POWER_CFG
#define TEMP        ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT
#define HUMIDITY    ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY
#define PRESSURE    ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT
#define ILLUMINANCE ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT

#define ZCL_UINT8   ZCL_DATATYPE_UINT8
#define ZCL_UINT16  ZCL_DATATYPE_UINT16
#define ZCL_UINT32   ZCL_DATATYPE_UINT32
#define ZCL_INT16   ZCL_DATATYPE_INT16


#define ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE_HPA       0x0200
#define ATTRID_POWER_CFG_BATTERY_VOLTAGE_RAW_ADC                0x0200
#define ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE_RAW_ADC      0x0200



/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */

extern SimpleDescriptionFormat_t zclApp_FirstEP;
extern SimpleDescriptionFormat_t zclApp_SecondEP;

extern uint8 zclApp_BatteryVoltage;
extern uint8 zclApp_BatteryPercentageRemainig;
extern uint16 zclApp_BatteryVoltageRawAdc;
extern int16 zclApp_Temperature_Sensor_MeasuredValue;
extern int16 zclApp_PressureSensor_MeasuredValue;
extern uint32 zclApp_PressureSensor_MeasuredValueHPA;
extern uint16 zclApp_HumiditySensor_MeasuredValue;
extern int16 zclApp_DS18B20_MeasuredValue;
extern uint16 zclApp_SoilHumiditySensor_MeasuredValue;
extern uint16 zclApp_SoilHumiditySensor_MeasuredValueRawAdc;
extern uint16 zclApp_IlluminanceSensor_MeasuredValue;
extern uint16 zclApp_IlluminanceSensor_MeasuredValueRawAdc;

// attribute list
extern CONST zclAttrRec_t zclApp_AttrsFirstEP[];
extern CONST zclAttrRec_t zclApp_AttrsSecondEP[];
extern CONST uint8 zclApp_AttrsSecondEPCount;
extern CONST uint8 zclApp_AttrsFirstEPCount;


extern const uint8 zclApp_ManufacturerName[];
extern const uint8 zclApp_ModelId[];
extern const uint8 zclApp_PowerSource;

// APP_TODO: Declare application specific attributes here

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialization for the task
 */
extern void zclApp_Init(byte task_id);
extern byte zclApp_KeyCodeToButton(byte key);

/*
 *  Event Process for the task
 */
extern UINT16 zclApp_event_loop(byte task_id, UINT16 events);

void user_delay_ms(uint32_t period);

#ifdef __cplusplus
}
#endif

#endif /* ZCL_APP_H */
