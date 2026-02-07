---
layout: default
title: 01. Bare Metal Blinky
parent: Phase 1. Bare_Metal architecture - Cortex M4
nav_order: 1
---
# Module 01: The Silicon Handshake

### 🧠 Strategic Objective
Establishing the first line of code execution on raw silicon without vendor abstraction.

### 🧱 Implementation Highlights
* **Manual Memory Zeroing:** Implemented the `.bss` loop to ensure a zeroed state for uninitialized globals.
* **Data Relocation:** Managed the LMA (Flash) to VMA (RAM) copy for initialized variables.
* **Direct Register Access:** Used the `RCC` and `GPIO` memory-mapped addresses to drive the LD2 Green LED.

**Refactor Note:** This project now inherits its boot logic from the global `shared/startup.c`.

