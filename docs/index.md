# STM32L475 Platform Architecture
**Principal Engineer Candidate Knowledge Base**

This repository contains a modular firmware ecosystem for the STM32L475 (ARM Cortex-M4). Instead of isolated projects, I have engineered a **Shared SDK** model to ensure deterministic boot sequences and memory safety across all modules.

## 🏛 Platform Architecture (The `shared/` SDK)
To minimize technical debt and maximize code reuse, the following core components are centralized:
* **[Linker Specification](./shared-linker):** Physical memory mapping and section allocation.
* **[The Startup Sequence](./shared-startup):** C-Runtime initialization and Vector Table management.
* **[Hardware Abstraction](./shared-map):** Low-level register mapping (MMIO).
* **[Unified Build System](./shared-build):** A modular Makefile framework.

## 🚀 Engineering Modules
| Module | Focus | Learning Outcome |
| :--- | :--- | :--- |
| [01: Bare Metal](./module-1-baremetal) | Bootstrapping | Reset handlers and GPIO control. |
| [02: Precision Timing](./module-2-systick) | Interrupts | SysTick hardware and asynchronous heartbeats. |
| [03: UART/Debug](./module-3-uart) | Serial I/O | *In Progress* |