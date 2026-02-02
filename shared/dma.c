#include "my_stm32_map.h"
#include "dma.h"


volatile uint8_t dma_tx_complete = 1; // Flag to track status

void DMA1_CH4_IRQHandler (void){
    // Check if the Transfer Complete flag is set for Channel 4
    if (DMA1_ISR & (1UL << 13)) { 
        // 1. Clear the flag in the Interrupt Flag Clear Register (IFCR)
        DMA1_IFCR = (1UL << 13);
        
        // 2. Mark the transfer as finished
        dma_tx_complete = 1;
        
        // 3. Optional: Toggle an LED for visual confirmation
         GPIOB_ODR ^= (1UL << 14); 
    }
}


void dma1_ch4_uart1_tx_init(void) {
    
    /* 1. Enable DMA1 Clock */
    RCC_AHB1ENR |= (1UL << 0);

    /* 2. Map USART1_TX to DMA1 Channel 4 */
    // DMA1_CSELR: Bits 12-15 control Channel 4. Value 0010 = USART1_TX
    DMA1_CSELR &= ~(0xFUL << 12);
    DMA1_CSELR |=  (2UL << 12);

    /* 3. Configure Channel 4 Control Register (CCR) */
    DMA1_CH4_CCR &= ~(1UL << 0); // Disable channel to configure

    // Priority: Low (00), Size: 8-bit (00), MINC (7)=1, DIR (4)=1 (Mem-to-Periph)
    // TCIE (1)=1 (Transfer Complete Interrupt Enable)
    DMA1_CH4_CCR |= ((1 << 7) | (1 << 4) | (1 << 1));

    /* 4. Set Addresses */
    DMA1_CH4_CPAR =(uint32_t) &USART1->TDR;

    /* 5. Prepare UART for DMA */
    USART1->CTRL3 |= (1 << 7); // Enable DMAT bit in UART Control Register

}

/**
 * Trigger a DMA transfer for a block of memory
 */
void dma1_uart1_send(const char* data, uint16_t length) {
    // 1. Force Disable the channel (just in case)
    DMA1_CH4_CCR &= ~(1UL << 0);

    // 2. Wait for the EN bit to actually clear in hardware
    while (DMA1_CH4_CCR & (1UL << 0)); 

    // 3. Clear ALL flags for Channel 4 (Bits 12, 13, 14, 15)
    // Writing 1 to IFCR bits 12-15 clears Global, TC, HT, and TE flags.
    DMA1_IFCR = (0xFUL << 12); // Global flag
    //DMA1_IFCR = (1UL << 13); // Transfer Complete flag

    // 4. Mark as busy for main loop synchronization
    dma_tx_complete = 0;
    
    // 5. Update Memory Address and Length
    DMA1_CH4_CMAR = (uint32_t)data;   // Source: RAM buffer
    DMA1_CH4_CNDTR = length;         // Number of data items

    // 6. Final check: Ensure the UART TX register is empty before starting
    // This prevents the DMA from starting while the UART is still busy from a previous manual send
    while(!(USART1->ISR & (1UL << 7)));

    // 7. Re-enable channel to start transfer
    DMA1_CH4_CCR |= (1UL << 0);      // Enable channel to start transfer
}