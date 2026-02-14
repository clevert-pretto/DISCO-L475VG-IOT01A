---
layout: default
title: Home
nav_order: 1
description: "STM32L475, bare metal, FreeRTOS portfolio"
permalink: /
---
# STM32L475, bare metal, FreeRTOS portfolio

## 📂 Knowledge Base Roadmap
1. [**Phase 1: Bare-Metal SDK**](./phase-1.md)
   - From Silicon Handshake to Interrupt-Driven Bluetooth.
2. [**Phase 2: Building custom Schedulers**](./phase-2.md)
   - Context Switching, Priority Inversion, and SMP concepts.
3. [**Phase 3: Building a FreeRTOS application on STM32L475 from Scratch**](./phase-3.md)
   - Building a FreeRTOS application on STM32L475 (B-L475E-IOT01A) from Scratch without using IDE.
---

## 🚀 Engineering Modules
### Phase 1. Bare_Metal architecture (Cortex M4)
- [**01. Silicon Handshake**](./01_BareMetal/module-1-baremetal.md) - Bootstrapping, GPIO, and Modular Makefiles.
- [**02. Precision Timing**](./01_BareMetal/module-2-systick) - SysTick Hardware, ISRs, and `volatile` memory safety.
- [**03. Serial Communication**](./01_BareMetal/module-3-uart) - UART Peripheral and Clock Trees.
- [ **04. Environmental Sensing**](./01_BareMetal/module-4-i2c) - I2C, Hardware FPU, and Sensor Interpolation.
- [**05. DMA Acceleration**](./01_BareMetal/module-5-dma) - DMA Controller , DMA1_CH4, IRQ Handlers, Background Telemetry, and CPU Offloading.
- [**06. Independant Watchdog**](./01_BareMetal/module-6-watchdog) - System Reliability IWDG, LSI Clocking, Fail-safe mechanisms.
- [**07. SPI Bluetooth Hello**](./01_BareMetal/module-07-spi-BLE) - High-Speed Serial, Full-duplex SPI, FIFO management, Reset sequencing.
- [**08. External Interupt via User Button**](./01_BareMetal/module-8-ext_intrrupt_usr_btn) - Verified EXTI/NVIC pipeline & VTOR.

---
### Phase 2: Building custom Schedulers
- [**01. Preemptive Round-Robin Scheduler**](./02_Scheduler/module-1-RR-scheduler.md) - context switching, pendSV Handler, Assembly code, time sliced preemption, Round-Robin, Real-Time kernel, and Modular Makefiles, .vscode launch.json for debugging.

- [**02. Preemptive Priority-Based Scheduler**](./02_Scheduler/module-2-PB-scheduler.md) - Priority preemption, state based scheduling, non-blocking delays, Idle task, High priority ready algorithm

---
### "Phase 3: Building a FreeRTOS application on STM32L475 from Scratch"
- [**01.Guide to prepare project setup**](./03_FreeRTOS/00-guide.md) - FreeRTOS Kernel, STM32L4 ST library, prepare build system, run blinky.

---
## 🛠 Tech Stack
- **Hardware:** Discovery kit IoT node (B-L475E-IOT01A)
- **Compiler:** `arm-none-eabi-gcc` (Optimization: `-O0` for debug clarity)
- **Debugger:** OpenOCD via ST-Link V2-1
- **Operating environment** Ubuntu on WSL
- **IDE** Microsoft Visual Studio Code

---
## Lesson learned
In [Lesson Learned](./lesson-learned.md) section - Everything that I learned - The references, the logic, the hurdles and outcomes.

---