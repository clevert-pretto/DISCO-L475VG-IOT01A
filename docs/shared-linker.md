---
layout: default
title: Linker Specification
parent: Shared SDK Architecture
nav_order: 2
---
# Linker Script Architecture

### 🧠 The Core Concept: LMA vs. VMA
A Staff Engineer must distinguish between where code "lives" and where it "runs."
* **LMA (Load Memory Address):** The address in **Flash** where the data is stored permanently.
* **VMA (Virtual Memory Address):** The address in **SRAM** where the data is copied so the CPU can read/write to it during execution.



### 🧱 Memory Map Definitions
In `shared/linker.ld`, I defined the physical boundaries of the STM32L475VG:

```ld
MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
    SRAM  (rwx): ORIGIN = 0x20000000, LENGTH = 96K
}