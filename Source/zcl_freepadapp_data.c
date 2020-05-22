#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ms.h"
#include "zcl_ha.h"

#include "zcl_freepadapp.h"

#include "battery.h"
#include "version.h"
/*********************************************************************
 * CONSTANTS
 */

#define FREEPADAPP_DEVICE_VERSION 2
#define FREEPADAPP_FLAGS 0

#define FREEPADAPP_HWVERSION 1
#define FREEPADAPP_ZCLVERSION 1

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
const uint16 zclFreePadApp_clusterRevision_all = 0x0001;
uint8 zclFreePadApp_BatteryVoltage = 0xff;
uint8 zclFreePadApp_BatteryPercentageRemainig = 0xff;

uint16 zclSampleTemperatureSensor_MeasuredValue = 0x22;
const int16 zclSampleTemperatureSensor_MinMeasuredValue = SAMPLETEMPERATURESENSOR_MIN_MEASURED_VALUE;
const uint16 zclSampleTemperatureSensor_MaxMeasuredValue = SAMPLETEMPERATURESENSOR_MAX_MEASURED_VALUE;

uint16 zclSampleTemperatureSensorDS18B20_MeasuredValue = 0x22;
const int16 zclSampleTemperatureSensorDS18B20_MinMeasuredValue = SAMPLETEMPERATURESENSOR_MIN_MEASURED_VALUE;
const uint16 zclSampleTemperatureSensorDS18B20_MaxMeasuredValue = SAMPLETEMPERATURESENSOR_MAX_MEASURED_VALUE;

uint16 zclSamplePressureSensor_MeasuredValue = 0xff;
const int16 zclSamplePressureSensor_MinMeasuredValue = SAMPLE_PRESSURE_SENSOR_MIN_MEASURED_VALUE;
const uint16 zclSamplePressureSensor_MaxMeasuredValue = SAMPLE_PRESSURE_SENSOR_MAX_MEASURED_VALUE;

uint16 zclSampleHumiditySensor_MeasuredValue = 0xff;
const int16 zclSampleHumiditySensor_MinMeasuredValue = SAMPLE_HUMODITY_SENSOR_MIN_MEASURED_VALUE;
const uint16 zclSampleHumiditySensor_MaxMeasuredValue = SAMPLE_HUMODITY_SENSOR_MAX_MEASURED_VALUE;

// Basic Cluster
const uint8 zclFreePadApp_HWRevision = FREEPADAPP_HWVERSION;
const uint8 zclFreePadApp_ZCLVersion = FREEPADAPP_ZCLVERSION;
const uint8 zclFreePadApp_ApplicationVersion = 2;
const uint8 zclFreePadApp_StackVersion = 4;

//{lenght, 'd', 'a', 't', 'a'}
const uint8 zclFreePadApp_ManufacturerName[] = {9, 'm', 'o', 'd', 'k', 'a', 'm', '.', 'r', 'u'};
const uint8 zclFreePadApp_ModelId[] = {13, 'D', 'I', 'Y', 'R', 'u', 'Z', '_', 'F', 'l', 'o', 'w', 'e', 'r'};
const uint8 zclFreePadApp_PowerSource = POWER_SOURCE_BATTERY;

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */


CONST zclAttrRec_t zclFreePadApp_AttrsFirstEP[] = {
    {BASIC, {ATTRID_BASIC_APPL_VERSION, ZCL_UINT8, R, (void *)&zclFreePadApp_ApplicationVersion}},
    {BASIC, {ATTRID_BASIC_STACK_VERSION, ZCL_UINT8, R, (void *)&zclFreePadApp_StackVersion}},
    {BASIC, {ATTRID_BASIC_HW_VERSION, ZCL_UINT8, R, (void *)&zclFreePadApp_HWRevision}},
    {BASIC, {ATTRID_BASIC_ZCL_VERSION, ZCL_UINT8, R, (void *)&zclFreePadApp_ZCLVersion}},
    {BASIC, {ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR, R, (void *)zclFreePadApp_ManufacturerName}},
    {BASIC, {ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclFreePadApp_ModelId}},
    {BASIC, {ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_ENUM8, R, (void *)&zclFreePadApp_PowerSource}},
    {BASIC, {ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, R, (void *)&zclFreePadApp_clusterRevision_all}},
    {BASIC, {ATTRID_BASIC_DATE_CODE, ZCL_DATATYPE_CHAR_STR, R, (void *)zclFreePadApp_DateCode}},
    {BASIC, {ATTRID_BASIC_SW_BUILD_ID, ZCL_UINT8, R, (void *)&zclFreePadApp_ApplicationVersion}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_VOLTAGE, ZCL_UINT8, RR, (void *)&zclFreePadApp_BatteryVoltage}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, ZCL_UINT8, RR, (void *)&zclFreePadApp_BatteryPercentageRemainig}},

    // TODO: add ILLUMINANCE
    // {ILLUMINANCE, {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclSampleTemperatureSensor_MeasuredValue}},
    // {ILLUMINANCE, {ATTRID_MS_TEMPERATURE_MIN_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclSampleTemperatureSensor_MinMeasuredValue}},
    // {ILLUMINANCE, {ATTRID_MS_TEMPERATURE_MAX_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclSampleTemperatureSensor_MaxMeasuredValue}},

    {TEMP, {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclSampleTemperatureSensor_MeasuredValue}},
    {TEMP, {ATTRID_MS_TEMPERATURE_MIN_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclSampleTemperatureSensor_MinMeasuredValue}},
    {TEMP, {ATTRID_MS_TEMPERATURE_MAX_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclSampleTemperatureSensor_MaxMeasuredValue}},

    {PRESSURE, {ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclSamplePressureSensor_MeasuredValue}},
    {PRESSURE, {ATTRID_MS_PRESSURE_MEASUREMENT_MIN_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclSamplePressureSensor_MinMeasuredValue}},
    {PRESSURE, {ATTRID_MS_PRESSURE_MEASUREMENT_MAX_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclSamplePressureSensor_MaxMeasuredValue}},

    {HUMIDITY, {ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclSampleHumiditySensor_MeasuredValue}},
    {HUMIDITY, {ATTRID_MS_RELATIVE_HUMIDITY_MIN_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclSampleHumiditySensor_MinMeasuredValue}},
    {HUMIDITY, {ATTRID_MS_RELATIVE_HUMIDITY_MAX_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclSampleHumiditySensor_MaxMeasuredValue}},
};


