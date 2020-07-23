#include "Debug.h"
#include "battery.h"
#include "hal_adc.h"
#include "utils.h"
// (( 3 * 1.15 ) / (( 2^14 / 2 ) - 1 )) * 1000
#define MULTI (double) 0.4211939934

#define VOLTAGE_MIN (double) 2.0
#define VOLTAGE_MAX (double) 3.3

uint8 getBatteryVoltageZCL(void) {
    uint16 volt16 = getBatteryVoltage();
    uint8 volt8 = (uint8)(volt16 / 100);
    if ((volt16 - (volt8 * 100)) > 50) {
        return volt8 + 1;
    } else {
        return volt8;
    }
}
//return millivolts
uint16 getBatteryVoltage(void) {
    HalAdcSetReference(HAL_ADC_REF_125V);
    double rawADC = adcReadSampled(HAL_ADC_CHANNEL_VDD, HAL_ADC_RESOLUTION_14, HAL_ADC_REF_125V, 10);
    return (uint16) (rawADC * MULTI);
}

uint8 getBatteryRemainingPercentageZCL(void) {
    return mapRange(VOLTAGE_MIN, VOLTAGE_MAX, 0, 200, getBatteryVoltage());
}