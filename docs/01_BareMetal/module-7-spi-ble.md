---
layout: default
title: 07. SPI & Bluetooth Verification
nav_order: 7
parent: Phase 1. Bare_Metal architecture - Cortex M4
---
# Module 07: SPI Protocol & Bluetooth LE Hardware Handshake

### 🧠 Strategic Objective
Implementing a robust **Serial Peripheral Interface (SPI)** driver from scratch to communicate with the onboard **SPBTLE-RF** Bluetooth module. This module marks the transition from simple sensors to complex wireless systems.

### ⚙️ Technical Specs
- **Peripheral:** SPI3.
- **Mode:** Full-Duplex Master, 8-bit Data Size.
- **Clock:** 8MHz (based on 16MHz HSI/MSI source).
- **Pins:** SCK (PC10), MISO (PC11), MOSI (PC12).
- **Control Pins:** BT_CSN (PE0), BT_RST (PA8).

### 🧱 Engineering Challenges & Solutions

#### **1. The 8-bit FIFO "Double-Write" Trap**
The STM32L475 features a 4-level deep FIFO. I discovered that writing to the `DR` register using a standard 32-bit assignment can inadvertently push multiple bytes or 16-bit values. 
**Solution:** I implemented a **`volatile uint8_t *`** pointer cast in the transfer function to force a single 8-bit `STRB` instruction, ensuring exactly 8 clock pulses per byte.

#### **2. Multi-Port Clock Gating**
Unlike previous modules, this driver requires three separate GPIO ports to be active simultaneously: **Port A** (Reset), **Port C** (SPI), and **Port E** (Chip Select). 
**Solution:** Synchronized the `RCC_AHB2ENR` configuration to enable all three clock gates before peripheral initialization.

#### **3. Hardware Reset Sequence**
The Bluetooth module remains in a high-impedance state if the reset pin is not handled. 
**Solution:** Implemented a power-on sequence in `main()` that drives **PA8** High followed by a software delay to allow the BlueNRG-MS chip to stabilize before the first SPI header is sent.

### 📊 Verification Results
By sending the **BlueNRG Read Header (0x0B)**, the system successfully retrieves the hardware status.
- **Expected:** Non-zero, non-0xFF status byte.
- **Result:** `Bluetooth Status: 0xXX` (Verified over UART).