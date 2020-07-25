#include "ds18b20.h"

#define MSK (BV(0) | BV(1) | BV(2))
#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE
#define DS18B20_WRITE_SCRATCHPAD 0x4E

// Device resolution
#define TEMP_9_BIT 0x1F  //  9 bit
#define TEMP_10_BIT 0x3F // 10 bit
#define TEMP_11_BIT 0x5F // 11 bit
#define TEMP_12_BIT 0x7F // 12 bit

#define MAX_CONVERSION_TIME (750 * 1.2) // ms 750ms + some overhead

static void _delay_us(uint16);
static void _delay_ms(uint16);
static void ds18b20_send(uint8);
static uint8 ds18b20_read(void);
static void ds18b20_send_byte(int8);
static uint8 ds18b20_read_byte(void);
static uint8 ds18b20_Reset(void);
static void ds18b20_GroudPins(void);
static void ds18b20_setResolution(uint8_t resolution);

static void _delay_us(uint16 microSecs) {
    while (microSecs--) {
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
    }
}

static void _delay_ms(uint16 milliSecs) {
    while (milliSecs--) {
        _delay_us(1000);
    }
}

// Sends one bit to bus
static void ds18b20_send(uint8 bit) {
    TSENS_SBIT = 1;
    TSENS_DIR |= TSENS_BV; // output
    TSENS_SBIT = 0;
    if (bit != 0)
        _delay_us(8);
    else
        _delay_us(80);
    TSENS_SBIT = 1;
    if (bit != 0)
        _delay_us(80);
    else
        _delay_us(2);
    // TSENS_SBIT = 1;
}

// Reads one bit from bus
static uint8 ds18b20_read(void) {
    TSENS_SBIT = 1;
    TSENS_DIR |= TSENS_BV; // output
    TSENS_SBIT = 0;
    _delay_us(2);
    // TSENS_SBIT = 1;
    //_delay_us(15);
    TSENS_DIR &= ~TSENS_BV; // input
    _delay_us(5);
    uint8 i = TSENS_SBIT;
    _delay_us(60);
    return i;
}

// Sends one byte to bus
static void ds18b20_send_byte(int8 data) {
    uint8 i, x;
    for (i = 0; i < 8; i++) {
        x = data >> i;
        x &= 0x01;
        ds18b20_send(x);
    }
    //_delay_us(100);
}

// Reads one byte from bus
static uint8 ds18b20_read_byte(void) {
    uint8 i;
    uint8 data = 0;
    for (i = 0; i < 8; i++) {
        if (ds18b20_read())
            data |= 0x01 << i;
        //_delay_us(25);
    }
    return (data);
}

// Sends reset pulse
static uint8 ds18b20_Reset(void) {
    TSENS_SBIT = 0;
    TSENS_DIR |= TSENS_BV; // output
    _delay_us(600);
    TSENS_DIR &= ~TSENS_BV; // input
    _delay_us(70);
    uint8 i = TSENS_SBIT;
    _delay_us(200);
    TSENS_SBIT = 1;
    TSENS_DIR |= TSENS_BV; // output
    _delay_us(600);
    return i;
}

static void ds18b20_GroudPins(void) {
    // TSENS_SBIT = 0;
    TSENS_DIR &= ~TSENS_BV; // input
}

static void ds18b20_setResolution(uint8_t resolution) {
    ds18b20_Reset();
    ds18b20_send_byte(DS18B20_SKIP_ROM);
    ds18b20_send_byte(DS18B20_WRITE_SCRATCHPAD);
    // two dummy values for LOW & HIGH ALARM
    ds18b20_send_byte(0);
    ds18b20_send_byte(100);
    switch (resolution) {
    case 12:
        ds18b20_send_byte(TEMP_12_BIT);
        break;

    case 11:
        ds18b20_send_byte(TEMP_11_BIT);
        break;

    case 10:
        ds18b20_send_byte(TEMP_10_BIT);
        break;

    case 9:
    default:
        ds18b20_send_byte(TEMP_9_BIT);
        break;
    }
    ds18b20_Reset();
}

int16 readTemperature(void) {
    float temperature = 0;
    uint8 temp1, temp2, retry_count = 50, delay_time = MAX_CONVERSION_TIME / retry_count;
    ds18b20_setResolution(TEMP_12_BIT);
    ds18b20_Reset();

    ds18b20_send_byte(DS18B20_SKIP_ROM);
    ds18b20_send_byte(DS18B20_CONVERT_T);

    while (retry_count) {
        _delay_ms(delay_time);
        ds18b20_Reset();
        ds18b20_send_byte(DS18B20_SKIP_ROM);
        ds18b20_send_byte(DS18B20_READ_SCRATCHPAD);
        temp1 = ds18b20_read_byte();
        temp2 = ds18b20_read_byte();
        ds18b20_Reset();

        if (temp1 == 0xff && temp2 == 0xff) {
            // No sensor found.
            ds18b20_GroudPins();
            return 0;
        }
        if (temp1 == 0x50 && temp2 == 0x05) {
            // Power-up State, not ready yet
            retry_count--;
            continue;
        }
        temperature = (uint16)temp1 | (uint16)(temp2 & MSK) << 8;
        // neg. temp
        if (temp2 & (BV(3))) {
            temperature = temperature / 16.0 - 128.0;
        }
        // pos. temp
        else {
            temperature = temperature / 16.0;
        }

        ds18b20_GroudPins();
        return (int16)(temperature * 100);
    }

    ds18b20_GroudPins();
    return 1;
}