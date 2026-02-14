---
layout: default
title: "Phase 3: Building a FreeRTOS application on STM32L475 from Scratch"
nav_order: 5
has_children: true
---

# Phase 3: Building a FreeRTOS application on STM32L475 (B-L475E-IOT01A) from Scratch
**No IDE | GCC | Make | OpenOCD**

## 🧠 The Mission
Escaping the abstraction of proprietary IDEs to build a production-grade **FreeRTOS** environment from raw source code. This phase focuses on the "plumbing" of embedded systems—mastering the build process, dependency management, and hardware bring-up without the "magic" of code generators.

---

## 🛠 Core Objectives

### 1. Toolchain Independence
* **GCC & Make:** Replacing "Click-to-Build" buttons with manual compilation rules, `vpath` source discovery, and `wildcard` file handling.
* **Linker Mechanics:** Understanding memory layout (`.text`, `.data`, `.bss`) and manually configuring the **STM32L475** Flash/RAM partitioning via `.ld` scripts.
* **Garbage Collection:** Utilizing `-ffunction-sections` and `--gc-sections` to strip unused HAL drivers from the final binary.

### 2. Manual BSP Integration (The "Hunt")
* **Driver Extraction:** Manually gathering **CMSIS** core headers, **STM32L4 HAL** drivers, and **Startup Assembly** code from vendor SDKs.
* **Dependency Resolution:** Solving complex include chains (Legacy headers, Extended `_ex` drivers) and eliminating conflicting template files.
* **Interrupt Mapping:** Manually mapping FreeRTOS kernel handlers (`SVC`, `PendSV`, `SysTick`) to the STM32 vector table.

### 3. OS Configuration
* **Minimal Config:** Crafting a `FreeRTOSConfig.h` from scratch, tailored specifically for the Cortex-M4F.
* **Hook Management:** configuring the OS to run without standard hooks (Idle, Tick, Stack Overflow) for initial bring-up simplicity.

---

## 🚀 Key Milestones
1.  **The Clean Architecture:** A modular project structure separating `Drivers`, `Middlewares`, and `Application` code.
2.  **The Bulletproof Makefile:** A robust build script capable of compiling the entire HAL + OS stack.
3.  **Alive on Hardware:** A validated Green LED blink (PB14) driven by the FreeRTOS scheduler, flashed and verified using **OpenOCD**.

---