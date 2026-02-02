/* shared/i2c.c */
#include "my_stm32_map.h"
#include "i2c.h"

void i2c2_init(void) {
    /* 1. Explicitly select HSI16 as the I2C2 Clock Source */
    // RCC_CCIPR: Bits 22-23 for I2C2. 10 = HSI16 selected.
    RCC_CCIPR &= ~(3UL << 22);
    RCC_CCIPR |=  (2UL << 22);

    /* 2. Enable Clocks */
    RCC_AHB2ENR  |= (1UL << 1);   // GPIOB
    RCC_APB1ENR1 |= (1UL << 22);  // I2C2

    /* 3. GPIO Muxing (AF4 for PB10/11) */
    GPIOB_MODER &= ~((3UL << 20) | (3UL << 22));
    GPIOB_MODER |=  ((2UL << 20) | (2UL << 22)); 
    
    // CRITICAL: Open-Drain is mandatory for I2C
    GPIOB_OTYPER |= (1UL << 10) | (1UL << 11);

    // Use AFRH for pins 10 and 11
    GPIOB_AFRH &= ~((0xFUL << 8) | (0xFUL << 12)); 
    GPIOB_AFRH |=  ((4UL << 8) | (4UL << 12));

    /* 4. Timing Config for 16MHz HSI (100kHz) */
    I2C2->CR1 &= ~(1UL << 0); // PE=0
    I2C2->TIMINGR = (3UL << 28) | (4UL << 20) | (2UL << 16) | (0xFUL << 8) | 0x13UL;
    I2C2->CR1 |= (1UL << 0); // PE=1
}

uint8_t i2c2_read_reg(uint8_t dev_addr, uint8_t reg_addr) {
    // Phase 1: Write Register Address
    I2C2->CR2 = (dev_addr << 1) | (1UL << 16) | (1UL << 13);
    
    // Diagnostic Timeout
    uint32_t timeout = 10000;
    while (!(I2C2->ISR & (1UL << 1))) { // Wait for TXIS
        if (--timeout == 0) return 0xEE; // Return Error Code if stuck
    }
    I2C2->TXDR = reg_addr;
    
    timeout = 10000;
    while (!(I2C2->ISR & (1UL << 6))) { // Wait for TC
        if (--timeout == 0) return 0xEF;
    }

    // Phase 2: Read Data
    I2C2->CR2 = (dev_addr << 1) | (1UL << 10) | (1UL << 16) | (1UL << 13) | (1UL << 25);
    
    timeout = 10000;
    while (!(I2C2->ISR & (1UL << 2))) { // Wait for RXNE
        if (--timeout == 0) return 0xF1;
    }
    
    return (uint8_t)I2C2->RXDR;
}

/**
 * Basic Register Write: START -> ADDR+W -> REG -> DATA -> STOP
 */
void i2c2_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    uint32_t timeout = 10000;
    I2C2->CR2 = (dev_addr << 1) | (2UL << 16) | (1UL << 13) | (1UL << 25);

    while (!(I2C2->ISR & (1UL << 1))) { // TXIS
        if (--timeout == 0) return; 
    }
    I2C2->TXDR = reg_addr;

    timeout = 10000;
    while (!(I2C2->ISR & (1UL << 1))) { // TXIS
        if (--timeout == 0) return;
    }
    I2C2->TXDR = data;

    timeout = 10000;
    while (!(I2C2->ISR & (1UL << 6))) { // TC
        if (--timeout == 0) return;
    }
}