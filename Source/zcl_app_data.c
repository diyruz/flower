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

#define FLOWER_APP_DEVICE_VERSION 2
#define FLOWER_APP_FLAGS 0

#define FLOWER_APP_HWVERSION 1
#define FLOWER_APP_ZCLVERSION 1

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
const uint16 zclFlowerApp_clusterRevision_all = 0x0001;
uint8 zclFlowerApp_BatteryVoltage = 0xff;
uint16 zclFlowerApp_BatteryVoltageRawAdc = 0xff;
uint8 zclFlowerApp_BatteryPercentageRemainig = 0xff;

int16 zclFlowerApp_Temperature_Sensor_MeasuredValue = 0xff;
int16 zclFlowerApp_PressureSensor_MeasuredValue = 0xff;
uint32 zclFlowerApp_PressureSensor_MeasuredValueHPA = 0xff;
uint16 zclFlowerApp_HumiditySensor_MeasuredValue = 0xff;
uint16 zclFlowerApp_SoilHumiditySensor_MeasuredValue = 0xff;
uint16 zclFlowerApp_SoilHumiditySensor_MeasuredValueRawAdc = 0xff;
int16 zclFlowerApp_DS18B20_MeasuredValue = 0xff;

uint16 zclFlowerApp_IlluminanceSensor_MeasuredValue = 0xff;
uint16 zclFlowerApp_IlluminanceSensor_MeasuredValueRawAdc = 0xff;

// Basic Cluster
const uint8 zclFlowerApp_HWRevision = FLOWER_APP_HWVERSION;
const uint8 zclFlowerApp_ZCLVersion = FLOWER_APP_ZCLVERSION;
const uint8 zclFlowerApp_ApplicationVersion = 2;
const uint8 zclFlowerApp_StackVersion = 4;

//{lenght, 'd', 'a', 't', 'a'}
const uint8 zclFlowerApp_ManufacturerName[] = {9, 'm', 'o', 'd', 'k', 'a', 'm', '.', 'r', 'u'};
const uint8 zclFlowerApp_ModelId[] = {13, 'D', 'I', 'Y', 'R', 'u', 'Z', '_', 'F', 'l', 'o', 'w', 'e', 'r'};
const uint8 zclFlowerApp_PowerSource = POWER_SOURCE_BATTERY;

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
CONST zclAttrRec_t zclFlowerApp_AttrsFirstEP[] = {
    {BASIC, {ATTRID_BASIC_APPL_VERSION, ZCL_UINT8, R, (void *)&zclFlowerApp_ApplicationVersion}},
    {BASIC, {ATTRID_BASIC_STACK_VERSION, ZCL_UINT8, R, (void *)&zclFlowerApp_StackVersion}},
    {BASIC, {ATTRID_BASIC_HW_VERSION, ZCL_UINT8, R, (void *)&zclFlowerApp_HWRevision}},
    {BASIC, {ATTRID_BASIC_ZCL_VERSION, ZCL_UINT8, R, (void *)&zclFlowerApp_ZCLVersion}},
    {BASIC, {ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR, R, (void *)zclFlowerApp_ManufacturerName}},
    {BASIC, {ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclFlowerApp_ModelId}},
    {BASIC, {ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_ENUM8, R, (void *)&zclFlowerApp_PowerSource}},
    {BASIC, {ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, R, (void *)&zclFlowerApp_clusterRevision_all}},
    {BASIC, {ATTRID_BASIC_DATE_CODE, ZCL_DATATYPE_CHAR_STR, R, (void *)zclFlowerApp_DateCode}},
    {BASIC, {ATTRID_BASIC_SW_BUILD_ID, ZCL_UINT8, R, (void *)&zclFlowerApp_ApplicationVersion}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_VOLTAGE, ZCL_UINT8, RR, (void *)&zclFlowerApp_BatteryVoltage}},
