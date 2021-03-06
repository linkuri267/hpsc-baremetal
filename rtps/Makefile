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

# List all possible test and config flags here
CONFIG_FLAGS = \
	TEST_FLOAT \
	TEST_SORT \
	TEST_GTIMER \
	TEST_WDT \
	TEST_RTI_TIMER \
	TEST_RTPS_TRCH_MAILBOX \
	TEST_RT_MMU \
	TEST_RTPS_MMU \
	TEST_RTPS_DMA \
	TEST_RTPS_DMA_CB \
	TEST_SOFT_RESET \
	TEST_R52_SMP \
	CONFIG_GTIMER \
	CONFIG_SLEEP_TIMER \
	CONFIG_WDT \
	CONFIG_HPPS_RTPS_MAILBOX \

include Makefile.defconfig
include Makefile.config

# Most tests are standalone, but these are exceptions
ifeq ($(strip $(CONFIG_SLEEP_TIMER)),1)
ifneq ($(strip $(CONFIG_GTIMER)),1)
$(error CONFIG_SLEEP_TIMER requires a timer to be enabled)
endif
endif

CONFIG_ARGS = $(foreach m,$(CONFIG_FLAGS),-D$(m)=$($(m)))
CONFIG_ARGS += -DCONFIG_CONSOLE__$(CONFIG_CONSOLE)

VPATH = ..
# startup.o must be first
OBJS = \
	startup.o \
	drivers/cortex-r52.o \
	drivers/dma.o \
	drivers/gic.o \
	drivers/gtimer.o \
	drivers/mailbox.o \
	drivers/mmu.o \
	drivers/ns16550.o \
	drivers/rti-timer.o \
	drivers/wdt.o \
	lib/balloc.o \
	lib/bit.o \
	lib/command.o \
	lib/intc.o \
	lib/mailbox-link.o \
	lib/mem.o \
	lib/mutex.o \
	lib/object.o \
	lib/panic.o \
	lib/printf.o \
	lib/sleep.o \
	plat/console.o \
	main.o \
	server.o \

ifeq ($(strip $(CONFIG_WDT)),1)
OBJS += watchdog.o
endif

ifeq ($(strip $(TEST_FLOAT)),1)
OBJS += tests/float.o
endif
ifeq ($(strip $(TEST_SORT)),1)
OBJS += tests/sort.o
endif
ifeq ($(strip $(TEST_GTIMER)),1)
OBJS += tests/gtimer.o
endif
ifeq ($(strip $(TEST_WDT)),1)
OBJS += tests/wdt.o
endif
ifeq ($(strip $(TEST_RTI_TIMER)),1)
OBJS += test/test-rti-timer.o tests/rti-timer.o
endif
ifeq ($(strip $(TEST_RT_MMU)),1)
OBJS += tests/rt-mmu.o
endif
ifeq ($(strip $(TEST_RTPS_MMU)),1)
OBJS += tests/rtps-mmu.o
endif
ifeq ($(strip $(TEST_RTPS_TRCH_MAILBOX)),1)
OBJS += tests/mailbox.o
endif
ifeq ($(strip $(TEST_RTPS_DMA)),1)
OBJS += tests/dma.o
endif
ifeq ($(strip $(TEST_R52_SMP)),1)
OBJS += tests/smp.o
endif

TARGET=rtps

.DEFAULT_GOAL := $(BLDDIR)/$(TARGET).uimg

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
	$(CONFIG_ARGS) \
	-I. -Itests $(foreach d,$(SUBDIRS),-I../$(d)) \

# Enable use of TCM in assembler code with -DTCM

AOPT = --warn --fatal-warnings $(CPU_FLAGS)

# -z max-page-size: alignment of program headers in the ELF file
#	removes large gaps (0x10000) in the ELF file (see: readelf -l)
LDFLAGS = \
	$(CPU_FLAGS) \
	--entry=Start  \
	-lrdimon \
	-Wl,--gc-sections \
	-static \
	-Wl,-z,max-page-size=4 \

$(BLDDIR)/%.o : %.s | dirs
	$(CROSS_COMPILE)gcc -c $(CCOPT) -x assembler-with-cpp -o $@ $<
#	$(CROSS_COMPILE)as $(AOPT) -o $@ $^

$(BLDDIR)/%.o : %.c | dirs
	$(CROSS_COMPILE)gcc -MMD -c $(CCOPT) -o $@ $<

$(BLDDIR)/$(TARGET).dbg.elf : rtps.ld $(addprefix $(BLDDIR)/,$(OBJS))
	$(CROSS_COMPILE)gcc $(LDFLAGS) -o $@ -T $^

$(BLDDIR)/$(TARGET).elf : $(BLDDIR)/$(TARGET).dbg.elf
	$(CROSS_COMPILE)strip -x -s -R .comment -R '.ARM.*' -o $@ $<

$(BLDDIR)/$(TARGET).bin : $(BLDDIR)/$(TARGET).elf
	$(CROSS_COMPILE)objcopy -O binary $< $@

%.dis.s: %.o
	$(CROSS_COMPILE)objdump $<  -D > $@

%.gz: %
	gzip -9 -c $^ > $@

%.uimg: %.bin.gz %.elf
	mkimage -n "RTPS Bare-Metal App" -A arm -T kernel -O rtems -C gzip \
		-a $$(readelf -h $*.elf | grep -i 'Entry point' | cut -d: -f2) -d $< $@

$(addprefix $(BLDDIR)/,$(OBJS)) : Makefile Makefile.defconfig Makefile.config

# Empty config means all settings come from defconfig
Makefile.config:
	touch $@

-include $(addprefix $(BLDDIR)/,$(OBJS:.o=.d))
