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

# List all possible test flags here
TEST_FLAGS = \
	TEST_FLOAT \
	TEST_SORT \
	TEST_GTIMER \
	TEST_RTPS_TRCH_MAILBOX \
	TEST_HPPS_RTPS_MAILBOX \
	TEST_SOFT_RESET \
	TEST_RT_MMU \
	TEST_RTPS_MMU \
	TEST_RTPS_DMA \
	TEST_RTPS_DMA_CB \

# Enable disable tests here:
TEST_FLOAT = 1
TEST_SORT = 1
TEST_GTIMER = 1
TEST_RTPS_TRCH_MAILBOX = 1
TEST_HPPS_RTPS_MAILBOX = 1
TEST_SOFT_RESET = 0
TEST_RT_MMU = 1
TEST_RTPS_MMU = 1
TEST_RTPS_DMA = 1
TEST_RTPS_DMA_CB = 1

TEST_ARGS = $(foreach m,$(TEST_FLAGS),-D$(m)=$($(m)))

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
	drivers/dma.o \
	drivers/mmu.o \
	drivers/gtimer.o \
	lib/printf.o \
	lib/panic.o \
	lib/mem.o \
	lib/object.o \
	lib/intc.o \
	lib/mailbox-link.o \
	lib/balloc.o \
	lib/command.o \
	server.o \
	main.o \

ifeq ($(strip $(TEST_FLOAT)),1)
OBJS += test-float.o
endif
ifeq ($(strip $(TEST_SORT)),1)
OBJS += test-sort.o
endif
ifeq ($(strip $(TEST_GTIMER)),1)
OBJS += test-gtimer.o
endif
ifeq ($(strip $(TEST_RT_MMU)),1)
OBJS += test-rt-mmu.o
endif
ifeq ($(strip $(TEST_RTPS_MMU)),1)
OBJS += test-rtps-mmu.o
endif
ifeq ($(strip $(TEST_RTPS_TRCH_MAILBOX)),1)
OBJS += test-mailbox.o
endif
ifeq ($(strip $(TEST_RTPS_DMA)),1)
OBJS += test-dma.o
endif

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
	$(TEST_ARGS) \
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

$(BLDDIR)/$(TARGET) : rtps.ld $(addprefix $(BLDDIR)/,$(OBJS))
	$(ARM_NONE_EABI)-gcc -T $^ -o $@ $(LDFLAGS)

%.dis.s: %.o
	$(ARM_NONE_EABI)-objdump $<  -D > $@

-include $(addprefix $(BLDDIR)/,$(OBJS:.o=.d))
