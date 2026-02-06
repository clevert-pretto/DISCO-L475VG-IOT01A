/* 05_DMA_Acceleration/src/main.c */
#include "dma.h"
#include "i2c.h"
#include "uart.h"
#include "hts221.h"
#include "my_stm32_map.h"

extern volatile uint8_t dma_tx_complete;
volatile uint32_t msTicks = 0;
void SysTick_Handler(void) { 
    msTicks++; 
    if (msTicks % 500 == 0) GPIOB_ODR ^= (1UL << LED_PIN); // Heartbeat
}

// Global buffer for DMA safety (DMA cannot access local stack variables easily)
char dma_buffer[64];

/**
 * Simple helper to format temperature into a string without sprintf
 */
int format_temp_msg(char* buf, float temp) {
    int32_t whole = (int32_t)temp;
    int32_t frac = (int32_t)((temp - (float)whole) * 100.0f);
    if (frac < 0) frac = -frac;

    // Manual string construction: "Temp: XX.YY C\r\n"
    // (In a real project, you'd use a lightweight custom snprintf)
    buf[0] = 'T'; buf[1] = 'e'; buf[2] = 'm'; buf[3] = 'p'; buf[4] = ':'; buf[5] = ' ';
    
    // Simplistic 2-digit whole number conversion
    buf[6] = (whole / 10) + '0';
    buf[7] = (whole % 10) + '0';
    buf[8] = '.';
    buf[9] = (frac / 10) + '0';
    buf[10] = (frac % 10) + '0';
    buf[11] = ' '; buf[12] = 'C'; buf[13] = '\r'; buf[14] = '\n'; buf[15] = '\0';
    
    return 15; // Length of the string
}

int main(void) {
    // 1. Hardware Init
    RCC_CR |= (1UL << 8); 
    while (!(RCC_CR & (1UL << 10)));

    RCC_CFGR &= ~3UL;
     RCC_CFGR |= 1UL;
    while(((RCC_CFGR >> 2) & 0x3) != 0x1);

    uart_init(115200);
    i2c2_init();
    dma1_ch4_uart1_tx_init(); //

    SYSTICK->LOAD = 15999;
    SYSTICK->CTRL = 0x07;

    // 2. Enable DMA Interrupt in NVIC (IRQ 14)
    // NVIC_ISER0 bit 14 corresponds to DMA1_CH4
    NVIC_ISER0 |= (1UL << 14); 

    // 3. Hardware Setup (PB14 LED) */
    RCC_AHB2ENR |= GPIOB_EN;         // Enable GPIOB Clock
    GPIOB_MODER &= ~(3UL << (LED_PIN * 2));
    GPIOB_MODER |=  (1UL << (LED_PIN * 2)); // PB14 as Output

    uart_send_string(" -- 05. DMA Acceleration Engine Online --\r\n");

    uint32_t next_tx = 0;

    while(1) {
        if (msTicks >= next_tx) {
            next_tx = msTicks + 1000;

            while(!dma_tx_complete);
            // 3. CPU does the heavy math
            
            float temp = hts221_get_temperature();
            int len = format_temp_msg(dma_buffer, temp);

            // 4. Trigger DMA
            // This returns instantly. The CPU is free while the LED blinks.
            dma1_uart1_send(dma_buffer, len);
        }
    }
}