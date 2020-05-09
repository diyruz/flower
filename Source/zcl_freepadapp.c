
#include "AF.h"
#include "MT_SYS.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "OSAL_PwrMgr.h"
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
#include "hal_drivers.h"
#include "hal_key.h"
#include "hal_led.h"

#include "battery.h"
#include "version.h"
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

bool holdSend = false;
byte currentKey = 0;
byte clicksCount = 0;

afAddrType_t Coordinator_DstAddr = {.addrMode = (afAddrMode_t)Addr16Bit, .addr.shortAddr = 0, .endPoint = 1};

afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 0, .addr.shortAddr = 0};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclFreePadApp_HandleKeys(byte shift, byte keys);
static void zclFreePadApp_BindNotification(bdbBindNotificationData_t *data);
static void zclFreePadApp_ReportBattery(void);
static void zclFreePadApp_Rejoin(void);
static void zclFreePadApp_SendButtonPress(uint8 endPoint, byte clicksCount);
static void zclFreePadApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);
static void zclFreePadApp_SendKeys(byte keyCode, byte pressCount, byte pressTime);
static void zclFreePadApp_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg);

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
    zcl_registerAttrList(1, zclFreePadApp_NumAttributes, zclFreePadApp_Attrs);
    zclGeneral_RegisterCmdCallbacks(1, &zclFreePadApp_CmdCallbacks);
    for (int i = 0; i < zclFreePadApp_SimpleDescsCount; i++) {
        bdb_RegisterSimpleDescriptor(&zclFreePadApp_SimpleDescs[i]);
    }
    zcl_registerForMsg(zclFreePadApp_TaskID);

    // Register for all key events - This app will handle all key events
    RegisterForKeys(zclFreePadApp_TaskID);

    bdb_RegisterBindNotificationCB(zclFreePadApp_BindNotification);
    bdb_RegisterCommissioningStatusCB(zclFreePadApp_ProcessCommissioningStatus);
    bdb_StartCommissioning(BDB_COMMISSIONING_REJOIN_EXISTING_NETWORK_ON_STARTUP);

    DebugInit();
    LREP("Started build %s \r\n", zclFreePadApp_DateCodeNT);
    osal_start_reload_timer(zclFreePadApp_TaskID, FREEPADAPP_REPORT_EVT, FREEPADAPP_REPORT_DELAY);
    // this allows power saving, PM2
    osal_pwrmgr_task_state(zclFreePadApp_TaskID, PWRMGR_CONSERVE);

    LREP("Battery voltage=%d prc=%d \r\n", getBatteryVoltage(), getBatteryRemainingPercentage());
    ZMacSetTransmitPower(TX_PWR_PLUS_4); //set 4dBm
}

static void zclFreePadApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg) {
    LREP("bdbCommissioningMode=%d bdbCommissioningStatus=%d bdbRemainingCommissioningModes=0x%X\r\n",
         bdbCommissioningModeMsg->bdbCommissioningMode, bdbCommissioningModeMsg->bdbCommissioningStatus,
         bdbCommissioningModeMsg->bdbRemainingCommissioningModes);
    switch (bdbCommissioningModeMsg->bdbCommissioningMode) {
    case BDB_COMMISSIONING_INITIALIZATION:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_NO_NETWORK:
            LREP("No network\r\n");
            HalLedBlink(HAL_LED_1, 3, 50, 500);
            break;
        default:
            break;
        }
        break;
    case BDB_COMMISSIONING_NWK_STEERING:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_SUCCESS:
            HalLedBlink(HAL_LED_1, 5, 50, 500);
            LREPMaster("BDB_COMMISSIONING_SUCCESS\r\n");
            break;

        default:
            HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
            break;
        }

        break;

    case BDB_COMMISSIONING_PARENT_LOST:
        LREPMaster("BDB_COMMISSIONING_PARENT_LOST\r\n");
        if (bdbCommissioningModeMsg->bdbCommissioningStatus != BDB_COMMISSIONING_NETWORK_RESTORED) {
            HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
            // // Parent not found, attempt to rejoin again after a fixed delay
            osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_END_DEVICE_REJOIN_EVT,
                               FREEPADAPP_END_DEVICE_REJOIN_DELAY);
        }
        break;
    default:
        break;
    }
}

