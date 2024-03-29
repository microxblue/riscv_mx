#include <stdint.h>
#include "firmware.h"




#define SPLASH 21

// Tile definitions
#define TILE_BRICK  64
#define TILE_BLOCK  65
#define TILE_TRAP   66
#define TILE_LADDER 67
#define TILE_HIDDEN 68
#define TILE_ROPE   69
#define TILE_GOLD   70
#define TILE_GROUND 71
#define TILE_DIG_LEFT_U1 72
#define TILE_DIG_LEFT_L1 73
#define TILE_DIG_LEFT_U2 74
#define TILE_DIG_LEFT_L2 75
#define TILE_DIG_LEFT_U3 76
#define TILE_DIG_LEFT_L3 77
#define TILE_DIG_LEFT_U4 78
#define TILE_DIG_LEFT_L4 79
#define TILE_DIG_LEFT_U5 80
#define TILE_DIG_LEFT_L5 81
#define TILE_DIG_LEFT_U6 82
#define TILE_DIG_LEFT_L6 83

#define TILE_DIG_RIGHT_U1 84
#define TILE_DIG_RIGHT_L1 85
#define TILE_DIG_RIGHT_U2 86
#define TILE_DIG_RIGHT_L2 87
#define TILE_DIG_RIGHT_U3 88
#define TILE_DIG_RIGHT_L3 89
#define TILE_DIG_RIGHT_U4 90
#define TILE_DIG_RIGHT_L4 91
#define TILE_DIG_RIGHT_U5 92
#define TILE_DIG_RIGHT_L5 93
#define TILE_DIG_RIGHT_U6 94
#define TILE_DIG_RIGHT_L6 95
#define TILE_REGEN1 96
#define TILE_REGEN2 97
#define TILE_REGEN3 98

#define TILE_BLANK  32
#define TILE_GUARD  36
#define TILE_RUNNER 37

// Level definitions
#define LEVEL_COUNT get_hbc_addr(0xa000)
#define LEVEL_BASE  get_hbc_addr(0xa004)
#define LEVEL_ROW_OFFSET 28
#define LEVEL_ROW_BANK_OFFSET 14
#define LEVEL_ROW_COUNT 16
#define LEVEL_OFFSET 224

// Sprite images
#define RUNNER_1        0x00 //0x1e000
#define RUNNER_2        0x01 //0x1e020
#define RUNNER_3        0x02 //0x1e040

#define RUNNER_CLIMB_1  0x10 //0x1e060
#define RUNNER_CLIMB_2  0x11 //0x1e080
#define RUNNER_FALLING  0x12 //0x1e0a0

#define RUNNER_RAPPEL_1 0x20 //0x1e0c0
#define RUNNER_RAPPEL_2 0x21 //0x1e0e0
#define RUNNER_RAPPEL_3 0x22 //0x1e100

#define GUARD_1         0x30 //0x1e120
#define GUARD_2         0x31 //0x1e140
#define GUARD_3         0x32 //0x1e160

#define GUARD_CLIMB_1   0x40 //0x1e180
#define GUARD_CLIMB_2   0x41 //0x1e1a0
#define GUARD_FALLING   0x42 //0x1e1c0

#define GUARD_RAPPEL_1  0x50 //0x1e1e0
#define GUARD_RAPPEL_2  0x51 //0x1e200
#define GUARD_RAPPEL_3  0x52 //0x1e220

#define MAX_SPRITE      4
#define MAX_GUARDS      (MAX_SPRITE-1)

#define CHAR_UP         0x48
#define CHAR_DOWN       0x50
#define CHAR_ENTER      0x0D
#define CHAR_LT2        0x3C
#define CHAR_GT2        0x3E
#define CHAR_LT         0x7A
#define CHAR_GT         0x78
#define CHAR_LEFT       0x4B
#define CHAR_RIGHT      0x4D

// Actions
#define ACT_UNKNOWN     -1
#define ACT_STOP        ' '
#define ACT_LEFT        CHAR_LEFT
#define ACT_RIGHT       CHAR_RIGHT
#define ACT_UP          CHAR_UP
#define ACT_DOWN        CHAR_DOWN
#define ACT_FALL        5
#define ACT_FALL_BAR    6
#define ACT_DIG_LEFT    CHAR_LT
#define ACT_DIG_RIGHT   CHAR_GT
#define ACT_DIGGING     9
#define ACT_IN_HOLE     10
#define ACT_CLIMB_OUT   11
#define ACT_REBORN      12
#define ACT_START       CHAR_ENTER


// Score increments
#define SCORE_GET_GOLD      250
#define SCORE_IN_HOLE       75
#define SCORE_GUARD_DEAD    75
#define SCORE_COMPLETE_LEVEL    1500
#define SCORE_COUNT 50
#define SCORE_INCREMENT (SCORE_COMPLETE_LEVEL / SCORE_COUNT)

// Hole regeneration timixng
#define HOLE_REGEN1     490
#define HOLE_REGEN2     498
#define HOLE_REGEN3     506
#define HOLE_REGEN4     514

// Game states
#define GAME_SPLASH 0
#define GAME_RUNNING 1
#define GAME_NEW_LEVEL 2
#define GAME_NEXT_LEVEL 3
#define GAME_PREV_LEVEL 4
#define GAME_RUNNER_DEAD 5
#define GAME_FINISH 6
#define GAME_WIN 7
#define GAME_OVER 8
#define GAME_FINISH_SCORE_COUNT 9

// God mode (immortal runner)
#define GOD_MODE 1
#define MORTAL 0

// Declarations of all functions

// Loading resources from .BIN files - loader.c
extern int loadFiles();

// Screen configuration and tile set/get - screen.c
extern int screenConfig();
extern void screenReset();
extern void setTileOffsets(uint8_t x, uint8_t y);
extern void setTile(uint8_t x, uint8_t y, uint8_t tile, uint8_t paletteOffset);
extern uint8_t getTile(uint8_t x, uint8_t y);
extern uint8_t getTileXY (uint16_t x, uint16_t y);
extern uint8_t getTileBelowXY (uint16_t x, uint16_t y);



extern void splash(void);
