
#include "AF.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "OSAL_PwrMgr.h"
#include "ZComDef.h"
#include "ZDApp.h"
#include "ZDNwkMgr.h"
#include "ZDObject.h"
#include "math.h"

#include "nwk_util.h"
#include "zcl.h"
#include "zcl_app.h"
#include "zcl_diagnostic.h"
#include "zcl_general.h"
#include "zcl_lighting.h"
#include "zcl_ms.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "gp_interface.h"

#include "Debug.h"

#include "OnBoard.h"

/* HAL */
#include "bme280.h"
#include "ds18b20.h"
#include "hal_adc.h"
#include "hal_drivers.h"
#include "hal_i2c.h"
#include "hal_key.h"
#include "hal_led.h"

#include "battery.h"
#include "commissioning.h"
#include "factory_reset.h"
#include "utils.h"
#include "version.h"

/*********************************************************************
 * MACROS
 */
#define HAL_KEY_CODE_RELEASE_KEY HAL_KEY_CODE_NOKEY

// use led4 as output pin, osal will shitch it low when go to PM
#define POWER_ON_SENSORS()                                                                                                                 \
    do {                                                                                                                                   \
        HAL_TURN_ON_LED4();                                                                                                                \
        st(T3CTL |= BV(4););                                                                                                               \
        IO_PUD_PORT(OCM_CLK_PORT, IO_PUP);                                                                                                 \
        IO_PUD_PORT(OCM_DATA_PORT, IO_PUP);                                                                                                \
        IO_PUD_PORT(DS18B20_PORT, IO_PUP);                                                                                                 \
    } while (0)
#define POWER_OFF_SENSORS()                                                                                                                \
    do {                                                                                                                                   \
        HAL_TURN_OFF_LED4();                                                                                                               \
        st(T3CTL &= ~BV(4); T3CTL |= BV(2););                                                                                              \
        IO_PUD_PORT(OCM_CLK_PORT, IO_PDN);                                                                                                 \
        IO_PUD_PORT(OCM_DATA_PORT, IO_PDN);                                                                                                \
        IO_PUD_PORT(DS18B20_PORT, IO_PDN);                                                                                                 \
    } while (0)

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

extern bool requestNewTrustCenterLinkKey;
byte zclApp_TaskID;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 0, .addr.shortAddr = 0};
struct bme280_data bme_results;
struct bme280_dev bme_dev = {.dev_id = BME280_I2C_ADDR_PRIM,
                             .intf = BME280_I2C_INTF,
                             .read = I2C_ReadMultByte,
                             .write = I2C_WriteMultByte,
                             .delay_ms = user_delay_ms};
/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclApp_HandleKeys(byte shift, byte keys);
static void zclApp_ReadSensors(void);
static void zclApp_InitBME280(struct bme280_dev *dev);
static void zclApp_ReadBME280(struct bme280_dev *dev);
static void zclApp_ReadDS18B20(void);
static void zclApp_ReadLumosity(void);
static void zclApp_ReadSoilHumidity(void);
static void zclApp_InitPWM(void);

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclApp_CmdCallbacks = {
    NULL, // Basic Cluster Reset command
    NULL, // Identify Trigger Effect command
    NULL, // On/Off cluster commands
    NULL, // On/Off cluster enhanced command Off with Effect
    NULL, // On/Off cluster enhanced command On with Recall Global Scene
    NULL, // On/Off cluster enhanced command On with Timed Off
    NULL, // RSSI Location command
    NULL  // RSSI Location Response command
};

void zclApp_Init(byte task_id) {
    IO_IMODE_PORT_PIN(SOIL_MOISTURE_PORT, SOIL_MOISTURE_PIN, IO_TRI); // tri state p0.4 (soil humidity pin)
    IO_IMODE_PORT_PIN(LUMOISITY_PORT, LUMOISITY_PIN, IO_TRI);         // tri state p0.7 (lumosity pin)
    IO_PUD_PORT(OCM_CLK_PORT, IO_PUP);
    IO_PUD_PORT(OCM_DATA_PORT, IO_PUP)
    IO_PUD_PORT(DS18B20_PORT, IO_PUP);
    POWER_OFF_SENSORS();

    HalI2CInit();
    zclApp_InitPWM();
    // this is important to allow connects throught routers
    // to make this work, coordinator should be compiled with this flag #define TP2_LEGACY_ZC
    requestNewTrustCenterLinkKey = FALSE;

    zclApp_TaskID = task_id;

    zclGeneral_RegisterCmdCallbacks(1, &zclApp_CmdCallbacks);
    zcl_registerAttrList(zclApp_FirstEP.EndPoint, zclApp_AttrsFirstEPCount, zclApp_AttrsFirstEP);
    bdb_RegisterSimpleDescriptor(&zclApp_FirstEP);

    zcl_registerAttrList(zclApp_SecondEP.EndPoint, zclApp_AttrsSecondEPCount, zclApp_AttrsSecondEP);
    bdb_RegisterSimpleDescriptor(&zclApp_SecondEP);

    zcl_registerForMsg(zclApp_TaskID);

    // Register for all key events - This app will handle all key events
    RegisterForKeys(zclApp_TaskID);
    LREP("Started build %s \r\n", zclApp_DateCodeNT);

    osal_start_reload_timer(zclApp_TaskID, APP_REPORT_EVT, APP_REPORT_DELAY);
}

