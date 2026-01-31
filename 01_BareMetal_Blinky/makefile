# Toolchain definition
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

# CPU Specific Flags (Cortex-M4 for STM32L475)
CFLAGS = -mcpu=cortex-m4 -mthumb -Wall -O0 -g
LDFLAGS = -nostdlib -T linker.ld

# Targets
all: main.bin

main.elf: main.o startup.o
	$(CC) $(CFLAGS) $(LDFLAGS) main.o startup.o -o main.elf

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

startup.o: startup.c
	$(CC) $(CFLAGS) -c startup.c -o startup.o

main.bin: main.elf
	$(OBJCOPY) -O binary main.elf main.bin

clean:
	rm -f *.o *.elf *.bin