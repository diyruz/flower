#ifndef _DEBUG_H
#define _DEBUG_H

#include "hal_types.h"
#include "hal_uart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define BUFFLEN 128

extern halUARTCfg_t halUARTConfig;
extern bool DebugInit(void);
extern void LREP(char *format, ...);
extern void LREPMaster(const char *data);

void vprint(const char *fmt, va_list argp);

#endif