
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
#include "zcl_lighting.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "bdb_touchlink.h"
#include "bdb_touchlink_initiator.h"
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
byte currentKeyCode = 0;
byte clicksCount = 0;

byte rejoinsLeft = FREEPADAPP_END_DEVICE_REJOIN_TRIES;
uint32 rejoinDelay = FREEPADAPP_END_DEVICE_REJOIN_START_DELAY;

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
static void zclFreePadApp_ResetBackoffRetry(void);
static void zclFreePadApp_OnConnect(void);
static void zclFreePadApp_BasicResetCB(void);
static ZStatus_t zclFreePadApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper);
static void zclFreePadApp_SaveAttributesToNV(void);
static void zclFreePadApp_RestoreAttributesFromNV(void);
static void zclFreePadApp_StartTL(void);
ZStatus_t zclFreePadApp_TL_NotifyCb(epInfoRec_t *pData);

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclFreePadApp_CmdCallbacks = {
    zclFreePadApp_BasicResetCB, // Basic Cluster Reset command
    NULL,                       // Identify Trigger Effect command
    NULL,                       // On/Off cluster commands
    NULL,                       // On/Off cluster enhanced command Off with Effect
    NULL,                       // On/Off cluster enhanced command On with Recall Global Scene
    NULL,                       // On/Off cluster enhanced command On with Timed Off
    NULL,                       // RSSI Location command
    NULL                        // RSSI Location Response command
};
// static CONST zclAttrRec_t attrs[FREEPAD_BUTTONS_COUNT];

static void zclFreePadApp_BasicResetCB(void) {
    LREP("zclFreePadApp_BasicResetCB\r\n");
    zclFreePadApp_ResetAttributesToDefaultValues();
    zclFreePadApp_SaveAttributesToNV();
}

/*
this is workaround, since we don't have CB after attribute write, we will hack into write auth CB
and save attributes few secons later
*/
static ZStatus_t zclFreePadApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper) {
    LREP("AUTH CB called %x %x %x\r\n", srcAddr->addr, pAttr->attr, pAttr->clusterID);

    osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_SAVE_ATTRS_EVT, 2000);
    return ZSuccess;
}
void zclFreePadApp_Init(byte task_id) {
    DebugInit();
    zclFreePadApp_TaskID = task_id;
    zclFreePadApp_ResetAttributesToDefaultValues();
    zclFreePadApp_RestoreAttributesFromNV();
    zclFreePadApp_InitClusters();

    zclGeneral_RegisterCmdCallbacks(1, &zclFreePadApp_CmdCallbacks);
    for (int i = 0; i < zclFreePadApp_SimpleDescsCount; i++) {
        SimpleDescriptionFormat_t descriptor = zclFreePadApp_SimpleDescs[i];
        if (i == 0) {
            zcl_registerAttrList(descriptor.EndPoint, zclFreePadApp_AttrsFirstEPCount, zclFreePadApp_AttrsFirstEP);
        } else {
            // take one lower [i-1], since first ep  handled by zclFreePadApp_AttrsFirstEP
            zcl_registerAttrList(descriptor.EndPoint, FREEPAD_ATTRS_COUNT, zclFreePadApp_Attrs[i - 1]);
        }

        bdb_RegisterSimpleDescriptor(&zclFreePadApp_SimpleDescs[i]);

        zcl_registerReadWriteCB(descriptor.EndPoint, NULL, zclFreePadApp_ReadWriteAuthCB);
    }
    zcl_registerForMsg(zclFreePadApp_TaskID);

    // Register for all key events - This app will handle all key events
    RegisterForKeys(zclFreePadApp_TaskID);

    bdb_RegisterBindNotificationCB(zclFreePadApp_BindNotification);
    bdb_RegisterCommissioningStatusCB(zclFreePadApp_ProcessCommissioningStatus);
    touchLinkInitiator_RegisterNotifyTLCB(zclFreePadApp_TL_NotifyCb);

    bdb_StartCommissioning(BDB_COMMISSIONING_REJOIN_EXISTING_NETWORK_ON_STARTUP);

    LREP("Started build %s \r\n", zclFreePadApp_DateCodeNT);
    osal_start_reload_timer(zclFreePadApp_TaskID, FREEPADAPP_REPORT_EVT, FREEPADAPP_REPORT_DELAY);
    // this allows power saving, PM2
    osal_pwrmgr_task_state(zclFreePadApp_TaskID, PWRMGR_CONSERVE);

    LREP("Battery voltage=%d prc=%d \r\n", getBatteryVoltage(), getBatteryRemainingPercentageZCL());
    ZMacSetTransmitPower(TX_PWR_PLUS_4); // set 4dBm
}

