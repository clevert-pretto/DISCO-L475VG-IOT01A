---
layout: default
title: Build System (Makefile)
parent: Shared SDK Architecture
nav_order: 1
---
# Unified Build System (`base.mk`)

To ensure consistent optimization and hardware acceleration across all projects, I utilized a shared Makefile logic.

### ⚙️ Compiler & Linker Configuration
* **Optimization**: `-O0` is used for debugging clarity and register-level visibility.
* **FPU Acceleration**: Flags `-mfloat-abi=hard` and `-mfpu=fpv4-sp-d16` enable the hardware FPU.
* **Standard Library Management**: Used `-nostdlib` but explicitly linked `-lm` (math) and `-lgcc` (helpers) to support floating-point to integer conversions.

### 🛠 VPATH Strategy
The build system uses a `VPATH` to pull in shared drivers automatically:
```makefile
COMMON_SRCS = startup.c uart.c i2c.c dma.c hts221.c