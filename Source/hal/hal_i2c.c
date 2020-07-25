/**************************************************************************************************
  Filename:       hal_i2c.c
  Revised:        $Date: 2012-09-21 06:30:38 -0700 (Fri, 21 Sep 2012) $
  Revision:       $Revision: 31581 $

  Description:    This module defines the HAL I2C API for the CC2541ST. It
                  implements the I2C master.


  Copyright 2012 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/
#include "hal_i2c.h"

#include "Debug.h"
#include "ioCC2530.h"
#include "zcomdef.h"
#include "utils.h"

#define STATIC static

#if !defined HAL_I2C_RETRY_CNT
#define HAL_I2C_RETRY_CNT 3
#endif

// the default cofiguration below uses P0.6 for SDA and P0.5 for SCL.
// change these as needed.
#ifndef OCM_CLK_PORT
#define OCM_CLK_PORT 0
#endif

#ifndef OCM_DATA_PORT
#define OCM_DATA_PORT 0
#endif

#ifndef OCM_CLK_PIN
#define OCM_CLK_PIN 5
#endif

#ifndef OCM_DATA_PIN
#define OCM_DATA_PIN 6
#endif



#define OCM_ADDRESS (0xA0)

#define SMB_ACK (0)
#define SMB_NAK (1)
#define SEND_STOP (0)
#define NOSEND_STOP (1)
#define SEND_START (0)
#define NOSEND_START (1)

// device specific as to where the 17th address bit goes...

// *************************   MACROS   ************************************
#undef P



// OCM port I/O defintions
#define OCM_SCL BNAME(OCM_CLK_PORT, OCM_CLK_PIN)
#define OCM_SDA BNAME(OCM_DATA_PORT, OCM_DATA_PIN)



#define OCM_DATA_HIGH() { IO_DIR_PORT_PIN(OCM_DATA_PORT, OCM_DATA_PIN, IO_IN); }

#define OCM_DATA_LOW()                                                                                                                     \
    {                                                                                                                                      \
        IO_DIR_PORT_PIN(OCM_DATA_PORT, OCM_DATA_PIN, IO_OUT);                                                                              \
        OCM_SDA = 0;                                                                                                                       \
    }



STATIC void hali2cSend(uint8 *buffer, uint16 len, uint8 sendStart, uint8 sendStop);
STATIC _Bool hali2cSendByte(uint8 dByte);
STATIC void hali2cWrite(bool dBit);
STATIC void hali2cClock(bool dir);
STATIC void hali2cStart(void);
STATIC void hali2cStop(void);
STATIC void hali2cReceive(uint8 address, uint8 *buffer, uint16 len);
STATIC uint8 hali2cReceiveByte(void);
STATIC _Bool hali2cRead(void);
STATIC void hali2cSendDeviceAddress(uint8 address);

STATIC __near_func void hali2cWait(uint8);

static void hali2cGroudPins(void);

void hali2cGroudPins(void) {
    IO_DIR_PORT_PIN(OCM_DATA_PORT, OCM_DATA_PIN, IO_IN);
    IO_DIR_PORT_PIN(OCM_CLK_PORT, OCM_CLK_PIN, IO_IN);
}

STATIC uint8 s_xmemIsInit;

/*********************************************************************
 * @fn      HalI2CInit
 * @brief   Initializes two-wire serial I/O bus
 * @param   void
 * @return  void
 */
void HalI2CInit(void) {
    if (!s_xmemIsInit) {
        s_xmemIsInit = 1;

        // // Set port pins as inputs
        // IO_DIR_PORT_PIN(OCM_CLK_PORT, OCM_CLK_PIN, IO_IN);
        // IO_DIR_PORT_PIN(OCM_DATA_PORT, OCM_DATA_PIN, IO_IN);
        // //
        // // Set for general I/O operation
        // IO_FUNC_PORT_PIN(OCM_CLK_PORT, OCM_CLK_PIN, IO_GIO);
        // IO_FUNC_PORT_PIN(OCM_DATA_PORT, OCM_DATA_PIN, IO_GIO);
        // //
        // // Set I/O mode for pull-up/pull-down
        // IO_IMODE_PORT_PIN(OCM_CLK_PORT, OCM_CLK_PIN, IO_PUD);
        // IO_IMODE_PORT_PIN(OCM_DATA_PORT, OCM_DATA_PIN, IO_PUD);

        // // Set pins to pull-up
        // IO_PUD_PORT(OCM_CLK_PORT, IO_PUP);
        // IO_PUD_PORT(OCM_DATA_PORT, IO_PUP);
    }
}

