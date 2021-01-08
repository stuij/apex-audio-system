#include "AAS_Shared.h"

static AAS_s16 AAS_mod_num_store AAS_IN_EWRAM = -2;
static AAS_s16 AAS_mod_num AAS_IN_EWRAM = -1;
static AAS_u16 AAS_mod_song_pos AAS_IN_EWRAM;
static AAS_u16 AAS_mod_line_num AAS_IN_EWRAM;

struct AAS_MOD_Channel
{
	AAS_u32* pattern;
	const AAS_u8* samp_start;
	AAS_u16 effect;
	AAS_u16 period;
	AAS_u16 slide_target_period;
	AAS_u8 samp_num;
	AAS_u8 slide_rate;
	AAS_u8 volume;
	AAS_u8 vibrato_rate;
	AAS_u8 vibrato_depth;
	AAS_u8 vibrato_pos;
	AAS_u8 tremolo_rate;
	AAS_u8 tremolo_depth;
	AAS_u8 tremolo_pos;
	AAS_s8 trigger;
	AAS_u8 arpeggio_pos;
	AAS_u8 note;
}; // 28 bytes

static struct AAS_MOD_Channel AAS_mod_chan[AAS_MAX_CHANNELS] AAS_IN_EWRAM;

static AAS_s32 AAS_mod_timer AAS_IN_EWRAM;
static AAS_u32 AAS_mod_tempo AAS_IN_EWRAM;
static AAS_u8 AAS_mod_bpm AAS_IN_EWRAM;
static AAS_u8 AAS_mod_speed AAS_IN_EWRAM;
static AAS_BOOL AAS_mod_looped AAS_IN_EWRAM = AAS_FALSE;
static AAS_s16 AAS_mod_overall_volume AAS_IN_EWRAM = 256;
static AAS_u8 AAS_mod_loop_start AAS_IN_EWRAM = 0;
static AAS_s8 AAS_mod_loop_counter AAS_IN_EWRAM = 0;
static AAS_BOOL AAS_mod_loop AAS_IN_EWRAM = AAS_TRUE;
static AAS_u8 AAS_mod_last_filter_value AAS_IN_EWRAM = 0;
static AAS_u8 AAS_mod_num_chans AAS_IN_EWRAM = 0;
static AAS_BOOL AAS_mod_active_effects AAS_IN_EWRAM = AAS_FALSE;
static AAS_s8 AAS_mod_next_song_pos AAS_IN_EWRAM = -1;

int AAS_MOD_GetNumChannels()
{
	return AAS_mod_num_chans;
}

int AAS_MOD_GetLastFilterValue()
{
	return AAS_mod_last_filter_value;
}

int AAS_MOD_QueueSongPos( int song_pos )
{
	if ( AAS_mod_num >= 0 )
	{
		if ( (song_pos >= 0) && (song_pos < 128) )
		{
			if ( AAS_Sequence[AAS_mod_num][song_pos][0] == -1 )
			{
				return AAS_ERROR_INVALID_SONG_POS;
			}
			else
			{
				AAS_mod_next_song_pos = song_pos;
				
				return AAS_OK;
			}
		}
		else
		{
			return AAS_ERROR_INVALID_SONG_POS;
		}
	}
	else
	{
		return AAS_ERROR_NO_MOD_PLAYING;
	}
}

int AAS_MOD_SetSongPos( int song_pos )
{
	if ( AAS_mod_num >= 0 )
	{
		if ( (song_pos >= 0) && (song_pos < 128) )
		{
			if ( AAS_Sequence[AAS_mod_num][song_pos][0] == -1 )
			{
				return AAS_ERROR_INVALID_SONG_POS;
			}
			else
			{
				struct AAS_MOD_Channel* mod_chan = AAS_mod_chan;
				int chan;
				
				for( chan = 0; chan < AAS_mod_num_chans; ++chan )
				{
					mod_chan->pattern = (AAS_u32*)(AAS_PatternData + (((int)AAS_Sequence[AAS_mod_num][song_pos][chan])<<8));
					++mod_chan;
				}
				AAS_mod_line_num = 0;
				AAS_mod_song_pos = song_pos;
				
				return AAS_OK;
			}
		}
		else
		{
			return AAS_ERROR_INVALID_SONG_POS;
		}
	}
	else
	{
		return AAS_ERROR_NO_MOD_PLAYING;
	}
}

