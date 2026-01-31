# Module 02: Precision Timing & Interrupts

### 🧠 Strategic Objective
Transitioning from blocking "loop-based" delays to non-blocking, frequency-independent "hardware-timed" heartbeats.

### 🧱 Technical Deep-Dive
1. **The Vector Table Update:** Added the `SysTick_Handler` at index 15.
2. **Deterministic Ticks:** Configured the 24-bit down-counter for 1ms intervals based on the 4MHz MSI clock.
3. **The `volatile` Contract:** Declared `ms_ticks` as volatile to prevent the compiler from caching its value during the `while` loop, ensuring the main thread "sees" the change made by the interrupt handler.

### ⚙️ Register Configuration
* `SYSTICK->LOAD`: Calculated as `(CPU_FREQ / 1000) - 1`.
* `SYSTICK->CTRL`: Set to `0x7` (Internal Clock, Interrupt Enable, Counter Enable).