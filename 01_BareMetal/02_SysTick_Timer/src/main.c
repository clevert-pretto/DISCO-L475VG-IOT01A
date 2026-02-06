#include "../shared/my_stm32_map.h"

volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void) {
    ms_ticks++;
}

void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms);
}

int main(void) {
    // 1. Clock Enable for GPIOB
    RCC_AHB2ENR |= (1UL << 1); 
    
    // 2. PB14 Output Mode
    GPIOB_MODER &= ~(3UL << (14 * 2));
    GPIOB_MODER |=  (1UL << (14 * 2));

    // 3. Init SysTick: 4MHz clock / 1000 = 4000 ticks for 1ms
    SYSTICK->LOAD = 4000 - 1;
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = (1UL << 2) | (1UL << 1) | (1UL << 0);

    while(1) {
        GPIOB_ODR ^= (1UL << 14); // Toggle LED
        delay_ms(500);           // Exact 1 second delay
    }
}