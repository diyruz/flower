
#include "AF.h"
#include "MT_SYS.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "ZComDef.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "math.h"

#include "nwk_util.h"

#include "zcl.h"
#include "zcl_diagnostic.h"
#include "zcl_general.h"
#include "zcl_genericapp.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "gp_interface.h"

#include "Debug.h"

#include "onboard.h"

/* HAL */
#include "hal_adc.h"
#include "hal_drivers.h"
#include "hal_key.h"
#include "hal_led.h"

/*********************************************************************
 * MACROS
 */
#define HAL_KEY_CODE_RELEASE_KEY HAL_KEY_CODE_NOKEY
#define HAL_KEY_CODE_1 0x22  // 2x2
#define HAL_KEY_CODE_2 0x32  // 3x2
#define HAL_KEY_CODE_3 0x42  // 4x2
#define HAL_KEY_CODE_4 0x52  // 5x2
#define HAL_KEY_CODE_5 0x62  // 6x2
#define HAL_KEY_CODE_6 0x23  // 2x3
#define HAL_KEY_CODE_7 0x33  // 3x3
#define HAL_KEY_CODE_8 0x43  // 4x3
#define HAL_KEY_CODE_9 0x53  // 5x3
#define HAL_KEY_CODE_10 0x63 // 6x3
#define HAL_KEY_CODE_11 0x24 // 2x4
#define HAL_KEY_CODE_12 0x34 // 3x4
#define HAL_KEY_CODE_13 0x44 // 4x4
#define HAL_KEY_CODE_14 0x54 // 5x4
#define HAL_KEY_CODE_15 0x64 // 6x4
#define HAL_KEY_CODE_16 0x25 // 2x5
#define HAL_KEY_CODE_17 0x35 // 3x5
#define HAL_KEY_CODE_18 0x45 // 4x5
#define HAL_KEY_CODE_19 0x55 // 5x5
#define HAL_KEY_CODE_20 0x65 // 6x5

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zclGenericApp_TaskID;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
bool initilizeRejoin = false;

devStates_t zclGenericApp_NwkState = DEV_INIT;

afAddrType_t Coordinator_DstAddr = {.addrMode = (afAddrMode_t)Addr16Bit, .addr.shortAddr = 0, .endPoint = 1};

afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 0, .addr.shortAddr = 0};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclGenericApp_HandleKeys(byte shift, byte keys);
static void zclGenericApp_BasicResetCB(void);
static void zclGenericApp_BindNotification(bdbBindNotificationData_t *data);
bool halProcessKeyInterrupt(void);

void zclGenericApp_ReportOnOff(uint8 endPoint, bool state);
void zclGenericApp_LeaveNetwork(void);
void zclGenericApp_ReportBattery(void);
void rejoin(void);

// Functions to process ZCL Foundation incoming Command/Response messages
static void zclGenericApp_ProcessIncomingMsg(zclIncomingMsg_t *msg);
#ifdef ZCL_READ
static uint8 zclGenericApp_ProcessInReadRspCmd(zclIncomingMsg_t *pInMsg);
#endif

static uint8 zclGenericApp_ProcessInDefaultRspCmd(zclIncomingMsg_t *pInMsg);
static void zclGenericApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);

/*********************************************************************
 * STATUS STRINGS
 */

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclGenericApp_CmdCallbacks = {
    zclGenericApp_BasicResetCB, // Basic Cluster Reset command
    NULL,                       // Identify Trigger Effect command
    NULL,                       // On/Off cluster commands
    NULL,                       // On/Off cluster enhanced command Off with Effect
    NULL,                       // On/Off cluster enhanced command On with Recall Global Scene
    NULL,                       // On/Off cluster enhanced command On with Timed Off
#ifdef ZCL_LEVEL_CTRL
    NULL, // Level Control Move to Level command
    NULL, // Level Control Move command
    NULL, // Level Control Step command
    NULL, // Level Control Stop command
#endif
#ifdef ZCL_GROUPS
    NULL, // Group Response commands
#endif
#ifdef ZCL_SCENES
    NULL, // Scene Store Request command
    NULL, // Scene Recall Request command
    NULL, // Scene Response command
#endif
#ifdef ZCL_ALARMS
    NULL, // Alarm (Response) commands
#endif
#ifdef SE_UK_EXT
    NULL, // Get Event Log command
    NULL, // Publish Event Log command
#endif
    NULL, // RSSI Location command
    NULL  // RSSI Location Response command
};

