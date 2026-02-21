################################################################################
# Makefile for STM32L475 FreeRTOS Blinky - Full Driver Version
################################################################################
# Define the tool path
LIZARD = /home/ravichilwant/.local/bin/lizard

# 1. Project Name
TARGET = freeRTOSApp

# 2. Directories
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
LST_DIR = $(BUILD_DIR)/lst

# 3. Toolchain
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SZ = arm-none-eabi-size

# 4. Sources
# automatically find all .c files in these directories
C_SOURCES = $(sort \
	$(wildcard App/*.c) \
	$(wildcard App/Src/*.c) \
	$(wildcard Drivers/BSP/B-L475E-IOT01/*.c) \
	$(wildcard Drivers/BSP/Components/hts221/*.c) \
	$(wildcard Drivers/CMSIS/Device/ST/STM32L4xx/Source/*.c) \
	$(wildcard Drivers/STM32L4xx_HAL_Driver/Src/*.c) \
	$(wildcard Middlewares/FreeRTOS/*.c) \
	$(wildcard Middlewares/FreeRTOS/portable/*.c) \
)

# Manually add startup file
ASM_SOURCES = \
Drivers/CMSIS/Device/ST/STM32L4xx/Source/startup_stm32l475xx.s

# 5. Compiler Flags
MCU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
C_DEFS = -DSTM32L475xx -DUSE_HAL_DRIVER

C_INCLUDES =  \
-IApp \
-IApp/Inc \
-IDrivers/BSP/B-L475E-IOT01 \
-IDrivers/BSP/Components/hts221 \
-IDrivers/BSP/Components/mx25r6435f \
-IDrivers/CMSIS/Core/Include \
-IDrivers/CMSIS/Device/ST/STM32L4xx/Include \
-IDrivers/STM32L4xx_HAL_Driver \
-IDrivers/STM32L4xx_HAL_Driver/Inc \
-IMiddlewares/FreeRTOS \
-IMiddlewares/FreeRTOS/include \
-IMiddlewares/FreeRTOS/portable

OPT = -O0 -g3
# Added -ffunction-sections -fdata-sections to allow linker to remove unused code
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

# 6. Linker Flags
LDSCRIPT = STM32L475VGTx_FLASH.ld
LIBS = -lc -lm -lnosys
# --gc-sections is CRITICAL here. It removes the massive amount of unused HAL code we just copied.
LDFLAGS = $(MCU) -specs=nosys.specs -T$(LDSCRIPT) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--gc-sections

# 7. Build Logic
C_OBJS = $(addprefix $(OBJ_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
ASM_OBJS = $(addprefix $(OBJ_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
OBJECTS = $(C_OBJS) $(ASM_OBJS)

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# 8. Hardware Specifications for STM32L475VGT6
FLASH_SIZE = 1048576 # 0x100000 
RAM_SIZE = 98304	 # 0x18000

# --- Artifacts to Purge ---
LINTER_ARTIFACTS = .cppcheck-addon-ctu-file ctu-info .cppcheck_cache

# Build Rules
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin print_usage

$(OBJ_DIR)/%.o: %.c | $(BUILD_DIR) $(OBJ_DIR) $(LST_DIR)
	@echo "Compiling $<..."
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(LST_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(OBJ_DIR)/%.o: %.s | $(OBJ_DIR)
	@echo "Assembling $<..."
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	@echo "Linking $@..."
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -O binary -S $< $@

$(BUILD_DIR) $(OBJ_DIR) $(LST_DIR)::
	mkdir -p $@

print_usage: $(BUILD_DIR)/$(TARGET).elf
	@echo ""
	@echo "--------------------- MEMORY USAGE REPORT ---------------------"
	@$(SZ) $<
	@echo ""
	@$(SZ) $< | python3 -c "import sys; \
		text, data, bss = map(int, sys.stdin.read().split()[7:10]); \
		flash_used = text + data; \
		ram_used = data + bss; \
		print(f'Flash: {flash_used} / $(FLASH_SIZE) bytes ({flash_used/$(FLASH_SIZE)*100:.2f}%)'); \
		print(f'RAM:   {ram_used} / $(RAM_SIZE) bytes ({ram_used/$(RAM_SIZE)*100:.2f}%)');" \
	@echo "---------------------------------------------------------------"

clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)
	rm -rf .cppcheck-addon-ctu-file/
	rm -f .ctu-info
	rm -f .dump

# --- Static Analysis ---
lint:
	@echo "--- Running Lint analysis ---"
	cppcheck --enable=all --inline-suppr --std=c11 \
	-I App/Inc -I Drivers/CMSIS/Core/Include \
	--suppress=*:Drivers/* --suppress=*:Middlewares/* \
	App/
	@echo "--- Lint Analysis Finished ---"

# --- Complexity Metrics ---
metrics:
	@echo "Running Complexity Analysis on App folder..."
	lizard -w -l 15 -C 10 App/
	@echo "--- Code complexity Analysis Finished ---"

# Deep clean: Use this before a final commit or if linter acts weird
linter-clean:
	rm -rf $(LINTER_ARTIFACTS)
	@echo "Linter artifacts purged."

.PHONY: all clean lint metrics linter-clean