static void zclFreePadApp_ResetBackoffRetry(void) {
    rejoinsLeft = FREEPADAPP_END_DEVICE_REJOIN_TRIES;
    rejoinDelay = FREEPADAPP_END_DEVICE_REJOIN_START_DELAY;
}

static void zclFreePadApp_OnConnect(void) {
    zclFreePadApp_ResetBackoffRetry();
    osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_REPORT_EVT, FREEPADAPP_CONST_ONE_MINUTE_IN_MS); // 1 minute
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
        case BDB_COMMISSIONING_NETWORK_RESTORED:
            zclFreePadApp_OnConnect();
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
            zclFreePadApp_OnConnect();
            break;

        default:
            HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
            break;
        }

        break;

    case BDB_COMMISSIONING_PARENT_LOST:
        LREPMaster("BDB_COMMISSIONING_PARENT_LOST\r\n");
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_NETWORK_RESTORED:
            zclFreePadApp_ResetBackoffRetry();
            break;

        default:
            HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
            // // Parent not found, attempt to rejoin again after a exponential backoff delay
            LREP("rejoinsLeft %d rejoinDelay=%ld\r\n", rejoinsLeft, rejoinDelay);
            if (rejoinsLeft > 0) {
                rejoinDelay *= FREEPADAPP_END_DEVICE_REJOIN_BACKOFF;
                rejoinsLeft -= 1;
            } else {
                rejoinDelay = FREEPADAPP_END_DEVICE_REJOIN_MAX_DELAY;
            }
            osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_END_DEVICE_REJOIN_EVT, rejoinDelay);
            break;
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
    uint8 endPoint = zclFreePadApp_SimpleDescs[button - 1].EndPoint;
    uint8 switchAction = zclFreePadApp_SwitchActions[button - 1];
    LREP("button %d clicks %d isRelease %d action %d\r\n", button, pressCount, isRelease, switchAction);

    switch (pressCount) {
    case 1:

        switch (switchAction) {
        case ON_OFF_SWITCH_ACTIONS_OFF:
            zclGeneral_SendOnOff_CmdOff(endPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
            break;
        case ON_OFF_SWITCH_ACTIONS_ON:
            zclGeneral_SendOnOff_CmdOn(endPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
            break;
        case ON_OFF_SWITCH_ACTIONS_TOGGLE:
        default:
            zclGeneral_SendOnOff_CmdToggle(endPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
            break;
        }

        if (button % 2 == 0) {
            // even numbers (2 4, send up to lower odd number)
            zclGeneral_SendLevelControlStepWithOnOff(endPoint - 1, &inderect_DstAddr, LEVEL_STEP_UP, FREEPADAPP_LEVEL_STEP_SIZE,
                                                     FREEPADAPP_LEVEL_TRANSITION_TIME, TRUE, bdb_getZCLFrameCounter());

            zclLighting_ColorControl_Send_StepColorCmd(endPoint - 1, &inderect_DstAddr, 1000, 0, 0, true, bdb_getZCLFrameCounter());
            // works zclLighting_ColorControl_Send_MoveToColorCmd( endPoint - 1, &inderect_DstAddr, 45914, 19615, 0, TRUE,
            // bdb_getZCLFrameCounter());

            // zclLighting_ColorControl_Send_StepColorCmd( endPoint - 1, &inderect_DstAddr, 1000, 1000, 0, TRUE, bdb_getZCLFrameCounter());
            // zclLighting_ColorControl_Send_StepHueCmd(endPoint - 1, &inderect_DstAddr, LIGHTING_STEP_HUE_UP, 100, 0, TRUE,
            // bdb_getZCLFrameCounter());

        } else {
            // odd number (1 3, send LEVEL_MOVE_DOWN to self)
            zclGeneral_SendLevelControlStepWithOnOff(endPoint, &inderect_DstAddr, LEVEL_STEP_DOWN, FREEPADAPP_LEVEL_STEP_SIZE,
                                                     FREEPADAPP_LEVEL_TRANSITION_TIME, TRUE, bdb_getZCLFrameCounter());

            zclLighting_ColorControl_Send_StepColorCmd(endPoint - 1, &inderect_DstAddr, -1000, 0, 0, true, bdb_getZCLFrameCounter());
            // zclLighting_ColorControl_Send_StepSaturationCmd(endPoint, &inderect_DstAddr, LIGHTING_STEP_SATURATION_UP, 100, 0, TRUE,
            // bdb_getZCLFrameCounter());
            // works zclLighting_ColorControl_Send_MoveToColorCmd( endPoint, &inderect_DstAddr, 11298, 48942, 0, TRUE,
            // bdb_getZCLFrameCounter());
        }

        if (button % 4 == 1) {
            zclLighting_ColorControl_Send_StepColorCmd(endPoint, &inderect_DstAddr, FREEPADAPP_COLOR_LEVEL_STEP_X_SIZE,
                                                       FREEPADAPP_COLOR_LEVEL_STEP_Y_SIZE, FREEPADAPP_COLOR_LEVEL_TRANSITION_TIME, true,
                                                       bdb_getZCLFrameCounter());
        } else if (button % 4 == 2) {
            zclLighting_ColorControl_Send_StepColorCmd(endPoint - 1, &inderect_DstAddr, -FREEPADAPP_COLOR_LEVEL_STEP_X_SIZE,
                                                       FREEPADAPP_COLOR_LEVEL_STEP_Y_SIZE, FREEPADAPP_COLOR_LEVEL_TRANSITION_TIME, true,
                                                       bdb_getZCLFrameCounter());
        } else if (button % 4 == 3) {
            zclLighting_ColorControl_Send_StepColorCmd(endPoint - 2, &inderect_DstAddr, FREEPADAPP_COLOR_LEVEL_STEP_X_SIZE,
                                                       FREEPADAPP_COLOR_LEVEL_STEP_Y_SIZE, FREEPADAPP_COLOR_LEVEL_TRANSITION_TIME, true,
                                                       bdb_getZCLFrameCounter());
        } else if (button % 4 == 0) {
            zclLighting_ColorControl_Send_StepColorCmd(endPoint - 3, &inderect_DstAddr, -FREEPADAPP_COLOR_LEVEL_STEP_X_SIZE,
                                                       FREEPADAPP_COLOR_LEVEL_STEP_Y_SIZE, FREEPADAPP_COLOR_LEVEL_TRANSITION_TIME, true,
                                                       bdb_getZCLFrameCounter());
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

static void zclFreePadApp_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg) {
    LREP("ZCL_INCOMING_MSG srcAddr=0x%X endPoint=0x%X clusterId=0x%X commandID=0x%X %d\r\n", pInMsg->srcAddr, pInMsg->endPoint,
         pInMsg->clusterId, pInMsg->zclHdr.commandID, pInMsg->zclHdr.commandID);

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
                LREP("zclFreePadApp_NwkState=%d\r\n", zclFreePadApp_NwkState);
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
                LREP("SysEvent 0x%X status 0x%X macSrcAddr 0x%X endPoint=0x%X\r\n", MSGpkt->hdr.event, MSGpkt->hdr.status,
                     MSGpkt->macSrcAddr, MSGpkt->endPoint);
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
        zclFreePadApp_SendKeys(currentKeyCode, clicksCount, holdSend);
        clicksCount = 0;
        currentKeyCode = 0;
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
        byte button = zclFreePadApp_KeyCodeToButton(currentKeyCode);

        uint8 endPoint = zclFreePadApp_SimpleDescs[button - 1].EndPoint;
        zclFreePadApp_SendButtonPress(endPoint, HOLD_CODE);
        holdSend = true;

        return (events ^ FREEPADAPP_HOLD_START_EVT);
    }

    if (events & FREEPADAPP_SAVE_ATTRS_EVT) {
        LREPMaster("FREEPADAPP_SAVE_ATTRS_EVT\r\n");
        zclFreePadApp_SaveAttributesToNV();
        return (events ^ FREEPADAPP_SAVE_ATTRS_EVT);
    }

    if (events & FREEPADAPP_TL_START_EVT) {
        LREPMaster("FREEPADAPP_TL_START_EVT\r\n");
        HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
        zclFreePadApp_StartTL();
        return (events ^ FREEPADAPP_TL_START_EVT);
    }

    // Discard unknown events
    return 0;
}

static void zclFreePadApp_Rejoin(void) {
    LREP("Recieved rejoin command\r\n");
    if (bdb_isDeviceNonFactoryNew()) {
        zgWriteStartupOptions(ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE | ZCD_STARTOPT_DEFAULT_CONFIG_STATE);
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

        zcl_SendReportCmd(endPoint, &inderect_DstAddr, ZCL_CLUSTER_ID_GEN_MULTISTATE_INPUT_BASIC, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR,
                          TRUE, bdb_getZCLFrameCounter());
    }
    osal_mem_free(pReportCmd);
}

static void zclFreePadApp_HandleKeys(byte shift, byte keyCode) {
    static uint32 pressTime = 0;
    static byte prevKeyCode = 0;
    if (keyCode == prevKeyCode) {
        return;
    }

    prevKeyCode = keyCode;

    if (keyCode == HAL_KEY_CODE_RELEASE_KEY) {
        osal_stop_timerEx(zclFreePadApp_TaskID, FREEPADAPP_RESET_EVT);
        osal_stop_timerEx(zclFreePadApp_TaskID, FREEPADAPP_TL_START_EVT);
        byte prevButton = zclFreePadApp_KeyCodeToButton(currentKeyCode);
        uint8 prevSwitchType = zclFreePadApp_SwitchTypes[prevButton - 1];

        switch (prevSwitchType) {
        case ON_OFF_SWITCH_TYPE_TOGGLE:
        case ON_OFF_SWITCH_TYPE_MOMENTARY:
            LREPMaster("Rlease \r\n");
            break;

        case ON_OFF_SWITCH_TYPE_MULTIFUNCTION:
        default:
            if (pressTime > 0) {
                pressTime = 0;
                osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_SEND_KEYS_EVT, FREEPADAPP_SEND_KEYS_DELAY);
                osal_stop_timerEx(zclFreePadApp_TaskID, FREEPADAPP_HOLD_START_EVT);
            }
            break;
        }

    } else {
        byte button = zclFreePadApp_KeyCodeToButton(keyCode);
        uint32 resetHoldTime = FREEPADAPP_RESET_DELAY;
        uint32 TLHoldTime = FREEPADAPP_TL_START_DELAY;
        if (bdb_isDeviceNonFactoryNew()) {
            if (devState != DEV_END_DEVICE) {
                LREP("devState=%d try to restore network\r\n", devState);
                bdb_ZedAttemptRecoverNwk();
            }
            resetHoldTime = resetHoldTime >> 2;
            TLHoldTime = TLHoldTime >> 2;
        }

        switch (button) {
        case 1:
            osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_RESET_EVT, resetHoldTime);
            break;

        case 2:
            osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_TL_START_EVT, TLHoldTime);
            break;

        default:
            break;
        }

        uint8 switchType = zclFreePadApp_SwitchTypes[button - 1];

        HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
        pressTime = osal_getClock();

        currentKeyCode = keyCode;

        switch (switchType) {
        case ON_OFF_SWITCH_TYPE_TOGGLE:
        case ON_OFF_SWITCH_TYPE_MOMENTARY:
            clicksCount = 0;
            zclFreePadApp_SendKeys(keyCode, 1, false);
            LREPMaster("ON_OFF_SWITCH_TYPE_TOGGLE or ON_OFF_SWITCH_TYPE_MOMENTARY\r\n");
            break;

        case ON_OFF_SWITCH_TYPE_MULTIFUNCTION:
        default:
            clicksCount++;
            LREPMaster("ON_OFF_SWITCH_TYPE_MULTIFUNCTION or default\r\n");
            osal_stop_timerEx(zclFreePadApp_TaskID, FREEPADAPP_SEND_KEYS_EVT);
            osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_HOLD_START_EVT, FREEPADAPP_HOLD_START_DELAY);
            break;
        }
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
    zclFreePadApp_BatteryPercentageRemainig = getBatteryRemainingPercentageZCL();
    bdb_RepChangedAttrValue(1, ZCL_CLUSTER_ID_GEN_POWER_CFG, ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING);
}

