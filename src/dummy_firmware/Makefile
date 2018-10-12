
ARMGNU = arm-none-eabi

FLOAT_FLAG = -mfloat-abi=hard 
AOPS = --warn --fatal-warnings -mcpu=cortex-m4 -mthumb $(FLOAT_FLAG)
COPS = -Wall -Werror -O0 -g -nostdlib -nostartfiles -ffreestanding  -mcpu=cortex-m4 -mthumb $(FLOAT_FLAG)

IRQS = 161:mbox_rtps_rcv_isr,162:mbox_rtps_ack_isr,163:mbox_hpps_isr,164

OBJS = \
       vectors.o \
       trch.o \
       isr.o \
       float.o \
       command.o \
       mailbox.o \
       nvic.o \
       mmu.o \
       reset.o \
       printf.o \
       uart.o \

all : trch.elf

clean:
	rm -f *.o *.d *.elf *.list vectors.s

vectors.s isr.c: genisr.py
	./genisr.py --handlers $(IRQS) vectors.s isr.c

%.o : %.s
	$(ARMGNU)-as $(AOPS) $< -o $@

%.o : %.c
	$(ARMGNU)-gcc -MMD $(COPS) -c $< -o $@

trch.elf: trch.ld $(OBJS)
	$(ARMGNU)-ld -o $@ -T $^

-include $(OBJS:.o=.d)
