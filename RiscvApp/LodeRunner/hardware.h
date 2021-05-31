#ifndef __HARDWARE__
#define __HARDWARE__

#include "firmware.h"

#define videomode(x)
#undef  vera_sprites_enable
#define vera_sprites_enable(x)     sprites_enable (0xff, x)

int  tprintf(const char *fmt, ...);
void sleep_us(int us);
void sprites_enable (unsigned char idx, unsigned char enable);
unsigned char has_key();
unsigned char get_key();
void waitvsync();
void switch_bank (unsigned int page);
unsigned int get_hbc_addr (unsigned int offset);

#define  hasch           haschar
#define  getch           getchar
#define  rand(x)         GET_RAND(x)
#define  SWITCH_BANK(x)  switch_bank(x)

#define  RES_HIGH   1

#endif
