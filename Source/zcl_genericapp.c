/**************************************************************************************************
  Filename:       zcl_genericapp.c
  Revised:        $Date: 2014-10-24 16:04:46 -0700 (Fri, 24 Oct 2014) $
  Revision:       $Revision: 40796 $


  Description:    Zigbee Cluster Library - sample device application.


  Copyright 2006-2014 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
  This application is a template to get started writing an application
  from scratch.

  Look for the sections marked with "GENERICAPP_TODO" to add application
  specific code.

  Note: if you would like your application to support automatic attribute
  reporting, include the BDB_REPORTING compile flag.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "MT_SYS.h"

#include "nwk_util.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_diagnostic.h"
#include "zcl_genericapp.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "gp_interface.h"

#include "Debug.h"

#if defined(INTER_PAN)
#if defined(BDB_TL_INITIATOR)
#include "bdb_touchlink_initiator.h"
#endif // BDB_TL_INITIATOR
#if defined(BDB_TL_TARGET)
#include "bdb_touchlink_target.h"
#endif // BDB_TL_TARGET
#endif // INTER_PAN

#if defined(BDB_TL_INITIATOR) || defined(BDB_TL_TARGET)
#include "bdb_touchlink.h"
#endif

#include "onboard.h"

/* HAL */
#include "hal_led.h"
#include "hal_key.h"
#include "hal_drivers.h"


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

uint8 gPermitDuration = 0; // permit joining default to disabled
uint8 SeqNum = 0;
devStates_t zclGenericApp_NwkState = DEV_INIT;
static uint8 halKeySavedKeys;


// Endpoint to allow SYS_APP_MSGs
static endPointDesc_t sampleSw_TestEp =
{
  1,                  // endpoint
  0,
  &zclGenericApp_TaskID,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclGenericApp_HandleKeys(byte shift, byte keys);
static void zclGenericApp_BasicResetCB(void);
static void zclGenericApp_ProcessIdentifyTimeChange(uint8 endpoint);
static void zclGenericApp_BindNotification(bdbBindNotificationData_t *data);
#if (defined(BDB_TL_TARGET) && (BDB_TOUCHLINK_CAPABILITY_ENABLED == TRUE))
static void zclGenericApp_ProcessTouchlinkTargetEnable(uint8 enable);
#endif


void DIYRuZRT_HalKeyInit( void );
void zclDIYRuZRT_ReportOnOff( void );

static void zclGenericApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);

// Functions to process ZCL Foundation incoming Command/Response messages
static void zclGenericApp_ProcessIncomingMsg(zclIncomingMsg_t *msg);
#ifdef ZCL_READ
static uint8 zclGenericApp_ProcessInReadRspCmd(zclIncomingMsg_t *pInMsg);
#endif
#ifdef ZCL_WRITE
static uint8 zclGenericApp_ProcessInWriteRspCmd(zclIncomingMsg_t *pInMsg);
#endif
static uint8 zclGenericApp_ProcessInDefaultRspCmd(zclIncomingMsg_t *pInMsg);
#ifdef ZCL_DISCOVER
static uint8 zclGenericApp_ProcessInDiscCmdsRspCmd(zclIncomingMsg_t *pInMsg);
static uint8 zclGenericApp_ProcessInDiscAttrsRspCmd(zclIncomingMsg_t *pInMsg);
static uint8 zclGenericApp_ProcessInDiscAttrsExtRspCmd(zclIncomingMsg_t *pInMsg);
#endif

static void zclSampleApp_BatteryWarningCB(uint8 voltLevel);

