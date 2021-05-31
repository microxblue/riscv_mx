//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

static void
VgaTest2 (void)
{
  int   n;
  int   x, y, t;
  int   s, w, h;
  WORD *ptr;

  s = MMIO_B(VGA_INFO);
  w = (320 - 16) / s;
  h = (240 - 16) / s;
  if (w > 0x20)  w = 0x20;
  if (h > 0x10)  h = 0x10;

  MMIO(VGA_GFX_BUF_BASE) = VGA_GFX_BASE;
  MMIO(VGA_SPR_BUF_BASE) = VGA_SPR_BASE;

  if (1)
    memset ((void *)VGA_GFX_BASE, 0x0, 320*240*2);

  ptr = (WORD *)VGA_SPR_BASE;
  if (1) {
    for (n = 0; n < 0x10000; n+=2) {
      *ptr++ = (0x1111 * ((n >> 9)+1)) & 0x7fff;
    }
    ptr = (WORD *)VGA_SPR_BASE;
    ptr[0x00] = 0;
    ptr[0xFF] = 0;
  }

  x = 10;
  y = 10;
  h = 1;
  w = 4;
  if (1) {
    for (n = 0; n < s; n++) {
      SET_VGA_SPR_POS_Y   (n, y + h * n + ((w * n) / 300) * 16);  // pos_y
      SET_VGA_SPR_POS_X   (n, x + (w * n) % 300 );  // pos_x
      SET_VGA_SPR_BUF_IDX (n, n);
      t = VGA_SPR_ATTR_EN;
      SET_VGA_SPR_ATTR    (n, t);
    }
  } else {
    for (n = 0; n < s>>2; n++) {
      SetSpritePos (n, x + 0x10 * n, y + 0x08 * n, 0x80 + ((n&3)<<4));
    }
  }
}

static void
VgaText2 (void)
{
  int    i;
  WORD  *ptr;

  if (0) {
    // clear text buffer
    ptr = (WORD *)VGA_TXT_BASE;
    for (i = 0; i < 0x2000; i += 2) {
      *ptr++ = 0x4E00 + 'G';
    }
    ptr = (WORD *)VGA_TXT_BASE;
    ptr[0]        = 0x3242;
    ptr[39]       = '9' + 0x1200;
    ptr[40]       = '6' + 0x2300;
    ptr[40*19]    = 'R' + 0x3400;
    ptr[40*19+39] = 'K' + 0x4500;
    MMIO_B(VGA_TXT_START) = 0; //12 * 18;
  } else {

    // Enable VGA
    MMIO_B(VGA_CTLBASE) = VGA_CTL_EN_HPC | VGA_CTL_EN | VGA_CTL_TILE_EN;

    // clear text buffer
    ptr = (WORD *)VGA_TILE_BASE;
    for (i = 0; i < 0x8000; i += 2) {
      *ptr++ = 0;
    }

    ptr = (WORD *)VGA_TXT_BASE;
    for (i = 0; i < 30*40*2; i ++) {
      *ptr++ = i;
    }

    ptr = (WORD *)VGA_TILE_BASE;
    for (i = 0; i < 0x25800; i += 2) {
      *ptr++ = (i>>7) * 0x07;
    }

    MMIO_B(VGA_CTLBASE) = MMIO_B(VGA_CTLBASE) | VGA_CTL_TILE_EN;
  }

  MMIO_B(VGA_TXT_START) = 16 * 12;

}

static void TileTest (void)
{
  int    i;
  WORD  *ptr;

  // Set full text mode
  MMIO_B(VGA_TXT_START) = 0;

  // Init CHAR buffer
  ptr = (WORD *)VGA_TXT_BASE;
  for (i = 0; i < 30*40*2; i ++) {
    *ptr++ = i;
  }

  // Init tile plyph buffer
  ptr = (WORD *)VGA_TILE_BASE;
  for (i = 0; i < 0x25800; i += 2) {
    //*ptr++ = (i>>7) * 0x07;
    *ptr++ = 0;
  }

  // Enable VGA
  MMIO_B(VGA_CTLBASE)   = VGA_CTL_EN_HPC | VGA_CTL_EN | VGA_CTL_TILE_EN;
}


