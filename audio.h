/* 
 *  audio.h
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

#ifndef DV_AUDIO_H
#define DV_AUDIO_H

#define DV_AUDIO_MAX_SAMPLES 1920

/* From Section 8.1 of 61834-4: Audio auxiliary data source pack fields pc1-pc4.
 * Need this data to figure out what format audio is in the stream. */

typedef struct dv_aaux_as_pc1_s {
  guint8 af_size : 6; /* Samples per frame: 
		       * 32 kHz: 1053-1080
		       * 44.1: 1452-1489
		       * 48: 1580-1620
		       */
  guint8 res0    : 1; // Should be 1
  guint8 lf      : 1; // Locked mode flag (1 = unlocked)
} dv_aaux_as_pc1_t;

typedef struct dv_aaux_as_pc2_s {
  guint8 audio_mode: 4; // See 8.1...
  guint8 pa        : 1; // pair bit: 0 = one pair of channels, 1 = independent channel (for sm = 1, pa shall be 1) 
  guint8 chn       : 2; // number of audio channels per block: 0 = 1 channel, 1 = 2 channels, others reserved
  guint8 sm        : 1; // stereo mode: 0 = Multi-stereo, 1 = Lumped
} dv_aaux_as_pc2_t;

typedef struct dv_aaux_as_pc3_s {
  guint8 stype     :5; // 0x0 = SD (525/625), 0x2 = HD (1125,1250), others reserved
  guint8 system    :1; // 0 = 60 fields, 1 = 50 field
  guint8 ml        :1; // Multi-languag flag
  guint8 res0      :1;
} dv_aaux_as_pc3_t;

typedef struct dv_aaux_as_pc4_s {
  guint8 qu        :3; // quantization: 0=16bits linear, 1=12bits non-linear, 2=20bits linear, others reserved
  guint8 smp       :3; // sampling frequency: 0=48kHz, 1=44,1 kHz, 2=32 kHz
  guint8 tc        :1; // time constant of emphasis: 1=50/15us, 0=reserved
  guint8 ef        :1; // emphasis: 0=on, 1=off
} dv_aaux_as_pc4_t;

typedef struct dv_auux_as_s {
  guint8 pc0; // value is 0x50;
  dv_aaux_as_pc1_t pc1;
  dv_aaux_as_pc2_t pc2;
  dv_aaux_as_pc3_t pc3;
  dv_aaux_as_pc4_t pc4;
} dv_aaux_as_t;

typedef struct dv_audio_s {
  dv_aaux_as_t  aaux_as;           // low-level audio format info direct from the stream
  gint          samples_this_frame; 
  gint          max_samples;
  gint          frequency;
  gint          num_channels;
} dv_audio_t;


/* Low-level routines */
extern gboolean dv_parse_audio_header(dv_audio_t *dv_audio, guchar *inbuf);
extern gboolean dv_update_num_samples(dv_audio_t *dv_audio, guint8 *inbuf);
extern gint dv_decode_audio_block(dv_audio_t *dv_audio, guint8 *buffer, gint ds, gint audio_dif, gint16 **outbufs);
extern void dv_dump_aaux_as(void *buffer, int ds, int audio_dif);

#endif // DV_AUDIO_H
