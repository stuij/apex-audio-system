// Copyright (c) 2003-2021 James Daniels
// Distributed under the MIT License
// license terms: see LICENSE file in root or http://opensource.org/licenses/MIT

#include "AAS_Shared.h"

int AAS_SFX_GetNumChannels() {
  if (AAS_initialised) {
    if (AAS_volscale == 9)
      return 16 - AAS_MOD_GetNumChannels();
    else if (AAS_volscale == 8)
      return 8 - AAS_MOD_GetNumChannels();
    else
      return 4 - AAS_MOD_GetNumChannels();
  } else {
    return 0;
  }
}

struct AAS_Channel *AAS_SFX_GetChannel(int channel) {
  if (AAS_volscale == 9)
    return &AAS_channels[AAS_chan_rearrange[15 - channel]];
  else if (AAS_volscale == 8)
    return &AAS_channels[AAS_chan_rearrange[7 - channel]];
  else
    return &AAS_channels[AAS_chan_rearrange[3 - channel]];
}

int AAS_SFX_GetOutput(int channel) {
  if (AAS_volscale == 9)
    return AAS_chan_rearrange[15 - channel] >> 3;
  else if (AAS_volscale == 8)
    return AAS_chan_rearrange[7 - channel] >> 3;
  else
    return AAS_chan_rearrange[3 - channel] >> 3;
}

AAS_BOOL AAS_SFX_ChannelExists(int channel) {
  if ((channel >= 0) && (channel < AAS_SFX_GetNumChannels())) {
    return AAS_TRUE;
  } else {
    return AAS_FALSE;
  }
}

int AAS_SFX_Play(int channel, int sample_volume, int sample_frequency,
                 const AAS_s8 *sample_start, const AAS_s8 *sample_end,
                 const AAS_s8 *sample_restart) {
  if (AAS_initialised) {
    if (AAS_SFX_ChannelExists(channel)) {
      if ((sample_frequency >= 1) && (sample_frequency <= 65535)) {
        if ((sample_volume >= 0) && (sample_volume <= 64)) {
          if (sample_start && (sample_end > sample_start) &&
              (sample_restart < sample_end)) {
            struct AAS_Channel *ch;

            ch = AAS_SFX_GetChannel(channel);
            ch->active = AAS_FALSE;
            ch->volume = (sample_volume << 8) >> AAS_volscale;
            ch->frequency = sample_frequency;
            ch->delta = AAS_Min(
                4095, ((sample_frequency * AAS_mix_scale) + 32768) >> 16);
            ch->pos = sample_start;
            ch->pos_fraction = 0;
            if (sample_restart)
              ch->loop_length = sample_end - sample_restart;
            else
              ch->loop_length = 0;
            ch->end = sample_end;
            ch->active = AAS_TRUE;

            AAS_changed[AAS_SFX_GetOutput(channel)] = AAS_TRUE;

            return AAS_OK;
          } else {
            return AAS_ERROR_INVALID_SAMPLE_ADDRESS;
          }
        } else {
          return AAS_ERROR_VOLUME_OUT_OF_RANGE;
        }
      } else {
        return AAS_ERROR_FREQUENCY_OUT_OF_RANGE;
      }
    } else {
      return AAS_ERROR_CHANNEL_NOT_AVAILABLE;
    }
  } else {
    return AAS_ERROR_CALL_SET_CONFIG_FIRST;
  }
}

AAS_BOOL AAS_SFX_IsActive(int channel) {
  if (AAS_SFX_ChannelExists(channel)) {
    return AAS_SFX_GetChannel(channel)->active;
  } else {
    return AAS_FALSE;
  }
}

int AAS_SFX_EndLoop(int channel) {
  if (AAS_initialised) {
    if (AAS_SFX_ChannelExists(channel)) {
      AAS_SFX_GetChannel(channel)->loop_length = 0;

      return AAS_OK;
    } else {
      return AAS_ERROR_CHANNEL_NOT_AVAILABLE;
    }
  } else {
    return AAS_ERROR_CALL_SET_CONFIG_FIRST;
  }
}

int AAS_SFX_SetFrequency(int channel, int sample_frequency) {
  if (AAS_initialised) {
    if (AAS_SFX_ChannelExists(channel)) {
      if ((sample_frequency >= 1) && (sample_frequency <= 65535)) {
        struct AAS_Channel *ch;

        ch = AAS_SFX_GetChannel(channel);
        ch->frequency = sample_frequency;
        ch->delta =
            AAS_Min(4095, ((sample_frequency * AAS_mix_scale) + 32768) >> 16);

        AAS_changed[AAS_SFX_GetOutput(channel)] = AAS_TRUE;

        return AAS_OK;
      } else {
        return AAS_ERROR_FREQUENCY_OUT_OF_RANGE;
      }
    } else {
      return AAS_ERROR_CHANNEL_NOT_AVAILABLE;
    }
  } else {
    return AAS_ERROR_CALL_SET_CONFIG_FIRST;
  }
}

int AAS_SFX_SetVolume(int channel, int sample_volume) {
  if (AAS_initialised) {
    if (AAS_SFX_ChannelExists(channel)) {
      if ((sample_volume >= 0) && (sample_volume <= 64)) {
        AAS_SFX_GetChannel(channel)->volume =
            (sample_volume << 8) >> AAS_volscale;

        AAS_changed[AAS_SFX_GetOutput(channel)] = AAS_TRUE;

        return AAS_OK;
      } else {
        return AAS_ERROR_VOLUME_OUT_OF_RANGE;
      }
    } else {
      return AAS_ERROR_CHANNEL_NOT_AVAILABLE;
    }
  } else {
    return AAS_ERROR_CALL_SET_CONFIG_FIRST;
  }
}

int AAS_SFX_Stop(int channel) {
  if (AAS_initialised) {
    if (AAS_SFX_ChannelExists(channel)) {
      AAS_SFX_GetChannel(channel)->active = AAS_FALSE;

      AAS_changed[AAS_SFX_GetOutput(channel)] = AAS_TRUE;

      return AAS_OK;
    } else {
      return AAS_ERROR_CHANNEL_NOT_AVAILABLE;
    }
  } else {
    return AAS_ERROR_CALL_SET_CONFIG_FIRST;
  }
}

int AAS_SFX_Resume(int channel) {
  if (AAS_initialised) {
    if (AAS_SFX_ChannelExists(channel)) {
      struct AAS_Channel *ch;

      ch = AAS_SFX_GetChannel(channel);

      if (!ch->active) {
        if (ch->loop_length) {
          ch->active = AAS_TRUE;

          AAS_changed[AAS_SFX_GetOutput(channel)] = AAS_TRUE;

          return AAS_OK;
        } else {
          if (ch->pos < ((ch->end - (ch->delta >> 6)) - 1)) {
            ch->active = AAS_TRUE;

            AAS_changed[AAS_SFX_GetOutput(channel)] = AAS_TRUE;

            return AAS_OK;
          } else {
            return AAS_ERROR_CHANNEL_UNRESUMEABLE;
          }
        }
      } else {
        return AAS_ERROR_CHANNEL_ACTIVE;
      }
    } else {
      return AAS_ERROR_CHANNEL_NOT_AVAILABLE;
    }
  } else {
    return AAS_ERROR_CALL_SET_CONFIG_FIRST;
  }
}
