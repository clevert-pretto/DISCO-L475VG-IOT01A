---
layout: default
title: "Phase 2: Advanced RTOS & Scheduling"
nav_order: 4
has_children: true
---

# Phase 2: Advanced RTOS & Scheduling

## 🧠 The Mission
Transitioning from simple **Super-Loop** architectures to a deterministic, **Multi-threaded Preemptive Kernel**. This phase focuses on the "under the hood" mechanics of how an OS manages the CPU.

---

## 🛠 Core Objectives

### 1. Context Switching
Mastering the transition between tasks by manually saving and restoring CPU registers using the **PendSV** exception.

### 2. Priority Management
* **Scheduling Logic:** Implementing Fixed-Priority and Round-Robin algorithms.
* **Priority Inversion:** Simulating the "Unbounded Priority Inversion" bug and implementing **Priority Inheritance** as a professional-grade fix.

### 3. Memory Safety & Analysis
* **Stack Guard Bands:** Implementing magic-number watermarking to detect stack growth.
* **MPU Integration:** Using the Memory Protection Unit to trigger a `HardFault` on stack overflows.

---

## 🚀 Key Milestones
1.  **The Custom Scheduler**: A "from-scratch" implementation on the STM32L475.
2.  **Multicore Concepts**: Introduction to SMP (Symmetric Multiprocessing) and Spinlocks.