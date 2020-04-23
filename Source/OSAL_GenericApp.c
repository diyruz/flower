/**************************************************************************************************
  Filename:       OSAL_GenericApp.c
  Revised:        $Date: 2008-02-07 12:10:00 -0800 (Thu, 07 Feb 2008) $
  Revision:       $Revision: 16360 $

  Description:    This file contains all the settings and other functions
                  that the user should set and change.


  Copyright 2006-2007 Texas Instruments Incorporated. All rights reserved.

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
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
 * INCLUDES
 */

#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "ZComDef.h"
#include "hal_drivers.h"


#if defined(MT_TASK)
#include "MT.h"
#include "MT_TASK.h"
#endif

#include "APS.h"
#include "ZDApp.h"
#include "nwk.h"


#include "bdb_interface.h"
#if !defined(DISABLE_GREENPOWER_BASIC_PROXY) && (ZG_BUILD_RTR_TYPE)
#include "gp_common.h"
#endif

#if defined(ZIGBEE_FREQ_AGILITY) || defined(ZIGBEE_PANID_CONFLICT)
#include "ZDNwkMgr.h"
#endif
#if defined(ZIGBEE_FRAGMENTATION)
#include "aps_frag.h"
#endif

#if defined(INTER_PAN)
#include "stub_aps.h"
#if defined(BDB_TL_INITIATOR)
#include "bdb_touchlink_initiator.h"
#endif // BDB_TL_INITIATOR
#if defined(BDB_TL_TARGET)
#include "bdb_touchlink_target.h"
#endif // BDB_TL_TARGET
#endif // INTER_PAN

#include "zcl_genericapp.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */

// The order in this table must be identical to the task initialization calls
// below in osalInitTask.
const pTaskEventHandlerFn tasksArr[] = {macEventLoop,
                                        nwk_event_loop,
#if !defined(DISABLE_GREENPOWER_BASIC_PROXY) && (ZG_BUILD_RTR_TYPE)
                                        gp_event_loop,
#endif
                                        Hal_ProcessEvent,
#if defined(MT_TASK)
                                        MT_ProcessEvent,
#endif
                                        APS_event_loop,
#if defined(ZIGBEE_FRAGMENTATION)
                                        APSF_ProcessEvent,
#endif
                                        ZDApp_event_loop,
#if defined(ZIGBEE_FREQ_AGILITY) || defined(ZIGBEE_PANID_CONFLICT)
                                        ZDNwkMgr_event_loop,
#endif
// Added to include TouchLink functionality
#if defined(INTER_PAN)
                                        StubAPS_ProcessEvent,
#endif
// Added to include TouchLink initiator functionality
#if defined(BDB_TL_INITIATOR)
                                        touchLinkInitiator_event_loop,
#endif
// Added to include TouchLink target functionality
#if defined(BDB_TL_TARGET)
                                        touchLinkTarget_event_loop,
#endif
                                        zcl_event_loop,
                                        bdb_event_loop,
                                        zclGenericApp_event_loop};

const uint8 tasksCnt = sizeof(tasksArr) / sizeof(tasksArr[0]);
uint16 *tasksEvents;

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      osalInitTasks
 *
 * @brief   This function invokes the initialization function for each task.
 *
 * @param   void
 *
 * @return  none
 */
void osalInitTasks(void) {
  uint8 taskID = 0;

  tasksEvents = (uint16 *)osal_mem_alloc(sizeof(uint16) * tasksCnt);
  osal_memset(tasksEvents, 0, (sizeof(uint16) * tasksCnt));

  macTaskInit(taskID++);
  nwk_init(taskID++);
#if !defined(DISABLE_GREENPOWER_BASIC_PROXY) && (ZG_BUILD_RTR_TYPE)
  gp_Init(taskID++);
#endif
  Hal_Init(taskID++);
#if defined(MT_TASK)
  MT_TaskInit(taskID++);
#endif
  APS_Init(taskID++);
#if defined(ZIGBEE_FRAGMENTATION)
  APSF_Init(taskID++);
#endif
  ZDApp_Init(taskID++);
#if defined(ZIGBEE_FREQ_AGILITY) || defined(ZIGBEE_PANID_CONFLICT)
  ZDNwkMgr_Init(taskID++);
#endif
  // Added to include TouchLink functionality
#if defined(INTER_PAN)
  StubAPS_Init(taskID++);
#endif
// Added to include TouchLink initiator functionality
#if defined(BDB_TL_INITIATOR)
  touchLinkInitiator_Init(taskID++);
#endif
// Added to include TouchLink target functionality
#if defined(BDB_TL_TARGET)
  touchLinkTarget_Init(taskID++);
#endif
  zcl_Init(taskID++);
  bdb_Init(taskID++);
  zclGenericApp_Init(taskID);
}

/*********************************************************************
*********************************************************************/
