/**************************************************************************************************
 *                                            INCLUDES
 **************************************************************************************************/
#include "hal_key.h"
#include "hal_adc.h"
#include "hal_defs.h"
#include "hal_drivers.h"
#include "hal_led.h"
#include "hal_mcu.h"
#include "hal_types.h"
#include "osal.h"
#include <stdio.h>

/**************************************************************************************************
 *                                              MACROS
 **************************************************************************************************/

/**************************************************************************************************
 *                                            CONSTANTS
 **************************************************************************************************/

#define HAL_KEY_BIT0 0x01
#define HAL_KEY_BIT1 0x02
#define HAL_KEY_BIT2 0x04
#define HAL_KEY_BIT3 0x08
#define HAL_KEY_BIT4 0x10
#define HAL_KEY_BIT5 0x20
#define HAL_KEY_BIT6 0x40
#define HAL_KEY_BIT7 0x80

#define HAL_KEY_RISING_EDGE 0
#define HAL_KEY_FALLING_EDGE 1

#define HAL_KEY_DEBOUNCE_VALUE 25 // TODO: adjust this value

#if defined(HAL_BOARD_FLOWER)
#define HAL_KEY_P0_GPIO_PINS 0x00
#define HAL_KEY_P1_GPIO_PINS 0x00
#define HAL_KEY_P2_GPIO_PINS (HAL_KEY_BIT0)

#define HAL_KEY_P0_INPUT_PINS 0x00
#define HAL_KEY_P1_INPUT_PINS 0x00
#define HAL_KEY_P2_INPUT_PINS (HAL_KEY_BIT0)

#define HAL_KEY_P0_INTERRUPT_PINS 0x00
#define HAL_KEY_P1_INTERRUPT_PINS 0x00
#define HAL_KEY_P2_INTERRUPT_PINS (HAL_KEY_BIT0)

#elif defined(HAL_BOARD_CHDTECH_DEV)
#define HAL_KEY_P0_GPIO_PINS (HAL_KEY_BIT1)
#define HAL_KEY_P1_GPIO_PINS 0x00
#define HAL_KEY_P2_GPIO_PINS (HAL_KEY_BIT0)

#define HAL_KEY_P0_INPUT_PINS (HAL_KEY_BIT1)
#define HAL_KEY_P1_INPUT_PINS 0x00
#define HAL_KEY_P2_INPUT_PINS (HAL_KEY_BIT0)

#define HAL_KEY_P0_INTERRUPT_PINS (HAL_KEY_BIT1)
#define HAL_KEY_P1_INTERRUPT_PINS 0x00
#define HAL_KEY_P2_INTERRUPT_PINS (HAL_KEY_BIT0)
#endif

/**************************************************************************************************
 *                                            TYPEDEFS
 **************************************************************************************************/

/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/
static halKeyCBack_t pHalKeyProcessFunction;
bool Hal_KeyIntEnable;

uint8 halSaveIntKey; /* used by ISR to save state of interrupt-driven keys */

static uint8 halKeyTimerRunning; // Set to true while polling timer is running in interrupt
                                 // enabled mode

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/
void halProcessKeyInterrupt(void);

void HalKeyInit(void) {
    P0SEL &= ~HAL_KEY_P0_GPIO_PINS;
    P1SEL &= ~HAL_KEY_P1_GPIO_PINS;
    P2SEL &= ~HAL_KEY_P2_GPIO_PINS;

    P0DIR &= ~(HAL_KEY_P0_INPUT_PINS);
    P1DIR &= ~(HAL_KEY_P1_INPUT_PINS);
    P2DIR &= ~(HAL_KEY_P2_INPUT_PINS);
    pHalKeyProcessFunction = NULL;

    halKeyTimerRunning = FALSE;
}

