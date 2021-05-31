//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#include "firmware.h"
#include "Memory.h"

void WriteMem(UINT32 addr, UINT32 value, int unit)
{
  volatile BYTE    *pb;
  volatile WORD    *pw;
  volatile DWORD   *pd;

  if      (unit==1) {
    pb = (volatile BYTE *)addr;
    *pb =(BYTE)value;
  } else if (unit==2) {
    pw = (volatile WORD *)(addr&0xFFFFFFFE);
    *pw =(WORD)value;
  } else if (unit==4) {
    pd = (volatile DWORD *)(addr&0xFFFFFFFC);
    *pd =(DWORD)value;
  }

  return;
}

void ReadMem(UINT32 addr, UINT32 len, int unit)
{
  BYTE  dat;
  volatile BYTE  *pb = 0;
  volatile WORD  *pw = 0;
  volatile DWORD *pd = 0;
  volatile DWORD *fd = (volatile DWORD *)(SPI_FLASH_BASE + SPI_CMD_OFF);
  UINT32 addrstart, oldaddr, i, j, dw;
  BYTE  flash;

  flash = (unit >> 4) & 1;
  if (flash) {
    addr &= 0x7FFFFF;
  }
  unit &= 0x0F;

  if      (unit==1) {
    pb = (volatile BYTE  *)addr;
    oldaddr=(UINT32)pb;
  } else if (unit==2) {
    pw = (volatile WORD  *)(addr&0xFFFFFFFE);
    oldaddr=(UINT32)pw;
  } else if (unit==4) {
    pd = (volatile DWORD *)(addr&0xFFFFFFFC);
    oldaddr=(UINT32)pd;
  } else return;

  if (flash) {
    // Set address and fast read command
    fd[0] = 0x0B000000 + addr;
    // Send command and 3 address byes
    fd[1] = B_SPI_CTL_RUN + B_SPI_CTL_CS_ASSERT;
    while (fd[1] & B_SPI_STS_BUSY);
    // Send one dummy cycle
    fd[1] = B_SPI_CTL_RUN + B_SPI_CTL_CS_ASSERT + 0x01;
    while (fd[1] & B_SPI_STS_BUSY);
  }

  addrstart = addr & 0xFFFFFFF0;
  printf ("%08X: ",addrstart);
  for (i=0; i<len; i+=0) {
    if (unit == 1) {  //Read Flash
      if ((addrstart+i)>=oldaddr) {
        dat = *pb++;
        printf ("%02X ", dat);
      } else  {
        printf ("   ");
        len+=1;
      }
      j=1;
    } else if (unit == 2) {
      if ((addrstart+i)>=oldaddr) printf ("%04X ",*pw++);
      else {
        printf ("     ");
        len+=2;
      }
      j=2;
    } else {
      if ((addrstart+i)>=oldaddr) {
        if (flash) {
          fd[1] = B_SPI_CTL_RUN + B_SPI_CTL_CS_ASSERT;
          while (fd[1] & B_SPI_STS_BUSY);
          dw = fd[2];
        } else {
          dw = *pd;
        }
        pd++;
        printf ("%08X ", dw);
      } else {
        printf ("         ");
        len+=4;
      }
      j=4;
    }
    i+=j;
    addr+=j;
    if (!(i&0xf) && (i<len)) printf ("\n%08X: ",addrstart+i);
  }
  if (flash) {
    fd[1] = 0;
  }
  printf ("\n");

}

