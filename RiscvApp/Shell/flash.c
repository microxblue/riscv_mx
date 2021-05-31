//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#include "flash.h"

volatile DWORD *mSpiBase = (volatile DWORD *)(SPI_FLASH_BASE + SPI_CMD_OFF);

#define  WAIT_FOR_DONE   while (mSpiBase[1] & B_SPI_STS_BUSY)
#define  DEASSERT_CS     mSpiBase[1] = 0
#define  SPI_RD_DATA     mSpiBase[2]

static
VOID  SendCommand (DWORD Cmd, DWORD Arg)
{
  mSpiBase[0] = Cmd;
  mSpiBase[1] = B_SPI_CTL_RUN + Arg;
  WAIT_FOR_DONE;
}

static
BYTE  ReadFlagStatus (VOID)
{
  SendCommand (0x70000000, 2);
  return mSpiBase[2] & 0xFF;
}

#define  BLK_SIZE   SIZE_64KB
INT32 EraseFlash (UINT32 FlashAddr, UINT32 Length)
{
  UINT32  Idx;

  Length    = ALIGN_UP (Length, BLK_SIZE) & (FLASH_SIZE - 1);
  FlashAddr = ALIGN_DOWN (FlashAddr, BLK_SIZE) & (FLASH_SIZE - 1);
  if (FlashAddr + Length > FLASH_SIZE) {
    return -1;
  }

  Idx = 0;
  while (Length > 0) {
    // Enable write
    SendCommand (0x06000000 + FlashAddr, 1);
    if (BLK_SIZE == SIZE_64KB) {
      // Erase 64KB
      SendCommand (0xd8000000 + FlashAddr, 0);
    } else {
      // Erase 4KB
      SendCommand (0x20000000 + FlashAddr, 0);
    }

    // Wait for done
    printf ("Erasing ... %06X\n", FlashAddr);

    while (!(ReadFlagStatus () & BIT7));
    Length    -= BLK_SIZE;
    FlashAddr += BLK_SIZE;
    Idx += 1;
  }
  return 0;
}

INT32 WriteFlash (UINT32 FlashAddr, UINT32 MemAddr, UINT32 Length)
{
  UINT32 Loop;
  UINT32 Wlen;
  UINT32 Idx;

  Length    &= (FLASH_SIZE - 1);
  FlashAddr &= (FLASH_SIZE - 1);
  MemAddr   &= 0xFFFFFFFC;

  if (FlashAddr + Length > FLASH_SIZE) {
    return -1;
  }

  Idx  = 0;
  Wlen = FLASH_WR_SIZE;
  while (Length > 0) {
    if (Length < Wlen) {
      Wlen = Length;
    }
    // Enable write
    SendCommand (0x06000000 + FlashAddr, 1);
    // Write 256 bytes
    SendCommand (0x02000000 + FlashAddr, B_SPI_CTL_CS_ASSERT);
    for (Loop = 0; Loop < ALIGN_UP (Wlen, sizeof(UINT32)); Loop += sizeof(UINT32)) {
      SendCommand (MMIO(MemAddr + Loop), B_SPI_CTL_CS_ASSERT);
    }
    DEASSERT_CS;
    if ((Idx & 0xFF) == 0) {
      printf ("Writing ... %06X\n", FlashAddr);
    }
    // Wait for done
    while (!(ReadFlagStatus () & BIT7));
    Length    -= Wlen;
    FlashAddr += Wlen;
    MemAddr   += Wlen;
    Idx += 1;
  }
  return 0;
}

INT32 ReadFlash (UINT32 FlashAddr, UINT32 MemAddr, UINT32 Length)
{
  UINT32 Rlen;

  Length    &= (FLASH_SIZE - 1);
  FlashAddr &= (FLASH_SIZE - 1);
  if (FlashAddr + Length > FLASH_SIZE) {
    return -1;
  }
  // Fast read + Address
  SendCommand (0x0B000000 + FlashAddr, B_SPI_CTL_CS_ASSERT);
  // One dummy
  SendCommand (0, B_SPI_CTL_CS_ASSERT + 1);
  while (Length > 0) {
    // Read 32 bits
    SendCommand (0, B_SPI_CTL_CS_ASSERT);
    if (Length > sizeof(UINT32)) {
      Rlen = sizeof(UINT32);
    } else {
      Rlen = Length;
    }
    memcpy ((void *)MemAddr, (void *)(UINT32)&SPI_RD_DATA, Rlen);
    Length  -= Rlen;
    MemAddr += Rlen;
  }
  DEASSERT_CS;

  return 0;
}

INT32 ProgramFlash (UINT32 FlashAddr, UINT32 MemAddr, UINT32 Length)
{
  INT32  Ret;

  Ret = EraseFlash (FlashAddr, Length);
  if (Ret != 0) {
    return Ret;
  }

  Ret = WriteFlash (FlashAddr, MemAddr, Length);
  if (Ret != 0) {
    printf ("Writing failed !");
    return Ret;
  }

  printf ("Done !\n");
  return 0;
}
