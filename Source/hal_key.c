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

#define HAL_KEY_PDUP2 0x80
#define HAL_KEY_PDUP1 0x40
#define HAL_KEY_PDUP0 0x20

#define HAL_KEY_DEBOUNCE_VALUE 25 // TODO: adjust this value
#define HAL_KEY_POLLING_VALUE 100

#define HAL_KEY_NUM_COLUMNS 4
#if defined(HAL_BOARD_FREEPAD_20)
#define HAL_KEY_NUM_ROWS 6

/* The following define which port pins are being used by keypad service */
#define HAL_KEY_P0_GPIO_PINS (HAL_KEY_BIT2 | HAL_KEY_BIT3 | HAL_KEY_BIT4 | HAL_KEY_BIT5 | HAL_KEY_BIT6)
#define HAL_KEY_P1_GPIO_PINS (HAL_KEY_BIT2 | HAL_KEY_BIT3 | HAL_KEY_BIT4 | HAL_KEY_BIT5)

/* These defines indicate the direction of each pin */
#define HAL_KEY_P0_INPUT_PINS (HAL_KEY_BIT2 | HAL_KEY_BIT3 | HAL_KEY_BIT4 | HAL_KEY_BIT5 | HAL_KEY_BIT6)
#define HAL_KEY_P0_OUTPUT_PINS 0x00

#define HAL_KEY_P1_INPUT_PINS (HAL_KEY_BIT2 | HAL_KEY_BIT3 | HAL_KEY_BIT4 | HAL_KEY_BIT5)
#define HAL_KEY_P1_OUTPUT_PINS 0x00

/* Which pins are used for key interrupts */
#define HAL_KEY_P0_INTERRUPT_PINS 0x00
#define HAL_KEY_P1_INTERRUPT_PINS (HAL_KEY_BIT2 | HAL_KEY_BIT3 | HAL_KEY_BIT4 | HAL_KEY_BIT5)
#endif

/**************************************************************************************************
 *                                            TYPEDEFS
 **************************************************************************************************/

/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/
static uint8 halKeySavedKeys; /* used to store previous key state in polling mode */
static halKeyCBack_t pHalKeyProcessFunction;
bool Hal_KeyIntEnable; /* interrupt enable/disable flag */
uint8 halSaveIntKey;   /* used by ISR to save state of interrupt-driven keys */

static uint8 HalKeyConfigured;
static uint8 halKeyTimerRunning; // Set to true while polling timer is running in interrupt
                                 // enabled mode

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/
void halProcessKeyInterrupt(void);

/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/
/**************************************************************************************************
 * @fn      HalKeyInit
 *
 * @brief   Initilize Key Service
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalKeyInit(void) {

    /* Initialize previous key to 0 */
    halKeySavedKeys = HAL_KEY_CODE_NOKEY;

    /* Configure pin function as GPIO for pins related to keypad */
    P0SEL &= (uint8)~HAL_KEY_P0_GPIO_PINS;
    P1SEL &= (uint8)~HAL_KEY_P1_GPIO_PINS;

    /* Configure direction of pins related to keypad */
    P0DIR |= (uint8)HAL_KEY_P0_OUTPUT_PINS;
    P0DIR &= (uint8)~HAL_KEY_P0_INPUT_PINS;

    P1DIR |= (uint8)HAL_KEY_P1_OUTPUT_PINS;
    P1DIR &= (uint8)~HAL_KEY_P1_INPUT_PINS;

    // pull down port0
    P2INP |= HAL_KEY_BIT5;

    /* Initialize callback function */
    pHalKeyProcessFunction = NULL;

    /* Start with key is not configured */
    HalKeyConfigured = FALSE;

    halKeyTimerRunning = FALSE;
}

/**************************************************************************************************
 * @fn      HalKeyConfig
 *
 * @brief   Configure the Key serivce
 *
 * @param   interruptEnable - TRUE/FALSE, enable/disable interrupt
 *          cback - pointer to the CallBack function
 *
 * @return  None
 **************************************************************************************************/
