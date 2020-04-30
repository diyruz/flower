
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
#include "zcl_FREEPADAPP.h"
#include "zcl_diagnostic.h"
#include "zcl_general.h"

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
byte zclFreePadApp_TaskID;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

devStates_t zclFreePadApp_NwkState = DEV_INIT;

afAddrType_t Coordinator_DstAddr = {.addrMode = (afAddrMode_t)Addr16Bit, .addr.shortAddr = 0, .endPoint = 1};

afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 0, .addr.shortAddr = 0};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclFreePadApp_HandleKeys(byte shift, byte keys);
static void zclFreePadApp_BindNotification(bdbBindNotificationData_t *data);
bool halProcessKeyInterrupt(void);

void zclFreePadApp_ReportOnOff(uint8 endPoint, bool state);
void zclFreePadApp_LeaveNetwork(void);
void zclFreePadApp_ReportBattery(void);
void rejoin(void);

// Functions to process ZCL Foundation incoming Command/Response messages
static void zclFreePadApp_ProcessIncomingMsg(zclIncomingMsg_t *msg);
#ifdef ZCL_READ
static uint8 zclFreePadApp_ProcessInReadRspCmd(zclIncomingMsg_t *pInMsg);
#endif

static uint8 zclFreePadApp_ProcessInDefaultRspCmd(zclIncomingMsg_t *pInMsg);
static void zclFreePadApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);

/*********************************************************************
 * STATUS STRINGS
 */

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclFreePadApp_CmdCallbacks = {
    NULL, // Basic Cluster Reset command
    NULL, // Identify Trigger Effect command
    NULL, // On/Off cluster commands
    NULL, // On/Off cluster enhanced command Off with Effect
    NULL, // On/Off cluster enhanced command On with Recall Global Scene
    NULL, // On/Off cluster enhanced command On with Timed Off
    NULL, // RSSI Location command
    NULL  // RSSI Location Response command
};

void zclFreePadApp_Init(byte task_id) {
    zclFreePadApp_TaskID = task_id;
    zclFreePadApp_InitClusters();

    for (int i = 0; i < zclFreePadApp_SimpleDescsCount; i++) {
        bdb_RegisterSimpleDescriptor(&zclFreePadApp_SimpleDescs[i]);
        zclGeneral_RegisterCmdCallbacks(zclFreePadApp_SimpleDescs[i].EndPoint, &zclFreePadApp_CmdCallbacks);
        zcl_registerAttrList(zclFreePadApp_SimpleDescs[i].EndPoint, zclFreePadApp_NumAttributes, zclFreePadApp_Attrs);
    }
    zcl_registerForMsg(zclFreePadApp_TaskID);

    // Register for all key events - This app will handle all key events
    RegisterForKeys(zclFreePadApp_TaskID);

    bdb_RegisterBindNotificationCB(zclFreePadApp_BindNotification);
    bdb_RegisterCommissioningStatusCB(zclFreePadApp_ProcessCommissioningStatus);
    bdb_initialize();

    DebugInit();
    printf("Initialized debug module \n");
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
static void zclFreePadApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg) {
    LREP("%s %s\n", bdbCommissioningModes[bdbCommissioningModeMsg->bdbCommissioningMode],
         bdbCommissioningStatuses[bdbCommissioningModeMsg->bdbCommissioningStatus]);

    switch (bdbCommissioningModeMsg->bdbCommissioningMode) {
    case BDB_COMMISSIONING_NWK_STEERING:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_SUCCESS:
            zclFreePadApp_ReportBattery();
            HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
            break;

        default:
            break;
        }

        break;

    case BDB_COMMISSIONING_PARENT_LOST:
        if (bdbCommissioningModeMsg->bdbCommissioningStatus != BDB_COMMISSIONING_NETWORK_RESTORED) {
            // Parent not found, attempt to rejoin again after a fixed delay
            osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_END_DEVICE_REJOIN_EVT,
                               FREEPADAPP_END_DEVICE_REJOIN_DELAY);
        }
        break;
    default:
        break;
    }
}

