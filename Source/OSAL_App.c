#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "ZComDef.h"
#include "hal_drivers.h"

#include "APS.h"
#include "ZDApp.h"
#include "nwk.h"

#include "bdb_interface.h"
#include "zcl_app.h"
#include "factory_reset.h"
#include "commissioning.h"
#include "Debug.h"

#if defined ( MT_TASK )
  #include "MT.h"
  #include "MT_TASK.h"
#endif

const pTaskEventHandlerFn tasksArr[] = {macEventLoop,
                                        nwk_event_loop,
                                        Hal_ProcessEvent,
#if defined( MT_TASK )
                                        MT_ProcessEvent,
#endif
                                        APS_event_loop,
                                        ZDApp_event_loop,
                                        zcl_event_loop,
                                        bdb_event_loop,
                                        zclApp_event_loop,
                                        zclFactoryResetter_loop,
                                        zclCommissioning_event_loop
                                        };

const uint8 tasksCnt = sizeof(tasksArr) / sizeof(tasksArr[0]);
uint16 *tasksEvents;

void osalInitTasks(void) {
    DebugInit();
    uint8 taskID = 0;

    tasksEvents = (uint16 *)osal_mem_alloc(sizeof(uint16) * tasksCnt);
    osal_memset(tasksEvents, 0, (sizeof(uint16) * tasksCnt));
    macTaskInit(taskID++);
    nwk_init(taskID++);
    Hal_Init(taskID++);
#if defined( MT_TASK )
  MT_TaskInit( taskID++ );
#endif
    APS_Init(taskID++);
    ZDApp_Init(taskID++);
    zcl_Init(taskID++);
    bdb_Init(taskID++);
    zclApp_Init(taskID++);
    zclFactoryResetter_Init(taskID++);
    zclCommissioning_Init(taskID++);
}

/*********************************************************************
*********************************************************************/
