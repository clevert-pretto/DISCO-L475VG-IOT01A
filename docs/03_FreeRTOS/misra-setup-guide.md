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
Maps the internal Cppcheck MISRA script to our rules text file downlaoded from [MISRA C 2012](https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/tools). I used [misra_c_2012__headlines_for_cppcheck%20-%20AMD1+AMD2.txt](https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/tools/-/blob/main/misra_c_2012__headlines_for_cppcheck%20-%20AMD1+AMD2.txt?ref_type=heads).

### C. Suppressions (`build_utils/cppcheck_suppressions.txt`)
Essential for silencing false positives from FreeRTOS kernel hooks and vendor HAL quirks without disabling the rules entirely.

## 3. VS Code Environment Configuration
To ensure MISRA rules and complexity constraints are visible during active development, the IDE must be strictly configured.

### A. Required Extensions
Install the following extensions in VS Code:

**1. C/C++ (ms-vscode.cpptools):** Core language support and Clang-Format engine.

**2. C/C++ Advanced Lint (jbenden.c-cpp-flylint):** Real-time Cppcheck and MISRA enforcement in the editor.

**3. Doxygen Documentation Generator (cschlosser.doxdocgen):** For standardized function headers.

### B. Workspace Settings (.vscode/settings.json)
By defining these settings at the workspace level, anyone opening this repository automatically inherits the project's formatting and linting rules. Create a folder named .vscode in the root and add a settings.json file

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