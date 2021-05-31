#include "hardware.h"
#include "loderunner.h"

// Offset between rows in tile map
#define OFFSET 256

int screenConfig()
{
    uint8_t x = 0;
    uint8_t y = 0;

    videomode(VIDEOMODE_40x30);

    // Clear the full tilemap
    for (x = 0; x < 40; x++) {
        for (y = 0; y < 30; y++) {
            setTile(x, y, TILE_BLANK, 0);
        }
    }
    return 1;
}

void screenReset() { videomode(VIDEOMODE_80x60); }

static uint8_t xOffset = 0;
static uint8_t yOffset = 0;

void setTileOffsets(uint8_t x, uint8_t y)
{
    xOffset = x;
    yOffset = y;
}

void setTile(uint8_t x, uint8_t y, uint8_t tile, uint8_t paletteOffset)
{
    uint16_t offset;

    (void)paletteOffset;

    x += xOffset;
    y += yOffset;
    if ((x >= 40) || (y >= 30)) {
      return;
    }

    offset = (y * 40 + x) * 2;
    VGA_TXT_BASE_PTR[offset]   = tile;
    VGA_TXT_BASE_PTR[offset+1] = 0;
}
