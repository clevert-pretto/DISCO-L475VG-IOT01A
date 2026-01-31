# shared/base.mk

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

# Flags: Added -I. and -I../shared to find headers in both places
CFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -g -Wall -I. -I../shared
LDFLAGS = -nostdlib -T ../shared/linker.ld

# 1. Define Common Sources
COMMON_SRCS = ../shared/startup.c

# 2. Combine with Project Sources (passed from Project Makefile)
ALL_SRCS = $(SRCS) $(COMMON_SRCS)

# 3. Use 'notdir' to turn "src/main.c" into "main.o" 
# and "../shared/startup.c" into "startup.o"
OBJS = $(notdir $(ALL_SRCS:.c=.o))

# 4. Tell Make where to look for .c files (The VPATH)
# This is the "magic" that lets Make find src/main.c when we ask for main.o
VPATH = $(dir $(SRCS)) ../shared

all: main.bin

main.elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

# Pattern rule: How to turn any .c into a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

main.bin: main.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o *.elf *.bin