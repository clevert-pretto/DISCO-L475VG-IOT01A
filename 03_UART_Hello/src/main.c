#include "my_stm32_map.h"

// Task Timing Counters
volatile uint32_t msTicks = 0;
volatile uint32_t rx_blink_trigger = 0; // Set to 1 when char received

void SysTick_Handler(void) {
    msTicks++;
}

// USART1 Interrupt Handler
// Ensure "USART1_Handler" name matches your vector table in startup.c
void USART1_IRQHandler(void) {
    // Check if RXNE (Read Data Register Not Empty) bit is set
    if (USART1->ISR & (1 << 5)) {
        char received = (char)(USART1->RDR & 0xFF);
        
        // 1. Loopback: Send character back immediately
        while(!(USART1->ISR & (1 << 7))); // Wait for TXE
        USART1->TDR = received;

        // 2. Trigger the "Double Blink" flag
        rx_blink_trigger = 1; 
    }

    // Clear Overrun error if it occurs to prevent UART lockup
    if (USART1->ISR & (1 << 3)) {
        USART1->ICR |= (1 << 3);
    }
}

void uart_send_char(char c) {
    while(!(USART1->ISR & (1 << 7))); // Wait for TXE
    USART1->TDR = c;
}

int main(void) {
    /* 1. Clock & SysTick (16MHz) */
    RCC_CR |= (1 << 8); 
    while (!(RCC_CR & (1 << 10))); 

    /* 2. Select HSI16 for USART1 in CCIPR (Bits 0:1) */
    RCC_CCIPR &= ~(3UL << 0);
    RCC_CCIPR |=  (2UL << 0);

    SYSTICK->LOAD = (16000000 / 1000) - 1; 
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = (1 << 0) | (1 << 1) | (1 << 2);

    /* 2. Enable Clocks (GPIOB and USART1) */
    RCC_AHB2ENR |= (1UL << 1);  // Enable GPIOB
    RCC_APB2ENR |= (1UL << 14); // Enable USART1 (Note: USART1 is on APB2)

    /* 3. Pin Muxing (PB6/PB7 to AF7) */
    GPIOB_MODER &= ~((3UL << (VCP_TX_PIN * 2)) | (3UL << (VCP_RX_PIN * 2)));
    GPIOB_MODER |=  ((2UL << (VCP_TX_PIN * 2)) | (2UL << (VCP_RX_PIN * 2)));
    
    // AF7 is 0111 (0x7). PB6/7 are in AFRL (bits 24-31)
    GPIOB_AFRL &= ~((0xFUL << (VCP_TX_PIN * 4)) | (0xFUL << (VCP_RX_PIN * 4)));
    GPIOB_AFRL |=  ((0x7UL << (VCP_TX_PIN * 4)) | (0x7UL << (VCP_RX_PIN * 4)));

    /* 4. LED Config: PB14 */
    GPIOB_MODER &= ~(3UL << (LED_PIN * 2));
    GPIOB_MODER |=  (1UL << (LED_PIN * 2));

    /* 4. USART1 Configuration */
    USART1->BRR = 139; // 115200 Baud @ 16MHz
    USART1->CTRL1 = (1 << 5) |(1 << 3) | (1 << 2) | (1 << 0); // TE, RE and UE

    /* 5. Enable USART1 Interrupt in NVIC (Nested Vectored Interrupt Controller)
     USART1 is IRQ 37 on STM32L475
     NVIC->ISER[1] manages IRQs 32-63. 37 - 32 = bit 5. */
    NVIC_ISER1 = (1 << 5);

    uint32_t last_heartbeat = 0;
    uint32_t last_tx_a = 0;

    while(1) {

        uint32_t current_time = msTicks;

        // Spec 1: Blink LED every 500ms (Heartbeat)
        if (current_time - last_heartbeat >= 500) {
            GPIOB_ODR ^= (1UL << 14); 
            last_heartbeat = current_time;
        }

        // Spec 4: Transmit 'A' every 1 second
        if (current_time - last_tx_a >= 1000) {
            uart_send_char('A');
            last_tx_a = current_time;
        }

        // Spec 2: Blink LED twice every 250ms when Character received
        if (rx_blink_trigger) {
            for (int i = 0; i < 2; i++) {
                GPIOB_ODR |= (1UL << LED_PIN); // ON
                uint32_t wait = msTicks + 125;
                while(msTicks < wait); 
                
                GPIOB_ODR &= ~(1UL << LED_PIN); // OFF
                wait = msTicks + 125;
                while(msTicks < wait);
            }
            rx_blink_trigger = 0; // Reset trigger
        }
    }
}