int8 HalI2CReceive(uint8 address, uint8 *buf, uint16 len) {
    hali2cReceive(address, buf, len);

    return 0;
}

int8 HalI2CSend(uint8 address, uint8 *buf, uint16 len) {
    // begin the write sequence with the address byte
    hali2cSendDeviceAddress(address);
    hali2cSend(buf, len, NOSEND_START, SEND_STOP);

    return 0;
}
/*********************************************************************
 * @fn      hali2cSend
 * @brief   Sends buffer contents to SM-Bus device
 * @param   buffer - ptr to buffered data to send
 * @param   len - number of bytes in buffer
 * @param   sendStart - whether or not to send start condition.
 * @param   sendStop - whether or not to send stop condition.
 * @return  void
 */
STATIC void hali2cSend(uint8 *buffer, uint16 len, uint8 sendStart, uint8 sendStop) {
    uint16 i;
    uint8 retry = HAL_I2C_RETRY_CNT;

    if (!len) {
        return;
    }

    if (sendStart == SEND_START) {
        hali2cStart();
    }

    for (i = 0; i < len; i++) {
        do {
            if (hali2cSendByte(buffer[i])) // takes care of ack polling
            {
                break;
            }
        } while (--retry);
    }

    if (sendStop == SEND_STOP) {
        hali2cStop();
    }
}

/*********************************************************************
 * @fn      hali2cSendByte
 * @brief   Serialize and send one byte to SM-Bus device
 * @param   dByte - data byte to send
 * @return  ACK status - 0=none, 1=received
 */
STATIC _Bool hali2cSendByte(uint8 dByte) {
    uint8 i;

    for (i = 0; i < 8; i++) {
        // Send the MSB
        hali2cWrite(dByte & 0x80);
        // Next bit into MSB
        dByte <<= 1;
    }
    // need clock low so if the SDA transitions on the next statement the
    // slave doesn't stop. Also give opportunity for slave to set SDA
    hali2cClock(0);
    OCM_DATA_HIGH(); // set to input to receive ack...
    hali2cClock(1);
    hali2cWait(1);

    return (!OCM_SDA); // Return ACK status
}

/*********************************************************************
 * @fn      hali2cWrite
 * @brief   Send one bit to SM-Bus device
 * @param   dBit - data bit to clock onto SM-Bus
 * @return  void
 */
STATIC void hali2cWrite(bool dBit) {
    hali2cClock(0);
    hali2cWait(1);
    if (dBit) {
        OCM_DATA_HIGH();
    } else {
        OCM_DATA_LOW();
    }

    hali2cClock(1);
    hali2cWait(1);
}

/*********************************************************************
 * @fn      hali2cClock
 * @brief   Clocks the SM-Bus. If a negative edge is going out, the
 *          I/O pin is set as an output and driven low. If a positive
 *          edge is going out, the pin is set as an input and the pin
 *          pull-up drives the line high. This way, the slave device
 *          can hold the node low if longer setup time is desired.
 * @param   dir - clock line direction
 * @return  void
 */
STATIC void hali2cClock(bool dir) {
    uint8 maxWait = 10;
    if (dir) {
        IO_DIR_PORT_PIN(OCM_CLK_PORT, OCM_CLK_PIN, IO_IN);
        /* Wait until clock is high */
        while (!OCM_SCL && maxWait) {
            hali2cWait(1);
            maxWait -= 1;
        }

    } else {
        IO_DIR_PORT_PIN(OCM_CLK_PORT, OCM_CLK_PIN, IO_OUT);
        OCM_SCL = 0;
    }
    hali2cWait(1);
}

