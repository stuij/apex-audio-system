// Copyright (c) 2003-2021 James Daniels
// Distributed under the MIT License
// license terms: see LICENSE file in root or http://opensource.org/licenses/MIT

// Notes:
//  + Max number of MODs = 256
//  + Max total size of all samples = 33,554,432 bytes
//  + Max total size of all patterns = 33,554,432 bytes
//  + All functions declared static to work around problem with GCC

#include <dirent.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define BYTE int8_t
#define UBYTE uint8_t
#define BOOL int8_t
#define WORD int16_t
#define UWORD uint16_t
#define ULONG uint32_t
#define TRUE 1
#define FALSE 0

__inline static int Min(int a, int b) {
  if (a < b)
    return a;
  else
    return b;
}

__inline static UWORD SwapUWORD(UWORD a) {
  return (a >> 8) + ((a & 0xff) << 8);
}

static char GetHex(int val) {
  const char dat[] = "0123456789ABCDEF";

  if ((val >= 0) && (val <= 15)) {
    return dat[val];
  } else {
    return '?';
  }

  /*
  switch (val) {
    case 0: return '0';
    case 1: return '1';
    case 2: return '2';
    case 3: return '3';
    case 4: return '4';
    case 5: return '5';
    case 6: return '6';
    case 7: return '7';
    case 8: return '8';
    case 9: return '9';
    case 10: return 'A';
    case 11: return 'B';
    case 12: return 'C';
    case 13: return 'D';
    case 14: return 'E';
    case 15: return 'F';
    default: return '?';
  }
  */
}

static int memcmp_new(UBYTE *a, UBYTE *b, int length) {
  for (; length > 0; --length)
    if (*a++ != *b++)
      return 1;

  return 0;
}

static void memcpy_new(UBYTE *dest, UBYTE *source, int length) {
  for (; length > 0; --length)
    *dest++ = *source++;
}

#define MAX_DATA_LENGTH 33554432
typedef struct data_pack {
  const char *chunk_name;
  int chunk_data_length;
  int chunk_unique;
  int chunk_actual;
  unsigned char chunk_data[MAX_DATA_LENGTH];
} DATAPACK;

static DATAPACK *DATA_Create(const char *name) {
  DATAPACK *new_data = (DATAPACK *)malloc(sizeof(DATAPACK));

  if (new_data) {
    // printf( "DATA_Create( %s ): addr:%p\n", name, new_data );

    new_data->chunk_name = name;
    new_data->chunk_data_length = 0;
    new_data->chunk_unique = 0;
    new_data->chunk_actual = 0;
  } else {
    printf("DATA_Create( %s ): Failed (address:%p length:%)...\n", name,
           new_data, sizeof(DATAPACK));
  }

  return new_data;
}

static int DATA_AppendAligned(DATAPACK *curr_data, unsigned char *data,
                              int length, int block_length) {
  ++curr_data->chunk_actual;

  if ((block_length * ((int)(length / block_length))) != length) {
    printf("DATA_AppendAligned( %p, %p, %d, %d ): Warning length not exactly "
           "divisible by block_length!\n",
           curr_data, data, length, block_length);
    return 0;
  } else if (length == 0) {
    return 0;
  } else {
    int offset = 0;

    while ((offset + block_length) <= curr_data->chunk_data_length) {
      if (memcmp(data, curr_data->chunk_data + offset, length) == 0) {
        return offset / block_length;
      }

      offset += block_length;
    };

    if (curr_data->chunk_data_length <= (MAX_DATA_LENGTH - length)) {
      memmove(curr_data->chunk_data + offset, data, length);
      curr_data->chunk_data_length += length;
      ++curr_data->chunk_unique;
      return offset / block_length;
    } else {
      --curr_data->chunk_actual;
      printf("DATA_AppendAligned( %p, %p, %d, %d ): Data >%d bytes!\n",
             curr_data, data, length, block_length, MAX_DATA_LENGTH);
      return 0;
    }
  }
}

static int DATA_Append(DATAPACK *curr_data, unsigned char *data, int length) {
  ++curr_data->chunk_actual;

  if (length == 0) {
    return 0;
  } else {
    int offset = 0;

    while ((offset + length) <= curr_data->chunk_data_length) {
      if (memcmp_new(data, curr_data->chunk_data + offset, length) == 0)
        return offset;
      ++offset;
    };

    if (curr_data->chunk_data_length <= (MAX_DATA_LENGTH - length)) {
      offset = curr_data->chunk_data_length;
      memmove(curr_data->chunk_data + offset, data, length);
      curr_data->chunk_data_length += length;
      ++curr_data->chunk_unique;
      return offset;
    } else {
      --curr_data->chunk_actual;
      printf("DATA_Append( %p, %d ): Data >%d bytes!\n", data, length,
             MAX_DATA_LENGTH);
      return 0;
    }
  }
}

