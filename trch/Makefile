include ../Makefile.common

# List all possible test flags here
CONFIG_FLAGS = \
	TEST_IPI \
	TEST_FLOAT \
	TEST_SYSTICK \
	TEST_TRCH_WDT \
	TEST_RTPS_WDT \
	TEST_HPPS_WDT \
	TEST_TRCH_DMA \
	TEST_TRCH_DMA_CB \
	TEST_RT_MMU \
	CONFIG_SYSTICK \
	CONFIG_SLEEP_TIMER \
	CONFIG_HPPS_TRCH_MAILBOX \
	CONFIG_HPPS_TRCH_MAILBOX_SSW \
	CONFIG_RTPS_TRCH_MAILBOX \
	CONFIG_TRCH_WDT \
	CONFIG_RTPS_WDT \
	CONFIG_HPPS_WDT \
	CONFIG_TRCH_DMA \
	CONFIG_RT_MMU \
	CONFIG_BOOT_RTPS \
	CONFIG_BOOT_HPPS \

# Enable/disable standalone tests here:
TEST_IPI 			= 0
TEST_FLOAT 			= 1
TEST_SYSTICK			= 1
TEST_TRCH_WDT 			= 1
TEST_RTPS_WDT 			= 1
TEST_HPPS_WDT 			= 1
TEST_TRCH_DMA			= 1
TEST_TRCH_DMA_CB 		= 1 # if set, use callback, otherwise call dma_wait
TEST_RT_MMU			= 1

# Set build configuration here
CONFIG_SYSTICK			= 1
CONFIG_SLEEP_TIMER 		= 1 # implement sleep() using a timer
CONFIG_HPPS_TRCH_MAILBOX 	= 1
CONFIG_HPPS_TRCH_MAILBOX_SSW 	= 1
CONFIG_RTPS_TRCH_MAILBOX 	= 1
CONFIG_TRCH_WDT 		= 1
CONFIG_RTPS_WDT 		= 1
CONFIG_HPPS_WDT 		= 1
CONFIG_TRCH_DMA 		= 1
CONFIG_RT_MMU 			= 1 # RTPS/TRCH->HPPS MMU
CONFIG_BOOT_RTPS 		= 1
CONFIG_BOOT_HPPS 		= 1

ifeq ($(strip $(CONFIG_SLEEP_TIMER)),1)
ifneq ($(strip $(CONFIG_SYSTICK)),1)
$(error CONFIG_SLEEP_TIMER requires a timer to be enabled)
endif
endif

# Most tests are standalone, but some are not
ifeq ($(strip $(TEST_RT_MMU)),1)
ifneq ($(strip $(CONFIG_RT_MMU)),1)
$(error TEST_RT_MMU requires CONFIG_RT_MMU)
endif
endif

CONFIG_ARGS = $(foreach m,$(CONFIG_FLAGS),-D$(m)=$($(m)))

CPU_FLAGS = -mcpu=cortex-m4 -mthumb 
FLOAT_FLAG = -mfloat-abi=hard 
AOPS = --warn --fatal-warnings $(CPU_FLAGS) $(FLOAT_FLAG)
INC_FLAGS = -I. -Itests $(foreach d,$(SUBDIRS),-I../$(d)) \

COPS = \
	-Wall -Werror \
	-O0 -g \
	$(CPU_FLAGS) $(FLOAT_FLAG) \
	-nostdlib -nostartfiles -ffreestanding \
	-DUART_BASE=LSIO_UART0_BASE \
	-DPRINTF_SUPPORT_FLOAT \
	-DPRINTF_NO_DOUBLE_WORKAROUND \
	$(CONFIG_ARGS) \
	$(INC_FLAGS) \

# -z max-page-size: alignment of program headers in the ELF file
#	removes large gaps (0x10000) in the ELF file (see: readelf -l)
LDFLAGS = \
	-z max-page-size=4 \

VPATH = ..
OBJS = \
       vectors.o \
       isr.o \
       lib/sha256.o \
       lib/mem.o \
       lib/str.o \
       lib/sleep.o \
       lib/object.o \
       lib/balloc.o \
       lib/printf.o \
       lib/panic.o \
       lib/intc.o \
       lib/mailbox-link.o \
       lib/command.o \
       drivers/systick.o \
       drivers/nvic.o \
       drivers/uart.o \
       drivers/smc.o \
       drivers/wdt.o \
       drivers/mmu.o \
       drivers/dma.o \
       drivers/mailbox.o \
       subsys.o \
       reset.o \
       boot.o \
       main.o \

ifeq ($(call cfg-or,$(CONFIG_TRCH_WDT) $(CONFIG_RTPS_WDT) $(CONFIG_HPPS_WDT)),1)
OBJS += watchdog.o
endif
ifeq ($(strip $(CONFIG_RT_MMU)),1)
OBJS += mmus.o
endif
ifeq ($(strip $(CONFIG_TRCH_DMA)),1)
OBJS += dmas.o
endif
ifeq ($(call cfg-or,\
	$(CONFIG_HPPS_TRCH_MAILBOX) \
	$(CONFIG_HPPS_TRCH_MAILBOX_SSW) \
	$(CONFIG_RTPS_TRCH_MAILBOX)),1)
OBJS += mailbox-isr.o server.o
endif

ifeq ($(strip $(TEST_FLOAT)),1)
OBJS += tests/float.o
endif
ifeq ($(strip $(TEST_SYSTICK)),1)
OBJS += tests/systick.o
endif
ifeq ($(strip $(TEST_TRCH_DMA)),1)
OBJS += tests/dma.o
endif
ifeq ($(strip $(TEST_RT_MMU)),1)
OBJS += tests/mmu.o
endif
ifeq ($(call cfg-or,$(TEST_TRCH_WDT) $(TEST_RTPS_WDT) $(TEST_HPPS_WDT)),1)
OBJS += tests/wdt.o
endif

TARGET=trch

.DEFAULT_GOAL := $(BLDDIR)/$(TARGET).bin

clean:
	rm -rf $(BLDDIR) vectors.s vectors.s.gen isr.c isr.c.gen

isrs: irqmap genisr.py Makefile
	./genisr.py -v $(INC_FLAGS) $(CONFIG_ARGS) --irqmap $< vectors.s.gen isr.c.gen
.INTERMEDIATE: isrs
vectors.s.gen: isrs
isr.c.gen: isrs
vectors.s: vectors.s.gen isrs
	cp $< $@
isr.c: isr.c.gen isrs
	cp $< $@

$(BLDDIR)/%.o : %.s | dirs
	$(ARM_NONE_EABI)-as $(AOPS) $< -o $@

$(BLDDIR)/%.o : %.c | dirs
	$(ARM_NONE_EABI)-gcc -MMD $(COPS) -c $< -o $@

$(BLDDIR)/$(TARGET).dbg.elf: trch.ld $(addprefix $(BLDDIR)/,$(OBJS))
	$(ARM_NONE_EABI)-ld $(LDFLAGS) -o $@ -T $^

$(BLDDIR)/$(TARGET).elf : $(BLDDIR)/$(TARGET).dbg.elf
	$(ARM_NONE_EABI)-strip -x -s -R .comment -R '.ARM.*' -o $@ $<

$(BLDDIR)/$(TARGET).bin : $(BLDDIR)/$(TARGET).elf
	$(ARM_NONE_EABI)-objcopy -O binary $< $@

$(addprefix $(BLDDIR)/,$(OBJS)) : Makefile

-include $(addprefix $(BLDDIR)/,$(OBJS:.o=.d))
