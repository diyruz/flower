#ifndef FACTORY_RESET_H
#define FACTORY_RESET_H

#define FACTORY_RESET_EVT 0x1000
#define FACTORY_BOOTCOUNTER_RESET_EVT 0x2000

#ifndef FACTORY_RESET_HOLD_TIME_LONG
    #define FACTORY_RESET_HOLD_TIME_LONG ((uint32)10 * 1000)
#endif

#ifndef FACTORY_RESET_HOLD_TIME_FAST
    #define FACTORY_RESET_HOLD_TIME_FAST 1000
#endif

#ifndef FACTORY_RESET_BOOTCOUNTER_MAX_VALUE
    #define FACTORY_RESET_BOOTCOUNTER_MAX_VALUE 5
#endif

#ifndef FACTORY_RESET_BOOTCOUNTER_RESET_TIME
    #define FACTORY_RESET_BOOTCOUNTER_RESET_TIME 10 * 1000
#endif

#ifndef FACTORY_RESET_BY_LONG_PRESS
    #define FACTORY_RESET_BY_LONG_PRESS TRUE
#endif

#ifndef FACTORY_RESET_BY_BOOT_COUNTER
    #define FACTORY_RESET_BY_BOOT_COUNTER TRUE
#endif

extern void zclFactoryResetter_Init(uint8 task_id);
extern uint16 zclFactoryResetter_loop(uint8 task_id, uint16 events);
extern void zclFactoryResetter_HandleKeys(uint8 shift, uint8 keyCode);
#endif