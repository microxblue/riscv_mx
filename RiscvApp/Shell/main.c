//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#include  "firmware.h"

#define     MAX_LINE_LEN    64

uint8_t     gCmdPos;
char        gCmdLineBuf[MAX_LINE_LEN];


static
uint8_t GetCommandLine (char *gCmdLine)
{
  char          ch;
  uint8_t       result;

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
void  ParseCommand (char *str)
{
  printf ("CMD: %s\n>", str);

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


void main (void)
{
  Shell ();
}
