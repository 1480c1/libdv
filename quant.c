/* 
 *  quant.c
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

#include <math.h>
#include <glib.h>
#include "quant.h"

static guint8 dv_88_areas[64] = {
-1,0,0,1,1,1,2,2, 
 0,0,1,1,1,2,2,2, 
 0,1,1,1,2,2,2,3, 
 1,1,1,2,2,2,3,3, 

 1,1,2,2,2,3,3,3, 
 1,2,2,2,3,3,3,3, 
 2,2,2,3,3,3,3,3, 
 2,2,3,3,3,3,3,3 };

static guint8 dv_248_areas[64] = {
-1,0,1,1,1,2,2,3, 
 0,1,1,2,2,2,3,3, 
 1,1,2,2,2,3,3,3, 
 1,2,2,2,3,3,3,3,

 0,0,1,1,2,2,2,3, 
 0,1,1,2,2,2,3,3, 
 1,1,2,2,2,3,3,3, 
 1,2,2,3,3,3,3,3 };

static guint8 dv_quant_steps[22][4] = {
  { 8,8,16,16 }, 
  { 8,8,16,16 }, 

  { 4,8,8,16 }, 
  { 4,8,8,16 },

  { 4,4,8,8 }, 
  { 4,4,8,8 }, 

  { 2,4,4,8 }, 
  { 2,4,4,8 }, 

  { 2,2,4,4 }, 
  { 2,2,4,4 }, 

  { 1,2,2,4 }, 
  { 1,2,2,4 }, 

  { 1,1,2,2 }, 
  { 1,1,2,2 },

  { 1,1,1,2 }, 

  { 1,1,1,1 }, 
  { 1,1,1,1 }, 
  { 1,1,1,1 }, 
  { 1,1,1,1 }, 
  { 1,1,1,1 }, 
  { 1,1,1,1 }, 
  { 1,1,1,1 }
};

static guint8 dv_quant_offset[4] = { 6,3,0,1 };

void quant_88(dv_coeff_t *block,int qno,int class) {
  int i;
  dv_coeff_t dc;

  dc = block[0];
  for (i=0;i<64;i++) {
    block[i] /=
      dv_quant_steps[qno+dv_quant_offset[class]][dv_88_areas[i]];
  }
  if(class==3) { for (i=0;i<64;i++) block[i] /= 2; }
  block[0] = dc;
}

void quant_248(dv_coeff_t *block,int qno,int class) {
  int i;
  dv_coeff_t dc;

  dc = block[0];
  for (i=0;i<64;i++) {
    block[i] /=
      dv_quant_steps[qno+dv_quant_offset[class]][dv_248_areas[i]];
  }
  if(class==3) { for (i=0;i<64;i++) block[i] /= 2; }
  block[0] = dc;
}

void quant_88_inverse(dv_coeff_t *block,int qno,int class) {
  int i;
  dv_coeff_t dc;
  guint8 *pq;			/* pointer to the four quantization
                                   factors that we'll use */

  dc = block[0];		/* preserve the DC term */

  pq = dv_quant_steps[qno + dv_quant_offset[class]];
  for (i = 0; i < 64; i++)
    block[i] *= pq[dv_88_areas[i]];

  if(class==3) { for (i=0;i<64;i++) block[i] *= 2; }

  block[0] = dc;
}

void quant_248_inverse(dv_coeff_t *block,int qno,int class) {
  int i;
  dv_coeff_t dc;

  dc = block[0];
  for (i=0;i<64;i++) {
    block[i] *=
      dv_quant_steps[qno+dv_quant_offset[class]][dv_248_areas[i]];
  }
  if(class==3) { for (i=0;i<64;i++) block[i] *= 2; }
  block[0] = dc;
}
