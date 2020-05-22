
#include "AF.h"
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
#include "zcl_app.h"
#include "zcl_general.h"
#include "zcl_lighting.h"
#include "zcl_ms.h"

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
extern bool requestNewTrustCenterLinkKey;
byte zclFlowerApp_TaskID;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
byte rejoinsLeft = FLOWER_APP_END_DEVICE_REJOIN_TRIES;
uint32 rejoinDelay = FLOWER_APP_END_DEVICE_REJOIN_START_DELAY;

afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 0, .addr.shortAddr = 0};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclFlowerApp_HandleKeys(byte shift, byte keys);
static void zclFlowerApp_BindNotification(bdbBindNotificationData_t *data);
static void zclFlowerApp_Report(void);
static void zclFlowerApp_Rejoin(void);

static void zclFlowerApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);
static void zclFlowerApp_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg);
static void zclFlowerApp_ResetBackoffRetry(void);
static void zclFlowerApp_OnConnect(void);

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclFlowerApp_CmdCallbacks = {
    NULL, // Basic Cluster Reset command
    NULL, // Identify Trigger Effect command
    NULL, // On/Off cluster commands
    NULL, // On/Off cluster enhanced command Off with Effect
    NULL, // On/Off cluster enhanced command On with Recall Global Scene
    NULL, // On/Off cluster enhanced command On with Timed Off
    NULL, // RSSI Location command
    NULL  // RSSI Location Response command
};

void zclFlowerApp_Init(byte task_id) {
    DebugInit();
    // this is important to allow connects throught routers
    // to make this work, coordinator should be compiled with this flag #define TP2_LEGACY_ZC
    requestNewTrustCenterLinkKey = FALSE;

    zclFlowerApp_TaskID = task_id;

    zclGeneral_RegisterCmdCallbacks(1, &zclFlowerApp_CmdCallbacks);
    zcl_registerAttrList(zclFlowerApp_FirstEP.EndPoint, zclFlowerApp_AttrsFirstEPCount, zclFlowerApp_AttrsFirstEP);
    bdb_RegisterSimpleDescriptor(&zclFlowerApp_FirstEP);

    zcl_registerAttrList(zclFlowerApp_SecondEP.EndPoint, zclFlowerApp_AttrsSecondEPCount, zclFlowerApp_AttrsSecondEP);
    bdb_RegisterSimpleDescriptor(&zclFlowerApp_SecondEP);

    zcl_registerForMsg(zclFlowerApp_TaskID);

    // Register for all key events - This app will handle all key events
    RegisterForKeys(zclFlowerApp_TaskID);

    bdb_RegisterBindNotificationCB(zclFlowerApp_BindNotification);
    bdb_RegisterCommissioningStatusCB(zclFlowerApp_ProcessCommissioningStatus);

    bdb_StartCommissioning(BDB_COMMISSIONING_REJOIN_EXISTING_NETWORK_ON_STARTUP);

    LREP("Started build %s \r\n", zclFlowerApp_DateCodeNT);
    osal_start_reload_timer(zclFlowerApp_TaskID, FLOWER_APP_REPORT_EVT, FLOWER_APP_REPORT_DELAY);
    // this allows power saving, PM2
    osal_pwrmgr_task_state(zclFlowerApp_TaskID, PWRMGR_CONSERVE);

    LREP("Battery voltage=%d prc=%d \r\n", getBatteryVoltage(), getBatteryRemainingPercentageZCL());
    ZMacSetTransmitPower(TX_PWR_PLUS_4); // set 4dBm
}

static void zclFlowerApp_ResetBackoffRetry(void) {
    rejoinsLeft = FLOWER_APP_END_DEVICE_REJOIN_TRIES;
    rejoinDelay = FLOWER_APP_END_DEVICE_REJOIN_START_DELAY;
}

static void zclFlowerApp_OnConnect(void) {
    zclFlowerApp_ResetBackoffRetry();
    osal_start_timerEx(zclFlowerApp_TaskID, FLOWER_APP_REPORT_EVT, FLOWER_APP_CONST_ONE_MINUTE_IN_MS); // 1 minute
}

static void zclFlowerApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg) {
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
            zclFlowerApp_OnConnect();
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
            zclFlowerApp_OnConnect();
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
            zclFlowerApp_ResetBackoffRetry();
            break;

        default:
            HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
            // // Parent not found, attempt to rejoin again after a exponential backoff delay
            LREP("rejoinsLeft %d rejoinDelay=%ld\r\n", rejoinsLeft, rejoinDelay);
            if (rejoinsLeft > 0) {
                rejoinDelay *= FLOWER_APP_END_DEVICE_REJOIN_BACKOFF;
                rejoinsLeft -= 1;
            } else {
                rejoinDelay = FLOWER_APP_END_DEVICE_REJOIN_MAX_DELAY;
            }
            osal_start_timerEx(zclFlowerApp_TaskID, FLOWER_APP_END_DEVICE_REJOIN_EVT, rejoinDelay);
            break;
        }
        break;
    default:
        break;
    }
}

