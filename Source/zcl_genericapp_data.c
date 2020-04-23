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

// Basic Cluster
const uint8 zclGenericApp_HWRevision = GENERICAPP_HWVERSION;
const uint8 zclGenericApp_ZCLVersion = GENERICAPP_ZCLVERSION;
const uint8 zclGenericApp_ManufacturerName[] = {4, 'T', 'e', 's', 't'};
const uint8 zclGenericApp_ModelId[] = {16,  'T', 'I', '0', '0', '0',
                                       '1', ' ', ' ', ' ', ' ', ' ',
                                       ' ', ' ', ' ', ' ', ' '};
const uint8 zclGenericApp_DateCode[] = {16,  '2', '0', '0', '6', '0',
                                        '8', '3', '1', ' ', ' ', ' ',
                                        ' ', ' ', ' ', ' ', ' '};
const uint8 zclGenericApp_PowerSource = POWER_SOURCE_BATTERY;

uint8 zclGenericApp_LocationDescription[17] = {16,  ' ', ' ', ' ', ' ', ' ',
                                               ' ', ' ', ' ', ' ', ' ', ' ',
                                               ' ', ' ', ' ', ' ', ' '};
uint8 zclGenericApp_PhysicalEnvironment = 0;
uint8 zclGenericApp_DeviceEnable = DEVICE_ENABLED;

// Identify Cluster
uint16 zclGenericApp_IdentifyTime;

/* GENERICAPP_TODO: declare attribute variables here. If its value can change,
 * initialize it in zclGenericApp_ResetAttributesToDefaultValues. If its
 * value will not change, initialize it here.
 */

#if ZCL_DISCOVER
CONST zclCommandRec_t zclGenericApp_Cmds[] = {
    {ZCL_CLUSTER_ID_GEN_BASIC, COMMAND_BASIC_RESET_FACT_DEFAULT,
     CMD_DIR_SERVER_RECEIVED},

};

CONST uint8 zclCmdsArraySize =
    (sizeof(zclGenericApp_Cmds) / sizeof(zclGenericApp_Cmds[0]));
#endif // ZCL_DISCOVER

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
CONST zclAttrRec_t zclGenericApp_Attrs[] = {
    // *** General Basic Cluster Attributes ***
    {ZCL_CLUSTER_ID_GEN_BASIC, // Cluster IDs - defined in the foundation (ie.
                               // zcl.h)
     {
         // Attribute record
         ATTRID_BASIC_HW_VERSION, // Attribute ID - Found in Cluster Library
                                  // header (ie. zcl_general.h)
         ZCL_DATATYPE_UINT8,      // Data Type - found in zcl.h
         ACCESS_CONTROL_READ,     // Variable access control - found in zcl.h
         (void *)&zclGenericApp_HWRevision // Pointer to attribute variable
     }},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_ZCL_VERSION, ZCL_DATATYPE_UINT8, ACCESS_CONTROL_READ,
      (void *)&zclGenericApp_ZCLVersion}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ, (void *)zclGenericApp_ManufacturerName}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zclGenericApp_ModelId}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_DATE_CODE, ZCL_DATATYPE_CHAR_STR, ACCESS_CONTROL_READ,
      (void *)zclGenericApp_DateCode}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_ENUM8, ACCESS_CONTROL_READ,
      (void *)&zclGenericApp_PowerSource}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_LOCATION_DESC, ZCL_DATATYPE_CHAR_STR,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)zclGenericApp_LocationDescription}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_PHYSICAL_ENV, ZCL_DATATYPE_ENUM8,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclGenericApp_PhysicalEnvironment}},
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_BASIC_DEVICE_ENABLED, ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclGenericApp_DeviceEnable}},

#ifdef ZCL_IDENTIFY
    // *** Identify Cluster Attribute ***
    {ZCL_CLUSTER_ID_GEN_IDENTIFY,
     {// Attribute record
      ATTRID_IDENTIFY_TIME, ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&zclGenericApp_IdentifyTime}},
#endif
    {ZCL_CLUSTER_ID_GEN_BASIC,
     {// Attribute record
      ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, ACCESS_CONTROL_READ,
      (void *)&zclGenericApp_clusterRevision_all}},
    {ZCL_CLUSTER_ID_GEN_IDENTIFY,
     {// Attribute record
      ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, ACCESS_CONTROL_READ,
      (void *)&zclGenericApp_clusterRevision_all}},
};

uint8 CONST zclGenericApp_NumAttributes =
    (sizeof(zclGenericApp_Attrs) / sizeof(zclGenericApp_Attrs[0]));

/*********************************************************************
 * SIMPLE DESCRIPTOR
 */
// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
const cId_t zclGenericApp_InClusterList[] = {
    ZCL_CLUSTER_ID_GEN_BASIC, ZCL_CLUSTER_ID_GEN_IDENTIFY

    // GENERICAPP_TODO: Add application specific Input Clusters Here.
    //       See zcl.h for Cluster ID definitions

};
#define ZCLGENERICAPP_MAX_INCLUSTERS                                           \
  (sizeof(zclGenericApp_InClusterList) / sizeof(zclGenericApp_InClusterList[0]))

const cId_t zclGenericApp_OutClusterList[] = {
    ZCL_CLUSTER_ID_GEN_BASIC, ZCL_CLUSTER_ID_GEN_POWER_CFG
    // GENERICAPP_TODO: Add application specific Output Clusters Here.
    //       See zcl.h for Cluster ID definitions
};
#define ZCLGENERICAPP_MAX_OUTCLUSTERS                                          \
  (sizeof(zclGenericApp_OutClusterList) /                                      \
   sizeof(zclGenericApp_OutClusterList[0]))

