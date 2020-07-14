#include "battery.h"
#include "hal_adc.h"

uint8 getBatteryVoltageZCL(void) {
    uint16 volt16 = getBatteryVoltage();
    uint8 volt8 = (uint8)(volt16 / 100);
    if ((volt16 - (volt8 * 100)) > 50) {
        return volt8 + 1;
    } else {
        return volt8;
    }
}

uint16 getBatteryVoltage(void) {
    HalAdcSetReference(HAL_ADC_REF_125V);
    // for 1.9V ADC 4428, for 3.3V ADC 7608 // ((7608-4428)/(3300-1900))=2.271
    float result = 0;
    result = 1900 + ((HalAdcRead(HAL_ADC_CHANNEL_VDD, HAL_ADC_RESOLUTION_14) - 4428) / 2.271);
    return (uint16)result;
}

/***
Specifies the remaining battery life as a half integer percentage of the full battery capacity (e.g., 34.5%, 45%, 68.5%, 90%)
with a range between zero and 100%, with 0x00 = 0%, 0x64 = 50%, and 0xC8 = 100%.
 *           The calculation is based on a linearized version of the battery's discharge
 *           curve. 3.0V returns 100% battery level. The limit for power failure is 2.1V and
 *           is considered to be the lower boundary.
 *
 *           The discharge curve for CR2032 is non-linear. In this model it is split into
 *           4 linear sections:
 *           - Section 1: 3.0V - 2.9V = 100% - 42% (58% drop on 100 mV)
 *           - Section 2: 2.9V - 2.74V = 42% - 18% (24% drop on 160 mV)
 *           - Section 3: 2.74V - 2.44V = 18% - 6% (12% drop on 300 mV)
 *           - Section 4: 2.44V - 2.1V = 6% - 0% (6% drop on 340 mV)
 *
 *           These numbers are by no means accurate. Temperature and
 *           load in the actual application is not accounted for!
*/
uint8 getBatteryRemainingPercentageZCL(void) {
    uint16 volt16 = getBatteryVoltage();
    float battery_level;
    if (volt16 >= 3000) {
        battery_level = 100;
    } else if (volt16 > 2900) {
        battery_level = 100 - ((3000 - volt16) * 58) / 100;
    } else if (volt16 > 2740) {
        battery_level = 42 - ((2900 - volt16) * 24) / 160;
    } else if (volt16 > 2440) {
        battery_level = 18 - ((2740 - volt16) * 12) / 300;
    } else if (volt16 > 2100) {
        battery_level = 6 - ((2440 - volt16) * 6) / 340;
    } else {
        battery_level = 0;
    }
    return (uint8)(battery_level * 2);
}