void zclGenericApp_Init(byte task_id) {
    zclGenericApp_TaskID = task_id;
    zclGenericApp_ResetAttributesToDefaultValues();
    for (int i = 0; i < zclGenericApp_SimpleDescsCount; i++) {
        bdb_RegisterSimpleDescriptor(&zclGenericApp_SimpleDescs[i]);
        zclGeneral_RegisterCmdCallbacks(zclGenericApp_SimpleDescs[i].EndPoint, &zclGenericApp_CmdCallbacks);
        zcl_registerAttrList(zclGenericApp_SimpleDescs[i].EndPoint, zclGenericApp_NumAttributes, zclGenericApp_Attrs);
        // Register for a test endpoint

#ifdef ZCL_DIAGNOSTIC
        // Register the application's callback function to read/write attribute
        // data. This is only required when the attribute data format is unknown
        // to ZCL.
        zcl_registerReadWriteCB(zclGenericApp_SimpleDescs[i].EndPoint, zclDiagnostic_ReadWriteAttrCB, NULL);

        if (zclDiagnostic_InitStats() == ZSuccess) {
            // Here the user could start the timer to save Diagnostics to NV
        }
#endif
    }
    zcl_registerForMsg(zclGenericApp_TaskID);

    // Register for all key events - This app will handle all key events
    RegisterForKeys(zclGenericApp_TaskID);

    bdb_RegisterBindNotificationCB(zclGenericApp_BindNotification);
    bdb_RegisterCommissioningStatusCB(zclGenericApp_ProcessCommissioningStatus);
    bdb_initialize();

    DebugInit();
    printf("Initialized debug module \n");
    printf("Hello world \n");

    // HalLedBlink(HAL_LED_1, HAL_LED_DEFAULT_FLASH_COUNT, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME);
    // osal_start_reload_timer(zclGenericApp_TaskID, HAL_LED_BLINK_EVENT, 500);
}
const char *bdbCommissioningModes[] = {
    "BDB_COMMISSIONING_INITIALIZATION",  "BDB_COMMISSIONING_NWK_STEERING", "BDB_COMMISSIONING_FORMATION",
    "BDB_COMMISSIONING_FINDING_BINDING", "BDB_COMMISSIONING_TOUCHLINK",    "BDB_COMMISSIONING_PARENT_LOST,",
};

const char *bdbCommissioningStatuses[] = {"BDB_COMMISSIONING_SUCCESS",
                                          "BDB_COMMISSIONING_IN_PROGRESS",
                                          "BDB_COMMISSIONING_NO_NETWORK",
                                          "BDB_COMMISSIONING_TL_TARGET_FAILURE",
                                          "BDB_COMMISSIONING_TL_NOT_AA_CAPABL",
                                          "BDB_COMMISSIONING_TL_NO_SCAN_RESPONS",
                                          "BDB_COMMISSIONING_TL_NOT_PERMITTE",
                                          "BDB_COMMISSIONING_TCLK_EX_FAILURE",
                                          "BDB_COMMISSIONING_FORMATION_FAILUR",
                                          "BDB_COMMISSIONING_FB_TARGET_IN_PROGRESS",
                                          "BDB_COMMISSIONING_FB_INITITATOR_IN_PROGRES",
                                          "BDB_COMMISSIONING_FB_NO_IDENTIFY_QUERY_RESPONS",
                                          "BDB_COMMISSIONING_FB_BINDING_TABLE_FUL",
                                          "BDB_COMMISSIONING_NETWORK_RESTORED",
                                          "BDB_COMMISSIONING_FAILURE"};
