---
layout: default
title: "01. FreeRTOS Migration, Hybrid Memory and using Queue for IPC"
parent: "Phase 3: Building a FreeRTOS application on STM32L475 from Scratch"
nav_order: 2
---

# Module 01: FreeRTOS Migration, Hybrid Memory Management and using Queue for Inter-Process Communication

## 🎯 Strategic Objective
Transition from a custom scheduler to industry-standard **FreeRTOS** on the STM32L475. 

This module focuses on establishing a robust, "mixed-mode" architecture that leverages both **Static** and **Dynamic** memory allocation to balance reliability with flexibility.

Following are the advanced aspects of the system we developed:

**1. Hybrid Memory Model** - We implemented a mixed allocation strategy. Critical tasks (like the `HeartBeat` and `TempSensor`) use **Static Allocation** (pre-allocated buffers in `.bss`) for absolute determinism, while non-critical tasks (like `PrintQueue`) use **Dynamic Allocation** (Heap) for flexibility.

**2. Gatekeeper Pattern (IPC)** - Instead of using Mutexes to guard the UART, we implemented a **Queue-based Gatekeeper**. The *Sensor Task* pushes data to a queue, and a dedicated *Print Task* pops and prints. This decouples data acquisition from the slow I/O process and makes the UART thread-safe by design.

**3. Robust Error Handling** - We moved beyond standard hard faults by implementing FreeRTOS-specific hooks: `vApplicationStackOverflowHook` (to catch stack overflows) and `vApplicationMallocFailedHook` (to catch heap exhaustion).
*Though we kept the hooks blank for now.*

**4. HAL Integration** - We successfully integrated the STM32 Cube HAL with the FreeRTOS scheduler, resolving interrupt vector collisions (`SVC_Handler`, `PendSV_Handler`) and ensuring the HAL Timebase doesn't conflict with the RTOS Tick.

## ⚙️ Technical Focus

### The Queue-Based Gatekeeper
To prevent UART corruption (e.g., "Hello World" interleaved with "Temp: 25C"), we avoided raw `HAL_UART_Transmit` calls from multiple tasks.
* **Producer:** `tempSensorTask` sends a struct pointer to `xPrintQueue`.
* **Consumer:** `vAppLoggerTask` blocks waiting for the queue. Once data arrives, it claims the UART.
This ensures serialization without the priority inversion risks sometimes associated with Mutexes.

---

## 🧱 Engineering Challenges

| Challenge | Technical Nuance |
| :--- | :--- |
| **Silent Stack Corruption** | We faced a system hang where the LED stayed solid. The root cause was a mismatch between the allocated `static StackType_t` array size and the size passed to `xTaskCreateStatic`, causing the task to overwrite the Idle Task's TCB. |
| **Interrupt Vector Collisions** | The linker reported "Multiple Definitions" for `SVC_Handler`. We learned that FreeRTOS's `port.c` must own these handlers to drive the scheduler, requiring us to disable the default implementations in `stm32l4xx_it.c`. |
| **The "Ghost" UART** | `BSP_COM_Init` Though we were initialising everything, UART was not sending any byte to terminal. We learned that as we are working without IDE, we have to Configure the UART parameters BEFORE BSP_UART_Init() manually (In CUbeMX GUI does this hard part) in `main.c`. Advantage of using your own project environment 😂.

---

## 🛠 Deliverables
* **Hybrid Scheduler:** Running `xTaskCreate` (Dynamic) and `xTaskCreateStatic` (Static) side-by-side.
* **Queue IPC:** Successful string passing from Sensor Task to UART Task.
* **Stack Overflow Protection:** Working `vApplicationStackOverflowHook` with visual error indication (Fast Blink).
* **Custom BSP Override:** Functional UART on PB6/PB7 despite library defaults.
* **v2.1** - Tagged rightly

---