void HalKeyConfig(bool interruptEnable, halKeyCBack_t cback) {

    /* Enable/Disable Interrupt */
    Hal_KeyIntEnable = interruptEnable;

    /* Register the callback fucntion */
    pHalKeyProcessFunction = cback;

    /* Determine if interrupt is enabled or not */
    if (Hal_KeyIntEnable) {
        /* The key rows are split between P0 and P1. Thus, we will
         * need to configure interrupts on both ports.
         */
        PICTL |= (HAL_KEY_BIT1 | HAL_KEY_BIT2);

        // P0IEN |= HAL_KEY_P0_INTERRUPT_PINS;
        P1IEN |= HAL_KEY_P1_INTERRUPT_PINS;

        /* Enable P0 and P1 interrupts */
        IEN1 |= HAL_KEY_BIT5;
        IEN2 |= HAL_KEY_BIT4;

        /* Do this only after the hal_key is configured - to work with sleep stuff */
        if (HalKeyConfigured == TRUE) {
            osal_stop_timerEx(Hal_TaskID, HAL_KEY_EVENT); /* Cancel polling if active */
        }
    } else /* Interrupts NOT enabled */
    {
        // disable interrupt
        // P0IEN &= ~(HAL_KEY_P0_INTERRUPT_PINS);
        P1IEN &= ~(HAL_KEY_P1_INTERRUPT_PINS);
        IEN1 &= ~(HAL_KEY_BIT5);
        IEN2 &= ~(HAL_KEY_BIT4);

        osal_start_timerEx(Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_POLLING_VALUE); /* Kick off polling */
    }

    /* Key now is configured */
    HalKeyConfigured = TRUE;
}

/**************************************************************************************************
 * @fn      HalKeyRead
 *
 * @brief   Read the current value of a key
 *
 * @param   None
 *
 * @return  keys - current keys status
 **************************************************************************************************/
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
    P2INP |= HAL_KEY_BIT6;  // pull down port 1

    for (int i = 0; i < 255; i++) {
        asm("NOP");
    }

    for (int i = 0; i < 8; i++) {
        if ((HAL_KEY_P0_INPUT_PINS >> i) & 1 && ((P0 >> i) & 0x01) == 0) {
            rowcode = i;
            break;
        }
    }
    // printf("rowcode %d colcode %d\n", rowcode, colcode);

    // change KPb pins back

    P2INP |= HAL_KEY_BIT5;  // pull up port 1
    P2INP &= ~HAL_KEY_BIT6; // pull down port 0

    P1IFG = (uint8)(~HAL_KEY_P1_INTERRUPT_PINS);
    P0IF = 0;
    P1IF = 0;
    P1IEN |= HAL_KEY_P1_INTERRUPT_PINS;

    if (colcode == HAL_KEY_CODE_NOKEY || rowcode == HAL_KEY_CODE_NOKEY) {
        return HAL_KEY_CODE_NOKEY; // no key pressed
    } else {
        return (rowcode << 4) | colcode;
    }
}

/**************************************************************************************************
 * @fn      HalKeyPoll
 *
 * @brief   Called by hal_driver to poll the keys
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalKeyPoll(void) {

    uint8 keys = 0;

    /*
     *  If interrupts are enabled, get the status of the interrupt-driven keys from 'halSaveIntKey'
     *  which is updated by the key ISR.  If Polling, read these keys directly.
     */
    keys = HalKeyRead();

    /* Exit if polling and no keys have changed */
    if (!Hal_KeyIntEnable) {
        if (keys == halKeySavedKeys) {
            return;
        }
        halKeySavedKeys = keys; /* Store the current keys for comparation next time */
    }

    /* Invoke Callback if new keys were depressed */
    if ((keys != HAL_KEY_CODE_NOKEY || Hal_KeyIntEnable) && (pHalKeyProcessFunction)) {
        // When interrupt is enabled, send HAL_KEY_CODE_NOKEY as well so that
        // application would know the previous key is no longer depressed.
        (pHalKeyProcessFunction)(keys, HAL_KEY_STATE_NORMAL);
    }

    if (Hal_KeyIntEnable) {
        if (keys != HAL_KEY_CODE_NOKEY) {
            // In order to trigger callback again as far as the key is depressed,
            // timer is called here.
            osal_start_timerEx(Hal_TaskID, HAL_KEY_EVENT, 50);
        } else {
            halKeyTimerRunning = FALSE;
        }
    }
}

