#m Copyright (c) 2016 Arm Limited (or its affiliates). All rights reserved.
# Use, modification and redistribution of this file is subject to your possession of a
# valid End User License Agreement for the Arm Product of which these examples are part of 
# and your compliance with all applicable terms and conditions of such licence agreement.
#
# This makefile is intended to allow command-line users to build this project outside of Eclipse
# This makefile is NOT used by the Eclipse managed builder project (which creates its own makefiles)
#
# This makefile is intended for use with GNU make
# This example is intended to be built with Arm Compiler 6

include ../Makefile.common

VPATH = ..

BLDDIR = bld
SUBDIRS = \
	lib \
	drivers \
	plat \

OBJS = \
	startup.o \
	drivers/uart.o \
	drivers/gic.o \
	drivers/mailbox.o \
	lib/printf.o \
	lib/panic.o \
	lib/mem.o \
	lib/object.o \
	lib/mailbox-link.o \
	lib/command.o \
	server.o \
	float.o \
	sorts.o \
	main.o \

TARGET=rtps.elf

.DEFAULT_GOAL := $(BLDDIR)/$(TARGET)

clean:
	rm -rf $(BLDDIR)

# If hardware floating point is either not present or not required, add -mfpu=none to the compile step
CPU_FLAGS = -mcpu=cortex-r52 -mthumb -mfloat-abi=hard -mfpu=vfpv3
CCOPT = \
	-g -O0 $(CPU_FLAGS) \
	-nostdlib -nostartfiles -ffreestanding \
	-DUART_BASE=LSIO_UART1_BASE \
	-DPRINTF_SUPPORT_LONG_LONG \
	-DPRINTF_SUPPORT_FLOAT \
	-DPRINTF_NO_DOUBLE_WORKAROUND \
	$(foreach d,$(SUBDIRS),-I../$(d)) \

# Enable use of TCM in assembler code with -DTCM

AOPT = --warn --fatal-warnings $(CPU_FLAGS)
LDFLAGS = \
	$(CPU_FLAGS) \
	--entry=Start  \
 	-lgcc -lc -lrdimon \
	-Wl,--gc-sections \
	-static \

$(BLDDIR)/%.o : %.s | dirs
	$(ARM_NONE_EABI)-gcc -c $(CCOPT) -x assembler-with-cpp -o $@ $<
#	$(ARM_NONE_EABI)-as $(AOPT) -o $@ $^

$(BLDDIR)/%.o : %.c | dirs
	$(ARM_NONE_EABI)-gcc -MMD -c $(CCOPT) -o $@ $<

$(BLDDIR)/$(TARGET) : startup.ld $(addprefix $(BLDDIR)/,$(OBJS))
	$(ARM_NONE_EABI)-gcc -T $^ -o $@ $(LDFLAGS)

%.dis.s: %.o
	$(ARM_NONE_EABI)-objdump $<  -D > $@

-include $(addprefix $(BLDDIR)/,$(OBJS:.o=.d))
