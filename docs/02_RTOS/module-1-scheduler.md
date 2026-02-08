---
layout: default
title: "01. Custom RTOS Scheduler"
parent: "Phase 2: Advanced RTOS & Scheduling"
nav_order: 2
---

# Module 01: Building a Round-Robin Scheduler

## 🎯 Strategic Objective
Develop a basic preemptive kernel that leverages the **SysTick** timer to multiplex CPU time across multiple independent threads of execution.



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
| **FPU Management** | Handling the **Lazy Stacking** of floating-point registers ($S0-S31$) to prevent data corruption during math-heavy tasks. |
| **Atomic Operations** | Ensuring the scheduler's linked list isn't corrupted if an interrupt occurs during a task swap. |

---

## 🛠 Deliverables
- [ ] **Context Switcher:** Assembly-level `PendSV_Handler` implementation.
- [ ] **Task Manager:** C-level logic to initialize the TCB array.
- [ ] **Wiki Documentation:** Detailed breakdown of the ARM Cortex-M4 register set.