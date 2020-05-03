
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
static void zclFreePadApp_SendButtonPress(uint8 buttonNumber);
static void zclFreePadApp_SendButtonLongPress(uint8 buttonNumber);
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
}

static void zclFreePadApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg) {

    switch (bdbCommissioningModeMsg->bdbCommissioningMode) {
    case BDB_COMMISSIONING_NWK_STEERING:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_SUCCESS:
            HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
            break;

        default:
            break;
        }

        break;

    case BDB_COMMISSIONING_PARENT_LOST:
        if (bdbCommissioningModeMsg->bdbCommissioningStatus != BDB_COMMISSIONING_NETWORK_RESTORED) {
            HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
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

    if (events & FREEPADAPP_EVT_GO_TO_SLEEP) {
        LREPMaster("Going to sleep....\n\r");
        halSleep(0);
        return (events ^ FREEPADAPP_EVT_GO_TO_SLEEP);
    }
    // Discard unknown events
    return 0;
}

// Инициализация выхода из сети
static void zclFreePadApp_LeaveNetwork(void) {
    LREPMaster("Leaving network\n\r");

    NLME_LeaveReq_t leaveReq;
    // Set every field to 0
    osal_memset(&leaveReq, 0, sizeof(NLME_LeaveReq_t));

    // This will enable the device to rejoin the network after reset.
    leaveReq.rejoin = FALSE;

    // Set the NV startup option to force a "new" join.
    zgWriteStartupOptions(ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE);

    ZStatus_t leaveStatus = NLME_LeaveReq(&leaveReq);
    LREP("NLME_LeaveReq(&leaveReq) %x\n\r", leaveStatus);
    if (leaveStatus != ZSuccess) {
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

static void zclFreePadApp_SendButtonPress(byte buttonNumber) {
    if (buttonNumber != HAL_UNKNOWN_BUTTON) {
        LREP("Pressed button %d\n\r", buttonNumber);

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

static void zclFreePadApp_SendButtonLongPress(uint8 buttonNumber) {}

static void zclFreePadApp_SendButtonRelease(uint8 buttonNumber) {
    if (buttonNumber != HAL_UNKNOWN_BUTTON) {
        const uint8 NUM_ATTRIBUTES = 1;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;
            pReportCmd->attrList[0].attrID = ATTRID_ON_OFF;
            pReportCmd->attrList[0].dataType = ZCL_DATATYPE_BOOLEAN;
            pReportCmd->attrList[0].attrData = (void *)(false);

            zcl_SendReportCmd(zclFreePadApp_SimpleDescs[buttonNumber - 1].EndPoint, &Coordinator_DstAddr,
                              ZCL_CLUSTER_ID_GEN_ON_OFF, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR, false,
                              bdb_getZCLFrameCounter());
        }
    }
}

static void zclFreePadApp_HandleKeys(byte shift, byte keys) {
    static uint32 pressTime = 0;

    static byte prevKey = 0;
    if (keys == prevKey) {
        return;
    }

    if (keys == HAL_KEY_CODE_RELEASE_KEY) {
        LREPMaster("Released key\n\r");

        if (pressTime > 0) {
            uint16 timeDiff = (osal_getClock() - pressTime);
            pressTime = 0;
            byte button = zclFreePadApp_KeyCodeToButton(prevKey);
            if (timeDiff > 10) {
                LREPMaster("It was very long press\n\r");
                zclFreePadApp_Rejoin();
            } else if (timeDiff > 3) {
                LREPMaster("It was long press\n\r");
                zclFreePadApp_SendButtonLongPress(button);
            } else {
                LREPMaster("It was short press\n\r");
                zclFreePadApp_SendButtonRelease(button);
                if (button != HAL_UNKNOWN_BUTTON) {
                    zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[button - 1].EndPoint, &inderect_DstAddr,
                                                   FALSE, bdb_getZCLFrameCounter());
                }
                if (button == 1) {
                    zclFreePadApp_ReportBattery();
                }
            }
        }
        osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_EVT_GO_TO_SLEEP, (uint32)FREEPADAPP_AWAKE_TIMEOUT);

    } else {
        osal_stop_timerEx(zclFreePadApp_TaskID, FREEPADAPP_EVT_GO_TO_SLEEP);
        bdb_ZedAttemptRecoverNwk();
        HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
        pressTime = osal_getClock();
        zclFreePadApp_SendButtonPress(zclFreePadApp_KeyCodeToButton(keys));
    }
    // zclFreePadApp_ReportBattery();
    prevKey = keys;
}

static void zclFreePadApp_BindNotification(bdbBindNotificationData_t *data) {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
    LREP("Recieved bind request clusterId=0x%X dstAddr=0x%X \n\r", data->clusterId, data->dstAddr);
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
