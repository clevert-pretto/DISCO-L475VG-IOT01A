#include "my_stm32_map.h"
#include "uart.h"
#include "i2c.h"
#include "hts221.h"

#define HTS221_ADDR 0x5F

// --- Global Timing & State ---
volatile uint32_t msTicks = 0;

void SysTick_Handler(void) {
    msTicks++; 
    // Toggle every 500ms
    if (msTicks % 500 == 0) {
        GPIOB_ODR ^= (1UL << LED_PIN); 
    }
}

uint32_t now = 0;
// --- Main Execution ---
int main(void) {
    
    /* 1. System Clock Setup: Switch to HSI16 (16MHz) */
    RCC_CR |= (1UL << 8);            // Enable HSI16
    while (!(RCC_CR & (1UL << 10))); // Wait for ready
    
    
    /* Switch System Clock to HSI16 */
    RCC_CFGR &= ~3UL;
    RCC_CFGR |= 1UL; 
    while(((RCC_CFGR >> 2) & 0x3) != 0x1); // Wait for HSI16 to be system clock
    
    /* 2. Peripheral Initialization */
    uart_init(115200);               // Initialize VCP UART
    i2c2_init();                     // Initialize I2C2 for sensors
    
    SYSTICK->LOAD = 15999;           // 1ms ticks @ 16MHz
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = 0x07;            // Enable with Interrupts

    /* 3. Hardware Setup (PB14 LED) */
    RCC_AHB2ENR |= GPIOB_EN;         // Enable GPIOB Clock
    GPIOB_MODER &= ~(3UL << (LED_PIN * 2));
    GPIOB_MODER |=  (1UL << (LED_PIN * 2)); // PB14 as Output

    uart_send_string("\r\n--- Project 04: HTS221 Temperature Monitor Online ---\r\n");
    
    /* 4. Sensor Verification */
    uint8_t id = i2c2_read_reg(HTS221_ADDR, 0x0F); // WHO_AM_I
    if (id != 0xBC) {
        uart_send_string("Error: HTS221 not detected!\r\n");
    } else {
        uart_send_string("HTS221 Sensor Linked (0xBC)\r\n");
    }

    uint32_t next_read = 0;

    while(1) {
        // Task: Read and display temperature every 1000ms
        if (msTicks >= next_read) {
            next_read = msTicks + 1000;
            
            // 1. Fetch live data from the sensor
            float temp = hts221_get_temperature();
            
            uart_send_string("Current temperature : ");

            // 2. Separate the float into whole and fractional parts
            // This avoids using complex 'sprintf' floating-point libraries
            int32_t whole = (int32_t)temp;
            int32_t frac = (int32_t)((temp - (float)whole) * 100.0f);
            
            // Handle negative fractional values
            if (frac < 0) frac = -frac;

            uart_send_number(whole);
            uart_send_char('.');
            
            // Critical: Add leading zero if fraction is less than 10 (e.g., .05)
            if (frac < 10) uart_send_char('0');
            uart_send_number(frac);            
            uart_send_string(" C\r\n");
        }
    }
}