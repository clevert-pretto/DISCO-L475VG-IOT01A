---
layout: default
title: "Lesson learned"
parent: "Phase 2: Building custom Schedulers"
nav_order: 99
has_children: false
---

# Phase 2: Lesson learned

## 📌 Project Overview
Phase 2 focused on the core logic of task management and CPU orchestration. I evolved the kernel from a basic **Round-Robin** approach (equal time-slicing) to a **Priority-Based Preemptive** model. This transition is fundamental for real-time systems like SSD controllers, where critical I/O operations must interrupt background maintenance tasks.

---

## 🏗️ Key Architectural Concepts

### 1. Round-Robin Scheduling
The first iteration used a "fairness" model. Every task in the system was treated equally, and the scheduler moved to the next task in a circular linked list on every `SysTick`.
* **Mechanism:** `currentTask = currentTask->nextPtr;`
* **Limitation:** In a Round-Robin system, a "High Priority" emergency task must wait its turn behind every other task, which is unacceptable for deterministic real-time requirements.

[Image of Round-Robin scheduling process in RTOS]

### 2. Priority-Based Preemption
I modified the TCB (Task Control Block) to include a `priority` field. The scheduler was updated to scan the task list and always select the task with the highest priority (the lowest numerical value).
* **Preemption:** If a high-priority task wakes up (e.g., its sleep timer expires), it immediately triggers a `PendSV` to preempt the lower-priority task currently on the CPU.
* **Mechanism:** A search algorithm scans the linked list to find the highest-priority task in the `READY` state.

---

## 🛠️ Implementation Challenges & Lessons Learned

### 1. The "Bootstrap" Race Condition
**The Problem:** The system would crash or hang immediately after enabling interrupts (`__enable_irq()`).
**The Lesson:** Enabling the hardware timer (`SysTick`) before the software environment is 100% consistent is a "Kernel Killer." If a `SysTick` occurs while the CPU is still using the **Main Stack Pointer (MSP)** but the handler expects to save context to the **Process Stack Pointer (PSP)**, the system triggers a HardFault.
**The Fix:** 1. Initialize all TCBs and Stacks first.
2. Manually point the `PSP` to the first task's stack.
3. Initialize `SysTick` as the absolute last step before turning on interrupts.

[Image of ARM Cortex-M4 stack pointer selection using CONTROL register]

### 2. Context Switching Logic (Assembly)
**The Problem:** Register corruption caused tasks to behave unpredictably or crash after a switch.
**The Lesson:** ARM Cortex-M hardware only saves "caller-saved" registers ($R0-R3, R12, LR, PC, xPSR$) automatically. The "callee-saved" registers ($R4-R11$) must be handled manually in the `PendSV_Handler` via Assembly.
**The Fix:** Implemented `STMDB` (Store Multiple Decrement Before) to save the software context and `LDMIA` (Load Multiple Increment After) to restore it.

[Image of ARM Cortex-M4 stack frame organization including hardware and software saved registers]

### 3. Task Starvation
**The Problem:** When I added a high-priority task that never called `Task_Sleep`, lower-priority tasks (like the UART telemetry) never ran.
**The Lesson:** Priority scheduling requires "polite" tasks. If a high-priority task doesn't "yield," it will starve the rest of the system.
**The Fix:** Ensured all high-priority tasks perform non-blocking operations or use `Task_Sleep` to allow lower-priority background tasks to execute.

---

## 📈 Technical Achievements
* **Preemptive Kernel:** Successfully implemented a system where task switching is triggered by both time (SysTick) and software events (Yielding/Sleep).
* **Manual Context Management:** Wrote bare-metal Assembly to manage the ARM Cortex-M4 register bank manually.
* **UART Telemetry:** Integrated a serial monitoring system to verify that tasks were being scheduled according to their assigned priorities.

---

## 🚀 Transition to FreeRTOS
Having mastered the "How" of scheduling via bare-metal coding, Phase 3 involves moving to **FreeRTOS**. This will allow the use of industry-standard features like **Mutexes**, **Queues**, and **Priority Inheritance**, shifting the focus from kernel-building to application-level firmware development.

---