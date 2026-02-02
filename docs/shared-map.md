---
layout: default
title: Memory Map (MMIO)
parent: Shared SDK Architecture
nav_order: 2
---
# Memory Mapped I/O
Instead of using magic numbers, `my_stm32_map.h` defines the hardware architecture for the STM32L475VG:

* **RCC_BASE**: `0x40021000` (Includes CCIPR for peripheral clock selection)
* **I2C2_BASE**: `0x40005800` (Mapped to sensor bus)
* **DMA1_BASE**: `0x40020000` (Mapped for UART TX offloading)
* **FPU_CPACR**: `0xE000ED88` (Enables hardware math coprocessors CP10 and CP11)

### Peripheral Mapping
| Peripheral | Pin | Function |
| :--- | :--- | :--- |
| USART1_TX | PB6 | VCP Telemetry (AF7) |
| I2C2_SCL | PB10 | Sensor Clock (AF4) |
| I2C2_SDA | PB11 | Sensor Data (AF4) |
| LED_GREEN | PB14 | Heartbeat Indicator |