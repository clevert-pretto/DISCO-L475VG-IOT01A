---
layout: default
title: startup file
parent: Shared SDK Architecture
nav_order: 4
---
# Shared Startup

The "Shared" layer acts as the hardware abstraction and boot-strapping engine for the entire platform.

### The Boot Handshake
The Linker Script (`linker.ld`) defines memory boundaries, and `startup.c` performs the following:
1. **FPU Enablement**: Sets bits 20-23 in the CPACR (Coprocessor Access Control Register) to enable hardware floating-point math before `main()` starts.
2. **Data Relocation**: Copies the `.data` section from Flash (LMA) to RAM (VMA).
3. **Zeroing BSS**: Clears uninitialized global variables in RAM.
4. Calling `main()`.

# Shared Startup & Interrupt Vector Table

### 🧱 Vector Table & Interrupts
The vector table supports both core exceptions and peripheral IRQs:
* **SysTick (Offset 0x3C)**: Powers the precision timing system.
* **DMA1 Channel 4 (IRQ 14)**: Added for high-speed UART TX acceleration.
* **USART1 (IRQ 37)**: Handles asynchronous serial events.
* **Index 56 (IRQ 40)**: Mapped to `EXTI15_10_IRQHandler` for the User Button (PC13).


### 🧠 The Strategy: Weak vs. Strong Symbols
In this platform, the `startup.c` provides a complete Vector Table for the Cortex-M4. To maintain a single source of truth while allowing project-specific flexibility, I utilized **Weak Aliases**.

* **Weak Symbols:** The `SysTick_Handler` is defined as `weak`. This allows projects that don't need a timer (like Project 01) to compile without "Undefined Reference" errors.
* **Symbol Overriding:** When a project (like Project 02) defines its own `SysTick_Handler`, the linker automatically prioritizes it over the weak version.

### 🧱 Safety-First Default Handler
All unused interrupts are aliased to a `Default_Handler`. This ensures that if the hardware triggers an unexpected interrupt (e.g., a BusFault or a stray NMI), the system enters a predictable infinite loop rather than executing garbage memory.

**Update:** The vector table has been expanded to support **External Interrupts (IRQs)**. Specifically, 
**Index 53** is now mapped to the `USART1_IRQHandler` to support asynchronous serial communication on the B-L475E-IOT01A platform.
**Index 30** is now mapped to the `DMA1_CH4_IRQHandler` to support DMA1 CH4 interrupt on the B-L475E-IOT01A platform.
**Update:** To ensure stability, the `Reset_Handler` now explicitly sets the **VTOR** (Vector Table Offset Register) to $0x08000000$. This prevents "silent" ISR failures caused by the CPU looking for handlers in the wrong memory landmark.