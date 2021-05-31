APP              = Shell
COMPRESSED_ISA   =
GCC_WARNS        = -Wall -Wextra -Wshadow -Wundef -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wno-implicit-fallthrough
GCC_WARNS       += -Wredundant-decls -Wstrict-prototypes -Wmissing-prototypes -pedantic
GCC_WARNS       += -Wno-unused-function -Wno-cast-align
TOOLCHAIN_PREFIX = riscv-none-embed-
SRCDIR           = .
OUTDIR           = out
CC               = $(TOOLCHAIN_PREFIX)gcc
CPFLAGS          = -march=rv32im$(subst C,c,$(COMPRESSED_ISA)) -O2 --std=c99 $(GCC_WARNS) -ffreestanding -nostdlib -I../Common
ASFLAGS          = -march=rv32im$(subst C,c,$(COMPRESSED_ISA))
LDFLAGS          = -O2 -ffreestanding -nostdlib
TOOLDIR          = ../../Tools
MEM_TGT          = sram2

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

all: prebuild $(OUTDIR)/Shell.bin

FIRMWARE_OBJS=\
    $(OUTDIR)/start.o \
    $(OUTDIR)/entry.o \
    $(OUTDIR)/tprintf.o \
    $(OUTDIR)/common.o \
    $(OUTDIR)/memory.o \
    $(OUTDIR)/firmware.o \
    $(OUTDIR)/bootloader.o \
    $(OUTDIR)/flash.o \
    $(OUTDIR)/demo.o \
    $(OUTDIR)/shell.o

$(OUTDIR)/%.o : $(SRCDIR)/../Common/%.c
		$(CC) -c $(CPFLAGS) $< -o $@

$(OUTDIR)/%.o : $(SRCDIR)/../Common/%.S
		$(CC) -c $(ASFLAGS) $< -o $@

$(OUTDIR)/%.o : $(SRCDIR)/%.c
		$(CC) -c $(CPFLAGS) $< -o $@

$(OUTDIR)/%.o : $(SRCDIR)/%.S
		$(CC) -c $(ASFLAGS) $< -o $@

$(OUTDIR)/Shell.mif: $(OUTDIR)/Shell.bin $(SRCDIR)/makemif.py
		python ../Common/makemif.py $< 8192 > $@

$(OUTDIR)/Shell.bin: $(OUTDIR)/Shell.elf
		$(TOOLCHAIN_PREFIX)objcopy -O binary $< $@

$(OUTDIR)/Shell.elf: $(FIRMWARE_OBJS) $(SRCDIR)/../Common/$(LDS_FILE).lds Makefile
		$(CC) $(LDFLAGS) -o $@ -Wl,-Bstatic,-T,$(SRCDIR)/../Common/$(LDS_FILE).lds,-Map,$(OUTDIR)/Shell.map,--strip-debug $(FIRMWARE_OBJS) -lgcc
		$(TOOLCHAIN_PREFIX)objdump -S $@ > $@.s

images:
		@if not exist ..\..\Demo   mkdir ..\..\Demo
		python  $(TOOLDIR)/imgtool.py img2  ../../Images/space.jpg        ../../Demo/space.bin
		python  $(TOOLDIR)/imgtool.py spr3s ../../Images/fight_spr_r.png  ../../Demo/fight_spr_r.bin
		python  $(TOOLDIR)/imgtool.py spr3s ../../Images/fight_spr_b.png  ../../Demo/fight_spr_b.bin
		python  $(TOOLDIR)/imgtool.py sprb  ../../Images/sprite32.png     ../../Demo/sprite32.bin
		python  $(TOOLDIR)/imgtool.py sprh  ../../Images/sprite64.png     ../../Demo/sprite64.bin
		python  $(TOOLDIR)/imgtool.py img1  ../../Images/raiden.png       ../../Demo/raiden.bin
		python  $(TOOLDIR)/imgtool.py img2  ../../Images/clouds.jpg       ../../Demo/clouds.bin
		python  $(TOOLDIR)/imgtool.py img2  ../../Images/space.jpg        ../../Demo/space.bin
		python  $(TOOLDIR)/imgtool.py img3  ../../Images/raiden.png       ../../Demo/raiden_tile.bin
		python  $(TOOLDIR)/imgtool.py spr3s ../../Images/fight_spr_r.png  ../../Demo/fight_spr_r.bin
		python  $(TOOLDIR)/imgtool.py spr3s ../../Images/fight_spr_b.png  ../../Demo/fight_spr_b.bin
		python  $(TOOLDIR)/imgtool.py spr3b ../../Images/fight_spr_r.png  ../../Demo/fight_spr_r32.bin
		python  $(TOOLDIR)/imgtool.py spr3b ../../Images/fight_spr_b.png  ../../Demo/fight_spr_b32.bin
		python  $(TOOLDIR)/imgtool.py sprb  ../../Images/sprite32.png     ../../Demo/sprite32.bin
		python  $(TOOLDIR)/imgtool.py sprh  ../../Images/sprite64.png     ../../Demo/sprite64.bin
		python  $(TOOLDIR)/prep_raiden.py   ../../Images                  ../../Demo


