#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "ZComDef.h"
#include "hal_drivers.h"

#include "APS.h"
#include "stub_aps.h"
#include "ZDApp.h"
#include "nwk.h"

#include "bdb_interface.h"
#include "bdb_touchlink_initiator.h"
#include "zcl_freepadapp.h"

const pTaskEventHandlerFn tasksArr[] = {macEventLoop,
                                        nwk_event_loop,
                                        Hal_ProcessEvent,
                                        APS_event_loop,
                                        ZDApp_event_loop,                                        
                                        zcl_event_loop,
                                        StubAPS_ProcessEvent,
                                        touchLinkInitiator_event_loop,
                                        bdb_event_loop,
                                        zclFreePadApp_event_loop};

const uint8 tasksCnt = sizeof(tasksArr) / sizeof(tasksArr[0]);
uint16 *tasksEvents;

void osalInitTasks(void) {
    uint8 taskID = 0;

    tasksEvents = (uint16 *)osal_mem_alloc(sizeof(uint16) * tasksCnt);
    osal_memset(tasksEvents, 0, (sizeof(uint16) * tasksCnt));
    macTaskInit(taskID++);
    nwk_init(taskID++);
    Hal_Init(taskID++);
    APS_Init(taskID++);
    ZDApp_Init(taskID++);
    zcl_Init(taskID++);
    StubAPS_Init(taskID++);
    touchLinkInitiator_Init(taskID++);
    bdb_Init(taskID++);
    zclFreePadApp_Init(taskID);
}

/*********************************************************************
*********************************************************************/