static void zclFreePadApp_RestoreAttributesFromNV(void) {
    LREPMaster("Restoring attributes to NV size=%d \r\n");

    if (osal_nv_item_init(FREEPAD_NW_SWITCH_ACTIONS, FREEPAD_BUTTONS_COUNT, &zclFreePadApp_SwitchActions) == ZSuccess) {
        if (osal_nv_read(FREEPAD_NW_SWITCH_ACTIONS, 0, FREEPAD_BUTTONS_COUNT, &zclFreePadApp_SwitchActions) != ZSuccess) {
            LREPMaster("fail FREEPAD_NW_SWITCH_ACTIONS\r\n");
        }
    }
    if (osal_nv_item_init(FREEPAD_NW_SWITCH_TYPES, FREEPAD_BUTTONS_COUNT, &zclFreePadApp_SwitchTypes) == ZSuccess) {
        if (osal_nv_read(FREEPAD_NW_SWITCH_TYPES, 0, FREEPAD_BUTTONS_COUNT, &zclFreePadApp_SwitchTypes) != ZSuccess) {
            LREPMaster("fail FREEPAD_NW_SWITCH_TYPES\r\n");
        }
    }
}
static void zclFreePadApp_SaveAttributesToNV(void) {
    LREPMaster("Saving attributes to NV\r\n");
    if (osal_nv_item_init(FREEPAD_NW_SWITCH_ACTIONS, FREEPAD_BUTTONS_COUNT, &zclFreePadApp_SwitchActions) == ZSuccess) {
        osal_nv_write(FREEPAD_NW_SWITCH_ACTIONS, 0, FREEPAD_BUTTONS_COUNT, &zclFreePadApp_SwitchActions);
    }
    if (osal_nv_item_init(FREEPAD_NW_SWITCH_TYPES, FREEPAD_BUTTONS_COUNT, &zclFreePadApp_SwitchTypes) == ZSuccess) {
        osal_nv_write(FREEPAD_NW_SWITCH_TYPES, 0, FREEPAD_BUTTONS_COUNT, &zclFreePadApp_SwitchTypes);
    }
}

static void zclFreePadApp_StartTL(void) {
    LREPMaster("zclFreePadApp_StartTL\r\n");
    touchLinkInitiator_StartDevDisc();
}

ZStatus_t zclFreePadApp_TL_NotifyCb(epInfoRec_t *pData) {
    LREPMaster("zclFreePadApp_TL_NotifyCb\r\n");
    return touchLinkInitiator_ResetToFNSelectedTarget();
}
/****************************************************************************
****************************************************************************/
