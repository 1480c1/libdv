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
#include "dv.h"

#define DVC_IMAGE_WIDTH 720
#define DVC_IMAGE_CHANS 3
#define DVC_IMAGE_ROWOFFSET (DVC_IMAGE_WIDTH * DVC_IMAGE_CHANS)

gint32 table_2_018[256];
gint32 table_0_813[256];
gint32 table_0_391[256];
gint32 table_1_596[256];

gint32 real_ylut[512], *ylut;

void dv_ycrcb_init()
{
  gint i;
  for(i=0;
      i<256;
      ++i) {
    table_2_018[i] = (gint32)(2.018 * 256 * (gint8)i);
    table_0_813[i] = (gint32)(0.813 * 256 * (gint8)i);
    table_0_391[i] = (gint32)(0.391 * 256 * (gint8)i);
    table_1_596[i] = (gint32)(1.596 * 256 * (gint8)i);
  }
  for(i=0; i < 512; i++) {
    gint clamped_offset;

    clamped_offset = (i - 128) + (128 - 16);
    if (clamped_offset < 0)
      clamped_offset = 0;
    else if (clamped_offset > 255)
      clamped_offset = 255;

    real_ylut[i] = (gint32)(1.164 * 256 * clamped_offset);
  } // for 
  ylut = real_ylut + 128;
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
        guint8 cb = *cb_frame++;  /* -128,-1  => 0x80,0xff */
        guint8 cr = *cr_frame++;
        int ro = table_1_596[cr];
        int go = table_0_813[cr] + table_0_391[cb];
        int bo =                   table_2_018[cb];
 
        for (k = 0; k < 4; ++k) { // 4-pixel span
          gint32 y = ylut[*Ytmp++];
          gint32 r = (y + ro) >> 8;
          gint32 g = (y - go) >> 8;
          gint32 b = (y + bo) >> 8;
          *rgbp++ = (guint8)CLAMP(r,0,255);
          *rgbp++ = (guint8)CLAMP(g,0,255);
          *rgbp++ = (guint8)CLAMP(b,0,255);
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
          guint8 cb = *cb_frame++; // +128;
          guint8 cr = *cr_frame++; // +128
	  int ro = table_1_596[cr];
	  int go = table_0_813[cr] + table_0_391[cb];
	  int bo =                   table_2_018[cb];
	
          for (k = 0; k < 2; ++k) { // 2x2 pixel
            gint32 y = ylut[*Ytmp0++];
            gint32 r = (y + ro) >> 8;
            gint32 g = (y - go) >> 8;
            gint32 b = (y + bo) >> 8;
            *(rgbp0)++ = (guint8)CLAMP(r,0,255);
            *(rgbp0)++ = (guint8)CLAMP(g,0,255);
            *(rgbp0)++ = (guint8)CLAMP(b,0,255);
#if (DVC_IMAGE_CHANS == 4)
            rgbp0++;
#endif

            y = ylut[*Ytmp1++];
            r = (y + ro) >> 8;
            g = (y - go) >> 8;
            b = (y + bo) >> 8;
            *(rgbp1)++ = (guint8)CLAMP(r,0,255);
            *(rgbp1)++ = (guint8)CLAMP(g,0,255);
            *(rgbp1)++ = (guint8)CLAMP(b,0,255);
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
