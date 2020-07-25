#define TC_LINKKEY_JOIN
#define NV_INIT
#define NV_RESTORE


#define TP2_LEGACY_ZC
//patch sdk
// #define ZDSECMGR_TC_ATTEMPT_DEFAULT_KEY TRUE

#define NWK_AUTO_POLL
#define MULTICAST_ENABLED FALSE

#define ZCL_READ
#define ZCL_BASIC
#define ZCL_IDENTIFY
#define ZCL_REPORTING_DEVICE

#define ZSTACK_DEVICE_BUILD (DEVICE_BUILD_ENDDEVICE)

#define DISABLE_GREENPOWER_BASIC_PROXY
#define BDB_FINDING_BINDING_CAPABILITY_ENABLED 1
#define BDB_REPORTING TRUE


#define ISR_KEYINTERRUPT
#define HAL_BUZZER FALSE

#define HAL_LED TRUE
#define HAL_I2C TRUE
#define BLINK_LEDS TRUE


//one of this boards
// #define HAL_BOARD_FLOWER
// #define HAL_BOARD_CHDTECH_DEV

#if !defined(HAL_BOARD_FLOWER) && !defined(HAL_BOARD_CHDTECH_DEV)
#error "Board type must be defined"
#endif

#define BDB_MAX_CLUSTERENDPOINTS_REPORTING 10

#define BME280_32BIT_ENABLE


#define TSENS_SBIT P1_3
#define TSENS_BV BV(3)
#define TSENS_DIR P1DIR

#define SOIL_MOISTURE_PORT 0
#define SOIL_MOISTURE_PIN 4

#define LUMOISITY_PORT 0
#define LUMOISITY_PIN 7



#if defined(HAL_BOARD_FLOWER)
    // #define INT_HEAP_LEN (2688-0xC4-0x15-0x44-0x20-0x1E)
    // #define HAL_UART TRUE
    // #define HAL_UART_DMA 1
    #define HAL_UART FALSE
    #define POWER_SAVING
#elif defined(HAL_BOARD_CHDTECH_DEV)
    #define INT_HEAP_LEN (2688-0xC4-0x15-0x44-0x20-0x1E)
    #define HAL_UART TRUE
    #define HAL_UART_DMA 1
#endif


#include "hal_board_cfg.h"

#include "stdint.h"
#include "stddef.h"
