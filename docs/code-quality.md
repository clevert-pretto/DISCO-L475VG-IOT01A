---
layout: default
title: Code Quality Metrics
nav_order: 93
---

# Code Quality & Automation 🛡️

## Current Build Status
![Quality Gate](https://github.com/clevert-pretto/DISCO-L475VG-IOT01A/actions/workflows/ci_quality.yml/badge.svg)

---

## Static Analysis (Linting)
We use **Cppcheck** to catch semantic bugs that a standard compiler might miss.
* **Standards:** C11
* **Checks:** Variable tracking, pointer dereferencing, and buffer overflows.
* **Target:** All files in the `/App` directory.

## Source Monitoring (Metrics)
We use **Lizard** to monitor code maintainability. This ensures our RTOS tasks remain deterministic and easy to debug.

| Metric | Limit | Purpose |
| :--- | :--- | :--- |
| **CCN** | < 10 | Limits "Cyclomatic Complexity" (logic paths) |
| **Line Count** | < 15 | Keeps functions modular and stack-friendly |



---

## Automation Workflow
The pipeline follows a **Test-then-Deploy** model:
1. **Push:** Code is pushed to GitHub.
2. **Audit:** GitHub Actions runs Cppcheck and Lizard.
3. **Deploy:** If (and only if) the quality gate passes, this documentation site is updated.