//#include <stdio.h>
//#include <stdlib.h>
//#include <conio.h>
//#include <unistd.h>
#include "hardware.h"
#include "loderunner.h"
#include "runner.h"
#include "guard.h"
#include "levels.h"
#include "sound.h"
#include "key.h"

static uint8_t processTick[SPEED_OPTIONS][SPEED_TICKS] = {
	// SLOW - 3 out of 6 ticks
	{ 1, 0, 1, 0, 1, 0 },
	// MEDIUM - 4 out of 6 ticks
	{ 1, 1, 0, 1, 1, 0 },
	// FAST - 6 out of 6 ticks
	{ 1, 1, 1, 1, 1, 1 }
};

// Main loop while playing the game
void playGame()
{
	static uint8_t tick = 0;

	if (processTick[currentGame.speed][tick] ) {
		if (currentLevel.goldComplete && runner.y == 0 && runner.yOffset == 0) {
			currentGame.gameState = GAME_FINISH;
			return;
		}

		if (!isDigging()) {
			moveRunner();
		} else {
			processDigHole();
		}
		if (currentGame.gameState != GAME_RUNNER_DEAD) {
			moveGuard();
		}

		processGuardShake();
	}

	tick++;
	if (tick == SPEED_TICKS) {
		tick = 0;
	}

	processFillHole();
	playSoundFx();

}

static uint8_t scoreCount;


void mainTick()
{
	switch (currentGame.gameState) {
	case GAME_SPLASH:
		splash();
		break;
	case GAME_RUNNING:
		playGame();
		break;
	case GAME_NEW_LEVEL:
		if (loadLevel (currentGame.world, currentGame.level)) {
			displayLevel (currentGame.level - 1);
			// Enable sprites
			sprites_enable (0xff, 1);
			currentGame.gameState = GAME_RUNNING;
		} else {
			worldComplete();
			currentGame.level = 1;
			currentGame.gameState = GAME_OVER;
		}
		break;
	case GAME_FINISH:
		// Disable sprites
		sprites_enable (0xff, 0);
		// Increase score for level completion
		scoreCount = 0;
		// Start the sound effect for updating the score
		playScoringFx();
		currentGame.gameState = GAME_FINISH_SCORE_COUNT;
		break;
	case GAME_FINISH_SCORE_COUNT:
		while (scoreCount <= SCORE_COUNT) {
			++scoreCount;
			displayScore (SCORE_INCREMENT);

			playSoundFx();

			waitvsync();
		}

		stopScoringFx();
		currentGame.lives++;
		currentGame.gameState = GAME_NEXT_LEVEL;
		break;
	case GAME_NEXT_LEVEL:
		currentGame.level++;
		currentGame.gameState = GAME_NEW_LEVEL;
		break;
	case GAME_PREV_LEVEL:
		if (currentGame.level) {
			currentGame.level--;
		}
		currentGame.gameState = GAME_NEW_LEVEL;
		break;
	case GAME_RUNNER_DEAD:
		currentGame.lives--;
		displayLives();
		if (currentGame.lives <= 0) {
			// TODO: Game Over
			gameOver();
			currentGame.gameState = GAME_OVER;
		} else {
			currentGame.gameState = GAME_NEW_LEVEL;
		}
		break;
	case GAME_OVER:
		// Keep "Game Over" or "WORLD COMPLETE" displayed for 5 seconds then go back to splash
		sleep_us (5);
		currentGame.gameState = GAME_SPLASH;
		break;
	default:
		break;
	}
}

void sprites_enable (unsigned char sprite_id, unsigned char enable)
{
	uint8_t   idx;
  uint8_t   attr;

	for (idx = 0; idx < MAX_SPRITE; idx += 1) {
		if ((sprite_id == idx) || (sprite_id = 0xFF)) {
			attr = VGA_SPR_ATTR_SIZE_8 | VGA_SPR_ATTR_FRONT;
			if (enable) {
				attr |= VGA_SPR_ATTR_EN;
			}
			SET_VGA_SPR_ATTR (idx, attr);
		}
	}
}

void board_init()
{
	uint8_t   Idx;
	uint32_t  Addr;
	uint16_t  CfgVal;

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

	// Turn on TEXT mode
	MMIO_B(VGA_TXT_START) = 0; //12 * 18;

	// Disable spirit
	int x, y;
	for (x = 0; x < 128; x++) {
		// Fill spirit buffer
		SET_VGA_SPR_ATTR (x,  0);
	}

	// Turn on VGA HPRAM access
  MMIO_B(VGA_CTLBASE) = VGA_CTL_EN_HPC | VGA_CTL_EN | VGA_CTL_TILE_EN;

	unsigned short *ptrs = (unsigned short *)(VGA_TXT_BASE);
	for (y = 0; y < 30; y++) {
		for (x = 0; x < 40; x++) {
			*ptrs++ = 0x0000;
		}
	}

}

