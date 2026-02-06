/* shared/uart.h */
#ifndef UART_H
#define UART_H

#include <stdint.h>

// Professional API: Initialize with a baud rate
void uart_init(uint32_t baudrate);

// Basic I/O
void uart_send_char(char c);
void uart_send_string(const char *str);
void uart_send_number(int32_t num);
#endif