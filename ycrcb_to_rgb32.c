/* 
 *  ycrcb_to_rgb32.c
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

#include <glib.h>
#include <math.h>
#include "dv.h"

#define DVC_IMAGE_WIDTH 720
#define DVC_IMAGE_CHANS 3
#define DVC_IMAGE_ROWOFFSET (DVC_IMAGE_WIDTH * DVC_IMAGE_CHANS)

#define COLOR_FRACTION_BITS 10
#define COLOR_FRACTION_MUL  (1 << COLOR_FRACTION_BITS)

gint32 real_table_2_018[256];
gint32 real_table_0_813[256];
gint32 real_table_0_391[256];
gint32 real_table_1_596[256];

gint32 *table_2_018;
gint32 *table_0_813;
gint32 *table_0_391;
gint32 *table_1_596;

gint32 real_ylut[512], *ylut;

guint8 real_clamptab[768], *clamptab;

void dv_ycrcb_init()
{
  gint i;
  gint clamped_offset;
  table_2_018 = real_table_2_018 + 128;
  table_0_813 = real_table_0_813 + 128;
  table_0_391 = real_table_0_391 + 128;
  table_1_596 = real_table_1_596 + 128;
  ylut = real_ylut + 256;
  clamptab = real_clamptab + 256;

  for(i=-128;
      i<128;
      ++i) {
    if(i < (16-128)) clamped_offset = (16-128);
    else if(i > (240-128)) clamped_offset = (240-128);
    else clamped_offset = i;
    table_2_018[i] = (gint32)rint(2.018 * COLOR_FRACTION_MUL * clamped_offset);
    table_0_813[i] = (gint32)rint(0.813 * COLOR_FRACTION_MUL * clamped_offset);
    table_0_391[i] = (gint32)rint(0.391 * COLOR_FRACTION_MUL * clamped_offset);
    table_1_596[i] = (gint32)rint(1.596 * COLOR_FRACTION_MUL * clamped_offset);
  }
  for(i=-256; i < 256; i++) {
    if(i < (16-128)) clamped_offset = (16-128);
    else if(i > (235-128)) clamped_offset = (235-128);
    else clamped_offset = i;
    ylut[i] = (gint32)rint(1.164 * COLOR_FRACTION_MUL * (clamped_offset+128-16));
  }

  for(i=-256; i < 512; i++) {
    clamptab[i] = CLAMP(i, 0, 255);
  }
}

void dv_ycrcb_411_block(guint8 *base, dv_block_t *bl)
{
  dv_coeff_t *Y[4], *cr_frame, *cb_frame;
  guint8 *rgbp = base;
  int i,j,k, row;
  Y[0] = bl[0].coeffs;
  Y[1] = bl[1].coeffs;
  Y[2] = bl[2].coeffs;
  Y[3] = bl[3].coeffs;
  cr_frame = bl[4].coeffs;
  cb_frame = bl[5].coeffs;
  for (row = 0; row < 8; ++row) { // Eight rows
    for (i = 0; i < 4; ++i) {     // Four Y blocks
      dv_coeff_t *Ytmp = Y[i]; // less indexing in inner loop speedup?
      for (j = 0; j < 2; ++j) {   // two 4-pixel spans per Y block
        gint8 cb = *cb_frame++;  /* -128,-1  => 0x80,0xff */
        gint8 cr = *cr_frame++;
        int ro = table_1_596[cr];
        int go = table_0_813[cr] + table_0_391[cb];
        int bo =                   table_2_018[cb];
 
        for (k = 0; k < 4; ++k) { // 4-pixel span
          gint32 y = ylut[*Ytmp++];
          gint32 r = (y + ro) >> COLOR_FRACTION_BITS;
          gint32 g = (y - go) >> COLOR_FRACTION_BITS;
          gint32 b = (y + bo) >> COLOR_FRACTION_BITS;
          *rgbp++ = clamptab[r];
          *rgbp++ = clamptab[g];
          *rgbp++ = clamptab[b];
#if (DVC_IMAGE_CHANS == 4)
          rgbp++;
#endif
        }
      }
      Y[i] = Ytmp;
    }
    rgbp += DVC_IMAGE_ROWOFFSET - 4 * 8 * DVC_IMAGE_CHANS; // 4 rows, 8 pixels
  }
}

void dv_ycrcb_420_block(guint8 *base, dv_block_t *bl)
{
  dv_coeff_t *Y[4], *cr_frame, *cb_frame;
  guint8 *rgbp0, *rgbp1;
  int i, j, k, row, col;
  rgbp0 = base;
  rgbp1 = base + DVC_IMAGE_ROWOFFSET;
  Y[0] = bl[0].coeffs;
  Y[1] = bl[1].coeffs;
  Y[2] = bl[2].coeffs;
  Y[3] = bl[3].coeffs;
  cr_frame = bl[4].coeffs;
  cb_frame = bl[5].coeffs;
  for (j = 0; j < 4; j += 2) { // Two rows of blocks j, j+1
    for (row = 0; row < 8; row+=2) { // 4 pairs of two rows
      for (i = 0; i < 2; ++i) { // Two columns of blocks
        int yindex = j + i;
        dv_coeff_t *Ytmp0 = Y[yindex];
        dv_coeff_t *Ytmp1 = Y[yindex] + 8;
        for (col = 0; col < 4; ++col) {  // 4 spans of 2x2 pixels
          gint8 cb = *cb_frame++; // +128;
          gint8 cr = *cr_frame++; // +128
	  int ro = table_1_596[cr];
	  int go = table_0_813[cr] + table_0_391[cb];
	  int bo =                   table_2_018[cb];
	
          for (k = 0; k < 2; ++k) { // 2x2 pixel
            gint32 y = ylut[*Ytmp0++];
            gint32 r = (y + ro) >> COLOR_FRACTION_BITS;
            gint32 g = (y - go) >> COLOR_FRACTION_BITS;
            gint32 b = (y + bo) >> COLOR_FRACTION_BITS;
	    *rgbp0++ = clamptab[r];
	    *rgbp0++ = clamptab[g];
	    *rgbp0++ = clamptab[b];
#if (DVC_IMAGE_CHANS == 4)
            rgbp0++;
#endif

            y = ylut[*Ytmp1++];
            r = (y + ro) >> COLOR_FRACTION_BITS;
            g = (y - go) >> COLOR_FRACTION_BITS;
            b = (y + bo) >> COLOR_FRACTION_BITS;
	    *rgbp1++ = clamptab[r];
	    *rgbp1++ = clamptab[g];
	    *rgbp1++ = clamptab[b];
#if (DVC_IMAGE_CHANS == 4)
            rgbp1++;
#endif
          }
        }
        Y[yindex] = Ytmp1;
      }
      rgbp0 += 2 * DVC_IMAGE_ROWOFFSET - 2 * 8 * DVC_IMAGE_CHANS;
      rgbp1 += 2 * DVC_IMAGE_ROWOFFSET - 2 * 8 * DVC_IMAGE_CHANS;
    }
  }
}
