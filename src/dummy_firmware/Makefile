
ARMGNU = arm-none-eabi

FLOAT_FLAG = -mfloat-abi=hard 
AOPS = --warn --fatal-warnings -mcpu=cortex-m4 -mthumb $(FLOAT_FLAG)
COPS = -Wall -Werror -O0 -g -nostdlib -nostartfiles -ffreestanding  -mcpu=cortex-m4 -mthumb $(FLOAT_FLAG)

IRQS = 72:mbox_rtps_rcv_isr,75:mbox_rtps_ack_isr,136:mbox_hpps_rcv_isr,139:mbox_hpps_ack_isr

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
       test-mailbox.o \

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
