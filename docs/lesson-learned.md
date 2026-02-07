---
layout: default
title: Lesson Learned
nav_order: 95
has_children: false
---

# 📓 Engineering Journal

This journal documents the technical hurdles, register-level insights, and architectural breakthroughs encountered during the development of the STM32L475 Bare-Metal SDK.

<details>
<summary><b>Module 01 & 02: The Bare-Metal Foundation</b></summary>

* **Reference Manual:** RM0351 (STM32L4x5).
* **Registers:** `RCC_AHB2ENR` (0x4002104C), `GPIOB_MODER` (0x48000400), `USART1_BRR` (0x4001380C).
* **Logic:** Manual calculation of Baud Rate divisors and bit-masking for GPIO Alternative Functions (`AF7 for UART`).
* **Hurdles:** Initial attempts failed because the peripheral clock gating was not enabled before configuring registers.
* **Lesson:** Always enable the clock in RCC before touching peripheral registers, or the writes will be ignored by the silicon.

</details>

<details>
<summary><b>Module 03 & 04: Precision Timing & HTS221 Sensing</b></summary>

* **Reference Manual:** RM0351 & Cortex-M4 Programming Manual.
* **Registers:** `SYSTICK_LOAD` (0xE000E014), `CPACR` (0xE000ED88), `I2C2_TIMINGR` (0x40005810).
* **Logic:** Implemented linear interpolation using factory-stored calibration coefficients.
* **Hurdles:** The system hung immediately upon performing float math because the hardware Floating Point Unit (FPU) is disabled by default.
* **Lesson:** Enabling the FPU requires a specific "Coprocessor Access" sequence in the Reset_Handler.
* **Clocking:** Used HSI16 (16MHz) as a stable clock source for I2C timing to ensure a consistent 100kHz bus speed.

</details>

<details>
<summary><b>Module 05: DMA Acceleration & Interrupts</b></summary>

* **Reference Manual:** RM0351 (DMA & NVIC Sections).
* **Registers:** `DMA1_CSELR` (0x400200A8), `DMA1_IFCR` (0x40020004), `NVIC_ISER0` (0xE000E100).
* **Logic:** Configured DMA1 Channel 4 to offload UART transmissions from the CPU.
* **Hurdles:** Encountered the "One-Shot" bug where DMA would only send data once.
* **Lesson:** The Transfer Complete (TC) flag in `IFCR` must be cleared via a direct assignment (`=`), not a bitwise OR (`|=`), because `IFCR` is a write-only register.

</details>

<details>
<summary><b>Module 06: System Reliability (IWDG)</b></summary>

* **Reference Manual:** RM0351 Section 32.
* **Registers:** `IWDG_KR` (Key), `IWDG_PR` (Prescaler), `IWDG_RLR` (Reload).
* **Logic:** Implemented a fail-safe timer that resets the CPU if the main loop hangs.
* **Hurdles:** Verified that the IWDG runs on the LSI (32kHz) clock, making it independent of the main HSI16 clock tree.
* **Lesson:** Security through isolation. By running on the LSI, the watchdog can rescue the system even if the main oscillator fails or the PLL loses lock.

</details>

<details>
<summary><b>Module 07: SPI & The Silent Bluetooth Chip</b></summary>

* **Reference Manual:** RM0351 Section 38.
* **The Register Trap (MODER):** Identified that using a pin-number constant in a 2-bit `MODER` field causes a "bit spill," corrupting adjacent pins.
* **The