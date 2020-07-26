
#include "AF.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "OSAL_PwrMgr.h"
#include "ZComDef.h"
#include "ZDApp.h"
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
#include "utils.h"
#include "version.h"
/*********************************************************************
 * MACROS
 */
#define HAL_KEY_CODE_RELEASE_KEY HAL_KEY_CODE_NOKEY

// use led4 as output pin, osal will shitch it low when go to PM
#define POWER_ON_SENSORS() HAL_TURN_ON_LED4()
#define POWER_OFF_SENSORS() HAL_TURN_OFF_LED4()

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
byte rejoinsLeft = APP_END_DEVICE_REJOIN_TRIES;
uint32 rejoinDelay = APP_END_DEVICE_REJOIN_START_DELAY;
static uint8 currentSensorsReadingPhase = 0;

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
static void zclApp_BindNotification(bdbBindNotificationData_t *data);
static void zclApp_Report(void);
static void zclApp_Rejoin(void);

static void zclApp_ReadSensors(void);
static void zclApp_Battery(void);
static void zclApp_ReadBME280(struct bme280_dev *dev);
static void zclApp_ReadDS18B20(void);
static void zclApp_ReadLumosity(void);
static void zclApp_ReadSoilHumidity(void);

static void zclApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg);
static void zclApp_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg);
static void zclApp_ResetBackoffRetry(void);
static void zclApp_OnConnect(void);

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

    HalI2CInit();
    DebugInit();
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

    bdb_RegisterBindNotificationCB(zclApp_BindNotification);
    bdb_RegisterCommissioningStatusCB(zclApp_ProcessCommissioningStatus);

    LREP("Started build %s \r\n", zclApp_DateCodeNT);
    // this allows power saving, PM2
    osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_CONSERVE);

    LREP("Battery voltage=%d prc=%d \r\n", getBatteryVoltageZCL(), getBatteryRemainingPercentageZCL());
    ZMacSetTransmitPower(TX_PWR_PLUS_4); // set 4dBm

    bdb_StartCommissioning(BDB_COMMISSIONING_MODE_NWK_STEERING | BDB_COMMISSIONING_MODE_FINDING_BINDING);

    // moisture sensor
    IO_DIR_PORT_PIN(SOIL_MOISTURE_PORT, SOIL_MOISTURE_PIN, IO_IN);
    IO_FUNC_PORT_PIN(SOIL_MOISTURE_PORT, SOIL_MOISTURE_PIN, IO_GIO);
    IO_IMODE_PORT_PIN(SOIL_MOISTURE_PORT, SOIL_MOISTURE_PIN, IO_PUD);
    IO_PUD_PORT(SOIL_MOISTURE_PORT, IO_PDN);

    IO_DIR_PORT_PIN(LUMOISITY_PORT, LUMOISITY_PIN, IO_IN);
    IO_FUNC_PORT_PIN(LUMOISITY_PORT, LUMOISITY_PIN, IO_GIO);
    IO_IMODE_PORT_PIN(LUMOISITY_PORT, LUMOISITY_PIN, IO_PUD);
    IO_PUD_PORT(LUMOISITY_PORT, IO_PDN);

    IO_PUD_PORT(0, IO_PDN);
    IO_PUD_PORT(1, IO_PDN);
    // IO_PUD_PORT(2, IO_PDN);
    // IO_DIR_PORT_PIN(0, 7, IO_IN); //light sens
    IO_DIR_PORT_PIN(0, 6, IO_IN);
    IO_DIR_PORT_PIN(0, 5, IO_IN);
    // IO_DIR_PORT_PIN(0, 4, IO_IN); //soil
    IO_DIR_PORT_PIN(0, 3, IO_IN);
    IO_DIR_PORT_PIN(0, 2, IO_IN);
    // IO_DIR_PORT_PIN(0, 1, IO_IN); //led1
    IO_DIR_PORT_PIN(0, 0, IO_IN);

    IO_DIR_PORT_PIN(1, 7, IO_IN);
    IO_DIR_PORT_PIN(1, 6, IO_IN);
    IO_DIR_PORT_PIN(1, 5, IO_IN);
    IO_DIR_PORT_PIN(1, 4, IO_IN);
    // IO_DIR_PORT_PIN(1, 3, IO_IN); //DS18B20
    IO_DIR_PORT_PIN(1, 2, IO_IN);
    // IO_DIR_PORT_PIN(1, 1, IO_IN); //power pint
    IO_DIR_PORT_PIN(1, 0, IO_IN);

    IO_DIR_PORT_PIN(2, 1, IO_IN);
    IO_DIR_PORT_PIN(2, 2, IO_IN);
    IO_DIR_PORT_PIN(2, 3, IO_IN);
}

