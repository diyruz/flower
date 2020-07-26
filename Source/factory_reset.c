#include "factory_reset.h"
#include "OnBoard.h"
#include "AF.h"
#include "Debug.h"
#include "bdb.h"
#include "bdb_interface.h"
#include "hal_led.h"

static void zclFactoryResetter_ResetToFN(void);


static uint8 zclFactoryResetter_TaskID;

uint16 zclFactoryResetter_loop(uint8 task_id, uint16 events) {
    if (events & FACTORY_RESET_EVT) {
        LREPMaster("FACTORY_RESET_EVT\r\n");
        zclFactoryResetter_ResetToFN();
        return (events ^ FACTORY_RESET_EVT);
    }
    return 0;
}

void zclFactoryResetter_Init(uint8 task_id) {
    zclFactoryResetter_TaskID = task_id;
    /**
     * We can't register more than one task, call in main app taks
     * zclFactoryResetter_HandleKeys(shift, keyCode);
     * */
    // RegisterForKeys(task_id);
    zcl_registerForMsg(task_id);
}


void zclFactoryResetter_ResetToFN(void) {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
    LREPMaster("zclFactoryResetter: Reset to FN\r\n");
    bdb_resetLocalAction();
}



void zclFactoryResetter_HandleKeys(uint8 shift, uint8 keyCode) {
    if (keyCode == HAL_KEY_CODE_NOKEY) {
        LREPMaster("zclFactoryResetter: Key release\r\n");
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT);
    } else {
        LREPMaster("zclFactoryResetter: Key press\r\n");
        osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT, bdbAttributes.bdbNodeIsOnANetwork ? FACTORY_RESET_HOLD_TIME_LONG : FACTORY_RESET_HOLD_TIME_FAST);
    }
}