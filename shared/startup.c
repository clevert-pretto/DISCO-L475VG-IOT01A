#include <stdint.h>
#include "my_stm32_map.h"

// These symbols come from the Linker Script (.ld)
extern uint32_t _estack;  // Top of RAM (Stack)
extern uint32_t _sdata;   // Start of .data in RAM
extern uint32_t _edata;   // End of .data in RAM
extern uint32_t _sidata;  // Start of .data in FLASH (the values)
extern uint32_t _sbss;    // Start of .bss in RAM
extern uint32_t _ebss;    // End of .bss in RAM

// 0. Declaration of the main function
extern int main(void);

// 1. Define the handler first
void Default_Handler(void) {// Enable GPIOB for LED if not already enabled
    RCC_AHB2ENR |= (1UL << 1); 
    // Set PB14 to Output
    GPIOB_MODER &= ~(3UL << (2 * LED_PIN));
    GPIOB_MODER |=  (1UL << (2 * LED_PIN));

    while(1) {
        // Rapid Blink = System Error / Crash
        GPIOB_ODR ^= (1UL << 14);
        for(volatile int i = 0; i < 200000; i++); 
    }
}

// 2. Now define the aliases (Ensure the string "Default_Handler" matches exactly)
void NMI_Handler(void)          __attribute__ ((weak, alias ("Default_Handler")));
void HardFault_Handler(void)    __attribute__ ((weak, alias ("Default_Handler")));
void MemManage_Handler(void)    __attribute__ ((weak, alias ("Default_Handler")));
void BusFault_Handler(void)     __attribute__ ((weak, alias ("Default_Handler")));
void UsageFault_Handler(void)   __attribute__ ((weak, alias ("Default_Handler")));
void SVC_Handler(void)          __attribute__ ((weak, alias ("Default_Handler")));
void DebugMon_Handler(void)     __attribute__ ((weak, alias ("Default_Handler")));
void PendSV_Handler(void)       __attribute__ ((weak, alias ("Default_Handler")));
void USART1_IRQHandler(void)    __attribute__ ((weak, alias ("Default_Handler")));

// 3. SysTick is special—we want to define it in main.c
// Add this line to provide a "safety net" for the SysTick_Handler
void SysTick_Handler(void)      __attribute__ ((weak, alias ("Default_Handler")));
void DMA1_CH4_IRQHandler(void)  __attribute__ ((weak, alias ("Default_Handler")));
void EXTI15_10_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));

void Reset_Handler(void) {
    
    // CPACR is at 0xE000ED88. Set bits 20-23 to enable CP10 and CP11
    CPACR |= (0xFUL << 20);
    __asm("DSB"); // Data Synchronization Barrier
    __asm("ISB"); // Instruction Synchronization Barrier
    
    // 1. Copy .data section from FLASH to RAM
    uint32_t size = (uint32_t)&_edata - (uint32_t)&_sdata;
    uint8_t *pDst = (uint8_t *)&_sdata;      // RAM
    uint8_t *pSrc = (uint8_t *)&_sidata;     // FLASH

    for (uint32_t i = 0; i < size; i++) {
        *pDst++ = *pSrc++;
    }

    // 2. Initialize .bss section to zero in RAM
    size = (uint32_t)&_ebss - (uint32_t)&_sbss;
    pDst = (uint8_t *)&_sbss;
    for (uint32_t i = 0; i < size; i++) {
        *pDst++ = 0;
    }

    // 3. Jump to main
    main();

    while(1); // Should never get here
}

// The Vector Table
// Place it in the ".isr_vector" section as defined in linker.ld
__attribute__((section(".isr_vector")))
uint32_t *vector_table[] = {
    (uint32_t *)&_estack,       // 0: Initial Stack Pointer
    (uint32_t *)Reset_Handler,   // 1: Reset Vector
    [2 ... 14] = 0, // 2-14: Exceptions
    (uint32_t *)SysTick_Handler, // 15: SysTick Timer (Offset 0x3C)

    /* --- External Interrupts (IRQs) start here --- */
    [16 ... 29] = 0,          // 16-29: ...
    (uint32_t *)DMA1_CH4_IRQHandler, // 30 : DMA1 CH4
    [31 ... 52] = 0,
    (uint32_t *)USART1_IRQHandler, // 53: USART1 (IRQ 37)
    0, //54
    0, //55
    (uint32_t *)EXTI15_10_IRQHandler // 56 (IRQ 40)

};


