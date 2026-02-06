

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 1024K
    SRAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 96K
}

/* Define the end of the stack - 
   SRAM1 starts at 0x20000000 and is 96KB (0x18000) 
   So top of stack is 0x20018000 */
_estack = ORIGIN(SRAM) + LENGTH(SRAM); /* End of SRAM for Stack Pointer */

SECTIONS
{
    /* Vector Table goes first */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Main Code */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        . = ALIGN(4);
    } > FLASH

    /* Read-only data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
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
        _sbss = .;
        *(.bss)
        *(.bss*)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM
}