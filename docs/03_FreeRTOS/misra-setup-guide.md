---
layout: default
title: "Establishing MISRA C & Complexity Quality Gates"
parent: "Phase 3: Building a FreeRTOS application on STM32L475 from Scratch"
nav_order: 99
---

# Establishing MISRA C & Complexity Quality Gates

## Overview
This document outlines the step-by-step process used to establish a professional-grade static analysis and complexity pipeline for an STM32 FreeRTOS project. The goal is to enforce **MISRA C:2012** compliance and maintain a Cyclomatic Complexity Number (CCN) of under **10**, meeting the high-reliability standards required in domains like semiconductor and automotive firmware.

## 1. Directory Architecture
To keep the project root clean and mimic enterprise-level repositories, all configuration files are isolated in a dedicated directory (w.r.t 03_FreeRTOS project):

```text
. (Root)
├── 03_FreeRTOS/         # Source code
├── docs/                # Github pages
├── .github/             # GitHub Actions workflows
└── build_utils/         # Quality gate configurations
    ├── .pre-commit-config.yaml
    ├── misra_rules.json
    ├── misra_rules.txt
    └── cppcheck_suppressions.txt
```

## 2. CI/CD Configuration Files

### A. Pre-Commit Configuration (build_utils/.pre-commit-config.yaml)
We utilize pre-commit to manage both cppcheck (for MISRA) and lizard (for complexity). Both are run as local hooks to avoid GitHub repository tag issues and ensure stable Python environments.

### B. MISRA Mapping (`build_utils/misra_rules.json`)
Maps the internal Cppcheck MISRA script to our rules text file downlaoded from [MISRA C 2012](https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/tools). I used [misra_c_2012__headlines_for_cppcheck - AMD1+AMD2.txt](https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/tools/-/blob/main/misra_c_2012__headlines_for_cppcheck%20-%20AMD1+AMD2.txt?ref_type=heads).

### C. Suppressions (`build_utils/cppcheck_suppressions.txt`)
Essential for silencing false positives from FreeRTOS kernel hooks and vendor HAL quirks without disabling the rules entirely.

### D. Silencing Unused Analyzers
The "C/C++ FlyLint" extension is a "multi-analyzer". By default, it tries to activate every tool it supports to give you the most coverage. To prevent "Unable to activate" warnings (for Flawfinder, flexlint, clang analyser), explicitly disable analyzers not included in the project's specific quality gate. **In `.vscode/settings.json`.
Because my project is specifically tuned for MISRA C compliance using Cppcheck, having these other tools active creates "noise" and "line could not be parsed" errors because they don't understand your custom

## 3. VS Code Environment Configuration
To ensure MISRA rules and complexity constraints are visible during active development, the IDE must be strictly configured.

### A. Required Extensions
Install the following extensions in VS Code:

**1. C/C++ (ms-vscode.cpptools):** Core language support and Clang-Format engine.

**2. C/C++ Advanced Lint (jbenden.c-cpp-flylint):** Real-time Cppcheck and MISRA enforcement in the editor.

**3. Doxygen Documentation Generator (cschlosser.doxdocgen):** For standardized function headers.

### B. Workspace Settings (.vscode/settings.json)
By defining these settings at the workspace level, anyone opening this repository automatically inherits the project's formatting and linting rules. Create a folder named .vscode in the root and add a settings.json file

### C. Task Automation (`.vscode/tasks.json`)
To eliminate terminal context-switching, the quality gate is mapped to the default VS Code build command. 

Created a task labelled as `Run Quality Gate (MISRA & Complexity)` and added `.vscode/tasks.json`.

## 4. Workflow Execution
To initialize or reset the environment locally:

1. Clear the cache: `pre-commit clean`

2. Install hooks to git: `pre-commit install --config build_utils/.pre-commit-config.yaml`

3. Run manually across all files: `pre-commit run --all-files --config build_utils/.pre-commit-config.yaml`

## Key Learnings & Troubleshooting

### Learning 1: Isolating Vendor Code (The "Clutter" Problem)
**The Struggle:** Initial scans failed massively because the linter was analyzing STMicroelectronics HAL and FreeRTOS middleware files, which are not strictly MISRA compliant.
**The Solution:** Use the files: `^03_FreeRTOS/App/` regex in the pre-commit config to explicitly restrict static analysis to user-written application code.

### Learning 2: Managing RTOS "Unused" Hooks
**The Struggle:** Cppcheck flagged critical FreeRTOS callback functions (like vApplicationStackOverflowHook) as unused because they are called by the kernel/hardware, not by the user application.
**The Solution:** Explicitly suppress unusedFunction for these specific signatures in cppcheck_suppressions.txt and provide forward declarations in main.c to satisfy MISRA Rule 8.4.

