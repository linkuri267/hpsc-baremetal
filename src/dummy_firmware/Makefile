
ARMGNU = arm-none-eabi

AOPS = --warn --fatal-warnings -mcpu=cortex-m4
COPS = -Wall -Werror -O2 -nostdlib -nostartfiles -ffreestanding  -mcpu=cortex-m4 -mthumb

IRQS = 161,162

all : m4uart.bin m4fsb.bin m4resetA53.bin

clean:
	rm -f *.bin
	rm -f *.o
	rm -f *.elf
	rm -f *.list

vectors.s isr.c: genvec.py
	./genisr.py --handlers $(IRQS) vectors.s isr.c

vectors.o : vectors.s
	$(ARMGNU)-as $(AOPS) vectors.s -o vectors.o

isr.o : isr.c
	$(ARMGNU)-gcc $(COPS) -c isr.c -o isr.o

m4uart.o : m4uart.c
	$(ARMGNU)-gcc $(COPS) -c m4uart.c -o m4uart.o

uart.o : uart.c
	$(ARMGNU)-gcc $(COPS) -c uart.c -o uart.o

m4uart.bin : uart-memmap vectors.o m4uart.o
	$(ARMGNU)-ld -o m4uart.elf -T uart-memmap vectors.o m4uart.o
	$(ARMGNU)-objdump -D m4uart.elf > m4uart.list
	$(ARMGNU)-objcopy m4uart.elf m4uart.bin -O binary

m4fsb.o : m4fsb.S
	$(ARMGNU)-as $(AOPS) m4fsb.S -o m4fsb.o

m4fsb.bin : uart-memmap m4fsb.o 
	$(ARMGNU)-ld -o m4fsb.elf -T uart-memmap m4fsb.o 
	$(ARMGNU)-objdump -D m4fsb.elf > m4fsb.list
	$(ARMGNU)-objcopy m4fsb.elf m4fsb.bin -O binary

m4resetA53.o : m4resetA53.c
	$(ARMGNU)-gcc $(COPS) -c m4resetA53.c -o m4resetA53.o

m4resetA53.elf: resetA53-memmap vectors.o m4resetA53.o uart.o isr.o
	$(ARMGNU)-ld -o $@ -T $<

m4resetA53.list: m4resetA53.elf
	$(ARMGNU)-objdump -D $< > $@

m4resetA53.bin: m4resetA53.elf
	$(ARMGNU)-objcopy $< $@ -O binary
