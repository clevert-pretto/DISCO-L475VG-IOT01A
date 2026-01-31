---
layout: default
title: Memory Map (MMIO)
parent: Shared SDK Architecture
nav_order: 2
---
# Memory Mapped I/O
Instead of magic numbers, I created `my_stm32_map.h`. 
- **RCC_BASE**: `0x40021000`
- **GPIOB_BASE**: `0x48000400`
- **SYSTICK_BASE**: `0xE000E010`