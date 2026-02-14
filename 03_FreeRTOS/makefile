################################################################################
# Makefile for STM32L475 FreeRTOS Blinky - Full Driver Version
################################################################################

# 1. Project Name
TARGET = blinky

# 2. Directories
BUILD_DIR = build

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
-IApp/Inc \
-IDrivers/BSP/B-L475E-IOT01 \
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
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--gc-sections

# 7. Build Logic
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	@echo "Assembling $<..."
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	@echo "Linking $@..."
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(OBJCOPY) -O binary -S $< $@

$(BUILD_DIR):
	mkdir $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean