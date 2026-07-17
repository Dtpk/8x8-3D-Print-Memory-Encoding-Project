# GBA Huffman Makefile (Manual Build)

TARGET      := huffman_gba
SOURCES     := main.c
ASMS        := crt0.s
OBJS        := $(ASMS:.s=.o) $(SOURCES:.c=.o)

CC          := arm-none-eabi-gcc
OBJCOPY     := arm-none-eabi-objcopy

# No -specs=gba.specs, using manual linker script
CFLAGS      := -mthumb -mthumb-interwork -O2 -Wall -fno-strict-aliasing
ASFLAGS     := -mthumb-interwork
LDFLAGS     := -mthumb -mthumb-interwork -nostartfiles -T gba.ld

all: $(TARGET).gba

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	python3 gbafix.py $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(ASFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET).gba $(TARGET).elf $(OBJS)

.PHONY: all clean
