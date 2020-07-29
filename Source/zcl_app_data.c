#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ms.h"
#include "zcl_ha.h"

#include "zcl_app.h"

#include "battery.h"
#include "version.h"
/*********************************************************************
 * CONSTANTS
 */

#define APP_DEVICE_VERSION 2
#define APP_FLAGS 0

#define APP_HWVERSION 1
#define APP_ZCLVERSION 1

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Global attributes
const uint16 zclApp_clusterRevision_all = 0x0001;
uint8 zclApp_BatteryVoltage = 0xff;
uint16 zclApp_BatteryVoltageRawAdc = 0xff;
uint8 zclApp_BatteryPercentageRemainig = 0xff;

int16 zclApp_Temperature_Sensor_MeasuredValue = 0x8000;
int16 zclApp_PressureSensor_MeasuredValue = 0x8000;
uint32 zclApp_PressureSensor_MeasuredValueHPA = 0x8000;
uint16 zclApp_HumiditySensor_MeasuredValue = 0xffff;

uint16 zclApp_SoilHumiditySensor_MeasuredValue = 0xffff;
uint16 zclApp_SoilHumiditySensor_MeasuredValueRawAdc = 0xffff;

int16 zclApp_DS18B20_MeasuredValue = 0x8000;

uint16 zclApp_IlluminanceSensor_MeasuredValue = 0xffff;
uint16 zclApp_IlluminanceSensor_MeasuredValueRawAdc = 0xffff;

// Basic Cluster
const uint8 zclApp_HWRevision = APP_HWVERSION;
const uint8 zclApp_ZCLVersion = APP_ZCLVERSION;
const uint8 zclApp_ApplicationVersion = 2;
const uint8 zclApp_StackVersion = 4;

//{lenght, 'd', 'a', 't', 'a'}
const uint8 zclApp_ManufacturerName[] = {9, 'm', 'o', 'd', 'k', 'a', 'm', '.', 'r', 'u'};
const uint8 zclApp_ModelId[] = {13, 'D', 'I', 'Y', 'R', 'u', 'Z', '_', 'F', 'l', 'o', 'w', 'e', 'r'};
const uint8 zclApp_PowerSource = POWER_SOURCE_BATTERY;

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */


// msTemperatureMeasurement int16
// msRelativeHumidity:  uint16
// msPressureMeasurement   int16
// msIlluminanceMeasurement uint16
// #define ZCL_CLUSTER_ID_GEN_BASIC                             0x0000
// #define ZCL_CLUSTER_ID_GEN_POWER_CFG                         0x0001
// #define ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT            0x0400
// #define ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT            0x0402
// #define ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT               0x0403
// #define ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY                  0x0405
CONST zclAttrRec_t zclApp_AttrsFirstEP[] = {
    {BASIC, {ATTRID_BASIC_APPL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ApplicationVersion}},
    {BASIC, {ATTRID_BASIC_STACK_VERSION, ZCL_UINT8, R, (void *)&zclApp_StackVersion}},
    {BASIC, {ATTRID_BASIC_HW_VERSION, ZCL_UINT8, R, (void *)&zclApp_HWRevision}},
    {BASIC, {ATTRID_BASIC_ZCL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ZCLVersion}},
    {BASIC, {ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ManufacturerName}},
    {BASIC, {ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ModelId}},
    {BASIC, {ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_ENUM8, R, (void *)&zclApp_PowerSource}},
    {BASIC, {ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, R, (void *)&zclApp_clusterRevision_all}},
    {BASIC, {ATTRID_BASIC_DATE_CODE, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_DateCode}},
    {BASIC, {ATTRID_BASIC_SW_BUILD_ID, ZCL_UINT8, R, (void *)&zclApp_ApplicationVersion}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_VOLTAGE, ZCL_UINT8, RR, (void *)&zclApp_BatteryVoltage}},
