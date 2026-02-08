---
layout: default
title: "STM32 Development & Debugging on WSL"
nav_order: 5
has_children: false
---

# 🛠️ STM32 Development & Debugging on WSL

This guide covers the end-to-end process of connecting an ST-LINK debugger to WSL, configuring VS Code, and setting up the Cortex-Debug environment for "No-HAL" development.

## 1. Share ST-LINK from Windows to WSL
WSL does not natively see USB devices. We use `usbipd-win` to bridge the connection.

1.  **On Windows Host:** Download and install [usbipd-win](https://github.com/dorssel/usbipd-win/releases).
2.  **In Windows PowerShell (Admin):**
    * List devices: `usbipd list`
    * Find the **BUSID** for "ST-Link Debug" (usually `0483:374b`).
    ``` 
        PS C:\Users\mypc> usbipd list
        Connected:
        BUSID  VID:PID    DEVICE                     
        2-4    0483:374b  ST-Link Debug, USB Mass Storage Device, STMicroelectronic...  Shared

        Persisted:
        GUID                                  DEVICE
    ```

    * Bind the device (first time only): `usbipd bind --busid 2-4` Replace 2-4 with your's.
    * Attach to WSL: `usbipd attach --wsl --busid 2-4`
3.  **In WSL Terminal:** Run `lsusb` to verify. You should see `STMicroelectronics ST-LINK/V2.1`.
    ```
        username@laptop:~$ lsusb
        Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
        Bus 001 Device 002: ID 0483:374b STMicroelectronics ST-LINK/V2.1
        Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
    ```

---

## 2. Install Linux Toolchain & Debugger
Modern Ubuntu/Debian versions use `gdb-multiarch` to support ARM processors.

```bash
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi openocd gdb-multiarch
```

### 🔓 Fix USB Permissions
To allow OpenOCD to access the ST-LINK without sudo:

**1. Create rule:** sudo nano /etc/udev/rules.d/99-stlink.rules

**2. Add:** SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374b", MODE="666"

**3. Restart WSL:** Run wsl --shutdown in Windows PowerShell, then reopen your WSL terminal.

## 3. VS Code Configuration
Install the [`Cortex-Debug extension (by marus25)`](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug).

### Folder Structure
Ensure your .vscode folder is at the root of the workspace you have opened in VS Code.

### launch.json Setup
Create .vscode/launch.json with the following:

```
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug RTOS (WSL)",
            "cwd": "${workspaceFolder}/<Folder>/<sub-foler>",
            "executable": "${workspaceFolder}//<Folder>/<sub-foler>/<project>.elf",
            "request": "launch",
            "type": "cortex-debug",
            "servertype": "openocd",
            "device": "STM32L475",
            "interface": "swd",
            "runToEntryPoint": "main",
            "configFiles": [
                "interface/stlink.cfg",
                "target/stm32l4x.cfg"
            ],
            /* Important for WSL: points to the gdb you installed */
            "gdbPath": "/usr/bin/gdb-multiarch",
            "toolchainPrefix": "arm-none-eabi"
        }
    ]
}
```

*Replace `<Folder>`, `<sub-foler>`, and `<project>.elf` with your dedicated names, or simply path.*
*Note that `workspaceFolder` is root and your `.vscode/launch.json` located there.*

## 4. Debugging Workflow
**Build:** Run make to generate .elf.

**Attach:** Ensure usbipd is attached in PowerShell.

**Launch:** Go to the Run and Debug tab (Ctrl+Shift+D).

**Select:** Choose "Debug RTOS" and press F5.

