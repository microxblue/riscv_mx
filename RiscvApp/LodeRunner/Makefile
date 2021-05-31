.PHONY: all clean run prebuild

TOOLCHAIN_PREFIX = riscv-none-embed-
CC               = $(TOOLCHAIN_PREFIX)gcc
GCC_WARNS        = -Wno-builtin-declaration-mismatch
CPFLAGS          = -march=rv32im$(subst C,c,$(COMPRESSED_ISA)) -O2 --std=c99 $(GCC_WARNS) -ffreestanding -nostdlib -I../Common
ASFLAGS          = -march=rv32im$(subst C,c,$(COMPRESSED_ISA))
LDFLAGS          = -O2 -ffreestanding -nostdlib
INCLUDES         = -I . -I ../Common
TOOLDIR          = $(TOOLDIR)

SRCDIR           = .
OUTDIR           = out
DATDIR           = data
MEM_TGT         ?= sram2

ifeq ($(MEM_TGT),sdram)
  TGT_ADDR = 0xF0000000
  LDS_FILE = sdram
endif
ifeq ($(MEM_TGT),sram)
	TGT_ADDR = 0
  LDS_FILE = sram
endif
ifeq ($(MEM_TGT),sram2)
  TGT_ADDR = 0x200
  LDS_FILE = sram2
endif


APP  = LodeRunner

BINS = $(DATDIR)/PALETTE.BIN $(DATDIR)/PALETTE.BIN $(DATDIR)/SPLASH.BIN $(DATDIR)/TILES.BIN  \
       $(DATDIR)/CLASSIC.BIN $(DATDIR)/CHAMP.BIN $(DATDIR)/PRO.BIN $(DATDIR)/FANBOOK.BIN  \
		   $(DATDIR)/REVENGE.BIN $(DATDIR)/REVENGE.BIN $(DATDIR)/CUSTOM.BIN $(DATDIR)/SPRITES_EXT.BIN

all: prebuild $(BINS) $(OUTDIR)/$(APP).bin

$(OUTDIR)/%.o : $(SRCDIR)/../Common/%.c
		$(CC) -c $(CPFLAGS) $< -o $@

$(OUTDIR)/%.o : $(SRCDIR)/../Common/%.S
		$(CC) -c $(ASFLAGS) $< -o $@

$(OUTDIR)/%.o : $(SRCDIR)/%.c
		$(CC) -c $(CPFLAGS) $< -o $@

$(OUTDIR)/%.o : $(SRCDIR)/%.S
		$(CC) -c $(ASFLAGS) $< -o $@

FIRMWARE_OBJS=\
    $(OUTDIR)/start.o \
    $(OUTDIR)/entry.o \
    $(OUTDIR)/tprintf.o \
    $(OUTDIR)/common.o \
    $(OUTDIR)/mainloop.o \
    $(OUTDIR)/levels.o \
    $(OUTDIR)/runner.o \
    $(OUTDIR)/guard.o \
    $(OUTDIR)/screen.o \
    $(OUTDIR)/sound.o \
    $(OUTDIR)/splash.o \
    $(OUTDIR)/state.o \
    $(OUTDIR)/lib.o

$(DATDIR)/SPRITES_EXT.BIN: assets\sprites_ext.txt
	python $(TOOLDIR)\buildimage.py spr_c16 16 8 assets/sprites_ext.txt $(DATDIR)/SPRITES_EXT.BIN

$(DATDIR)/PALETTE.BIN: assets/palette.txt
	python scripts/ascii2bin.py --input assets/palette.txt --output $(DATDIR)/PALETTE.BIN --addr 1000

$(DATDIR)/SPLASH.BIN: assets/splash.txt
	python scripts/ascii2bin.py --input assets/splash.txt --output $(DATDIR)/SPLASH.BIN --addr a000

$(DATDIR)/TILES.BIN: assets/tiles.txt
	python scripts/graphic.py --input assets/tiles.txt --output $(DATDIR)/TILES.BIN --x 8 --y 8 --bpp 2 --obpp 16 --addr 0000

$(DATDIR)/CLASSIC.BIN: assets/classic.json scripts/levels.py
	python scripts/levels.py --input assets/classic.json --output $(DATDIR)/CLASSIC.BIN

