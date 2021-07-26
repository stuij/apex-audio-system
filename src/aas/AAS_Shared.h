// Copyright (c) 2003-2021 James Daniels
// Distributed under the MIT License
// license terms: see LICENSE file in root or http://opensource.org/licenses/MIT

#ifndef __AAS_SHARED__
#define __AAS_SHARED__

#include "AAS.h"
#include "AAS_Mixer.h"

#define AAS_MAX_CHANNELS 16

#define REG_SOUNDCNT_L (*(volatile AAS_u16 *)0x4000080)
#define REG_SOUNDCNT_H (*(volatile AAS_u16 *)0x4000082)
#define REG_SOUNDCNT_X (*(volatile AAS_u16 *)0x4000084)

#define REG_DMA1SAD    (*(volatile AAS_u32 *)0x40000BC)
#define REG_DMA1DAD    (*(volatile AAS_u32 *)0x40000C0)
#define REG_DMA1CNT_H  (*(volatile AAS_u16 *)0x40000C6)
#define REG_DMA1CNT    (*(volatile AAS_u32 *)0x40000C4)

#define REG_DMA2SAD    (*(volatile AAS_u32 *)0x40000C8)
#define REG_DMA2DAD    (*(volatile AAS_u32 *)0x40000CC)
#define REG_DMA2CNT_H  (*(volatile AAS_u16 *)0x40000D2)
#define REG_DMA2CNT    (*(volatile AAS_u32 *)0x40000d0)

#define REG_TM0CNT     (*(volatile AAS_u16 *)0x4000102)
#define REG_TM0D       (*(volatile AAS_u16 *)0x4000100)

#define REG_TM1CNT     (*(volatile AAS_u16 *)0x4000106)
#define REG_TM1D       (*(volatile AAS_u16 *)0x4000104)

#define REG_IE         (*(volatile AAS_u16 *)0x4000200)
#define REG_IME        (*(volatile AAS_u16 *)0x4000208)
#define REG_IF         (*(volatile AAS_u16 *)0x4000202)

#define	DISPCNT		   (*(volatile AAS_u16 *)0x4000000)
#define REG_VCOUNT     (*(volatile AAS_u16 *)0x4000006)

struct AAS_ModSample
{
  AAS_u32 data;   // offset in bytes
  AAS_u16 repeat; // in halfwords
  AAS_u16 length; // in halfwords
  AAS_u8 finetune;
  AAS_u8 volume;
  AAS_u16 padding;
};

extern const AAS_u16 AAS_DATA_NUM_MODS;
extern const struct AAS_ModSample AAS_ModSamples[][31];
extern const AAS_s16 AAS_Sequence[][128][16];
extern const AAS_u8 AAS_NumChans[];
extern const AAS_u8 AAS_RestartPos[];
extern const AAS_u8 AAS_PatternData[];
extern const AAS_s8 AAS_SampleData[];

extern const AAS_u16 AAS_DivTable[8192];
extern const AAS_u16 AAS_MOD_period_conv_table[2048];
extern const AAS_u16 AAS_period_table[16][60];

extern const AAS_u16 AAS_tick_rate[41];

extern const AAS_s8 AAS_sin[64];

extern AAS_s16 AAS_mod_num AAS_IN_EWRAM;
extern AAS_BOOL AAS_initialised AAS_IN_EWRAM;
extern AAS_u8 AAS_volscale AAS_IN_EWRAM;
extern AAS_u8 AAS_mod_num_chans AAS_IN_EWRAM;
extern AAS_u16 AAS_mix_scale AAS_IN_EWRAM;
extern AAS_BOOL AAS_changed[2] AAS_IN_IWRAM;

extern AAS_u8 AAS_chan_rearrange[AAS_MAX_CHANNELS] AAS_IN_EWRAM;

__inline static int AAS_Min(int a, int b) {
  if (a < b)
    return a;
  else
    return b;
}

void AAS_MOD_Interrupt();

// fns defined in assembly
void AAS_MixAudio_SetMode_Normal();
void AAS_MixAudio_SetMode_Boost();
void AAS_MixAudio_SetMode_BoostAndClip();

#endif
