/* Define the end of the stack - 
   SRAM1 starts at 0x20000000 and is 96KB (0x18000) 
   So top of stack is 0x20018000 */
_estack = 0x20018000;

MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 96K
}

SECTIONS
{
    /* The program code and other data goes into FLASH */
    .text :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector)) /* The Vector Table must come first! */
        *(.text) /*Matches only the section exactly named .text. This is where the bulk of your standard functions go. */
        *(.text*) /*Matches any section that starts with .text. This includes "special" sub-sections like .text.startup, .text.hot, .text.unlikely, or individual function sections if you use specific compiler flags.*/
        *(.rodata) /*same concept as above but for data*/
        *(.rodata*) /*same as above*/
        . = ALIGN(4);
        _etext = .;        /* End of code in Flash */
    } > FLASH

    /* Used by the startup to initialize data */
    _sidata = LOADADDR(.data);

    /* Initialized data sections go into RAM, but are stored in FLASH */
    .data : 
    {
        . = ALIGN(4);
        _sdata = .;        /* Start of .data in RAM */
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;        /* End of .data in RAM */
    } > RAM AT > FLASH

    /* Uninitialized data section goes into RAM */
    /*bss = Block Started by Symbol*/
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;         /* Start of .bss */
        *(.bss)
        *(.bss*)
        . = ALIGN(4);
        _ebss = .;         /* End of .bss */
    } > RAM
}