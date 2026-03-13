---
layout: default
title: Monorepo Development & Quality Infrastructure
Parent: "Code Quality Metrics"
nav_order: 2
---


# 🛠️ Monorepo Development & Quality Infrastructure

This repository manages two distinct FreeRTOS firmware projects with a shared, high-integrity tooling infrastructure.

## 📂 Repository Architecture

| Component | Path | Description |
| :--- | :--- | :--- |
| **C Project** | `03_FreeRTOS/` | Legacy C implementation using MISRA C:2012.|
| **C++ Project** | `04_FreeRTOS_Cpp/` | Modern C++ implementation using Native C++ Static Analysis.|
| **Tooling Hub** | `build_utils/` | Centralized Linter rules, Suppressions, and Pre-commit configs.|
| **VS Code Config** | `.vscode/` | Shared Build Tasks and Cortex-Debug profiles.|

---

## 🚀 Build & Debug Environment

We use **VS Code** as a unified IDE for both projects. Build tasks are context-aware using the `cwd` (Current Working Directory) attribute. 

### VS Code Tasks (`tasks.json`)
- **Build 03_FreeRTOS**: Executes `make` within the C project root. 
- **CMake Build 04_FreeRTOS_Cpp**: Handles CMake generation and parallel build (`make -j`) for the C++ project. 
- **Run Quality Gate**: Triggers the local `pre-commit` suite against all files. 

### Debugging (`launch.json`)
- **Cortex-Debug**: Separate profiles for each project point to their respective `.elf` artifacts. 
- **GDB Path**: Explicitly mapped to `/usr/bin/gdb-multiarch` for WSL2 compatibility. 

---

## 🛡️ Quality Gate & Static Analysis

To prevent "Rule Friction" between C and C++ standards, the analysis pipeline is split into parallel hooks. 

### 1. Cppcheck (Split Strategy)
- **C Hook**: Applies strict **MISRA C:2012** headlines.
- **C++ Hook**: Utilizes the **Native Cppcheck Engine** (`--enable=all`) with `--std=c++17`. This resolves false positives related to C++ Initializer Lists (Rule 12.3) and Namespace usage.
- **Suppressions**: Managed via `build_utils/cppcheck_suppressions.txt` to handle FreeRTOS task entry points and HAL callbacks. 

### 2. Code Metrics (Lizard)
- Enforces a **Cyclomatic Complexity (CCN) < 10** across both projects. 
- Limits function length to **15 lines** to ensure maintainable C++ class methods. 

### 3. Git Hooks (Pre-commit)
The hooks are force-linked to our subfolder configuration to ensure every commit is audited:
```bash
pre-commit install --config build_utils/.pre-commit-config.yaml --overwrite
```

## 📈 Scaling the Monorepo

To add a new firmware project to this ecosystem:
1. **Create Folder**: Use the `XX_ProjectName` convention.

2. **Update Matrix**: Add the folder name to `.github/workflows/ci_quality.yml`.
    **Matrix Update**: Simply add the new folder name to the project_path list in ci_quality.yml:
    ```YAML
    strategy:
    matrix:
        project_path: ["03_FreeRTOS", "04_FreeRTOS_Cpp", "05_NewProject"]
    ```
    **Logic**: The existing if/else bash logic will handle it automatically, applying C or C++ rules based on your folder naming convention.

3. **Register Hook**: Add a project-specific Cppcheck hook in `build_utils/.pre-commit-config.yaml`.
    1. **Cppcheck**: Add a new hook entry in `.pre-commit-config.yaml`.
        Set files: `^05_NewProject/App/`.
        Add the new `-I` include paths to the args section.
    2. **Lizard**: Update the regex in the Lizard hook to include the new folder: `^(03|04|05)_Project/App/`.

4. **CWD Mapping**: Add a new build task and launch profile in `.vscode/` using the `cwd` parameter.
    1. **Tasks**: Duplicate an existing build task in tasks.json and update the label and cwd (path) to the new folder.
    2. **Debug**: Add a new configuration to launch.json pointing to the new project's .elf file.