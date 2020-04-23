#ifndef _DEBUG_H
#define _DEBUG_H

#include "hal_types.h"
#include "hal_uart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define BUFFLEN 128
#define BUFFER 100

#define PRINT_NUMBER_TYPE long

#define PAD_RIGHT 1
#define PAD_ZERO 2

#define PRINT_BUF_LEN 12

#define PRINT_IMMEDIATE_PRINT 0


#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

extern halUARTCfg_t halUARTConfig;

extern bool DebugInit(void);
extern void LREP(char *format, ...);
extern void LREPMaster(const char *data);

void vprint(const char *fmt, va_list argp);

#endif