/**************************************************************************************************
  Filename:       zcl_FREEPADAPP.h
  Revised:        $Date: 2014-06-19 08:38:22 -0700 (Thu, 19 Jun 2014) $
  Revision:       $Revision: 39101 $

  Description:    This file contains the ZigBee Cluster Library Home
                  Automation Sample Application.


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

#ifndef ZCL_FREEPADAPP_H
#define ZCL_FREEPADAPP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include "zcl.h"
#include "version.h"

/*********************************************************************
 * CONSTANTS
 */

#ifdef HAL_BOARD_FREEPAD_20
#define FREEPAD_BUTTONS_COUNT 20
#elif defined(HAL_BOARD_FREEPAD_12)
#define FREEPAD_BUTTONS_COUNT 12
#elif defined(HAL_BOARD_FREEPAD_8)
#define FREEPAD_BUTTONS_COUNT 8
#elif defined(HAL_BOARD_CHDTECH_DEV)
#define FREEPAD_BUTTONS_COUNT 2
#endif

#define HAL_UNKNOWN_BUTTON HAL_KEY_CODE_NOKEY
// Application Events

#define FREEPADAPP_AWAKE_TIMEOUT 1000 * 60 // 60 seconds

#define FREEPADAPP_END_DEVICE_REJOIN_EVT 0x0001
#define FREEPADAPP_SEND_KEYS_EVT 0x0002
#define FREEPADAPP_RESET_EVT 0x0004
#define FREEPADAPP_REPORT_EVT 0x0010
#define FREEPADAPP_HOLD_START_EVT 0x0020

#define FREEPADAPP_SEND_KEYS_DELAY 250
#define FREEPADAPP_HOLD_START_DELAY (FREEPADAPP_SEND_KEYS_DELAY + 750)
#define FREEPADAPP_RESET_DELAY 10 * 1000
#define FREEPADAPP_END_DEVICE_REJOIN_DELAY 10 * 1000 // 10 seconds
#define FREEPADAPP_REPORT_DELAY 30 * 60 * 1000            // 30 minutes

#define FREEPADAPP_LEVEL_STEP_SIZE 255 >> 2
#define FREEPADAPP_LEVEL_TRANSITION_TIME 10

/*********************************************************************
 * MACROS
 */
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */

extern SimpleDescriptionFormat_t zclFreePadApp_SimpleDescs[];
extern uint8 zclFreePadApp_SimpleDescsCount;
extern uint8 zclFreePadApp_BatteryVoltage;
extern uint8 zclFreePadApp_BatteryPercentageRemainig;
extern CONST zclCommandRec_t zclFreePadApp_Cmds[];

extern CONST uint8 zclCmdsArraySize;

// attribute list
extern CONST zclAttrRec_t zclFreePadApp_Attrs[];
extern CONST uint8 zclFreePadApp_NumAttributes;
extern const uint8 zclFreePadApp_ManufacturerName[];
extern const uint8 zclFreePadApp_ManufacturerNameNT[];
extern const uint8 zclFreePadApp_ModelId[];
extern const uint8 zclFreePadApp_ModelIdNT[];
extern const uint8 zclFreePadApp_PowerSource;

// FREEPADAPP_TODO: Declare application specific attributes here

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialization for the task
 */
extern void zclFreePadApp_Init(byte task_id);
extern void zclFreePadApp_InitClusters(void);
extern byte zclFreePadApp_KeyCodeToButton(byte key);

/*
 *  Event Process for the task
 */
extern UINT16 zclFreePadApp_event_loop(byte task_id, UINT16 events);

/*********************************************************************
 *********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZCL_FREEPADAPP_H */