uint8_t has_key()
{
	if (hasch()) {
		return 1;
	}

  if (MMIO_S(SYS_CTRL_JOYSTICK) != 0xffff) {
		if (MMIO (TIMER_CNT0) == 0) {
			return 1;
		}
	}

	if ((MMIO (PS2_STS) & PS2_STS_AVL) != 0) {
    return 1;
  }

	return 0;
}

uint8_t wait_key (unsigned int timeout)
{
	MMIO (TIMER_CNT2) = timeout ;
	while (!hasch() && MMIO (TIMER_CNT2));
	return hasch();
}


#define  PS2_FLAG_RELEASE    BIT0
#define  PS2_FLAG_EXT        BIT1
#define  PS2_FLAG_MOD_SHIFT  BIT4
#define  PS2_FLAG_MOD_CTRL   BIT5
#define  PS2_FLAG_MOD_ALT    BIT6


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

static
char get_joystick_key (void)
{
	uint16_t  state;
	char      c;

	state = MMIO_S(SYS_CTRL_JOYSTICK);
	if ((state == 0xFFFF) || MMIO (TIMER_CNT0)) {
		return 0;
	}

	if (!(state & 0x0400)) {
		c = 'z';
	} else if (!(state & 0x0800)) {
		c = 'x';
	} else if (!(state & 0x1000)) {
		c = '\r';
	} else if (!(state & 0x0010)) {
		c = 0x48;
	} else if (!(state & 0x0040)) {
		c = 0x50;
  } else if (!(state & 0x0080)) {
		c = 0x4B;
	} else if (!(state & 0x0020)) {
		c = 0x4D;
	} else {
		c = 0;
	}

	if (c) {
		MMIO (TIMER_CNT0) = 0xFFFFFF;
	}

	return c;
}

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

	  // so we got scancodes, map to
	  if (c > 0 && c < sizeof(ps2_set2)) {
			c = ps2_set2[c];
		} else {
			if (c == 0xe075) {
				c = 0x48;
			} else if (c == 0xe072) {
				c = 0x50;
			} else if (c == 0xe06b) {
				c = 0x4B;
			} else if (c == 0xe074) {
				c = 0x4D;
			} else {
				c = 0;
			}
		}

	  //if (ps2_flags | PS2_FLAG_MOD_SHIFT)
	  //  c = toupper(c);

	  if (ps2_release)
	    c = 0;

	  ps2_flags &= ~(PS2_FLAG_RELEASE | PS2_FLAG_EXT);
	}

  return (char)c;
}


#define WAIT_TIMOUT  80000
uint8_t get_key()
{
	uint8_t key, key2;

	key = 0;
	if (hasch ()) {
		key = getch();
		if (key == 0x1b) {
			key = getch();
			if (key == 0x5b) {
				key = getch();
				if (key == 0x41) {
					key = 0x48;
				} else if (key == 0x42) {
					key = 0x50;
				} else if (key == 0x44) {
					key = 0x4B;
				} else if (key == 0x43) {
					key = 0x4D;
				}
			}
		} else {
			if (key == '4') {
				key = 0x4B;
			} else if (key == '5') {
				key = 0x50;
			} else if (key == '6') {
				key = 0x4D;
			} else if (key == '8') {
				key = 0x48;
			}
		}
		return key;
	} else {
		key = get_joystick_key ();
		if (!key)
			key = get_kbd_char ();
	}

	return key;
}

void sleep_us (int us)
{
	UINT64     End;
	UINT64_HL  Curr;

	End = GET_TSC (Curr) + CLK_FREQ_MHZ * us;
	while (Curr.Value < End) {
		(void)GET_TSC (Curr);
	}
}

void waitvsync()
{
	// Wait from timeout
	while (MMIO (TIMER_CNT1));

	// Reset timer  30ms
	MMIO (TIMER_CNT1) = 800000 * 3;
}


unsigned int *irq(unsigned int *regs, unsigned int irqs)
{
  (void)irqs;
  return regs;
}

int main()
{
	uint8_t result = 0;

	board_init ();

	currentGame.world = WORLD_CLASSIC;
	// Test world for benchmarks and isolated testing
	//world = WORLD_CUSTOM;
	currentGame.level = 1;
	currentGame.gameState = GAME_SPLASH;
	currentGame.lives = 5;

	printf ("loading resources...\n");

	// result = loadFiles();
	result = 1;

	if (result) {
		printf ("loaded resources successfully\n");
	} else {
		printf ("failed to load all resources\n");
		return result;
	}
	initLevels();
	currentGame.currentScore = 0;

	screenConfig();

	// Enable sprites
	// sprites_enable (0xff, 1);

	do {
		// Wait for next VSYNC interrupt
		waitvsync();

		mainTick();

	} while (1);

	return 0;
}
