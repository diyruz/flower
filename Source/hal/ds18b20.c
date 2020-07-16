#include "ds18b20.h"

#define MSK (BV(0) | BV(1) | BV(2))
#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE

static void _delay_us(uint16);
static void _delay_ms(uint16);
static void ds18b20_send(uint8);
static uint8 ds18b20_read(void);
static void ds18b20_send_byte(int8);
static uint8 ds18b20_read_byte(void);
static uint8 ds18b20_RST_PULSE(void);

static void _delay_us(uint16 microSecs) {
    while (microSecs--) {
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
static uint8 ds18b20_RST_PULSE(void) {
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

int16 readTemperature(void) {
    float temperature = 0;
    uint8 temp1, temp2, retry_count = 5;
    if (!ds18b20_RST_PULSE()) {
        while (retry_count) {
            ds18b20_send_byte(DS18B20_SKIP_ROM);
            ds18b20_send_byte(DS18B20_CONVERT_T);
            _delay_ms(750);
            ds18b20_RST_PULSE();
            ds18b20_send_byte(DS18B20_SKIP_ROM);
            ds18b20_send_byte(DS18B20_READ_SCRATCHPAD);
            temp1 = ds18b20_read_byte();
            temp2 = ds18b20_read_byte();
            ds18b20_RST_PULSE();

            if (temp1 == 0xff && temp2 == 0xff) {
                // No sensor found.
                return 0;
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

            if (temperature == 85) { // if 85, sensor is not yet ready
                retry_count--;
            } else {
                return (int16)(temperature * 100);
            }
        }
    } else {
        // Fail
        return 1;
    }
    return 1;
}