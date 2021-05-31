//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#include  "shell.h"

#if  VGA_DEMO
typedef struct {
  short   x;
  short   y;
  short  ox;
  short  oy;
  char    p;
  char   op;
} SPR_STS;

typedef struct {
  unsigned char en;
  unsigned char id;
  unsigned char style;
  unsigned char img;
  short         x;
  short         y;
} SHOOT_STS;

SHOOT_STS  shoot_sts[100];
DWORD      gLoop;

static
void SetSprite32Pos (short g, short x, short y, char pos)
{
  BYTE  i, n, t;

  n = 124 + g;
  i = VGA_SPR_ATTR_EN | VGA_SPR_ATTR_SIZE_32 | VGA_SPR_ATTR_FRONT;
  SET_VGA_SPR_POS_Y   (n, y + 0x10 * (i>>1));  // pos_y
  SET_VGA_SPR_POS_X   (n, x + 0x10 * (i&1));   // pos_x
  if (pos == 1) {
      t = 1;
  } else if (pos == 2) {
      t = 1;
      i |= VGA_SPR_ATTR_MIR;
  } else {
      t = 2;
  }
  SET_VGA_SPR_ATTR    (n, i);
  SET_VGA_SPR_BUF_IDX (n, g * 3 + t);
}

static void
SetShoot (SHOOT_STS *ps_sts)
{
  int  s;

  SET_VGA_SPR_POS_Y   (ps_sts->id, ps_sts->y);
  SET_VGA_SPR_POS_X   (ps_sts->id, ps_sts->x);
  s = VGA_SPR_ATTR_EN | VGA_SPR_ATTR_SIZE_8 | VGA_SPR_ATTR_FRONT;
  if (ps_sts->style == 2) {
    s |= VGA_SPR_ATTR_SIZE_64;
  }
  SET_VGA_SPR_BUF_IDX (ps_sts->id, ps_sts->img);
  SET_VGA_SPR_ATTR    (ps_sts->id, s);
}

static void AddShoot (int x, int y, char style)
{
  unsigned int  i;
  unsigned int  c;
  SHOOT_STS    *ps_sts;

  c = 0;
  for (i =0; i < sizeof(shoot_sts)/sizeof(SHOOT_STS); i++) {
    ps_sts = &shoot_sts[i];
    if (ps_sts->en == 0) {
      ps_sts->img   = 0;
      ps_sts->style = style;
      ps_sts->en = (style == 0) ? 2 : 1;
      ps_sts->id = 16 + i;
      if ((style == 0) || (style == 2)) {
        ps_sts->x  = x;
      } else if (style == 1) {
        if (c==0) {
          ps_sts->x  = x - 3;
        } else {
          ps_sts->x  = x + 3;
        }
      }
      ps_sts->y  = y;
      SetShoot (ps_sts);
      c++;
    }
    if (((style == 0) || (style == 2)) && (c==1)) {
      break;
    }
    if ((style == 1) && (c==2)) {
      break;
    }
  }
}

static void UpdateShoot (void)
{
  unsigned int  i;
  SHOOT_STS    *ps_sts;

  for (i =0; i < sizeof(shoot_sts)/sizeof(SHOOT_STS); i++) {
    ps_sts = &shoot_sts[i];
    if (ps_sts->en) {
      if (ps_sts->style < 2) {
        ps_sts->y -= ps_sts->en;
      } else {
        if ((gLoop & 0x7) == 0) {
          ps_sts->y += 1;
          if (ps_sts->y > 240) {
            ps_sts->y = 0;
          }
          if (ps_sts->img >= 7) {
            ps_sts->y   = 0;
            ps_sts->img = 0;
          } else {
            ps_sts->img++;
          }
        }
      }
      if (ps_sts->y <= 0) {
        ps_sts->en = 0;
        SET_VGA_SPR_ATTR (ps_sts->id, 0);
      } else {
        SetShoot (ps_sts);
      }
    }
  }
}

