---
layout: default
title: "Phase 4: The C++ Paradigm Shift (Modern C++ for Constrained Systems)"
nav_order: 5
has_children: true
---

## 🎯 The Core Strategy:

Modern embedded programming you must prove three things:

1. You write scalable, safe Modern C++ (not just C).
2. You understand modern build systems (CMake) and validation (Python HIL).
3. You know modern RTOS ecosystems (Zephyr / Advanced FreeRTOS).


## 🎯 The Goal: 

Transition STM32 `FreeRTOS application` codebase from **procedural C** to **Object-Oriented C++**.

## 🗝️Key Concepts: 

- RAII (Resource Acquisition Is Initialization)
- Static Polymorphism (Templates/CRTP) vs. Dynamic Polymorphism (Virtual Functions), and constexpr for compile-time register math.

## 🗺️ Route Plan:

1. Refactor existing `03_FreeRTOS\App` into a C++ application.
2. Compile using `cmake`.