static void zclFlowerApp_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg) {
    LREP("ZCL_INCOMING_MSG srcAddr=0x%X endPoint=0x%X clusterId=0x%X commandID=0x%X %d\r\n", pInMsg->srcAddr, pInMsg->endPoint,
         pInMsg->clusterId, pInMsg->zclHdr.commandID, pInMsg->zclHdr.commandID);

    if (pInMsg->attrCmd)
        osal_mem_free(pInMsg->attrCmd);
}
uint16 zclFlowerApp_event_loop(uint8 task_id, uint16 events) {
    afIncomingMSGPacket_t *MSGpkt;

    (void)task_id; // Intentionally unreferenced parameter
    devStates_t zclFlowerApp_NwkState;
    if (events & SYS_EVENT_MSG) {
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclFlowerApp_TaskID))) {

            switch (MSGpkt->hdr.event) {

            case KEY_CHANGE:
                zclFlowerApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;
            case ZDO_STATE_CHANGE:
                HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
                zclFlowerApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
                LREP("zclFlowerApp_NwkState=%d\r\n", zclFlowerApp_NwkState);
                if (zclFlowerApp_NwkState == DEV_END_DEVICE) {
                    HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
                }
                break;

            case ZCL_INCOMING_MSG:
                zclFlowerApp_ProcessIncomingMsg((zclIncomingMsg_t *)MSGpkt);
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
    if (events & FLOWER_APP_END_DEVICE_REJOIN_EVT) {
        LREPMaster("FLOWER_APP_END_DEVICE_REJOIN_EVT\r\n");
        bdb_ZedAttemptRecoverNwk();
        return (events ^ FLOWER_APP_END_DEVICE_REJOIN_EVT);
    }

    if (events & FLOWER_APP_RESET_EVT) {
        LREPMaster("FLOWER_APP_RESET_EVT\r\n");
        zclFlowerApp_Rejoin();
        return (events ^ FLOWER_APP_RESET_EVT);
    }

    if (events & FLOWER_APP_REPORT_EVT) {
        LREPMaster("FLOWER_APP_REPORT_EVT\r\n");
        zclFlowerApp_Report();
        return (events ^ FLOWER_APP_REPORT_EVT);
    }

    // Discard unknown events
    return 0;
}

static void zclFlowerApp_Rejoin(void) {
    LREPMaster("Recieved rejoin command\r\n");
    HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
    if (bdbAttributes.bdbNodeIsOnANetwork) {
        LREPMaster("Reset to FN\r\n");
        bdb_resetLocalAction();
    } else {
        LREPMaster("StartCommissioning STEEREING\r\n");
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING);
    }
}

static void zclFlowerApp_HandleKeys(byte shift, byte keyCode) {
    static byte prevKeyCode = 0;
    if (keyCode == prevKeyCode) {
        return;
    }
    LREP("devState %d bdbNodeIsOnANetwork %d \r\n", devState, bdbAttributes.bdbNodeIsOnANetwork);

    prevKeyCode = keyCode;

    if (keyCode == HAL_KEY_CODE_RELEASE_KEY) {
        osal_stop_timerEx(zclFlowerApp_TaskID, FLOWER_APP_RESET_EVT);
    } else {
        byte button = zclFlowerApp_KeyCodeToButton(keyCode);
        uint32 resetHoldTime = FLOWER_APP_RESET_DELAY;
        if (devState == DEV_NWK_ORPHAN) {
            LREP("devState=%d try to restore network\r\n", devState);
            bdb_ZedAttemptRecoverNwk();
        }
        if (!bdbAttributes.bdbNodeIsOnANetwork) {
            resetHoldTime = resetHoldTime >> 2;
        }

        LREP("resetHoldTime %ld\r\n", resetHoldTime);
        osal_start_timerEx(zclFlowerApp_TaskID, FLOWER_APP_RESET_EVT, resetHoldTime);
    }
}

static void zclFlowerApp_BindNotification(bdbBindNotificationData_t *data) {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
    LREP("Recieved bind request clusterId=0x%X dstAddr=0x%X \r\n", data->clusterId, data->dstAddr);
    uint16 maxEntries = 0, usedEntries = 0;
    bindCapacity(&maxEntries, &usedEntries);
    LREP("bindCapacity %d %usedEntries %d \r\n", maxEntries, usedEntries);
}

static void zclFlowerApp_Report(void) {
    zclFlowerApp_BatteryVoltage = getBatteryVoltage();
    zclFlowerApp_BatteryPercentageRemainig = getBatteryRemainingPercentageZCL();
    // todo: update sensors

    bdb_RepChangedAttrValue(zclFlowerApp_FirstEP.EndPoint, POWER_CFG, ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING);

    bdb_RepChangedAttrValue(zclFlowerApp_FirstEP.EndPoint, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
    bdb_RepChangedAttrValue(zclFlowerApp_FirstEP.EndPoint, PRESSURE, ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE);
    bdb_RepChangedAttrValue(zclFlowerApp_FirstEP.EndPoint, HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE);

    bdb_RepChangedAttrValue(zclFlowerApp_SecondEP.EndPoint, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
}

/****************************************************************************
****************************************************************************/