static void zclGenericApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg) {
    LREP("%s %s\n", bdbCommissioningModes[bdbCommissioningModeMsg->bdbCommissioningMode],
         bdbCommissioningStatuses[bdbCommissioningModeMsg->bdbCommissioningStatus]);

    switch (bdbCommissioningModeMsg->bdbCommissioningMode) {
    case BDB_COMMISSIONING_NWK_STEERING:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_SUCCESS:
            zclGenericApp_ReportBattery();
            osal_stop_timerEx(zclGenericApp_TaskID, HAL_LED_BLINK_EVENT);
            break;

        default:
            break;
        }

        break;

    case BDB_COMMISSIONING_PARENT_LOST:
        if (bdbCommissioningModeMsg->bdbCommissioningStatus != BDB_COMMISSIONING_NETWORK_RESTORED) {
            // Parent not found, attempt to rejoin again after a fixed delay
            osal_start_timerEx(zclGenericApp_TaskID, GENERICAPP_END_DEVICE_REJOIN_EVT,
                               GENERICAPP_END_DEVICE_REJOIN_DELAY);
        }
        break;
    default:
        break;
    }
}

uint16 zclGenericApp_event_loop(uint8 task_id, uint16 events) {
    afIncomingMSGPacket_t *MSGpkt;

    (void)task_id; // Intentionally unreferenced parameter

    if (events & SYS_EVENT_MSG) {
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclGenericApp_TaskID))) {

            switch (MSGpkt->hdr.event) {
            case ZCL_INCOMING_MSG:
                // Incoming ZCL Foundation command/response messages
                zclGenericApp_ProcessIncomingMsg((zclIncomingMsg_t *)MSGpkt);
                break;

            case KEY_CHANGE:
                zclGenericApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;
            case ZDO_STATE_CHANGE:
                zclGenericApp_NwkState = (devStates_t)(MSGpkt->hdr.status);

                // Теперь мы в сети
                if ((zclGenericApp_NwkState == DEV_ZB_COORD) || (zclGenericApp_NwkState == DEV_ROUTER) ||
                    (zclGenericApp_NwkState == DEV_END_DEVICE)) {
                    osal_stop_timerEx(zclGenericApp_TaskID, HAL_LED_BLINK_EVENT);
                    printf("Connected to network....\n");
                    zclGenericApp_ReportBattery();
                }
                break;

            default:
                break;
            }

            // Release the memory
            osal_msg_deallocate((uint8 *)MSGpkt);
        }

        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & GENERICAPP_END_DEVICE_REJOIN_EVT) {
        bdb_ZedAttemptRecoverNwk();
        return (events ^ GENERICAPP_END_DEVICE_REJOIN_EVT);
    }

    /* GENERICAPP_TODO: handle app events here */

    if (events & HAL_LED_BLINK_EVENT) {
        // toggle LED 2 state, start another timer for 500ms
        HalLedSet(HAL_LED_2, HAL_LED_MODE_TOGGLE);
        // osal_start_timerEx(zclGenericApp_TaskID, HAL_LED_BLINK_EVENT, 500);
        return (events ^ HAL_LED_BLINK_EVENT);
    }

    if (events & GENERICAPP_EVT_GO_TO_SLEEP) {
        printf("Going to sleep....\n");
        halSleep(10000);
        return (events ^ GENERICAPP_EVT_GO_TO_SLEEP);
    }

    if (events & GENERICAPP_SW1_LONG_PRESS) {
        printf("GENERICAPP_SW1_LONG_PRESS detected \n");

        initilizeRejoin = true;

        return events ^ GENERICAPP_SW1_LONG_PRESS;
    }

    if (events & GENERICAPP_END_DEVICE_REJOIN_EVT) {
        bdb_ZedAttemptRecoverNwk();
        return (events ^ GENERICAPP_END_DEVICE_REJOIN_EVT);
    }

    // Discard unknown events
    return 0;
}