void raiden_demo (void)
{
  WORD       Dat;
  unsigned int base;
  int        i, j, n, l, s, t;
  SPR_STS    spr[2];

  // disable all sprite
  s = MMIO_B(VGA_INFO);
  for (n = 0; n < s; n++) {
    SET_VGA_SPR_ATTR    (n, 0);
  }

  memset (shoot_sts, 0, sizeof(shoot_sts));

  // enable flighters
  // Set group
  memset (spr, 0, sizeof(spr));
  i = 160;
  j = 200;
  spr[0].x = i+16;
  spr[1].x = i-16;
  spr[0].y = j;
  spr[1].y = j;
  for  (l = 0; l < 2; l++) {
    SetSprite32Pos (l, spr[l].x, spr[l].y, spr[l].p);
  }

  base = 0xF0400000 + 320 * 2 * (2984 - 240);
  MMIO_B(VGA_TXT_START) = 0xFF;
  MMIO(VGA_BUF_BASE(0)) = base;


  // Enable VGA
  MMIO_B(VGA_CTLBASE) = VGA_CTL_EN_HPC | VGA_CTL_EN;

  j = 0;
  t = 0;
  gLoop = 0;
  while (base > 0xF0400000) {

    j++;

    MMIO(VGA_BUF_BASE(0)) = base;

    t++;
    if (t > 8) {
        base = base - 320 * 2;
        t = 0;
    }

    UpdateShoot ();

    Dat = MMIO_S(SYS_CTRL_JOYSTICK);
    for (l = 0; l < 2; l++) {

      if (((j&0xf) == 0) && (l == 0)) {
        if (!(Dat & 0x0800)) { // 1
          AddShoot (spr[0].x + 11, spr[0].y, 0);
        }
        if (!(Dat & 0x0200)) { // 2
          AddShoot (spr[0].x + 11, spr[0].y, 2);
        }
        if (!(Dat & 0x0400)) { // 1
          AddShoot (spr[1].x + 11, spr[1].y, 0);
        }
        if (!(Dat & 0x0100)) { // 2
          AddShoot (spr[1].x + 11, spr[1].y, 1);
        }
      }

      spr[l].p = 0;
      if (!(Dat & 0x1000)) { // up
        spr[l].y -= 1;
        spr[l].p  = 0;
      }

      if (!(Dat & 0x4000)) { // down
        spr[l].y += 1;
        spr[l].p = 0;
      }

      if (!(Dat & 0x8000)) { // left
        spr[l].x -= 1;
        spr[l].p = 1;
      }

      if (!(Dat & 0x2000)) { // right
        spr[l].x += 1;
        spr[l].p = 2;
      }

      if (spr[l].x < 0) spr[l].x = 0;
      if (spr[l].x >= 320-28) spr[l].x = 320-28;

      if (spr[l].y < 0) spr[l].y = 0;
      if (spr[l].y >= 240-32) spr[l].y = 240-32;

      if ((spr[l].ox != spr[l].x) || (spr[l].oy != spr[l].y) || (spr[l].op != spr[l].p)) {
        SetSprite32Pos (l, spr[l].x, spr[l].y, spr[l].p);
        spr[l].ox = spr[l].x;
        spr[l].oy = spr[l].y;
        spr[l].op = spr[l].p;
      }

      Dat = Dat << 8;
    }

    if (haschar()) {
      getchar();
      break;
    }

    delay (8000);
    gLoop ++;
  }

}


static
void SetSpritePos (short g, short x, short y, char pos)
{
  BYTE  i, n, s, t;

  for  (i = 0; i < 4; i++) {
    n = 4*g + i;
    s = g*12;
    SET_VGA_SPR_POS_Y   (n, y + 0x10 * (i>>1));  // pos_y
    SET_VGA_SPR_POS_X   (n, x + 0x10 * (i&1));   // pos_x
    if (pos == 1) {
      SET_VGA_SPR_ATTR (n, VGA_SPR_ATTR_SIZE_16 | VGA_SPR_ATTR_EN);
      t = i+4+s;
    } else if (pos == 2) {
      SET_VGA_SPR_ATTR (n, VGA_SPR_ATTR_MIR | VGA_SPR_ATTR_SIZE_16 | VGA_SPR_ATTR_EN);
      if      (i == 0) t = 4+1+s;
      else if (i == 1) t = 4+0+s;
      else if (i == 2) t = 4+3+s;
      else             t = 4+2+s;
    } else {
      SET_VGA_SPR_ATTR (n, VGA_SPR_ATTR_SIZE_16 | VGA_SPR_ATTR_EN);
      t = i+8+s;
    }
    SET_VGA_SPR_BUF_IDX(n, t);
  }
}


static BYTE
SwapSprite (BYTE m, BYTE n)
{
  DWORD  oam;
  oam = MMIO(VGA_SPR_OAM(m));
  MMIO(VGA_SPR_OAM(m)) = MMIO(VGA_SPR_OAM(n));
  MMIO(VGA_SPR_OAM(n)) = oam;
  return n;
}


static void
VgaText (void)
{
  int    i;
  WORD  *ptr;

  // clear text buffer
  ptr = (WORD *)VGA_TXT_BASE;
  for (i = 0; i < 0x2000; i += 2) {
    *ptr++ = 0x4E00 + ' ';
  }
  ptr = (WORD *)VGA_TXT_BASE;

  ptr[0]        = 0x3242;
  ptr[39]       = '9' + 0x1200;
  ptr[40]       = '6' + 0x2300;
  ptr[40*19]    = 'R' + 0x3400;
  ptr[40*19+39] = 'K' + 0x4500;

  MMIO_B(VGA_TXT_START) = 12 * 18;

  for (i = 0; i < 40 * 2; i++) {
    ptr[40 * 18 + i] = 0x0720+ (i & 0xff);
  }
}


static
void VgaDemo320x240 (void)
{
  WORD       Dat;
  DWORD      i, j;
  SPR_STS    spr[2];
  int        s, t, l;

  // Enable VGA
  MMIO_B(VGA_CTLBASE) = VGA_CTL_EN_HPC | VGA_CTL_EN;

  VgaText ();

  // Disable all sprite
  j = MMIO_B(VGA_INFO);
  for  (i = 0; i < j; i++) {
    SET_VGA_SPR_ATTR (i, 0);
  }

  // Enable sprite bitmap
  for  (i = 0; i < 8; i++) {
    SET_VGA_SPR_ATTR    (i, VGA_SPR_ATTR_EN | VGA_SPR_ATTR_SIZE_16);
    j = (i&3) + (i>>2)*24;
    SET_VGA_SPR_BUF_IDX (i, j);
  }

  for  (i = 8; i < 16; i++) {
    SET_VGA_SPR_POS_X   (i, 10 + (i-8) * 35);
    SET_VGA_SPR_POS_Y   (i, 60 + (i-8) * 10);
    SET_VGA_SPR_BUF_IDX (i, 0);
    SET_VGA_SPR_ATTR    (i, VGA_SPR_ATTR_EN | VGA_SPR_ATTR_SIZE_32 | VGA_SPR_ATTR_FRONT);
  }

  for  (i = 16; i < 18; i++) {
    SET_VGA_SPR_POS_X   (i, 10 + (i-16) * 100);
    SET_VGA_SPR_POS_Y   (i, 100 + (i-16) * 40);
    SET_VGA_SPR_BUF_IDX (i, 0);
    SET_VGA_SPR_ATTR    (i, VGA_SPR_ATTR_EN | VGA_SPR_ATTR_SIZE_64 | VGA_SPR_ATTR_FRONT);
  }

  i = 32;
  SET_VGA_SPR_POS_X   (i,  10);
  SET_VGA_SPR_POS_Y   (i,  10);
  SET_VGA_SPR_BUF_IDX (i,  0);
  SET_VGA_SPR_ATTR    (i,  VGA_SPR_ATTR_EN );

  // Set group
  memset (spr, 0, sizeof(spr));
  i = 160;
  j = 20;
  spr[0].x = i+16;
  spr[1].x = i-16;
  spr[0].y = j;
  spr[1].y = j;
  SetSpritePos (0, spr[0].x, spr[0].y, 0);
  SetSpritePos (1, spr[0].x, spr[1].y, 0);

  MMIO_B(VGA_CTLBASE)   = VGA_CTL_EN_HPC | VGA_CTL_EN;

  t = 0;
  s = MMIO(VGA_BUF_BASE(0));
  while (1) {
    MMIO(VGA_BUF_BASE(0)) = s + (240-(t>>3)) * 320 * 2;

    t++;
    if ((t>>3) >= 240)  t = 0;

    Dat = MMIO_S(SYS_CTRL_JOYSTICK);
    for (l = 0; l < 2; l++) {



      spr[l].p = 0;
      if (!(Dat & 0x1000)) { // up
        spr[l].y -= 1;
        spr[l].p  = 0;
      }

      if (!(Dat & 0x4000)) { // down
        spr[l].y += 1;
        spr[l].p = 0;
      }

      if (!(Dat & 0x8000)) { // left
        spr[l].x -= 1;
        spr[l].p = 1;
      }

      if (!(Dat & 0x2000)) { // right
        spr[l].x += 1;
        spr[l].p = 2;
      }

      if (spr[l].x < 0) spr[l].x = 0;
      if (spr[l].x >= 320-28) spr[l].x = 320-28;

      if (spr[l].y < 0) spr[l].y = 0;
      if (spr[l].y >= 240-32) spr[l].y = 240-32;

      if ((spr[l].ox != spr[l].x) || (spr[l].oy != spr[l].y) || (spr[l].op != spr[l].p)) {
        SetSpritePos (l, spr[l].x, spr[l].y, spr[l].p);
        spr[l].ox = spr[l].x;
        spr[l].oy = spr[l].y;
        spr[l].op = spr[l].p;
      }

      Dat = Dat << 8;
    }

    if (haschar()) {
      getchar();
      break;
    }

    delay (8000);
  }

}

void vga_demo (void)
{
  VgaDemo320x240 ();
}

#else

void vga_demo (void)
{
  printf ("VGA demo not enabled!\n");
}

#endif