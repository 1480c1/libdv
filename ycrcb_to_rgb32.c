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

void dv_ycrcb_to_rgb32(gint8 *y_frame, gint8 *cr_frame, gint8 *cb_frame, guint8 *rgb_frame,gint height) {
  int i,j;
  gint32 y, cr, cb;
  gint32 r, g, b;
  gint32 impact[4];
  for(i=0;
      i<height*180;
      i++,cr_frame++,cb_frame++) {
    cr = *cr_frame; // +128 
    cb = *cb_frame; // +128;
    impact[0] = 409 * cb;
    impact[1] = 100 * cb;
    impact[2] = 208 * cr;
    impact[3] = 516 * cr;
    for(j=0;
	j<4;
	j++) {
      y = 298 * (*y_frame++ + 128 - 16);
      r = (y                            + (impact[0])) / 256;
      g = (y - (impact[1]) - (impact[2])) / 256;
      b = (y + (impact[2]                            )) / 256;
      *rgb_frame++ = CLAMP(r,0,255);
      *rgb_frame++ = CLAMP(g,0,255);
      *rgb_frame++ = CLAMP(b,0,255);
      rgb_frame++;
    } // for j
  } // for i
} // dv_ycrcb_to_rgb32