typedef struct {
  unsigned char en;
  unsigned char id;
  unsigned char style;
  unsigned char img;
  unsigned char subidx;
  unsigned char mir;
  short         x;
  short         y;
} SHOOT_STS;


static void
SetShoot (SHOOT_STS *ps_sts)
{
  unsigned char s;
  SET_VGA_SPR_POS_Y   (ps_sts->id, ps_sts->y);
  SET_VGA_SPR_POS_X   (ps_sts->id, ps_sts->x);
  SET_VGA_SPR_BUF_IDX (ps_sts->id, ps_sts->img);
  switch (ps_sts->style) {
  case 3:
    s = VGA_SPR_ATTR_SIZE_64;
    break;
  case 2:
    s = VGA_SPR_ATTR_SIZE_32;
    break;
  case 1:
    s = VGA_SPR_ATTR_SIZE_16;
    break;
  default:
    s = VGA_SPR_ATTR_SIZE_8;
    break;
  }
  s |= (VGA_SPR_ATTR_EN | VGA_SPR_ATTR_FRONT);
  if (ps_sts->mir)
    s |= VGA_SPR_ATTR_MIR;
  if (ps_sts->subidx)
    s = (s & ~VGA_SPR_ATTR_SUB_IDX) | ((ps_sts->subidx & 3) << 2);
  SET_VGA_SPR_ATTR    (ps_sts->id, s);
}

static void SpriteTest (void)
{
  SHOOT_STS    shoot_sts;
  SHOOT_STS   *ps_sts;
  int          idx;

  for (idx = 0; idx < 16; idx++) {
    SET_VGA_SPR_ATTR (idx, 0);
  }

  // 64
  ps_sts = &shoot_sts;
  ps_sts->img    = 0;
  ps_sts->style  = 3;
  ps_sts->en     = 1;
  ps_sts->id     = 0;
  ps_sts->x      = 64;
  ps_sts->y      = 64;
  ps_sts->mir    = 0;
  ps_sts->subidx = 0;
  SetShoot (ps_sts);
  ps_sts->id     = 1;
  ps_sts->x      = 64;
  ps_sts->y      = 64 + 64;
  ps_sts->mir    = 1;
  SetShoot (ps_sts);

  // 32
  ps_sts->img    = 0;
  ps_sts->style  = 2;
  ps_sts->id     = 2;
  ps_sts->x      = 64*3;
  ps_sts->y      = 64;
  ps_sts->mir    = 0;
  ps_sts->subidx = 0;
  SetShoot (ps_sts);
  ps_sts->id     = 3;
  ps_sts->x      = 64*3;
  ps_sts->y      = 64 + 32;
  ps_sts->mir    = 1;
  SetShoot (ps_sts);

  // 16
  ps_sts->img    = 8;
  ps_sts->style  = 1;
  ps_sts->id     = 4;
  ps_sts->x      = 64*4;
  ps_sts->y      = 64;
  ps_sts->mir    = 0;
  ps_sts->subidx = 0;
  SetShoot (ps_sts);
  ps_sts->id     = 5;
  ps_sts->x      = 64*4;
  ps_sts->y      = 64 + 16;
  ps_sts->mir    = 1;
  SetShoot (ps_sts);

  // 8
  ps_sts->img    = 0;
  ps_sts->style  = 0;
  ps_sts->id     = 6;
  ps_sts->x      = 16;
  ps_sts->y      = 0;
  ps_sts->mir    = 0;
  ps_sts->subidx = 0;
  SetShoot (ps_sts);
  ps_sts->id     = 7;
  ps_sts->x      = 16;
  ps_sts->y      = 0 + 8;
  ps_sts->mir    = 1;
  SetShoot (ps_sts);
}
