
ARMGNU = arm-none-eabi

FLOAT_FLAG = -mfloat-abi=hard 
AOPS = --warn --fatal-warnings -mcpu=cortex-m4 -mthumb $(FLOAT_FLAG)
COPS = -Wall -Werror -O0 -g -nostdlib -nostartfiles -ffreestanding  -mcpu=cortex-m4 -mthumb $(FLOAT_FLAG)

OBJS = \
       vectors.o \
       trch.o \
       isr.o \
       float.o \
       command.o \
       server.o \
       mailbox.o \
       nvic.o \
       mmu.o \
       reset.o \
       printf.o \
       uart.o \
       panic.o \
       mailbox-link.o \
       mailbox-isr.o \

all : trch.elf

clean:
	rm -f *.o *.d *.elf *.list vectors.s

vectors.s isr.c: genisr.py irqmap
	./genisr.py --irqmap irqmap vectors.s isr.c

%.o : %.s
	$(ARMGNU)-as $(AOPS) $< -o $@

%.o : %.c
	$(ARMGNU)-gcc -MMD $(COPS) -c $< -o $@

trch.elf: trch.ld $(OBJS)
	$(ARMGNU)-ld -o $@ -T $^

-include $(OBJS:.o=.d)
