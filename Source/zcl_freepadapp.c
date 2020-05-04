
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

uint32 timeDiff = 0;
byte currentKey = 0;
byte clicksCount = 0;

devStates_t zclFreePadApp_NwkState = DEV_INIT;

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
static void zclFreePadApp_Send_Keys(byte keyCode, byte pressCount, byte pressTime);

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
    bdb_StartCommissioning(BDB_COMMISSIONING_NWK_STEERING | BDB_COMMISSIONING_FINDING_BINDING);

    DebugInit();
    HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);

    osal_start_reload_timer(zclFreePadApp_TaskID, FREEPADAPP_SLEEP_EVT, (uint32)FREEPADAPP_SLEEP_DELAY);
}

static void zclFreePadApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg) {
    LREP("bdbCommissioningMode=%d bdbCommissioningStatus=%d bdbRemainingCommissioningModes=0x%X\n\r",
         bdbCommissioningModeMsg->bdbCommissioningMode, bdbCommissioningModeMsg->bdbCommissioningStatus,
         bdbCommissioningModeMsg->bdbRemainingCommissioningModes);
    switch (bdbCommissioningModeMsg->bdbCommissioningMode) {
    case BDB_COMMISSIONING_NWK_STEERING:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_SUCCESS:
            HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
            break;

        default:
            HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
            osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_END_DEVICE_REJOIN_EVT,
                               FREEPADAPP_END_DEVICE_REJOIN_DELAY);
            break;
        }

        break;

    case BDB_COMMISSIONING_PARENT_LOST:
        if (bdbCommissioningModeMsg->bdbCommissioningStatus != BDB_COMMISSIONING_NETWORK_RESTORED) {
            HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
            // Parent not found, attempt to rejoin again after a fixed delay
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
static void zclFreePadApp_Send_Keys(byte keyCode, byte pressCount, byte holdTime) {

    byte button = zclFreePadApp_KeyCodeToButton(keyCode);
    LREP("button %d clicks %d hold %d\n\r", button, pressCount, holdTime);
    if (button != HAL_UNKNOWN_BUTTON) {
        uint8 endPoint = zclFreePadApp_SimpleDescs[button - 1].EndPoint;
        switch (pressCount) {
        case 1:
            zclGeneral_SendOnOff_CmdToggle(endPoint, &inderect_DstAddr, FALSE, bdb_getZCLFrameCounter());

            if (button % 2 == 0) {
                // even numbers (2 4, send up to lower odd number)
                zclGeneral_SendLevelControlMoveWithOnOff(endPoint - 1, &inderect_DstAddr, LEVEL_MOVE_UP,
                                                         FREEPADAPP_CMD_LEVEL_MOVE_RATE, FALSE,
                                                         bdb_getZCLFrameCounter());
            } else {
                // odd number (1 3, send LEVEL_MOVE_DOWN to self)
                zclGeneral_SendLevelControlMoveWithOnOff(endPoint, &inderect_DstAddr, LEVEL_MOVE_DOWN,
                                                         FREEPADAPP_CMD_LEVEL_MOVE_RATE, FALSE,
                                                         bdb_getZCLFrameCounter());
            }

            break;
        case 2:
            break;

        default:
            break;
        }

        if (holdTime >= 2) {
            zclFreePadApp_SendButtonPress(endPoint, HOLD_CODE);
        } else {
            zclFreePadApp_SendButtonPress(endPoint, pressCount);
        }
        zclFreePadApp_SendButtonPress(endPoint, RELEASE_CODE);
        if (button == 1) {
            zclFreePadApp_ReportBattery();
        }
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
    if (events & FREEPADAPP_SEND_KEYS_EVT) {
        zclFreePadApp_Send_Keys(currentKey, clicksCount, timeDiff);
        clicksCount = 0;
        currentKey = 0;
        return (events ^ FREEPADAPP_SEND_KEYS_EVT);
    }

    if (events & FREEPADAPP_RESET_EVT) {
        LREPMaster("Initialize rejoin seqence \n\r");
        zclFreePadApp_Rejoin();
        return (events ^ FREEPADAPP_RESET_EVT);
    }

    if (events & FREEPADAPP_SLEEP_EVT) {
        LREPMaster("Sleep... \n\r");
#ifdef HAL_BOARD_FREEPAD
        osal_clear_event(zclFreePadApp_TaskID, FREEPADAPP_SLEEP_EVT);
        halSleep(0); // sleep
#endif
        return (events ^ FREEPADAPP_SLEEP_EVT);
    }

    // Discard unknown events
    return 0;
}

static void zclFreePadApp_Rejoin(void) { bdb_resetLocalAction(); }

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
                          ZCL_FRAME_CLIENT_SERVER_DIR, false, bdb_getZCLFrameCounter());
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
            timeDiff = (osal_getClock() - pressTime);
            pressTime = 0;
            osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_SEND_KEYS_EVT, FREEPADAPP_SEND_KEYS_DELAY);
        }
    } else {
        osal_stop_timerEx(zclFreePadApp_TaskID, FREEPADAPP_SLEEP_EVT);
        osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_SLEEP_EVT, (uint32)FREEPADAPP_SLEEP_DELAY);


        osal_start_timerEx(zclFreePadApp_TaskID, FREEPADAPP_RESET_EVT, FREEPADAPP_RESET_DELAY);
        osal_stop_timerEx(zclFreePadApp_TaskID, FREEPADAPP_SEND_KEYS_EVT);
        bdb_ZedAttemptRecoverNwk();
        HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
        pressTime = osal_getClock();
        clicksCount++;
        currentKey = keys;
    }
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
