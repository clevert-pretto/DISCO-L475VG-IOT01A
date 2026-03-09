---
layout: default
parent : "Phase 4: The C++ Paradigm Shift (Modern C++ for Constrained Systems)"
title: "1: Project Post-Mortem: Transitioning STM32 FreeRTOS from C to Modern C++"
nav_order: 1
---

## Project Post-Mortem: Transitioning STM32 FreeRTOS from C to Modern C++

### Overview
This document outlines the architectural transition of an embedded FreeRTOS project on the **STM32L475VG (B-L475E-IOT01A)** from standard C with a rigid **Makefile** to **Modern C++17** using a robust **CMake** build system. It captures the debugging journey, the resolution of strict compiler/linker errors, and the critical architectural "Golden Rules" established along the way.

## Phase 1: The Build System (Make -> CMake)

The legacy Makefile was replaced with a modular, toolchain-driven CMake architecture to improve scalability and enforce strict quality gates.

**Custom Toolchain File (my.cmake):** We defined the cross-compiler (arm-none-eabi-gcc/g++) and explicitly set the hardware FPU and Cortex-M4 CPU flags globally.

**Debug Symbols:** We injected -g3 and -O0 into the toolchain flags to ensure GDB could map hardware memory addresses back to exact C++ line numbers.

**The Generator Expression Trick:** Vendor code (ST HAL, CMSIS) is rarely written to pass strict C++ linting. We used CMake Generator Expressions ``($<$<COMPILE_LANGUAGE:CXX>:-Werror -Wpedantic ...>)`` to apply Staff-level strictness only to our Application C++ files, allowing the vendor C files to compile cleanly without failing the build.

**Automated Memory Reporting:** A post-build Python script was embedded into `CMakeLists.txt` to parse the .elf file and automatically print Flash and RAM consumption percentages.

## Phase 2: Modern C++ & FreeRTOS Architecture

We abandoned global variables and naked struct passing in favor of Object-Oriented RTOS design.

**Encapsulation & Dependency Injection:** Tasks and their data were encapsulated into classes (`appLogger`, `systemManager`, `AppHeartbeat`, `appSensorRead`). Hardware handles (`UART_HandleTypeDef`, `QSPI_HandleTypeDef`) and RTOS Event Groups were passed directly into constructors, eliminating hidden global dependencies.

**100% Static Memory Allocation:** To ensure deterministic behavior and prevent runtime heap fragmentation, all Tasks, Queues, Mutexes, and Event Groups were created using FreeRTOS *Static APIs (e.g., `xTaskCreateStatic`, `xQueueCreateStatic`).

**Unified State Machine:** A `systemManagerTask` was introduced to oversee the boot sequence, managing `SYS_STATE_INIT_HARDWARE`, `SYS_STATE_OPERATIONAL`, and `SYS_STATE_FAULT` states.

## Phase 3: Resolving The "Gotchas" (Logical & Hardware Bugs)

The transition exposed several deep architectural and hardware-level bugs. Here is how they were resolved:

**1. The Pre-Scheduler Crash**
**Symptom:** The board was completely dead on boot; no Heartbeat LED, no UART prints.

**Root Cause:** The `appLogger::init()` function called `storageInit()`, which attempted to take a Mutex using `xSemaphoreTake` with a timeout. This happened in `main()`, before `vTaskStartScheduler()` was called. Blocking before the OS tick timer starts causes an immediate hardware fault.

**Resolution:** Initialization was split. Queue/Mutex creation stayed in `main()`, but hardware execution (`storageInit()`) was moved into the `systemManagerTask` so it safely executes under the RTOS context.

**2. Watchdog Starvation via portMAX_DELAY**
**Symptom:** The MCU would continuously reset itself every 5 seconds.

**Root Cause:** The `vCommandTask` used `xQueueReceive` with `portMAX_DELAY` to wait for UART input. Because it blocked infinitely, it never reported back to the `systemManagerTask`, causing the `IWDG` (Independent Watchdog) to starve and reset the board.

**Resolution:** Replaced infinite delays with finite timeouts (e.g., `pdMS_TO_TICKS(1000)`), ensuring tasks always wake up to set their Watchdog Event Group bits.

**3. The Default_Handler Hardware Trap**
**Symptom:** Sending the Bulk Erase command (p) caused the system to hang permanently.

**Root Cause:** The QSPI Auto-Poller triggered a hardware interrupt. However, because the C++ compiler mangles function names, the silicon couldn't find `QUADSPI_IRQHandler`. It fell back to the Default_Handler infinite loop trap.

**Resolution:** Wrapped the hardware ISR in an extern "C" block so the ARM Cortex-M4 vector table could correctly route the interrupt to the HAL state machine.

**4. Flash Memory Desync & Double-Writes**
**Symptom:** Flash memory headers were reading as completely blank (`0xFFFFFFFF`), but sensor data was being written perfectly.

**Root Cause:** Raw HAL commands were mixed with high-level BSP calls (`BSP_QSPI_Write`), corrupting the QSPI software state machine. Furthermore, writing the header twice without an erase cycle corrupted the flash cells.

**Resolution:** Standardized on Vendor BSP functions (`BSP_QSPI_Erase_Chip`). Implemented an RTOS-friendly polling loop (`while (BSP_QSPI_GetStatus() == QSPI_BUSY)`) that yields the CPU (`vTaskDelay`) and feeds the watchdog (`HAL_IWDG_Refresh`) while waiting for the 25-second erase cycle to finish.

## 🏆 The Golden Rules of Embedded C++ Architecture learned

**Never Block Before the Scheduler Starts:** Mutexes, Semaphores, and Delays are illegal until time is moving (after `vTaskStartScheduler()`).

**Isolate Vendor Code:** Never hold vendor ST HAL/BSP code to your own pedantic C++ standards. Treat them as `SYSTEM` headers and use `CMake Generator Expressions` to isolate them.

**Watchdogs Forbid Infinite Blocking:** Any task participating in a software Watchdog event group must never use `portMAX_DELAY` on a queue or semaphore.

**Hardware Interrupts Demand extern "C":** If an ISR is written inside a `.cpp` file, it must be wrapped in `extern "C"` to prevent name mangling, or the hardware will lose the bridge to the software.

**Don't Mix HAL and BSP State Machines:** Pick one layer of abstraction for a peripheral and stick to it to prevent internal state desynchronization.

---