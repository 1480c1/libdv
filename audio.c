/* 
 *  audio.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_LIBPOPT
#include <popt.h>
#endif // HAVE_LIBPOPT


#include "dv.h"
#include "audio.h"

/*
 * DV audio data is shuffled within the frame data.  The unshuffle 
 * tables are indexed by DIF sequence number, then audio DIF block number.
 * The first audio channel (pair) is in the first half of DIF sequences, and the
 * second audio channel (pair) is in the second half.
 *
 * DV audio can be sampled at 48, 44.1, and 32 kHz.  Normally, samples
 * are quantized to 16 bits, with 2 channels.  In 32 kHz mode, it is
 * possible to support 4 channels by non-linearly quantizing to 12
 * bits.
 *
 * The code here always returns 16 bit samples.  In the case of 12
 * bit, we upsample to 16 according to the DV standard defined
 * mapping.
 *
 * DV audio can be "locked" or "unlocked".  In unlocked mode, the
 * number of audio samples per video frame can vary somewhat.  Header
 * info in the audio sections (AAUX AS) is used to tell the decoder on
 * a frame be frame basis how many samples are present. 
 * */

static int dv_audio_unshuffle_60[5][9] = {
  { 0, 15, 30, 10, 25, 40,  5, 20, 35 },
  { 3, 18, 33, 13, 28, 43,  8, 23, 38 },
  { 6, 21, 36,  1, 16, 31, 11, 26, 41 },
  { 9, 24, 39,  4, 19, 34, 14, 29, 44 },
  {12, 27, 42,  7, 22, 37,  2, 17, 32 },
};

static int dv_audio_unshuffle_50[6][9] = {
  {  0, 18, 36, 13, 31, 49,  8, 26, 44 },
  {  3, 21, 39, 16, 34, 52, 11, 29, 47 },
  {  6, 24, 42,  1, 19, 37, 14, 32, 50 }, 
  {  9, 27, 45,  4, 22, 40, 17, 35, 53 }, 
  { 12, 30, 48,  7, 24, 43,  2, 20, 38 },
  { 15, 33, 51, 10, 28, 46,  5, 23, 41 },
};

/* Minumum number of samples, indexed by system (PAL/NTSC) and
   sampling rate (32 kHz, 44.1 kHz, 48 kHz) */

static gint min_samples[2][3] = {
  { 1580, 1452, 1053 }, // 60 fields (NTSC)
  { 1896, 1742, 1264 }, // 50 fields (PAL)
};

static gint max_samples[2][3] = {
  { 1620, 1489, 1080 }, // 60 fields (NTSC)
  { 1944, 1786, 1296 }, // 50 fields (PAL)
};

static gint frequency[3] = {
  48000, 44100, 32000
};

static guchar *quantization[8] = {
  [0] "16 bits linear",
  [1] "12 bits non-linear",
  [2] "20 bits linear",
  [3 ... 7] = "unknown",
};


#if HAVE_LIBPOPT
static void
dv_audio_popt_callback(poptContext con, enum poptCallbackReason reason, 
		       const struct poptOption * opt, const char * arg, const void * data)
{
  dv_audio_t *audio = (dv_audio_t *)data;

  if((audio->arg_audio_frequency < 0) || (audio->arg_audio_frequency > 3)) {
    dv_opt_usage(con, audio->option_table, DV_AUDIO_OPT_FREQUENCY);
  } // if
  
  if((audio->arg_audio_quantization < 0) || (audio->arg_audio_quantization > 2)) {
    dv_opt_usage(con, audio->option_table, DV_AUDIO_OPT_QUANTIZATION);
  } // if

} // dv_audio_popt_callback 
#endif // HAVE_LIBPOPT

static gint 
dv_audio_samples_per_frame(dv_aaux_as_t *dv_aaux_as) {
  gint result = -1;
  
  if(!(dv_aaux_as->pc3.system < 2) && (dv_aaux_as->pc4.smp < 3)) goto unknown_format;

  result = dv_aaux_as->pc1.af_size + min_samples[dv_aaux_as->pc3.system][dv_aaux_as->pc4.smp];
  return(result);

 unknown_format:
  fprintf(stderr, "libdv(%s):  badly formed AAUX AS data [pc3.system:%d, pc4.smp:%d]\n",
	  __FUNCTION__, dv_aaux_as->pc3.system, dv_aaux_as->pc4.smp);
  return(result);

} // dv_audio_samples_per_frame

