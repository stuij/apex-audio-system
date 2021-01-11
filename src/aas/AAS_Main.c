// Copyright (c) 2003-2021 James Daniels
// Distributed under the MIT License
// license terms: see LICENSE file in root or http://opensource.org/licenses/MIT

#include "AAS_Shared.h"

extern const int AAS_data_v111;
int AAS_lib_v111 AAS_IN_EWRAM;

static AAS_BOOL AAS_da_active AAS_IN_EWRAM;
static AAS_BOOL AAS_db_active AAS_IN_EWRAM;
static AAS_BOOL AAS_dynamic_mix_rate AAS_IN_EWRAM;

// 0 = 0 Hz mix rate
// 1 = 800 Hz mix rate, 16 byte mix buffer
// 2 = 1600 Hz mix rate, 32 byte mix buffer
// n = 800n Hz mix rate, 16n byte mix buffer
// n <= 40
// REG_TM1D = 0x10000 - 16n;
// REG_TM0D = AAS_tick_rate[n]; // 0x10000 - ((int)(16777216/800n));
static AAS_u8 AAS_req_mix_rate AAS_IN_EWRAM;
static AAS_u8 AAS_next_mix_rate AAS_IN_EWRAM;

static AAS_u8* AAS_next_mixing_buffer AAS_IN_EWRAM;

static AAS_BOOL AAS_DSA_first AAS_IN_EWRAM = AAS_TRUE;

static AAS_u8 AAS_MaxChans AAS_IN_EWRAM = 4;

static AAS_BOOL AAS_Loud AAS_IN_EWRAM = AAS_FALSE;
static AAS_u8 AAS_MixAudio_Mode AAS_IN_EWRAM = AAS_MIXAUDIO_MODE_NORMAL;

void AAS_MixAudio_SetMode( int mode )
{
	if ( mode != AAS_MixAudio_Mode )
	{
		switch( mode )
		{
			case AAS_MIXAUDIO_MODE_NORMAL:
				AAS_MixAudio_SetMode_Normal();
				AAS_MixAudio_Mode = AAS_MIXAUDIO_MODE_NORMAL;
				AAS_changed[0] = AAS_TRUE;
				AAS_changed[1] = AAS_TRUE;
			break;
			
			case AAS_MIXAUDIO_MODE_BOOST:
				AAS_MixAudio_SetMode_Boost();
				AAS_MixAudio_Mode = AAS_MIXAUDIO_MODE_BOOST;
				AAS_changed[0] = AAS_TRUE;
				AAS_changed[1] = AAS_TRUE;
			break;
			
			case AAS_MIXAUDIO_MODE_BOOSTANDCLIP:
				AAS_MixAudio_SetMode_BoostAndClip();
				AAS_MixAudio_Mode = AAS_MIXAUDIO_MODE_BOOSTANDCLIP;
				AAS_changed[0] = AAS_TRUE;
				AAS_changed[1] = AAS_TRUE;
			break;
			
			default:
			break;
		}
	}
}

