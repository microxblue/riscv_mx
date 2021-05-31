//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#include  "firmware.h"
#include  "memory.h"

#define  MAX_LINE_LEN    64
#define  CMD_HASH(x,y)    (((x)<<8)| (y))

#define  SIGNATURE_16(A, B)   ((A) | (B << 8))
#define  CMD_SIG(A, B, C, D)  (SIGNATURE_16 (A, B) | (SIGNATURE_16 (C, D) << 16))


uint8_t     gCmdPos;
char        gCmdLineBuf[MAX_LINE_LEN];
BYTE        gParamNum;
DWORD       gParams[4];

static uint8_t  GetCommandLineFromDpRam (char *gCmdLine)
{
  uint8_t       result;
  uint32_t      data;
  char         *ptr;

  result = 0;
  data = MMIO(DPRAM_SHELL_CMD);
  if (data == 0x3A444D43) { // CMD:
    ptr = (char *)(DPRAM_SHELL_CMD + 4);
    memcpy (gCmdLine, ptr, MAX_LINE_LEN - 4);
    result = strlen (gCmdLine);
    MMIO(DPRAM_SHELL_CMD) = 0;
    printf ("%s\n", gCmdLine);
    return result;
  } else {
    return result;
  }
}

static
uint8_t GetCommandLine (char *gCmdLine)
{
  char          ch;
  uint8_t       result;

  result = GetCommandLineFromDpRam (gCmdLine);
  if (result > 0) {
    return result;
  }

  if (!haschar())
    return 0;

  result = 0;
  ch = (int)getchar();

  switch (ch) {
  	case 0x09:    /* Tab */
  	  if (!gCmdPos) {
  	  	printf("%s\n", gCmdLine);
  	  	result = findchar(gCmdLine, 0) - gCmdLine;
  	  }
  	  break;
    case 0x08:    /* Backspace */
    case 0x7F:    /* Delete */
      if (gCmdPos > 0) {
        gCmdPos -= 1;
        putchar(0x08);  /* backspace */
        putchar(' ');
        putchar(0x08);  /* backspace */
      }
      break;
    case 0x0D:
    case 0x0A:
      putchar(0x0D);  /* CR */
      putchar(0x0A);  /* CR */
      gCmdLine[gCmdPos] = '\0';
      result = gCmdPos + 1;
      gCmdPos = 0;
      break;
    default:
      if ((gCmdPos+1) < MAX_LINE_LEN) {
        /* only printable characters */
        if (ch > 0x1f) {
          gCmdLine[gCmdPos++] = (char)ch;
          putchar((char)ch);
        }
      }
      break;
  }
  return result;
}

static
void PrintHelper(void)
{
  printf ("RB    address [length]      ;Read  Memory BYTE\n"
          "WB    address  value        ;Write Memory BYTE\n"
          "RW    address [length]      ;Read  Memory WORD\n"
          "WW    address  value        ;Write Memory WORD\n"
          "RD    address [length]      ;Read  Memory DWORD\n"
          "WD    address  value        ;Write Memory DWORD\n"
          );
}

static
void  ParseCommand (char *str)
{
  DWORD          addr;
  DWORD          len;
  DWORD          dat;
  DWORD          cmd;
  BYTE           cmdstr[2];

  str = skipchar (str, ' ');

  if (*str == 0) {
    goto Quit;
  }

  if (*str == '?') {
    PrintHelper ();
    goto Quit;
  }

  if (str[1] && ((str[2] == ' ') || !str[2])) {
    cmdstr[0] = tolower(str[0]);
    cmdstr[1] = tolower(str[1]);
    cmd = CMD_HASH(cmdstr[0], cmdstr[1]);
  } else {
    cmd = CMD_SIG (str[0], str[1], str[2], str[3]);
  }

  gParamNum = 1;
  do {
    str = findchar (str, ' ');
    str = skipchar (str, ' ');
    if (*str) {
      gParams[gParamNum] = xtoi(str);
      gParamNum++;
    }
  } while ((*str) && (gParamNum < 4));

  for (dat = gParamNum; dat < 4; dat++)
      gParams[dat] = 0;

  gParamNum--;

  addr = gParams[1];
  len  = gParams[2];
  dat  = gParams[3];

  if (gParamNum < 2) {
     len = 16;
  }

  switch(cmd) {
    case (CMD_HASH('r' , 'b')): //"rb"
      ReadMem(addr, len, 1);
      break;
    case (CMD_HASH('r' , 'w')): //"rw"
      ReadMem(addr, len, 2);
      break;
    case (CMD_HASH('r' , 'd')): //"rd"
      ReadMem(addr, len, 4);
      break;
    case (CMD_HASH('w' , 'b')): //"wb"
      WriteMem(addr, len, 1);
      break;
    case (CMD_HASH('w' , 'w')): //"ww"
      WriteMem(addr, len, 2);
      break;
    case (CMD_HASH('w' , 'd')): //"wd"
      WriteMem(addr, len, 4);
      break;
    default:
      cmd = 0xFFFFFFFF;
      break;
  }

  if (cmd == 0xFFFFFFFF) {
    // extended command
    printf ("Unknown command!\n");
    goto Quit;
  }

Quit:
  printf ("\n>");
  return;
}

static
void PrintShellBanner (void)
{
	const char *StarLine = "*********************************";

  printf ("\n\n");
  printf ("%s\n", StarLine);
  printf ("*       Welcome to Risc-V       *\n");
  printf ("*    Micro Blue   (2020-2021)   *\n");
  printf ("*        Mini Shell 1.1         *\n");
  printf ("%s\n\n>", StarLine);
}

static
void Shell(void)
{
  PrintShellBanner ();

  gCmdPos = 0;
  while(1) {
    if (GetCommandLine(gCmdLineBuf)) {
      ParseCommand(gCmdLineBuf);
    }
  }
}

static
void BoardInit (void)
{
  // Set optimal timing for HyperMem
  // Use 2x latency    (bit 3 = 1, 1x is not supported by 66WVH16M8)
  // Use 3 clk latency (bit 7:4 = 15)
  MMIO_S(HYRAM_REG_0) = 0x8fef;
  MMIO_S(HYRAM_REG_0 + HYRAM_DIE_2) = 0x8fef;
  // Set WR latency to 9
  MMIO_B(HYRAM_ID_0)  = 0x09;
}

void main (void)
{
  BoardInit ();
  Shell ();
}
