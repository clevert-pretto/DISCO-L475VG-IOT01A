# --- Module Selection ---
# Default module if none is provided
MODULE ?= 02-pre-priority-sch
MODULE_PATH := 0-Modules/$(MODULE)

# --- Project Configuration ---
TARGET      := final
BUILD_DIR   := obj
RELEASE_DIR := release

# --- Toolchain Selection ---
CC 		:= arm-none-eabi-gcc
AS 		:= arm-none-eabi-as
LD 		:= arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

# --- CPU & Compilation Flags ---
# To enable FPU set =mfloat-abi=hard -mfpu=fpv4-sp-d16
CPU		:=  -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
CFLAGS 	:=  $(CPU) -O0 -g3 -Wall
ASFLAGS	:=  $(CPU)
LDFLAGS :=  $(CPU) -nostdlib -T linker.ld -nostartfiles -lm -lgcc

# To Remove -nostdlib if you want the compiler to find __libc_init_array automatically
# LDFLAGS = -T linker.ld -specs=nano.specs -specs=nosys.specs
# nano.specs: Links a smaller, optimized version of the standard library for embedded systems.
# nosys.specs: Provides "empty" system calls (like _exit or _sbrk) so the linker doesn't complain about missing OS-level hooks.


# --- File Discovery ---
# 1. Find all .c and .s files in the SELECTED module, plus cmsis and src
C_SRCS  := $(shell find $(MODULE_PATH) cmsis -name "*.c" 2>/dev/null)
AS_SRCS := $(shell find $(MODULE_PATH) cmsis src -name "*.s" 2>/dev/null)

# 2. Add the specific module path to include flags
INC_DIRS := $(shell find inc cmsis $(MODULE_PATH) -type d 2>/dev/null)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# 3. Object file mapping (src/main.c -> obj/src/main.o)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))
OBJS += $(patsubst %.s,$(BUILD_DIR)/%.o,$(AS_SRCS))

# --- Build Rules ---

all: $(RELEASE_DIR)/$(TARGET).elf $(RELEASE_DIR)/$(TARGET).bin

# Rule for C files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INC_FLAGS) -c $< -o $@

# Rule for Assembly files
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(@D)  # Creates the build subdirectory if it doesn't exist
	$(CC) -c $< -o $@

# Rule for .bin file
$(RELEASE_DIR)/$(TARGET).bin: $(RELEASE_DIR)/$(TARGET).elf
	@mkdir -p $(@D)
	$(OBJCOPY) -O binary $< $@
	@echo "------------------------------------------------"
	@echo "Build Success for Module: $(MODULE)"
	$(SIZE) $<
	@echo "------------------------------------------------"

# Rule for .elf file
$(RELEASE_DIR)/$(TARGET).elf: $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

verify:
	@echo "Linked Files for $(TARGET):"
	@echo "Core: $(CORE_C_SRCS)"
	@echo "Module: $(MOD_C_SRCS)"

clean:
	rm -rf $(BUILD_DIR) $(RELEASE_DIR)
	@echo "Cleaned."

.PHONY: all clean