uint16 zclApp_event_loop(uint8 task_id, uint16 events) {
    afIncomingMSGPacket_t *MSGpkt;

    (void)task_id; // Intentionally unreferenced parameter
    if (events & SYS_EVENT_MSG) {
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclApp_TaskID))) {
            switch (MSGpkt->hdr.event) {
            case KEY_CHANGE:
                zclApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;
            case ZCL_INCOMING_MSG:
                if (((zclIncomingMsg_t *)MSGpkt)->attrCmd) {
                    osal_mem_free(((zclIncomingMsg_t *)MSGpkt)->attrCmd);
                }
                break;

            default:
                break;
            }
            // Release the memory
            osal_msg_deallocate((uint8 *)MSGpkt);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & APP_REPORT_EVT) {
        LREPMaster("APP_REPORT_EVT\r\n");
        zclApp_ReadSensors();
        return (events ^ APP_REPORT_EVT);
    }
    // Discard unknown events
    return 0;
}

static void zclApp_HandleKeys(byte portAndAction, byte keyCode) {
    LREP("zclApp_HandleKeys portAndAction=0x%X keyCode=0x%X\r\n", portAndAction, keyCode);
    zclFactoryResetter_HandleKeys(portAndAction, keyCode);
    zclCommissioning_HandleKeys(portAndAction, keyCode);
    if (portAndAction & HAL_KEY_PRESS) {
        LREPMaster("Key press\r\n");
        osal_start_timerEx(zclApp_TaskID, APP_REPORT_EVT, 200);
    }
}
static void zclApp_InitPWM(void) {
    PERCFG &= ~(0x20); // Select Timer 3 Alternative 1 location
    P2SEL |= 0x20;
    P2DIR |= 0xC0;  // Give priority to Timer 1 channel2-3
    P1SEL |= BV(4); // Set P1_4 to peripheral, Timer 1,channel 2
    P1DIR |= BV(4);

    T3CTL &= ~BV(4); // Stop timer 3 (if it was running)
    T3CTL |= BV(2);  // Clear timer 3
    T3CTL &= ~0x08;  // Disable Timer 3 overflow interrupts
    T3CTL |= 0x03;   // Timer 3 mode = 3 - Up/Down

    T3CCTL1 &= ~0x40; // Disable channel 0 interrupts
    T3CCTL1 |= BV(2); // Ch0 mode = compare
    T3CCTL1 |= BV(4); // Ch0 output compare mode = toggle on compare

    T3CTL &= ~(BV(7) | BV(6) | BV(5)); // Clear Prescaler divider value
    T3CC0 = 4;                         // Set ticks
}

static void zclApp_ReadSensors(void) {
    POWER_ON_SENSORS();
    HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
    osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_HOLD);


    zclApp_ReadLumosity();

    zclBattery_Report();
    zclApp_ReadSoilHumidity();

    zclApp_InitBME280(&bme_dev);



    zclApp_ReadBME280(&bme_dev);

    osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_CONSERVE);

    zclApp_ReadDS18B20();

    POWER_OFF_SENSORS();
}

