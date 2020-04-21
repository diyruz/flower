#ifndef _DEBUG_H
#define _DEBUG_H

#include "hal_uart.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "hal_types.h"


#define BUFFLEN    128
#define BUFFER 100

#define		PRINT_NUMBER_TYPE			long

#define 	PAD_RIGHT 					1
#define 	PAD_ZERO  					2

#define 	PRINT_BUF_LEN 				12

#define		PRINT_IMMEDIATE_PRINT		0	

extern halUARTCfg_t halUARTConfig;

extern bool DebugInit(void);
extern void LREP(char *format, ...);
extern void LREPMaster(const char *data);

void vprint(const char *fmt, va_list argp);

#endif