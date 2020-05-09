#include "hal_adc.h"
#include "battery.h"

#define MIN_VOLTAGE 20
#define MAX_VOLTAGE 33
/* (( 3 * 1,15 ) / (( 2^12 / 2 ) - 1 )) * 10  */
#define MULTI 0.0168539326

uint8 getBatteryVoltage(void) {
    uint16 value;
    HalAdcSetReference(HAL_ADC_REF_125V);
    value = HalAdcRead(HAL_ADC_CHANNEL_VDD, HAL_ADC_RESOLUTION_12);
    HalAdcSetReference(HAL_ADC_REF_AVDD);
    return (uint8)(value * MULTI);
}

/***
Specifies the remaining battery life as a half integer percentage of the full battery capacity (e.g., 34.5%, 45%, 68.5%, 90%) 
with a range between zero and 100%, with 0x00 = 0%, 0x64 = 50%, and 0xC8 = 100%.
***/
uint8 getBatteryRemainingPercentageZCL(void) {
    return ((getBatteryVoltage() - MIN_VOLTAGE) * 200) / (MAX_VOLTAGE - MIN_VOLTAGE);
}
