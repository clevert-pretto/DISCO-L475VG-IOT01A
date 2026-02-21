---
layout: default
title: "03. Data Persistence & Command Line Telemetry"
parent: "Phase 3: Building a FreeRTOS application on STM32L475 from Scratch"
nav_order: 4
---

# Module 3: Data Persistence & Command Line Telemetry

## 📌 Project Overview
In this phase, the firmware evolved from a volatile real-time monitor into a robust **Data Logger**. The core objective was to implement a non-volatile storage engine utilizing the MX25R6435F QSPI Flash, simulating enterprise SSD block management and wear-leveling. Additionally, an interrupt-driven Command Line Interface (CLI) was built to retrieve telemetry without disrupting the real-time sensor acquisition.

---

## 🏗️ System Architecture

The architecture now includes a dedicated **Storage Abstraction Layer** and an **Asynchronous Command Listener**, protected by RTOS concurrency primitives.


### 1. The Storage Engine (NAND Simulation)
To maximize the lifespan of the silicon and ensure power-loss recovery, the storage layer mimics professional SSD firmware techniques:
* **Sector Metadata:** The first 16 bytes of the 4KB sector act as a packed header containing a `Magic Signature` (0x54414733), version control, and a persistent `Erase Count` to track hardware wear.
* **Page-Aligned Circular Buffer:** Sensor telemetry is packed into 16-byte `sStorageEvent_t` structures. To minimize Write Amplification, events are cached in RAM and flushed to flash only when a full "Page" (16 events) is assembled.
* **Auto-Format on Full:** Upon filling the sector boundary, the engine scans the flash, reads the historical wear, erases the sector, increments the `Erase Count`, and seamlessly wraps the write head.

### 2. Interrupt-Driven CLI
A dedicated `vCommandTask` was introduced to provide a diagnostic interface via UART.
* **Hardware-Offloaded RX:** Moving away from blocking polling, the UART is configured in interrupt mode. The CPU sleeps 100% of the time until a byte physically arrives.
* **Queue-Based Dispatch:** The ISR (`HAL_UART_RxCpltCallback`) safely pushes incoming keystrokes into an `xCommandQueue`, waking the Command Task to execute the requested routine.

| Command | Action | Execution Profile |
| :--- | :--- | :--- |
| **d** | Dump Flash Logs | Iterates through flash, decodes floats, and streams to terminal. |
| **p** | Bulk Erase | Executes QSPI Chip Erase, wiping all data except the Sector Header. |
| **s** | Stack Health | Queries RTOS for Task High Watermarks and remaining FreeRTOS Heap. |
| **n** | Erase event log sector | Erases event log sector of 4KB size |

---

## 🚦 Thread-Safe Hardware Access
With multiple tasks now requiring access to the same peripherals, **Mutexes** (`xSemaphoreCreateMutex`) became mandatory.
* **`xQSPIMutex`:** Ensures that if the CLI requests a Flash Dump while the Logger is flushing a page, the memory bus is strictly locked, preventing data corruption.
* **`xUART1Mutex`:** Prevents terminal scrambling by locking the serial bus, ensuring standard log messages and telemetry dumps never interleave.

---

## 🛠️ Engineering Journal & Lessons Learned

### 1. The Stack Overflow Trap (`vApplicationStackOverflowHook`)
* **Observation:** The system immediately crashed into a Hard Fault upon boot after adding the page buffers and string formatters.
* **Discovery:** Allocating large 256-byte `sStorageEvent_t` arrays and 128-byte string buffers on the local task stack instantly exhausted the `configMINIMAL_STACK_SIZE` limit.
* **Resolution:** Relocated large buffers to `static` memory (`.bss` section) to alleviate stack pressure. Implemented a dynamic "Stack Health" command using `uxTaskGetStackHighWaterMark()` to actively monitor memory margins.

### 2. The "Silent Killer" Wear-Leveling Bug
* **Observation:** The flash was wearing out 8x faster than anticipated; events were writing individually instead of waiting for a full 16-event page.
* **Discovery:** A mismatch in queue timeouts (`10ms` vs `1000ms` sensor reads) caused the `LogEventProcess` to time out constantly, triggering an immediate, premature flash write.
* **Resolution:** Adjusted the queue reception to `portMAX_DELAY` (with periodic polling for print messages), ensuring the task sleeps efficiently and only writes to silicon when the RAM page is genuinely full.

### 3. NVIC Priorities and Starvation
* **Observation:** Enabling the UART RX interrupt caused an immediate FreeRTOS configuration assertion.
* **Discovery:** The STM32 HAL sets default interrupt priorities to 0 (highest). FreeRTOS prohibits calling "FromISR" API functions from interrupts higher than `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` (Priority 5).
* **Resolution:** Safely relegated the USART1 and QUADSPI interrupts to Priority 6 in the NVIC, allowing the RTOS to preempt hardware streams safely.


### 4. MISRA-Compliant Formatter Overflows
* **Observation:** The custom `app_itoa` function produced corrupted Erase Counts and Magic Signatures during the terminal dump.
* **Discovery:** Concatenating the entire flash header into a single 128-byte array caused a buffer overflow. Additionally, the Magic Signature required Base-16 (Hex) formatting, while the custom `itoa` was hardcoded for Base-10.
* **Resolution:** Fragmented the UART transmission into smaller, safe line-by-line buffers and implemented strict radix handling for hexadecimal vs. decimal conversions.

---

## 🛠 Deliverables
* **Atomic Storage Engine:** A QSPI flash manager featuring metadata persistence, wear tracking, and power-loss recovery scanning.
* **Non-Blocking Telemetry CLI:** An interrupt-driven UART interface for live system diagnostics and hardware format commands.
* **Thread-Safe Shared Resources:** Implemented FreeRTOS Mutexes protecting the I2C, UART, and QSPI buses from concurrent access collisions.
* **Memory Diagnostics:** Integrated run-time heap and stack watermark monitoring to preemptively detect memory leaks or overflow risks.
* **v2.3** - Tag to refer.
* **[sample log file from UART Terminal](./flash-log-over-uart.txt)**
--- 

## 🚀 Future Roadmap: Module 4
With stable multitasking and persistent storage complete, the final phase will focus on **System Reliability & Hardware Offloading**:
1. **Interrupt-Driven Flash Erasure:** Implementing QSPI Auto-Polling to unblock the CPU during the 5+ second Flash Erase cycles, preventing Heartbeat starvation.
2. **Watchdog Timer (IWDG):** Developing a definitive failsafe to reset the microcontroller if the RTOS scheduler ever hangs or a task deadlocks.

---