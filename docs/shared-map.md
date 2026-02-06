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
* **IWDG_BASE**: `0x40003000` (Independent Watchdog)
* **SPI3_BASE**: `0x40003C00UL` (SPI3)
* **GPIOA_BASE**: `0x48000000UL` (GPIOA base address)
* **GPIOB_BASE**: `0x48000400UL` (GPIOB base address)
* **GPIOC_BASE**: `0x48000800UL` (GPIOC base address)
* **GPIOD_BASE**: `0x48000C00UL` (GPIOD base address)
* **GPIOE_BASE**: `0x48001000UL` (GPIOE base address)
* **SYSCFG_BASE**: `0x40010000` (Used for EXTI port routing).
* **EXTI_BASE**: `0x40010400` (Independent peripheral for edge-detection and masking).
* **NVIC_ISER1**: `0xE000E104` (Enables IRQs 32-63).


### Peripheral Mapping
| Peripheral | Pin | Function |
| :--- | :--- | :--- |
| USART1_TX | PB6 | VCP Telemetry (AF7) |
| I2C2_SCL | PB10 | Sensor Clock (AF4) |
| I2C2_SDA | PB11 | Sensor Data (AF4) |
| LED_GREEN | PB14 | Heartbeat Indicator |
| USR_BTN | PC13 | User push Button |

### **Hardware Pinout (Wireless)**
* **PE0**: BT_CSN (Chip Select)
* **PA8**: BT_RST (Reset)
* **PE1**: BT_IRQ (Interrupt)
* **PC10**: SPI3_SCK_PIN (SPI3 Clock pin)
* **PC11**: SPI3_MISO_PIN (SPI3 MISO pin)
* **PC12**: SPI3_MOSI_PIN (SPI3 MOSI pin)

### **Hardware Pinout (Diagnostic & User Interface)**
* **PC13**: B1_USER_BUTTON (Active Low, EXTI13).
* **PB14**: LED_GREEN (Diagnostic Heartbeat).