---
layout: default
title: 06. Independent Watchdog (IWDG)
parent: Phase 1. Bare_Metal architecture (Cortex M4)
nav_order: 6
---
# Module 06: System Reliability & Fail-Safe Design

### 🧠 Strategic Objective
Implementing an **Independent Watchdog (IWDG)** to ensure system recovery in the event of software hangs, infinite loops, or hardware-induced deadlocks.

### ⚙️ Technical Specs
- **Peripheral:** IWDG (Independent Watchdog).
- **Clock Source:** LSI (Low Speed Internal) @ ~32kHz.
- **Timeout:** ~2000ms (Prescaler 64, Reload 1000).
- **Safety Mechanism:** Hardware-level System Reset on counter underflow.

### 🧱 Engineering Challenges & Solutions

#### **1. Clock Domain Isolation**
Unlike the Window Watchdog (WWDG), the **IWDG** is clocked by the **LSI**, making it independent of the main HSI16/PLL system clocks. This ensures that even if the main clock tree fails or the CPU enters a low-power state incorrectly, the watchdog remains functional as a "last line of defense".

#### **2. The "Key" Protection Mechanism**
The IWDG registers are protected against accidental writes. I implemented the standard access sequence:
1. Writing `0x5555` to the **Key Register (KR)** to unlock the Prescaler and Reload registers.
2. Configuring the timing.
3. Writing `0xAAAA` to "feed" the dog and lock the registers again.

#### **3. Testing System Recovery**
To verify the implementation, I intentionally created a "Simulated Hang" in the firmware:
```c
if (msTicks > 10000) {
    while(1); // Stop feeding the dog to trigger a hardware reset
}