static void AAS_DoConfig( int mix_rate, int volscale, AAS_BOOL stereo, AAS_BOOL dynamic, AAS_BOOL loud, int chans )
{
	struct AAS_Channel* ch;
	int i;
	
	AAS_MOD_Stop();
	
	if ( !AAS_initialised )
	{
		REG_SOUNDCNT_X = 0x0080; // turn sound chip on
		
		REG_DMA1SAD = (AAS_u32)AAS_mix_buffer; //dma1 source
		REG_DMA1DAD = 0x040000a0; //write to FIFO A address
		REG_DMA1CNT_H = 0xb600; //dma control: DMA enabled+ start on FIFO+32bit+repeat+increment source&dest
	
		REG_DMA2SAD = (AAS_u32)(AAS_mix_buffer + 160); //dma2 source
		REG_DMA2DAD = 0x040000a4; //write to FIFO B address
		REG_DMA2CNT_H = 0xb600; //dma control: DMA enabled+ start on FIFO+32bit+repeat+increment source&dest
		
		AAS_next_mixing_buffer = ((AAS_u8*)AAS_mix_buffer) + 1280;
		
		REG_IE |= 0x10; // Enable irq for timer 1
		REG_IME = 1;    // Enable all interrupts
		
		AAS_da_active = AAS_FALSE;
		AAS_db_active = AAS_FALSE;
	}
	
	REG_TM0CNT = 0x0;
	REG_TM1CNT = 0x0;
	
	if ( chans != AAS_MaxChans )
	{
		switch( chans )
		{
			case 8:
				AAS_MixAudio_SetMaxChans_8();
			break;
			
			case 4:
				AAS_MixAudio_SetMaxChans_4();
			break;
			
			default:
				AAS_MixAudio_SetMaxChans_2();
			break;
		}
		AAS_MaxChans = chans;
	}
	
	AAS_next_mix_rate = AAS_req_mix_rate = mix_rate;
	AAS_mix_scale = ((AAS_DivTable[mix_rate]*82)+128)>>6;
	AAS_dynamic_mix_rate = dynamic;
	
	if ( stereo )
	{
		const AAS_u8 chan_rearrange[AAS_MAX_CHANNELS] = { 0, 8, 9, 1, 2, 10, 11, 3, 4, 12, 13, 5, 6, 14, 15, 7 };
				
		for( i = 0; i < AAS_MAX_CHANNELS; ++i )
		{
			AAS_chan_rearrange[i] = chan_rearrange[i];
		}
		
		REG_SOUNDCNT_H = 0x9a0d; //enable DS A&B + fifo reset + use timer0 + 100% volume to L and R
	}
	else
	{
		int a, b;
				
		a = 0;
		for( b = 0; b < chans; ++b )
		{
			AAS_chan_rearrange[a] = b;
			++a;
		}
		for( b = 0; b < chans; ++b )
		{
			AAS_chan_rearrange[a] = b + 8;
			++a;
		}
		
		REG_SOUNDCNT_H = 0xbb0d; //enable DS A&B + fifo reset + use timer0 + 100% volume to L and R
	}
	
	ch = &AAS_channels[0];
	for( i = 16; i > 0; --i )
	{
	  ch->active = AAS_FALSE;
	  ch->loop_length = 0;
	  ch->pos = 0;
	  ch->end = 0;
	  ch->delta = 0;
	  ++ch;
	 }
	
	AAS_volscale = volscale;
	
	AAS_Loud = loud;
	if ( !loud )
		AAS_MixAudio_SetMode( AAS_MIXAUDIO_MODE_NORMAL );
	
	REG_TM0D = AAS_tick_rate[mix_rate];
	REG_TM0CNT = 0x0080; // Enable timer0
	
	REG_TM1D = 0x10000 - (mix_rate<<4);
	REG_TM1CNT = 0xC4; //enable timer1 + irq and cascade from timer 0
	
	AAS_lib_v111 = AAS_data_v111;
}

