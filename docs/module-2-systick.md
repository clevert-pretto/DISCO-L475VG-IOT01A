# Module 02: Precision Timing & Interrupts

### 🧠 The "Why"
Loop-based delays (`delay(500000)`) are frequency-dependent and block the CPU. To build an RTOS, we need a frequency-independent "Heartbeat."

### 🧱 Architectural Pillars
1. **SysTick Peripheral:** A 24-bit down-counter built into the ARM Core. 
   

2. **Interrupt Service Routines (ISRs):** Updating the Vector Table at index 15 to handle the `SysTick_Handler`.

3. **Atomicity & Volatile:**
   Using the `volatile` keyword to prevent compiler optimization on variables changed within an interrupt context.