//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

//
// Copyright (c) 2021.  Micro Blue
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

const uint8_t ps2_set2[] = {
// 0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,'\t', '~',   0, //  0
   0,   0,   0,   0,   0, 'q', '1',   0,   0,   0, 'z', 's', 'a', 'w', '2',   0, // 10
   0, 'c', 'x', 'd', 'e', '4', '3',   0,   0, ' ', 'v', 'f', 't', 'r', '5',   0, // 20
 'n', 'n', 'b', 'h', 'g', 'y', '6',   0,   0,   0, 'm', 'j', 'u', '7', '8',   0, // 30
   0, '<', 'k', 'i', 'o', '0', '9',   0,   0, '>', '?', 'l', ':', 'p', '-',   0, // 40
   0,   0, '"',   0, '[', '+',   0,   0,   0,   0,'\r', ']',   0, '|',   0,   0, // 50
   0,   0,   0,   0,   0,   0,'\b',   0,   0,   0,   0,   0,   0,   0,   0,   0, // 60
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 70
};


#define  PS2_FLAG_RELEASE    BIT0
#define  PS2_FLAG_EXT        BIT1
#define  PS2_FLAG_MOD_SHIFT  BIT4
#define  PS2_FLAG_MOD_CTRL   BIT5
#define  PS2_FLAG_MOD_ALT    BIT6

static
char get_kbd_char (void)
{
  UINT32  c;
  static  BYTE ps2_flags = 0;
  BYTE    ps2_release;

  if ((MMIO (PS2_STS) & PS2_STS_AVL) == 0) {
    return 0;
  }

  c = MMIO (PS2_DAT);
	switch (c) {
	case 0xf0:
    ps2_flags |= PS2_FLAG_RELEASE;
    c = 0;
    break;

	case 0xe0:
    ps2_flags |= PS2_FLAG_EXT;
    c = 0;
    break;

	default:
	  if (ps2_flags & PS2_FLAG_EXT)
	    c |= 0xe000;

    ps2_release = ps2_flags & PS2_FLAG_RELEASE;
	  if (c == 0x11 || c == 0xe011) ps2_flags = !ps2_release ? (ps2_flags | PS2_FLAG_MOD_ALT) : (ps2_flags & ~PS2_FLAG_MOD_ALT);
	  if (c == 0x12 || c == 0x59)   ps2_flags = !ps2_release ? (ps2_flags | PS2_FLAG_MOD_SHIFT) : (ps2_flags & ~PS2_FLAG_MOD_SHIFT);
	  if (c == 0x14 || c == 0xe014) ps2_flags = !ps2_release ? (ps2_flags | PS2_FLAG_MOD_CTRL) : (ps2_flags & ~PS2_FLAG_MOD_CTRL);

	  // so we got scancodes, map to ASCII
	  c = (c > 0 && c < sizeof(ps2_set2)) ? ps2_set2[c] : 0;

	  if (ps2_flags | PS2_FLAG_MOD_SHIFT)
	    c = toupper(c);

	  if (ps2_release)
	    c = 0;

	  ps2_flags &= ~(PS2_FLAG_RELEASE | PS2_FLAG_EXT);
	}

  return (char)c;
}