/**
 * FYI: calculating battery percentage can be tricky, since this device can be powered from 2xAA or 1xCR2032 batteries
 * */
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, ZCL_UINT8, RR, (void *)&zclApp_BatteryPercentageRemainig}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_VOLTAGE_RAW_ADC, ZCL_UINT16, RR, (void *)&zclApp_BatteryVoltageRawAdc}},


    {ILLUMINANCE, {ATTRID_MS_ILLUMINANCE_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclApp_IlluminanceSensor_MeasuredValue}},
    {TEMP, {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_INT16, RR, (void *)&zclApp_Temperature_Sensor_MeasuredValue}},
    {PRESSURE, {ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE, ZCL_INT16, RR, (void *)&zclApp_PressureSensor_MeasuredValue}},
    {PRESSURE, {ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE_HPA, ZCL_UINT32, RR, (void *)&zclApp_PressureSensor_MeasuredValueHPA}},
    {HUMIDITY, {ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclApp_HumiditySensor_MeasuredValue}}
};

/**
 * FYI: ATTRID_POWER_CFG_BATTERY_VOLTAGE_RAW_ADC and ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE_RAW_ADC
 * can be used to calculate relative humidity in converter
*/

CONST zclAttrRec_t zclApp_AttrsSecondEP[] = {
    {TEMP, {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_INT16, RR, (void *)&zclApp_DS18B20_MeasuredValue}},
    {HUMIDITY, {ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclApp_SoilHumiditySensor_MeasuredValue}}
};
uint8 CONST zclApp_AttrsSecondEPCount = (sizeof(zclApp_AttrsSecondEP) / sizeof(zclApp_AttrsSecondEP[0]));
uint8 CONST zclApp_AttrsFirstEPCount = (sizeof(zclApp_AttrsFirstEP) / sizeof(zclApp_AttrsFirstEP[0]));

const cId_t zclApp_InClusterList[] = {ZCL_CLUSTER_ID_GEN_BASIC};

#define APP_MAX_INCLUSTERS (sizeof(zclApp_InClusterList) / sizeof(zclApp_InClusterList[0]))

const cId_t zclApp_OutClusterListFirstEP[] = {POWER_CFG, ILLUMINANCE, TEMP, PRESSURE, HUMIDITY};
const cId_t zclApp_OutClusterListSecondEP[] = {TEMP, HUMIDITY};

#define APP_MAX_OUTCLUSTERS_FIRST_EP (sizeof(zclApp_OutClusterListFirstEP) / sizeof(zclApp_OutClusterListFirstEP[0]))
#define APP_MAX_OUTCLUSTERS_SECOND_EP (sizeof(zclApp_OutClusterListSecondEP) / sizeof(zclApp_OutClusterListSecondEP[0]))





SimpleDescriptionFormat_t zclApp_FirstEP = {
    1,                                                  //  int Endpoint;
    ZCL_HA_PROFILE_ID,                                  //  uint16 AppProfId[2];
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,                      //  uint16 AppDeviceId[2];
    APP_DEVICE_VERSION,                          //  int   AppDevVer:4;
    APP_FLAGS,                                   //  int   AppFlags:4;
    APP_MAX_INCLUSTERS,                          //  byte  AppNumInClusters;
    (cId_t *)zclApp_InClusterList,                //  byte *pAppInClusterList;
    APP_MAX_OUTCLUSTERS_FIRST_EP,                //  byte  AppNumInClusters;
    (cId_t *)zclApp_OutClusterListFirstEP         //  byte *pAppInClusterList;
};


SimpleDescriptionFormat_t zclApp_SecondEP = {
    2,                                                  //  int Endpoint;
    ZCL_HA_PROFILE_ID,                                  //  uint16 AppProfId[2];
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,                      //  uint16 AppDeviceId[2];
    APP_DEVICE_VERSION,                          //  int   AppDevVer:4;
    APP_FLAGS,                                   //  int   AppFlags:4;
    0,                                                  //  byte  AppNumInClusters;
    (cId_t *)NULL,                                      //  byte *pAppInClusterList;
    APP_MAX_OUTCLUSTERS_SECOND_EP,               //  byte  AppNumInClusters;
    (cId_t *)zclApp_OutClusterListSecondEP        //  byte *pAppInClusterList;
};
byte zclApp_KeyCodeToButton(byte key) {
    switch (key) {

#if defined(HAL_BOARD_CHDTECH_DEV)
    case 0x1: // row=4 col=4
        return 1;
    case 0x2: // row=4 col=8
        return 2;
#endif

    default:
        return HAL_UNKNOWN_BUTTON;
        break;
    }
}