static void DATA_Write(DATAPACK *curr_data, FILE *out_file) {
  int i, len;
  UBYTE val;

  len = curr_data->chunk_data_length;
  if (len & 0x3) {
    len &= ~0x3;
    len += 0x4;
  }

  printf("Writing %s (Unique:%d Actual:%d Length:%d)...", curr_data->chunk_name,
         curr_data->chunk_unique, curr_data->chunk_actual, len);

  fprintf(out_file, "\n.ALIGN\n.GLOBAL %s\n%s:\n.byte", curr_data->chunk_name,
          curr_data->chunk_name);

  if (len > 0) {
    for (i = 0; i < len; ++i) {
      if (i >= curr_data->chunk_data_length)
        val = 0;
      else
        val = *(curr_data->chunk_data + i);
      if (i == 0)
        fprintf(out_file, " %d", val);
      else
        fprintf(out_file, ", %d", val);
    }
    fprintf(out_file, "\n");
  } else {
    fprintf(out_file, " 0\n");
  }

  printf("Done!\n");

  free(curr_data);
}

struct ModSampleTemp {
  UWORD repeat;  // in halfwords
  UWORD length;  // in halfwords
  UWORD truelen; // in halfwords
  UBYTE volume;
  UBYTE finetune;
};

struct ModSample {
  ULONG data;   // offset in bytes
  UWORD repeat; // in halfwords
  UWORD length; // in halfwords
  UBYTE finetune;
  UBYTE volume;
};

static void MISC_ProcessSound(BYTE *data, int length) {
  for (; length > 0; --length) {
    int val = *data;

    if (val == -128)
      val = -127;

    *data++ = val;
  }
}

static void MISC_ConvSoundToSigned(UBYTE *data, int length) {
  BYTE *bdata = (BYTE *)data;
  for (; length > 0; --length) {
    int val_s = *data;
    int val_u = val_s - 128;
    *bdata++ = val_u;
  }
}

static void MOD_LoadSound(struct ModSample *smp, FILE *in_file,
                          DATAPACK *samples, int length, int truelen,
                          int repeat) {
  BYTE *samp = (BYTE *)malloc((length << 1) + 16);
  int data;

  fread(samp + 16, (length << 1), 1, in_file);

  if (truelen > 0) {
    memset(samp, 0, 16);
    if ((repeat < 65535) && (truelen > repeat))
      memmove(samp + (repeat << 1), samp + (truelen << 1),
              16); // should really merge, also wasteful of memory
    MISC_ProcessSound(samp, (truelen << 1) + 16);
    data = DATA_Append(samples, samp, (truelen << 1) + 16);
    smp->data = data + 16;
  } else {
    smp->data = 0;
  }
  smp->length = truelen;

  free(samp);
}

#define MAX_MODS 256

static struct ModSample samps[MAX_MODS][31];
static WORD patts[MAX_MODS][128][16]; // [mod][pattern][channel]
static UBYTE mod_num_chans[MAX_MODS];
static UBYTE mod_song_restart_pos[MAX_MODS];
static int max_song_length = 0;

static const char *effect_name[32] = {"*0: Arpeggio",
                                      "*1: Slide Up",
                                      "*2: Slide Down",
                                      "*3: Tone Portamento",
                                      "*4: Vibrato",
                                      "*5: Tone Portamento + Volume Slide",
                                      "*6: Vibrato + Volume Slide",
                                      "*7: Tremolo",
                                      "8: Set Panning Position",
                                      "*9: Set Sample Offset",
                                      "*A: Volume Slide",
                                      "*B: Position Jump",
                                      "*C: Set Volume",
                                      "*D: Pattern Break",
                                      "*F: Set Speed (BPM)",
                                      "*F: Set Speed (Ticks)",
                                      "*E0: Set Filter",
                                      "*E1: Fine Slide Up",
                                      "*E2: Fine Slide Down",
                                      "E3: Glissando Control",
                                      "E4: Set Vibrato Waveform",
                                      "E5: Set FineTune",
                                      "*E6: Set/Jump to Loop",
                                      "E7: Set Tremolo Waveform",
                                      "E8: Illegal",
                                      "*E9: Retrigger Note",
                                      "*EA: Fine Volume Slide Up",
                                      "*EB: Fine Volume Slide Down",
                                      "*EC: Note Cut",
                                      "*ED: Note Delay",
                                      "*EE: Pattern Delay",
                                      "EF: Invert Loop"};

static int MOD_FindNote(int period) {
  const int period_table[61] = {
      0,   1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960, 906,
      856, 808,  762,  720,  678,  640,  604,  570,  538,  508,  480,  453, 428,
      404, 381,  360,  339,  320,  302,  285,  269,  254,  240,  226,  214, 202,
      190, 180,  170,  160,  151,  143,  135,  127,  120,  113,  107,  101, 95,
      90,  85,   80,   75,   71,   67,   63,   60,   56};
  int i, d, r;
  int closest = 1000000000;

  r = -1;

  for (i = 0; i < 61; ++i) {
    d = abs(period_table[i] - period);
    if (d < closest) {
      r = i;
      closest = d;
    }
  }

  if (r == -1) {
    printf("Can't find note with period:%d...\n", period);
  }

  return r;
}

#define FOUR_CHAR_CODE(ch0, ch1, ch2, ch3)                                     \
  ((long)(unsigned char)(ch0) | ((long)(unsigned char)(ch1) << 8) |            \
   ((long)(unsigned char)(ch2) << 16) | ((long)(unsigned char)(ch3) << 24))

