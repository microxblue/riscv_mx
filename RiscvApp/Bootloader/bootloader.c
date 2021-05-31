//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#include "common.h"
#include "firmware.h"

/* ------------------------------------------------------------------------- */
/* Intel HEX file record types                                               */
/* ------------------------------------------------------------------------- */
#define HEX_DATA_REC          0   /* data record */
#define HEX_EOF_REC           1   /* end of file record */
#define HEX_SEG_ADDR_REC      3   /* start segment address record */
#define HEX_EXT_ADDR_REC      4   /* extened linear address record */
#define HEX_START_ADDR_REC    5   /* start linear address record */
#define HEX_READ_ADDR_REC     6   /* read data at address record */

#define MAX_LEN  0x80

#define FLAG_PRINT_DOT   0x01

#define  WAIT_FOR_DONE   while (m_spi_base[1] & B_SPI_STS_BUSY)
#define  DEASSERT_CS     m_spi_base[1] = 0
#define  SPI_RD_DATA     m_spi_base[2]

volatile DWORD *m_spi_base = (volatile DWORD *)(SPI_FLASH_BASE + SPI_CMD_OFF);

typedef int (*IMG_ENTRY)(void);

static
void  memcopy (volatile DWORD *dest, volatile DWORD *src, DWORD cont)
{
  cont = (cont + 3) >> 2;
  while (cont--) {
    *dest++ = *src++;
  }
}

#define WR_MEM   0x01
#define RD_MEM   0x02
#define WR_CONT  0x03
#define RD_CONT  0x04
#define WR_FLAG  0x05
#define RD_SPI   0x06
#define OP_DONE  0x0D
#define OP_EXE   0x0E

#define FLAG_PRINT_DOT   0x01

#define ARG_IDX  0x00
#define CMD_IDX  0x01

static
void bootloader(void)
{
  DWORD   wcont;
  DWORD   waddr;
  DWORD   rcont;
  DWORD   raddr;
  DWORD   command;
  DWORD   pcnt;
  BYTE    cmdtype;
  BYTE    loop;

  IMG_ENTRY entry;
  volatile DWORD *dpramdat = (volatile DWORD *)DPRAM_DAT;
  volatile DWORD *dpramcmd = (volatile DWORD *)DPRAM_CMD;

  raddr = 0;
  waddr = 0;
  pcnt  = 0;
  dpramcmd[CMD_IDX] = 0;

  loop = 1;
  while (loop) {

    command = dpramcmd[CMD_IDX];
    if ((!(command & 0xF000)) || (0xF000 == (command & 0xF000))) {  // No commands
      continue;
    }

    cmdtype = (command & 0xF000) >> 12;
    switch (cmdtype) {

    case RD_MEM: // rd data from memory location
      raddr  = dpramcmd[ARG_IDX];

    case RD_CONT:
      rcont  = command & 0x03FF;
      if (rcont > 512) rcont = 512;
      memcopy (dpramdat, (DWORD *)raddr, rcont);
      raddr += rcont;
      if ((pcnt & 0x1F) == 0) {
        putchar('.');
      }
      break;

    case WR_MEM: // wr data to memory location
      waddr  = dpramcmd[ARG_IDX];

    case WR_CONT:
      wcont  = command & 0x03FF;
      if (wcont > 512) wcont = 512;
      memcopy ((DWORD *)waddr, dpramdat, wcont);
      waddr += wcont;
      if ((pcnt & 0x1F) == 0) {
        putchar('.');
      }
      break;

    case OP_EXE:
      dpramcmd[CMD_IDX] = 0;
      entry  = (IMG_ENTRY)(dpramcmd[ARG_IDX]);
      // flush cache
      entry();
      break;

    case OP_DONE:
      // flush cache
      printf("\nDone\n");
      loop = 0;
      break;

    default:
      break;
    }
    dpramcmd[CMD_IDX] = 0;
    pcnt++;
  }
}

static
VOID  spi_command (DWORD Cmd, DWORD Arg)
{
  m_spi_base[0] = Cmd;
  m_spi_base[1] = B_SPI_CTL_RUN + Arg;
  WAIT_FOR_DONE;
}

static
void bootshell (void)
{
  UINT32    rlen;
  UINT32    length;
  UINT32    memadr;
  IMG_ENTRY entry;

  memadr = 0;
  length = 0x4000;
  entry  = (IMG_ENTRY)(memadr);
  // Fast read + Address
  spi_command (0x0B000000 + 0x0700000, B_SPI_CTL_CS_ASSERT);
  // One dummy
  spi_command (0, B_SPI_CTL_CS_ASSERT + 1);
  while (length > 0) {
    // Read 32 bits
    spi_command (0, B_SPI_CTL_CS_ASSERT);
    if (length > sizeof(UINT32)) {
      rlen = sizeof(UINT32);
    } else {
      rlen = length;
    }
    memcpy ((void *)memadr, (void *)(UINT32)&SPI_RD_DATA, rlen);
    length  -= rlen;
    memadr  += rlen;
  }
  DEASSERT_CS;

  // flush cache
  entry();
}

unsigned int *irq(unsigned int *regs, unsigned int irqs)
{
  (void)irqs;
  return regs;
}


int main (void)
{
  if (MMIO(GPIO_BTN_DIP) & B_DIP_1) {
    printf("\nCYCLONE10LP RISC-V Loader v1.0: OK\n");
    bootloader ();
  } else {
    bootshell ();
  }

  return 0;
}
