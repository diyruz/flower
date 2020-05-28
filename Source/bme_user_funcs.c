#include "bme_user_funcs.h"
#include "hal_i2c.h"

void user_delay_ms(uint32_t period) {
   HAL_BOARD_DELAY_USEC(period / 1000);
}

int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len){
    return I2C_ReadMultByte(dev_id, reg_addr, &reg_data, len)
}

int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len) {
    return I2C_WriteMultByte(dev_id, reg_addr, &reg_data, len);
}