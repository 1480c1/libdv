/* 
 *  oss.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - January 2001
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  decoder.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <glib.h>
#include "oss.h"

/* Very simplistic for sound output using the OSS API */

gboolean
dv_oss_init(dv_audio_t *audio, dv_oss_t *oss)
{
  gint format = AFMT_S16_NE, rate_request, channels_request; 

  rate_request = audio->frequency;
  channels_request = audio->num_channels;
  oss->buffer=NULL;
  oss->fd=-1;
  /* open audio device */
  if ((oss->fd = open("/dev/audio", O_WRONLY, 0)) == -1) goto no_device;
  /* set sample format -- try for 16bit */
  if (ioctl(oss->fd, SNDCTL_DSP_SETFMT, &format) == -1) goto format_ioctl;
  if (format != AFMT_S16_NE) goto format_unsupported;
  /* set stereo mode */
  if (ioctl(oss->fd, SNDCTL_DSP_CHANNELS, &channels_request) == -1) goto channels_ioctl;
  if (channels_request != audio->num_channels) goto channels_unsupported;
  /* set sampling rate */
  if (ioctl(oss->fd, SNDCTL_DSP_SPEED, &rate_request) == -1) goto rate_ioctl;
  if(rate_request != audio->frequency) goto rate_unsupported;
  if(!(oss->buffer = malloc(DV_AUDIO_MAX_SAMPLES * audio->num_channels * sizeof(gint16)))) goto no_memory;
  return(TRUE);

 format_unsupported:
  fprintf(stderr, "soundcard doesn't support format\n");
  goto fail;

 rate_unsupported:
  fprintf(stderr,"audio rate found : %d Hz\n", rate_request);
  fprintf(stderr, "soundcard doesn't support 48kHz (does %d)\n", rate_request);
  goto fail;

 channels_unsupported:
  fprintf(stderr, "soundcard doesn't support stereo\n");
  goto fail;

 no_device:
  perror("/dev/dsp");
  goto fail;

 format_ioctl:
  perror("SNDCTL_DSP_SETFMT");
  goto fail;

 channels_ioctl:
  perror("SNDCTL_DSP_CHANNELS");
  goto fail;

 rate_ioctl:
  perror("SNDCTL_DSP_SPEED");
  goto fail;

 no_memory:
  fprintf(stderr, "out of memory\n");
  goto fail;

 fail:
  dv_oss_close(oss);
  return(FALSE);
} // dv_oss_init

gboolean
dv_oss_play(dv_audio_t *audio, dv_oss_t *oss, gint16 **out)
{
  gint ch, i, j=0, total, written=0, result;

  /* Interleave the audio into a single buffer */
  for(i=0; i < audio->samples_this_frame; i++) {
    for(ch=0; ch < audio->num_channels; ch++) {
      oss->buffer[j++] = out[ch][i];
    } // for
  } // for

  /* Send the audio to the device */
  total = audio->samples_this_frame * audio->num_channels * sizeof(gint16);
  do {
    result = write(oss->fd, oss->buffer + written, total - written);
    if(result <= 0) goto write_error;
    written += result;
  } while(total > written);
  if (ioctl(oss->fd, SNDCTL_DSP_POST, NULL) == -1) goto post_ioctl;

  return(TRUE);

 write_error:
  perror("write");
  return(FALSE);

 post_ioctl:
  perror("SNDCTL_DSP_POST");
  return(FALSE);

} // dv_oss_play

void
dv_oss_close(dv_oss_t *oss) 
{
  if(oss->fd != -1) {
    close(oss->fd);
    oss->fd = -1;
  } // if
  if(oss->buffer) {
    oss->buffer = NULL;
    free(oss->buffer);
  } // if
} // dv_oss_close
