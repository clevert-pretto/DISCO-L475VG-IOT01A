---
layout: default
title: "04. System Reliability & Hardware Offloading"
parent: "Phase 3: Building a FreeRTOS application on STM32L475 from Scratch"
nav_order: 5
---

# Module 4: System Reliability & Hardware Offloading

## 📌 Project Overview
The final module of the project focused on transforming the firmware into a production-grade, fault-tolerant system. The primary objectives were to implement a **Multi-Level Watchdog** architecture and offload long-duration hardware operations to the QSPI peripheral’s state machine, ensuring the system remains responsive even during intensive flash maintenance.

---

## 🏗️ System Architecture

The reliability layer utilizes both hardware safety timers and RTOS-level health monitoring to prevent system "zombification".

### 1. The Multi-Level Watchdog
To ensure total system integrity, the watchdog is implemented at two levels:
* **Hardware (IWDG):** A 12-bit Independent Watchdog running on the LSI (32kHz) clock. It is configured with a **Prescaler of 64** and a **Reload value of 2500** to provide a definitive 5-second hardware reset window.
* **Software (Event Group):** An `xWatchdogEventGroup` tracks the "check-in" status of all mandatory tasks (Heartbeat, Sensor Read, Logger, and Command). The hardware dog is only "petted" if all tasks report healthy execution within the window.

### 2. Interrupt-Driven QSPI Offloading
Long-duration operations like the 25-second **Bulk Erase** were moved from blocking polling to interrupt-driven **Auto-Polling mode**:
* **Hardware Polling:** The QSPI peripheral automatically monitors the Flash Status Register's "Write In Progress" (WIP) bit.
* **Binary Semaphore:** The calling task blocks on an `xEraseCompleteSemaphore`, yielding 100% of the CPU back to the scheduler while the silicon erases.
* **Watchdog-Safe Wait:** While blocked, the task wakes up every 1000ms to refresh the hardware watchdog and check in with the software watchdog group.

---

## 🚦 Mandatory Health Gate
The `vSystemManagerTask` serves as the central Quality Gate. It utilizes `xEventGroupWaitBits` to synchronize task health.

| Task | Check-In Frequency | Failure Impact |
| :--- | :--- | :--- |
| **Heartbeat** | 1000ms | IWDG Timeout (Hardware Reset) |
| **Sensor Read**| 1000ms | IWDG Timeout (Hardware Reset) |
| **App Logger** | 1000ms (Max) | IWDG Timeout (Hardware Reset) |
| **Command** | 1000ms (Max) | IWDG Timeout (Hardware Reset) |

---

## 🛠️ Engineering Journal & Lessons Learned

### 1. The 12-Bit Register Overflow (Hardware Trap)
* **Observation:** Setting a 5000ms timeout with a Prescaler of 32 caused the system to reset every ~900ms.
* **Discovery:** The IWDG Reload Register (IWDG_RLR) is a **12-bit register** (max value 4095). A value of 5000 was truncated to 904, making the watchdog faster than the task check-in loops.
* **Resolution:** Increased the **Prescaler to 64**, allowing a reload value of 2500 to represent a true 5000ms safety window.

### 2. The "Idle Command" Starvation
* **Observation:** The system would reset if no keys were pressed in the terminal for 5 seconds.
* **Discovery:** The Command Task was blocked on a queue using `portMAX_DELAY`, preventing it from reaching the watchdog bit-setting code.
* **Resolution:** Implemented a **1000ms timeout** on the command queue. This ensures the task wakes up periodically to "report in" even when the user is idle.

### 3. Logger Task Synchronization
* **Observation:** If sensors stopped sending data, the App Logger task would time out and cause a system reset.
* **Resolution:** Configured the `LogEventProcess` queue reception with a 1-second timeout. This decoupling ensures the task's health reporting is independent of the data arrival frequency.

---

## 🛠 Deliverables
* **Fault-Tolerant Watchdog:** A synchronized hardware/software safety system that detects and recovers from task deadlocks or infinite loops.
* **Non-Blocking Hardware Operations:** Bulk Erase implementation that offloads status monitoring to the QSPI peripheral, maintaining system responsiveness during flash maintenance.
* **Deterministic Reliability Gate:** A centralized health monitor in the System Manager that enforces strict task-level check-ins before petting the hardware watchdog.
* **v3.1** - Tag to refer.

--- 

## 🏆 Project Completion
Phase 3 is now complete. The system is a fully operational, multi-threaded FreeRTOS application with persistent storage, diagnostic telemetry, and a fail-safe reliability architecture.