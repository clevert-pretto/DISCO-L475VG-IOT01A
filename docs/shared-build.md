---
layout: default
title: Build System (Makefile)
parent: Shared SDK Architecture
nav_order: 1
---
# Shared SDK Architecture

### 🛠 The Unified Build System
To ensure every project uses the same optimization (`-O0`) and safety flags (`-Wall`), I implemented a `base.mk` strategy.

**Benefits:**
* **Consistency:** Every binary produced shares the same Linker Script and memory layout.
* **Scalability:** Adding a new project only requires a 2-line Makefile.
* **Maintainability:** Fixes in the `startup.c` logic propagate to all projects instantly.