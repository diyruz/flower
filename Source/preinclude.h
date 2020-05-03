#define SECURE 1
#define TC_LINKKEY_JOIN
#define NV_INIT
#define NV_RESTORE

#define POWER_SAVING
#define NWK_AUTO_POLL

#define MULTICAST_ENABLED FALSE
#define ZCL_READ

#define ZCL_BASIC
#define ZCL_IDENTIFY
#define ZCL_ON_OFF
#define ZCL_LEVEL_CTRL

#define ZCL_REPORTING_DEVICE

#define DISABLE_GREENPOWER_BASIC_PROXY
#define BDB_FINDING_BINDING_CAPABILITY_ENABLED 1
#define BDB_REPORTING TRUE

#ifdef DEFAULT_CHANLIST
#undef DEFAULT_CHANLIST
#define DEFAULT_CHANLIST 0x07FFF800
#else
#define DEFAULT_CHANLIST 0x07FFF800
#endif

/* ----------- Minimum safe bus voltage ---------- */

// Vdd/3 / Internal Reference X ENOB --> (Vdd / 3) / 1.15 X 127
#define VDD_2_0  74   // 2.0 V required to safely read/write internal flash.
#define VDD_2_7  100  // 2.7 V required for the Numonyx device.

#define VDD_MIN_NV   (VDD_2_0+4)  // 5% margin over minimum to survive a page erase and compaction.
#define VDD_MIN_GOOD (VDD_2_0+8)  // 10% margin over minimum to survive a page erase and compaction.
#define VDD_MIN_XNV  (VDD_2_7+5)  // 5% margin over minimum to survive a page erase and compaction.

#define ISR_KEYINTERRUPT
#define HAL_BUZZER FALSE

#define HAL_LED TRUE
#define BLINK_LEDS TRUE

// #ifdef NWK_MAX_BINDING_ENTRIES
//     #undef NWK_MAX_BINDING_ENTRIES
// #endif
// #define NWK_MAX_BINDING_ENTRIES 20

//one of this boards
// #define HAL_BOARD_FREEPAD_20
// #define HAL_BOARD_FREEPAD_12
// #define HAL_BOARD_FREEPAD_8
#define HAL_BOARD_CHDTECH_DEV



#if defined(HAL_BOARD_FREEPAD_20) || defined(HAL_BOARD_FREEPAD_12) || defined(HAL_BOARD_FREEPAD_8)
    #define HAL_BOARD_FREEPAD
    #define HAL_UART FALSE
#elif defined(HAL_BOARD_CHDTECH_DEV)
    #define HAL_UART TRUE
    #define HAL_UART_ISR 1
    #define HAL_UART_DMA 2
#endif


#include "hal_board_cfg.h"
