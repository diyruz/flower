#ifndef ZCL_FLOWER_APP_H
#define ZCL_FLOWER_APP_H

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

#define FLOWER_APP_CONST_ONE_MINUTE_IN_MS ((uint32) 60 * (uint32) 1000)

#define FLOWER_APP_END_DEVICE_REJOIN_EVT 0x0001
#define FLOWER_APP_REPORT_EVT 0x0002
#define FLOWER_APP_RESET_EVT 0x0004


#define FLOWER_APP_RESET_DELAY 10 * 1000

#define FLOWER_APP_END_DEVICE_REJOIN_MAX_DELAY ((uint32)1800000) // 30 minutes 30 * 60 * 1000
#define FLOWER_APP_END_DEVICE_REJOIN_START_DELAY 10 * 1000 // 10 seconds
#define FLOWER_APP_END_DEVICE_REJOIN_BACKOFF ((float) 1.2)
#define FLOWER_APP_END_DEVICE_REJOIN_TRIES 20


#define FLOWER_APP_REPORT_DELAY (30 * FLOWER_APP_CONST_ONE_MINUTE_IN_MS)


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

extern SimpleDescriptionFormat_t zclFlowerApp_FirstEP;
extern SimpleDescriptionFormat_t zclFlowerApp_SecondEP;

extern uint8 zclFlowerApp_BatteryVoltage;
extern uint8 zclFlowerApp_BatteryPercentageRemainig;
extern uint16 zclFlowerApp_BatteryVoltageRawAdc;
extern int16 zclFlowerApp_Temperature_Sensor_MeasuredValue;
extern int16 zclFlowerApp_PressureSensor_MeasuredValue;
extern uint32 zclFlowerApp_PressureSensor_MeasuredValueHPA;
extern uint16 zclFlowerApp_HumiditySensor_MeasuredValue;
extern int16 zclFlowerApp_DS18B20_MeasuredValue;
extern uint16 zclFlowerApp_SoilHumiditySensor_MeasuredValue;
extern uint16 zclFlowerApp_SoilHumiditySensor_MeasuredValueRawAdc;
extern uint16 zclFlowerApp_IlluminanceSensor_MeasuredValue;
extern uint16 zclFlowerApp_IlluminanceSensor_MeasuredValueRawAdc;

// attribute list
extern CONST zclAttrRec_t zclFlowerApp_AttrsFirstEP[];
extern CONST zclAttrRec_t zclFlowerApp_AttrsSecondEP[];
extern CONST uint8 zclFlowerApp_AttrsSecondEPCount;
extern CONST uint8 zclFlowerApp_AttrsFirstEPCount;


extern const uint8 zclFlowerApp_ManufacturerName[];
extern const uint8 zclFlowerApp_ModelId[];
extern const uint8 zclFlowerApp_PowerSource;

// FLOWER_APP_TODO: Declare application specific attributes here

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialization for the task
 */
extern void zclFlowerApp_Init(byte task_id);
extern byte zclFlowerApp_KeyCodeToButton(byte key);

/*
 *  Event Process for the task
 */
extern UINT16 zclFlowerApp_event_loop(byte task_id, UINT16 events);

void user_delay_ms(uint32_t period);

#ifdef __cplusplus
}
#endif

#endif /* ZCL_FLOWER_APP_H */