/*********************************************************************
 * STATUS STRINGS
 */

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclGenericApp_CmdCallbacks =
    {
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

/*********************************************************************
 * GENERICAPP_TODO: Add other callback structures for any additional application specific 
 *       Clusters being used, see available callback structures below.
 *
 *       bdbTL_AppCallbacks_t 
 *       zclApplianceControl_AppCallbacks_t 
 *       zclApplianceEventsAlerts_AppCallbacks_t 
 *       zclApplianceStatistics_AppCallbacks_t 
 *       zclElectricalMeasurement_AppCallbacks_t 
 *       zclGeneral_AppCallbacks_t 
 *       zclGp_AppCallbacks_t 
 *       zclHVAC_AppCallbacks_t 
 *       zclLighting_AppCallbacks_t 
 *       zclMS_AppCallbacks_t 
 *       zclPollControl_AppCallbacks_t 
 *       zclPowerProfile_AppCallbacks_t 
 *       zclSS_AppCallbacks_t  
 *
 */

/*********************************************************************
 * @fn          zclGenericApp_Init
 *
 * @brief       Initialization function for the zclGeneral layer.
 *
 * @param       none
 *
 * @return      none
 */
void zclGenericApp_Init(byte task_id)
{
  zclGenericApp_TaskID = task_id;

  for (int i =0; i < zclGenericApp_SimpleDescsCount; i++) {
    bdb_RegisterSimpleDescriptor(&zclGenericApp_SimpleDescs[i]);
    // Register the ZCL General Cluster Library callback functions
    zclGeneral_RegisterCmdCallbacks(zclGenericApp_SimpleDescs[i].EndPoint, &zclGenericApp_CmdCallbacks);

     // Register the application's attribute list
  zcl_registerAttrList(zclGenericApp_SimpleDescs[i].EndPoint, zclGenericApp_NumAttributes, zclGenericApp_Attrs);

  #ifdef ZCL_DISCOVER
    // Register the application's command list
    zcl_registerCmdList(zclGenericApp_SimpleDescs[i].EndPoint, zclCmdsArraySize, zclGenericApp_Cmds);
  #endif


    // Register for a test endpoint
  afRegister( &sampleSw_TestEp );

  #ifdef ZCL_DIAGNOSTIC
    // Register the application's callback function to read/write attribute data.
    // This is only required when the attribute data format is unknown to ZCL.
    zcl_registerReadWriteCB(zclGenericApp_SimpleDescs[i].EndPoint, zclDiagnostic_ReadWriteAttrCB, NULL);

    if (zclDiagnostic_InitStats() == ZSuccess)
    {
      // Here the user could start the timer to save Diagnostics to NV
    }
  #endif
  }

  // GENERICAPP_TODO: Register other cluster command callbacks here


  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg(zclGenericApp_TaskID);


  // Register low voltage NV memory protection application callback
  RegisterVoltageWarningCB(zclSampleApp_BatteryWarningCB);

  // Register for all key events - This app will handle all key events
  RegisterForKeys(zclGenericApp_TaskID);

  bdb_RegisterCommissioningStatusCB(zclGenericApp_ProcessCommissioningStatus);
  bdb_RegisterIdentifyTimeChangeCB(zclGenericApp_ProcessIdentifyTimeChange);
  bdb_RegisterBindNotificationCB(zclGenericApp_BindNotification);

#if (defined(BDB_TL_TARGET) && (BDB_TOUCHLINK_CAPABILITY_ENABLED == TRUE))
  bdb_RegisterTouchlinkTargetEnableCB(zclGenericApp_ProcessTouchlinkTargetEnable);
#endif

  bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_FORMATION | BDB_COMMISSIONING_MODE_NWK_STEERING | BDB_COMMISSIONING_MODE_FINDING_BINDING);

  DebugInit();
  LREPMaster("Initialized debug module \n");
  LREPMaster("Hello world \n");

    osal_start_reload_timer( zclGenericApp_TaskID, HAL_KEY_EVENT, 100);

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
uint16 zclGenericApp_event_loop(uint8 task_id, uint16 events)
{
  afIncomingMSGPacket_t *MSGpkt;

  (void)task_id; // Intentionally unreferenced parameter

  if (events & SYS_EVENT_MSG)
  {
    while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclGenericApp_TaskID)))
    {
      switch (MSGpkt->hdr.event)
      {
      case ZCL_INCOMING_MSG:
        // Incoming ZCL Foundation command/response messages
        zclGenericApp_ProcessIncomingMsg((zclIncomingMsg_t *)MSGpkt);
        break;

      case KEY_CHANGE:
        zclGenericApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
        break;

      case ZDO_STATE_CHANGE:
        zclGenericApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
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
  if (events & GENERICAPP_END_DEVICE_REJOIN_EVT)
  {
    bdb_ZedAttemptRecoverNwk();
    return (events ^ GENERICAPP_END_DEVICE_REJOIN_EVT);
  }
#endif

  /* GENERICAPP_TODO: handle app events here */

  if (events & GENERICAPP_EVT_1)
  {
    // toggle LED 2 state, start another timer for 500ms
    HalLedSet(HAL_LED_2, HAL_LED_MODE_TOGGLE);
    osal_start_timerEx(zclGenericApp_TaskID, GENERICAPP_EVT_1, 500);

    return (events ^ GENERICAPP_EVT_1);
  }

  /*
  if ( events & GENERICAPP_EVT_2 )
  {
    
    return ( events ^ GENERICAPP_EVT_2 );
  }
  
  if ( events & GENERICAPP_EVT_3 )
  {
    
    return ( events ^ GENERICAPP_EVT_3 );
  }
  */

  // событие опроса кнопок
  if (events & HAL_KEY_EVENT)
  {
    /* Считывание кнопок */
    DIYRuZRT_HalKeyPoll();

    return events ^ HAL_KEY_EVENT;
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      zclGenericApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_5
 *                 HAL_KEY_SW_4
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
static void zclGenericApp_HandleKeys(byte shift, byte keys)
{
  LREP("zclGenericApp_HandleKeys %x", keys & 0xff);
  if (keys & HAL_KEY_SW_1)
  {
    LREPMaster("Pressed button 1");
    static bool LED_OnOff = FALSE;

    /* GENERICAPP_TODO: add app functionality to hardware keys here */

    // for example, start/stop LED 2 toggling with 500ms period
    if (LED_OnOff)
    {
      // if the LED is blinking, stop the osal timer and turn the LED off
      osal_stop_timerEx(zclGenericApp_TaskID, GENERICAPP_EVT_1);
      HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
      LED_OnOff = FALSE;
    }
    else
    {
      // turn on LED 2 and start an osal timer to toggle it after 500ms, search
      // for GENERICAPP_EVT_1 to see event handling after expired timer
      osal_start_timerEx(zclGenericApp_TaskID, GENERICAPP_EVT_1, 500);
      HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
      LED_OnOff = TRUE;
    }
  }
  // Start the BDB commissioning method
  if (keys & HAL_KEY_SW_2)
  {
    LREPMaster("Pressed button 2");

    bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_FORMATION | BDB_COMMISSIONING_MODE_NWK_STEERING | BDB_COMMISSIONING_MODE_FINDING_BINDING | BDB_COMMISSIONING_MODE_INITIATOR_TL);
  }
  if (keys & HAL_KEY_SW_3)
  {

    // touchlink target commissioning, if enabled
#if (defined(BDB_TL_TARGET) && (BDB_TOUCHLINK_CAPABILITY_ENABLED == TRUE))
    bdb_StartCommissioning(BDB_COMMISSIONING_MODE_FINDING_BINDING);
    touchLinkTarget_EnableCommissioning(30000);
#endif
  }

  if (keys & HAL_KEY_SW_5)
  {
    bdb_resetLocalAction();
  }
}

void DIYRuZRT_HalKeyInit( void )
{
  /* Сбрасываем сохраняемое состояние кнопок в 0 */
  halKeySavedKeys = 0;

  PUSH1_SEL &= ~(PUSH1_BV); /* Выставляем функцию пина - GPIO */
  PUSH1_DIR &= ~(PUSH1_BV); /* Выставляем режим пина - Вход */
  
  PUSH1_ICTL &= ~(PUSH1_ICTLBIT); /* Не генерируем прерывания на пине */
  PUSH1_IEN &= ~(PUSH1_IENBIT);   /* Очищаем признак включения прерываний */
  
  PUSH2_SEL &= ~(PUSH2_BV); /* Set pin function to GPIO */
  PUSH2_DIR &= ~(PUSH2_BV); /* Set pin direction to Input */
  
  PUSH2_ICTL &= ~(PUSH2_ICTLBIT); /* don't generate interrupt */
  PUSH2_IEN &= ~(PUSH2_IENBIT);   /* Clear interrupt enable bit */
}


// Считывание кнопок
void DIYRuZRT_HalKeyPoll (void)
{
  uint8 keys = 0;

  // нажата кнопка 1 ?
  if (HAL_PUSH_BUTTON1())
  {
    keys |= HAL_KEY_SW_1;
  }
  
  // нажата кнопка 2 ?
  if (HAL_PUSH_BUTTON2())
  {
    keys |= HAL_KEY_SW_2;
  }
  
  if (keys == halKeySavedKeys)
  {
    // Выход - нет изменений
    return;
  }
  // Сохраним текущее состояние кнопок для сравнения в след раз
  halKeySavedKeys = keys;

  // Вызовем генерацию события изменений кнопок
  OnBoard_SendKeys(keys, HAL_KEY_STATE_NORMAL);
}

/*********************************************************************
 * @fn      zclGenericApp_ProcessCommissioningStatus
 *
 * @brief   Callback in which the status of the commissioning process are reported
 *
 * @param   bdbCommissioningModeMsg - Context message of the status of a commissioning process
 *
 * @return  none
 */
static void zclGenericApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg)
{
  switch (bdbCommissioningModeMsg->bdbCommissioningMode)
  {
  case BDB_COMMISSIONING_FORMATION:
    if (bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
    {
      //After formation, perform nwk steering again plus the remaining commissioning modes that has not been process yet
      bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | bdbCommissioningModeMsg->bdbRemainingCommissioningModes);
    }
    else
    {
      //Want to try other channels?
      //try with bdb_setChannelAttribute
    }
    break;
  case BDB_COMMISSIONING_NWK_STEERING:
    if (bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
    {
      //YOUR JOB:
      //We are on the nwk, what now?
    }
    else
    {
      //See the possible errors for nwk steering procedure
      //No suitable networks found
      //Want to try other channels?
      //try with bdb_setChannelAttribute
    }
    break;
  case BDB_COMMISSIONING_FINDING_BINDING:
    if (bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_SUCCESS)
    {
      //YOUR JOB:
    }
    else
    {
      //YOUR JOB:
      //retry?, wait for user interaction?
    }
    break;
  case BDB_COMMISSIONING_INITIALIZATION:
    //Initialization notification can only be successful. Failure on initialization
    //only happens for ZED and is notified as BDB_COMMISSIONING_PARENT_LOST notification

    //YOUR JOB:
    //We are on a network, what now?

    break;
#if ZG_BUILD_ENDDEVICE_TYPE
  case BDB_COMMISSIONING_PARENT_LOST:
    if (bdbCommissioningModeMsg->bdbCommissioningStatus == BDB_COMMISSIONING_NETWORK_RESTORED)
    {
      //We did recover from losing parent
    }
    else
    {
      //Parent not found, attempt to rejoin again after a fixed delay
      osal_start_timerEx(zclGenericApp_TaskID, GENERICAPP_END_DEVICE_REJOIN_EVT, GENERICAPP_END_DEVICE_REJOIN_DELAY);
    }
    break;
#endif
  }
}

/*********************************************************************
 * @fn      zclGenericApp_ProcessIdentifyTimeChange
 *
 * @brief   Called to process any change to the IdentifyTime attribute.
 *
 * @param   endpoint - in which the identify has change
 *
 * @return  none
 */
static void zclGenericApp_ProcessIdentifyTimeChange(uint8 endpoint)
{
  (void)endpoint;

  if (zclGenericApp_IdentifyTime > 0)
  {
    HalLedBlink(HAL_LED_2, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME);
  }
  else
  {
    HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
  }
}

/*********************************************************************
 * @fn      zclGenericApp_BindNotification
 *
 * @brief   Called when a new bind is added.
 *
 * @param   data - pointer to new bind data
 *
 * @return  none
 */
static void zclGenericApp_BindNotification(bdbBindNotificationData_t *data)
{
  // GENERICAPP_TODO: process the new bind information
}

/*********************************************************************
 * @fn      zclGenericApp_ProcessTouchlinkTargetEnable
 *
 * @brief   Called to process when the touchlink target functionality
 *          is enabled or disabled
 *
 * @param   none
 *
 * @return  none
 */
#if (defined(BDB_TL_TARGET) && (BDB_TOUCHLINK_CAPABILITY_ENABLED == TRUE))
static void zclGenericApp_ProcessTouchlinkTargetEnable(uint8 enable)
{
  if (enable)
  {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
  }
  else
  {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
  }
}
#endif

/*********************************************************************
 * @fn      zclGenericApp_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to default values.
 *
 * @param   none
 *
 * @return  none
 */
static void zclGenericApp_BasicResetCB(void)
{

  /* GENERICAPP_TODO: remember to update this function with any
     application-specific cluster attribute variables */

  zclGenericApp_ResetAttributesToDefaultValues();
}
/*********************************************************************
 * @fn      zclSampleApp_BatteryWarningCB
 *
 * @brief   Called to handle battery-low situation.
 *
 * @param   voltLevel - level of severity
 *
 * @return  none
 */
void zclSampleApp_BatteryWarningCB(uint8 voltLevel)
{
  if (voltLevel == VOLT_LEVEL_CAUTIOUS)
  {
    // Send warning message to the gateway and blink LED
  }
  else if (voltLevel == VOLT_LEVEL_BAD)
  {
    // Shut down the system
  }
}

/******************************************************************************
 *
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

/*********************************************************************
 * @fn      zclGenericApp_ProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  none
 */
static void zclGenericApp_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg)
{
  switch (pInMsg->zclHdr.commandID)
  {
#ifdef ZCL_READ
  case ZCL_CMD_READ_RSP:
    zclGenericApp_ProcessInReadRspCmd(pInMsg);
    break;
#endif
#ifdef ZCL_WRITE
  case ZCL_CMD_WRITE_RSP:
    zclGenericApp_ProcessInWriteRspCmd(pInMsg);
    break;
#endif
  case ZCL_CMD_CONFIG_REPORT:
  case ZCL_CMD_CONFIG_REPORT_RSP:
  case ZCL_CMD_READ_REPORT_CFG:
  case ZCL_CMD_READ_REPORT_CFG_RSP:
  case ZCL_CMD_REPORT:
    //bdb_ProcessIncomingReportingMsg( pInMsg );
    break;

  case ZCL_CMD_DEFAULT_RSP:
    zclGenericApp_ProcessInDefaultRspCmd(pInMsg);
    break;
#ifdef ZCL_DISCOVER
  case ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP:
    zclGenericApp_ProcessInDiscCmdsRspCmd(pInMsg);
    break;

  case ZCL_CMD_DISCOVER_CMDS_GEN_RSP:
    zclGenericApp_ProcessInDiscCmdsRspCmd(pInMsg);
    break;

  case ZCL_CMD_DISCOVER_ATTRS_RSP:
    zclGenericApp_ProcessInDiscAttrsRspCmd(pInMsg);
    break;

  case ZCL_CMD_DISCOVER_ATTRS_EXT_RSP:
    zclGenericApp_ProcessInDiscAttrsExtRspCmd(pInMsg);
    break;
#endif
  default:
    break;
  }

  if (pInMsg->attrCmd)
    osal_mem_free(pInMsg->attrCmd);
}

#ifdef ZCL_READ
/*********************************************************************
 * @fn      zclGenericApp_ProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclGenericApp_ProcessInReadRspCmd(zclIncomingMsg_t *pInMsg)
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;

  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < readRspCmd->numAttr; i++)
  {
    // Notify the originator of the results of the original read attributes
    // attempt and, for each successfull request, the value of the requested
    // attribute
  }

  return (TRUE);
}
#endif // ZCL_READ

#ifdef ZCL_WRITE
/*********************************************************************
 * @fn      zclGenericApp_ProcessInWriteRspCmd
 *
 * @brief   Process the "Profile" Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclGenericApp_ProcessInWriteRspCmd(zclIncomingMsg_t *pInMsg)
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;

  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < writeRspCmd->numAttr; i++)
  {
    // Notify the device of the results of the its original write attributes
    // command.
  }

  return (TRUE);
}
#endif // ZCL_WRITE

/*********************************************************************
 * @fn      zclGenericApp_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclGenericApp_ProcessInDefaultRspCmd(zclIncomingMsg_t *pInMsg)
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  // Device is notified of the Default Response command.
  (void)pInMsg;

  return (TRUE);
}
afAddrType_t zclDIYRuZRT_DstAddr;

// Информирование о состоянии реле
void zclDIYRuZRT_ReportOnOff(void) {
  const uint8 NUM_ATTRIBUTES = 1;

  zclReportCmd_t *pReportCmd;
  uint8 RELAY_STATE = 0;

  pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) +
                              (NUM_ATTRIBUTES * sizeof(zclReport_t)));
  if (pReportCmd != NULL) {
    pReportCmd->numAttr = NUM_ATTRIBUTES;

    pReportCmd->attrList[0].attrID = ATTRID_ON_OFF;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_BOOLEAN;
    pReportCmd->attrList[0].attrData = (void *)(&RELAY_STATE);;

    zclDIYRuZRT_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
    zclDIYRuZRT_DstAddr.addr.shortAddr = 0;
    zclDIYRuZRT_DstAddr.endPoint = 1;

    zcl_SendReportCmd(zclGenericApp_SimpleDescs[0].EndPoint, &zclDIYRuZRT_DstAddr,
                      ZCL_CLUSTER_ID_GEN_ON_OFF, pReportCmd,
                      ZCL_FRAME_CLIENT_SERVER_DIR, false, SeqNum++);
  }

  osal_mem_free(pReportCmd);
}

#ifdef ZCL_DISCOVER
/*********************************************************************
 * @fn      zclGenericApp_ProcessInDiscCmdsRspCmd
 *
 * @brief   Process the Discover Commands Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclGenericApp_ProcessInDiscCmdsRspCmd(zclIncomingMsg_t *pInMsg)
{
  zclDiscoverCmdsCmdRsp_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverCmdsCmdRsp_t *)pInMsg->attrCmd;
  for (i = 0; i < discoverRspCmd->numCmd; i++)
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return (TRUE);
}

/*********************************************************************
 * @fn      zclGenericApp_ProcessInDiscAttrsRspCmd
 *
 * @brief   Process the "Profile" Discover Attributes Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclGenericApp_ProcessInDiscAttrsRspCmd(zclIncomingMsg_t *pInMsg)
{
  zclDiscoverAttrsRspCmd_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverAttrsRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < discoverRspCmd->numAttr; i++)
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return (TRUE);
}

/*********************************************************************
 * @fn      zclGenericApp_ProcessInDiscAttrsExtRspCmd
 *
 * @brief   Process the "Profile" Discover Attributes Extended Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclGenericApp_ProcessInDiscAttrsExtRspCmd(zclIncomingMsg_t *pInMsg)
{
  zclDiscoverAttrsExtRsp_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverAttrsExtRsp_t *)pInMsg->attrCmd;
  for (i = 0; i < discoverRspCmd->numAttr; i++)
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return (TRUE);
}
#endif // ZCL_DISCOVER

/****************************************************************************
****************************************************************************/