int AAS_MOD_GetSongPos()
{
	if ( AAS_mod_num >= 0 )
		return AAS_mod_song_pos;
	else
		return AAS_ERROR_NO_MOD_PLAYING;
}

int AAS_MOD_GetLineNum()
{
	if ( AAS_mod_num >= 0 )
		return AAS_mod_line_num;
	else
		return AAS_ERROR_NO_MOD_PLAYING;
}

int AAS_MOD_GetVolume()
{
	return AAS_mod_overall_volume;
}

int AAS_MOD_SetVolume( int vol )
{
	if ( (vol >= 0) && (vol <= 256) )
	{
		int i;
		
		for( i = AAS_mod_num_chans-1; i >= 0; --i )
			AAS_channels[AAS_chan_rearrange[i]].volume = (AAS_mod_chan[i].volume*vol)>>AAS_volscale;
		
		AAS_mod_overall_volume = vol;
		
		return AAS_OK;
	}
	else
	{
		return AAS_ERROR_VOLUME_OUT_OF_RANGE;
	}
}

void AAS_MOD_Pause()
{
	int i;
	
	AAS_mod_num_store = AAS_mod_num;
	AAS_mod_num = -1;
	
	for( i = 0; i < AAS_mod_num_chans; ++i )
	{
  	AAS_channels[AAS_chan_rearrange[i]].active = AAS_FALSE;
  }
}

void AAS_MOD_Resume()
{
	if ( AAS_mod_num_store != -2 )
	{
		struct AAS_Channel* ch;
		int i;
		
		for( i = 0; i < AAS_mod_num_chans; ++i )
		{
			ch = &AAS_channels[AAS_chan_rearrange[i]];
			
			if ( !ch->active )
			{
				if ( ch->loop_length )
				{
					ch->active = AAS_TRUE;
				}
				else
				{
					if ( ch->pos < ((ch->end - (ch->delta>>6)) - 1) )
					{
						ch->active = AAS_TRUE;
					}
				}
			}
		}
		
		AAS_mod_num = AAS_mod_num_store;
	}
}

void AAS_MOD_Stop()
{
	int i;
	struct AAS_Channel* ch;
	AAS_mod_num_store = -2;
	AAS_mod_num = -1;
	AAS_mod_next_song_pos = -1;
	
	for( i = 0; i < AAS_mod_num_chans; ++i )
	{
		ch = &AAS_channels[AAS_chan_rearrange[i]];
  	ch->active = AAS_FALSE;
  	ch->loop_length = 0;
	  ch->pos = 0;
	  ch->end = 0;
	  ch->delta = 0;
	  AAS_mod_chan[i].tremolo_pos = 0;
		AAS_mod_chan[i].vibrato_pos = 0;
	  AAS_mod_chan[i].effect = 0;
	  AAS_mod_chan[i].volume = 0;
	  AAS_mod_chan[i].trigger = -1;
	  AAS_mod_chan[i].note = 0;
	  AAS_mod_chan[i].slide_target_period = 0;
 	}
 	AAS_mod_active_effects = AAS_FALSE;
  AAS_mod_looped = AAS_FALSE;
  AAS_mod_loop_counter = 0;
  AAS_mod_loop_start = 0;
  AAS_mod_last_filter_value = 0;
  AAS_mod_num_chans = 0;
}

AAS_BOOL AAS_MOD_HasLooped()
{
	return AAS_mod_looped;
}

AAS_BOOL AAS_MOD_IsPlaying()
{
	return (AAS_mod_num>=0);
}

