/* 
 *  dv.h
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
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

#ifndef __DV_H__
#define __DV_H__

#include <stdio.h>
#include <bitstream.h>

#define DV_DCT_248	(1)
#define DV_DCT_88	(0)

#define DV_SCT_HEADER    (0x0)
#define DV_SCT_SUBCODE   (0x1)
#define DV_SCT_VAUX      (0x2)
#define DV_SCT_AUDIO     (0x3)
#define DV_SCT_VIDEO     (0x4)

#define DV_FSC_0         (0)
#define DV_FSC_1         (1)

#if ARCH_X86
#define DV_WEIGHT_BIAS	 6
#else
#define DV_WEIGHT_BIAS	 0
#endif

#define DV_QUALITY_COLOR       1     /* Clear this bit to make monochrome */

#define DV_QUALITY_AC_MASK     (0x3 << 1)
#define DV_QUALITY_DC          (0x0 << 1)
#define DV_QUALITY_AC_1        (0x1 << 1)
#define DV_QUALITY_AC_2        (0x2 << 1)

#define DV_QUALITY_BEST       (DV_QUALITY_COLOR | DV_QUALITY_AC_2)
#define DV_QUALITY_FASTEST     0     /* Monochrome, DC coeffs only */

typedef enum color_space_e { 
  e_dv_color_yuv, 
  e_dv_color_rgb 
} dv_color_space_t;

typedef enum sample_e { 
  e_dv_sample_none = 0,
  e_dv_sample_411,
  e_dv_sample_420,
  e_dv_sample_422,
} dv_sample_t;

typedef enum system_e { 
  e_dv_system_none = 0,
  e_dv_system_525_60,    // NTSC
  e_dv_system_625_50,    // PAL
} dv_system_t;

typedef enum std_e { 
  e_dv_std_none = 0,
  e_dv_std_smpte_314m,    
  e_dv_std_iec_61834,    
} dv_std_t;

typedef gint16 dv_coeff_t;
typedef gint32 dv_248_coeff_t;

typedef struct dv_id_s {
  gint8 sct;      // Section type (header,subcode,aux,audio,video)
  gint8 dsn;      // DIF sequence number (0-12)
  gboolean fsc;   // First (0)/Second channel (1)
  gint8 dbn;      // DIF block number (0-134)
} dv_id_t;

typedef struct dv_header_s {
  gboolean dsf;   // DIF sequence flag: 525/60 (0) or 625,50 (1)
  gint8 apt;
  gboolean tf1;
  gint8 ap1;
  gboolean tf2;
  gint8 ap2;
  gboolean tf3;
  gint8 ap3;
} dv_header_t;

typedef struct dv_block_s {
  dv_coeff_t   coeffs[64] __attribute__ ((aligned (8)));
  dv_248_coeff_t coeffs248[64];
  gint         dct_mode; 
  gint         class_no;
  gint8        *reorder;
  gint8        *reorder_sentinel;
  gint         offset;   // bitstream offset of first unused bit 
  gint         end;      // bitstream offset of last bit + 1
  gint         eob;
  gboolean     mark;     // used during passes 2 & 3 for tracking fragmented vlcs
} dv_block_t;

typedef struct dv_macroblock_s {
  gint       i,j;   // superblock row/column, 
  gint       k;     // macroblock no. within superblock */
  gint       x, y;  // top-left corner position
  dv_block_t b[6];
  gint       qno;
  gint       sta;
  gint       vlc_error;
  gint       eob_count;
} dv_macroblock_t;

typedef struct dv_videosegment_s {
  gint            i, k;
  bitstream_t    *bs;
  dv_macroblock_t mb[5]; 
  gboolean        isPAL;
} dv_videosegment_t;

typedef struct dv_dif_sequence_s {
  dv_videosegment_t seg[27];
} dv_dif_sequence_t;

// Frame
typedef struct dv_frame_s {
  gboolean           placement_done;
  dv_dif_sequence_t  ds[12];  
} dv_frame_t;

typedef struct dv_decoder_s {
  guint       quality;
  dv_system_t system;
  dv_std_t    std;
  dv_sample_t sampling;
  gint        num_dif_seqs; // DIF sequences per frame
  gint        height, width;
  size_t      frame_size;
  dv_header_t header;
#if ARCH_X86
  gboolean    use_mmx;
#endif
} dv_decoder_t;

static const gint header_size = 80 * 6;
static const gint frame_size_525_60 = 10 * 150 * 80;
static const gint frame_size_625_50 = 12 * 150 * 80;

/* Main API */
extern void dv_init(dv_decoder_t *dv);
extern gint dv_parse_header(dv_decoder_t *dv, guchar *buffer);
extern void dv_decode_full_frame(dv_decoder_t *dv, guchar *buffer, 
				 dv_color_space_t color_space, guchar **pixels, guint16 *pitches);

/* Low level API */
extern gint dv_parse_video_segment(dv_videosegment_t *seg, guint quality);
extern void dv_decode_video_segment(dv_decoder_t *dv, dv_videosegment_t *seg, guint quality);

extern void dv_render_video_segment_rgb(dv_decoder_t *dv, dv_videosegment_t *seg, 
					guchar *pixels, gint pitch );

extern void dv_render_video_segment_yuv(dv_decoder_t *dv, dv_videosegment_t *seg, 
					guchar **pixels, guint16 *pitches);

#endif /* __DV_H__ */