/* Take a DV 12bit audio sample upto 16 bits. 
 * See IEC 61834-2, Figure 16, on p. 129 */

static __inline__ gint32 
dv_upsample(gint32 sample) {
  gint32 shift, result;
  
  shift = (sample & 0xf00) >> 8;
  switch(shift) {
  case 0x2 ... 0x7:
    shift--;
    result = (sample - (256 * shift)) << shift;
    break;
  case 0x8 ... 0xd:
    shift = 0xe - shift;
    result = ((sample + ((256 * shift) + 1)) << shift) - 1;
    break;
  default:
    result = sample;
    break;
  } // switch
  return(result);
} // dv_upsample

dv_audio_t *
dv_audio_new(void)
{
  dv_audio_t *result;
  
  if(!(result = calloc(1,sizeof(dv_audio_t)))) goto no_mem;

#if HAVE_LIBPOPT
  result->option_table[DV_AUDIO_OPT_FREQUENCY] = (struct poptOption){ 
    longName:   "frequency", 
    shortName:  'f', 
    argInfo:    POPT_ARG_INT, 
    arg:        &result->arg_audio_frequency,
    descrip:    "audio frequency: 0=autodetect [default], 1=32 kHz, 2=44.1 kHz, 3=48 kHz",
    argDescrip: "(0|1|2|3)"
  }; // freq

  result->option_table[DV_AUDIO_OPT_QUANTIZATION] = (struct poptOption){ 
    longName:   "quantization", 
    shortName:  'Q', 
    argInfo:    POPT_ARG_INT, 
    arg:        &result->arg_audio_quantization,
    descrip:    "force audio quantization: 0=autodetect [default], 1=12 bit, 2=16bit",
    argDescrip: "(0|1|2)"
  }; // quant

  result->option_table[DV_AUDIO_OPT_CALLBACK] = (struct poptOption){
    argInfo: POPT_ARG_CALLBACK|POPT_CBFLAG_POST,
    arg:     dv_audio_popt_callback,
    descrip: (char *)result, // data passed to callback
  }; // callback

#endif // HAVE_LIBPOPT
  return(result);

 no_mem:
  return(result);
} // dv_audio_new

void 
dv_dump_aaux_as(void *buffer, int ds, int audio_dif) 
{
  dv_aaux_as_t *dv_aaux_as;

  dv_aaux_as = buffer + 3;

  if(dv_aaux_as->pc0 == 0x50) {
    // AAUX AS 

    printf("DS %d, Audio DIF %d, AAUX AS pack: ", ds, audio_dif);

    if(dv_aaux_as->pc1.lf) {
      printf("Unlocked audio");
    } else {
      printf("Locked audio");
    }

    printf(", Sampling ");
    printf("%.1f kHz", (float)frequency[dv_aaux_as->pc4.smp] / 1000.0);

    printf(" (%d samples, %d fields)", 
	   dv_audio_samples_per_frame(dv_aaux_as),
	   (dv_aaux_as->pc3.system ? 50 : 60));

    printf(", Quantization %s", quantization[dv_aaux_as->pc4.qu]);
    
    printf(", Emphasis %s\n", (dv_aaux_as->pc4.ef ? "off" : "on"));

  } else {

    fprintf(stderr, "libdv(%s):  Missing AAUX AS PACK!\n", __FUNCTION__);

  } // else

} // dv_dump_aaux_as

gboolean
dv_parse_audio_header(dv_audio_t *dv_audio, guchar *inbuf)
{
  dv_aaux_as_t *dv_aaux_as = (dv_aaux_as_t *)(inbuf + 80*6+80*16*3 + 3);

  if(dv_aaux_as->pc0 != 0x50) goto bad_id;

  dv_audio->max_samples =  max_samples[dv_aaux_as->pc3.system][dv_aaux_as->pc4.smp];
  // For now we assume that 12bit = 4 channels
  if(dv_aaux_as->pc4.qu > 1) goto unsupported_quantization;
  dv_audio->num_channels = (dv_aaux_as->pc4.qu+1) * 2; // TODO verify this is right with known 4-channel input
  dv_audio->frequency = frequency[dv_aaux_as->pc4.smp];
  printf("Audio is %.1f kHz, %s quantization, %d channels\n",
	 (float)frequency[dv_aaux_as->pc4.smp] / 1000.0,
	 quantization[dv_aaux_as->pc4.qu],
	 dv_audio->num_channels);
  dv_audio->samples_this_frame = dv_audio_samples_per_frame(dv_aaux_as);

  dv_audio->aaux_as = *dv_aaux_as;

  return(TRUE);

 bad_id:
  return(FALSE);
  
 unsupported_quantization:
  fprintf(stderr, "libdv(%s):  Malformrmed AAUX AS? pc4.qu == %d\n", 
	  __FUNCTION__, dv_audio->aaux_as.pc4.qu);
  return(FALSE);

} // dv_parse_audio_header