static int MOD_GetNumChans(ULONG file_format) {
  switch (file_format) {
  case FOUR_CHAR_CODE('T', 'D', 'Z', '1'):
    return 1;
    break;

  case FOUR_CHAR_CODE('2', 'C', 'H', 'N'):
  case FOUR_CHAR_CODE('T', 'D', 'Z', '2'):
    return 2;
    break;

  case FOUR_CHAR_CODE('T', 'D', 'Z', '3'):
    return 3;
    break;

  case FOUR_CHAR_CODE('M', '.', 'K', '.'):
  case FOUR_CHAR_CODE('F', 'L', 'T', '4'):
  case FOUR_CHAR_CODE('M', '!', 'K', '!'):
    return 4;
    break;

  case FOUR_CHAR_CODE('5', 'C', 'H', 'N'):
    return 5;
    break;

  case FOUR_CHAR_CODE('6', 'C', 'H', 'N'):
    return 6;
    break;

  case FOUR_CHAR_CODE('7', 'C', 'H', 'N'):
    return 7;
    break;

  case FOUR_CHAR_CODE('8', 'C', 'H', 'N'):
  case FOUR_CHAR_CODE('O', 'C', 'T', 'A'):
  case FOUR_CHAR_CODE('C', 'D', '8', '1'):
    return 8;
    break;

  case FOUR_CHAR_CODE('F', 'L', 'T', '8'):
    return -2; // Unsupported MOD type
    break;

  case FOUR_CHAR_CODE('9', 'C', 'H', 'N'):
    return 9;
    break;

  case FOUR_CHAR_CODE('1', '0', 'C', 'H'):
    return 10;
    break;

  case FOUR_CHAR_CODE('1', '1', 'C', 'H'):
    return 11;
    break;

  case FOUR_CHAR_CODE('1', '2', 'C', 'H'):
    return 12;
    break;

  case FOUR_CHAR_CODE('1', '3', 'C', 'H'):
    return 13;
    break;

  case FOUR_CHAR_CODE('1', '4', 'C', 'H'):
    return 14;
    break;

  case FOUR_CHAR_CODE('1', '5', 'C', 'H'):
    return 15;
    break;

  case FOUR_CHAR_CODE('1', '6', 'C', 'H'):
    return 16;
    break;

  case FOUR_CHAR_CODE('1', '7', 'C', 'H'):
  case FOUR_CHAR_CODE('1', '8', 'C', 'H'):
  case FOUR_CHAR_CODE('1', '9', 'C', 'H'):
  case FOUR_CHAR_CODE('2', '0', 'C', 'H'):
  case FOUR_CHAR_CODE('2', '1', 'C', 'H'):
  case FOUR_CHAR_CODE('2', '2', 'C', 'H'):
  case FOUR_CHAR_CODE('2', '3', 'C', 'H'):
  case FOUR_CHAR_CODE('2', '4', 'C', 'H'):
  case FOUR_CHAR_CODE('2', '5', 'C', 'H'):
  case FOUR_CHAR_CODE('2', '6', 'C', 'H'):
  case FOUR_CHAR_CODE('2', '7', 'C', 'H'):
  case FOUR_CHAR_CODE('2', '8', 'C', 'H'):
  case FOUR_CHAR_CODE('2', '9', 'C', 'H'):
  case FOUR_CHAR_CODE('3', '0', 'C', 'H'):
  case FOUR_CHAR_CODE('3', '1', 'C', 'H'):
  case FOUR_CHAR_CODE('3', '2', 'C', 'H'):
  case FOUR_CHAR_CODE('3', '3', 'C', 'H'):
  case FOUR_CHAR_CODE('3', '4', 'C', 'H'):
  case FOUR_CHAR_CODE('3', '5', 'C', 'H'):
  case FOUR_CHAR_CODE('3', '6', 'C', 'H'):
  case FOUR_CHAR_CODE('3', '7', 'C', 'H'):
  case FOUR_CHAR_CODE('3', '8', 'C', 'H'):
  case FOUR_CHAR_CODE('3', '9', 'C', 'H'):
  case FOUR_CHAR_CODE('4', '0', 'C', 'H'):
  case FOUR_CHAR_CODE('4', '1', 'C', 'H'):
  case FOUR_CHAR_CODE('4', '2', 'C', 'H'):
  case FOUR_CHAR_CODE('4', '3', 'C', 'H'):
  case FOUR_CHAR_CODE('4', '4', 'C', 'H'):
  case FOUR_CHAR_CODE('4', '5', 'C', 'H'):
  case FOUR_CHAR_CODE('4', '6', 'C', 'H'):
  case FOUR_CHAR_CODE('4', '7', 'C', 'H'):
  case FOUR_CHAR_CODE('4', '8', 'C', 'H'):
  case FOUR_CHAR_CODE('4', '9', 'C', 'H'):
  case FOUR_CHAR_CODE('5', '0', 'C', 'H'):
  case FOUR_CHAR_CODE('5', '1', 'C', 'H'):
  case FOUR_CHAR_CODE('5', '2', 'C', 'H'):
  case FOUR_CHAR_CODE('5', '3', 'C', 'H'):
  case FOUR_CHAR_CODE('5', '4', 'C', 'H'):
  case FOUR_CHAR_CODE('5', '5', 'C', 'H'):
  case FOUR_CHAR_CODE('5', '6', 'C', 'H'):
  case FOUR_CHAR_CODE('5', '7', 'C', 'H'):
  case FOUR_CHAR_CODE('5', '8', 'C', 'H'):
  case FOUR_CHAR_CODE('5', '9', 'C', 'H'):
  case FOUR_CHAR_CODE('6', '0', 'C', 'H'):
  case FOUR_CHAR_CODE('6', '1', 'C', 'H'):
  case FOUR_CHAR_CODE('6', '2', 'C', 'H'):
  case FOUR_CHAR_CODE('6', '3', 'C', 'H'):
  case FOUR_CHAR_CODE('6', '4', 'C', 'H'):
    return -3; // Too many channels
    break;

  default:
    return -1; // Unrecognised MOD type
    break;
  }

  // 'FLT4', 'FLT8': Startrekker 4/8 channel file. ('FLT6' doesn't exist)
  // 'CD81'        : Falcon 8 channel MODs
  // '2CHN'        : FastTracker 2 Channel MODs
  // 'yyCH' where yy can be 10, 12, .. 30, 32: FastTracker yy Channel MODs
  // 'yyCH' where yy can be 11, 13, 15: TakeTracker 11, 13, 15 channel MODs
  // 'TDZx' where x can be 1, 2 or 3: TakeTracker 1, 2, 3 channel MODs
  // ModPlug Tracker saves 33-64 channel MODs as '33CH' to '64CH'
}

