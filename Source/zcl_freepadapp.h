#ifndef ZCL_FREEPADAPP_H
#define ZCL_FREEPADAPP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include "version.h"
#include "zcl.h"


/*********************************************************************
 * CONSTANTS
 */

#define FREEPAD_NW_SWITCH_ACTIONS 0x1000
#define FREEPAD_NW_SWITCH_TYPES 0x2000

#define HAL_UNKNOWN_BUTTON HAL_KEY_CODE_NOKEY
// Application Events

#define FREEPADAPP_CONST_ONE_MINUTE_IN_MS ((uint32) 60 * (uint32) 1000)

#define FREEPADAPP_END_DEVICE_REJOIN_EVT 0x0001
#define FREEPADAPP_SEND_KEYS_EVT 0x0002
#define FREEPADAPP_RESET_EVT 0x0004
#define FREEPADAPP_REPORT_EVT 0x0008
#define FREEPADAPP_HOLD_START_EVT 0x0010
#define FREEPADAPP_SAVE_ATTRS_EVT 0x0020


#define FREEPADAPP_SEND_KEYS_DELAY 250
#define FREEPADAPP_HOLD_START_DELAY (FREEPADAPP_SEND_KEYS_DELAY + 750)
#define FREEPADAPP_RESET_DELAY 10 * 1000

#define FREEPADAPP_END_DEVICE_REJOIN_MAX_DELAY ((uint32)1800000) // 30 minutes 30 * 60 * 1000
#define FREEPADAPP_END_DEVICE_REJOIN_START_DELAY 10 * 1000 // 10 seconds
#define FREEPADAPP_END_DEVICE_REJOIN_BACKOFF ((float) 1.2)
#define FREEPADAPP_END_DEVICE_REJOIN_TRIES 20


#define FREEPAD_ATTRS_COUNT 2

#define FREEPADAPP_REPORT_DELAY ((uint32)1800000) // 30 minutes 30 * 60 * 1000
#define FREEPADAPP_LEVEL_STEP_SIZE 255 >> 2
#define FREEPADAPP_LEVEL_TRANSITION_TIME 10

#define FREEPADAPP_COLOR_LEVEL_STEP_X_SIZE 1000
#define FREEPADAPP_COLOR_LEVEL_STEP_Y_SIZE 1000
#define FREEPADAPP_COLOR_LEVEL_TRANSITION_TIME 0


/*********************************************************************
 * MACROS
 */
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */
extern uint8 zclFreePadApp_SwitchActions[];
extern uint8 zclFreePadApp_SwitchTypes[];


extern SimpleDescriptionFormat_t zclFreePadApp_SimpleDescs[];
extern uint8 zclFreePadApp_SimpleDescsCount;
extern uint8 zclFreePadApp_BatteryVoltage;
extern uint8 zclFreePadApp_BatteryPercentageRemainig;
extern CONST zclCommandRec_t zclFreePadApp_Cmds[];

extern CONST uint8 zclCmdsArraySize;

// attribute list
extern CONST zclAttrRec_t zclFreePadApp_Attrs[][FREEPAD_ATTRS_COUNT];
extern CONST zclAttrRec_t zclFreePadApp_AttrsFirstEP[];
extern CONST uint8 zclFreePadApp_AttrsFirstEPCount;

extern const uint8 zclFreePadApp_ManufacturerName[];
extern const uint8 zclFreePadApp_ModelId[];
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


extern void zclFreePadApp_ResetAttributesToDefaultValues(void);

#ifdef __cplusplus
}
#endif

#endif /* ZCL_FREEPADAPP_H */
