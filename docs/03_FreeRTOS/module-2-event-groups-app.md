---
layout: default
title: "02. Building a Robust FreeRTOS Application"
parent: "Phase 3: Building a FreeRTOS application on STM32L475 from Scratch"
nav_order: 3
---

# Module 2: Building a Robust FreeRTOS Application

## 📌 Project Overview
In this phase, the project shifted from manual register-level scheduling to an industry-standard **FreeRTOS** framework. The goal was to build a multi-threaded system capable of simultaneous sensor acquisition, system health monitoring, and asynchronous logging. This architecture mimics the "Manager-Worker" pattern used in enterprise firmware.

---

## 🏗️ System Architecture

The application is built on a **Modular Layered Architecture**. Each functional block is isolated into its own task, communicating through thread-safe FreeRTOS primitives (Queues and Event Groups) to ensure deterministic behavior.



### 1. Task Hierarchy & Priorities
To prevent latency in critical system functions, tasks were assigned a strict priority hierarchy. This ensures that background telemetry never blocks system-level recovery.

| Task | Priority | Stack Size | Responsibility |
| :--- | :--- | :--- | :--- |
| **System Manager** | 3 (Highest) | 128 words | Orchestrates the boot sequence, probes hardware, and manages system state. |
| **App Logger** | 2 | 256 words | Asynchronously drains the `xPrintQueue` to UART without blocking data pipelines. |
| **Sensor Read** | 1 | 256 words | Samples I2C sensors (HTS221/LIS3MDL) and formats telemetry strings. |
| **Heartbeat** | 0 (Lowest) | 128 words | Provides a visual "System Alive" indicator via LED2. |



### 2. Inter-Task Communication (ITC)
The system utilizes a **Message Queue** (`xPrintQueue`) to bridge functional tasks with the I/O peripheral.
* **Asynchronous Execution:** Functional tasks "fire and forget" telemetry messages into the queue.
* **Pass-by-Copy Safety:** The queue passes a `sAppLoggerMessage_t` structure by copy. This ensures memory safety even if the sending task immediately overwrites its local buffer.

---

## 🚦 Thread-Safe State Management
Initially, the system state was managed via a global `enum`. To align with production standards and prevent race conditions, this was upgraded to use **FreeRTOS Event Groups**.

* **The Mechanism:** The System Manager acts as the "Producer," setting specific bits (e.g., `EVENT_BIT_INIT_SUCCESS` or `EVENT_BIT_FAULT_DETECTED`) upon completing hardware checks.
* **The Consumers:** Worker tasks (like the Heartbeat) act as consumers. Instead of polling a variable and wasting CPU cycles, they block on `xEventGroupWaitBits`, consuming 0% CPU until the exact state condition is met.



---

## 🛠️ Engineering Journal & Lessons Learned

### 1. Task Starvation (The "Busy-Wait" Trap)
* **Observation:** After assigning the Logger to Priority 2, lower-priority tasks (Sensor and Heartbeat) stopped executing entirely.
* **Discovery:** The Logger used a `0` timeout on `xQueueReceive`. Because it was a high-priority task that never entered a "Blocked" state, the scheduler never yielded the CPU to lower-priority tasks.
* **Resolution:** Implemented `portMAX_DELAY` on the receiver side. This puts the Logger into a dormant state when the queue is empty, instantly freeing CPU cycles for the rest of the system.

### 2. Stack Integrity & MISRA C Compliance
* **Observation:** Standard library string formatting (`snprintf` with `%f`) consumes massive amounts of stack memory (~400+ bytes) and caused unpredictable HardFaults.
* **Resolution:** * Enabled the Hardware Floating-Point Unit (FPU) via `-mfloat-abi=hard` in the Makefile to prevent software emulation bloat.
    * Developed a custom `app_ftoa` and formatting macro (`LOG_SENSOR`) to avoid standard library unpredictability.
    * Replaced local array instantiation inside infinite task loops with `static` buffers, moving large memory allocations from the task stack to the `.bss` section in accordance with **MISRA C** guidelines.

### 3. I2C Initialization Ownership
* **Observation:** `BSP_TSENSOR_Init()` returned errors despite the bus being physically healthy.
* **Discovery:** The Board Support Package (BSP) maintains its own internal `I2C_HandleTypeDef`. Attempting to manually initialize I2C in `main.c` caused a hardware state conflict.
* **Resolution:** Delegated all I2C configuration strictly to the BSP layer, ensuring a single "source of truth" for the peripheral.

---

## 🛠 Deliverables
* **Multi-Threaded Firmware Architecture:** A fully operational FreeRTOS project structured with isolated functional tasks (`System Manager`, `Logger`, `Sensor Read`, `Heartbeat`).
* **Asynchronous Logging Engine:** A queue-based, non-blocking UART logging system that guarantees real-time execution of critical data paths.
* **Thread-Safe Quality Gate:** An event-driven state machine utilizing `xEventGroupWaitBits` to ensure reliable peripheral initialization without polling overhead.
* **MISRA C Aligned Codebase:** Refactored sensor processing and string manipulation using statically allocated buffers and hardware FPU acceleration, eliminating stack overflow vulnerabilities.
* **Live Telemetry Stream:** A robust UART data feed outputting deterministic sensor readings (Temperature and Humidity) immune to priority inversion.
* **v2.2** - Tag to refer.

--- 

## 🚀 Future Roadmap: Module 3
With the RTOS core stabilized, the project moves toward **Data Persistence & Reliability**, core concepts in the storage domain:
1. **QSPI Flash Logging:** Initializing the MX25R6435F to implement a circular buffer in non-volatile memory, simulating NAND block management.
2. **Watchdog Check-In:** Developing a "Task Heartbeat" mechanism where each thread must report its health to prevent an Independent Watchdog (IWDG) system reset.

---