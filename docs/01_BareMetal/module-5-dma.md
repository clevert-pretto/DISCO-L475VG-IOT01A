---
layout: default
title: 05. DMA Acceleration
parent: Phase 1. Bare_Metal architecture - Cortex M4
nav_order: 5
---
# Module 05: DMA-Accelerated Telemetry

### 🧠 Strategic Objective
Reducing CPU overhead by offloading UART string transmissions to the **DMA1 (Direct Memory Access)** controller. This allows the processor to remain responsive for sensor math while communication happens in the background.

### ⚙️ Technical Specs
- **DMA Stream**: DMA1 Channel 4 mapped to USART1_TX.
- **Interrupts**: DMA Transfer Complete (TC) IRQ (Position 30 in vector table).
- **Synchronization**: `volatile` flag-based handshake between ISR and Main Loop.

### 🧱 Engineering Challenges & Solutions

#### **1. The "One-Shot" Hardware Trap**
During development, the DMA would successfully send the first packet but fail all subsequent attempts. I identified that the STM32 DMA controller requires a specific "Handshake" to restart:
1. The channel must be disabled in the `CCR` register.
2. **CRITICAL**: The **Transfer Complete (TC)** flag must be explicitly cleared in the `IFCR` register.
If these flags aren't cleared, the hardware state machine assumes the previous transfer is still active and refuses to trigger.

#### **2. CPU/DMA Synchronization**
Because DMA is asynchronous, there is a risk of the CPU overwriting the `dma_buffer` with new temperature data while the DMA is still reading it. I implemented a `dma_tx_complete` flag synchronization:
```c
while(!dma_tx_complete); // Ensure previous transfer is finished
float temp = hts221_get_temperature();
dma1_uart1_send(dma_buffer, len);