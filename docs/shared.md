---
layout: default
title: Shared SDK Architecture
parent: Phase 1. Bare-Metal architecture
nav_order: 99
has_children: true
---
# Core SDK Architecture
These components form the foundation of every project in this repository.


## 🏛 Core SDK Architecture (The `shared/` Layer)
These components form the foundation of every project in this repository. 

* **[Shared Startup Logic](./shared/shared-startup)**: Handles the Vector Table, Reset Handler, and Weak Interrupt Aliasing.
* **[Linker Specification](./shared/shared-linker)**: Defines the LMA/VMA memory regions for Flash and RAM.
* **[Memory Map](./shared/shared-map)**: Hardware abstraction of the STM32L475 MMIO registers.
* **[Modular Build System](./shared/shared-build)**: The `base.mk` framework for path-agnostic builds.

---