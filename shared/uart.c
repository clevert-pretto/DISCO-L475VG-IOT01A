/* shared/uart.c */
#include "my_stm32_map.h"
#include "uart.h"

void uart_init(uint32_t baudrate) {
    /* 1. Clock Configuration */
    RCC_CR |= (1UL << 8); // Enable HSI16
    while (!(RCC_CR & (1UL << 10))); // Wait for ready

    // Select HSI16 for USART1 in CCIPR
    RCC_CCIPR &= ~(3UL << 0);
    RCC_CCIPR |=  (2UL << 0);

    /* 2. Enable Clocks (GPIOB and USART1) */
    RCC_AHB2ENR |= (1UL << 1);  // Enable GPIOB
    RCC_APB2ENR |= (1UL << 14); // Enable USART1 (Note: USART1 is on APB2)

    /* 2. Pin Muxing (PA2, PA3) */
    GPIOB_MODER &= ~((3UL << (VCP_TX_PIN * 2)) | (3UL << (VCP_RX_PIN * 2)));
    GPIOB_MODER |=  ((2UL << (VCP_TX_PIN * 2)) | (2UL << (VCP_RX_PIN * 2)));
    
    GPIOB_AFRL &= ~((0xFUL << (VCP_TX_PIN * 4)) | (0xFUL << (VCP_RX_PIN * 4)));
    GPIOB_AFRL |=  ((0x7UL << (VCP_TX_PIN * 4)) | (0x7UL << (VCP_RX_PIN * 4)));

    /* 3. Baud Rate Calculation */
    // Logic: 16,000,000 / baudrate
    USART1->CTRL1 = 0; 
    USART1->BRR = (16000000UL / baudrate);
    USART1->CTRL1 = (1 << 5) |(1 << 3) | (1 << 2) | (1 << 0); // TE, RE and UE

    while(!(USART1->ISR & (1UL << 21))); // Wait for TEACK
}

void uart_send_char(char c) {
    while(!(USART1->ISR & (1UL << 7)));
    USART1->TDR = c;
}

void uart_send_string(const char *str) {
    while(*str) uart_send_char(*str++);
}

/* Converts an integer to a string and sends it via UART */
void uart_send_number(int32_t num) {
    char buffer[11]; // Max length for a 32-bit int + null
    int i = 0;

    // Handle 0 explicitly
    if (num == 0) {
        uart_send_char('0');
        return;
    }

    // Handle negative numbers
    if (num < 0) {
        uart_send_char('-');
        num = -num;
    }

    // Extract digits in reverse order
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    // Send digits in correct order (reverse the buffer)
    while (i > 0) {
        uart_send_char(buffer[--i]);
    }
}