int AAS_MOD_Play( int song_num )
{
	AAS_MOD_Stop();
	
	if ( AAS_initialised )
	{
		if ( (song_num >= 0) && (song_num < AAS_DATA_NUM_MODS) && AAS_NumChans[song_num] )
		{
			int i;
			
			if ( AAS_volscale == 9 )
				i = 16;
			else if ( AAS_volscale == 8 )
				i = 8;
			else
				i = 4;
			
			if ( AAS_NumChans[song_num] > i )
			{
				return AAS_ERROR_NOT_ENOUGH_CHANNELS;
			}
			else
			{
				//AAS_mod_num = 0;
				AAS_mod_loop = AAS_TRUE;
				AAS_mod_num_store = -2;
				AAS_mod_song_pos = 0;
				AAS_mod_num_chans = AAS_NumChans[song_num];
				
				for( i = 0; i < AAS_mod_num_chans; ++i )
					AAS_mod_chan[i].pattern = (AAS_u32*)(AAS_PatternData + (((int)AAS_Sequence[song_num][0][i])<<8));
				
				AAS_mod_line_num = 0;
				AAS_mod_speed = 6;
				AAS_mod_bpm = 125;
				AAS_mod_tempo = AAS_DivTable[AAS_mod_speed]*AAS_mod_bpm;
				AAS_mod_timer = 0x7d0000 - AAS_mod_tempo;
				AAS_mod_num = song_num;
				return AAS_OK;
			}
		}
		else
		{
			return AAS_ERROR_MOD_DOES_NOT_EXIST;
		}
	}
	else
	{
		return AAS_ERROR_CALL_SET_CONFIG_FIRST;
	}
}

int AAS_MOD_SetLoop( AAS_BOOL loop )
{
	if ( AAS_initialised )
	{
		if ( AAS_mod_num >= 0  )
		{
			AAS_mod_loop = loop;
			return AAS_OK;
		}
		else
		{
			return AAS_ERROR_NO_MOD_PLAYING;
		}
	}
	else
	{
		return AAS_ERROR_CALL_SET_CONFIG_FIRST;
	}
}