int AAS_SetConfig( int config_mix, int config_chans, int config_spatial, int config_dynamic )
{
	int i, chans, mix_rate, volscale, ret;
	AAS_BOOL stereo;
	AAS_BOOL dynamic;
	AAS_BOOL loud;
	
	ret = AAS_OK;
	
	switch( config_mix )
	{
		case AAS_CONFIG_MIX_32KHZ:
			mix_rate = 40;
		break;
		
		case AAS_CONFIG_MIX_28KHZ:
			mix_rate = 35;
		break;
		
		case AAS_CONFIG_MIX_24KHZ:
			mix_rate = 30;
		break;
		
		case AAS_CONFIG_MIX_20KHZ:
			mix_rate = 25;
		break;
		
		case AAS_CONFIG_MIX_16KHZ:
			mix_rate = 20;
		break;
		
		case AAS_CONFIG_MIX_12KHZ:
			mix_rate = 15;
		break;
		
		case AAS_CONFIG_MIX_8KHZ:
			mix_rate = 10;
		break;
		
		default:
			ret = AAS_ERROR_INVALID_CONFIG;
		break;
	}
	
	switch( config_chans )
	{
		case AAS_CONFIG_CHANS_16_LOUD:
			volscale = 9;
			loud = AAS_TRUE;
			chans = 8;
		break;
		
		case AAS_CONFIG_CHANS_8_LOUD:
			volscale = 8;
			loud = AAS_TRUE;
			chans = 4;
		break;
		
		case AAS_CONFIG_CHANS_4_LOUD:
			volscale = 7;
			loud = AAS_TRUE;
			chans = 2;
		break;
		
		case AAS_CONFIG_CHANS_16:
			volscale = 9;
			loud = AAS_FALSE;
			chans = 8;
		break;
		
		case AAS_CONFIG_CHANS_8:
			volscale = 8;
			loud = AAS_FALSE;
			chans = 4;
		break;
		
		case AAS_CONFIG_CHANS_4:
			volscale = 7;
			loud = AAS_FALSE;
			chans = 2;
		break;
		
		default:
			ret = AAS_ERROR_INVALID_CONFIG;
		break;
	}
	
	switch( config_spatial )
	{
		case AAS_CONFIG_SPATIAL_MONO:
			stereo = AAS_FALSE;
		break;
		
		case AAS_CONFIG_SPATIAL_STEREO:
			stereo = AAS_TRUE;
		break;
		
		default:
			ret = AAS_ERROR_INVALID_CONFIG;
		break;
	}
	
	switch( config_dynamic )
	{
		case AAS_CONFIG_DYNAMIC_ON:
			dynamic = AAS_TRUE;
		break;
		
		case AAS_CONFIG_DYNAMIC_OFF:
			dynamic = AAS_FALSE;
		break;
		
		default:
			ret = AAS_ERROR_INVALID_CONFIG;
		break;
	}
	
	if ( ret == AAS_OK )
	{
  	AAS_DoConfig( mix_rate, volscale, stereo, dynamic, loud, chans );
  	
  	AAS_initialised = AAS_TRUE;
  }
  
  return ret;
}

const AAS_s8* AAS_GetOutputBufferAddress( int buffer )
{
	switch( buffer )
	{
		case 0:
			if ( AAS_da_active )
				return AAS_next_mixing_buffer;
			else
				return AAS_NULL;
		break;
		
		case 1:
			if ( AAS_db_active )
				return AAS_next_mixing_buffer + 640;
			else
				return AAS_NULL;
		break;
		
		default:
			return AAS_NULL;
		break;
	}
}

int AAS_GetOutputBufferLength()
{
	return AAS_next_mix_rate*16;
}

const AAS_u32 AAS_zero_vols[160] = { 0 };

static AAS_BOOL AAS_interrupt_occured AAS_IN_EWRAM = AAS_FALSE;

void AAS_FastTimer1InterruptHandler()
{
	if ( AAS_dynamic_mix_rate )
	{
		REG_TM0CNT = 0x0;
		REG_TM0D = AAS_tick_rate[AAS_next_mix_rate];
		REG_TM0CNT = 0x0080; // Enable timer0
		REG_TM1CNT = 0x0;
		REG_TM1D = 0x10000 - (AAS_next_mix_rate<<4);
		REG_TM1CNT = 0xC4; // Enable timer1 + irq and cascade from timer 0
	}
	
	REG_DMA1CNT = 0x84400004;
	REG_DMA2CNT = 0x84400004;
	REG_DMA1CNT_H = 0x0440;
	REG_DMA2CNT_H = 0x0440;
	if ( AAS_da_active )
		REG_DMA1SAD = (unsigned long)AAS_next_mixing_buffer; // DMA1 source
	else
		REG_DMA1SAD = (unsigned long)AAS_zero_vols;
	REG_DMA1CNT_H = 0xb600; // DMA control: DMA enabled+start on FIFO+32bit+repeat+increment source&dest
	// Get click on hardware when switch off DMA on Direct Sound B, so have to do it this way instead
	if ( AAS_db_active )
		REG_DMA2SAD = (unsigned long)AAS_next_mixing_buffer + 640; // DMA2 source
	else
		REG_DMA2SAD = (unsigned long)AAS_zero_vols; // DMA2 source
	REG_DMA2CNT_H = 0xb600; // DMA control: DMA enabled+start on FIFO+32bit+repeat+increment source&dest
	
	AAS_interrupt_occured = AAS_TRUE;
}

