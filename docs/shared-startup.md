---
layout: default
title: Shared SDK Architecture
nav_order: 2
has_children: true
---
# Shared SDK Architecture

The "Shared" layer acts as the hardware abstraction and boot-strapping engine for the entire platform.

### The Boot Handshake
The Linker Script (`linker.ld`) defines where memory starts, and the Startup Code (`startup.c`) performs the manual labor of:
1. Setting the **Stack Pointer**.
2. Copying the **.data** section from Flash to RAM.
3. Zeroing the **.bss** section.
4. Calling `main()`.

# Shared Startup & Interrupt Vector Table

### 🧠 The Strategy: Weak vs. Strong Symbols
In this platform, the `startup.c` provides a complete Vector Table for the Cortex-M4. To maintain a single source of truth while allowing project-specific flexibility, I utilized **Weak Aliases**.

* **Weak Symbols:** The `SysTick_Handler` is defined as `weak`. This allows projects that don't need a timer (like Project 01) to compile without "Undefined Reference" errors.
* **Symbol Overriding:** When a project (like Project 02) defines its own `SysTick_Handler`, the linker automatically prioritizes it over the weak version.



### 🧱 Safety-First Default Handler
All unused interrupts are aliased to a `Default_Handler`. This ensures that if the hardware triggers an unexpected interrupt (e.g., a BusFault or a stray NMI), the system enters a predictable infinite loop rather than executing garbage memory.

**Update:** The vector table has been expanded to support **External Interrupts (IRQs)**. Specifically, **Index 53** is now mapped to the `USART1_IRQHandler` to support asynchronous serial communication on the B-L475E-IOT01A platform.