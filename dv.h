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

typedef gint16 dv_coeff_t;

typedef struct dv_block_s {
  dv_coeff_t   coeffs[64];
  double       fcoeffs[64];
  gint         dct_mode; 
  gint         class_no;
  gint        *reorder;
  gint        *reorder_sentinel;
  gint         offset;   // bitstream offset of first unused bit 
  gint         end;      // bitstream offset of last bit + 1
  gint         eob;
  gboolean     mark;     // used during passes 2 & 3 for tracking fragmented vlcs
} dv_block_t;

typedef struct dv_macroblock_s {
  gint       i,j,k; /* i,j are superblock row/column, 
		 k is macroblock within superblock */
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
} dv_videosegment_t;

#endif /* __DV_H__ */
