#include "my_stm32_map.h"

void delay(volatile uint32_t count) {
    while(count--) {
        __asm("nop"); // Prevent compiler from optimizing out the loop
    }
}

int main(void) {
    // 1. Enable Clock for GPIOB (Bit 1 in AHB2ENR)
    RCC_AHB2ENR |= GPIOB_EN;

    // 2. Set PB14 as Output
    // MODER register has 2 bits per pin. Pin 14 is bits 29:28.
    // Setting 01 = General purpose output mode.
    GPIOB_MODER &= ~(3UL << (LED_PIN * 2)); // Clear bits 28, 29
    GPIOB_MODER |=  (1UL << (LED_PIN * 2)); // Set bit 28 to 1

    while(1) {
        // 3. Toggle PB14 using ODR (Output Data Register)
        GPIOB_ODR ^= (1UL << LED_PIN);
        
        delay(125000);
    }

    return 0; // Should never reach here
}