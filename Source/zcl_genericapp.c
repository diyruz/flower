
#include "AF.h"
#include "MT_SYS.h"
#include "OSAL.h"
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
static uint8 halKeySavedKeys;

afAddrType_t Coordinator_DstAddr = {.addrMode = (afAddrMode_t)Addr16Bit, .addr.shortAddr = 0, .endPoint = 1};

afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 0, .addr.shortAddr = 0};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclGenericApp_HandleKeys(byte shift, byte keys);
static void zclGenericApp_BasicResetCB(void);
static void zclGenericApp_BindNotification(bdbBindNotificationData_t *data);
bool halProcessKeyInterrupt(void);

void GenericApp_HalKeyInit(void);
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
    LREPMaster("Initialized debug module \n");
    LREPMaster("Hello world \n");
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

/*********************************************************************
 * @fn          zclSample_event_loop
 *
 * @brief       Event Loop Processor for zclGeneral.
 *
 * @param       none
 *
 * @return      none
 */
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
                    LREPMaster("Connected to network....\n");
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

#if ZG_BUILD_ENDDEVICE_TYPE
    if (events & GENERICAPP_END_DEVICE_REJOIN_EVT) {
        bdb_ZedAttemptRecoverNwk();
        return (events ^ GENERICAPP_END_DEVICE_REJOIN_EVT);
    }
#endif

    /* GENERICAPP_TODO: handle app events here */

    if (events & HAL_LED_BLINK_EVENT) {
        // toggle LED 2 state, start another timer for 500ms
        HalLedSet(HAL_LED_2, HAL_LED_MODE_TOGGLE);
        // osal_start_timerEx(zclGenericApp_TaskID, HAL_LED_BLINK_EVENT, 500);
        return (events ^ HAL_LED_BLINK_EVENT);
    }

    if (events & GENERICAPP_EVT_GO_TO_SLEEP) {
        LREPMaster("Going to sleep....\n");
        halSleep(10000);
        return (events ^ GENERICAPP_EVT_GO_TO_SLEEP);
    }

    if (events & GENERICAPP_SW1_LONG_PRESS) {
        LREPMaster("GENERICAPP_SW1_LONG_PRESS detected \n");

        initilizeRejoin = true;

        return events ^ GENERICAPP_SW1_LONG_PRESS;
    }

    // событие опроса кнопок
    if (events & HAL_KEY_EVENT) {
        /* Считывание кнопок */
        GenericApp_HalKeyPoll();

        return events ^ HAL_KEY_EVENT;
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
    LREPMaster("Leaving network\n");
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
        LREPMaster("Couldn't send out leave; prepare to reset anyway\n");
        ZDApp_LeaveReset(FALSE);
    }
}
void rejoin(void) {
    initilizeRejoin = false;
    if (bdbAttributes.bdbNodeIsOnANetwork) {
        zclGenericApp_LeaveNetwork();
    } else {
        LREPMaster("bdb_StartCommissioning.(BDB_COMMISSIONING_MODE_NWK_STEERING)...\n");
        bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING);
        osal_start_reload_timer(zclGenericApp_TaskID, HAL_LED_BLINK_EVENT, 100);
    }
}
static void zclGenericApp_HandleKeys(byte shift, byte keys) {
    LREP("zclGenericApp_HandleKeys" BYTE_TO_BINARY_PATTERN "", BYTE_TO_BINARY(keys));
    LREPMaster("\n");

    if (!keys) {
        LREPMaster("Button released\n");

        osal_stop_timerEx(zclGenericApp_TaskID, GENERICAPP_SW1_LONG_PRESS);
        osal_clear_event(zclGenericApp_TaskID, GENERICAPP_SW1_LONG_PRESS);

        osal_stop_timerEx(zclGenericApp_TaskID, HAL_KEY_EVENT);
        osal_clear_event(zclGenericApp_TaskID, HAL_KEY_EVENT);

        halKeySavedKeys = 0;

        if (initilizeRejoin) {
            rejoin();
        }
        // osal_start_timerEx(zclGenericApp_TaskID, GENERICAPP_EVT_GO_TO_SLEEP,
        // 10000);
    }

    if (keys & HAL_KEY_SW_1) {
        LREPMaster("Pressed button 1\n");
        zclGeneral_SendOnOff_CmdToggle(zclGenericApp_SimpleDescs[0].EndPoint, &inderect_DstAddr, FALSE, bdb_getZCLFrameCounter());

        zclGenericApp_ReportBattery();
    }
    if (keys & HAL_KEY_SW_2) {
        LREPMaster("Pressed button2\n");
        zclGeneral_SendOnOff_CmdToggle(zclGenericApp_SimpleDescs[1].EndPoint, &inderect_DstAddr, FALSE, bdb_getZCLFrameCounter());
    }
}