/*********************************************************************
 * @fn      hali2cStart
 * @brief   Initiates SM-Bus communication. Makes sure that both the
 *          clock and data lines of the SM-Bus are high. Then the data
 *          line is set high and clock line is set low to start I/O.
 * @param   void
 * @return  void
 */
STATIC void hali2cStart(void) {
    uint8 retry = HAL_I2C_RETRY_CNT;

    // set SCL to input but with pull-up. if slave is pulling down it will stay down.
    hali2cClock(1);

    do {
        // wait for slave to release clock line...
        if (OCM_SCL) // wait until the line is high...
        {
            break;
        }
        hali2cWait(1);
    } while (--retry);

    // SCL low to set SDA high so the transition will be correct.
    hali2cClock(0);
    OCM_DATA_HIGH(); // SDA high
    hali2cClock(1);  // set up for transition
    hali2cWait(1);
    OCM_DATA_LOW(); // start

    hali2cWait(1);
    hali2cClock(0);
}

/*********************************************************************
 * @fn      hali2cStop
 * @brief   Terminates SM-Bus communication. Waits unitl the data line
 *          is low and the clock line is high. Then sets the data line
 *          high, keeping the clock line high to stop I/O.
 * @param   void
 * @return  void
 */
STATIC void hali2cStop(void) {
    // Wait for clock high and data low
    hali2cClock(0);
    OCM_DATA_LOW(); // force low with SCL low
    hali2cWait(1);

    hali2cClock(1);
    OCM_DATA_HIGH(); // stop condition
    hali2cWait(1);

    hali2cGroudPins();
}

/*********************************************************************
 * @fn      hali2cWait
 * @brief   Wastes a an amount of time.
 * @param   count: down count in busy-wait
 * @return  void
 */
STATIC __near_func void hali2cWait(uint8 count) {
    while (count--) {
        asm("NOP");
    }
}

/*********************************************************************
 * @fn      hali2cReceiveByte
 * @brief   Read the 8 data bits.
 * @param   void
 * @return  character read
 */
STATIC uint8 hali2cReceiveByte() {
    int8 i, rval = 0;

    for (i = 7; i >= 0; --i) {
        if (hali2cRead()) {
            rval |= 1 << i;
        }
    }

    return rval;
}
/**************************************************************************************************
**************************************************************************************************/
/*********************************************************************
 * @fn      hali2cReceive
 * @brief   reads data into a buffer
 * @param   address: linear address on part from which to read
 * @param   buffer: target array for read characters
 * @param   len: max number of characters to read
 * @return  void
 */
STATIC void hali2cReceive(uint8 address, uint8 *buffer, uint16 len) {
    // uint8  ch;
    uint16 i;

    if (!len) {
        return;
    }

    hali2cSendDeviceAddress(address);

    // ch = OCM_ADDRESS_BYTE(0, OCM_READ);
    // hali2cSend(&ch, 1, SEND_START, NOSEND_STOP);

    for (i = 0; i < len - 1; i++) {
        // SCL may be high. set SCL low. If SDA goes high when input
        // mode is set the slave won't see a STOP
        hali2cClock(0);
        OCM_DATA_HIGH();

        buffer[i] = hali2cReceiveByte();
        hali2cWrite(SMB_ACK); // write leaves SCL high
    }

    // condition SDA one more time...
    hali2cClock(0);
    OCM_DATA_HIGH();
    buffer[i] = hali2cReceiveByte();
    hali2cWrite(SMB_NAK);

    hali2cStop();
}

/*********************************************************************
 * @fn      hali2cRead
 * @brief   Toggle the clock line to let the slave set the data line.
 *          Then read the data line.
 * @param   void
 * @return  TRUE if bit read is 1 else FALSE
 */
STATIC _Bool hali2cRead(void) {
    // SCL low to let slave set SDA. SCL high for SDA
    // valid and then get bit
    hali2cClock(0);
    hali2cWait(1);
    hali2cClock(1);
    hali2cWait(1);

    return OCM_SDA;
}

/*********************************************************************
 * @fn      hali2cSendDeviceAddress
 * @brief   Send onlythe device address. Do ack polling
 *
 * @param   void
 * @return  none
 */
