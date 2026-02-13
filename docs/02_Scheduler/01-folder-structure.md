---
layout: default
title: "Folder Structure"
parent: "Phase 2: Building custom Schedulers"
nav_order: 1
---


## 🚀 STM32L475 Custom RTOS Scheduler (Bare-Metal)
This repository contains a from-scratch implementation of a preemptive Round-Robin scheduler for the ARM Cortex-M4 (STM32L475VG). The project demonstrates low-level context switching, hardware timer configuration, and memory management without the use of a commercial RTOS or high-level HAL libraries.
---
### 📂 1. Project Structure & File Purpose
```
|.vscode/
│   └── launch.json                 # Debugging configuration for WSL + GDB
02_Scheduler/
├── cmsis/
│   │──inc/
│   │   │──m-profile/
│   │   │       │──armv7m_mpu.h     # Copied from CMSIS_6-6.3.0
│   │   │       └──cmsis_gcc_m.h    # Copied from CMSIS_6-6.3.0
│   │   │──cmsis_compiler.h         # Copied from CMSIS_6-6.3.0
│   │   │──cmsis_gcc.h              # Copied from CMSIS_6-6.3.0
│   │   │──cmsis_version.h          # Copied from CMSIS_6-6.3.0
│   │   │──cmsis_cm4.h              # Copied from CMSIS_6-6.3.0
│   │   │──startup_stm32l475xx.s    # Vector table and Reset Handler, copied from cmsis-device-l4-1.7.5
│   │   └──stm32l475xx.h            # CMSIS Peripheral Access Layer, copied from cmsis-device-l4-1.7.5
│   └──src/
│       startup_stm32l475xx.s       # Startup file, copied from cmsis-device-l4-1.7.5
│──inc/
│   └── scheduler.c                 # Structure definitions (TCB_t)
│──src/
│   │── context_switch.s            # Assembly implementation of PendSV switch
│   ├── main.c                      # Hardware Init, Clock Config, and Task logic
│   ├── scheduler.c                 # TCB management and Stack Watermarking 
├── linker.ld                       # Custom linker script for memory mapping
└── makefile                        # custom Makefile to build using arm-none-eabi-gcc compiler
```

  *NOTE : further project structure will not be updated in here, refer latest directory structure for more details*
---

### 🛠️ 2. CMSIS & Startup Integration
To ensure professional-grade register access while staying "No-HAL," the project utilizes:

**CMSIS Headers:** *stm32l475xx.h* was sourced from the official STMicroelectronics CMSIS device repository. This provides the standardized structures used for direct register manipulation (e.g., `GPIOA->BSRR`).

**Startup Code:** The *startup_stm32l475xx.s* file was adapted from the GCC toolchain templates. It was modified to ensure the PendSV_Handler is globally accessible and properly linked to our custom assembly implementation.

---
### 🏗️ 3. Linker Script Construction
The linker.ld was manually crafted to align with the Cortex-M4 memory map:

**Memory Definitions:** Specifically mapped FLASH (1024K) and SRAM1 (96K).

#### Section Placement:

**.isr_vector:** Placed at the very top of Flash to ensure the CPU finds the Stack Pointer and Reset Handler on boot.

**.text:** Aggregates code from all object files.

**.data / .bss:** Configured for proper initialization of global variables during the startup sequence.

**Stack Alignment:** Enforced 8-byte alignment for task stacks to comply with the ARM Procedure Call Standard (AAPCS).

---
### ⚙️ 4. Technical Implementation Tasks
**Task 1: Clock & Timer Configuration**
The system was moved from the default 4MHz MSI clock to a stable 16MHz configuration. The SysTick timer was configured to generate an interrupt every 1ms to serve as the OS heartbeat.

**Task 2: Task Stack Initialization**
Each task is assigned a private stack. During initialization, we "fake" an exception frame. When the scheduler first switches to a task, the CPU "returns" from an interrupt that never actually happened, popping our dummy values into the registers and starting the task function.

**Task 3: Preemptive Context Switching**
The switching logic is split between C and Assembly:

**SysTick (C):** Increments the global tick counter and pends the PendSV interrupt.

**PendSV (Assembly):**

1. **Saves** *R4-R11* of the current task to its stack.

2. **Updates** the currentTask pointer to the next TCB in the circular linked list.

3. **Restores** *R4-R11* from the new task's stack and updates the Process Stack Pointer (PSP).

**Task 4: Stack Watermarking**
To detect potential memory corruption—a critical requirement in SSD firmware—we implemented stack painting. The stack is filled with `0xDEADBEEF`, and a background function calculates the unused space by searching for the "pristine" magic numbers from the bottom up.

---
### 🚀 5. How to Build & Debug (WSL)
**USB Bridge:** Use usbipd in Windows PowerShell to attach the ST-Link to WSL:

**PowerShell**
`usbipd attach --wsl --busid <bus-id>`
**Compile:** Use the arm-none-eabi-gcc toolchain via the provided Makefile.

**Debug:** Launch the Cortex-Debug configuration in VS Code. The launch.json is configured to use `gdb-multiarch` and `openocd`.

*Refer [STM32 Development & Debugging on WSL](../wsl-debug-setup.md) for more details*

---
### 📈 6. Project Results
**Multi-Tasking:** Two independent tasks toggling PA5 and PB14 LEDs at different frequencies (`500ms` vs `1000ms`).

**Preemption:** Demonstrated that the system can swap tasks even during a blocking Delay_ms loop.

**Stability:** Verified `16-byte` stack alignment and FPU-safe standard stacking.

---
### 7. Run time changes in folder structure (IMPORTANT TO BUILD CORRECT MODULE)
To build multiple module using same set of linker, startup, cmsis files, we have to make certain changes as below,
1. In Makefile we have added `MODULE ?= <Folder name here>`.
2. Type correct folder name for project you want to build from `0-Modules` folder so that it can generate it's relevant `final.bin` and `final.elf` files in `release` folder.
3. With usual F5, now you can debug the correct active files, or you can load `final.bin` directly.

---

