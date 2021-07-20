// AAS Example for projects with other CPU intensive interrupts
// Notes:
//  + AAS_DoWork() must be called at least 50 times/sec. In this example, it's
//    being called during VBlank.

#include "AAS.h"
#include "AAS_Data.h"
#include "tonc.h"

// Registers for GBA keys
#define REG_KEY (*(volatile AAS_u16 *)0x04000130)
#define REG_KEY_A 0x0001
#define REG_KEY_B 0x0002

void main() {
  int keys, keys_changed;
  int prev_keys = 0;

  // Enable vblank interrupt
  REG_DISPSTAT |= 0x8;
  REG_IE |= 0x1;

  // Initialise AAS
  AAS_SetConfig(AAS_CONFIG_MIX_32KHZ, AAS_CONFIG_CHANS_8,
                AAS_CONFIG_SPATIAL_STEREO, AAS_CONFIG_DYNAMIC_OFF);

  // Set up the interrupt handlers
  irq_init(NULL);
  // set timer 1 to AAS_FastTimer1InterruptHandler()
  irq_add(II_TIMER1, AAS_FastTimer1InterruptHandler);
  // call AAS_DoWork() during vblank
  irq_add(II_VBLANK, AAS_DoWork);
  
  // Start playing MOD
  AAS_MOD_Play(AAS_DATA_MOD_CreamOfTheEarth);

  // Show AAS Logo (not required)
  AAS_ShowLogo();

  // Main loop
  do {
    // Work out which keys have just been pressed
    keys = ~REG_KEY;
    keys_changed = keys ^ prev_keys;
    prev_keys = keys;

    // Play looping ambulance sound effect out of left speaker if A button is
    // pressed, stop when released
    if (keys_changed & REG_KEY_A) {
      if (keys & REG_KEY_A)
        AAS_SFX_Play(0, 64, 16000, AAS_DATA_SFX_START_Ambulance,
                     AAS_DATA_SFX_END_Ambulance, AAS_DATA_SFX_START_Ambulance);
      else
        AAS_SFX_Stop(0);
    }

    // Play explosion sound effect out of right speaker if B button is pressed
    if (keys_changed & keys & REG_KEY_B)
      AAS_SFX_Play(1, 64, 8000, AAS_DATA_SFX_START_Boom, AAS_DATA_SFX_END_Boom,
                   AAS_NULL);
  } while (1);
}
