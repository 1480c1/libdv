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

gint32 ylut[256];
gint32 impactcbr[256];
gint32 impactcbg[256];
gint32 impactcrg[256];
gint32 impactcrb[256];

static int initted = 0;
static void dv_ycrcb_init()
{
  int i;
  for(i=0;
      i<256;
      ++i) {
    ylut[i] = 298 * ((signed char)(i) + 128 - 16);
    impactcbr[i] = 409 * (signed char)(i);
    impactcbg[i] = 100 * (signed char)(i);
    impactcrg[i] = 208 * (signed char)(i);
    impactcrb[i] = 516 * (signed char)(i);
  }
}

void dv_ycrcb_411_to_rgb32(unsigned char *y_frame, unsigned char *cr_frame, unsigned char *cb_frame, guint8 *rgb_frame, gint height) {
  int i;
  if (!initted) {
    dv_ycrcb_init();
    initted = 1;
  }
  for(i=0;
      i<height*180;
      i++) {
    int cr = *cr_frame++; // +128 
    int cb = *cb_frame++; // +128;
    int cbr = impactcbr[cb];
    int cbg = impactcbg[cb];
    int crg = impactcrg[cr];
    int crb = impactcrb[cr];
    int j;
    
    for(j=0;
	j<4;
	j++) {
      gint32 y = ylut[*y_frame++];
      gint32 r = (y       + cbr) >> 8;
      gint32 g = (y - cbg - crg) >> 8;
      gint32 b = (y + crb      ) >> 8;
      *rgb_frame++ = CLAMP(r,0,255);
      *rgb_frame++ = CLAMP(g,0,255);
      *rgb_frame++ = CLAMP(b,0,255);
      rgb_frame++;
    } // for j
  } // for i
} // dv_ycrcb_to_rgb32

void dv_ycrcb_420_to_rgb32(unsigned char *y_frame, unsigned char *cr_frame, unsigned char *cb_frame, guint8 *rgb_frame,gint height) {
  int i;
  gint32 y, cr, cb;
  gint32 r, g, b;
  guint8 *rgb;
  if (!initted) {
    dv_ycrcb_init();
    initted = 1;
  }
  for(i=0;
      i<height;
      i+=2) {
    int j;
    for(j=0;
	j<180*2;
	j++) {
      int cr = *cr_frame++; // +128 
      int cb = *cb_frame++; // +128;
      int cbr = impactcbr[cb];
      int cbg = impactcbg[cb];
      int crg = impactcrg[cr];
      int crb = impactcrb[cr];

      y = ylut[*y_frame++];
      r = (y       + cbr) / 256;
      g = (y - cbg - crg) / 256;
      b = (y + crb      ) / 256;
      *rgb_frame++ = CLAMP(r,0,255);
      *rgb_frame++ = CLAMP(g,0,255);
      *rgb_frame++ = CLAMP(b,0,255);
      rgb_frame++;

      y = ylut[*y_frame++];
      r = (y       + cbr) / 256;
      g = (y - cbg - crg) / 256;
      b = (y + crb      ) / 256;
      *rgb_frame++ = CLAMP(r,0,255);
      *rgb_frame++ = CLAMP(g,0,255);
      *rgb_frame++ = CLAMP(b,0,255);
      rgb_frame++;

      rgb = rgb_frame + ((720-2)*4);

      y = ylut[*y_frame++];
      r = (y       + cbr) / 256;
      g = (y - cbg - crg) / 256;
      b = (y + crb      ) / 256;
      *rgb++ = CLAMP(r,0,255);
      *rgb++ = CLAMP(g,0,255);
      *rgb++ = CLAMP(b,0,255);
      rgb++;

      y = ylut[*y_frame++];
      r = (y       + cbr) / 256;
      g = (y - cbg - crg) / 256;
      b = (y + crb      ) / 256;
      *rgb++ = CLAMP(r,0,255);
      *rgb++ = CLAMP(g,0,255);
      *rgb++ = CLAMP(b,0,255);

    } // for i
    y_frame += 720;
    rgb_frame += (720*4);
  } // for j
} // dv_ycrcb_to_rgb32