STATIC void hali2cSendDeviceAddress(uint8 address) {
    uint8 retry = HAL_I2C_RETRY_CNT;

    do {
        hali2cStart();
        if (hali2cSendByte(address)) // do ack polling...
        {
            break;
        }
    } while (--retry);
}

// http://e2e.ti.com/support/wireless-connectivity/zigbee-and-thread/f/158/t/140917
/*********************************************************************
 * @fn      I2C_ReadMultByte
 * @brief   reads data into a buffer
 * @param   address: linear address on part from which to read
 * @param   reg: internal register address on part read from
 * @param   buffer: target array for read characters
 * @param   len: max number of bytes to read
 */
int8 I2C_ReadMultByte(uint8 address, uint8 reg, uint8 *buffer, uint16 len) {
    uint16 i = 0;
    uint8 _address = 0;

    if (!len) {
        return I2C_ERROR;
    }

    /* Send START condition */
    hali2cStart();

    /* Set direction of transmission */
    // Reset the address bit0 for write
    // _address &= OCM_WRITE;

    _address = ((address << 1) | OCM_WRITE);

    /* Send Address  and get acknowledgement from slave*/
    if (!hali2cSendByte(_address)) {
        return I2C_ERROR;
    }

    /* Send internal register to read from to */
    if (!hali2cSendByte(reg))
        return I2C_ERROR;

    /* Send RESTART condition */
    hali2cStart();

    /* Set direction of transmission */
    // Reset the address bit0 for read
    // _address |= OCM_READ;
    _address = ((address << 1) | OCM_READ);
    /* Send Address  and get acknowledgement from slave*/
    if (!hali2cSendByte(_address))
        return I2C_ERROR;

    while (len) {
        // SCL may be high. set SCL low. If SDA goes high when input
        // mode is set the slave won't see a STOP
        hali2cClock(0);
        OCM_DATA_HIGH();
        buffer[i] = hali2cReceiveByte();
        // Acknowledgement if not sending last byte
        if (len > 1) {
            hali2cWrite(SMB_ACK); // write leaves SCL high
        }

        // increment buffer register
        i++;
        // Decrement the read bytes counter
        len--;
    }

    // condition SDA one more time...
    hali2cClock(0);
    OCM_DATA_HIGH();
    hali2cWrite(SMB_NAK);

    hali2cStop();

    // condition SDA one more time...
    //   hali2cClock(0);
    //   OCM_DATA_HIGH();
    //   buffer[i] = hali2cReceiveByte();
    //   hali2cWrite(SMB_NAK);

    //   hali2cStop();
    return I2C_SUCCESS;
}

/*********************************************************************
 * @fn      I2C_WriteMultByte
 * @brief   reads data into a buffer
 * @param   address: linear address on part from which to read
 * @param   reg: internal register address on part read from
 * @param   buffer: target array for read characters
 * @param   len: max number of bytes to read
 */
int8 I2C_WriteMultByte(uint8 address, uint8 reg, uint8 *buffer, uint16 len) {
    uint16 i = 0;
    uint8 _address = 0;

    if (!len) {
        return I2C_ERROR;
    }

    /* Send START condition */
    hali2cStart();
    // return I2C_ERROR;

    /* Set direction of transmission */
    // Reset the address bit0 for write
    // _address &= OCM_WRITE;
    _address = ((address << 1) | OCM_WRITE);

    /* Send Address  and get acknowledgement from slave*/
    if (!hali2cSendByte(_address))
        return I2C_ERROR;

    /* Send internal register to read from to */
    if (!hali2cSendByte(reg))
        return I2C_ERROR;

    /* Write data into register */
    // read bytes of data into buffer
    while (len) {
        // SCL may be high. set SCL low. If SDA goes high when input
        // mode is set the slave won't see a STOP
        hali2cClock(0);
        OCM_DATA_HIGH();

        /* Send Address  and get acknowledgement from slave*/
        if (!hali2cSendByte(buffer[i]))
            return I2C_ERROR;

        // increment buffer register
        i++;
        // Decrement the read bytes counter
        len--;
    }

    hali2cStop();
    return I2C_SUCCESS;
}