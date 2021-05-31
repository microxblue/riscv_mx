#include <unistd.h>
#include "hardware.h"
#include "lib.h"

#define YMREG(x,y)

static unsigned int curr_page = 0;

// Start or continue playing the specified sound effect when
// called from the main loop
int playFx(struct soundFx_t *fx)
{
    if (fx->delay) {
        fx->delay--;
        return 0;
    }
    else {
        uint8_t idx = fx->idx;
        uint8_t reg = fx->data[idx++];
        uint8_t value = fx->data[idx++];
        fx->idx = idx;
        if (reg == FX_DELAY_REG) {
            fx->delay = value;
            return 0;
        }
        if (reg == FX_DONE_REG) {
            fx->idx = 0;
            fx->delay = 0;
            return 1;
        }
        YMREG(reg, value);
        return 0;
    }
}

// Synchronously play a specified sound effect in its own main loop
void playFxSync(struct soundFx_t *fx)
{
    while (!playFx(fx)) {
         waitvsync();
    }
}

int stopFx(struct soundFx_t *fx) {
    int8_t i = 0;
    for (i = 0; i < 8; i++) {
        // If this effect uses this channel then send KEY OFF event
        if (fx->channelMask & (1 << i)) {
            YMREG(YM_KEY_ON,i);
        }
    }

    // Reset state information
    fx->idx = 0;
    fx->delay = 0;
    return 1;
}


uint16_t vload_host(const char *fileName, uint32_t rawAddr)
{
    (void)(fileName);
    (void)(rawAddr);
    return 0;
}

uint16_t load_bank_host(const char *fileName, uint8_t bank)
{
    (void)(fileName);
    (void)(bank);
    return 0;
}

void switch_bank (unsigned int page)
{
    curr_page = page;
}

unsigned int get_hbc_addr (unsigned int offset)
{
  return  HYRAM_BASE + 0x100000 + curr_page * 0x2000 + (offset - 0xA000);
}