static void zclApp_ResetBackoffRetry(void) {
    rejoinsLeft = APP_END_DEVICE_REJOIN_TRIES;
    rejoinDelay = APP_END_DEVICE_REJOIN_START_DELAY;
}

static void zclApp_OnConnect(void) {
    LREPMaster("OnConnect \r\n");
    zclApp_ResetBackoffRetry();
    osal_start_reload_timer(zclApp_TaskID, APP_REPORT_EVT, APP_REPORT_DELAY);
}

static void zclApp_ProcessCommissioningStatus(bdbCommissioningModeMsg_t *bdbCommissioningModeMsg) {
    LREP("bdbCommissioningMode=%d bdbCommissioningStatus=%d bdbRemainingCommissioningModes=0x%X\r\n",
         bdbCommissioningModeMsg->bdbCommissioningMode, bdbCommissioningModeMsg->bdbCommissioningStatus,
         bdbCommissioningModeMsg->bdbRemainingCommissioningModes);
    switch (bdbCommissioningModeMsg->bdbCommissioningMode) {
    case BDB_COMMISSIONING_INITIALIZATION:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_NO_NETWORK:
            LREP("No network\r\n");
            HalLedBlink(HAL_LED_1, 3, 50, 500);
            break;
        case BDB_COMMISSIONING_NETWORK_RESTORED:
            zclApp_OnConnect();
            break;
        default:
            break;
        }
        break;
    case BDB_COMMISSIONING_NWK_STEERING:
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_SUCCESS:
            HalLedBlink(HAL_LED_1, 5, 50, 500);
            LREPMaster("BDB_COMMISSIONING_SUCCESS\r\n");
            zclApp_OnConnect();
            break;

        default:
            HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
            break;
        }

        break;

    case BDB_COMMISSIONING_PARENT_LOST:
        LREPMaster("BDB_COMMISSIONING_PARENT_LOST\r\n");
        switch (bdbCommissioningModeMsg->bdbCommissioningStatus) {
        case BDB_COMMISSIONING_NETWORK_RESTORED:
            zclApp_ResetBackoffRetry();
            break;

        default:
            HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
            // // Parent not found, attempt to rejoin again after a exponential backoff delay
            LREP("rejoinsLeft %d rejoinDelay=%ld\r\n", rejoinsLeft, rejoinDelay);
            if (rejoinsLeft > 0) {
                rejoinDelay *= APP_END_DEVICE_REJOIN_BACKOFF;
                rejoinsLeft -= 1;
            } else {
                rejoinDelay = APP_END_DEVICE_REJOIN_MAX_DELAY;
            }
            osal_start_timerEx(zclApp_TaskID, APP_END_DEVICE_REJOIN_EVT, rejoinDelay);
            break;
        }
        break;
    default:
        break;
    }
}

