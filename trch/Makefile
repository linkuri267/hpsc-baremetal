include ../Makefile.common

CPU_FLAGS = -mcpu=cortex-m4 -mthumb 
FLOAT_FLAG = -mfloat-abi=hard 
AOPS = --warn --fatal-warnings $(CPU_FLAGS) $(FLOAT_FLAG)
INC_FLAGS = $(foreach d,$(SUBDIRS),-I../$(d)) \

COPS = \
	-Wall -Werror \
	-O0 -g \
	$(CPU_FLAGS) $(FLOAT_FLAG) \
	-nostdlib -nostartfiles -ffreestanding \
	-DUART_BASE=LSIO_UART0_BASE \
	-DPRINTF_SUPPORT_FLOAT \
	-DPRINTF_NO_DOUBLE_WORKAROUND \
	$(INC_FLAGS) \

VPATH = ..
OBJS = \
       vectors.o \
       isr.o \
       lib/mem.o \
       lib/object.o \
       lib/balloc.o \
       lib/printf.o \
       lib/panic.o \
       lib/mailbox-link.o \
       lib/command.o \
       drivers/mailbox.o \
       drivers/nvic.o \
       drivers/uart.o \
       drivers/mmu.o \
       reset.o \
       mailbox-isr.o \
       float.o \
       server.o \
       main.o \

TARGET=trch.elf

.DEFAULT_GOAL := $(BLDDIR)/$(TARGET)

clean:
	rm -rf $(BLDDIR) vectors.s isr.c

isrs: irqmap genisr.py
	./genisr.py -v $(INC_FLAGS) --irqmap $< vectors.s isr.c
.INTERMEDIATE: isrs
vectors.s: isrs
isr.c: isrs

$(BLDDIR)/%.o : %.s | dirs
	$(ARM_NONE_EABI)-as $(AOPS) $< -o $@

$(BLDDIR)/%.o : %.c | dirs
	$(ARM_NONE_EABI)-gcc -MMD $(COPS) -c $< -o $@

$(BLDDIR)/$(TARGET): trch.ld $(addprefix $(BLDDIR)/,$(OBJS))
	$(ARM_NONE_EABI)-ld -o $@ -T $^

-include $(addprefix $(BLDDIR)/,$(OBJS:.o=.d))
