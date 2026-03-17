---
layout: default
parent : "Phase 4: The C++ Paradigm Shift (Modern C++ for Constrained Systems)"
title: "2: DevOps & Testing Journey: Abstraction, Unit testing, Coverage, Github actions"
nav_order: 2
---

# The Journey: From Bare-Metal to Automated DevOps 🚀

Developing embedded software for the **STM32L475** (DISCO-L475VG-IOT01A) traditionally involves tightly coupling application logic to hardware-specific libraries (HAL) and RTOS primitives. While this works for simple projects, it makes unit testing without the physical board nearly impossible.

This document outlines our journey of modernizing the `04_FreeRTOS_Cpp` project: introducing hardware abstraction, implementing host-based unit testing, and establishing a rigorous CI/CD pipeline.

---

## Phase 1: Hardware Abstraction & Testability
To test our application logic (like `appHeartbeat.cpp` and `xtoa.cpp`) on a standard PC, we had to sever the hard dependencies on the STM32 hardware and FreeRTOS.

1. **Interface Injection:** Instead of calling `HAL_GPIO_TogglePin()` directly inside our heartbeat task, we created abstract C++ interfaces (e.g., `IGpio`). 
2. **Dependency Inversion:** The application logic now accepts an interface pointer. On the real hardware, we pass a concrete STM32 implementation. During testing, we pass a "Mock" object.
3. **Isolating Utility Functions:** Utility files like `xtoa.cpp` (integer-to-string conversion) were decoupled from any hardware dependencies, making them pure C++ functions that are highly testable.

---

## Phase 2: The Dual-Build Toolchain
We structured our build system to compile the code for two entirely different targets using **CMake**.

* **Target Build (ARM Cortex-M4):** Uses the `arm-none-eabi-gcc` cross-compiler to generate the `.elf`/`.bin` files for the physical STM32 board.
* **Host Build (x86/x64 PC):** Uses the native GCC compiler to build the application logic alongside our test suite. 
    * We used CMake's `FetchContent` to automatically download and link **GoogleTest** (GTest) and **GoogleMock**.
    * We added GCC coverage flags (`--coverage -fprofile-arcs -ftest-coverage`) exclusively to the host build to track which lines of code our tests actually hit.

---

## Phase 3: Unit Testing & Code Coverage

We achieved 100% test coverage for the application logic by leveraging host-based unit testing.

1. Shared Mock Infrastructure
To maintain a DRY (Don't Repeat Yourself) architecture, we established a shared `MockInterfaces.hpp`. This allows all test suites (`test_sysManager`, `test_appLogger`, etc.) to share the same simulated RTOS and Hardware environment.

2. The "Private" Testing Pattern
To test internal state machines and private methods without compromising production encapsulation, we utilized a preprocessor macro:
* In **Production**, members remain `private`.
* In **Unit Tests**, members become `public` via a `PRIVATE_FOR_TEST` macro defined in `CMakeLists.txt`. This allows the test harness to verify internal states like `currentState` or `handleCommand()`.


3. **Zero-Hardware Dependency**: The application logic layer is 100% decoupled from the STM32 hardware via abstract interfaces.

4. **Automated Analysis**: The project includes automated targets for static analysis (Cppcheck) and code formatting (Clang-Format).

To visualize our testing effectiveness, we automated the code coverage reporting:
1. **LCOV & GenHTML:** After GTest runs, we capture the `.gcda` files using `lcov` and generate a line-by-line HTML heat map.
2. **Dashboard Script:** We created a Python script (`generate_dashboard.py`) to parse the GoogleTest XML results and the LCOV data into a single, unified `index.html` dashboard.

---

## Phase 4: Local Quality Gates (Pre-Commit)
To prevent bad code from ever leaving our local machines, we instituted strict, automated quality gates using **pre-commit**.

We configured a `.pre-commit-config.yaml` to enforce two major checks before any `git commit` succeeds:
1. **Static Analysis (Cppcheck):** Enforces modern C++17 safety standards on the new project, while applying MISRA C:2012 rules to the legacy `03_FreeRTOS` project.
2. **Maintainability (Lizard):** Scans all C/C++ files to ensure functions don't exceed a Cyclomatic Complexity (CCN) of 10 or a length of 15 lines.

To ensure a seamless developer experience, we built a `bootstrap.sh` script that automatically sets up the Python virtual environment (`.venv`), installs the tools, and binds the git hooks.

---

## Phase 5: Continuous Integration (GitHub Actions)
The final step was mirroring our local quality gates in the cloud. We wrote a GitHub Actions workflow (`ci_quality.yml`) that acts as our ultimate source of truth.



**The Pipeline Workflow:**
1. **Matrix Strategy:** The pipeline dynamically runs checks for both `03_FreeRTOS` (Legacy) and `04_FreeRTOS_Cpp` (Modern).
2. **Cloud Linting:** Executes `cppcheck` and `lizard` to verify standards.
3. **Cloud Testing:** Compiles the host build, runs GoogleTest, and generates the LCOV coverage maps.
4. **Artifacts & Jekyll:** The Python dashboard script packages the results. The pipeline then downloads these HTML reports and natively embeds them into this Jekyll documentation site (`_site/reports`).

***

## 📖 Embedded DevOps Glossary

* **CI/CD (Continuous Integration / Continuous Deployment):** The practice of automating the building, testing, and deployment of code every time a change is made to the repository.
* **CCN (Cyclomatic Complexity Number):** A software metric used to indicate the complexity of a program. It measures the number of linearly independent paths through a program's source code (e.g., the number of `if/else` branches). Lower is better.
* **GoogleTest / GoogleMock:** A C++ testing and mocking framework. It allows us to simulate (mock) hardware responses to test how our application logic reacts without needing the physical board.
* **HAL (Hardware Abstraction Layer):** A layer of software that hides the underlying physical hardware details from the application code, making the application portable and testable.
* **LCOV:** A graphical front-end for GCC's coverage testing tool (gcov). It highlights exactly which lines of C++ code were executed during unit tests.
* **MISRA C/C++:** A set of software development guidelines for the C and C++ programming languages developed by the Motor Industry Software Reliability Association. Its aims are to facilitate code safety, security, portability, and reliability in the context of embedded systems.
* **RTOS (Real-Time Operating System):** An operating system designed to process data as it comes in, typically without buffering delays (e.g., FreeRTOS).
* **Toolchain:** A set of programming tools used to perform complex software development tasks. In our case, CMake, `arm-none-eabi-gcc` (target), and native `g++` (host).