static void zclApp_ProcessIncomingMsg(zclIncomingMsg_t *pInMsg) {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
    if (pInMsg->attrCmd)
        osal_mem_free(pInMsg->attrCmd);
}
uint16 zclApp_event_loop(uint8 task_id, uint16 events) {
    afIncomingMSGPacket_t *MSGpkt;

    (void)task_id; // Intentionally unreferenced parameter
    devStates_t zclApp_NwkState;
    if (events & SYS_EVENT_MSG) {
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclApp_TaskID))) {

            switch (MSGpkt->hdr.event) {

            case KEY_CHANGE:
                zclApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;
            case ZDO_STATE_CHANGE:
                HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
                zclApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
                LREP("NwkState=%d\r\n", zclApp_NwkState);
                if (zclApp_NwkState == DEV_END_DEVICE) {
                    HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
                }
                break;

            case ZCL_INCOMING_MSG:
                zclApp_ProcessIncomingMsg((zclIncomingMsg_t *)MSGpkt);
                break;

            case AF_DATA_CONFIRM_CMD:
                // This message is received as a confirmation of a data packet sent.
                break;

            default:
                LREP("SysEvent 0x%X status 0x%X macSrcAddr 0x%X endPoint=0x%X\r\n", MSGpkt->hdr.event, MSGpkt->hdr.status,
                     MSGpkt->macSrcAddr, MSGpkt->endPoint);
                break;
            }

            // Release the memory
            osal_msg_deallocate((uint8 *)MSGpkt);
        }

        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    if (events & APP_END_DEVICE_REJOIN_EVT) {
        LREPMaster("APP_END_DEVICE_REJOIN_EVT\r\n");
        bdb_ZedAttemptRecoverNwk();
        return (events ^ APP_END_DEVICE_REJOIN_EVT);
    }

    if (events & APP_REPORT_EVT) {
        LREPMaster("APP_REPORT_EVT\r\n");
        zclApp_Report();
        return (events ^ APP_REPORT_EVT);
    }

    if (events & APP_RESET_EVT) {
        LREPMaster("APP_RESET_EVT\r\n");
        zclApp_Rejoin();
        return (events ^ APP_RESET_EVT);
    }

    if (events & APP_READ_SENSORS_EVT) {
        LREPMaster("APP_READ_SENSORS_EVT\r\n");
        zclApp_ReadSensors();
        return (events ^ APP_READ_SENSORS_EVT);
    }

    // Discard unknown events
    return 0;
}

static void zclApp_Rejoin(void) {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
    LREPMaster("Reset to FN\r\n");
    bdb_resetLocalAction();
}

static void zclApp_HandleKeys(byte shift, byte keyCode) {
    static byte prevKeyCode = 0;
    if (keyCode == prevKeyCode) {
        return;
    }
    prevKeyCode = keyCode;

    if (keyCode == HAL_KEY_CODE_RELEASE_KEY) {
        LREPMaster("Key release\r\n");
        osal_stop_timerEx(zclApp_TaskID, APP_RESET_EVT);
        osal_start_timerEx(zclApp_TaskID, APP_REPORT_EVT, 200);
    } else {
        LREPMaster("Key press\r\n");
        uint32 resetHoldTime = APP_RESET_DELAY;
        if (devState == DEV_NWK_ORPHAN) {
            LREP("devState=%d try to restore network\r\n", devState);
            bdb_ZedAttemptRecoverNwk();
        }
        if (!bdbAttributes.bdbNodeIsOnANetwork) {
            resetHoldTime = resetHoldTime >> 2;
        }
        osal_start_timerEx(zclApp_TaskID, APP_RESET_EVT, resetHoldTime);
    }
}

static void zclApp_BindNotification(bdbBindNotificationData_t *data) {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
    LREP("Recieved bind request clusterId=0x%X dstAddr=0x%X ep=%d\r\n", data->clusterId, data->dstAddr, data->ep);
    uint16 maxEntries = 0, usedEntries = 0;
    bindCapacity(&maxEntries, &usedEntries);
    LREP("bindCapacity %d %usedEntries %d \r\n", maxEntries, usedEntries);
}

static void zclApp_Battery(void) {
    zclApp_BatteryVoltage = getBatteryVoltageZCL();
    zclApp_BatteryPercentageRemainig = getBatteryRemainingPercentageZCL();
    zclApp_BatteryVoltageRawAdc = adcReadSampled(HAL_ADC_CHANNEL_VDD, HAL_ADC_RESOLUTION_14, HAL_ADC_REF_125V, 10);

    bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, POWER_CFG, ATTRID_POWER_CFG_BATTERY_VOLTAGE);
    LREP("Battery raw=%d voltage(mV)=%d\r\n", zclApp_BatteryVoltageRawAdc, getBatteryVoltage());
}

static void zclApp_ReadSensors(void) {
    /**
     * FYI: split reading sensors into phases, so single call wouldn't block processor 
     * for extensive ammount of time
     * */
    switch (currentSensorsReadingPhase) {
    case 0:
        HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
        osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_HOLD);
        POWER_ON_SENSORS();
        break;
    case 1:
        zclApp_ReadLumosity();
        break;

    case 2:
        zclApp_Battery();
        zclApp_ReadSoilHumidity();
        break;

    case 3:
        HAL_CRITICAL_STATEMENT(zclApp_ReadBME280(&bme_dev));
        break;

    case 4:
        HAL_CRITICAL_STATEMENT(zclApp_ReadDS18B20());
        break;

    case 5:
        POWER_OFF_SENSORS();
        osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_CONSERVE);
        HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
        break;
    default:
        osal_stop_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT);
        break;
    }
    currentSensorsReadingPhase++;
}

