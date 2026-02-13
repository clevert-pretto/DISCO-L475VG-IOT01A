---
layout: default
title: "01. Custom Preemptive Round-Robin Scheduler"
parent: "Phase 2: Building custom Schedulers"
nav_order: 2
---

# Module 01: Building a Round-Robin Scheduler

## 🎯 Strategic Objective
Develop a basic **static, preemptive, Round-Robin Real-Time kernel** that leverages the **SysTick** timer to multiplex CPU time across multiple independent threads of execution.

Following are the aspects of system we developed:

**1.Preemptive** - Unlike a "Cooperative" system (where a task must voluntarily give up control), our system is Preemptive. The SysTick interrupt acts as a "hard" authority that pauses a task mid-execution to let another one run. This is the foundation of modern real-time systems

**2. Round-Robin** - Because TCBs (Task Control Blocks) are linked in a circle (tcbA -> tcbB -> tcbA), the scheduling algorithm is Round-Robin. Every task gets an equal "slice" of time (1ms in our case) before the baton is passed to the next player.

**3. Static** - The memory for the tasks (the stacks and TCB structures) is allocated at compile-time.

Traits of RTOS :

*Static RTOS:* High reliability, no memory fragmentation, used in safety-critical systems (like SSD controllers or medical devices).

*Dynamic RTOS:* Allows Task_Create() at runtime (like FreeRTOS), which is more flexible but carries a risk of "Out of Memory" errors during execution.

**4. Bare-Metal** - Since we are communicating directly with the ARM Cortex-M4 registers without an intermediate abstraction layer (like a HAL or a standard library), this is a Bare-Metal Implementation. This gives you the lowest possible latency and the smallest binary size.

---

### 🛡️ Why we call it a "Kernel" and not yet a full "OS"

To transition from a Kernel to a full Operating System, we would typically add "Middleware" services:

**1. Inter-Process Communication (IPC):** Semaphores, Mutexes, and Mailboxes.

**2. Memory Management:** Dynamic heaps or memory pools.

**3. Device Drivers:** Standardized ways for tasks to talk to UART, SPI, or I2C.

--- 

## ⚙️ Technical Focus
### The "Big Three" of Cortex-M Schedulers
* **PendSV (Pended Service Call):** The designated interrupt for context switching. It is set to the lowest priority to ensure it doesn't block time-critical ISRs.
* **Stack Framing:** Manually "faking" a stack frame for new tasks so they can be "returned to" by the CPU.
* **TCB (Task Control Block):** The data structure that serves as the "passport" for a task, storing its `StackPointer`, `State`, and `Priority`.

---

## 🧱 Engineering Challenges

| Challenge | Technical Nuance |
| :--- | :--- |
| **Reset vs. Switch** | Distinguishing between the initial boot-up (MSP) and task switching (PSP). |
| **FPU Management** | Handling the **Lazy Stacking** of floating-point registers `S0-S31` to prevent data corruption during math-heavy tasks. |
| **Atomic Operations** | Ensuring the scheduler's linked list isn't corrupted if an interrupt occurs during a task swap. |

---

## 🛠 Deliverables
- [x] **Context Switcher:** Assembly-level `PendSV_Handler` implementation.
- [x] **Task Manager:** C-level logic to initialize the TCB array.
- [x] **Wiki Documentation:** Detailed breakdown of the ARM Cortex-M4 register set.

---