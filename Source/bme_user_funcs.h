#ifndef bme_user_funcs_h
#define bme_user_funcs_h
extern void user_delay_ms(uint32_t period);
extern int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
extern int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
#endif