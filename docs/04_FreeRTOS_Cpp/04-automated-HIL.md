---
layout: default
parent : "Phase 4: The C++ Paradigm Shift (Modern C++ for Constrained Systems)"
title: "4: Automated HIL Integration"
nav_order: 4
---

# 🚀 Project Milestone: Automated HIL Integration
Frameworks: C++17, FreeRTOS, Pytest, GitHub Actions, Pre-commit

**Objective:** Establish a robust automated testing pipeline that verifies firmware logic on physical hardware before every commit and push.

# 🏗️ 1. Firmware Architecture Refactoring

To support automation, we moved away from hardcoded logic to a modular, command-driven architecture.

## Table-Driven Command Dispatcher
We replaced a high-complexity if-else chain in `appLogger::handleCommand` with a Dispatch Table using modern C++ standards.

**Pattern:** Uses a `CommandEntry` struct to map string commands (e.g., `dump_logs`) to private member functions.

**Logic:** Utilizes `std::find_if` to search the table, reducing Cyclomatic Complexity from 13 down to 2.

**Stack Safety:** Implemented Segmented Printing in handlers (e.g., `cmdGetTemp`) to minimize stack usage by sending labels and values separately.

**Task Registry (IRTOS Integration)**
Eliminated global task handles by implementing a Task Registry in the *IRTOS* interface.

**Impact:** appLogger now queries the RTOS for a list of registered tasks to report stack high-water marks without needing extern variables from *main.cpp*.

# 🐍 2. HIL Testing Framework (Python)
We built a pytest-based suite that communicates with the STM32 via UART to verify system state.

## Hardware Discovery Logic (conftest.py)
To prevent the framework from crashing when the device is unplugged, we added *"Hardware Discovery"*.

**Functionality:** Checks for */dev/ttyACM0*. If missing, it uses *pytest.skip* instead of failing, allowing CI/CD and pre-commit to proceed for non-hardware changes.

## Test Scenarios (test_system.py)**
**Stack Health:** Verifies that all RTOS tasks (*Heartbeat*, *SensorRead*, etc.) are reporting valid stack margins.

**Storage Integrity:** Automates dangerous operations like *bulk_erase* and *event_sector_erase*, verifying erase counts and data persistence.

**Sensor Accuracy:** Uses *regex* to validate that temperature and humidity readings follow the correct numeric format.

# 🛠️ 3. Local Automation & VS Code Integration
Integrated the HIL suite into daily coding workflow for "one-click" verification.

## VS Code Tasks (tasks.json)
Added a dedicated task to run HIL tests and generate a self-contained HTML report.

**Output:** Generates *hil_report.html* for instant visual feedback on hardware status.

**Local Guardrails (.pre-commit-config.yaml)**
Implemented a pre-commit hook that triggers HIL tests before every git commit.

**Optimization:** Used file filters (`files: ^04_FreeRTOS_Cpp/(App/|HIL_test/)`) to ensure HIL tests **only** run when firmware or test scripts are modified.

**Environment:** Configured the hook to use `.venv/bin/pytest` to ensure library consistency.

# ☁️ 4. Remote DevOps (GitHub Actions)

## The Pipeline (`ci_quality.yml`)
**Job 1:** Quality Gate (Cloud): Runs cppcheck, lizard, and unit tests on GitHub's servers.

**Job 2:** HIL Testing (Local PC):
- *Build:* Cross-compiles the C++ source into a `.bin` file.
- *Flash:* Physically writes the new firmware to the DISCO board via st-flash.
- *Verify:* Executes the *pytest* HIL suite on the fresh firmware.
- *Upload:* Sends the *hil_report.html* back to GitHub as a build artifact.

## Isolation Fix: 
Solved the `externally-managed-environment error (PEP 668)` by forcing the runner to use the local `.venv` for all pip operations.

## Permissioning: 
Ensured the runner user has dialout group access to interact with the USB serial port.

# 🪵 5. The "Hustle" Log (Key Bug Fixes)

During this development phase, we identified and resolved several critical "gotchas":

|Issue| Root cause | Resolution|
|-----|-------|--------|
|Silent `xtoa` Failure|Buffer size passed as 6.|Used `sizeof(buf)` to ensure `app_ftoa` safety checks pass.|
|Race Condition|Prompt `>` sent before Logger Task finished.|Switched operation completion messages to synchronous `sendCommandResponse`.|
|HIL False Positive|Search string `TS:` mismatch.|Synchronized Python `assert` strings with actual C++ output labels.|
|Bitwise Storage|Float logging confusion.|Clarified `memcpy` bit-preservation for storing `floats` in `uint32_t` arrays.|
|CI Failures|Missing symlinks for `.venv/bin/pytest`.|Added symlink "magic tricks" in the GitHub Action `quality-gate` job.|


## Useful Commands for Maintenance
**Restart Runner:** `cd ~/actions-runner && ./run.sh`

**Manual HIL Run:** `pytest 04_FreeRTOS_Cpp/HIL_test/test_system.py --html=report.html`

**Check USB Permissions:** `sudo usermod -a -G dialout $USER`