static void zclApp_ReadSoilHumidity(void) {
    zclApp_SoilHumiditySensor_MeasuredValueRawAdc = adcReadSampled(SOIL_MOISTURE_PIN, HAL_ADC_RESOLUTION_14, HAL_ADC_REF_AVDD, 5);
    // FYI: https://docs.google.com/spreadsheets/d/1qrFdMTo0ZrqtlGUoafeB3hplhU3GzDnVWuUK4M9OgNo/edit?usp=sharing
    uint16 soilHumidityMinRangeAir = (uint16)AIR_COMPENSATION_FORMULA(zclBattery_RawAdc);
    uint16 soilHumidityMaxRangeWater = (uint16)WATER_COMPENSATION_FORMULA(zclBattery_RawAdc);
    LREP("soilHumidityMinRangeAir=%d soilHumidityMaxRangeWater=%d\r\n", soilHumidityMinRangeAir, soilHumidityMaxRangeWater);
    zclApp_SoilHumiditySensor_MeasuredValue =
        (uint16)mapRange(soilHumidityMinRangeAir, soilHumidityMaxRangeWater, 0.0, 10000.0, zclApp_SoilHumiditySensor_MeasuredValueRawAdc);
    LREP("ReadSoilHumidity raw=%d mapped=%d\r\n", zclApp_SoilHumiditySensor_MeasuredValueRawAdc, zclApp_SoilHumiditySensor_MeasuredValue);

    bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, SOIL_HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE);
}

static void zclApp_ReadDS18B20(void) {
    int16 temp = readTemperature();
    if (temp != 1) {
        zclApp_DS18B20_MeasuredValue = temp;
        LREP("ReadDS18B20 t=%d\r\n", zclApp_DS18B20_MeasuredValue);
        bdb_RepChangedAttrValue(zclApp_SecondEP.EndPoint, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
    } else {
        LREPMaster("ReadDS18B20 error\r\n");
    }
}

static void zclApp_ReadLumosity(void) {
    zclApp_IlluminanceSensor_MeasuredValueRawAdc = adcReadSampled(LUMOISITY_PIN, HAL_ADC_RESOLUTION_14, HAL_ADC_REF_AVDD, 5);
    zclApp_IlluminanceSensor_MeasuredValue = zclApp_IlluminanceSensor_MeasuredValueRawAdc;
    bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, ILLUMINANCE, ATTRID_MS_ILLUMINANCE_MEASURED_VALUE);
    LREP("IlluminanceSensor_MeasuredValue value=%d\r\n", zclApp_IlluminanceSensor_MeasuredValue);
}

void user_delay_ms(uint32_t period) { MicroWait(period * 1000); }

static void zclApp_InitBME280(struct bme280_dev *dev) {
    int8_t rslt = bme280_init(dev);
    if (rslt == BME280_OK) {
        uint8_t settings_sel;
        dev->settings.osr_h = BME280_OVERSAMPLING_1X;
        dev->settings.osr_p = BME280_OVERSAMPLING_16X;
        dev->settings.osr_t = BME280_OVERSAMPLING_2X;
        dev->settings.filter = BME280_FILTER_COEFF_16;
        dev->settings.standby_time = BME280_STANDBY_TIME_62_5_MS;

        settings_sel = BME280_OSR_PRESS_SEL;
        settings_sel |= BME280_OSR_TEMP_SEL;
        settings_sel |= BME280_OSR_HUM_SEL;
        settings_sel |= BME280_STANDBY_SEL;
        settings_sel |= BME280_FILTER_SEL;
        rslt = bme280_set_sensor_settings(settings_sel, dev);
        rslt = bme280_set_sensor_mode(BME280_NORMAL_MODE, dev);

        uint32_t req_delay = bme280_cal_meas_delay(&dev->settings);
        dev->delay_ms(req_delay);
    } else {
        LREP("ReadBME280 init error %d\r\n", rslt);
    }
}
static void zclApp_ReadBME280(struct bme280_dev *dev) {
    int8_t rslt = bme280_get_sensor_data(BME280_ALL, &bme_results, dev);
    if (rslt == BME280_OK) {
        zclApp_Temperature_Sensor_MeasuredValue = (int16)bme_results.temperature;
        zclApp_PressureSensor_ScaledValue = (int16)(pow(10.0, (double)zclApp_PressureSensor_Scale) * (double)bme_results.pressure);
        zclApp_PressureSensor_MeasuredValue = bme_results.pressure / 100;
        LREP("ReadBME280 t=%ld, p=%ld h=%ld\r\n", bme_results.temperature, bme_results.pressure, bme_results.humidity);
        zclApp_HumiditySensor_MeasuredValue = (uint16)(bme_results.humidity * 100 / 1024);
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, PRESSURE, ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE);
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE);
    } else {
        LREP("ReadBME280 read error %d\r\n", rslt);
    }
}

/****************************************************************************
****************************************************************************/