#define READCH1 \
				if ( ch->active ) \
				{ \
					int vol = ch->volume; \
					if ( vol == 0 ) \
					{ \
						int delta = ch->delta; \
						const AAS_s8* end_addr; \
						addr = ch->pos + ((delta*curr_mix_rate)>>6); \
						end_addr = (ch->end - (delta>>6)) - 1; \
						if ( addr >= end_addr ) \
						{ \
							int ll = ch->loop_length; \
							if ( ll ) \
							{ \
								while( addr >= end_addr ) \
								{ \
									addr -= ll; \
								} \
							} \
							else \
							{ \
								ch->active = AAS_FALSE; \
							} \
						} \
						ch->pos = addr; \
					} \
					else \
					{ \
						tmp1 += vol; \
					} \
					ch->effective_volume = vol; \
				} \
				else \
				{ \
					ch->effective_volume = 0; \
				} \
			
#define READCH2 \
				if ( ch->active ) \
				{ \
					int vol = ch->volume; \
					if ( vol == 0 ) \
					{ \
						int delta = ch->delta; \
						const AAS_s8* end_addr; \
						addr = ch->pos + ((delta*curr_mix_rate)>>6); \
						end_addr = (ch->end - (delta>>6)) - 1; \
						if ( addr >= end_addr ) \
						{ \
							int ll = ch->loop_length; \
							if ( ll ) \
							{ \
								while( addr >= end_addr ) \
								{ \
									addr -= ll; \
								} \
							} \
							else \
							{ \
								ch->active = AAS_FALSE; \
							} \
						} \
						ch->pos = addr; \
					} \
					else \
					{ \
						tmp2 += vol; \
					} \
					ch->effective_volume = vol; \
				} \
				else \
				{ \
					ch->effective_volume = 0; \
				} \

