#include "my_stm32_map.h"
#include "spi.h"
#include "uart.h"

volatile uint32_t msTicks = 0;
void SysTick_Handler(void) { 
    msTicks++; 
    if (msTicks % 500 == 0) GPIOB_ODR ^= (1UL << LED_PIN); // Heartbeat
}

void delay_ms(uint32_t ms) {
    uint32_t startTick = msTicks;
    while ((msTicks - startTick) < ms); // Precise wait based on 1ms interrupt
}

int main (void)
{
     // 1. Hardware Init
    RCC_CR |= (1UL << 8); 
    while (!(RCC_CR & (1UL << 10)));

    RCC_CFGR &= ~3UL;
     RCC_CFGR |= 1UL;
    while(((RCC_CFGR >> 2) & 0x3) != 0x1);

    uart_init(115200);
    spi3_init();
    SYSTICK->LOAD = 15999;
    SYSTICK->VAL = 0;
    SYSTICK->CTRL = 0x07;

    // 3. Hardware Setup (PB14 LED) */
    RCC_AHB2ENR |= GPIOB_EN;         // Enable GPIOB Clock
    GPIOB_MODER &= ~(3UL << (LED_PIN * 2));
    GPIOB_MODER |=  (1UL << (LED_PIN * 2)); // PB14 as Output

    uart_send_string("\r\n-- 07-SPI BLE Verification --\r\n");

    // 1. Release Bluetooth Reset (PA8)
    GPIOA_ODR |= (1UL << BLE_RST); //
    delay_ms(200);   // Short delay for boot

    // 2. Chip Select LOW to start communication
    GPIOE_ODR &= ~(1UL << SPI3_BLE_CSN_PIN);

    // 3. Send a test command (Read Device Version)
    // Note: HCI format often requires a header byte first
    spi3_transfer(0x0B); // BlueNRG Read Header
    uint16_t version = spi3_transfer(0x00); // Dummy byte to clock out version

    // 4. Wait for SPI to finish and Chip Select HIGH
    while(SPI3->SR & (1UL << 7)); // Wait for BSY bit to clear
    GPIOE_ODR |= (1UL << SPI3_BLE_CSN_PIN); //

    if (version != 0 && version != 0xFF) {
        uart_send_string("SPI Communication Verified, version is = ");
        uart_send_number(version);
        uart_send_string("\r\n");
    }

    while(1);
}