sprite:
		@python $(TOOLDIR)/ftditalk.py none 0x80 DEDIPROG:ftdi:2232/1 r
		@python $(TOOLDIR)/ftditalk.py $(OUTDIR)/Shell.bin $(TGT_ADDR) DEDIPROG:ftdi:2232/1 e
		@python $(TOOLDIR)/ftditalk.py "dl" 0x0000  DEDIPROG:ftdi:2232/1 c
		@python $(TOOLDIR)/ftditalk.py ../../Demo/fight_spr_b.bin 0xF0740000 DEDIPROG:ftdi:2232/1 u+
		@python $(TOOLDIR)/ftditalk.py ../../Demo/fight_spr_r.bin 0xF0741800 DEDIPROG:ftdi:2232/1 u+
		@python $(TOOLDIR)/ftditalk.py ../../Demo/sprite32.bin      0xF0750000 DEDIPROG:ftdi:2232/1 u+
		@python $(TOOLDIR)/ftditalk.py ../../Demo/sprite64.bin      0xF0760000 DEDIPROG:ftdi:2232/1 u+
		@python $(TOOLDIR)/ftditalk.py ../../Demo/space.bin 0xF07A0000 DEDIPROG:ftdi:2232/1 u
		@python $(TOOLDIR)/ftditalk.py "dm 1" 0x0000  DEDIPROG:ftdi:2232/1 c


raiden:
		@python $(TOOLDIR)/ftditalk.py  none 0x80 DEDIPROG:ftdi:2232/1 r
		@python $(TOOLDIR)/ftditalk.py $(OUTDIR)/Shell.bin $(TGT_ADDR) DEDIPROG:ftdi:2232/1 e
		@python $(TOOLDIR)/ftditalk.py dl  0 DEDIPROG:ftdi:2232/1 c
		@python $(TOOLDIR)/ftditalk.py ../../Images/bg.bin 0xf0400000 DEDIPROG:ftdi:2232/1 u+
		@python $(TOOLDIR)/ftditalk.py ../../Images/fight_spr_r32.bin 0xf0750000 DEDIPROG:ftdi:2232/1 u+
		@python $(TOOLDIR)/ftditalk.py ../../Images/fight_spr_b32.bin 0xf0751800 DEDIPROG:ftdi:2232/1 u+
		@python $(TOOLDIR)/ftditalk.py ../../Images/shoot_8_1.bin 0xf0740000 DEDIPROG:ftdi:2232/1 u+
		@python $(TOOLDIR)/ftditalk.py ../../Images/bomb_64_9.bin 0xf0760000 DEDIPROG:ftdi:2232/1 u
		@python $(TOOLDIR)/ftditalk.py "dm 2" 0x0000  DEDIPROG:ftdi:2232/1 c


run: $(OUTDIR)/Shell.bin
		python  $(TOOLDIR)/ftditalk.py  none 0x80 DEDIPROG:ftdi:2232/1 r
		@python $(TOOLDIR)/ftditalk.py $(OUTDIR)/Shell.bin $(TGT_ADDR) DEDIPROG:ftdi:2232/1 e

burn: $(OUTDIR)/Shell.bin
		$(eval SHELL_SIZE := $(shell python -c 'import os; print(hex(os.path.getsize(r"$(OUTDIR)/Shell.bin")))'))
		@python $(TOOLDIR)/ftditalk.py "dl" 0 DEDIPROG:ftdi:2232/1 c
		@python $(TOOLDIR)/ftditalk.py $(OUTDIR)/Shell.bin 0xf0f00000 DEDIPROG:ftdi:2232/1 u
		@python $(TOOLDIR)/ftditalk.py "pf 700000 $(SHELL_SIZE) F0F00000" 0 DEDIPROG:ftdi:2232/1 c

prebuild:
		@if not exist $(OUTDIR)   mkdir $(OUTDIR)

clean:
		@if exist $(OUTDIR) rmdir /s /q $(OUTDIR)

.PHONY:  clean
