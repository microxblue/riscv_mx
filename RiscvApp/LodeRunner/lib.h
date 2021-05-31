#include <stdint.h>

// Soundfx "register" for delay
#define FX_DELAY_REG   2
// Soundfx "register" for end of fx
#define FX_DONE_REG    4

// Struct for a sound effect
struct soundFx_t {
    // Index into the sound effect data
    uint16_t idx;
    // Delay count (0 means to process the next data register/value)
    uint8_t delay;
    // Bitmask specifying which channels are used in case of preemption
    uint8_t channelMask;
    // Sound fx data consisting of 8-bit YM-2151 register and 8-bit value
    // Last register value must be of type FX_DONE_REG
    uint8_t data[];
};

// Start/continue playing a specified sound effect when called periodically from the main loop
// returns 1 when sound effect is complete
extern int playFx(struct soundFx_t *fx);

// Synchronously play a specified sound effect in its own main loop
void playFxSync(struct soundFx_t *fx);

// Preempt the specified sound effect and reset state information for
// next use of the effect
extern int stopFx(struct soundFx_t *fx);



// Load file to Banked RAM
extern uint16_t load_bank(const char *fileName, uint8_t device, uint8_t bank);

// Load from host (device 8)
extern uint16_t load_bank_host(const char *fileName, uint8_t bank);

// Load from SD (device 1)
extern uint16_t load_bank_sd(const char *fileName, uint8_t bank);

// Load file to specified address
extern uint16_t load_file(const char *fileName, uint8_t device, uint16_t addr);

// Load from host (device 8)
extern uint16_t load_file_host(const char *fileName, uint16_t addr);

// Load from SD (device 1)
extern uint16_t load_file_sd(const char *fileName, uint16_t addr);


// Load from specified device
extern uint16_t vload(const char *fileName, uint8_t device, uint32_t addr);

// Load from host (device 8)
extern uint16_t vload_host(const char *fileName, uint32_t addr);

// Load from SD card (device 1)
extern uint16_t vload_sd(const char *fileName, uint32_t addr);

