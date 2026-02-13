---
layout: default
title: "02. Custom Priority-Based Preemptive Scheduler"
parent: "Phase 2: Building custom Schedulers"
nav_order: 3
---

# Module 02: Building a Priority-Based Preemptive Scheduler

## 🎯 Strategic Objective
Transition from a fixed-slice Round-Robin model to a **Priority-Based Preemptive Kernel**. 

This architecture ensures that the highest-priority task in the `READY` state always occupies the CPU, a fundamental requirement for deterministic systems.

Following are the advanced aspects of the system we developed:

**1. Priority Preemptive** - In this model, the scheduler constantly evaluates which task is the most important. If a high-priority task wakes up, it immediately preempts the current lower-priority task, regardless of how much time was left in its "slice."

**2. State-Based Scheduling** - Tasks are no longer always "Ready." We implemented a State Machine (`TASK_READY`, `TASK_BLOCKED`) that allows tasks to yield the processor while waiting for events or timers.

**3. Non-Blocking Delays** - We moved away from "Spin-Wait" delays (`Delay_ms()`) to a non-blocking `Task_Sleep()`. This allows the CPU to perform useful work on other tasks while one task is "sleeping."

**4. The Idle Task** - To prevent system crashes when all tasks are sleeping, we implemented a background Idle Task. This task has the lowest priority and ensures the scheduler always has a valid thread to run.


## ⚙️ Technical Focus

### The "Highest Priority Ready" (HPR) Algorithm
The core of the scheduler is now a **selection search**. Instead of just moving to the `nextPtr`, the kernel scans the TCB linked list every millisecond(in `SysTick_Handler()`) to find the task with the lowest priority number (*0* = Highest Priority).

### Task State Transitions
A task now moves through a lifecycle. When `Task_Sleep(ticks)` is called, the task is moved to the `BLOCKED` state. The `SysTick_Handler` decrements the `sleepTicks` every millisecond. Once it hits zero, the task is promoted back to `READY`.

---

## 🧱 Engineering Challenges

| Challenge | Technical Nuance |
| :--- | :--- |
| **HardFault Loops** | Managing the "No Ready Task" scenario. Without an Idle Task, the scheduler would return a NULL pointer, causing a crash. |
| **Determinism** | Minimizing Jitter. By using priority, we ensure that critical tasks (like NAND Flash completion) are handled with predictable latency. |
| **Resource Contention** | Observing what happens when two tasks with different priorities fight over a single hardware resource (PA5 LED). |

---

## 🛠 Deliverables
- [x] **State Machine:** Implementation of TASK_READY and TASK_BLOCKED logic.
- [x] **Priority Search:** *O(n)* selection algorithm in Scheduler_SelectNext.
- [x] **Non-Blocking Sleep:** Task_Sleep implementation to replace blocking loops.
- [x] **Idle Task:** Lowest-priority fallback task for 100% CPU uptime.

---