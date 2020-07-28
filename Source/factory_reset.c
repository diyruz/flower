#include "factory_reset.h"
#include "AF.h"
#include "Debug.h"
#include "OnBoard.h"
#include "bdb.h"
#include "bdb_interface.h"
#include "hal_led.h"
#include "ZComDef.h"

static void zclFactoryResetter_ResetToFN(void);
static void zclFactoryResetter_ProcessBootCounter(void);
static void zclFactoryResetter_ResetBootCounter(void);

static uint8 zclFactoryResetter_TaskID;

uint16 zclFactoryResetter_loop(uint8 task_id, uint16 events) {
    LREP("zclFactoryResetter_loop 0x%X\r\n", events);
    if (events & FACTORY_RESET_EVT) {
        LREPMaster("FACTORY_RESET_EVT\r\n");
        zclFactoryResetter_ResetToFN();
        return (events ^ FACTORY_RESET_EVT);
    }

    if (events & FACTORY_BOOTCOUNTER_RESET_EVT) {
        LREPMaster("FACTORY_BOOTCOUNTER_RESET_EVT\r\n");
        zclFactoryResetter_ResetBootCounter();
        return (events ^ FACTORY_BOOTCOUNTER_RESET_EVT);
    }
    return 0;
}
void zclFactoryResetter_ResetBootCounter(void) {
    uint16 bootCnt = 0;
    LREPMaster("Clear boot counter\r\n");
    osal_nv_write(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
}

void zclFactoryResetter_Init(uint8 task_id) {
    zclFactoryResetter_TaskID = task_id;
    /**
     * We can't register more than one task, call in main app taks
     * zclFactoryResetter_HandleKeys(shift, keyCode);
     * */
    // RegisterForKeys(task_id);
#if FACTORY_RESET_BY_BOOT_COUNTER
    zclFactoryResetter_ProcessBootCounter();
#endif
}

void zclFactoryResetter_ResetToFN(void) {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
    LREP("bdbAttributes.bdbNodeIsOnANetwork=%d bdbAttributes.bdbCommissioningMode=0x%X\r\n", bdbAttributes.bdbNodeIsOnANetwork, bdbAttributes.bdbCommissioningMode);
    LREPMaster("zclFactoryResetter: Reset to FN\r\n");
    bdb_resetLocalAction();
}

void zclFactoryResetter_HandleKeys(uint8 shift, uint8 keyCode) {
#if FACTORY_RESET_BY_LONG_PRESS
    if (keyCode == HAL_KEY_CODE_NOKEY) {
        LREPMaster("zclFactoryResetter: Key release\r\n");
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT);
    } else {
        LREPMaster("zclFactoryResetter: Key press\r\n");
        osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT,
                           bdbAttributes.bdbNodeIsOnANetwork ? FACTORY_RESET_HOLD_TIME_LONG : FACTORY_RESET_HOLD_TIME_FAST);
    }
#endif
}

void zclFactoryResetter_ProcessBootCounter(void) {
    LREPMaster("zclFactoryResetter_ProcessBootCounter\r\n");
    osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_BOOTCOUNTER_RESET_EVT, FACTORY_RESET_BOOTCOUNTER_RESET_TIME);

    uint16 bootCnt = 0;
    if (osal_nv_item_init(ZCD_NV_BOOTCOUNTER, sizeof(bootCnt), &bootCnt) == ZSUCCESS) {
        osal_nv_read(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
    }
    LREP("bootCnt %d\r\n", bootCnt);
    bootCnt += 1;
    if (bootCnt >= FACTORY_RESET_BOOTCOUNTER_MAX_VALUE) {
        LREP("bootCnt =%d greater than, ressetting %d\r\n", bootCnt, FACTORY_RESET_BOOTCOUNTER_MAX_VALUE);
        bootCnt = 0;
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_BOOTCOUNTER_RESET_EVT);
        osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT, 5000);
    }
    osal_nv_write(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
}