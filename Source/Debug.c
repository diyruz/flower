#ifdef HAL_UART
#include "Debug.h"
#include "OSAL.h"
#include "OSAL_Memory.h"
#define UART_PORT HAL_UART_PORT_0
halUARTCfg_t halUARTConfig;
static void HalUARTCback(uint8 port, uint8 event) {}

bool DebugInit() {
    halUARTConfig.configured = TRUE;
    halUARTConfig.baudRate = HAL_UART_BR_115200;
    halUARTConfig.flowControl = FALSE;
    halUARTConfig.flowControlThreshold = 10; // this parameter indicates number of bytes left before Rx Buffer
                                             // reaches maxRxBufSize
    halUARTConfig.idleTimeout = 10;          // this parameter indicates rx timeout period in millisecond
    halUARTConfig.rx.maxBufSize = BUFFLEN;
    halUARTConfig.tx.maxBufSize = BUFFLEN;
    halUARTConfig.intEnable = TRUE;
    halUARTConfig.callBackFunc = HalUARTCback;
    HalUARTInit();
    return HalUARTOpen(UART_PORT, &halUARTConfig) == HAL_UART_SUCCESS;
}

void vprint(const char *fmt, va_list argp) {
    char string[200];
    if (0 < vsprintf(string, fmt, argp)) // build string
    {
        HalUARTWrite(UART_PORT, (uint8 *)string, strlen(string));
    }
}

void LREPMaster(const char *data) {
    if (data == NULL)
        return;
    HalUARTWrite(UART_PORT, (uint8 *)data, strlen(data));
}

void LREP(char *format, ...) {
    va_list argp;
    va_start(argp, format);
    vprint(format, argp);
    va_end(argp);
}
#else
#include <stdio.h>
bool DebugInit() {}
void LREP(char *format, ...) {}
void LREPMaster(const char *data) {
    printf(data);
}
void vprint(const char *fmt, va_list argp){}
#endif
