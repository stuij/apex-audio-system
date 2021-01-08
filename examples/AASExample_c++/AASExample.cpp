// AAS C++ Example for projects with no other CPU intensive interrupts
// Notes:
//  + crt0.s file included with the example should be set to use __FastInterrupts.
//  + Can also be made to work with __SingleInterrupts and __MultipleInterrupts.
//  + Use __AAS_MultipleInterrupts when there are other CPU intensive interrupts - see AASExample2.

#include "AAS.h"
#include "AAS_Data.h"

// Registers for interrupt handler
#define REG_IE (*(volatile AAS_u16 *)0x4000200)
#define REG_IF (*(volatile AAS_u16 *)0x4000202) 

extern "C" void InterruptProcess()
{
	AAS_u16 intr_bits = REG_IE & REG_IF;
	
	// It's best to test for AAS's Timer 1 interrupt first
	if ( intr_bits & 0x10 ) // Timer 1
		AAS_Timer1InterruptHandler();
	
	// Process other interrupts here by testing appropriate bits of "intr_bits"

	// Clear the interrupt flags
	REG_IF |= REG_IF;
}

// Registers for GBA keys
#define	REG_KEY (*(volatile AAS_u16 *)0x04000130)
#define REG_KEY_A 0x0001
#define REG_KEY_B 0x0002

extern "C" void AgbMain() 
{
	int keys, keys_changed;
	int prev_keys = 0;
	
	// Initialise AAS
	AAS_SetConfig( AAS_CONFIG_MIX_32KHZ, AAS_CONFIG_CHANS_8, AAS_CONFIG_SPATIAL_STEREO, AAS_CONFIG_DYNAMIC_OFF );
	
	// Start playing MOD
	AAS_MOD_Play( AAS_DATA_MOD_its_just_sonorous );
	
	// Show AAS Logo (required for non-commercial projects)
	AAS_ShowLogo();
	
	// Main loop
	do
	{
		// Work out which keys have just been pressed
		keys = ~REG_KEY;
		keys_changed = keys ^ prev_keys;
		prev_keys = keys;
		
		// Play looping ambulance sound effect out of left speaker if A button is pressed, stop when released
		if ( keys_changed & REG_KEY_A )
		{
			if ( keys & REG_KEY_A )
				AAS_SFX_Play( 0, 64, 16000, AAS_DATA_SFX_START_Ambulance, AAS_DATA_SFX_END_Ambulance, AAS_DATA_SFX_START_Ambulance );
			else
				AAS_SFX_Stop( 0 );
		}
		
		// Play explosion sound effect out of right speaker if B button is pressed
		if ( keys_changed & keys & REG_KEY_B )
			AAS_SFX_Play( 1, 64, 8000, AAS_DATA_SFX_START_Boom, AAS_DATA_SFX_END_Boom, AAS_NULL );
	}
	while( 1 );
}