### Learning 3: Unmatched Suppressions Break Strict Builds
**The Struggle:** After fixing all MISRA errors, the build still failed because Cppcheck reported "Unmatched suppressions" (rules we told it to ignore that it didn't find). Because we used --error-exitcode=1, this informational warning halted the pipeline.
**The Solution:** Add unmatchedSuppression to the suppressions list.

### Learning 4: Toolchain Stability (The Lizard Tag Issue)
**The Struggle:** Pulling lizard directly from GitHub using a rev tag caused InvalidManifestError and pathspec failures because the repository did not contain the required .pre-commit-hooks.yaml manifest.
**The Solution:** Switched lizard to a local hook utilizing language: python and additional_dependencies. This forces pre-commit to build an isolated virtual environment and pull the stable package directly from PyPI, ensuring maximum stability.

### Learning 5: Real-World MISRA Strictness
Enforcing MISRA C completely changes how C code must be written:
**Scope:** Variables used in a single function must have block scope (Rule 8.9).
**Pointers:** You cannot cast arbitrary integers (like a delay of 1000) to (void *) to pass them into RTOS tasks (Rule 11.6).
**Braces:** Single-line while(1) or for loops must have compound braces { } (Rule 15.6) to prevent accidental logic bypasses.

### Learning 6: Exact Type Matching for Kernel Hooks
**The Struggle:** The build failed with `conflicting types for 'vApplicationGetIdleTaskMemory'`.
**The Cause:** In high-reliability C, `uint32_t` and `size_t` are not interchangeable in function signatures, even if they share the same bit-width. 
**The Solution:** Always verify the kernel's expected prototype in `task.h` and ensure the application's hook implementation matches the exact typedef (e.g., using `uint32_t` vs `size_t`).

### Learning 7: Cppcheck Library Configuration Errors
**The Struggle:** The pipeline failed with `Failed to load library configuration file 'free_rtos'`.
**The Cause:** Some Cppcheck installations do not include the `free_rtos.cfg` file, or use a different naming convention (`freertos`).
**The Solution:** Instead of relying on built-in library configurations, provide explicit include paths for the RTOS headers and use wildcard suppressions (`*:*Middlewares/*`) to ignore non-compliant vendor code. This ensures the tool remains portable across different developer machines and CI environments.

### Learning 8: FlyLint Setting Deprecation
**The Struggle:** VS Code reported `Unknown Configuration Setting` for `suppressionFiles`.
**The Cause:** Extension updates often change specialized keys. 
**The Solution:** Use `c-cpp-flylint.cppcheck.extraArgs` to manually pass the `--suppressions-list` flag. This is more robust as it guarantees the IDE linter uses the exact same suppression logic as the CI/CD pipeline.

### Learning 9: Parser Noise & Progress Reports
**The Struggle:** VS Code extension crashed with `Error: Line could not be parsed: X% done`.
**The Cause:** The FlyLint extension expects every line of Cppcheck output to be an error message. Progress updates from the tool break the extension's parser.
**The Solution:** Add `--quiet` to the `extraArgs` in VS Code settings. This ensures Cppcheck only outputs actionable MISRA violations, keeping the IDE integration stable.

### Learning 10: The Void Comparison Trap (Rule 17.7)
**The Struggle:** Error: `expression must have arithmetic type but has type "void"`.
**The Cause:** Attempting to cast a function to `(void)` while simultaneously using it in an `if` statement.
**The Solution:** MISRA compliance doesn't mean you *must* use `(void)` on everything. It means you must not *ignore* the result. The correct pattern for RTOS APIs is to capture the `BaseType_t` result in a local variable and then perform the logic check on that variable.

### Learning 11: Peripheral Identifier Undefined (IntelliSense)
**The Struggle:** Error: `identifier "USART1" is undefined`.
**The Cause:** The IDE does not know which hardware abstraction layer to load because the specific chip family macro (e.g., `STM32L475xx`) is missing from the workspace configuration.
**The Solution:** Add the chip series and `USE_HAL_DRIVER` to the `defines` section of `c_cpp_properties.json`. This enables the conditional inclusion of peripheral register maps in the CMSIS headers.

### Learning 12: Pointer Casting and Vendor APIs
**The Struggle:** MISRA violation: `C-style pointer casting`.
**The Cause:** MISRA Rule 11.3 and 11.8 prohibit or restrict casting between different pointer types (e.g., `char *` to `uint8_t *`) to prevent memory aliasing issues.
**The Solution:** Ideally, define data structures using the exact types required by the hardware abstraction layer (HAL). When interfacing with read-only strings, use explicit casts or local suppressions to acknowledge the deviation from strict type safety for peripheral I/O.

### Learning 13: Understanding Checker Coverage (132/592)
**The Observation:** Cppcheck reports that only a subset of checkers (e.g., 132/592) are active.
**The Reality:** This is expected behavior. The total count includes C++ specific rules, experimental checkers, and premium addons that do not apply to a pure C MISRA project.
**The Professional Approach:** Use the `--checkers-report=<filename>` flag to audit the active ruleset. This audit file serves as evidence of compliance during a "Code Quality Review" or "Safety Audit."

