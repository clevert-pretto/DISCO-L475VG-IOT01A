---
layout: default
title: 04. I2C & Floating Point Math
parent: Phase 1. Bare_Metal architecture - Cortex M4
nav_order: 4
---
# Module 04: Hardware-Accelerated Environmental Sensing

### 🧠 Strategic Objective
Integrating the **HTS221** capacitive digital sensor to monitor temperature via the I2C2 bus, while implementing high-performance floating-point interpolation.

### ⚙️ Technical Specs
- **Peripheral:** I2C2 (PB10/SCL, PB11/SDA)
- **Clock Source:** HSI16 (16MHz) to ensure 100kHz I2C Standard Mode timing
- **Math:** Hardware FPU enabled for linear interpolation
- **Clock Tree**: HSI16 (16MHz) selected via `RCC_CFGR` for stable 115200 baud UART and 100kHz I2C timing.
- **FPU Activation**: Manual enablement of the **Coprocessor Access Control Register (CPACR)** bits 20-23 in the `Reset_Handler`.
- **Protocol**: I2C2 (PB10/SCL, PB11/SDA) using **Open-Drain** mode for physical pull-up compatibility.

### 🧱 Engineering Challenges & Solutions

#### **1. The Silicon "Handbrake": Enabling the FPU**
On the Cortex-M4F, the Floating Point Unit is disabled by default. Attempting to calculate temperature calibration values without enabling the **CPACR** (Coprocessor Access Control Register) resulted in immediate hardware exceptions. I updated the `Reset_Handler` to provide full access to CP10 and CP11 coprocessors.

#### **2. Non-Blocking I2C Driver**
To prevent a "Bus Hang" from freezing the entire system, I implemented diagnostic **timeouts** in the I2C read/write routines. This ensures that the SysTick-based heartbeat LED continues to blink even if the sensor is physically disconnected.

#### **3. Linear Interpolation Math**
The HTS221 provides raw ADC counts that must be mapped to factory-calibrated temperature points ($T0$ and $T1$). I utilized the following linear interpolation formula in the shared driver:

$$T = \frac{(T1 - T0) \cdot (T_{out} - T0_{out})}{T1_{out} - T0_{out}} + T0$$

I implemented a denominator check to ensure $T1_{out} \neq T0_{out}$, preventing a **Divide-by-Zero** crash.

#### **4. The Floating Point "Trap"**
While the Cortex-M4F has a hardware Floating Point Unit, it is disabled by default. Attempting to perform the linear interpolation required for HTS221 calibration resulted in immediate system hangs. I resolved this by adding FPU enablement logic to the `startup.c` before `main()` is called.

#### **5. Robust I2C State Machine**
I implemented diagnostic **timeouts** in all I2C polling loops (TXIS, TC, RXNE). This prevents the entire firmware from freezing if the sensor is disconnected, ensuring the SysTick-driven heartbeat LED remains operational.