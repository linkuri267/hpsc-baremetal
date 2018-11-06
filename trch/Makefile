include ../Makefile.common

# List all possible test flags here
TEST_FLAGS = \
	TEST_TRCH_DMA \
	TEST_TRCH_DMA_CB \
	TEST_RT_MMU \
	TEST_RT_MMU_ACCESS \
	TEST_RTPS \
	TEST_HPPS \
	TEST_FLOAT \
	TEST_IPI \
	TEST_HPPS_TRCH_MAILBOX \
	TEST_HPPS_TRCH_MAILBOX_SSW \
	TEST_RTPS_TRCH_MAILBOX \

# Enable disable tests here:
TEST_FLOAT = 1
TEST_IPI = 0
TEST_HPPS_TRCH_MAILBOX = 0
TEST_HPPS_TRCH_MAILBOX_SSW = 0
TEST_RTPS_TRCH_MAILBOX = 0
TEST_TRCH_DMA = 1
TEST_TRCH_DMA_CB = 1 # if set, use callback, otherwise call dma_wait
TEST_RT_MMU = 1 # RTPS/TRCH->HPPS MMU
TEST_RT_MMU_ACCESS = 1 # if not defined, then just init and deinit
TEST_RTPS = 1
TEST_HPPS = 1

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
       drivers/dma.o \
       reset.o \
       server.o \
       main.o \


ifeq ($(strip $(TEST_TRCH_DMA)),1)
OBJS += test-dma.o
endif
ifeq ($(strip $(TEST_RT_MMU)),1)
OBJS += test-mmu.o
endif
ifeq ($(strip $(TEST_FLOAT)),1)
OBJS += test-float.o
endif
ifeq ($(if $(filter-out 0,$(strip \
	$(TEST_HPPS_TRCH_MAILBOX) \
	$(TEST_HPPS_TRCH_MAILBOX_SSW) \
	$(TEST_RTPS_TRCH_MAILBOX))),1,0),1)
OBJS += mailbox-isr.o
endif

TARGET=trch.elf

.DEFAULT_GOAL := $(BLDDIR)/$(TARGET)

clean:
	rm -rf $(BLDDIR) vectors.s isr.c

isrs: irqmap genisr.py
	./genisr.py -v $(INC_FLAGS) $(TEST_ARGS) --irqmap $< vectors.s isr.c
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
