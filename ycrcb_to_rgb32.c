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
#define DVC_IMAGE_CHANS 4
#define DVC_IMAGE_ROWOFFSET (DVC_IMAGE_WIDTH * DVC_IMAGE_CHANS)

gint32 ylut[256];
gint32 impactcbr[256];
gint32 impactcbg[256];
gint32 impactcrg[256];
gint32 impactcrb[256];

static gint8 _dv_clamp[512];
static gint8 *dv_clamp;

static inline guint8 dv_clamp_c(gint d) { return d; }
static inline guint8 dv_clamp_y(gint d) {
	return dv_clamp[d];
} // dv_clamp_y

void dv_ycrcb_init()
{
  gint i;
  for(i=0;
      i<256;
      ++i) {
    ylut[i] = 298 * ((gint8)(i) + 128 - 16);
    impactcbr[i] = 409 * (gint8)(i);
    impactcbg[i] = 100 * (gint8)(i);
    impactcrg[i] = 208 * (gint8)(i);
    impactcrb[i] = 516 * (gint8)(i);
  }
  dv_clamp = _dv_clamp+128;
  for(i=-128; i<(256+128); i++) {
    if(i < 16) _dv_clamp[i] = 16-128;
    else if(i > 235) _dv_clamp[i] = 235-128;
    else _dv_clamp[i] = i-128;
  } // for 
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
  cb_frame = bl[4].coeffs;
  cr_frame = bl[5].coeffs;
  for (row = 0; row < 8; ++row) { // Eight rows
    for (i = 0; i < 4; ++i) {     // Four Y blocks
      dv_coeff_t *Ytmp = Y[i]; // less indexing in inner loop speedup?
      for (j = 0; j < 2; ++j) {   // two 4-pixel spans per Y block
        int cb = dv_clamp_c(*cb_frame++); // +128;
        int cr = dv_clamp_c(*cr_frame++); // +128
        int cbr = impactcbr[cb];
        int cbcrg = impactcbg[cb] + impactcrg[cr];
        int crb = impactcrb[cr];
 
        for (k = 0; k < 4; ++k) { // 4-pixel span
          gint32 y = ylut[dv_clamp_y(*Ytmp++)];
          gint32 r = (y + cbr  ) >> 8;
          gint32 g = (y - cbcrg) >> 8;
          gint32 b = (y + crb  ) >> 8;
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
  cb_frame = bl[4].coeffs;
  cr_frame = bl[5].coeffs;
  for (j = 0; j < 4; j += 2) { // Two rows of blocks j, j+1
    for (row = 0; row < 8; row+=2) { // 4 pairs of two rows
      for (i = 0; i < 2; ++i) { // Two columns of blocks
        int yindex = j + i;
        dv_coeff_t *Ytmp0 = Y[yindex];
        dv_coeff_t *Ytmp1 = Y[yindex] + 8;
        for (col = 0; col < 4; ++col) {  // 4 spans of 2x2 pixels
          int cb = dv_clamp_c(*cb_frame++); // +128;
          int cr = dv_clamp_c(*cr_frame++); // +128
          int cbr = impactcbr[cb];
          int cbcrg = impactcbg[cb] + impactcrg[cr];
          int crb = impactcrb[cr];
          for (k = 0; k < 2; ++k) { // 2x2 pixel
            gint32 y = ylut[dv_clamp_y(*Ytmp0++)];
            gint32 r = (y       + cbr) >> 8;
            gint32 g = (y - cbcrg) >> 8;
            gint32 b = (y + crb      ) >> 8;
            *(rgbp0)++ = (guint8)CLAMP(r,0,255);
            *(rgbp0)++ = (guint8)CLAMP(g,0,255);
            *(rgbp0)++ = (guint8)CLAMP(b,0,255);
#if (DVC_IMAGE_CHANS == 4)
            rgbp0++;
#endif

            y = ylut[dv_clamp_y(*Ytmp1++)];
            r = (y       + cbr) >> 8;
            g = (y - cbcrg) >> 8;
            b = (y + crb      ) >> 8;
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
