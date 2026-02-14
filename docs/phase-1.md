---
layout: default
title: "Phase 1. Bare-Metal architecture"
nav_order: 2
has_children: true
---
# Phase 1. Bare-Metal architecture
The transition from vendor-dependent HAL layers to a custom, deterministic **Bare-Metal SDK**. This phase focuses on mastering the ARM Cortex-M4 architecture by driving raw silicon through direct memory-mapped register access.

## 🧠 The Mission
Transitioning from simple **Super-Loop** architectures to a deterministic, **Multi-threaded Preemptive Kernel**. This phase focuses on the "under the hood" mechanics of how an OS manages the CPU.

---

## 🛠 Core Objectives

### 1. The Silicon Handshake
Establishing the first line of code execution. This involves manual **Data Relocation** (LMA to VMA), **BSS Zeroing**, and configuring the **Vector Table Offset Register (VTOR)** to ensure the CPU locates Interrupt Service Routines accurately.

### 2. Deterministic Communication & Timing
* **Peripheral Mapping:** Defining precise memory boundaries for RCC, GPIO, UART, and I2C peripherals.
* **Interrupt Pipeline:** Configuring the **NVIC** and **EXTI** controllers to handle asynchronous events, such as user button presses and Bluetooth data ready signals.
* **Precision Timing:** Implementing a global heartbeat using the **SysTick** timer to replace non-deterministic software delay loops.

### 3. Hardware Acceleration & Reliability
* **DMA Offloading:** Utilizing **Direct Memory Access (DMA1_CH4)** to handle high-speed UART telemetry without stalling the main CPU.
* **FPU Activation:** Unmasking the hardware **Floating Point Unit** in the `Reset_Handler` to support real-time sensor interpolation.
* **System Fail-Safes:** Implementing an **Independent Watchdog (IWDG)** clocked by the LSI to rescue the system from software deadlocks.

---

## 🚀 Key Milestones
1. **Modular Build System**: A path-agnostic `Makefile` framework for scalable firmware development.
2. **Diagnostic Baseline**: A verified EXTI/NVIC pipeline used as a known-good reference for complex wireless protocols.