uint16 zclFreePadApp_event_loop(uint8 task_id, uint16 events) {
    afIncomingMSGPacket_t *MSGpkt;

    (void)task_id; // Intentionally unreferenced parameter

    if (events & SYS_EVENT_MSG) {
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclFreePadApp_TaskID))) {

            switch (MSGpkt->hdr.event) {
            case ZCL_INCOMING_MSG:
                // Incoming ZCL Foundation command/response messages
                zclFreePadApp_ProcessIncomingMsg((zclIncomingMsg_t *)MSGpkt);
                break;

            case KEY_CHANGE:
                zclFreePadApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;
            case ZDO_STATE_CHANGE:
                zclFreePadApp_NwkState = (devStates_t)(MSGpkt->hdr.status);

                // Теперь мы в сети
                if ((zclFreePadApp_NwkState == DEV_ZB_COORD) || (zclFreePadApp_NwkState == DEV_ROUTER) ||
                    (zclFreePadApp_NwkState == DEV_END_DEVICE)) {

                    HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
                    // printf("Connected to network....\n");
                    zclFreePadApp_ReportBattery();
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

    if (events & FREEPADAPP_END_DEVICE_REJOIN_EVT) {
        bdb_ZedAttemptRecoverNwk();
        return (events ^ FREEPADAPP_END_DEVICE_REJOIN_EVT);
    }

    /* FREEPADAPP_TODO: handle app events here */

    if (events & FREEPADAPP_EVT_GO_TO_SLEEP) {
        // printf("Going to sleep....\n");
        halSleep(10000);
        return (events ^ FREEPADAPP_EVT_GO_TO_SLEEP);
    }

    if (events & FREEPADAPP_END_DEVICE_REJOIN_EVT) {
        bdb_ZedAttemptRecoverNwk();
        return (events ^ FREEPADAPP_END_DEVICE_REJOIN_EVT);
    }

    // Discard unknown events
    return 0;
}

// Инициализация выхода из сети
void zclFreePadApp_LeaveNetwork(void) {
    // printf("Leaving network\n");

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
        // printf("Couldn't send out leave; prepare to reset anyway\n");
        ZDApp_LeaveReset(FALSE);
    }
    osal_mem_free(&leaveReq);
}
void rejoin(void) {
    HalLedBlink(HAL_LED_1, (uint8)10, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME);
    if (bdbAttributes.bdbNodeIsOnANetwork) {
        zclFreePadApp_LeaveNetwork();
    } else {
        // printf("bdb_StartCommissioning.(BDB_COMMISSIONING_MODE_NWK_STEERING)...\n");
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING);
    }
}
static void zclFreePadApp_HandleKeys(byte shift, byte keys) {
    static uint32 pressTime = 0;

    static byte prevKey = 0;
    if (keys == prevKey) {
        return;
    } else {
        prevKey = keys;
    }
    // printf("zclFreePadApp_HandleKeys Decimal: %d, Hex: 0x%X\n", keys, keys);

    if (keys == HAL_KEY_CODE_RELEASE_KEY) {
        printf("Released key");

        if (pressTime > 0) {
            uint16 timeDiff = (osal_getClock() - pressTime);
            pressTime = 0;

            if (timeDiff > 15) {
                printf("It was very long press\n");
                rejoin();
            } else if (timeDiff > 3) {
                printf("It was long press\n");
                rejoin();
            } else {
                printf("It was short press\n");
                return;
            }
        }

    } else {
        zclFreePadApp_ReportBattery();
        HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
        pressTime = osal_getClock();

        switch (keys) {
#if defined(HAL_BOARD_FREEPAD)
        case HAL_KEY_CODE_1:
            printf("Pressed button1\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[0].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            // zclGeneral_SendLevelControlMoveWithOnOff( zclFreePadApp_SimpleDescs[0].EndPoint, &inderect_DstAddr,
            // LEVEL_MOVE_UP, 20, TRUE, bdb_getZCLFrameCounter()) );

            break;

        case HAL_KEY_CODE_2:
            printf("Pressed button2\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[1].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            // zclGeneral_SendLevelControlMoveWithOnOff( zclFreePadApp_SimpleDescs[0].EndPoint, &inderect_DstAddr,
            // LEVEL_MOVE_DOWN, 20, TRUE, bdb_getZCLFrameCounter()) );
            break;

        case HAL_KEY_CODE_3:
            printf("Pressed button3\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[2].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_4:
            printf("Pressed button4\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[3].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_5:
            printf("Pressed button5\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[4].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_6:
            printf("Pressed button6\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[5].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_7:
            printf("Pressed button7\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[6].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_8:
            printf("Pressed button8\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[7].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;
#endif
#if defined(HAL_BOARD_FREEPAD_12) || defined(HAL_BOARD_FREEPAD_20)
        case HAL_KEY_CODE_9:
            printf("Pressed button9\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[8].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_10:
            printf("Pressed button10\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[9].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_11:
            printf("Pressed button11\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[10].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_12:
            printf("Pressed button12\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[11].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;
#endif
#ifdef HAL_BOARD_FREEPAD_20

        case HAL_KEY_CODE_13:
            printf("Pressed button13\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[12].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_14:
            printf("Pressed button14\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[13].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_15:
            printf("Pressed button15\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[14].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_16:
            printf("Pressed button16\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[15].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_17:
            printf("Pressed button17\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[16].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_18:
            printf("Pressed button18\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[17].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_19:
            printf("Pressed button19\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[18].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;

        case HAL_KEY_CODE_20:
            printf("Pressed button20\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[19].EndPoint, &inderect_DstAddr, FALSE,
                                           bdb_getZCLFrameCounter());
            break;
#endif
        default:
            break;
        }
    }
}

static void zclFreePadApp_BindNotification(bdbBindNotificationData_t *data) {
    // LREP("zclFreePadApp_BindNotification %x %x %x", data->dstAddr, data->ep, data->clusterId);
    HalLedBlink(HAL_LED_1, 4, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME);
}

static void zclFreePadApp_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg) {
    LREP("pInMsg->zclHdr.commandID 0x%x \n", pInMsg->zclHdr.commandID);
    switch (pInMsg->zclHdr.commandID) {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
        zclFreePadApp_ProcessInReadRspCmd(pInMsg);
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
        zclFreePadApp_ProcessInDefaultRspCmd(pInMsg);
        break;
    default:
        break;
    }

    if (pInMsg->attrCmd)
        osal_mem_free(pInMsg->attrCmd);
}

#ifdef ZCL_READ
static uint8 zclFreePadApp_ProcessInReadRspCmd(zclIncomingMsg_t *pInMsg) {
    zclReadRspCmd_t *readRspCmd;
    uint8 i;
    // printf("zclFreePadApp_ProcessInReadRspCmd\n");

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
 * @fn      zclFreePadApp_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclFreePadApp_ProcessInDefaultRspCmd(zclIncomingMsg_t *pInMsg) {
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

void zclFreePadApp_ReportBattery(void) {
    zclFreePadApp_BatteryVoltage = readVoltage();
    zclFreePadApp_BatteryPercentageRemainig = (uint8)MIN(100, ceil(100.0 / 33.0 * zclFreePadApp_BatteryVoltage));

    LREP("zclFreePadApp_ReportBattery %d pct: %d\n", zclFreePadApp_BatteryVoltage,
         zclFreePadApp_BatteryPercentageRemainig);
    const uint8 NUM_ATTRIBUTES = 2;
    zclReportCmd_t *pReportCmd;
    pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
    if (pReportCmd != NULL) {
        pReportCmd->numAttr = NUM_ATTRIBUTES;

        pReportCmd->attrList[0].attrID = ATTRID_POWER_CFG_BATTERY_VOLTAGE;
        pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT8;
        pReportCmd->attrList[0].attrData = (void *)(&zclFreePadApp_BatteryVoltage);

        pReportCmd->attrList[1].attrID = ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING;
        pReportCmd->attrList[1].dataType = ZCL_DATATYPE_UINT8;
        pReportCmd->attrList[1].attrData = (void *)(&zclFreePadApp_BatteryPercentageRemainig);

        zcl_SendReportCmd(1, &inderect_DstAddr, ZCL_CLUSTER_ID_GEN_POWER_CFG, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR,
                          false, bdb_getZCLFrameCounter());
        zcl_SendReportCmd(1, &Coordinator_DstAddr, ZCL_CLUSTER_ID_GEN_POWER_CFG, pReportCmd,
                          ZCL_FRAME_CLIENT_SERVER_DIR, false, bdb_getZCLFrameCounter());
    }

    osal_mem_free(pReportCmd);
}

/****************************************************************************
****************************************************************************/
