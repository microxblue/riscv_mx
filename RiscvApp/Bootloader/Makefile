APP              = firmware
COMPRESSED_ISA   =
GCC_WARNS        = -Wall -Wextra -Wshadow -Wundef -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wno-implicit-fallthrough
GCC_WARNS       += -Wredundant-decls -Wstrict-prototypes -Wmissing-prototypes -pedantic # -Wconversion
TOOLCHAIN_PREFIX = riscv-none-embed-
SRCDIR           = .
OUTDIR           = out
CC               = $(TOOLCHAIN_PREFIX)gcc
CPFLAGS          = -march=rv32im$(subst C,c,$(COMPRESSED_ISA)) -Os --std=c99 $(GCC_WARNS) -ffreestanding -nostdlib -I../Common
ASFLAGS          = -march=rv32im$(subst C,c,$(COMPRESSED_ISA))
LDFLAGS          = -Os -ffreestanding -nostdlib
MODE             = loader
MEM_TGT          = sram2

all: prebuild $(OUTDIR)/$(APP).mif

FIRMWARE_OBJS=\
    $(OUTDIR)/start.o \
    $(OUTDIR)/entry.o \
    $(OUTDIR)/tprintf.o \
    $(OUTDIR)/common.o

ifeq ($(MODE),shell)
	FIRMWARE_OBJS += $(OUTDIR)/memory.o $(OUTDIR)/shell.o
else
  FIRMWARE_OBJS += $(OUTDIR)/bootloader.o
endif


$(OUTDIR)/%.o : $(SRCDIR)/../Common/%.c
		$(CC) -c $(CPFLAGS) $< -o $@

$(OUTDIR)/%.o : $(SRCDIR)/../Common/%.S
		$(CC) -c $(ASFLAGS) $< -o $@

$(OUTDIR)/%.o : $(SRCDIR)/%.c
		$(CC) -c $(CPFLAGS) $< -o $@

$(OUTDIR)/%.o : $(SRCDIR)/%.S
		$(CC) -c $(ASFLAGS) $< -o $@

$(OUTDIR)/$(APP).mif: $(OUTDIR)/$(APP).bin $(SRCDIR)/../Common/makemif.py
		python $(SRCDIR)/../Common/makemif.py  $< 10240 > $@
		python $(SRCDIR)/../Common/makebins.py $< 10240 $(OUTDIR)/$(APP)

$(OUTDIR)/$(APP).bin: $(OUTDIR)/$(APP).elf
		$(TOOLCHAIN_PREFIX)objcopy -O binary $< $@

$(OUTDIR)/$(APP).elf: $(FIRMWARE_OBJS) $(SRCDIR)/sram.lds
		$(CC) $(LDFLAGS) -o $@ -Wl,-Bstatic,-T,$(SRCDIR)/sram.lds,-Map,$(OUTDIR)/firmware.map,--strip-debug $(FIRMWARE_OBJS) -lgcc
		$(TOOLCHAIN_PREFIX)objdump -S $@ > $@.s

prebuild:
		@if not exist $(OUTDIR)   mkdir $(OUTDIR)

clean:
		@if exist $(OUTDIR) rmdir /s /q $(OUTDIR)

.PHONY:  clean
