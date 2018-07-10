
ARMGNU = arm-none-eabi

AOPS = --warn --fatal-warnings -mcpu=cortex-m4
COPS = -Wall -Werror -O2 -nostdlib -nostartfiles -ffreestanding  -mcpu=cortex-m4 -mthumb \
       -I$(PWD)/libmspprintf/src/include

IRQS = 161,162

all : m4uart.bin m4fsb.bin m4resetA53.bin

clean:
	rm -f *.bin
	rm -f *.o
	rm -f *.elf
	rm -f *.list

vectors.s isr.c: genisr.py
	./genisr.py --handlers $(IRQS) vectors.s isr.c

%.o : %.s
	$(ARMGNU)-as $(AOPS) $< -o $@

%.o : %.c
	$(ARMGNU)-gcc $(COPS) -c $< -o $@

m4uart.bin : uart-memmap vectors-orig.o m4uart.o
	$(ARMGNU)-ld -o m4uart.elf -T uart-memmap vectors-orig.o m4uart.o
	$(ARMGNU)-objdump -D m4uart.elf > m4uart.list
	$(ARMGNU)-objcopy m4uart.elf m4uart.bin -O binary


m4fsb.bin : uart-memmap m4fsb.o 
	$(ARMGNU)-ld -o m4fsb.elf -T uart-memmap m4fsb.o 
	$(ARMGNU)-objdump -D m4fsb.elf > m4fsb.list
	$(ARMGNU)-objcopy m4fsb.elf m4fsb.bin -O binary

m4resetA53.elf: resetA53-memmap vectors.o m4resetA53.o isr.o libmspprintf/src/printf.o uart.o
	$(ARMGNU)-ld -o $@ -T $^

m4resetA53.list: m4resetA53.elf
	$(ARMGNU)-objdump -D $< > $@

m4resetA53.bin: m4resetA53.elf
	$(ARMGNU)-objcopy $< $@ -O binary
