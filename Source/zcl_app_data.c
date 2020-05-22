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
uint8 zclFlowerApp_BatteryPercentageRemainig = 0xff;

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
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, ZCL_UINT8, RR, (void *)&zclFlowerApp_BatteryPercentageRemainig}},

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


CONST zclAttrRec_t zclFlowerApp_AttrsSecondEP[] = {
    {TEMP, {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclSampleTemperatureSensorDS18B20_MeasuredValue}},
    {TEMP, {ATTRID_MS_TEMPERATURE_MIN_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclSampleTemperatureSensorDS18B20_MinMeasuredValue}},
    {TEMP, {ATTRID_MS_TEMPERATURE_MAX_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclSampleTemperatureSensorDS18B20_MaxMeasuredValue}},

};
uint8 CONST zclFlowerApp_AttrsSecondEPCount = (sizeof(zclFlowerApp_AttrsSecondEP) / sizeof(zclFlowerApp_AttrsSecondEP[0]));
uint8 CONST zclFlowerApp_AttrsFirstEPCount = (sizeof(zclFlowerApp_AttrsFirstEP) / sizeof(zclFlowerApp_AttrsFirstEP[0]));

const cId_t zclFlowerApp_InClusterList[] = {ZCL_CLUSTER_ID_GEN_BASIC};

#define FLOWER_APP_MAX_INCLUSTERS (sizeof(zclFlowerApp_InClusterList) / sizeof(zclFlowerApp_InClusterList[0]))

const cId_t zclFlowerApp_OutClusterListFirstEP[] = {POWER_CFG, TEMP, PRESSURE, HUMIDITY, ILLUMINANCE};
const cId_t zclFlowerApp_OutClusterListSecondEP[] = {TEMP};

#define FLOWER_APP_MAX_OUTCLUSTERS_FIRST_EP (sizeof(zclFlowerApp_OutClusterListFirstEP) / sizeof(zclFlowerApp_OutClusterListFirstEP[0]))
#define FLOWER_APP_MAX_OUTCLUSTERS_SECOND_EP (sizeof(zclFlowerApp_OutClusterListSecondEP) / sizeof(zclFlowerApp_OutClusterListSecondEP[0]))





SimpleDescriptionFormat_t zclFlowerApp_FirstEP = {
    1,                                                 //  int Endpoint;
    ZCL_HA_PROFILE_ID,                                 //  uint16 AppProfId[2];
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,                     //  uint16 AppDeviceId[2];
    FLOWER_APP_DEVICE_VERSION,                         //  int   AppDevVer:4;
    FLOWER_APP_FLAGS,                                  //  int   AppFlags:4;
    FLOWER_APP_MAX_INCLUSTERS,                        //  byte  AppNumInClusters;
    (cId_t *)zclFlowerApp_InClusterList,                //  byte *pAppInClusterList;
    FLOWER_APP_MAX_OUTCLUSTERS_FIRST_EP,              //  byte  AppNumInClusters;
    (cId_t *)zclFlowerApp_OutClusterListFirstEP         //  byte *pAppInClusterList;
};


SimpleDescriptionFormat_t zclFlowerApp_SecondEP = {
    2,                                                 //  int Endpoint;
    ZCL_HA_PROFILE_ID,                                 //  uint16 AppProfId[2];
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,                     //  uint16 AppDeviceId[2];
    FLOWER_APP_DEVICE_VERSION,                         //  int   AppDevVer:4;
    FLOWER_APP_FLAGS,                                  //  int   AppFlags:4;
    0,                                                //  byte  AppNumInClusters;
    (cId_t *)NULL,                                      //  byte *pAppInClusterList;
    FLOWER_APP_MAX_OUTCLUSTERS_SECOND_EP,        //  byte  AppNumInClusters;
    (cId_t *)zclFlowerApp_OutClusterListSecondEP //  byte *pAppInClusterList;
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
