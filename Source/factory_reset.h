#ifndef FACTORY_RESET_H
#define FACTORY_RESET_H

#define FACTORY_RESET_EVT 0x1000
#define FACTORY_BOOTCOUNTER_RESET_EVT 0x2000

#define FACTORY_RESET_HOLD_TIME_LONG ((uint32) 10 * 1000)
#define FACTORY_RESET_HOLD_TIME_FAST 1000

#define FACTORY_RESET_BOOTCOUNTER_MAX_VALUE 5
#define FACTORY_RESET_BOOTCOUNTER_RESET_TIME 10*1000


extern void zclFactoryResetter_Init(uint8 task_id);
extern uint16 zclFactoryResetter_loop(uint8 task_id, uint16 events);
extern void zclFactoryResetter_HandleKeys(uint8 shift, uint8 keyCode);
#endif