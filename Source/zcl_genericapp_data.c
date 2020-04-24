#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"

#include "zcl_genericapp.h"

/*********************************************************************
 * CONSTANTS
 */

#define GENERICAPP_DEVICE_VERSION 0
#define GENERICAPP_FLAGS 0

#define GENERICAPP_HWVERSION 1
#define GENERICAPP_ZCLVERSION 1

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
const uint16 zclGenericApp_clusterRevision_all = 0x0001;
uint8 zclGenericApp_BatteryVoltage = 0x00;
uint8 zclGenericApp_BatteryPercentageRemainig = 0x00;

// Basic Cluster
const uint8 zclGenericApp_HWRevision = GENERICAPP_HWVERSION;
const uint8 zclGenericApp_ZCLVersion = GENERICAPP_ZCLVERSION;
const uint8 zclGenericApp_ManufacturerName[] = {4, 'T', 'e', 's', 't'};

const uint8 zclGenericApp_ModelId[] = {14, 'D', 'I', 'Y', 'R', 'u', 'Z', '_', 'F', 'r', 'e', 'e', 'P', 'a', 'd'};
const uint8 zclGenericApp_DateCode[] = {8,  '2', '0', '2', '0', '0', '4', '2', '4'};
const uint8 zclGenericApp_PowerSource = POWER_SOURCE_BATTERY;

uint8 zclGenericApp_LocationDescription[17] = {16,  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                                               ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
uint8 zclGenericApp_PhysicalEnvironment = 0;
uint8 zclGenericApp_DeviceEnable = DEVICE_ENABLED;

// Identify Cluster
uint16 zclGenericApp_IdentifyTime;


/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
CONST zclAttrRec_t zclGenericApp_Attrs[] = {
    // *** General Basic Cluster Attributes ***
    {ZCL_CLUSTER_ID_GEN_BASIC, // Cluster IDs - defined in the foundation (ie.
                               // zcl.h)
     {
         // Attribute record
         ATTRID_BASIC_HW_VERSION,          // Attribute ID - Found in Cluster Library
                                           // header (ie. zcl_general.h)
         ZCL_DATATYPE_UINT8,               // Data Type - found in zcl.h
         ACCESS_CONTROL_READ,              // Variable access control - found in zcl.h
         (void *)&zclGenericApp_HWRevision // Pointer to attribute variable
     }},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_ZCL_VERSION, ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ, (void *)&zclGenericApp_ZCLVersion}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zclGenericApp_ManufacturerName}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ, (void *)zclGenericApp_ModelId}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_DATE_CODE, ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ, (void *)zclGenericApp_DateCode}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_ENUM8, ACCESS_CONTROL_READ, (void *)&zclGenericApp_PowerSource}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_LOCATION_DESC, ZCL_DATATYPE_CHAR_STR, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)zclGenericApp_LocationDescription}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_PHYSICAL_ENV, ZCL_DATATYPE_ENUM8, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclGenericApp_PhysicalEnvironment}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_DEVICE_ENABLED, ZCL_DATATYPE_BOOLEAN, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclGenericApp_DeviceEnable}},

#ifdef ZCL_IDENTIFY
    // *** Identify Cluster Attribute ***
    {ZCL_CLUSTER_ID_GEN_IDENTIFY,
     {// Attribute record
      ATTRID_IDENTIFY_TIME, ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclGenericApp_IdentifyTime}},
#endif
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, ACCESS_CONTROL_READ, (void *)&zclGenericApp_clusterRevision_all}},

    {ZCL_CLUSTER_ID_GEN_POWER_CFG,
     {// Attribute record
      ATTRID_POWER_CFG_BATTERY_VOLTAGE, ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zclGenericApp_BatteryVoltage}},

    {ZCL_CLUSTER_ID_GEN_POWER_CFG,
     {// Attribute record
      ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zclGenericApp_BatteryPercentageRemainig}},

    {ZCL_CLUSTER_ID_GEN_IDENTIFY,
     {// Attribute record
      ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, ACCESS_CONTROL_READ, (void *)&zclGenericApp_clusterRevision_all}},
};

uint8 CONST zclGenericApp_NumAttributes = (sizeof(zclGenericApp_Attrs) / sizeof(zclGenericApp_Attrs[0]));

const cId_t zclSampleSw_InClusterList[] = {ZCL_CLUSTER_ID_GEN_BASIC, ZCL_CLUSTER_ID_GEN_IDENTIFY,
                                           ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG};

#define ZCLSAMPLESW_MAX_INCLUSTERS (sizeof(zclSampleSw_InClusterList) / sizeof(zclSampleSw_InClusterList[0]))

const cId_t zclSampleSw_OutClusterList[] = {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    ZCL_CLUSTER_ID_GEN_GROUPS,
};

#define ZCLSAMPLESW_MAX_OUTCLUSTERS (sizeof(zclSampleSw_OutClusterList) / sizeof(zclSampleSw_OutClusterList[0]))

SimpleDescriptionFormat_t zclGenericApp_SimpleDescs[] = {
    {
        1,                                   //  int Endpoint;
        ZCL_HA_PROFILE_ID,                   //  uint16 AppProfId[2];
        ZCL_HA_DEVICEID_ON_OFF_LIGHT_SWITCH, //  uint16 AppDeviceId[2];
        GENERICAPP_DEVICE_VERSION,           //  int   AppDevVer:4;
        GENERICAPP_FLAGS,                    //  int   AppFlags:4;
        ZCLSAMPLESW_MAX_INCLUSTERS,          //  byte  AppNumInClusters;
        (cId_t *)zclSampleSw_InClusterList,  //  byte *pAppInClusterList;
        ZCLSAMPLESW_MAX_OUTCLUSTERS,         //  byte  AppNumInClusters;
        (cId_t *)zclSampleSw_OutClusterList  //  byte *pAppInClusterList;
    },
    {
        2,                                   //  int Endpoint;
        ZCL_HA_PROFILE_ID,                   //  uint16 AppProfId[2];
        ZCL_HA_DEVICEID_ON_OFF_LIGHT_SWITCH, //  uint16 AppDeviceId[2];
        GENERICAPP_DEVICE_VERSION,           //  int   AppDevVer:4;
        GENERICAPP_FLAGS,                    //  int   AppFlags:4;
        ZCLSAMPLESW_MAX_INCLUSTERS,          //  byte  AppNumInClusters;
        (cId_t *)zclSampleSw_InClusterList,  //  byte *pAppInClusterList;
        ZCLSAMPLESW_MAX_OUTCLUSTERS,         //  byte  AppNumInClusters;
        (cId_t *)zclSampleSw_OutClusterList  //  byte *pAppInClusterList;
    },
};
uint8 zclGenericApp_SimpleDescsCount = (sizeof(zclGenericApp_SimpleDescs) / sizeof(zclGenericApp_SimpleDescs[0]));

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

void zclGenericApp_ResetAttributesToDefaultValues(void) {
    int i;

    zclGenericApp_LocationDescription[0] = 16;
    for (i = 1; i <= 16; i++) {
        zclGenericApp_LocationDescription[i] = ' ';
    }

    zclGenericApp_PhysicalEnvironment = PHY_UNSPECIFIED_ENV;
    zclGenericApp_DeviceEnable = DEVICE_ENABLED;

#ifdef ZCL_IDENTIFY
    zclGenericApp_IdentifyTime = 0;
#endif

    /* GENERICAPP_TODO: initialize cluster attribute variables. */
}

/****************************************************************************
****************************************************************************/
