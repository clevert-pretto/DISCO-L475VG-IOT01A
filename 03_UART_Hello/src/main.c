#include "my_stm32_map.h"
#include "uart.h"

volatile uint32_t msTicks = 0;
volatile uint8_t  rx_event = 0;

void SysTick_Handler(void) { msTicks++; }

void USART1_IRQHandler(void) {
    if (USART1->ISR & (1 << 5)) {
        char data = (char)(USART1->RDR & 0xFF);
        uart_send_char(data); // Echo back via HAL
        rx_event = 1;         // Trigger the double-blink task
    }
    if (USART1->ISR & (1 << 3)) USART1->ICR = (1 << 3); // Clear Overrun
}

// Simple task for the 250ms double blink
void perform_rx_indicator(void) {
    for (int i = 0; i < 2; i++) {
        GPIOB_ODR |= (1UL << 14);
        uint32_t t = msTicks + 125;
        while(msTicks < t);
        GPIOB_ODR &= ~(1UL << 14);
        t = msTicks + 125;
        while(msTicks < t);
    }
}

int main(void) {
    /* 1. System Hardware Setup */
    SYSTICK->LOAD = 15999; 
    SYSTICK->CTRL = 0x07; //

    /* 2. Initialize HAL with professional API */
    uart_init(115200);

    /* 2. Switch System Clock (SYSCLK) to HSI16 */
    // RCC_CFGR (Offset 0x08): Bits 0-1 set the SW (System clock switch)
    // 01: HSI16 selected as system clock
    RCC_CFGR &= ~3UL;   // Clear bits
    RCC_CFGR |= 1UL;    // Select HSI16 (01)
    /* 3. Enable Interrupt in NVIC */
    NVIC_ISER1 = (1UL << 5); // USART1 IRQ enable

    /* 4. Heartbeat LED Setup (PB14) */
    RCC_AHB2ENR |= (1UL << 1); 
    GPIOB_MODER &= ~(3UL << (LED_PIN * 2));
    GPIOB_MODER |= (1UL << (LED_PIN * 2));

    uint32_t last_heartbeat = 0;
    uint32_t last_tx_a = 0;

    uart_send_string("\r\n--- BProject 03: UART Hello ---\r\n");

    while(1) {
        // Task: 500ms Periodic Heartbeat
        if (msTicks - last_heartbeat >= 500) {
            GPIOB_ODR ^= (1UL << 14);
            last_heartbeat = msTicks;
        }

        // Task: 1s Periodic Telemetry
        if (msTicks - last_tx_a >= 1000) {
            uart_send_char('A');
            last_tx_a = msTicks;
        }

        // Task: Asynchronous RX Response
        if (rx_event) {
            perform_rx_indicator();
            rx_event = 0;
        }
    }
}