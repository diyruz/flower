#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"

#include "zcl_freepadapp.h"

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

// Basic Cluster
const uint8 zclFreePadApp_HWRevision = FREEPADAPP_HWVERSION;
const uint8 zclFreePadApp_ZCLVersion = FREEPADAPP_ZCLVERSION;
//{lenght, 'd', 'a', 't', 'a'}
const uint8 zclFreePadApp_ManufacturerName[] = {9, 'm', 'o', 'd', 'k', 'a', 'm', '.', 'r', 'u'};
const uint8 zclFreePadApp_ManufacturerNameNT[] = "modkam.ru";

const uint8 zclFreePadApp_ModelId[] = {14, 'D', 'I', 'Y', 'R', 'u', 'Z', '_', 'F', 'r', 'e', 'e', 'P', 'a', 'd'};
const uint8 zclFreePadApp_ModelIdNT[] = "DIYRuZ_FreePad";
const uint8 zclFreePadApp_PowerSource = POWER_SOURCE_BATTERY;


/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
CONST zclAttrRec_t zclFreePadApp_Attrs[] = {
    // *** General Basic Cluster Attributes ***
    {ZCL_CLUSTER_ID_GEN_BASIC, // Cluster IDs - defined in the foundation (ie.
                               // zcl.h)
     {
         // Attribute record
         ATTRID_BASIC_HW_VERSION,          // Attribute ID - Found in Cluster Library
                                           // header (ie. zcl_general.h)
         ZCL_DATATYPE_UINT8,               // Data Type - found in zcl.h
         ACCESS_CONTROL_READ,              // Variable access control - found in zcl.h
         (void *)&zclFreePadApp_HWRevision // Pointer to attribute variable
     }},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_ZCL_VERSION, ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ, (void *)&zclFreePadApp_ZCLVersion}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)&zclFreePadApp_ManufacturerName}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ, (void *)&zclFreePadApp_ModelId}},

    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_ENUM8, ACCESS_CONTROL_READ, (void *)&zclFreePadApp_PowerSource}},


    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, ACCESS_CONTROL_READ, (void *)&zclFreePadApp_clusterRevision_all}},

        {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)zclFreePadApp_DateCode
    }
  },

    {ZCL_CLUSTER_ID_GEN_POWER_CFG,
     {// Attribute record
      ATTRID_POWER_CFG_BATTERY_VOLTAGE, ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zclFreePadApp_BatteryVoltage}},

    {ZCL_CLUSTER_ID_GEN_POWER_CFG,
     {// Attribute record
      ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zclFreePadApp_BatteryPercentageRemainig}}
};

uint8 CONST zclFreePadApp_NumAttributes = (sizeof(zclFreePadApp_Attrs) / sizeof(zclFreePadApp_Attrs[0]));

const cId_t zclSampleSw_InClusterList[] = {ZCL_CLUSTER_ID_GEN_BASIC};

#define ZCLSAMPLESW_MAX_INCLUSTERS (sizeof(zclSampleSw_InClusterList) / sizeof(zclSampleSw_InClusterList[0]))

const cId_t zclSampleSw_OutClusterListOdd[] = {ZCL_CLUSTER_ID_GEN_ON_OFF, ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL};
const cId_t zclSampleSw_OutClusterListEven[] = {ZCL_CLUSTER_ID_GEN_ON_OFF};

#define ZCLSAMPLESW_MAX_OUTCLUSTERS_EVEN                                                                               \
    (sizeof(zclSampleSw_OutClusterListEven) / sizeof(zclSampleSw_OutClusterListEven[0]))
#define ZCLSAMPLESW_MAX_OUTCLUSTERS_ODD                                                                                \
    (sizeof(zclSampleSw_OutClusterListOdd) / sizeof(zclSampleSw_OutClusterListOdd[0]))

SimpleDescriptionFormat_t zclFreePadApp_SimpleDescs[FREEPAD_BUTTONS_COUNT];

void zclFreePadApp_InitClusters(void) {
    for (int i = 0; i < (int)FREEPAD_BUTTONS_COUNT; i++) {
        uint8 endPoint = i + 1;
        zclFreePadApp_SimpleDescs[i].EndPoint = endPoint;
        zclFreePadApp_SimpleDescs[i].AppProfId = ZCL_HA_PROFILE_ID;
        zclFreePadApp_SimpleDescs[i].AppDeviceId = ZCL_HA_DEVICEID_REMOTE_CONTROL;
        zclFreePadApp_SimpleDescs[i].AppDevVer = FREEPADAPP_DEVICE_VERSION;
        zclFreePadApp_SimpleDescs[i].Reserved = FREEPADAPP_FLAGS; // AF_V1_SUPPORT uses for AppFlags:4.
        zclFreePadApp_SimpleDescs[i].AppNumInClusters = ZCLSAMPLESW_MAX_INCLUSTERS;
        zclFreePadApp_SimpleDescs[i].pAppInClusterList = (cId_t *)zclSampleSw_InClusterList;
        if (endPoint % 2 == 0) {
            zclFreePadApp_SimpleDescs[i].AppNumOutClusters = ZCLSAMPLESW_MAX_OUTCLUSTERS_EVEN;
            zclFreePadApp_SimpleDescs[i].pAppOutClusterList = (cId_t *)zclSampleSw_OutClusterListEven;
        } else {
            zclFreePadApp_SimpleDescs[i].AppNumOutClusters = ZCLSAMPLESW_MAX_OUTCLUSTERS_ODD;
            zclFreePadApp_SimpleDescs[i].pAppOutClusterList = (cId_t *)zclSampleSw_OutClusterListOdd;
        }
    }
}
uint8 zclFreePadApp_SimpleDescsCount = FREEPAD_BUTTONS_COUNT;

byte zclFreePadApp_KeyCodeToButton(byte key) {
    switch (key) {
#if defined(HAL_BOARD_FREEPAD_20) || defined(HAL_BOARD_FREEPAD_12) || defined(HAL_BOARD_FREEPAD_8)
    case 0x9: // row=4 col=4
        return 1;
    case 0xa: // row=4 col=8
        return 2;
    case 0xc: // row=4 col=16
        return 3;
    case 0x8: // row=4 col=32
        return 4;
    case 0x11: // row=8 col=4
        return 5;
    case 0x12: // row=8 col=8
        return 6;
    case 0x14: // row=8 col=16
        return 7;
    case 0x18: // row=8 col=32
        return 8;
    #if defined(HAL_BOARD_FREEPAD_12) || defined(HAL_BOARD_FREEPAD_20)
        case 0x21: // row=16 col=4
            return 9;
        case 0x22: // row=16 col=8
            return 10;
        case 0x24: // row=16 col=16
            return 11;
        case 0x28: // row=16 col=32
            return 12;
    #endif
    #if defined(HAL_BOARD_FREEPAD_20)
        case 0x41: // row=32 col=4
            return 13;
        case 0x42: // row=32 col=8
            return 14;
        case 0x44: // row=32 col=16
            return 15;
        case 0x48: // row=32 col=32
            return 16;
        case 0x81: // row=64 col=4
            return 17;
        case 0x82: // row=64 col=8
            return 18;
        case 0x84: // row=64 col=16
            return 19;
        case 0x88: // row=64 col=32
            return 20;
    #endif
#elif defined(HAL_BOARD_CHDTECH_DEV)
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