void AAS_DoWork()
{
	if ( AAS_interrupt_occured )
	{
		AAS_interrupt_occured = AAS_FALSE;
		
		if ( AAS_next_mixing_buffer == (AAS_u8*)AAS_mix_buffer )
			AAS_next_mixing_buffer = ((AAS_u8*)AAS_mix_buffer) + 1280;
		else
			AAS_next_mixing_buffer = ((AAS_u8*)AAS_mix_buffer);
		
		AAS_MOD_Interrupt();
		
		{
			int tmp1, tmp2, val, curr_mix_rate;
			struct AAS_Channel* ch;
			const AAS_s8* addr;
			
			curr_mix_rate = AAS_req_mix_rate;
			
			if ( AAS_dynamic_mix_rate )
			{
				val = 0;
				ch = &AAS_channels[0];
				for( tmp2 = AAS_MaxChans; tmp2 > 0; --tmp2 )
				{
					if ( ch->active && (ch->volume > 0) )
					{
						tmp1 = ch->frequency;
						if ( tmp1 > val )
							val = tmp1;
					}
					++ch;
				}
				
				ch = &AAS_channels[8];
				for( tmp2 = AAS_MaxChans; tmp2 > 0; --tmp2 )
				{
					if ( ch->active && (ch->volume > 0) )
					{
						tmp1 = ch->frequency;
						if ( tmp1 > val )
							val = tmp1;
					}
					++ch;
				}
				
				val = ((val * 82)>>16)+1;
				if ( val < curr_mix_rate )
					curr_mix_rate = val;
				
				if ( AAS_next_mix_rate != curr_mix_rate )
				{
					AAS_next_mix_rate = curr_mix_rate;
					AAS_mix_scale = val = ((AAS_DivTable[curr_mix_rate]*82)+128)>>6;
					ch = &AAS_channels[0];
					for( tmp2 = AAS_MaxChans; tmp2 > 0; --tmp2 )
					{
						if ( ch->active )
							ch->delta = AAS_Min( 4095, ((ch->frequency*val)+32768)>>16 );
						++ch;
					}
					
					ch = &AAS_channels[8];
					for( tmp2 = AAS_MaxChans; tmp2 > 0; --tmp2 )
					{
						if ( ch->active )
							ch->delta = AAS_Min( 4095, ((ch->frequency*val)+32768)>>16 );
						++ch;
					}
					
					AAS_changed[0] = AAS_TRUE;
					AAS_changed[1] = AAS_TRUE;
				}
			}
			
			tmp1 = 0;
			tmp2 = 0;
			
			ch = &AAS_channels[0];
			switch( AAS_MaxChans )
			{
				case 8:
					READCH1
					++ch;
					READCH1
					++ch;
					READCH1
					++ch;
					READCH1
					++ch;
					READCH1
					++ch;
					READCH1
					++ch;
					READCH1
					++ch;
					READCH1
					++ch;
					READCH2
					++ch;
					READCH2
					++ch;
					READCH2
					++ch;
					READCH2
					++ch;
					READCH2
					++ch;
					READCH2
					++ch;
					READCH2
					++ch;
					READCH2
				break;
				
				case 4:
					READCH1
					++ch;
					READCH1
					++ch;
					READCH1
					++ch;
					READCH1
					ch = &AAS_channels[8];
					READCH2
					++ch;
					READCH2
					++ch;
					READCH2
					++ch;
					READCH2
				break;
					
				case 2:
				default:
					READCH1
					++ch;
					READCH1
					ch = &AAS_channels[8];
					READCH2
					++ch;
					READCH2
				break;
			}
			
			if ( AAS_Loud )
			{
				if ( AAS_DSA_first )
				{
					// Direct Sound A
					if ( tmp1 )
					{
						if ( tmp1 > 128 )
						{
							AAS_MixAudio_SetMode( AAS_MIXAUDIO_MODE_BOOSTANDCLIP );
						}
						else
						{
							AAS_MixAudio_SetMode( AAS_MIXAUDIO_MODE_BOOST );
						}
						
						if ( AAS_changed[0] )
						{
							AAS_INIT_BRANCH
							AAS_BRANCH( AAS_MixAudio, AAS_next_mixing_buffer, &AAS_channels[0], curr_mix_rate );
						}
						else
						{
							AAS_INIT_BRANCH
							AAS_BRANCH( AAS_MixAudio_NoChange, AAS_next_mixing_buffer, &AAS_channels[0], curr_mix_rate );
						}
						AAS_da_active = AAS_TRUE;
					}
					else
					{
						AAS_da_active = AAS_FALSE;
					}
					
					// Direct Sound B
					if ( tmp2 )
					{
						if ( tmp2 > 128 )
						{
							AAS_MixAudio_SetMode( AAS_MIXAUDIO_MODE_BOOSTANDCLIP );
						}
						else
						{
							AAS_MixAudio_SetMode( AAS_MIXAUDIO_MODE_BOOST );
						}
						
						{
							AAS_INIT_BRANCH
							AAS_BRANCH( AAS_MixAudio, AAS_next_mixing_buffer + 640, &AAS_channels[8], curr_mix_rate );
						}
						
						AAS_db_active = AAS_TRUE;
					}
					else
					{
						AAS_db_active = AAS_FALSE;
					}
				
					AAS_DSA_first = AAS_FALSE;
				}
				else
				{
					// Direct Sound B
					if ( tmp2 )
					{
						if ( tmp2 > 128 )
						{
							AAS_MixAudio_SetMode( AAS_MIXAUDIO_MODE_BOOSTANDCLIP );
						}
						else
						{
							AAS_MixAudio_SetMode( AAS_MIXAUDIO_MODE_BOOST );
						}
						
						if ( AAS_changed[1] )
						{
							AAS_INIT_BRANCH
							AAS_BRANCH( AAS_MixAudio, AAS_next_mixing_buffer + 640, &AAS_channels[8], curr_mix_rate );
						}
						else
						{
							AAS_INIT_BRANCH
							AAS_BRANCH( AAS_MixAudio_NoChange, AAS_next_mixing_buffer + 640, &AAS_channels[8], curr_mix_rate );
						}
						AAS_db_active = AAS_TRUE;
					}
					else
					{
						AAS_db_active = AAS_FALSE;
					}
					
					// Direct Sound A
					if ( tmp1 )
					{
						if ( tmp1 > 128 )
						{
							AAS_MixAudio_SetMode( AAS_MIXAUDIO_MODE_BOOSTANDCLIP );
						}
						else
						{
							AAS_MixAudio_SetMode( AAS_MIXAUDIO_MODE_BOOST );
						}
						
						{
							AAS_INIT_BRANCH
							AAS_BRANCH( AAS_MixAudio, AAS_next_mixing_buffer, &AAS_channels[0], curr_mix_rate );
						}
						
						AAS_da_active = AAS_TRUE;
					}
					else
					{
						AAS_da_active = AAS_FALSE;
					}
					
					AAS_DSA_first = AAS_TRUE;
				}
			}
			else
			{
				if ( AAS_DSA_first )
				{
					// Direct Sound A
					if ( tmp1 )
					{
						if ( AAS_changed[0] )
						{
							AAS_INIT_BRANCH
							AAS_BRANCH( AAS_MixAudio, AAS_next_mixing_buffer, &AAS_channels[0], curr_mix_rate );
						}
						else
						{
							AAS_INIT_BRANCH
							AAS_BRANCH( AAS_MixAudio_NoChange, AAS_next_mixing_buffer, &AAS_channels[0], curr_mix_rate );
						}
						AAS_da_active = AAS_TRUE;
					}
					else
					{
						AAS_da_active = AAS_FALSE;
					}
				
					// Direct Sound B
					if ( tmp2 )
					{
						{
							AAS_INIT_BRANCH
							AAS_BRANCH( AAS_MixAudio, AAS_next_mixing_buffer + 640, &AAS_channels[8], curr_mix_rate );
						}
						AAS_db_active = AAS_TRUE;
					}
					else
					{
						AAS_db_active = AAS_FALSE;
					}
					
					AAS_DSA_first = AAS_FALSE;
				}
				else
				{
					// Direct Sound B
					if ( tmp2 )
					{
						if ( AAS_changed[1] )
						{
							AAS_INIT_BRANCH
							AAS_BRANCH( AAS_MixAudio, AAS_next_mixing_buffer + 640, &AAS_channels[8], curr_mix_rate );
						}
						else
						{
							AAS_INIT_BRANCH
							AAS_BRANCH( AAS_MixAudio_NoChange, AAS_next_mixing_buffer + 640, &AAS_channels[8], curr_mix_rate );
						}
						AAS_db_active = AAS_TRUE;
					}
					else
					{
						AAS_db_active = AAS_FALSE;
					}
					
					// Direct Sound A
					if ( tmp1 )
					{
						{
							AAS_INIT_BRANCH
							AAS_BRANCH( AAS_MixAudio, AAS_next_mixing_buffer, &AAS_channels[0], curr_mix_rate );
						}
						
						AAS_da_active = AAS_TRUE;
					}
					else
					{
						AAS_da_active = AAS_FALSE;
					}
					
					AAS_DSA_first = AAS_TRUE;
				}
			}
			
			AAS_changed[0] = AAS_FALSE;
			AAS_changed[1] = AAS_FALSE;
		}
	}
}

void AAS_Timer1InterruptHandler()
{
	AAS_FastTimer1InterruptHandler();
	AAS_DoWork();
}

int AAS_GetActualMixRate()
{
	return AAS_next_mix_rate*800;
}
