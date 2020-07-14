#ifdef HAL_UART
#include "Debug.h"
#include "OSAL.h"
#include "OSAL_Memory.h"
#define UART_PORT HAL_UART_PORT_0
halUARTCfg_t halUARTConfig;

bool DebugInit() {
    halUARTConfig.configured = TRUE;
    halUARTConfig.baudRate = HAL_UART_BR_115200;
    halUARTConfig.flowControl = FALSE;
    halUARTConfig.flowControlThreshold = 10; // this parameter indicates number of bytes left before Rx Buffer
                                             // reaches maxRxBufSize
    halUARTConfig.idleTimeout = 10;          // this parameter indicates rx timeout period in millisecond
    halUARTConfig.rx.maxBufSize = 0;
    halUARTConfig.tx.maxBufSize = BUFFLEN;
    halUARTConfig.intEnable = FALSE;

    HalUARTInit();
    if (HalUARTOpen(UART_PORT, &halUARTConfig) == HAL_UART_SUCCESS) {
        LREPMaster("Initialized debug module \r\n");
        return true;
    }
    return false;
}

void vprint(const char *fmt, va_list argp) {
    char string[100];
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
bool DebugInit() {}
void LREP(char *format, ...) {}
void LREPMaster(const char *data) {}
void vprint(const char *fmt, va_list argp) {}
#endif
