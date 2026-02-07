---
layout: default
title: 02. Precision Timing
nav_order: 4
---
# Module 02: Hardware-Accelerated Precision Timing

### 🧠 The Evolution
Project 01 used software delay loops which were blocking and non-deterministic. Project 02 implements a **Global Heartbeat** using the ARM SysTick timer.

### ⚙️ Technical Specs
- **Clock Source:** MSI @ 4MHz.
- **Interrupt Frequency:** 1000Hz (1ms).
- **Control Register:** `0x7` (Processor Clock, Exception Enable, Counter Enable).

### 🚀 Key Learning: The `volatile` Keyword
The `ms_ticks` variable is shared between the **Background** (Interrupt Context) and the **Foreground** (Main Loop). Declaring it as `volatile` prevents the compiler from optimizing out "redundant" reads, ensuring the `delay_ms()` function always sees the most recent tick count.