void GenericApp_HalKeyInit(void) {
    /* Сбрасываем сохраняемое состояние кнопок в 0 */
    halKeySavedKeys = 0;

    PUSH1_SEL &= ~(PUSH1_BV);
    PUSH1_DIR &= ~(PUSH1_BV);
    PUSH1_ICTL |= PUSH1_ICTLBIT;
    PUSH1_IEN |= PUSH1_IENBIT;
    PUSH1_PXIFG = ~(PUSH1_BIT);

    PICTL &= ~(PUSH1_EDGEBIT); /* Clear the edge bit */
/* For falling edge, the bit must be set. */
#if (PUSH1_EDGE == HAL_KEY_FALLING_EDGE)
    PICTL |= PUSH1_EDGEBIT;
#endif

    PUSH2_SEL &= ~(PUSH2_BV);
    PUSH2_DIR &= ~(PUSH2_BV);
    PUSH2_ICTL |= PUSH2_ICTLBIT;
    PUSH2_IEN |= PUSH2_IENBIT;
    PUSH2_PXIFG = ~(PUSH2_BIT);

    PICTL &= ~(PUSH2_EDGEBIT); /* Clear the edge bit */
/* For falling edge, the bit must be set. */
#if (PUSH2_EDGE == HAL_KEY_FALLING_EDGE)
    PICTL |= PUSH2_EDGEBIT;
#endif
}

bool halProcessKeyInterrupt(void) {
    bool valid = false;

    if (PUSH1_PXIFG & PUSH1_BIT) /* Interrupt Flag has been set */
    {
        PUSH1_PXIFG = ~(PUSH1_BIT); /* Clear Interrupt Flag */
        valid = TRUE;
    }

    if (PUSH2_PXIFG & PUSH2_BIT) /* Interrupt Flag has been set */
    {
        PUSH2_PXIFG = ~(PUSH2_BIT); /* Clear Interrupt Flag */
        valid = TRUE;
    }

    if (valid) {
        osal_start_reload_timer(zclGenericApp_TaskID, HAL_KEY_EVENT, 100);
        osal_start_timerEx(zclGenericApp_TaskID, GENERICAPP_SW1_LONG_PRESS, 5000);
    }
    return valid;
}

HAL_ISR_FUNCTION(halKeyPort2Isr, P2INT_VECTOR) {
    HAL_ENTER_ISR();
    if (halProcessKeyInterrupt()) {
        HAL_KEY_CPU_PORT_2_IF = 0;
        PUSH2_PXIFG = 0;
        CLEAR_SLEEP_MODE();
    }

    HAL_EXIT_ISR();
}

HAL_ISR_FUNCTION(halKeyPort0Isr, P0INT_VECTOR) {
    HAL_ENTER_ISR();
    if (halProcessKeyInterrupt()) {
        HAL_KEY_CPU_PORT_0_IF = 0;
        PUSH1_PXIFG = 0;
        CLEAR_SLEEP_MODE();
    }

    HAL_EXIT_ISR();
}

// Считывание кнопок
void GenericApp_HalKeyPoll(void) {
    uint8 keys = 0;

    // нажата кнопка 1 ?
    if (HAL_PUSH_BUTTON1()) {
        keys |= HAL_KEY_SW_1;
    }

    // нажата кнопка 2 ?
    if (HAL_PUSH_BUTTON2()) {
        keys |= HAL_KEY_SW_2;
    }

    if (keys == halKeySavedKeys) {
        // Выход - нет изменений
        return;
    }
    // Сохраним текущее состояние кнопок для сравнения в след раз
    halKeySavedKeys = keys;

    // Вызовем генерацию события изменений кнопок
    OnBoard_SendKeys(keys, HAL_KEY_STATE_NORMAL);
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
    LREPMaster("zclGenericApp_ProcessInReadRspCmd\n");

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