static void zclApp_ReadSoilHumidity(void) {

    zclApp_SoilHumiditySensor_MeasuredValueRawAdc = adcReadSampled(HAL_ADC_CHN_AIN4, HAL_ADC_RESOLUTION_14, HAL_ADC_REF_AVDD, 5);
    // FYI: https://docs.google.com/spreadsheets/d/1qrFdMTo0ZrqtlGUoafeB3hplhU3GzDnVWuUK4M9OgNo/edit?usp=sharing
    uint16 soilHumidityMinRange = (uint16)(0.292 * (double)zclApp_BatteryVoltageRawAdc + 936.0);
    uint16 soilHumidityMaxRange = (uint16)(0.38 * (double)zclApp_BatteryVoltageRawAdc - 447.0);

    LREP("soilHumidityMinRange=%d soilHumidityMaxRange=%d\r\n", soilHumidityMinRange, soilHumidityMaxRange);

    zclApp_SoilHumiditySensor_MeasuredValue =
        (uint16)mapRange(soilHumidityMinRange, soilHumidityMaxRange, 0.0, 10000.0, zclApp_SoilHumiditySensor_MeasuredValueRawAdc);
    LREP("ReadSoilHumidity raw=%d mapped=%d\r\n", zclApp_SoilHumiditySensor_MeasuredValueRawAdc,
         zclApp_SoilHumiditySensor_MeasuredValue);

    bdb_RepChangedAttrValue(zclApp_SecondEP.EndPoint, HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE);
    // bdb_RepChangedAttrValue(zclApp_SecondEP.EndPoint, HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE_RAW_ADC);
}

static void zclApp_ReadDS18B20(void) {
    zclApp_DS18B20_MeasuredValue = readTemperature();
    if (zclApp_DS18B20_MeasuredValue != 1) {
        LREP("ReadDS18B20 t=%d\r\n", zclApp_DS18B20_MeasuredValue);
        bdb_RepChangedAttrValue(zclApp_SecondEP.EndPoint, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
    } else {
        LREPMaster("ReadDS18B20 error\r\n");
    }
}

static void zclApp_ReadLumosity(void) {
    zclApp_IlluminanceSensor_MeasuredValueRawAdc = adcReadSampled(HAL_ADC_CHN_AIN7, HAL_ADC_RESOLUTION_14, HAL_ADC_REF_AVDD, 5);
    zclApp_IlluminanceSensor_MeasuredValue = zclApp_IlluminanceSensor_MeasuredValueRawAdc;
    bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, ILLUMINANCE, ATTRID_MS_ILLUMINANCE_MEASURED_VALUE);
    LREP("IlluminanceSensor_MeasuredValue value=%d\r\n", zclApp_IlluminanceSensor_MeasuredValue);
}

void user_delay_ms(uint32_t period) { MicroWait(period * 1000); }

static void zclApp_ReadBME280(struct bme280_dev *dev) {
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

        for (uint8 i = 0; i < 35; i++) {
            dev->delay_ms(2);
        }
        rslt = bme280_get_sensor_data(BME280_ALL, &bme_results, dev);
        LREP("ReadBME280 t=%ld, p=%ld, h=%ld\r\n", bme_results.temperature, bme_results.pressure, bme_results.humidity);
        zclApp_Temperature_Sensor_MeasuredValue = (int16)bme_results.temperature;
        zclApp_PressureSensor_MeasuredValueHPA = bme_results.pressure;
        zclApp_PressureSensor_MeasuredValue = bme_results.pressure / 100;
        zclApp_HumiditySensor_MeasuredValue = (uint16)(bme_results.humidity * 100 / 1024);
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, PRESSURE, ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE);
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE);
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, HUMIDITY, ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE_HPA);
    } else {
        LREP("ReadBME280 init error %d\r\n", rslt);
    }
}
static void zclApp_Report(void) { 

    currentSensorsReadingPhase = 0;
    osal_start_reload_timer(zclApp_TaskID, APP_READ_SENSORS_EVT, 100);
 }

/****************************************************************************
****************************************************************************/
