# Project 01: Bare-Metal Silicon Handshake

## Executive Summary
This project demonstrates the fundamental boot sequence and hardware initialization of the STM32L475 (Cortex-M4). By bypassing all vendor abstraction layers (HAL/LL), I have implemented a deterministic firmware entry point suitable for safety-critical applications.

## Technical Implementation

### 1. Memory Management (Linker Script)
I authored `linker.ld` to map the physical memory layout:
* **Flash:** 1MB @ 0x08000000
* **SRAM1:** 96KB @ 0x20000000
Used `AT > FLASH` to manage the Load Memory Address (LMA) vs Virtual Memory Address (VMA) for initialized data.



### 2. Boot Sequence (Startup)
The `startup.c` file provides the hardware-software bridge:
* **Vector Table:** Defined the MSP and Reset Handler.
* **C-Runtime Init:** Manual implementation of the `.data` copy loop and `.bss` zeroing loop.
* **Entry:** Explicit branch to `main()`.

### 3. Register-Level I/O
Direct manipulation of Memory-Mapped I/O (MMIO) registers:
* **Clock Control:** Enabled AHB2 bus via `RCC_AHB2ENR`.
* **GPIO Config:** Configured `GPIOB_MODER` and `GPIOB_ODR` for high-frequency LED toggling.

## How to Build
1. Requirements: `arm-none-eabi-gcc`, `make`.
2. Run `make` to generate `main.bin`.
3. Flash using OpenOCD:
   `openocd -f interface/stlink.cfg -f target/stm32l4x.cfg -c "program main.bin verify reset exit 0x08000000"`