---
layout: default
title: 03. Interrupt-Driven UART
parent: Phase 1. Bare_Metal architecture (Cortex M4)
nav_order: 3
---
# Module 03: Asynchronous Serial Communication

### 🧠 Strategic Objective
Establishing a robust telemetry and command interface using the ST-Link Virtual COM Port (VCP) while managing concurrent hardware tasks.

### 🛠 The "Gotcha": Hardware Discovery
A key architectural discovery in this module was the physical routing of the **B-L475E-IOT01A** board. Unlike many STM32 boards where UART2 is the default VCP, this board routes the ST-Link bridge to **USART1** on **PB6 (TX)** and **PB7 (RX)**. 

### ⚙️ Technical Implementation
* **Clock Tree:** Transitioned from the default 4MHz MSI to the **16MHz HSI** (High-Speed Internal) oscillator to ensure precise baud rate timing at 115,200 bps.
* **NVIC Integration:** Configured the Nested Vectored Interrupt Controller to enable **IRQ 37** (USART1). This required targeting `NVIC_ISER1` (bit 5) to unmask the interrupt at the processor core level.
* **Deterministic Multi-Tasking:** Leveraged the SysTick-based `msTicks` counter to manage three independent timing domains:
  1. **Heartbeat:** 500ms LED toggle.
  2. **Telemetry:** 1000ms periodic transmission of 'A'.
  3. **Event Response:** Immediate "Double-Blink" (125ms states) triggered by the UART RX interrupt.

### 🚀 Key Learning: Interrupt-Driven RX
By enabling the `RXNEIE` (Read Data Register Not Empty Interrupt Enable) bit, the CPU no longer wastes cycles polling the UART status register. The hardware automatically context-switches to the `USART1_IRQHandler` only when data is ready, allowing for a high-performance, non-blocking echo loopback.