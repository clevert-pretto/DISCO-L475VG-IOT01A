#include <stdint.h>

// These symbols come from the Linker Script (.ld)
extern uint32_t _estack;  // Top of RAM (Stack)
extern uint32_t _sdata;   // Start of .data in RAM
extern uint32_t _edata;   // End of .data in RAM
extern uint32_t _sidata;  // Start of .data in FLASH (the values)
extern uint32_t _sbss;    // Start of .bss in RAM
extern uint32_t _ebss;    // End of .bss in RAM

// Declaration of the main function
extern int main(void);
extern void SysTick_Handler(void);

// Alias all unused interrupts to the Default_Handler
void NMI_Handler(void)          __attribute__ ((weak, alias ("Default_Handler")));
void HardFault_Handler(void)    __attribute__ ((weak, alias ("Default_Handler")));
// ... add others as needed ...

void Reset_Handler(void) {
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
    (uint32_t *)&_estack,      // 0: Initial Stack Pointer
    (uint32_t *)Reset_Handler,  // 1: Reset Vector (Code starts here)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 2-14: Reserved/Exceptions
    (uint32_t *)SysTick_Handler // 15: SysTick Timer
};