$(DATDIR)/CHAMP.BIN: assets/championship.json scripts/levels.py
	python scripts/levels.py --input assets/championship.json --output $(DATDIR)/CHAMP.BIN

$(DATDIR)/PRO.BIN: assets/professional.json scripts/levels.py
	python scripts/levels.py --input assets/professional.json --output $(DATDIR)/PRO.BIN

$(DATDIR)/FANBOOK.BIN: assets/fanbook.json scripts/levels.py
	python scripts/levels.py --input assets/fanbook.json --output $(DATDIR)/FANBOOK.BIN

$(DATDIR)/REVENGE.BIN: assets/revenge.json scripts/levels.py
	python scripts/levels.py --input assets/revenge.json --output $(DATDIR)/REVENGE.BIN

$(DATDIR)/CUSTOM.BIN: assets/custom.json scripts/levels.py
	python scripts/levels.py --input assets/custom.json --output $(DATDIR)/CUSTOM.BIN

$(DATDIR)/SPRITES.BIN: assets/sprites.txt scripts/graphic.py
	python scripts/graphic.py --input assets/sprites.txt --output $(DATDIR)/SPRITES.BIN --x 8 --y 8 --bpp 4 --addr e000

$(OUTDIR)/%.o : $(SRCDIR)/%.c
	$(CC) -c $(CPFLAGS) $< -o $@

$(OUTDIR)/$(APP).elf: $(FIRMWARE_OBJS) $(SRCDIR)/../Common/$(LDS_FILE).lds Makefile
		$(CC) $(LDFLAGS) -o $@ -Wl,-Bstatic,-T,$(SRCDIR)/../Common/$(LDS_FILE).lds,-Map,$(OUTDIR)/$(APP).map,--strip-debug $(FIRMWARE_OBJS) -lgcc
		$(TOOLCHAIN_PREFIX)objdump -S $@ > $@.s

$(OUTDIR)/$(APP).bin: $(BINS)
$(OUTDIR)/$(APP).bin: $(OUTDIR)/$(APP).elf
	$(TOOLCHAIN_PREFIX)objcopy -O binary $< $@

run: prebuild $(OUTDIR)/$(APP).bin
		python $(TOOLDIR)/ftditalk.py  none 0x80 DEDIPROG:ftdi:2232/1 r
		python $(TOOLDIR)/ftditalk.py  $(DATDIR)/classic.bin      0xf0102000   DEDIPROG:ftdi:2232/1  u+
		python $(TOOLDIR)/ftditalk.py  $(DATDIR)/champ.bin        0xf010c000   DEDIPROG:ftdi:2232/1  u+
		python $(TOOLDIR)/ftditalk.py  $(DATDIR)/pro.bin          0xf0110000   DEDIPROG:ftdi:2232/1  u+
		python $(TOOLDIR)/ftditalk.py  $(DATDIR)/fanbook.bin      0xf011a000   DEDIPROG:ftdi:2232/1  u+
		python $(TOOLDIR)/ftditalk.py  $(DATDIR)/revenge.bin      0xf01e0000   DEDIPROG:ftdi:2232/1  u+
		python $(TOOLDIR)/ftditalk.py  $(DATDIR)/custom.bin       0xf0120000   DEDIPROG:ftdi:2232/1  u+
		python $(TOOLDIR)/ftditalk.py  $(DATDIR)/splash.bin       0xf012a000   DEDIPROG:ftdi:2232/1  u+
		python $(TOOLDIR)/ftditalk.py  $(DATDIR)/tiles.bin:0x2    0xF0770000   DEDIPROG:ftdi:2232/1  u+
		python $(TOOLDIR)/ftditalk.py  $(DATDIR)/sprites_ext.bin  0xf0740000   DEDIPROG:ftdi:2232/1  u+
		@python $(TOOLDIR)/ftditalk.py $(OUTDIR)/$(APP).bin $(TGT_ADDR) DEDIPROG:ftdi:2232/1 e

prebuild:
		@if not exist $(OUTDIR)   mkdir $(OUTDIR)
		@if not exist $(DATDIR)   mkdir $(DATDIR)

clean:
		@if exist $(OUTDIR) rmdir /s /q $(OUTDIR)
		@if exist $(DATDIR) rmdir /s /q $(DATDIR)

