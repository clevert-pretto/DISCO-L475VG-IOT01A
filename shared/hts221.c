#include "my_stm32_map.h"
#include "uart.h"
#include "i2c.h"

// --- HTS221 Specific Logic ---
#define HTS221_ADDR 0x5F

/**
 * Reads a 16-bit value from HTS221.
 * HTS221 requires setting the MSB of the register address to 1 
 * for auto-incrementing during multi-byte reads.
 */
int16_t hts221_read_raw16(uint8_t reg) {
    uint8_t addr = reg | 0x80; 
    uint32_t timeout = 10000;
    
    // Phase 1: Point to register
    I2C2->CR2 = (HTS221_ADDR << 1) | (1UL << 16) | (1UL << 13);
    
    while (!(I2C2->ISR & (1UL << 1))) {
        if (--timeout == 0) return 0; // Exit if hardware hangs
    }
    I2C2->TXDR = addr;
    
    timeout = 10000;
    while (!(I2C2->ISR & (1UL << 6))) {
        if (--timeout == 0) return 0;
    }

    // Phase 2: Read 2 bytes
    I2C2->CR2 = (HTS221_ADDR << 1) | (1UL << 10) | (2UL << 16) | (1UL << 13) | (1UL << 25);
    
    // Wait for first byte
    timeout = 10000;
    while (!(I2C2->ISR & (1UL << 2))) { if (--timeout == 0) return 0; }
    uint8_t low = I2C2->RXDR;
    
    // Wait for second byte
    timeout = 10000;
    while (!(I2C2->ISR & (1UL << 2))) { if (--timeout == 0) return 0; }
    uint8_t high = I2C2->RXDR;
    
    return (int16_t)((high << 8) | low);
}

float hts221_get_temperature(void) {
    // 1. Power up and set Output Data Rate to 1Hz
    i2c2_write_reg(HTS221_ADDR, 0x20, 0x81); 

    // 2. Read Factory Calibration Temperatures (T0, T1)
    uint16_t t0_c_8 = i2c2_read_reg(HTS221_ADDR, 0x32);
    uint16_t t1_c_8 = i2c2_read_reg(HTS221_ADDR, 0x33);
    uint8_t t_msb = i2c2_read_reg(HTS221_ADDR, 0x35);
    
    // T0 and T1 are 10-bit values stored as (degC * 8)
    float T0 = (float)((t_msb & 0x03) << 8 | t0_c_8) / 8.0f;
    float T1 = (float)((t_msb & 0x0C) << 6 | t1_c_8) / 8.0f;

    // 3. Read Calibration Raw ADC Counts (MS0, MS1)
    int16_t T0_out = hts221_read_raw16(0x3C);
    int16_t T1_out = hts221_read_raw16(0x3E);

    // 4. Read Current Raw Data
    int16_t T_out = hts221_read_raw16(0x2A);

    // 5. Linear Interpolation
    float denominator = (float)(T1_out - T0_out);
    if (denominator == 0.0f) 
    {
        return 0.0f; // Prevent crash
    }
    return ((T1 - T0) * (float)(T_out - T0_out)) / denominator + T0;
}