/**************************************************************************************************
 * @fn      halProcessKeyInterrupt
 *
 * @brief   Checks to see if it's a valid key interrupt, saves interrupt driven key states for
 *          processing by HalKeyRead(), and debounces keys by scheduling HalKeyRead() 25ms later.
 *
 * @param
 *
 * @return
 **************************************************************************************************/
void halProcessKeyInterrupt(void) {

    if ((P1IFG & HAL_KEY_P1_INTERRUPT_PINS)) { //(P0IFG & HAL_KEY_P0_INTERRUPT_PINS) ||
        P1IEN &= (uint8)~HAL_KEY_P1_INTERRUPT_PINS;
        // interrupt flag has been set
        // P0IFG = (uint8)(~HAL_KEY_P0_INTERRUPT_PINS); // clear interrupt flag
        P1IFG = (uint8)(~HAL_KEY_P1_INTERRUPT_PINS);
        if (!halKeyTimerRunning) {
            halKeyTimerRunning = TRUE;
            osal_start_timerEx(Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_DEBOUNCE_VALUE);
        }
        P1IEN |= HAL_KEY_P1_INTERRUPT_PINS;
    }
}

/**************************************************************************************************
 * @fn      HalKeyEnterSleep
 *
 * @brief  - Get called to enter sleep mode
 *
 * @param
 *
 * @return
 **************************************************************************************************/
void HalKeyEnterSleep(void) {
    /* Sleep!!!
     * Nothing to do.
     */
}

/**************************************************************************************************
 * @fn      HalKeyExitSleep
 *
 * @brief   - Get called when sleep is over
 *
 * @param
 *
 * @return  - return saved keys
 **************************************************************************************************/
uint8 HalKeyExitSleep(void) {
    /* Wakeup!!!
     * Nothing to do. In fact. HalKeyRead() may not be called here.
     * Calling HalKeyRead() will trigger key scanning and interrupt flag clearing in the end,
     * which is no longer compatible with hal_sleep.c module.
     */
    /* Wake up and read keys */
    return TRUE;
}

/***************************************************************************************************
 *                                    INTERRUPT SERVICE ROUTINES
 ***************************************************************************************************/

/**************************************************************************************************
 * @fn      halKeyPort0Isr
 *
 * @brief   Port0 ISR
 *
 * @param
 *
 * @return
 **************************************************************************************************/
// HAL_ISR_FUNCTION(halKeyPort0Isr, P0INT_VECTOR) {
//     HAL_ENTER_ISR();

//     halProcessKeyInterrupt();

//     P0IFG = (uint8)(~HAL_KEY_P0_INTERRUPT_PINS);
//     P0IF = 0;

//     CLEAR_SLEEP_MODE();
//     HAL_EXIT_ISR();
// }

/**************************************************************************************************
 * @fn      halKeyPort1Isr
 *
 * @brief   Port1 ISR
 *
 * @param
 *
 * @return
 **************************************************************************************************/
HAL_ISR_FUNCTION(halKeyPort1Isr, P1INT_VECTOR) {
    HAL_ENTER_ISR();

    halProcessKeyInterrupt();

    P1IFG = (uint8)(~HAL_KEY_P1_INTERRUPT_PINS);
    P1IF = 0;

    CLEAR_SLEEP_MODE();
    HAL_EXIT_ISR();
}

/**************************************************************************************************
**************************************************************************************************/
