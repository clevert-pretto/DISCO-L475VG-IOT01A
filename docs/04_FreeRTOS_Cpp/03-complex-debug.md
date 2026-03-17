---
layout: default
parent : "Phase 4: The C++ Paradigm Shift (Modern C++ for Constrained Systems)"
title: "3: Debugging session: C++ FreeRTOS Port & I2C Hardware Initialization Failure"
nav_order: 3
---

# Debugging Log: C++ FreeRTOS Port & I2C Hardware Initialization Failure

## 1. The Symptoms

When porting a working STM32 FreeRTOS project from C to C++ using Dependency Injection (ISensor, Stm32Sensor), the firmware compiled successfully but failed at runtime with the following UART output:

```Plaintext
Starting Hardware Init...
I2C Bus Error: TSENSOR Init Failed
I2C Bus Error: HSENSOR Init Failed
Temperature Sensor Initialization Failed!
Hardware Initialization Failed!
```

Attempting to debug by stepping into `_tempSensor->init()` resulted in the CPU immediately crashing and jumping to the `NMI_Handler` (Non-Maskable Interrupt). The real gotcha was, when it "returns" to the next line in `appSensorRead.cpp` is actually the debugger's attempt to recover. It realizes the program counter is stuck and "fakes" a return so can keep stepping, which is why I see the `"Init Failed"` message immediately after.

## 2. Root Cause Analysis & Fixes

### Phase 1: The "Weak Linkage" Trap (C++ Name Mangling)

**The Problem:** To actually enable the I2C2 peripheral clock and configure the PB10/PB11 GPIO pins, the application must provide a "strong" override function.
While our .c files were correctly included in the CMake build, we implemented the strong `HAL_I2C_MspInit()` inside a C++ file (*main.cpp*). The C++ compiler "mangled" the function name to support function overloading .

**The Result:** The linker grabbed the empty 20-byte weak function from the HAL driver. The firmware built successfully, but the I2C hardware was never actually turned on.

**The Fix:** We consolidated the hardware overrides into `main.cpp` using an `extern "C"` block. The `extern "C"` wrapper prevents the C++ compiler from mangling the function name, allowing the linker to perfectly match it to the HAL's callback.

```C++
extern "C" {
    void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
        if(hi2c->Instance == I2C2) {
            __HAL_RCC_GPIOB_CLK_ENABLE();
            __HAL_RCC_I2C2_CLK_ENABLE();
            // ... GPIO open-drain configuration ...
        }
    }
}
```

### Phase 2: The Virtual Table & NMI Crash (Garbage Collected Constructors)

**The Problem:**
The MSP init was fixed, stepping into `_tempSensor->init()` caused same hardware fault (`NMI_Handler`).

Even after verifying that the *CMakeLists.txt* correctly passed the linker script `(target_link_options(${PROJECT_NAME}.elf PRIVATE -T${LINKER_SCRIPT}))` and that the *startup_stm32l475xx.s* assembly file correctly contained the `bl __libc_init_array` instruction to fire C++ constructors, the CPU still crashed into the `NMI_Handler` when calling `_tempSensor->init()`.


**The Result:**
Due to the earlier linkage mismatches and a stale CMake build cache, the linker's garbage collector (`-Wl,--gc-sections`) erroneously discarded the .`init_array` section.
Because the global C++ objects (`realTempSensor`, `realHumiditySensor`) were never constructed at boot, their Virtual Table Pointers (vptr) remained uninitialized at `0x00000000`.  When the FreeRTOS task attempted to call the virtual `init()` function via the interface pointer, the CPU tried to execute code at memory address `0x0` and triggered a severe hardware fault.

**The Final Fix:**
After fixing the `extern "C"` linkage, we performed a completely clean rebuild (`rm -rf build`). With the proper symbolic links restored, the linker recognized the dependencies, correctly preserved the `.init_array` section mapped by our linker script, and the assembly startup routine safely constructed the `Stm32Sensor` objects before jumping to main().

**Understanding What .init_array Actually Is**
In C++, when you declare global objects outside of `main()` (like `realTempSensor`, `realHumiditySensor`, and `realRtos`), they must be constructed before the `main()` function ever runs.

The compiler takes the memory addresses of all these constructor functions and puts them into a giant list.

This list of function pointers is stored in the memory section called `.init_array`.


# Explicitly apply the STM32 linker script
`target_link_options(${PROJECT_NAME}.elf PRIVATE -T${LINKER_SCRIPT})`
Once applied, the linker correctly preserved `.init_array`. The `startup_stm32l475xx.s` assembly file executed `bl __libc_init_array` before `main()`, which safely constructed the `Stm32Sensor` objects, populated their virtual tables, and allowed the I2C sequence to complete successfully.

## Final Verdict
By tying these pieces together — the discarded map section, the virtual function crash, and the missing C++ constructor boot sequence — it became clear that the linker script had to explicitly protect and define the `.init_array` memory block to bring your C++ objects to life.

## Key Takeaways for C++ on Bare-Metal ARM
**1. extern "C" is mandatory for callbacks:** Any function that an RTOS or C-based HAL needs to call (like interrupts, MSP inits, or FreeRTOS hooks) must be protected from C++ name mangling.

**2. Always check your .map file:** If hardware isn't turning on or breakpoints aren't hitting, search the *.map* file for "*Discarded input sections*" to see if the linker's garbage collector aggressively stripped critical hardware configurations or constructor arrays.

**3. Clean Builds Matter:** When altering linker configurations, linker scripts, or fixing deep symbolic linkage issues in CMake,* always delete the build directory* to ensure cached artifacts don't mask the solution.