#define HOLD_CODE 0
#define RELEASE_CODE 255
static void zclFreePadApp_SendKeys(byte keyCode, byte pressCount, bool isRelease) {

    byte button = zclFreePadApp_KeyCodeToButton(keyCode);
    LREP("button %d clicks %d isRelease %d\r\n", button, pressCount, isRelease);
    if (button != HAL_UNKNOWN_BUTTON) {
        uint8 endPoint = zclFreePadApp_SimpleDescs[button - 1].EndPoint;
        switch (pressCount) {
        case 1:
            zclGeneral_SendOnOff_CmdToggle(endPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());

            if (button % 2 == 0) {
                // even numbers (2 4, send up to lower odd number)
                zclGeneral_SendLevelControlStepWithOnOff(endPoint - 1, &inderect_DstAddr, LEVEL_STEP_UP,
                                                         FREEPADAPP_LEVEL_STEP_SIZE, FREEPADAPP_LEVEL_TRANSITION_TIME,
                                                         TRUE, bdb_getZCLFrameCounter());
            } else {
                // odd number (1 3, send LEVEL_MOVE_DOWN to self)
                zclGeneral_SendLevelControlStepWithOnOff(endPoint, &inderect_DstAddr, LEVEL_STEP_DOWN,
                                                         FREEPADAPP_LEVEL_STEP_SIZE, FREEPADAPP_LEVEL_TRANSITION_TIME,
                                                         TRUE, bdb_getZCLFrameCounter());
            }

            break;
        case 2:
            break;

        default:
            break;
        }

        if (isRelease) {
            zclFreePadApp_SendButtonPress(endPoint, RELEASE_CODE);
        } else {
            zclFreePadApp_SendButtonPress(endPoint, pressCount);
        }
    }
}

static void zclFreePadApp_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg) {
    LREP("ZCL_INCOMING_MSG srcAddr=0x%X endPoint=0x%X clusterId=0x%X commandID=0x%X %d\r\n", pInMsg->srcAddr,
         pInMsg->endPoint, pInMsg->clusterId, pInMsg->zclHdr.commandID, pInMsg->zclHdr.commandID);

    if (pInMsg->attrCmd)
        osal_mem_free(pInMsg->attrCmd);
}
uint16 zclFreePadApp_event_loop(uint8 task_id, uint16 events) {
    afIncomingMSGPacket_t *MSGpkt;

    (void)task_id; // Intentionally unreferenced parameter
    devStates_t zclFreePadApp_NwkState;
    if (events & SYS_EVENT_MSG) {
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclFreePadApp_TaskID))) {

            switch (MSGpkt->hdr.event) {

            case KEY_CHANGE:
                zclFreePadApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;
            case ZDO_STATE_CHANGE:
                HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
                zclFreePadApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
                LREP("ZDO_STATE_CHANGE zclFreePadApp_NwkState=%d\r\n", zclFreePadApp_NwkState);
                if (zclFreePadApp_NwkState == DEV_END_DEVICE) {
                    HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
                }
                break;

            case ZCL_INCOMING_MSG:
                zclFreePadApp_ProcessIncomingMsg((zclIncomingMsg_t *)MSGpkt);
                break;

            case AF_DATA_CONFIRM_CMD:
                // This message is received as a confirmation of a data packet sent.
                break;

            default:
                LREP("SysEvent 0x%X status 0x%X macSrcAddr 0x%X endPoint=0x%X\r\n", MSGpkt->hdr.event,
                     MSGpkt->hdr.status, MSGpkt->macSrcAddr, MSGpkt->endPoint);
                break;
            }

            // Release the memory
            osal_msg_deallocate((uint8 *)MSGpkt);
        }

        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    LREP("events 0x%X\r\n", events);
    if (events & FREEPADAPP_END_DEVICE_REJOIN_EVT) {
        LREPMaster("FREEPADAPP_END_DEVICE_REJOIN_EVT\r\n");
        bdb_ZedAttemptRecoverNwk();
        return (events ^ FREEPADAPP_END_DEVICE_REJOIN_EVT);
    }

    if (events & FREEPADAPP_SEND_KEYS_EVT) {
        LREPMaster("FREEPADAPP_SEND_KEYS_EVT\r\n");
        zclFreePadApp_SendKeys(currentKey, clicksCount, holdSend);
        clicksCount = 0;
        currentKey = 0;
        holdSend = false;
        return (events ^ FREEPADAPP_SEND_KEYS_EVT);
    }
    if (events & FREEPADAPP_RESET_EVT) {
        LREPMaster("FREEPADAPP_RESET_EVT\r\n");
        zclFreePadApp_Rejoin();
        return (events ^ FREEPADAPP_RESET_EVT);
    }

    if (events & FREEPADAPP_REPORT_EVT) {
        LREPMaster("FREEPADAPP_REPORT_EVT\r\n");
        zclFreePadApp_ReportBattery();
        return (events ^ FREEPADAPP_REPORT_EVT);
    }
    if (events & FREEPADAPP_HOLD_START_EVT) {
        LREPMaster("FREEPADAPP_HOLD_START_EVT\r\n");
        byte button = zclFreePadApp_KeyCodeToButton(currentKey);
        if (button != HAL_UNKNOWN_BUTTON) {
            uint8 endPoint = zclFreePadApp_SimpleDescs[button - 1].EndPoint;
            zclFreePadApp_SendButtonPress(endPoint, HOLD_CODE);
            holdSend = true;
        }
        return (events ^ FREEPADAPP_HOLD_START_EVT);
    }

    // Discard unknown events
    return 0;
}

