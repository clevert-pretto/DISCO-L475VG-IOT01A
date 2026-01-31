# STM32L475 Deep-Dive: Bare Metal to RTOS
**Principal Engineer Candidate Knowledge Base**

Welcome to the technical breakdown of my transition to High-End Embedded Systems. This site documents the silicon-level mastery of the ARM Cortex-M4 architecture.

## 🚀 Projects & Learnings
| Module | Focus | Core Concept |
| :--- | :--- | :--- |
| [01: Silicon Handshake](./module-1-baremetal) | Bare Metal | Linker Scripts, Startup Logic, Vector Tables |
| [02: Precision Timing](./module-2-systick) | Interrupts | SysTick Timer, ISRs, Asynchronous Heartbeats |
| [03: UART & Debugging](./module-3-uart) | Communication | *Coming Soon* |

---
## 🛠 Toolchain Architecture
* **Target:** STM32L475VG (Cortex-M4)
* **Compiler:** `arm-none-eabi-gcc`
* **OS:** Windows 11 + WSL2 (Ubuntu)
* **Flash/Debug:** OpenOCD