void HalKeyConfig(bool interruptEnable, halKeyCBack_t cback) {
    Hal_KeyIntEnable = true;
    pHalKeyProcessFunction = cback;
    P0IEN |= HAL_KEY_P0_INTERRUPT_PINS;
    P1IEN |= HAL_KEY_P1_INTERRUPT_PINS;
    P2IEN |= HAL_KEY_P2_INTERRUPT_PINS;

    PICTL |= HAL_KEY_BIT0 | HAL_KEY_BIT3; // set falling edge on port 0 and 2
    IEN1 |= HAL_KEY_BIT5;                 // enable port0 int
    IEN2 |= HAL_KEY_BIT1;                 // enable port2 int
}
uint8 HalKeyRead(void) {

    uint8 key = HAL_KEY_CODE_NOKEY;
#if defined(HAL_BOARD_FLOWER)

#elif defined(HAL_BOARD_CHDTECH_DEV)

    if (ACTIVE_LOW(P0 & HAL_KEY_P0_INPUT_PINS)) {
        key = 0x01;
    }

    if (ACTIVE_LOW(P2 & HAL_KEY_P2_INPUT_PINS)) {
        key = 0x02;
    }
#endif

    return key;
}
void HalKeyPoll(void) {
    uint8 keys = 0;

    keys = HalKeyRead();

    if (pHalKeyProcessFunction) {
        (pHalKeyProcessFunction)(keys, HAL_KEY_STATE_NORMAL);
    }

    if (keys != HAL_KEY_CODE_NOKEY) {
        osal_start_timerEx(Hal_TaskID, HAL_KEY_EVENT, 200);
    } else {
        halKeyTimerRunning = FALSE;
    }
}
void halProcessKeyInterrupt(void) {
    if (!halKeyTimerRunning) {
        halKeyTimerRunning = TRUE;
        osal_start_timerEx(Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_DEBOUNCE_VALUE);
    }
}

void HalKeyEnterSleep(void) {
    uint8 clkcmd = CLKCONCMD;
    uint8 clksta = CLKCONSTA;
    // Switch to 16MHz before setting the DC/DC to bypass to reduce risk of flash corruption
    CLKCONCMD = (CLKCONCMD_16MHZ | OSC_32KHZ);
    // wait till clock speed stablizes
    while (CLKCONSTA != (CLKCONCMD_16MHZ | OSC_32KHZ))
        ;

    CLKCONCMD = clkcmd;
    while (CLKCONSTA != (clksta))
        ;
}

uint8 HalKeyExitSleep(void) {
    uint8 clkcmd = CLKCONCMD;
    // Switch to 16MHz before setting the DC/DC to on to reduce risk of flash corruption
    CLKCONCMD = (CLKCONCMD_16MHZ | OSC_32KHZ);
    // wait till clock speed stablizes
    while (CLKCONSTA != (CLKCONCMD_16MHZ | OSC_32KHZ))
        ;

    CLKCONCMD = clkcmd;

    /* Wake up and read keys */
    return (HalKeyRead());
}

HAL_ISR_FUNCTION(halKeyPort0Isr, P0INT_VECTOR) {
    HAL_ENTER_ISR();

    if (P0IFG & HAL_KEY_P0_INTERRUPT_PINS) {
        halProcessKeyInterrupt();
    }

    P0IFG &= ~HAL_KEY_P0_INTERRUPT_PINS;
    P0IF = 0;

    CLEAR_SLEEP_MODE();
    HAL_EXIT_ISR();
}

HAL_ISR_FUNCTION(halKeyPort1Isr, P1INT_VECTOR) {
    HAL_ENTER_ISR();

    if (P1IFG & HAL_KEY_P1_INTERRUPT_PINS) {
        halProcessKeyInterrupt();
    }

    P1IFG &= ~HAL_KEY_P1_INTERRUPT_PINS;
    P1IF = 0;

    CLEAR_SLEEP_MODE();
    HAL_EXIT_ISR();
}

HAL_ISR_FUNCTION(halKeyPort2Isr, P2INT_VECTOR) {
    HAL_ENTER_ISR();

    if (P2IFG & HAL_KEY_P2_INTERRUPT_PINS) {
        halProcessKeyInterrupt();
    }

    P2IFG &= ~HAL_KEY_P2_INTERRUPT_PINS;
    P2IF = 0;

    CLEAR_SLEEP_MODE();
    HAL_EXIT_ISR();
}