static void MOD_ConvMod(FILE *out_file, DATAPACK *samples, DATAPACK *patterns,
                        const char *filename, int mod_num) {
  FILE *in_file;

  mod_num_chans[mod_num] = 0;

  in_file = fopen(filename, "rb");
  if (in_file) {
    struct ModSampleTemp samps_temp[32];
    char temp[23];
    int i, tmp, line, chan;
    UBYTE song_length, last_pattern;
    UBYTE song_data[128];
    int pat[64][16];
    UWORD tmp_uword;
    UBYTE tmp_ubyte;
    ULONG notes_temp[16][64];
    int effect_count[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    BOOL first;
    ULONG new_temp;
    int num_chans;
    UBYTE song_restart_pos;

    // printf( "\nMOD_ConvMod( %s ): Reading...\n", filename );

    fread(temp, 20, 1, in_file); // mod name

    for (i = 1; i <= 31; ++i) {
      fread(temp, 22, 1, in_file); // sample name

      fread(&tmp_uword, 2, 1, in_file); // sample length
      samps_temp[i].length = SwapUWORD(tmp_uword);

      fread(&tmp_ubyte, 1, 1, in_file); // finetune
      // if ( tmp_ubyte & 8 )
      //	tmp_ubyte -= 8;
      // else
      //	tmp_ubyte += 8;
      samps_temp[i].finetune = tmp_ubyte;
      if (tmp_ubyte > 15)
        printf("MOD_ConvMod( %s ): Error: finetune %u  > 15...\n",
               filename, tmp_ubyte);

      fread(&tmp_ubyte, 1, 1, in_file); // volume
      samps_temp[i].volume = tmp_ubyte;

      if (tmp_ubyte > 64) {
        printf("MOD_ConvMod( %s ): Error: sample vol %u > 64\n",
               filename, tmp_ubyte);
        return;
      }

      fread(&tmp_uword, 2, 1, in_file);
      samps_temp[i].repeat = SwapUWORD(tmp_uword);

      fread(&tmp_uword, 2, 1, in_file);
      tmp = SwapUWORD(tmp_uword);

      // samps_temp[i].truelen = samps_temp[i].length;

      if (tmp <= 1) {
        samps_temp[i].repeat = 65535;
        samps_temp[i].truelen = samps_temp[i].length;
      } else {
        samps_temp[i].truelen =
            Min(samps_temp[i].repeat + tmp, samps_temp[i].length);
      }

      // printf( "i:%d len:%d vol:%d rep:%d fine:%d\n", i,
      // samps_temp[i].truelen, samps_temp[i].volume, samps_temp[i].repeat,
      // samps_temp[i].finetune );
    }

    fread(&song_length, 1, 1, in_file);
    fread(&song_restart_pos, 1, 1, in_file);

    if ((song_length < 1) || (song_length > 128))
      printf("MOD_ConvMod( %s ): song_length:%d\n", filename, song_length);

    last_pattern = 0;
    for (i = 0; i <= 127; ++i) {
      fread(&tmp_ubyte, 1, 1, in_file);
      song_data[i] = tmp_ubyte;
      if (tmp_ubyte > last_pattern)
        last_pattern = tmp_ubyte;
    }

    if (last_pattern > 63)
      printf("MOD_ConvMod( %s ): last_pattern:%d\n", filename, last_pattern);

    fread(&new_temp, 4, 1, in_file);
    num_chans = MOD_GetNumChans(new_temp);
    if (num_chans <= 0) {
      switch (num_chans) {
      case -2: // Unsupported MOD type
        printf("Failed! Reason: Unsupported MOD type (%u)\n", new_temp);
        break;

      case -3: // Too many channels
        printf("Failed! Reason: Too many channels\n");
        break;

      default: // Unrecognised MOD type
        printf("Failed! Reason: Unrecognised MOD type (%u)\n", new_temp);
        break;
      }
      return;
    } else {
      if (num_chans == 1)
        printf("%d channel...", num_chans);
      else
        printf("%d channels...", num_chans);

      mod_num_chans[mod_num] = num_chans;
    }

    for (i = 0; i <= last_pattern; ++i) {
      for (line = 0; line < 64; ++line) {
        for (chan = 0; chan < num_chans; ++chan) {
          UBYTE byte1, byte2, byte3, byte4;
          int samp_num, period, effect;

          fread(&byte1, 1, 1, in_file);
          fread(&byte2, 1, 1, in_file);
          fread(&byte3, 1, 1, in_file);
          fread(&byte4, 1, 1, in_file);

          samp_num = (byte1 & 0xf0) + (byte3 >> 4);
          period = (((int)(byte1 & 0xf)) << 8) + ((int)byte2);
          effect = (((int)(byte3 & 0xf)) << 8) + byte4;
          if (effect != 0) {
            if ((effect >> 8) == 0xe)
              ++effect_count[((effect & 0xf0) >> 4) + 16];
            else {
              if ((effect >> 8) == 0xf) {
                if ((effect & 0xff) > 31)
                  ++effect_count[14];
                else
                  ++effect_count[15];
              } else {
                if ((effect >> 8) == 0xc)
                  if ((effect & 0xff) > 64)
                    effect = 0xc40;
                if ((effect >> 8) == 0xd) // pattern break
                  effect = (effect & 0xf00) + (((effect >> 4) & 0xf) * 10) +
                           (effect & 0xf);
                ++effect_count[effect >> 8];
              }
            }
          }
          notes_temp[chan][line] =
              (samp_num << 24) + (MOD_FindNote(period) << 12) + effect;
        }
      }

      for (chan = 0; chan < num_chans; ++chan) {
        pat[i][chan] = DATA_AppendAligned(
            patterns, (UBYTE *)(&notes_temp[chan][0]), 256, 256);
      }
    }

    for (i = 0; i <= 127; ++i) {
      if (i >= song_length) {
        for (chan = 0; chan < num_chans; ++chan)
          patts[mod_num][i][chan] = -1;
      } else {
        for (chan = 0; chan < num_chans; ++chan)
          patts[mod_num][i][chan] = pat[song_data[i]][chan];
      }
    }

    if (song_length > max_song_length)
      max_song_length = song_length;

    if (song_restart_pos < 127) {
      if (patts[mod_num][song_restart_pos][0] == -1)
        song_restart_pos = 0;
    } else {
      song_restart_pos = 0;
    }

    mod_song_restart_pos[mod_num] = song_restart_pos;

    for (i = 1; i <= 31; ++i) {
      if (samps_temp[i].length > 0) {
        MOD_LoadSound(&samps[mod_num][i - 1], in_file, samples,
                      samps_temp[i].length, samps_temp[i].truelen,
                      samps_temp[i].repeat);
        samps[mod_num][i - 1].volume = samps_temp[i].volume;
        samps[mod_num][i - 1].finetune = samps_temp[i].finetune;
        samps[mod_num][i - 1].repeat = samps_temp[i].repeat;
      }
    }

    printf("Done!\n");

    first = TRUE;
    for (i = 0; i <= 31; ++i)
      if (effect_count[i] > 0)
        if (*(effect_name[i]) != '*') {
          if (first) {
            printf("Unsupported effects:\n");
            first = FALSE;
          }
          printf("%s (%d)\n", effect_name[i], effect_count[i]);
        }

    printf("song_length:%d last_pattern:%d\n\n", song_length, last_pattern);

    fclose(in_file);
  } else {
    printf("MOD_ConvMod( %s ): Unable to open file for reading...\n", filename);
  }
}

static void MOD_WriteSamples(FILE *out_file, int num_mods) {
  int i, s;

  fprintf(out_file, "\n.ALIGN\n.GLOBAL AAS_ModSamples\nAAS_ModSamples:");

  for (i = 0; i < num_mods; ++i) {
    fprintf(out_file, "\n.word");
    for (s = 0; s < 31; ++s) {
      if (s == 0)
        fprintf(out_file, " %u, %u, %u", samps[i][s].data,
                (samps[i][s].length << 16) + samps[i][s].repeat,
                ((samps[i][s].volume & 0xff) << 8) +
                    (samps[i][s].finetune & 0xff));
      else
        fprintf(out_file, ", %u, %u, %u", samps[i][s].data,
                (samps[i][s].length << 16) + samps[i][s].repeat,
                ((samps[i][s].volume & 0xff) << 8) +
                    (samps[i][s].finetune & 0xff));
    }
  }

  fprintf(out_file, "\n");
}

static void MOD_WritePatterns(FILE *out_file, int num_mods) {
  int mod_num, pattern_num, line_num, channel_num;
  BOOL first;

  fprintf(out_file, "\n.ALIGN\n.GLOBAL AAS_Sequence\nAAS_Sequence:");

  for (mod_num = 0; mod_num < num_mods; ++mod_num) {
    fprintf(out_file, "\n.short");
    first = TRUE;
    for (pattern_num = 0; pattern_num < 128; ++pattern_num) {
      for (channel_num = 0; channel_num < 16; ++channel_num) {
        if (first) {
          first = FALSE;
          fprintf(out_file, " %d", patts[mod_num][pattern_num][channel_num]);
        } else {
          fprintf(out_file, ", %d", patts[mod_num][pattern_num][channel_num]);
        }
      }
    }
  }

  fprintf(out_file, "\n");
}

static void MOD_WriteNumChans(FILE *out_file, int num_mods) {
  int mod_num;

  fprintf(out_file, "\n.ALIGN\n.GLOBAL AAS_NumChans\nAAS_NumChans:\n.byte");

  for (mod_num = 0; mod_num < num_mods; ++mod_num) {
    if (mod_num)
      fprintf(out_file, ", %d", mod_num_chans[mod_num]);
    else
      fprintf(out_file, " %d", mod_num_chans[mod_num]);
  }

  fprintf(out_file, "\n");
}

static void MOD_WriteSongRestartPos(FILE *out_file, int num_mods) {
  int mod_num;

  fprintf(out_file, "\n.ALIGN\n.GLOBAL AAS_RestartPos\nAAS_RestartPos:\n.byte");

  for (mod_num = 0; mod_num < num_mods; ++mod_num) {
    if (mod_num)
      fprintf(out_file, ", %d", mod_song_restart_pos[mod_num]);
    else
      fprintf(out_file, " %d", mod_song_restart_pos[mod_num]);
  }

  fprintf(out_file, "\n");
}

/*
static void MOD_WritePeriodConvTable( FILE* out_file, int mix_rate )
{
        int i, freq;

        fprintf( out_file, "\nconst UWORD AAS_MOD_period_conv_table[2048] = {
65535" );

        for( i = 1; i < 2048; ++i )
        {
                freq = (int)((((double)7093789.2)/((double)(2*i))));
                if ( freq > 65535 )
                        freq = 65535;
                fprintf( out_file, ", %d", freq );
        }

        fprintf( out_file, " };\n" );
}
*/

static int last_samp_length = 0;

static int RAW_LoadSound(const char *filename, DATAPACK *sfx) {
  struct stat file_info;
  if (stat(filename, &file_info) != 0) {
    printf("\nRAW_LoadSound( %s ): Unable to open...\n", filename);
    return -1;
  }
  int length = file_info.st_size;

  FILE *in_file = fopen(filename, "rb");
  if (!in_file) {
    printf("\nRAW_LoadSound( %s ): Unable to open...\n", filename);
    return -1;
  }

  BYTE *samp = (BYTE *)malloc(length + 16);
  last_samp_length = length;
  fread(samp + 16, 1, length, in_file);
  fclose(in_file);

  memmove(samp, samp + length, 16);
  MISC_ProcessSound(samp, length + 16);
  int data = DATA_Append(sfx, samp, length + 16) + 16;
  free(samp);

  printf("Done!\n\n");
  return data;
}

static BOOL WAV_CheckHeaders(FILE* in_file, const char *filename) {
  ULONG tmp;
  fread(&tmp, 4, 1, in_file);
  if (tmp != 0x46464952) {
    printf("\nWAV_LoadSound( %s ): Failed: RIFF header missing...\n",
           filename);
    return FALSE;
  }

  fread(&tmp, 4, 1, in_file);
  fread(&tmp, 4, 1, in_file);
  if (tmp != 0x45564157) {
    printf("\nWAV_LoadSound( %s ): Failed: Not a WAVE file...\n", filename);
    return FALSE;
  }

  fread(&tmp, 4, 1, in_file);
  if (tmp != 0x20746d66) {
    printf("\nWAV_LoadSound( %s ): Failed: Missing fmt subchunk...\n",
           filename);
    return FALSE;
  }

  unsigned short format;
  fread(&tmp, 4, 1, in_file);
  fread(&format, 2, 1, in_file);
  if (format != 1) { // PCM format
    printf("\nWAV_LoadSound( %s ): Failed: Not in PCM format...\n",
           filename);
    return FALSE;
  }

  unsigned short channels;
  fread(&channels, 2, 1, in_file);
  if (channels != 1) { // mono
    printf("\nWAV_LoadSound( %s ): Failed: Not mono...\n", filename);
    return FALSE;
  }

  unsigned short bits_per_sample;
  fread(&tmp, 4, 1, in_file); // sample rate in hz
  fread(&tmp, 4, 1, in_file);
  fread(&tmp, 2, 1, in_file);
  fread(&bits_per_sample, 2, 1, in_file);

  if (bits_per_sample != 8) {
    printf("\nWAV_LoadSound( %s ): Failed: Not 8 bit...\n",
           filename);
    return FALSE;
  }

  return TRUE;
}

static int WAV_LoadSound(const char *filename, DATAPACK *sfx) {
  FILE *in_file;
  in_file = fopen(filename, "rb");
  if (!in_file) {
    printf("\nWAV_LoadSound( %s ): Failed: Unable to open...\n", filename);
    return -1;
  }

  if (!WAV_CheckHeaders(in_file, filename)) {
    // we've already shown an error message when checking headers
    return -1;
  }

  ULONG tmp;
  int ret, length;
  BOOL done = FALSE;

  int data = -1;
  last_samp_length = 0;
  length = 0;

  do {
    ret = fread(&tmp, 4, 1, in_file);

    if (ret == 1) {
      if (tmp == 0x61746164) { // "data"
        ret = fread(&length, 4, 1, in_file);

        if (ret == 1) {
          if (length > 0) {
            done = TRUE;
          }
        } else {
          done = TRUE;
        }
      } else {
        int size;
        ret = fread(&size, 4, 1, in_file);

        if (ret == 1) {
          fseek(in_file, size, SEEK_CUR);
        } else {
          done = TRUE;
        }
      }
    } else {
      done = TRUE;
    }
  } while (!done);

  if (length > 0) {
    BYTE *samp = (BYTE *)malloc(length + 16);
    last_samp_length = length;
    fread(samp + 16, 1, length, in_file);
    memmove(samp, samp + length, 16);
    MISC_ConvSoundToSigned((UBYTE *)samp, length + 16);
    MISC_ProcessSound(samp, length + 16);
    data = DATA_Append(sfx, samp, length + 16) + 16;
    free(samp);
    printf("Done!\n\n");
  } else {
    printf("\nWAV_LoadSound( %s ): Unable to find valid data subchunk...\n",
           filename);
  }

  fclose(in_file);
  return data;
}

__inline static char String_GetChar(const char *txt, int offset) {
  return *(txt + offset);
}

static BOOL String_EndsWithMOD(const char *txt) {
  int txt_len = strlen(txt);

  if (txt_len > 4) {
    if ((String_GetChar(txt, txt_len - 4) == '.') &&
        ((String_GetChar(txt, txt_len - 3) == 'm') ||
         (String_GetChar(txt, txt_len - 3) == 'M')) &&
        ((String_GetChar(txt, txt_len - 2) == 'o') ||
         (String_GetChar(txt, txt_len - 2) == 'O')) &&
        ((String_GetChar(txt, txt_len - 1) == 'd') ||
         (String_GetChar(txt, txt_len - 1) == 'D'))) {
      return TRUE;
    }
  }

  return FALSE;
}

static BOOL String_EndsWithRAW(const char *txt) {
  int txt_len = strlen(txt);

  if (txt_len > 4) {
    if ((String_GetChar(txt, txt_len - 4) == '.') &&
        ((String_GetChar(txt, txt_len - 3) == 'r') ||
         (String_GetChar(txt, txt_len - 3) == 'R')) &&
        ((String_GetChar(txt, txt_len - 2) == 'a') ||
         (String_GetChar(txt, txt_len - 2) == 'A')) &&
        ((String_GetChar(txt, txt_len - 1) == 'w') ||
         (String_GetChar(txt, txt_len - 1) == 'W'))) {
      return TRUE;
    }
  }

  return FALSE;
}

static BOOL String_EndsWithWAV(const char *txt) {
  int txt_len = strlen(txt);

  if (txt_len > 4) {
    if ((String_GetChar(txt, txt_len - 4) == '.') &&
        ((String_GetChar(txt, txt_len - 3) == 'w') ||
         (String_GetChar(txt, txt_len - 3) == 'W')) &&
        ((String_GetChar(txt, txt_len - 2) == 'a') ||
         (String_GetChar(txt, txt_len - 2) == 'A')) &&
        ((String_GetChar(txt, txt_len - 1) == 'v') ||
         (String_GetChar(txt, txt_len - 1) == 'V'))) {
      return TRUE;
    }
  }

  return FALSE;
}

static void String_MakeSafe(char *temp) {
  char c;

  while (*temp != (char)0) {
    c = *temp;
    if (!(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
          ((c >= '0') && (c <= '9'))))
      *temp = '_';
    ++temp;
  }
}

static void ShowHelp() {
  printf(" Usage: Conv2AAS input_dir\n\n");
  printf("  input_dir  Directory containing all Protracker\n");
  printf("             MODs & sample data\n\n");
  printf("  Output goes to files AAS_Data.s & AAS_Data.h\n");
}

int main(int argc, char *argv[]) {
  setbuf(stdout, 0);

  printf("\n");
  printf("/---------------------------------------------\\\n");
  printf("| Conv2AAS v1.11        WAV, RAW & MOD -> AAS |\n");
  printf("| Copyright (c) 2005, Apex Designs            |\n");
  printf("\\---------------------------------------------/\n\n");

  if (argc < 2) {
    ShowHelp();
    return 1;
  } else if ((argc == 2) && ((strcmp(argv[1], "-help") == 0) ||
                             (strcmp(argv[1], "-?") == 0))) {
    ShowHelp();
    return 0;
  }

  FILE *out_file_s;
  out_file_s = fopen("./AAS_Data.s", "w");
  if (!out_file_s) {
    printf("Unable to open AASData.s for writing...\n");
    return 1;
  }

  FILE *out_file_h;
  out_file_h = fopen("./AAS_Data.h", "w");
  if (!out_file_h) {
    printf("Unable to open AASData.h for writing...\n");
    return 1;
  }

  DIR *dir_info;
  dir_info = opendir(argv[1]);
  if (!dir_info) {
    printf("Unable to open directory %s...\n", argv[1]);
    return 1;
  }

  int files_converted = 0;
  int mods_found = 0;
  DATAPACK *samples;
  DATAPACK *patterns;
  struct dirent *file_info;

  samples = DATA_Create("AAS_SampleData");
  patterns = DATA_Create("AAS_PatternData");

  fprintf(
      out_file_h,
      "#ifndef __AAS_DATA__\n#define __AAS_DATA__\n\n#include "
      "\"AAS.h\"\n\n#if AAS_VERSION != 0x111\n#error AAS version does "
      "not match Conv2AAS version\n#endif\n\nAAS_BEGIN_DECLS\n");

  fprintf(out_file_s,
          ".TEXT\n.SECTION .rodata\n.ALIGN\n.ARM\n\n.ALIGN\n.EXTERN "
          "AAS_lib_v111\n.GLOBAL AAS_data_v111\nAAS_data_v111:\n.word "
          "AAS_lib_v111\n");

  do {
    file_info = readdir(dir_info);
    if (file_info) {
      char temp[512];

      strcpy(temp, argv[1]);
      strcat(temp, "/");
      strcat(temp, file_info->d_name);

      if (String_EndsWithMOD(file_info->d_name)) {
        ++files_converted;
        printf("Adding MOD %s...", file_info->d_name);
        MOD_ConvMod(out_file_h, samples, patterns, temp, mods_found);
        // printf( "Done!\n" );
        strcpy(temp, file_info->d_name);
        *(temp + strlen(temp) - 4) = 0;
        String_MakeSafe(temp);
        fprintf(out_file_h, "\nextern const AAS_u8 AAS_DATA_MOD_%s;\n",
                temp);
        fprintf(out_file_s,
                "\n.ALIGN\n.GLOBAL "
                "AAS_DATA_MOD_%s\nAAS_DATA_MOD_%s:\n.byte %d\n",
                temp, temp, mods_found);
        ++mods_found;
      } else if (String_EndsWithRAW(file_info->d_name)) {
        int val;
        ++files_converted;
        printf("Adding RAW %s...", file_info->d_name);
        val = RAW_LoadSound(temp, samples);
        if (val >= 0) {
          strcpy(temp, file_info->d_name);
          *(temp + strlen(temp) - 4) = 0;
          String_MakeSafe(temp);
          fprintf(
              out_file_h,
              "\nextern const AAS_s8* const AAS_DATA_SFX_START_%s;\n",
              temp);
          fprintf(out_file_s,
                  "\n.ALIGN\n.GLOBAL "
                  "AAS_DATA_SFX_START_%s\nAAS_DATA_SFX_START_%s:\n."
                  "word AAS_SampleData + %d\n",
                  temp, temp, val);
          fprintf(out_file_h,
                  "\nextern const AAS_s8* const AAS_DATA_SFX_END_%s;\n",
                  temp);
          fprintf(out_file_s,
                  "\n.ALIGN\n.GLOBAL "
                  "AAS_DATA_SFX_END_%s\nAAS_DATA_SFX_END_%s:\n.word "
                  "AAS_SampleData + %d\n",
                  temp, temp, val + last_samp_length);
        }
      } else if (String_EndsWithWAV(file_info->d_name)) {
        int val;
        ++files_converted;
        printf("Adding WAV %s...", file_info->d_name);
        val = WAV_LoadSound(temp, samples);
        if (val >= 0) {
          strcpy(temp, file_info->d_name);
          *(temp + strlen(temp) - 4) = 0;
          String_MakeSafe(temp);
          fprintf(
              out_file_h,
              "\nextern const AAS_s8* const AAS_DATA_SFX_START_%s;\n",
              temp);
          fprintf(out_file_s,
                  "\n.ALIGN\n.GLOBAL "
                  "AAS_DATA_SFX_START_%s\nAAS_DATA_SFX_START_%s:\n."
                  "word AAS_SampleData + %d\n",
                  temp, temp, val);
          fprintf(out_file_h,
                  "\nextern const AAS_s8* const AAS_DATA_SFX_END_%s;\n",
                  temp);
          fprintf(out_file_s,
                  "\n.ALIGN\n.GLOBAL "
                  "AAS_DATA_SFX_END_%s\nAAS_DATA_SFX_END_%s:\n.word "
                  "AAS_SampleData + %d\n",
                  temp, temp, val + last_samp_length);
        }
      }
    }
  } while (file_info);

  closedir(dir_info);

  fprintf(out_file_s,
          "\n.ALIGN\n.GLOBAL "
          "AAS_DATA_NUM_MODS\nAAS_DATA_NUM_MODS:\n.short %d\n",
          mods_found);

  MOD_WriteSamples(out_file_s, mods_found);
  MOD_WritePatterns(out_file_s, mods_found);
  MOD_WriteNumChans(out_file_s, mods_found);
  MOD_WriteSongRestartPos(out_file_s, mods_found);

  // MOD_WritePeriodConvTable( out_file_h, 24002 );

  DATA_Write(samples, out_file_s);
  DATA_Write(patterns, out_file_s);

  printf("\n");

  fprintf(out_file_h, "\nAAS_END_DECLS\n\n#endif\n");

  fclose(out_file_h);
  fclose(out_file_s);

  return 0;
}
