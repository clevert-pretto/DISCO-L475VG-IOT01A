# shared/base.mk
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -g -Wall
LDFLAGS = -nostdlib -T ../shared/linker.ld

# Common source files
COMMON_SRCS = ../shared/startup.c

# Combine project sources with common sources
ALL_SRCS = $(SRCS) $(COMMON_SRCS)
OBJS = $(ALL_SRCS:.c=.o)

all: main.bin

main.elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o *.elf *.bin ../shared/*.o