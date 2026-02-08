

ENTRY(Reset_Handler)

/* STM32L475 Memory Map */
MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
  SRAM  (xrw): ORIGIN = 0x20000000, LENGTH = 96K   /* SRAM1 */
  SRAM2 (xrw): ORIGIN = 0x10000000, LENGTH = 32K   /* SRAM2 */
}

/* Define the end of the stack - 
   SRAM1 starts at 0x20000000 and is 96KB (0x18000) 
   So top of stack is 0x20018000 */
_estack = ORIGIN(SRAM) + LENGTH(SRAM); /* End of SRAM for Stack Pointer */

SECTIONS
{
    /* Vector Table goes first */
    .text :
    {
        . = ALIGN(4);
        *(.isr_vector)
        *(.text)
        *(.text*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH
    
    /* Data to be moved to RAM */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH

    /* Uninitialized data section goes into RAM */
    /*bss = Block Started by Symbol*/
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;         /* Startup script looks for _sbss */
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;         /* Startup script looks for _ebss */
        __bss_end__ = _ebss;
    } > SRAM
}