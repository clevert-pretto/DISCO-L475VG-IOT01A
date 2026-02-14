---
layout: default
title: "Setting up environment for Building a FreeRTOS application on STM32L475 from Scratch"
nav_order: 1
parent : "Phase 3: Building a FreeRTOS application on STM32L475 from Scratch"
---

# Setting up environment to build application with freeRTOS on B-L475E-IOT01A Discovery Kit

This guide documents the process of bringing up the **[B-L475E-IOT01A Discovery Kit](https://www.st.com/en/evaluation-tools/b-l475e-iot01a.html)**  (uses : STM32L475VGT6) from "bare metal" using the STM32 HAL and FreeRTOS. It avoids proprietary IDEs in favor of a transparent, makefile-based build system.

---

## 1. Prerequisites

Ensure these tools are installed and in your system PATH:
* **Compiler:** `arm-none-eabi-gcc` (GNU Arm Embedded Toolchain)
* **Build System:** `make` (Windows users: install via MinGW or xPack)
* **Flash Tool:** `openocd`
* **Source Repositories:**
    * [STM32CubeL4 MCU Package](https://github.com/STMicroelectronics/STM32CubeL4) (Tested with v1.18.2)
    * [FreeRTOS Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel) (Tested with 202406.01 LTS)

---

## 2. Project Architecture

We will create a specific directory structure to separate Drivers, Middleware, and Application code.

```
03_FreeRTOS/
├── App/                                   # Application layer files, including main.c
│   ├── Src/                               # Application specific source files
│   ├── Inc/                               # Application specific Headers
│   └── main.c                             # Main
├── Drivers/                               # CMSIS, BSP & STM32L4 Core Drivers
│      ├── BSP/                            # Board Support Packages
│      │    ├── B-L475E-IOT01/             # B-BL475E-IOT01 Board Specific BSPs
│      │    └── Components/                # Libraries for components on B-BL475E-IOT01 Board 
│      │            └── Common/            # Copied as is
│      │            └── hts221/            # RH and temperature sensor
│      │            └── lis3mdl/           # 3-axis magnoetometer
│      │            └── lps22hb/           # Abosolute pressure sensosr
│      │            └── lsm6dsl/           # 3D accelerometer and gyroscope
│      │            └── mx25r6435f/        # 64MBit Flash memory
│      ├── CMSIS/                          # CMSIS libs
│      │    ├── Core/                      # Core CMSIS headers
│      │    │     └── Include/             # Cortex-M4 Headers
│      │    └── Device/                    # CMSIS Device support 
│      │          └── ST/                  # ST specific
│      │              └── STM32L4xx/       # STM32L4 specific
│      │                     ├── Include/  # Headers
│      │                     └── Source/   # Source
│      └── STM32L4xx_HAL_Driver/           # ST Hardware Abstraction Layer
│                 ├── Inc/                 # Includes
│                 │    └── Legacy/         # legacy Includes
│                 └── Src/                 # Source
│                      └── Legacy/         # Legacy source
├── Middlewares/                           # Selected Middleware files
│   └── FreeRTOS/                          # FreeRTOS specific files
│           ├── include/                   # RTOS Kernel files
│           └── portable/                  # RTOS Portable files
├── Makefile                               # Build Script
└── STM32L475VGTx_FLASH.ld                 # Linker Script

```
---
## 2. Project Architecture

We must cherry-pick files from the official SDKs. Follow these copy operations exactly.

| package |SDK set | Copy From  |   To | file names |
|---------|------------------|------------|------|------------|
| [STM32CubeL4 MCU Package](https://github.com/STMicroelectronics/STM32CubeL4) | CMSIS (Core System) | STM32CubeL4/Drivers/CMSIS/Core/Include | Drivers/CMSIS/Core/Include | cmsis_compiler.h, cmsis_gcc.h, cmsis_version.h, core_cm4.h, mpu_armv7.h |
|  |  | CMSIS/Device/ST/STM32L4xx/Include | Drivers/CMSIS/Device/ST/STM32L4xx/Include | stm32l4xx.h, stm32l475xx.h, system_stm32l4xx.h |
|  |  | CMSIS/Device/ST/STM32L4xx/Source/Templates | Drivers/CMSIS/Device/ST/STM32L4xx/Source | system_stm32l4xx.c |
|  | Startup Code (Boot Assembly) | CMSIS/Device/ST/STM32L4xx/Source/Templates/gcc | Drivers/CMSIS/Device/ST/STM32L4xx/Source | startup_stm32l475xx.s |
|  | HAL Drivers (Peripherals) | Drivers/STM32L4xx_HAL_Driver/Inc | Drivers/STM32L4xx_HAL_Driver/Inc | All files except *_template.h |
|  |   | Drivers/STM32L4xx_HAL_Driver/Inc/Legacy | Drivers/STM32L4xx_HAL_Driver/Inc/Legacy | All files |
|  |   | Drivers/STM32L4xx_HAL_Driver/Src | Drivers/STM32L4xx_HAL_Driver/Src | All files |
|  |   | Drivers/STM32L4xx_HAL_Driver/Src/Legacy | Drivers/STM32L4xx_HAL_Driver/Src/Legacy | All files |
|  |   | Drivers/STM32L4xx_HAL_Driver/Inc/stm32l4xx_hal_conf_template.h | Drivers/STM32L4xx_HAL_Driver | rename to stm32l4xx_hal_conf.h |
|  | BSP for B-L475E-IOT01 | Drivers/BSP/B-L475E-IOT01 | Drivers/BSP/B-L475E-IOT01 | All .c and .h files |
|  | On-Board componenets and sensors | Drivers/BSP/Components | Drivers/BSP/Components | folders named - Common, hts221, lis2mdl, lps22hb, lsm6dsl, mx25r6435f |
| [FreeRTOS Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel) | FreeRTOS (The OS) | FreeRTOS/FreeRTOS-Kernel | Middlewares/FreeRTOS | list.c, queue.c, tasks.c, timers.c |
|  |  | FreeRTOS/FreeRTOS-Kernel/include | Middlewares/FreeRTOS/include | All files |
|  |  | FreeRTOS-Kernel/portable/GCC/ARM_CM4F | Middlewares/FreeRTOS/include | All files |
|  |  | FreeRTOS-Kernel/examples/template_configuration | Middlewares/FreeRTOS | Refer freeRTOSConfig.h, but create your own |


## 3. Configuration

Make sure following lines are uncommented (if commented) in stm32l4xx_hal_conf.h -
```c
#define HAL_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
```

## 4. The Build System

### Linker Script (STM32L475VGTx_FLASH.ld)
We already copied it or use a standard L475 linker script (Flash: 1MB, RAM: 96KB/128KB).

### Makefile
The Makefile we created uses wildcards to compile all source files found in the directories, making it easy to add new files later.

## 5. Build and flash
1. Clean and Build:

```bash
make clean
make
```

*Upon Success:* You should see blinky.bin in the build/ folder.

2. Flash to Board (OpenOCD):
Connect the board via USB (ST-Link) and run:

```bash
openocd -f interface/stlink.cfg -f target/stm32l4x.cfg -c "program build/blinky.elf verify reset exit"
```

3. Result:
The Green LED (PB14) on the B-L475E-IOT01A will start blinking every 500ms.

*As per initial commit. Note that this will change as I keep developing.*

---