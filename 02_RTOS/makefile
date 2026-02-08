# shared/base.mk

CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

# Flags: Added -I. and -I../shared to find headers in both places 
# To enable FPU set =mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -O0 -g3 -Wall -Iinc -Icmsis/inc
LDFLAGS = -nostdlib -T linker.ld -nostartfiles -lm -lgcc

# To Remove -nostdlib if you want the compiler to find __libc_init_array automatically
# LDFLAGS = -T linker.ld -specs=nano.specs -specs=nosys.specs
# nano.specs: Links a smaller, optimized version of the standard library for embedded systems.
# nosys.specs: Provides "empty" system calls (like _exit or _sbrk) so the linker doesn't complain about missing OS-level hooks.


# 1. Sources
SRCS = src/main.c src/scheduler.c
AS_SRCS = src/context_switch.s cmsis/src/startup_stm32l475xx.s

# 2. Use 'notdir' to turn "src/main.c" into "main.o" 
# and "../shared/startup.c" into "startup.o"
OBJS = $(SRCS:.c=.o) $(AS_SRCS:.s=.o)

all: final.bin

final.elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o final.elf

final.bin: final.elf
	$(OBJCOPY) -O binary final.elf final.bin
clean:
	rm -f src/*.o *.o *.elf *.bin