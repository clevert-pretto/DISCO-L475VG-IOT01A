---
layout: default
title: Home
nav_order: 1
description: "Bare-Metal Firmware Engineering Portfolio"
permalink: /
---
# STM32L475 Platform Architecture
**Principal Engineer Candidate Knowledge Base**

This repository documents the transition from vendor-dependent HAL layers to a custom, deterministic **Bare-Metal SDK**.

---

## 🏛 Core SDK Architecture (The `shared/` Layer)
These components form the foundation of every project in this repository. 

* **[Shared Startup Logic](./shared-startup)**: Handles the Vector Table, Reset Handler, and Weak Interrupt Aliasing.
* **[Linker Specification](./shared-linker)**: Defines the LMA/VMA memory regions for Flash and RAM.
* **[Memory Map](./shared-map)**: Hardware abstraction of the STM32L475 MMIO registers.
* **[Modular Build System](./shared-build)**: The `base.mk` framework for path-agnostic builds.

---

## 🚀 Engineering Modules
| Module | Title | Technical Focus |
| :--- | :--- | :--- |
| **01** | [Silicon Handshake](./module-1-baremetal) | Bootstrapping, GPIO, and Modular Makefiles. |
| **02** | [Precision Timing](./module-2-systick) | SysTick Hardware, ISRs, and `volatile` memory safety. |
| **03** | [Serial Communication](./module-3-uart) | *Upcoming: UART Peripheral and Clock Trees.* |

---

## 🛠 Tech Stack
- **Hardware:** Discovery kit IoT node (B-L475E-IOT01A)
- **Compiler:** `arm-none-eabi-gcc` (Optimization: `-O0` for debug clarity)
- **Debugger:** OpenOCD via ST-Link V2-1