// Инициализация выхода из сети
void zclGenericApp_LeaveNetwork(void) {
    printf("Leaving network\n");
    zclGenericApp_ResetAttributesToDefaultValues();

    NLME_LeaveReq_t leaveReq;
    // Set every field to 0
    osal_memset(&leaveReq, 0, sizeof(NLME_LeaveReq_t));

    // This will enable the device to rejoin the network after reset.
    leaveReq.rejoin = FALSE;

    // Set the NV startup option to force a "new" join.
    zgWriteStartupOptions(ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE);

    // Leave the network, and reset afterwards

    ZStatus_t leaveStatus = NLME_LeaveReq(&leaveReq);
    LREP("NLME_LeaveReq(&leaveReq) %x\n", leaveStatus);
    if (leaveStatus != ZSuccess) {
        printf("Couldn't send out leave; prepare to reset anyway\n");
        ZDApp_LeaveReset(FALSE);
    }
}
void rejoin(void) {
    initilizeRejoin = false;
    if (bdbAttributes.bdbNodeIsOnANetwork) {
        zclGenericApp_LeaveNetwork();
    } else {
        printf("bdb_StartCommissioning.(BDB_COMMISSIONING_MODE_NWK_STEERING)...\n");
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING);
        osal_start_reload_timer(zclGenericApp_TaskID, HAL_LED_BLINK_EVENT, 100);
    }
}
static void zclGenericApp_HandleKeys(byte shift, byte keys) {
    static uint32 pressTime = 0;

    static byte prevKey = 0;
    if (keys == prevKey) {
        return;
    } else {
        prevKey = keys;
    }
    HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
    printf("printf zclGenericApp_HandleKeys Decimal: %d, Hex: 0x%X\n", keys, keys);

    if (keys == HAL_KEY_CODE_RELEASE_KEY) {
        printf("Released key");

        if (pressTime > 0) {
            uint16 timeDiff = (osal_getClock() - pressTime);
            pressTime = 0;

            if (timeDiff < 3) {
                printf("It was short press\n");
                // short press : scene recall

            } else {
                printf("It was long press\n");
                return;
            }
        }

    } else {
        pressTime = osal_getClock();

        if (keys == HAL_KEY_CODE_1) {
            printf("Pressed button1\n");
            // zclGeneral_SendOnOff_CmdOn(SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr, FALSE, sampleRemoteSeqNum++);
        }

        if (keys == HAL_KEY_CODE_2) {
            printf("Pressed button2\n");
            // zclGeneral_SendOnOff_CmdOn(SAMPLEREMOTE_ENDPOINT, &zllSampleRemote_DstAddr, FALSE, sampleRemoteSeqNum++);
        }
    }

    // LREP("zclGenericApp_HandleKeys" BYTE_TO_BINARY_PATTERN "", BYTE_TO_BINARY(keys));
    // printf("\n");

    // if (!keys) {
    //     printf("Button released\n");
    // }

    //     osal_stop_timerEx(zclGenericApp_TaskID, GENERICAPP_SW1_LONG_PRESS);
    //     osal_clear_event(zclGenericApp_TaskID, GENERICAPP_SW1_LONG_PRESS);

    //     osal_stop_timerEx(zclGenericApp_TaskID, HAL_KEY_EVENT);
    //     osal_clear_event(zclGenericApp_TaskID, HAL_KEY_EVENT);

    //     halKeySavedKeys = 0;

    //     if (initilizeRejoin) {
    //         rejoin();
    //     }
    //     // osal_start_timerEx(zclGenericApp_TaskID, GENERICAPP_EVT_GO_TO_SLEEP,
    //     // 10000);
    // }

    // if (keys & HAL_KEY_CODE_1) {
    //     printf("Pressed button 1\n");
    //     zclGeneral_SendOnOff_CmdToggle(zclGenericApp_SimpleDescs[0].EndPoint, &inderect_DstAddr, FALSE,
    //                                    bdb_getZCLFrameCounter());
    //     zclGenericApp_ReportBattery();
    // }
    // if (keys & HAL_KEY_SW_2) {
    //     printf("Pressed button2\n");
    //     zclGeneral_SendOnOff_CmdToggle(zclGenericApp_SimpleDescs[1].EndPoint, &inderect_DstAddr, FALSE,
    //     bdb_getZCLFrameCounter());
    // }
}

static void zclGenericApp_BindNotification(bdbBindNotificationData_t *data) {
    LREP("zclGenericApp_BindNotification %x %x %x", data->dstAddr, data->ep, data->clusterId);
    // GENERICAPP_TODO: process the new bind information
}