CONST zclAttrRec_t zclFreePadApp_AttrsSecondEP[] = {
    {TEMP, {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclSampleTemperatureSensorDS18B20_MeasuredValue}},
    {TEMP, {ATTRID_MS_TEMPERATURE_MIN_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclSampleTemperatureSensorDS18B20_MinMeasuredValue}},
    {TEMP, {ATTRID_MS_TEMPERATURE_MAX_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclSampleTemperatureSensorDS18B20_MaxMeasuredValue}},

};
uint8 CONST zclFreePadApp_AttrsSecondEPCount = (sizeof(zclFreePadApp_AttrsSecondEP) / sizeof(zclFreePadApp_AttrsSecondEP[0]));
uint8 CONST zclFreePadApp_AttrsFirstEPCount = (sizeof(zclFreePadApp_AttrsFirstEP) / sizeof(zclFreePadApp_AttrsFirstEP[0]));

const cId_t zclFreePadApp_InClusterList[] = {ZCL_CLUSTER_ID_GEN_BASIC};

#define FREEPADAPP_MAX_INCLUSTERS (sizeof(zclFreePadApp_InClusterList) / sizeof(zclFreePadApp_InClusterList[0]))

const cId_t zclFreePadApp_OutClusterListFirstEP[] = {POWER_CFG, TEMP, PRESSURE, HUMIDITY, ILLUMINANCE};
const cId_t zclFreePadApp_OutClusterListSecondEP[] = {TEMP};

#define FREEPADAPP_MAX_OUTCLUSTERS_FIRST_EP (sizeof(zclFreePadApp_OutClusterListFirstEP) / sizeof(zclFreePadApp_OutClusterListFirstEP[0]))
#define FREEPADAPP_MAX_OUTCLUSTERS_SECOND_EP (sizeof(zclFreePadApp_OutClusterListSecondEP) / sizeof(zclFreePadApp_OutClusterListSecondEP[0]))





SimpleDescriptionFormat_t zclFreePadApp_FirstEP = {
    1,                                                 //  int Endpoint;
    ZCL_HA_PROFILE_ID,                                 //  uint16 AppProfId[2];
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,                     //  uint16 AppDeviceId[2];
    FREEPADAPP_DEVICE_VERSION,                         //  int   AppDevVer:4;
    FREEPADAPP_FLAGS,                                  //  int   AppFlags:4;
    FREEPADAPP_MAX_INCLUSTERS,                        //  byte  AppNumInClusters;
    (cId_t *)zclFreePadApp_InClusterList,                //  byte *pAppInClusterList;
    FREEPADAPP_MAX_OUTCLUSTERS_FIRST_EP,              //  byte  AppNumInClusters;
    (cId_t *)zclFreePadApp_OutClusterListFirstEP         //  byte *pAppInClusterList;
};


SimpleDescriptionFormat_t zclFreePadApp_SecondEP = {
    2,                                                 //  int Endpoint;
    ZCL_HA_PROFILE_ID,                                 //  uint16 AppProfId[2];
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,                     //  uint16 AppDeviceId[2];
    FREEPADAPP_DEVICE_VERSION,                         //  int   AppDevVer:4;
    FREEPADAPP_FLAGS,                                  //  int   AppFlags:4;
    0,                                                //  byte  AppNumInClusters;
    (cId_t *)NULL,                                      //  byte *pAppInClusterList;
    FREEPADAPP_MAX_OUTCLUSTERS_SECOND_EP,        //  byte  AppNumInClusters;
    (cId_t *)zclFreePadApp_OutClusterListSecondEP //  byte *pAppInClusterList;
};
byte zclFreePadApp_KeyCodeToButton(byte key) {
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
