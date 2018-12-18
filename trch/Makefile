include ../Makefile.common

# List all possible test flags here
TEST_FLAGS = \
	TEST_IPI \
	TEST_FLOAT \
	TEST_SYSTICK \
	TEST_SYSTICK_STANDALONE \
	TEST_HPPS_TRCH_MAILBOX \
	TEST_HPPS_TRCH_MAILBOX_SSW \
	TEST_RTPS_TRCH_MAILBOX \
	TEST_TRCH_WDT \
	TEST_TRCH_WDT_STANDALONE \
	TEST_RTPS_WDT \
	TEST_RTPS_WDT_STANDALONE \
	TEST_HPPS_WDT \
	TEST_HPPS_WDT_STANDALONE \
	TEST_TRCH_DMA \
	TEST_TRCH_DMA_STANDALONE \
	TEST_TRCH_DMA_STANDALONE_CB \
	TEST_RT_MMU \
	TEST_RT_MMU_STANDALONE \
	TEST_BOOT_RTPS \
	TEST_BOOT_HPPS \

# Enable disable tests here:
TEST_IPI 			= 0
TEST_FLOAT 			= 1
TEST_SYSTICK			= 1
TEST_SYSTICK_STANDALONE		= 0
TEST_HPPS_TRCH_MAILBOX 		= 1
TEST_HPPS_TRCH_MAILBOX_SSW 	= 1
TEST_RTPS_TRCH_MAILBOX 		= 1
TEST_TRCH_WDT 			= 1
TEST_TRCH_WDT_STANDALONE 	= 0
TEST_RTPS_WDT 			= 1
TEST_RTPS_WDT_STANDALONE 	= 0
TEST_HPPS_WDT 			= 1
TEST_HPPS_WDT_STANDALONE 	= 0
TEST_TRCH_DMA_STANDALONE	= 1
TEST_TRCH_DMA_STANDALONE_CB 	= 1 # if set, use callback, otherwise call dma_wait
TEST_TRCH_DMA 			= 1
TEST_RT_MMU 			= 1 # RTPS/TRCH->HPPS MMU
TEST_RT_MMU_STANDALONE		= 1
TEST_BOOT_RTPS 			= 1
TEST_BOOT_HPPS 			= 1

TEST_ARGS = $(foreach m,$(TEST_FLAGS),-D$(m)=$($(m)))

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
	$(TEST_ARGS) \
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
       lib/delay.o \
       lib/object.o \
       lib/balloc.o \
       lib/printf.o \
       lib/panic.o \
       lib/intc.o \
       lib/mailbox-link.o \
       lib/command.o \
       drivers/mailbox.o \
       drivers/nvic.o \
       drivers/uart.o \
       drivers/mmu.o \
       drivers/dma.o \
       drivers/wdt.o \
       drivers/smc.o \
       systick.o \
       watchdog.o \
       reset.o \
       boot.o \
       server.o \
       main.o \


ifeq ($(strip $(TEST_TRCH_DMA)),1)
OBJS += test-dma.o
endif
ifeq ($(strip $(TEST_RT_MMU_STANDALONE)),1)
OBJS += test-mmu.o
endif
ifeq ($(strip $(TEST_RT_MMU)),1)
OBJS += mmus.o
endif
ifeq ($(strip $(TEST_TRCH_DMA)),1)
OBJS += dmas.o
endif
ifeq ($(strip $(TEST_FLOAT)),1)
OBJS += test-float.o
endif
ifeq ($(strip $(TEST_SYSTICK_STANDALONE)),1)
OBJS += test-systick.o
endif
ifeq ($(if $(filter-out 0,$(strip \
	$(TEST_HPPS_TRCH_MAILBOX) \
	$(TEST_HPPS_TRCH_MAILBOX_SSW) \
	$(TEST_RTPS_TRCH_MAILBOX))),1,0),1)
OBJS += mailbox-isr.o
endif
ifeq ($(if $(filter-out 0,$(strip \
	$(TEST_TRCH_WDT_STANDALONE) \
	$(TEST_RTPS_WDT_STANDALONE) \
	$(TEST_HPPS_WDT_STANDALONE))),1,0),1)
OBJS += test-wdt.o
endif

TARGET=trch

.DEFAULT_GOAL := $(BLDDIR)/$(TARGET).bin

clean:
	rm -rf $(BLDDIR) vectors.s isr.c

isrs: irqmap genisr.py Makefile
	./genisr.py -v $(INC_FLAGS) $(TEST_ARGS) --irqmap $< vectors.s isr.c
.INTERMEDIATE: isrs
vectors.s: isrs
isr.c: isrs

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