static void zclGenericApp_BasicResetCB(void) { zclGenericApp_ResetAttributesToDefaultValues(); }

static void zclGenericApp_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg) {
    LREP("pInMsg->zclHdr.commandID 0x%x \n", pInMsg->zclHdr.commandID);
    switch (pInMsg->zclHdr.commandID) {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
        zclGenericApp_ProcessInReadRspCmd(pInMsg);
        break;
#endif
    case ZCL_CMD_CONFIG_REPORT:
    case ZCL_CMD_CONFIG_REPORT_RSP:
    case ZCL_CMD_READ_REPORT_CFG:
    case ZCL_CMD_READ_REPORT_CFG_RSP:
    case ZCL_CMD_REPORT:
        // bdb_ProcessIncomingReportingMsg( pInMsg );
        break;

    case ZCL_CMD_DEFAULT_RSP:
        zclGenericApp_ProcessInDefaultRspCmd(pInMsg);
        break;
    default:
        break;
    }

    if (pInMsg->attrCmd)
        osal_mem_free(pInMsg->attrCmd);
}

#ifdef ZCL_READ
static uint8 zclGenericApp_ProcessInReadRspCmd(zclIncomingMsg_t *pInMsg) {
    zclReadRspCmd_t *readRspCmd;
    uint8 i;
    printf("zclGenericApp_ProcessInReadRspCmd\n");

    readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
    for (i = 0; i < readRspCmd->numAttr; i++) {
        // Notify the originator of the results of the original read attributes
        // attempt and, for each successfull request, the value of the requested
        // attribute
    }

    return (TRUE);
}
#endif // ZCL_READ

/*********************************************************************
 * @fn      zclGenericApp_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclGenericApp_ProcessInDefaultRspCmd(zclIncomingMsg_t *pInMsg) {
    // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t
    // *)pInMsg->attrCmd;

    // Device is notified of the Default Response command.
    (void)pInMsg;

    return (TRUE);
}

/* (( 3 * 1,15 ) / (( 2^12 / 2 ) - 1 )) * 10  */
#define MULTI 0.0168539326

static uint8 readVoltage(void) {
    uint16 value;

    HalAdcSetReference(HAL_ADC_REF_125V);
    value = HalAdcRead(HAL_ADC_CHANNEL_VDD, HAL_ADC_RESOLUTION_12);
    HalAdcSetReference(HAL_ADC_REF_AVDD);
    return (uint8)(value * MULTI);
}

void zclGenericApp_ReportBattery(void) {
    zclGenericApp_BatteryVoltage = readVoltage();
    zclGenericApp_BatteryPercentageRemainig = (uint8)MIN(100, ceil(100.0 / 33.0 * zclGenericApp_BatteryVoltage));

    LREP("zclGenericApp_ReportBattery %d pct: %d\n", zclGenericApp_BatteryVoltage,
         zclGenericApp_BatteryPercentageRemainig);
    const uint8 NUM_ATTRIBUTES = 2;
    zclReportCmd_t *pReportCmd;
    pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
    if (pReportCmd != NULL) {
        pReportCmd->numAttr = NUM_ATTRIBUTES;

        pReportCmd->attrList[0].attrID = ATTRID_POWER_CFG_BATTERY_VOLTAGE;
        pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT8;
        pReportCmd->attrList[0].attrData = (void *)(&zclGenericApp_BatteryVoltage);

        pReportCmd->attrList[1].attrID = ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING;
        pReportCmd->attrList[1].dataType = ZCL_DATATYPE_UINT8;
        pReportCmd->attrList[1].attrData = (void *)(&zclGenericApp_BatteryPercentageRemainig);

        zcl_SendReportCmd(1, &inderect_DstAddr, ZCL_CLUSTER_ID_GEN_POWER_CFG, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR,
                          false, bdb_getZCLFrameCounter());
        zcl_SendReportCmd(1, &Coordinator_DstAddr, ZCL_CLUSTER_ID_GEN_POWER_CFG, pReportCmd,
                          ZCL_FRAME_CLIENT_SERVER_DIR, false, bdb_getZCLFrameCounter());
    }

    osal_mem_free(pReportCmd);
}

/****************************************************************************
****************************************************************************/
