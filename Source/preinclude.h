#define SECURE 1
#define TC_LINKKEY_JOIN
#define NV_INIT
#define NV_RESTORE
#define POWER_SAVING
#define NWK_AUTO_POLL
#define xZTOOL_P1
#define xMT_TASK
#define xMT_APP_FUNC
#define xMT_SYS_FUNC
#define xMT_ZDO_FUNC
#define xMT_ZDO_MGMT
#define xMT_APP_CNF_FUNC
#define MULTICAST_ENABLED FALSE
#define ZCL_READ
#define ZCL_WRITE
#define ZCL_BASIC
#define ZCL_IDENTIFY
#define ZCL_SCENES
#define ZCL_GROUPS
#define DISABLE_GREENPOWER_BASIC_PROXY


#ifdef DEFAULT_CHANLIST
    #undef DEFAULT_CHANLIST
    #define DEFAULT_CHANLIST 0x07FFF800
#else
    #define DEFAULT_CHANLIST 0x07FFF800
#endif