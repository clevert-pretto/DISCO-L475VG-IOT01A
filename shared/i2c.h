/* shared/i2c.h */
#ifndef I2C_H
#define I2C_H

#include <stdint.h>

/* Standard API for I2C2 transactions */
void i2c2_init(void);
void i2c2_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
uint8_t i2c2_read_reg(uint8_t dev_addr, uint8_t reg_addr);

#endif