void AAS_MOD_Interrupt()
{
	if ( AAS_mod_num >= 0 )
	{
		AAS_mod_timer += AAS_mod_tempo;
		
		if ( AAS_mod_timer < 0x7d0000 )
		{
			if ( AAS_mod_active_effects )
			{
				const AAS_u8* chan_rearrange = AAS_chan_rearrange;
				struct AAS_MOD_Channel* mod_chan = AAS_mod_chan;
				int chan, num_chans;
				
				num_chans = AAS_mod_num_chans;
				
				for( chan = 0; chan < num_chans; ++chan )
				{
					int effect;
					
					effect = mod_chan->effect;
					
					if ( effect )
					{
						int val, tmp, output;
						struct AAS_Channel* out_chan;
						
						tmp = chan_rearrange[chan];
						out_chan = &AAS_channels[tmp];
						output = tmp>>3;
						
						switch( effect>>8 )
						{
							case 0xe: // extended effects
								switch( effect&0xf0 )
								{
									case 0xc0: // note cut
										val = effect&0xf;
										--val;
										if ( val <= 0 )
										{
											mod_chan->effect = 0;
											out_chan->active = AAS_FALSE;
											
											AAS_changed[output] = AAS_TRUE;
										}
										else
										{
											mod_chan->effect = (effect&0xff0)+val;
										}
									break;
									
									case 0x90: // retrigger sample
									case 0xd0: // delay sample
										if ( mod_chan->trigger >= 0 )
										{
											--mod_chan->trigger;
											if ( mod_chan->trigger < 0 )
											{
												const struct AAS_ModSample* samp = &AAS_ModSamples[AAS_mod_num][mod_chan->samp_num];
												int repeat = samp->repeat;
												int length = samp->length;
												const AAS_s8* data = AAS_SampleData + samp->data;
												
												mod_chan->samp_start = data;
												out_chan->pos = data;
												out_chan->pos_fraction = 0;
												if ( repeat == 65535 )
													out_chan->loop_length = 0;
												else
													out_chan->loop_length = ((AAS_u32)(length - repeat))<<1;
												out_chan->end = data + (length<<1);
												out_chan->frequency = AAS_MOD_period_conv_table[mod_chan->period];
												out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
												out_chan->active = AAS_TRUE;
												
												AAS_changed[output] = AAS_TRUE;
											}
										}
									break;
									
									default:
									break;
								}
							break;
							
							case 0x6: // vibrato + volume slide
								val = mod_chan->vibrato_pos;
								val += mod_chan->vibrato_rate;
								if ( val >= 64 )
									val -= 64;
								mod_chan->vibrato_pos = val;
								tmp = mod_chan->period + (((AAS_sin[val]*mod_chan->vibrato_depth)+32)>>6);
								if ( tmp < 113 )
									tmp = 113;
								if ( tmp > 856 )
									tmp = 856;
								out_chan->frequency = AAS_MOD_period_conv_table[tmp];
								out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
							// Intentionally no "break;"
								
							case 0xa: // volume slide
								val = effect&0xf0;
								if ( val )
									val >>= 4;
								else
									val = -(effect&0xf);
								tmp = mod_chan->volume;
								tmp += val;
								if ( tmp > 64 )
									tmp = 64;
								else if ( tmp < 0 )
									tmp = 0;
								mod_chan->volume = tmp;
								out_chan->volume = (tmp*AAS_mod_overall_volume)>>AAS_volscale;
								
								AAS_changed[output] = AAS_TRUE;
							break;
							
							case 0x1: // slide up
								val = effect&0xff;
								tmp = mod_chan->period - val;
								if ( tmp < 113 )
									tmp = 113;
								mod_chan->period = tmp;
								out_chan->frequency = AAS_MOD_period_conv_table[tmp];
								out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
								
								AAS_changed[output] = AAS_TRUE;
							break;
							
							case 0x2: // slide down
								val = effect&0xff;
								tmp = mod_chan->period + val;
								if ( tmp > 856 )
									tmp = 856;
								mod_chan->period = tmp;
								out_chan->frequency = AAS_MOD_period_conv_table[tmp];
								out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
								
								AAS_changed[output] = AAS_TRUE;
							break;
							
							case 0x3: // tone portamento
								val = mod_chan->period;
								tmp = mod_chan->slide_target_period;
								if ( val < tmp )
								{
									val += mod_chan->slide_rate;
									if ( val >= tmp )
									{
										mod_chan->effect = 0;
										val = tmp;
									}
								}
								else if ( val > tmp )
								{
									val -= mod_chan->slide_rate;
									if ( val <= tmp )
									{
										mod_chan->effect = 0;
										val = tmp;
									}
								}
								else if ( val == tmp )
								{
									mod_chan->effect = 0;
								}
								mod_chan->period = val;
								out_chan->frequency = AAS_MOD_period_conv_table[val];
								out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
								
								AAS_changed[output] = AAS_TRUE;
							break;
							
							case 0x5: // tone portamento + volume slide
								val = mod_chan->period;
								tmp = mod_chan->slide_target_period;
								if ( val < tmp )
								{
									val += mod_chan->slide_rate;
									if ( val >= tmp )
										val = tmp;
								}
								else if ( val > tmp )
								{
									val -= mod_chan->slide_rate;
									if ( val <= tmp )
										val = tmp;
								}
								mod_chan->period = val;
								out_chan->frequency = AAS_MOD_period_conv_table[val];
								out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
								
								val = effect&0xf0;
								if ( val )
									val >>= 4;
								else
									val = -(effect&0xf);
								tmp = mod_chan->volume;
								tmp += val;
								if ( tmp > 64 )
									tmp = 64;
								else if ( tmp < 0 )
									tmp = 0;
								mod_chan->volume = tmp;
								out_chan->volume = (tmp*AAS_mod_overall_volume)>>AAS_volscale;
								
								AAS_changed[output] = AAS_TRUE;
							break;
							
							case 0x4: // vibrato
								val = mod_chan->vibrato_pos;
								val += mod_chan->vibrato_rate;
								if ( val >= 64 )
									val -= 64;
								mod_chan->vibrato_pos = val;
								tmp = mod_chan->period + (((AAS_sin[val]*mod_chan->vibrato_depth)+32)>>6);
								if ( tmp < 113 )
									tmp = 113;
								else if ( tmp > 856 )
									tmp = 856;
								out_chan->frequency = AAS_MOD_period_conv_table[tmp];
								out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
								
								AAS_changed[output] = AAS_TRUE;
							break;
							
							case 0x7: // tremolo
								val = mod_chan->tremolo_pos;
								val += mod_chan->tremolo_rate;
								if ( val >= 64 )
									val -= 64;
								mod_chan->tremolo_pos = val;
								tmp = mod_chan->volume + (((AAS_sin[val]*mod_chan->tremolo_depth)+32)>>6);
								if ( tmp < 0 )
									tmp = 0;
								else if ( tmp > 64 )
									tmp = 64;
								out_chan->volume = (tmp*AAS_mod_overall_volume)>>AAS_volscale;
								
								AAS_changed[output] = AAS_TRUE;
							break;
							
							case 0x0: // possible arpeggio
								tmp = effect & 0xff;
								if ( tmp ) // definite arpeggio
								{
									++mod_chan->arpeggio_pos;
									val = mod_chan->note;
									switch( mod_chan->arpeggio_pos )
									{
										case 0:
										break;
											
										case 1:
											val += tmp>>4;
										break;
											
										case 2:
											val += tmp&0xf;
										// Intentionally no "break;" to allow AAS_mod_arpeggio_pos[chan] to be restarted
											
										default:
											mod_chan->arpeggio_pos = 0;
										break;
									}
									
									if ( val )
									{
										out_chan->frequency = AAS_MOD_period_conv_table[AAS_period_table[AAS_ModSamples[AAS_mod_num][mod_chan->samp_num].finetune][val-1]];
										out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
										
										AAS_changed[output] = AAS_TRUE;
									}
								}
							break;
							
							default:
							break;
						}
					}
					
					++mod_chan;
				}
			}
		}
		else
		{
			const AAS_u8* chan_rearrange = AAS_chan_rearrange;
			struct AAS_MOD_Channel* mod_chan = AAS_mod_chan;
			int chan, num_chans;
			int samp_num, effect, period, speed, tmp;
			int jump_ahead = -1;
			int jump_song_pos = -1;
			AAS_BOOL active_effects = AAS_FALSE;
			const struct AAS_ModSample* mod_samp = AAS_ModSamples[AAS_mod_num];
			
			num_chans = AAS_mod_num_chans;
			
			AAS_mod_timer -= 0x7d0000;
		
			for( chan = 0; chan < num_chans; ++chan )
			{
				int output;
				struct AAS_Channel* out_chan;
				AAS_u32 dat;
				
				tmp = chan_rearrange[chan];
				out_chan = &AAS_channels[tmp];
				output = tmp>>3;
				
				// Tidy up after arpeggio
				effect = mod_chan->effect;
				if ( effect )
				{
					if ( effect < 0x100 )
					{
						tmp = mod_chan->note;
						if ( tmp )
						{
							out_chan->frequency = AAS_MOD_period_conv_table[AAS_period_table[mod_samp[mod_chan->samp_num].finetune][tmp-1]];
							out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
							
							AAS_changed[output] = AAS_TRUE;
						}
					}
				}
				
				dat = *mod_chan->pattern++;
				samp_num = (dat>>24)-1;
				period = (dat>>12)&0xfff;
				effect = dat&0xfff;
				
				if ( samp_num >= 0 )
				{
					mod_chan->samp_num = samp_num;
					mod_chan->volume = tmp = mod_samp[samp_num].volume;
					out_chan->volume = (tmp*AAS_mod_overall_volume)>>AAS_volscale;
					
					AAS_changed[output] = AAS_TRUE;
				}
				else
				{
					samp_num = mod_chan->samp_num;
				}
				
				if ( period )
				{
					if ( samp_num >= 0 )
					{
						const struct AAS_ModSample* samp = &mod_samp[samp_num];
							
						mod_chan->note = period;
						period = AAS_period_table[samp->finetune][period-1];
						
						if ( (effect > 0xed0) && (effect < 0xee0) ) // delay sample
						{
							mod_chan->period = period;
							mod_chan->trigger = (effect&0xf)-1;
						}
						else
						{
							tmp = effect>>8;
							
							if ( (tmp != 0x3) && (tmp != 0x5) )
							{
								int repeat = samp->repeat;
								int length = samp->length;
								const AAS_s8* data = AAS_SampleData + samp->data;
								
								mod_chan->samp_start = data;
								out_chan->pos = data;
								out_chan->pos_fraction = 0;
								mod_chan->period = period;
								if ( repeat == 65535 )
									out_chan->loop_length = 0;
								else
									out_chan->loop_length = ((AAS_u32)(length - repeat))<<1;
								out_chan->end = data + (length<<1);
								out_chan->frequency = AAS_MOD_period_conv_table[period];
								out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
								out_chan->active = AAS_TRUE;
								
								AAS_changed[output] = AAS_TRUE;
							}
						}
					}
				}
				
				if ( effect )
				{
					switch( effect>>8 )
					{
						case 0xf: // set speed
							speed = effect&0xff;
							if ( speed > 0 )
							{
								if ( speed > 31 )
									AAS_mod_bpm = speed;
								else
									AAS_mod_speed = speed;
								
								//AAS_mod_tempo = (((1<<15)*AAS_mod_bpm*24)/(3000*AAS_mod_speed))<<13;  // use LUT
								
								// approximately:
								//AAS_mod_tempo = AAS_DivTable[AAS_mod_speed]*AAS_mod_bpm*33;
								AAS_mod_tempo = AAS_DivTable[AAS_mod_speed]*AAS_mod_bpm;
							}
							effect = 0; // No need to process this effect again
						break;
						
						case 0xe: // extended effects
							switch( effect&0xf0 )
							{
								case 0x00: // set filter (used to send messages to code instead)
									AAS_mod_last_filter_value = effect&0xf;
									effect = 0; // No need to process this effect again
								break;
								
								case 0x10: // fine slide up
									tmp = mod_chan->period - (effect&0xf);
									if ( tmp < 113 )
										tmp = 113;
									mod_chan->period = tmp;
									out_chan->frequency = AAS_MOD_period_conv_table[tmp];
									out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
									effect = 0; // No need to process this effect again
									
									AAS_changed[output] = AAS_TRUE;
								break;
								
								case 0x20: // fine slide down
									tmp = mod_chan->period + (effect&0xf);
									if ( tmp > 856 )
										tmp = 856;
									mod_chan->period = tmp;
									out_chan->frequency = AAS_MOD_period_conv_table[tmp];
									out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
									effect = 0; // No need to process this effect again
									
									AAS_changed[output] = AAS_TRUE;
								break;
								
								case 0x60: // set/jump to loop
									tmp = effect & 0xf;
									if ( tmp )
									{
										if ( AAS_mod_loop_counter )
											--AAS_mod_loop_counter;
										else
											AAS_mod_loop_counter = tmp;
										
										if ( AAS_mod_loop_counter )
										{
											int i;
											struct AAS_MOD_Channel* mod_chan2 = AAS_mod_chan;
											
											for( i = 0; i < num_chans; ++i )
											{
												mod_chan2->pattern = (AAS_u32*)(AAS_PatternData + (((int)AAS_Sequence[AAS_mod_num][AAS_mod_song_pos][i])<<8));
												mod_chan2->pattern += AAS_mod_loop_start;
												++mod_chan2;
											}
											AAS_mod_line_num = AAS_mod_loop_start;
										}
									}
									else
									{
										AAS_mod_loop_start = AAS_mod_line_num;
									}
									effect = 0; // No need to process this effect again
								break;
								
								case 0x90: // retrigger sample
									mod_chan->trigger = (effect&0xf)-1;
								break;
								
								case 0xa0: // fine volume slide up
									tmp = mod_chan->volume;
									tmp += effect&0xf;
									if ( tmp > 64 )
										tmp = 64;
									mod_chan->volume = tmp;
									out_chan->volume = (tmp*AAS_mod_overall_volume)>>AAS_volscale;
									effect = 0; // No need to process this effect again
									
									AAS_changed[output] = AAS_TRUE;
								break;
								
								case 0xb0: // fine volume slide down
									tmp = mod_chan->volume;
									tmp -= effect&0xf;
									if ( tmp < 0 )
										tmp = 0;
									mod_chan->volume = tmp;
									out_chan->volume = (tmp*AAS_mod_overall_volume)>>AAS_volscale;
									effect = 0; // No need to process this effect again
									
									AAS_changed[output] = AAS_TRUE;
								break;
								
								case 0xc0: // note cut
									if ( (effect&0xf) == 0 )
									{
										effect = 0;
										out_chan->active = AAS_FALSE;
										AAS_changed[output] = AAS_TRUE;
									}
								break;
								
								case 0xe0: // pattern delay
									AAS_mod_timer -= (effect&0xf)*0x7d0000;
									effect = 0; // No need to process this effect again
								break;
								
								default:
								break;
							}
						break;
						
						case 0xd: // pattern break
							//jump_ahead = (((effect>>4)&0xf)*10)+(effect&0xf);
							jump_ahead = effect&0xff;
							effect = 0; // No need to process this effect again
						break;
						
						case 0xc: // set volume
							mod_chan->volume = tmp = effect&0xff;
							out_chan->volume = (tmp*AAS_mod_overall_volume)>>AAS_volscale;
							effect = 0; // No need to process this effect again
							
							AAS_changed[output] = AAS_TRUE;
						break;
						
						case 0xb: // position jump
							jump_song_pos = effect&0xff;
							effect = 0; // No need to process this effect again
						break;
						
						case 0x9: // set sample offset
							tmp = effect & 0xff;
							if ( tmp )
							{
								const AAS_s8* new_pos = mod_chan->samp_start + (tmp<<8);
								
								if ( new_pos >= out_chan->end )
								{
									out_chan->active = AAS_FALSE;
									
									AAS_changed[output] = AAS_TRUE;
								}
								else
									out_chan->pos = new_pos;
							}
							effect = 0; // No need to process this effect again
						break;
						
						case 0x7: // tremolo
							if ( effect & 0xf0 )
								mod_chan->tremolo_rate = (effect & 0xf0)>>4;
							if ( effect & 0xf )
								mod_chan->tremolo_depth = effect & 0xf;
							if ( effect & 0xff )
								mod_chan->tremolo_pos = 0;
						break;
						
						case 0x4: // vibrato
							if ( effect & 0xf0 )
								mod_chan->vibrato_rate = (effect & 0xf0)>>4;
							if ( effect & 0xf )
								mod_chan->vibrato_depth = effect & 0xf;
							if ( effect & 0xff )
								mod_chan->vibrato_pos = 0;
						break;
						
						case 0x3: // tone portamento
							tmp = effect & 0xff;
							if ( tmp )
								mod_chan->slide_rate = tmp;
							
							if ( period )
								mod_chan->slide_target_period = period;
							else if ( mod_chan->slide_target_period == 0 )
								effect = 0;
						break;
						
						case 0x0: // possible arpeggio
							tmp = effect & 0xff;
							if ( tmp ) // definite arpeggio
							{
								tmp = mod_chan->note;
								mod_chan->arpeggio_pos = 0;
								if ( tmp )
								{
									out_chan->frequency = AAS_MOD_period_conv_table[AAS_period_table[mod_samp[mod_chan->samp_num].finetune][tmp-1]];
									out_chan->delta = AAS_Min( 4095, ((out_chan->frequency*AAS_mix_scale)+32768)>>16 );
									
									AAS_changed[output] = AAS_TRUE;
								}
							}
						break;
						
						default:
							//printf_special( "effect:%p period:%d samp:%d\n", effect, period, samp_num );
						break;
					}
				}
				
				mod_chan->effect = effect;
				if ( effect )
					active_effects = AAS_TRUE;
				++mod_chan;
			}
			
			AAS_mod_active_effects = active_effects;
			
			if ( jump_ahead >= 0 )
			{
				AAS_mod_loop_start = 0;
				++AAS_mod_song_pos;
				if ( AAS_Sequence[AAS_mod_num][AAS_mod_song_pos][0] == -1 )
				{
					AAS_mod_looped = AAS_TRUE;
					AAS_mod_song_pos = 0;
					mod_chan = AAS_mod_chan;
					for( chan = num_chans; chan > 0; --chan )
					{
						mod_chan->slide_target_period = 0;
						++mod_chan;
					}
				}
				
				mod_chan = AAS_mod_chan;
				for( chan = 0; chan < num_chans; ++chan )
				{
					mod_chan->pattern = (AAS_u32*)(AAS_PatternData + (((int)AAS_Sequence[AAS_mod_num][AAS_mod_song_pos][chan])<<8));
					mod_chan->pattern += jump_ahead;
					++mod_chan;
				}
				AAS_mod_line_num = jump_ahead;
			}
			else if ( jump_song_pos >= 0 )
			{
				AAS_mod_loop_start = 0;
				if ( jump_song_pos < 128 )
				{
					AAS_mod_song_pos = jump_song_pos;
					if ( AAS_Sequence[AAS_mod_num][AAS_mod_song_pos][0] == -1 )
					{
						AAS_mod_looped = AAS_TRUE;
						mod_chan = AAS_mod_chan;
						for( chan = num_chans; chan > 0; --chan )
						{
							mod_chan->slide_target_period = 0;
							++mod_chan;
						}
						AAS_mod_song_pos = 0;
					}
				}
				else
				{
					AAS_mod_looped = AAS_TRUE;
					mod_chan = AAS_mod_chan;
					for( chan = num_chans; chan > 0; --chan )
					{
						mod_chan->slide_target_period = 0;
						++mod_chan;
					}
					AAS_mod_song_pos = 0;
				}
				
				mod_chan = AAS_mod_chan;
				for( chan = 0; chan < num_chans; ++chan )
				{
					mod_chan->pattern = (AAS_u32*)(AAS_PatternData + (((int)AAS_Sequence[AAS_mod_num][AAS_mod_song_pos][chan])<<8));
					++mod_chan;
				}
				AAS_mod_line_num = 0;
			}
			else
			{
				++AAS_mod_line_num;
				if ( AAS_mod_line_num > 63 )
				{
					AAS_mod_line_num = 0;
					AAS_mod_loop_start = 0;
					
					if ( AAS_mod_next_song_pos == -1 )
					{
						++AAS_mod_song_pos;
					}
					else
					{
						AAS_mod_song_pos = AAS_mod_next_song_pos;
						AAS_mod_next_song_pos = -1;
					}
					
					if ( AAS_Sequence[AAS_mod_num][AAS_mod_song_pos][0] == -1 )
					{						
						AAS_mod_looped = AAS_TRUE;
						mod_chan = AAS_mod_chan;
						for( chan = num_chans; chan > 0; --chan )
						{
							mod_chan->slide_target_period = 0;
							++mod_chan;
						}
						AAS_mod_song_pos = AAS_RestartPos[AAS_mod_num]; 
					}
					
					mod_chan = AAS_mod_chan;
					for( chan = 0; chan < num_chans; ++chan )
					{
						mod_chan->pattern = (AAS_u32*)(AAS_PatternData + (((int)AAS_Sequence[AAS_mod_num][AAS_mod_song_pos][chan])<<8));
						++mod_chan;
					}
				}
			}
			
			if ( AAS_mod_looped )
				if ( !AAS_mod_loop )
					AAS_MOD_Stop();
		}
	}
}
