// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "firmware.h"
#include "common.h"

BYTE VgaPosX = 0;
BYTE VgaPosY = 0;

DWORD tick_to_us (UINT64 Tick)
{
  return (DWORD)(Tick / CLK_FREQ_MHZ);
}

void delay (DWORD us)
{
  UINT64     End;
  UINT64_HL  Curr;

  End = GET_TSC(Curr) + CLK_FREQ_MHZ * us;
  while (Curr.Value < End) {
    (void)GET_TSC(Curr);
  }
}


void  putchar_vga (char ch)
{
  if (ch == '\r') {
    VgaPosX = 0;
    return;
  } else if (ch == '\n') {
    VgaPosY += 1;
    VgaPosX  = 0;
    ch = 0;
  } else if ((ch == '\x7f') || (ch == '\x08')) {
    if (VgaPosX > 0) {
      VgaPosX -= 1;
    }
    return;
  } else if (ch < ' ') {
    return;
  }

  if (VgaPosX >= 80) {
    VgaPosX  = 0;
    VgaPosY += 1;
  }

  if (VgaPosY >= 25) {
    memcpy ((BYTE *)VGA_TXT_BASE, (BYTE *)VGA_TXT_BASE + 80 * 2, 80 * 2 * 24);
    memset ((BYTE *)VGA_TXT_BASE + 80 * 2 * 24, 0, 80 * 2);
    VgaPosX  = 0;
    VgaPosY  = 24;
  }

  if (ch > 0) {
    MMIO_S (VGA_TXT_BASE + ((80 * VgaPosY + VgaPosX) << 1)) = 0x0700 + ch;
    VgaPosX += 1;
  }
}

