
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
#define HAL_UNKNOWN_BUTTON 0xff

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
static byte zclFreePadApp_KeyCodeToButton(byte key);

static void zclFreePadApp_HandleKeys(byte shift, byte keys);
static void zclFreePadApp_BindNotification(bdbBindNotificationData_t *data);

static void zclFreePadApp_LeaveNetwork(void);
static void zclFreePadApp_ReportBattery(void);
static void zclFreePadApp_Rejoin(void);
static void zclFreePadApp_SendButton(uint8 buttonNumber);
static void zclFreePadApp_SendButtonRelease(uint8 buttonNumber);
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

static void zclFreePadApp_SendButton(byte buttonNumber) {
    if (buttonNumber != HAL_UNKNOWN_BUTTON) {
        printf("Pressed button %d\n", buttonNumber);
        zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[buttonNumber - 1].EndPoint, &inderect_DstAddr, FALSE,
                                       bdb_getZCLFrameCounter());

        const uint8 NUM_ATTRIBUTES = 1;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;
            pReportCmd->attrList[0].attrID = ATTRID_ON_OFF;
            pReportCmd->attrList[0].dataType = ZCL_DATATYPE_BOOLEAN;
            pReportCmd->attrList[0].attrData = (void *)(true);

            zcl_SendReportCmd(zclFreePadApp_SimpleDescs[buttonNumber - 1].EndPoint, &Coordinator_DstAddr,
                              ZCL_CLUSTER_ID_GEN_ON_OFF, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR, false,
                              bdb_getZCLFrameCounter());
        }
        osal_mem_free(pReportCmd);
    }
}

static void zclFreePadApp_SendButtonRelease(uint8 buttonNumber) {
    if (buttonNumber != HAL_UNKNOWN_BUTTON) {
        zclGeneral_SendOnOff_CmdOff(zclFreePadApp_SimpleDescs[buttonNumber - 1].EndPoint, &Coordinator_DstAddr, FALSE,
                                    bdb_getZCLFrameCounter());
    }
}

static byte zclFreePadApp_KeyCodeToButton(byte key) {
    switch (key) {

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
    case 0x21: // row=16 col=4
        return 9;
    case 0x22: // row=16 col=8
        return 10;
    case 0x24: // row=16 col=16
        return 11;
    case 0x28: // row=16 col=32
        return 12;
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

    default:
        return HAL_UNKNOWN_BUTTON;
        break;
    }
}

static void zclFreePadApp_HandleKeys(byte shift, byte keys) {
    static uint32 pressTime = 0;

    static byte prevKey = 0;
    if (keys == prevKey) {
        return;
    }
    // printf("zclFreePadApp_HandleKeys Decimal: %d, Hex: 0x%X\n", keys, keys);

    if (keys == HAL_KEY_CODE_RELEASE_KEY) {
        printf("Released key\n");

        if (pressTime > 0) {
            uint16 timeDiff = (osal_getClock() - pressTime);
            pressTime = 0;

            if (timeDiff > 5) {
                printf("It was very long press\n");
                // zclFreePadApp_Rejoin();
            } else if (timeDiff > 3) {
                printf("It was long press\n");

            } else {
                printf("It was short press\n");

                zclFreePadApp_SendButtonRelease(zclFreePadApp_KeyCodeToButton(prevKey));
            }
        }

    } else {
        HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
        pressTime = osal_getClock();
        zclFreePadApp_SendButton(zclFreePadApp_KeyCodeToButton(keys));
    }
    // zclFreePadApp_ReportBattery();
    prevKey = keys;
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
