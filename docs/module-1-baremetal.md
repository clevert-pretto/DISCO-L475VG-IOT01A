# Module 01: The Silicon Handshake

### 🧠 The "Why"
In safety-critical firmware, you cannot rely on vendor-provided abstraction layers (HAL) which may contain non-deterministic code. This project establishes a 100% controlled boot sequence.

### 🧱 Architectural Pillars
1. **The Linker Script (`.ld`):** Defining the physical boundaries of Flash (`0x08000000`) and SRAM (`0x20000000`).
   

2. **The Startup Flow:**
   - Copying `.data` from Flash (LMA) to RAM (VMA).
   - Zeroing out `.bss`.
   - Initializing the Stack Pointer (MSP).

3. **Register Mapping:**
   Direct manipulation of `RCC_AHB2ENR` and `GPIOB_MODER`.