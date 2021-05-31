#include "shell.h"
#include "flash.h"

#define  MAX_LINE_LEN    64
#define  CMD_HASH(x,y)    (((x)<<8)| (y))
#define  DP_CMD_SIGNATURE 0x3A444D43
#define  DP_CMD_RUNNING   1
#define  DP_CMD_FINISH    0

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
  if (data == DP_CMD_SIGNATURE) { // CMD:
    ptr = (char *)(DPRAM_SHELL_CMD + 4);
    memcpy (gCmdLine, ptr, MAX_LINE_LEN - 4);
    result = strlen (gCmdLine);
    MMIO(DPRAM_SHELL_CMD) = DP_CMD_RUNNING;
    printf ("%s\n", gCmdLine);
    return result;
  } else {
    return result;
  }
}

static void  SetCommandDoneForDpRam (void)
{
  if (MMIO(DPRAM_SHELL_CMD) == DP_CMD_RUNNING) {
    MMIO(DPRAM_SHELL_CMD) = DP_CMD_FINISH;
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
          "RF    address [length]      ;Read  Flash\n"
          "DL                          ;Download from SPI\n"
          "RT                          ;Reset System\n"
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
    case (CMD_HASH('r' , 'f')): //"rf"
      ReadMem(addr, len, 0x10 + 4);
      break;
    case (CMD_HASH('d' , 'l')): //"wd"
      bootloader ();
      break;
    case (CMD_HASH('p' , 'f')): //"pf"
      ProgramFlash (addr, dat, len);
      break;
    case (CMD_HASH('d' , 'm')): //"tt"
      if (addr == 1) {
        vga_demo ();
      } else if (addr == 2) {
        raiden_demo ();
      }
      break;
    case (CMD_HASH('t' , 't')): //"tt"
      break;
    case (CMD_HASH('r' , 't')): //"wd"
      printf ("\n");
      MMIO(SYS_CTRL_RESET) = 0x80000000;
      while (1);
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
  SetCommandDoneForDpRam ();
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
  printf ("*     Micro Blue (2011-2021)    *\n");
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
  UINT16  CfgVal;

  // Set optimal timing for HyperMem
  //   CAS  x1   x2
  //    3    3   9  (e)
  //    4    5   13 (f)
  //    5    7   17 (0)
  //    6    9   21 (1)

  // Turn off VGA HPRAM access
  MMIO_B(VGA_CTLBASE) = 0x00;
  delay_ms (1);

  // Detect endian
  CfgVal = 0x8fef;
  if (MMIO_B(HYRAM_ID_0) != 0x83) {
    CfgVal = (CfgVal << 8) + (CfgVal >> 8);
  }

  // Set optimal timing for HyperMem
  // Use 2x latency    (bit 3 = 1, 1x is not supported by 66WVH16M8)
  // Use 3 clk latency (bit 7:4 = 14)
  MMIO_S(HYRAM_REG_0) = CfgVal;
  MMIO_S(HYRAM_REG_0 + HYRAM_DIE_2) = CfgVal;
  // Set WR latency to 9
  MMIO_B(HYRAM_ID_0)  = 9;
  // Clear any command
  MMIO(DPRAM_SHELL_CMD) = 0;

  // Turn on VGA HPRAM access
  // MMIO_B(VGA_CTLBASE) = VGA_CTL_EN_HPC | VGA_CTL_EN;
}

static
void SpritTest (void)
{
  DWORD     *Ptr;
  int        i, j;

  // Init big spirit
  for (j = 0; j < 8; j++) {
    // Fill sprit bitmap buffer
    Ptr = (DWORD *)(VGA_SPR_B_BASE + j * 0x100);
    for (i = 0; i < 0x100; i+=4) {
      *Ptr++ = 0x01010101 * (j+1);
    }
    Ptr = (DWORD *)(VGA_SPR_B_CTL_BASE + j * 8);
    Ptr[1] = 0x00100000 + 32 * j + 5;  // set position
    Ptr[0] = 0x80000000 + j;       // enable and ID
  }

  // Init small spirit
  for (j = 0; j < 16; j++) {
    // Fill sprit bitmap buffer
    Ptr = (DWORD *)(VGA_SPR_S_BASE + j * 0x20);
    for (i = 0; i < 0x20; i+=4) {
      *Ptr++ = 0xffffffff;// * ((j&3)+1) * 5;
    }
    Ptr = (DWORD *)(VGA_SPR_S_CTL_BASE + j * 8);
    Ptr[1] = 0x00400000 + 0x10000 * j + 10 * (j) + 5;  // set position
    Ptr[0] = 0x80000000 + j + 0x40000 * (2);   // enable and ID
  }

}

static
void VgaTest (void)
{
      DWORD     *Ptr;
      WORD      *PtrS;
      DWORD      i;
      WORD       j;
      BYTE       t;
      int        x, y;

      t = 25;

      Ptr = (DWORD *)(0xF0740000);
      for (i=0; i < 0xc0000; i+=4) {
        *Ptr++ = 0;
      }

      volatile UINT8 *Fb1 = (volatile UINT8 *)VGA_GFX_BASE;
      volatile UINT8 *Fb2 = (volatile UINT8 *)VGA_GFX_BASE + (t * 16 - 4) * 640;
      for (x=0; x < 640; x+=4) {
        *(volatile DWORD *)(Fb1+x) = 0x07070707;
        *(volatile DWORD *)(Fb2+x) = 0x07070707;
      }

      Fb2 = (volatile UINT8 *)VGA_GFX_BASE;
      for (y=0; y < t * 16 - 4; y++) {
          *(Fb2 +   0) = 0x09;
          *(Fb2 + 639) = 0x09;
          Fb2 += 0x280;
      }

      SpritTest ();

      MMIO_B(VGA_CTLBASE)   = VGA_CTL_EN_HPC | VGA_CTL_EN;

      // 0x00: 8  x  8  x  1
      // 0x01: 8  x  8  x  2
      // 0x06: 8  x  8  x  4
      // 0x0B: 8  x  8  x  8
      // 0x10: 8  x 16  x  1
      // 0x20: 16 x  8  x  1
      // 0x25: 16 x  8  x  2
      // 0x2A: 16 x  8  x  4
      if (0) {
        UINT8  wide, bpp, chh, val;
        val  = 0;
        wide = MMIO_B(VGA_CHARBASE + 0xFFC);
        chh  = MMIO_B(VGA_CHARBASE + 0xFFD);
        bpp  = MMIO_B(VGA_CHARBASE + 0xFFE);

        if (wide == 2) {
          val |= 0x20;
        }
        if (bpp == 2) {
          val |= 0x01;
        } else if (bpp == 4) {
          val |= 0x02;
        } else if (bpp == 8) {
          val |= 0x03;
        }
        if (bpp * wide <= 2) {
          ;
        } else if (bpp * wide <= 4) {
          val |= 0x4;
        } else if (bpp * wide <= 8) {
          val |= 0x8;
        } else if (bpp * wide <= 16) {
          val |= 0xc;
        }
        if (chh == 2 ) {
          val |= 0x10;
        }
        printf ("Mode: %d x %d x %d (%02X)\n", wide*8, chh*8, bpp, val);
        MMIO_B(VGA_CTLBASE+2) = val;
      }

      MMIO_B(VGA_CTLBASE+2) = 0x10;

      PtrS = (WORD *)(VGA_TXT_BASE);
      if (1) {
        j = 0;
        for (y=0; y<60; y++) {
          for (x=0; x<80; x+=1) {
            *PtrS++ = 0x0f00 + (BYTE)j;
            j++;
          }
        }
      }

      PtrS = (WORD *)(VGA_TXT_BASE + t * 160);
      for (j=0; j < 16; j++) {
        *((BYTE *)(PtrS +j) + 1) = j;
      }


      while (0) {
        for (j = 0; j < 4; j++) {
          // Fill spirit buffer
          Ptr = (DWORD *)(VGA_SPR_B_CTL_BASE + j * 8);
          if (j == 0) {
            Ptr[1] += 0x10000;
          } else if (j == 1) {
            Ptr[1] += 0x1;
          } else if (j == 2) {
            Ptr[1] += 0x10001;
          } else {
            Ptr[1] += 0x10002;  // pos
          }
          Ptr[1] &= 0x02FF03FF;
          delay_ms (1);
        }
      }
}

unsigned int *irq(unsigned int *regs, unsigned int irqs)
{
  //_picorv32_irq_timer  (80000000);
  printf ("%08x\n", irqs);
  return regs;
}

static
void irq_init (void)
{
  //_picorv32_irq_enable (0x00000001);
  _picorv32_irq_mask   (0xfffffeff);
  //_picorv32_irq_timer  (80000000);
}

#if 0
#include "test.c"
#endif

void main (void)
{
  BoardInit ();

  //VgaTest2 ();
  //VgaText2 ();
  //Raiden ();
  //vga_demo ();

  //TileTest ();
  //SpriteTest ();

  Shell ();
}

