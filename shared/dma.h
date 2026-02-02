#ifndef DMA_H
#define DMA_H
#include <stdint.h>

void dma1_ch4_uart1_tx_init(void);
void dma1_uart1_send(const char* data, uint16_t length);

#endif