const cId_t remote_OutClusterList[] = {ZCL_CLUSTER_ID_GEN_ON_OFF};
#define remote_MAX_OUTCLUSTERS                                                 \
  (sizeof(remote_OutClusterList) / sizeof(remote_OutClusterList[0]))

SimpleDescriptionFormat_t zclGenericApp_SimpleDescs[] = {
    {
        1,                                    //  int Endpoint;
        ZCL_HA_PROFILE_ID,                    //  uint16 AppProfId;
        ZCL_HA_DEVICEID_REMOTE_CONTROL,       //  uint16 AppDeviceId;
        GENERICAPP_DEVICE_VERSION,            //  int   AppDevVer:4;
        GENERICAPP_FLAGS,                     //  int   AppFlags:4;
        0,                                    //  byte  AppNumInClusters;
        (cId_t *)zclGenericApp_InClusterList, //  byte *pAppInClusterList;
        ZCLGENERICAPP_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
        (cId_t *)zclGenericApp_OutClusterList //  byte *pAppInClusterList;
    },
    {
        11,                //  int Endpoint;
        ZCL_HA_PROFILE_ID, //  uint16 AppProfId;

        ZCL_HA_DEVICEID_ON_OFF_SWITCH, //  uint16 AppDeviceId;
        GENERICAPP_DEVICE_VERSION,     //  int   AppDevVer:4;
        GENERICAPP_FLAGS,              //  int   AppFlags:4;
        0,                             //  byte  AppNumInClusters;
        (cId_t *)NULL,                 //  byte *pAppInClusterList;
        remote_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
        (cId_t *)remote_OutClusterList //  byte *pAppInClusterList;
    },
    {
        12,                //  int Endpoint;
        ZCL_HA_PROFILE_ID, //  uint16 AppProfId;

        ZCL_HA_DEVICEID_ON_OFF_SWITCH, //  uint16 AppDeviceId;
        GENERICAPP_DEVICE_VERSION,     //  int   AppDevVer:4;
        GENERICAPP_FLAGS,              //  int   AppFlags:4;
        0,                             //  byte  AppNumInClusters;
        (cId_t *)NULL,                 //  byte *pAppInClusterList;
        remote_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
        (cId_t *)remote_OutClusterList //  byte *pAppInClusterList;
    },
    {
        13,                            //  int Endpoint;
        ZCL_HA_PROFILE_ID,             //  uint16 AppProfId;
        ZCL_HA_DEVICEID_ON_OFF_SWITCH, //  uint16 AppDeviceId;
        GENERICAPP_DEVICE_VERSION,     //  int   AppDevVer:4;
        GENERICAPP_FLAGS,              //  int   AppFlags:4;
        0,                             //  byte  AppNumInClusters;
        (cId_t *)NULL,                 //  byte *pAppInClusterList;
        remote_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
        (cId_t *)remote_OutClusterList //  byte *pAppInClusterList;
    },
    {
        14,                //  int Endpoint;
        ZCL_HA_PROFILE_ID, //  uint16 AppProfId;
        // GENERICAPP_TODO: Replace ZCL_HA_DEVICEID_ON_OFF_LIGHT with
        // application specific device ID
        ZCL_HA_DEVICEID_ON_OFF_SWITCH, //  uint16 AppDeviceId;
        GENERICAPP_DEVICE_VERSION,     //  int   AppDevVer:4;
        GENERICAPP_FLAGS,              //  int   AppFlags:4;
        0,                             //  byte  AppNumInClusters;
        (cId_t *)NULL,                 //  byte *pAppInClusterList;
        remote_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
        (cId_t *)remote_OutClusterList //  byte *pAppInClusterList;
    },
    {
        15,                //  int Endpoint;
        ZCL_HA_PROFILE_ID, //  uint16 AppProfId;
        // GENERICAPP_TODO: Replace ZCL_HA_DEVICEID_ON_OFF_LIGHT with
        // application specific device ID
        ZCL_HA_DEVICEID_ON_OFF_SWITCH, //  uint16 AppDeviceId;
        GENERICAPP_DEVICE_VERSION,     //  int   AppDevVer:4;
        GENERICAPP_FLAGS,              //  int   AppFlags:4;
        0,                             //  byte  AppNumInClusters;
        (cId_t *)NULL,                 //  byte *pAppInClusterList;
        remote_MAX_OUTCLUSTERS,        //  byte  AppNumInClusters;
        (cId_t *)remote_OutClusterList //  byte *pAppInClusterList;
    }};
uint8 zclGenericApp_SimpleDescsCount =
    (sizeof(zclGenericApp_SimpleDescs) / sizeof(zclGenericApp_SimpleDescs[0]));

// Added to include ZLL Target functionality
#if defined(BDB_TL_INITIATOR) || defined(BDB_TL_TARGET)
bdbTLDeviceInfo_t tlGenericApp_DeviceInfo = {
    GENERICAPP_ENDPOINT, // uint8 endpoint;
    ZCL_HA_PROFILE_ID,   // uint16 profileID;
    // GENERICAPP_TODO: Replace ZCL_HA_DEVICEID_ON_OFF_LIGHT with application
    // specific device ID
    ZCL_HA_DEVICEID_ON_OFF_LIGHT, // uint16 deviceID;
    GENERICAPP_DEVICE_VERSION,    // uint8 version;
    GENERICAPP_NUM_GRPS           // uint8 grpIdCnt;
};
#endif

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