/**
 * FYI: calculating battery percentage can be tricky, since this device can be powered from 2xAA or 1xCR2032 batteries
 * */
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, ZCL_UINT8, RR, (void *)&zclFlowerApp_BatteryPercentageRemainig}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_VOLTAGE_RAW_ADC, ZCL_UINT16, RR, (void *)&zclFlowerApp_BatteryVoltageRawAdc}},


    {ILLUMINANCE, {ATTRID_MS_ILLUMINANCE_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclFlowerApp_IlluminanceSensor_MeasuredValue}},
    {TEMP, {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_INT16, RR, (void *)&zclFlowerApp_Temperature_Sensor_MeasuredValue}},
    {PRESSURE, {ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE, ZCL_INT16, RR, (void *)&zclFlowerApp_PressureSensor_MeasuredValue}},
    {PRESSURE, {ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE_HPA, ZCL_UINT32, RR, (void *)&zclFlowerApp_PressureSensor_MeasuredValueHPA}},
    {HUMIDITY, {ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclFlowerApp_HumiditySensor_MeasuredValue}}
};

/**
 * FYI: ATTRID_POWER_CFG_BATTERY_VOLTAGE_RAW_ADC and ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE_RAW_ADC
 * can be used to calculate relative humidity in converter
*/

CONST zclAttrRec_t zclFlowerApp_AttrsSecondEP[] = {
    {TEMP, {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_INT16, RR, (void *)&zclFlowerApp_DS18B20_MeasuredValue}},
    {HUMIDITY, {ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclFlowerApp_SoilHumiditySensor_MeasuredValue}}
};
uint8 CONST zclFlowerApp_AttrsSecondEPCount = (sizeof(zclFlowerApp_AttrsSecondEP) / sizeof(zclFlowerApp_AttrsSecondEP[0]));
uint8 CONST zclFlowerApp_AttrsFirstEPCount = (sizeof(zclFlowerApp_AttrsFirstEP) / sizeof(zclFlowerApp_AttrsFirstEP[0]));

const cId_t zclFlowerApp_InClusterList[] = {ZCL_CLUSTER_ID_GEN_BASIC};

#define FLOWER_APP_MAX_INCLUSTERS (sizeof(zclFlowerApp_InClusterList) / sizeof(zclFlowerApp_InClusterList[0]))

const cId_t zclFlowerApp_OutClusterListFirstEP[] = {POWER_CFG, ILLUMINANCE, TEMP, PRESSURE, HUMIDITY};
const cId_t zclFlowerApp_OutClusterListSecondEP[] = {TEMP, HUMIDITY};

#define FLOWER_APP_MAX_OUTCLUSTERS_FIRST_EP (sizeof(zclFlowerApp_OutClusterListFirstEP) / sizeof(zclFlowerApp_OutClusterListFirstEP[0]))
#define FLOWER_APP_MAX_OUTCLUSTERS_SECOND_EP (sizeof(zclFlowerApp_OutClusterListSecondEP) / sizeof(zclFlowerApp_OutClusterListSecondEP[0]))





SimpleDescriptionFormat_t zclFlowerApp_FirstEP = {
    1,                                                  //  int Endpoint;
    ZCL_HA_PROFILE_ID,                                  //  uint16 AppProfId[2];
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,                      //  uint16 AppDeviceId[2];
    FLOWER_APP_DEVICE_VERSION,                          //  int   AppDevVer:4;
    FLOWER_APP_FLAGS,                                   //  int   AppFlags:4;
    FLOWER_APP_MAX_INCLUSTERS,                          //  byte  AppNumInClusters;
    (cId_t *)zclFlowerApp_InClusterList,                //  byte *pAppInClusterList;
    FLOWER_APP_MAX_OUTCLUSTERS_FIRST_EP,                //  byte  AppNumInClusters;
    (cId_t *)zclFlowerApp_OutClusterListFirstEP         //  byte *pAppInClusterList;
};


SimpleDescriptionFormat_t zclFlowerApp_SecondEP = {
    2,                                                  //  int Endpoint;
    ZCL_HA_PROFILE_ID,                                  //  uint16 AppProfId[2];
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,                      //  uint16 AppDeviceId[2];
    FLOWER_APP_DEVICE_VERSION,                          //  int   AppDevVer:4;
    FLOWER_APP_FLAGS,                                   //  int   AppFlags:4;
    0,                                                  //  byte  AppNumInClusters;
    (cId_t *)NULL,                                      //  byte *pAppInClusterList;
    FLOWER_APP_MAX_OUTCLUSTERS_SECOND_EP,               //  byte  AppNumInClusters;
    (cId_t *)zclFlowerApp_OutClusterListSecondEP        //  byte *pAppInClusterList;
};
byte zclFlowerApp_KeyCodeToButton(byte key) {
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
