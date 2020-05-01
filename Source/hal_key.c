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

#if defined(HAL_BOARD_FREEPAD_20)
#define HAL_KEY_P0_GPIO_PINS (HAL_KEY_BIT2 | HAL_KEY_BIT3 | HAL_KEY_BIT4 | HAL_KEY_BIT5 | HAL_KEY_BIT6)
#define HAL_KEY_P1_GPIO_PINS (HAL_KEY_BIT2 | HAL_KEY_BIT3 | HAL_KEY_BIT4 | HAL_KEY_BIT5)

#define HAL_KEY_P0_INPUT_PINS (HAL_KEY_BIT2 | HAL_KEY_BIT3 | HAL_KEY_BIT4 | HAL_KEY_BIT5 | HAL_KEY_BIT6)
#define HAL_KEY_P1_INPUT_PINS (HAL_KEY_BIT2 | HAL_KEY_BIT3 | HAL_KEY_BIT4 | HAL_KEY_BIT5)

#define HAL_KEY_P1_INTERRUPT_PINS (HAL_KEY_BIT2 | HAL_KEY_BIT3 | HAL_KEY_BIT4 | HAL_KEY_BIT5)
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

static uint8 HalKeyConfigured;
static uint8 halKeyTimerRunning; // Set to true while polling timer is running in interrupt
                                 // enabled mode

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/
void halProcessKeyInterrupt(void);

void HalKeyInit(void) {
    P0SEL &= (uint8)~HAL_KEY_P0_GPIO_PINS;
    P1SEL &= (uint8)~HAL_KEY_P1_GPIO_PINS;

    P0DIR &= (uint8)~HAL_KEY_P0_INPUT_PINS;
    P1DIR &= (uint8)~HAL_KEY_P1_INPUT_PINS;

    // pull down port0
    P2INP |= HAL_KEY_BIT5;
    HAL_BOARD_DELAY_USEC(30);

    pHalKeyProcessFunction = NULL;
    HalKeyConfigured = FALSE;
    halKeyTimerRunning = FALSE;
}

void HalKeyConfig(bool interruptEnable, halKeyCBack_t cback) {
    Hal_KeyIntEnable = true;

    pHalKeyProcessFunction = cback;

    PICTL |= (HAL_KEY_BIT1 | HAL_KEY_BIT2);
    P1IEN |= HAL_KEY_P1_INTERRUPT_PINS;
    IEN2 |= HAL_KEY_BIT4;

    if (HalKeyConfigured == TRUE) {
        osal_stop_timerEx(Hal_TaskID, HAL_KEY_EVENT); /* Cancel polling if active */
    }
    HalKeyConfigured = TRUE;
}
uint8 HalKeyRead(void) {

    uint8 colcode = HAL_KEY_CODE_NOKEY, rowcode = HAL_KEY_CODE_NOKEY;
    P1IEN &= ~(HAL_KEY_P1_INTERRUPT_PINS);

    for (int i = 0; i < 8; i++) {
        if ((HAL_KEY_P1_INPUT_PINS >> i) & 1 && ((P1 >> i) & 0x01) == 0) {
            colcode = i;
            break;
        }
    }

    P2INP &= ~HAL_KEY_BIT5; // pull up port 0
    // P2INP |= HAL_KEY_BIT6;  // pull down port 1

    HAL_BOARD_DELAY_USEC(50);

    for (int i = 0; i < 8; i++) {
        if ((HAL_KEY_P0_INPUT_PINS >> i) & 1 && ((P0 >> i) & 0x01) == 0) {
            rowcode = i;
            break;
        }
    }

    P2INP |= HAL_KEY_BIT5;  // pull down port 1
    P2INP &= ~HAL_KEY_BIT6; // pull up port 0

    HAL_BOARD_DELAY_USEC(50);

    P1IFG = (uint8)(~HAL_KEY_P1_INTERRUPT_PINS);
    P1IF = 0;
    P1IEN |= HAL_KEY_P1_INTERRUPT_PINS;

    if (colcode == HAL_KEY_CODE_NOKEY || rowcode == HAL_KEY_CODE_NOKEY) {
        return HAL_KEY_CODE_NOKEY;
    } else {
        return (rowcode << 4) | colcode;
    }
}
void HalKeyPoll(void) {

    uint8 keys = HalKeyRead();

    if (pHalKeyProcessFunction) {
        (pHalKeyProcessFunction)(keys, HAL_KEY_STATE_NORMAL);
    }

    if (keys != HAL_KEY_CODE_NOKEY) {
        osal_start_timerEx(Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_DEBOUNCE_VALUE);
    } else {
        halKeyTimerRunning = FALSE;
    }
}
void halProcessKeyInterrupt(void) {
    if ((P1IFG & HAL_KEY_P1_INTERRUPT_PINS)) {
        P1IEN &= (uint8)~HAL_KEY_P1_INTERRUPT_PINS;
        P1IFG = (uint8)(~HAL_KEY_P1_INTERRUPT_PINS);
        if (!halKeyTimerRunning) {
            halKeyTimerRunning = TRUE;
            osal_start_timerEx(Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_DEBOUNCE_VALUE);
        }
        P1IEN |= HAL_KEY_P1_INTERRUPT_PINS;
    }
}

void HalKeyEnterSleep(void) {}

uint8 HalKeyExitSleep(void) {
    /* Wakeup!!!
     * Nothing to do. In fact. HalKeyRead() may not be called here.
     * Calling HalKeyRead() will trigger key scanning and interrupt flag clearing in the end,
     * which is no longer compatible with hal_sleep.c module.
     */
    /* Wake up and read keys */
    return TRUE;
}

HAL_ISR_FUNCTION(halKeyPort1Isr, P1INT_VECTOR) {
    HAL_ENTER_ISR();

    halProcessKeyInterrupt();

    P1IFG = (uint8)(~HAL_KEY_P1_INTERRUPT_PINS);
    P1IF = 0;

    CLEAR_SLEEP_MODE();
    HAL_EXIT_ISR();
}