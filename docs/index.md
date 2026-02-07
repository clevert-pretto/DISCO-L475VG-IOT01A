---
layout: default
title: Home
nav_order: 1
description: "Bare-Metal Firmware Engineering Portfolio"
permalink: /
---
# STM32L475 Platform Architecture
**Principal Engineer Candidate Knowledge Base**

This repository documents follwing phases.
* **Phase 1: Bare-Metal SDK** - The transition from vendor-dependent HAL layers to a custom, deterministic Bare-MEtal SDK.
* **Phase 2: Advanced RTOS & scheduling** - Context Switching Internals, Priority Inversion & Inheritance, Stack Overflow Analysis, Multicore (SMP) concepts.

---

## 🏛 Core SDK Architecture (The `shared/` Layer)
These components form the foundation of every project in this repository. 

* **[Shared Startup Logic](./shared-startup)**: Handles the Vector Table, Reset Handler, and Weak Interrupt Aliasing.
* **[Linker Specification](./shared-linker)**: Defines the LMA/VMA memory regions for Flash and RAM.
* **[Memory Map](./shared-map)**: Hardware abstraction of the STM32L475 MMIO registers.
* **[Modular Build System](./shared-build)**: The `base.mk` framework for path-agnostic builds.
* **[Lesson learned](./lesson-learned)**: Everything that I learned - The references, the logic, the hurdles and outcomes.

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

### Phase 2: Advanced RTOS & scheduling
---

## 🛠 Tech Stack
- **Hardware:** Discovery kit IoT node (B-L475E-IOT01A)
- **Compiler:** `arm-none-eabi-gcc` (Optimization: `-O0` for debug clarity)
- **Debugger:** OpenOCD via ST-Link V2-1