static void zclFreePadApp_Rejoin(void) {
    LREP("Recieved rejoin command\r\n");
    if (bdb_isDeviceNonFactoryNew()) {
        bdb_resetLocalAction();
    } else {
        HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING);
    }
}

static void zclFreePadApp_SendButtonPress(uint8 endPoint, uint8 clicksCount) {

    const uint8 NUM_ATTRIBUTES = 1;
    zclReportCmd_t *pReportCmd;
    pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
    if (pReportCmd != NULL) {
        pReportCmd->numAttr = NUM_ATTRIBUTES;
        pReportCmd->attrList[0].attrID = ATTRID_IOV_BASIC_PRESENT_VALUE;
        pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT8;
        pReportCmd->attrList[0].attrData = (void *)(&clicksCount);

        zcl_SendReportCmd(endPoint, &Coordinator_DstAddr, ZCL_CLUSTER_ID_GEN_MULTISTATE_INPUT_BASIC, pReportCmd,
                          ZCL_FRAME_CLIENT_SERVER_DIR, TRUE, bdb_getZCLFrameCounter());
    }
    osal_mem_free(pReportCmd);
}

static void zclFreePadApp_HandleKeys(byte shift, byte keys) {
    static uint32 pressTime = 0;
    static byte prevKey = 0;
    if (keys == prevKey) {
        return;
    }
    prevKey = keys;

    if (keys == HAL_KEY_CODE_RELEASE_KEY) {
        osal_stop_timerEx(zclFreePadApp_TaskID, FREEPADAPP_RESET_EVT);
        if (pressTime > 0) {
            pressTime = 0;
            osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_SEND_KEYS_EVT, FREEPADAPP_SEND_KEYS_DELAY);
            osal_stop_timerEx(zclFreePadApp_TaskID, FREEPADAPP_HOLD_START_EVT);
        }
    } else {

        if (bdb_isDeviceNonFactoryNew()) {
            if (bdbAttributes.bdbCommissioningStatus != BDB_COMMISSIONING_SUCCESS && bdbAttributes.bdbCommissioningStatus != BDB_COMMISSIONING_NETWORK_RESTORED) {
                LREP("!bdbCommissioningStatus=%d try to restore network\r\n", bdbAttributes.bdbCommissioningStatus);
                bdb_ZedAttemptRecoverNwk();
            }
            osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_RESET_EVT, FREEPADAPP_RESET_DELAY);
        } else {
            osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_RESET_EVT,
                               FREEPADAPP_RESET_DELAY >> 2); // 4 times less
        }

        osal_stop_timerEx(zclFreePadApp_TaskID, FREEPADAPP_SEND_KEYS_EVT);
        osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_HOLD_START_EVT, FREEPADAPP_HOLD_START_DELAY);

        HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
        pressTime = osal_getClock();
        clicksCount++;
        currentKey = keys;
    }
}

static void zclFreePadApp_BindNotification(bdbBindNotificationData_t *data) {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
    LREP("Recieved bind request clusterId=0x%X dstAddr=0x%X \r\n", data->clusterId, data->dstAddr);
    uint16 maxEntries = 0, usedEntries = 0;
    bindCapacity(&maxEntries, &usedEntries);
    LREP("bindCapacity %d %usedEntries %d \r\n", maxEntries, usedEntries);
}

static void zclFreePadApp_ReportBattery(void) {
    zclFreePadApp_BatteryVoltage = getBatteryVoltage();
    zclFreePadApp_BatteryPercentageRemainig = getBatteryRemainingPercentage();
    const uint8 NUM_ATTRIBUTES = 2;
    zclReportCmd_t *pReportCmd;
    pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
    if (pReportCmd != NULL) {
        pReportCmd->numAttr = NUM_ATTRIBUTES;

        pReportCmd->attrList[0].attrID = ATTRID_POWER_CFG_BATTERY_VOLTAGE;
        pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT8;
        pReportCmd->attrList[0].attrData = (void *)&zclFreePadApp_BatteryVoltage;

        pReportCmd->attrList[1].attrID = ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING;
        pReportCmd->attrList[1].dataType = ZCL_DATATYPE_UINT8;
        pReportCmd->attrList[1].attrData = (void *)&zclFreePadApp_BatteryPercentageRemainig;

        zcl_SendReportCmd(1, &Coordinator_DstAddr, ZCL_CLUSTER_ID_GEN_POWER_CFG, pReportCmd,
                          ZCL_FRAME_CLIENT_SERVER_DIR, TRUE, bdb_getZCLFrameCounter());
    }

    osal_mem_free(pReportCmd);
}

/****************************************************************************
****************************************************************************/
