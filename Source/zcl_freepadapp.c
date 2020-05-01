
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
#include "zcl_freepadapp.h"
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

static void zclFreePadApp_LeaveNetwork(void);
static void zclFreePadApp_ReportBattery(void);
static void zclFreePadApp_Rejoin(void);
static void zclFreePadApp_SendButton(int buttonNumber);
static void zclFreePadApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);

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

static void zclFreePadApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg) {

    switch (bdbCommissioningModeMsg->bdbCommissioningMode) {
    case BDB_COMMISSIONING_NWK_STEERING:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_SUCCESS:
            // zclFreePadApp_ReportBattery();
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

            case KEY_CHANGE:
                zclFreePadApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;
            case ZDO_STATE_CHANGE:
                zclFreePadApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
                // Теперь мы в сети
                if (zclFreePadApp_NwkState == DEV_END_DEVICE) {
                    HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
                    // printf("Connected to network....\n");
                    // zclFreePadApp_ReportBattery();
                    // for (int i = 0; i < zclFreePadApp_SimpleDescsCount; i++) {
                    //     zclGeneral_SendIdentify(zclFreePadApp_SimpleDescs[i].EndPoint, &Coordinator_DstAddr,
                    //                             zclFreePadApp_IdentifyTime, true, bdb_getZCLFrameCounter());
                    // }
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
static void zclFreePadApp_LeaveNetwork(void) {
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
static void zclFreePadApp_Rejoin(void) {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
    if (bdbAttributes.bdbNodeIsOnANetwork) {
        zclFreePadApp_LeaveNetwork();
    } else {
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING);
    }
}

static void zclFreePadApp_SendButton(int buttonNumber) {
    printf("Pressed button %d\n", buttonNumber);
    zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[buttonNumber - 1].EndPoint, &inderect_DstAddr, FALSE,
                                   bdb_getZCLFrameCounter());
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

            if (timeDiff > 5) {
                printf("It was very long press\n");
                zclFreePadApp_Rejoin();
            } else if (timeDiff > 3) {
                printf("It was long press\n");

            } else {
                printf("It was short press\n");
                return;
            }
        }

    } else {
        HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
        pressTime = osal_getClock();

        switch (keys) {
#if defined(HAL_BOARD_FREEPAD_8) || defined(HAL_BOARD_FREEPAD_12) || defined(HAL_BOARD_FREEPAD_20)

        case HAL_KEY_CODE_1:
            zclFreePadApp_SendButton(1);
            break;

        case HAL_KEY_CODE_2:
            zclFreePadApp_SendButton(2);
            break;

        case HAL_KEY_CODE_3:
            zclFreePadApp_SendButton(3);
            break;

        case HAL_KEY_CODE_4:
            zclFreePadApp_SendButton(4);
            break;

        case HAL_KEY_CODE_5:
            zclFreePadApp_SendButton(5);
            break;

        case HAL_KEY_CODE_6:
            zclFreePadApp_SendButton(6);
            break;

        case HAL_KEY_CODE_7:
            zclFreePadApp_SendButton(7);
            break;

        case HAL_KEY_CODE_8:
            zclFreePadApp_SendButton(8);
            break;

#endif
#if defined(HAL_BOARD_FREEPAD_12) || defined(HAL_BOARD_FREEPAD_20)

        case HAL_KEY_CODE_9:
            zclFreePadApp_SendButton(9);
            break;

        case HAL_KEY_CODE_10:
            zclFreePadApp_SendButton(10);
            break;

        case HAL_KEY_CODE_11:
            zclFreePadApp_SendButton(11);
            break;

        case HAL_KEY_CODE_12:
            zclFreePadApp_SendButton(12);
            break;

#endif
#ifdef HAL_BOARD_FREEPAD_20

        case HAL_KEY_CODE_13:
            zclFreePadApp_SendButton(13);
            break;

        case HAL_KEY_CODE_14:
            zclFreePadApp_SendButton(14);
            break;

        case HAL_KEY_CODE_15:
            zclFreePadApp_SendButton(15);
            break;

        case HAL_KEY_CODE_16:
            zclFreePadApp_SendButton(16);
            break;

        case HAL_KEY_CODE_17:
            zclFreePadApp_SendButton(17);
            break;

        case HAL_KEY_CODE_18:
            zclFreePadApp_SendButton(18);
            break;

        case HAL_KEY_CODE_19:
            zclFreePadApp_SendButton(19);
            break;

        case HAL_KEY_CODE_20:
            zclFreePadApp_SendButton(20);
            break;

#endif

        default:
            break;
        }
    }
    zclFreePadApp_ReportBattery();
}

static void zclFreePadApp_BindNotification(bdbBindNotificationData_t *data) {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
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

        zcl_SendReportCmd(1, &Coordinator_DstAddr, ZCL_CLUSTER_ID_GEN_POWER_CFG, pReportCmd,
                          ZCL_FRAME_CLIENT_SERVER_DIR, false, bdb_getZCLFrameCounter());
    }

    osal_mem_free(pReportCmd);
}

/****************************************************************************
****************************************************************************/