gboolean
dv_update_num_samples(dv_audio_t *dv_audio, guint8 *inbuf) {

  dv_aaux_as_t *dv_aaux_as = (dv_aaux_as_t *)(inbuf + 80*6+80*16*3 + 3);

  if(dv_aaux_as->pc0 != 0x50) goto bad_id;
  dv_audio->samples_this_frame = dv_audio_samples_per_frame(dv_aaux_as);
  return(TRUE);

 bad_id:
  return(FALSE);

} // dv_update_num_samples

gint 
dv_decode_audio_block(dv_audio_t *dv_audio, guint8 *inbuf, gint ds, gint audio_dif, gint16 **outbufs) 
{
  gint channel, bp, i;
  guint16 *samples, *ysamples, *zsamples;
  gint32 msb_y, msb_z, lsb;
  gint half_ds;

#if 0
  if ((inbuf[0] & 0xe0) != 0x60) goto bad_id;
#endif

  half_ds = (dv_audio->aaux_as.pc3.system ? 6 : 5);

  if(ds < half_ds) {
    channel = 0;
  } else {
    channel = 1;
    ds -= half_ds;
  } // else

  switch(dv_audio->aaux_as.pc4.qu) {

  case 0: // 16 bits linear

    samples = outbufs[channel];
    if(dv_audio->aaux_as.pc3.system) {

      // PAL
      for (bp = 8; bp < 80; bp+=2) {
	i = dv_audio_unshuffle_50[ds][audio_dif] + (bp - 8)/2 * 45;
	samples[i] = ((gint16)inbuf[bp] << 8) | inbuf[bp+1];
      } // for

    } else {

      // NTSC
      for (bp = 8; bp < 80; bp+=2) {
	i = dv_audio_unshuffle_60[ds][audio_dif] + (bp - 8)/2 * 45;
	//	if(i < dv_audio->samples_this_frame) {
	  samples[i] = ((gint16)inbuf[bp] << 8) | inbuf[bp+1];
	  //	}
      } // for

    } // else
    break;

  case 1: // 12 bits non-linear

    // See 61834-2 figure 18, and text of section 6.5.3 and 6.7.2
    ysamples = outbufs[channel * 2];
    zsamples = outbufs[1 + channel * 2];

    if(dv_audio->aaux_as.pc3.system) {

      // PAL
      for (bp = 8; bp < 80; bp+=3) {
	i = dv_audio_unshuffle_50[ds][audio_dif] + (bp - 8)/3 * 45;
	msb_y = inbuf[bp];
	msb_z = inbuf[bp+1];
	lsb   = inbuf[bp+2];
	if(i >= 1920) {
	  g_assert(i < 1920);
	}
	ysamples[i] = dv_upsample((msb_y << 4) | (lsb >> 4));
	zsamples[i] = dv_upsample((msb_z << 4) | (lsb & 0xf));
      } // for

    } else {

      // NTSC
      for (bp = 8; bp < 80; bp+=3) {
	i = dv_audio_unshuffle_60[ds][audio_dif] + (bp - 8)/3 * 45;
	msb_y = inbuf[bp];
	msb_z = inbuf[bp+1];
	lsb   = inbuf[bp+2];
	ysamples[i] = dv_upsample((msb_y << 4) | (lsb >> 4)); 
	zsamples[i] = dv_upsample((msb_z << 4) | (lsb & 0xf));
      } // for

    } // else
    break;

  default:
    goto unsupported_sampling;
    break;
  } // switch

  return(0);

 unsupported_sampling:
  fprintf(stderr, "libdv(%s):  unsupported audio sampling.\n", __FUNCTION__);
  return(-1);

#if 0
 bad_id:
  fprintf(stderr, "libdv(%s):  not an audio block\n", __FUNCTION__);
  return(-1);
#endif
  
} // dv_decode_audio_block
