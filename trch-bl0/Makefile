include ../Makefile.common

# List all possible test flags here
CONFIG_FLAGS = 			

include Makefile.defconfig
include Makefile.config

CONFIG_ARGS = $(foreach m,$(CONFIG_FLAGS),-D$(m)=$($(m)))
CONFIG_ARGS += -DCONFIG_CONSOLE__$(CONFIG_CONSOLE)

CPU_FLAGS = -mcpu=cortex-m4 -mthumb 
FLOAT_FLAG = -mfloat-abi=hard 
AOPS = --warn --fatal-warnings $(CPU_FLAGS) $(FLOAT_FLAG)
INC_FLAGS = -I. -Itests $(foreach d,$(SUBDIRS),-I../$(d)) \

COPS = \
	-Wall -Werror \
	-fpic \
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
# vectors.o and isrs.o must be first
OBJS = \
       vectors.o \
       _main.o \
       relocate_code.o \
       isrs.o \
       drivers/cortex-m4.o \
       drivers/ns16550.o \
       drivers/smc.o \
       drivers/systick.o \
       lib/ecc.o \
       lib/intc.o \
       lib/mem.o \
       lib/object.o \
       lib/panic.o \
       lib/printf.o \
       lib/sha256.o \
       lib/str.o \
       plat/board.o \
       plat/console.o \
       main.o \
       sections.o \

TARGET=trch-bl0

.DEFAULT_GOAL := $(BLDDIR)/$(TARGET).bin

clean:
	rm -rf $(BLDDIR) vectors.s vectors.s.gen isr.c isr.c.gen

isrs: irqmap genisr.py Makefile Makefile.defconfig Makefile.config
	./genisr.py -v $(INC_FLAGS) $(CONFIG_ARGS) --irqmap $< vectors.s.gen isr.c.gen
.INTERMEDIATE: isrs
vectors.s.gen: isrs
isr.c.gen: isrs
vectors.s: vectors.s.gen isrs
	cp $< $@
isr.c: isr.c.gen isrs
	cp $< $@

$(BLDDIR)/%.o : %.s | dirs
	$(CROSS_COMPILE)as $(AOPS) $< -o $@

$(BLDDIR)/%.o : %.c | dirs
	$(CROSS_COMPILE)gcc -MMD $(COPS) -c $< -o $@

$(BLDDIR)/$(TARGET).dbg.elf: trch.ld $(addprefix $(BLDDIR)/,$(OBJS))
	$(CROSS_COMPILE)ld $(LDFLAGS) -o $@ -T $^

$(BLDDIR)/$(TARGET).elf : $(BLDDIR)/$(TARGET).dbg.elf
	$(CROSS_COMPILE)strip -x -s -R .comment -R '.ARM.*' -o $@ $<

$(BLDDIR)/$(TARGET).bin : $(BLDDIR)/$(TARGET).elf
	$(CROSS_COMPILE)objcopy -O binary $< $@

$(addprefix $(BLDDIR)/,$(OBJS)) : Makefile Makefile.defconfig Makefile.config

# Empty config means all settings come from defconfig
Makefile.config:
	touch $@

-include $(addprefix $(BLDDIR)/,$(OBJS:.o=.d))
