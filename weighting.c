/* 
 *  weighting.c
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
#include "weighting.h"

static double W[8];
static dv_coeff_t dv_weight_inverse_88_matrix[64];

static inline double CS(int m) {
  return cos(((double)m) * M_PI / 16.0);
}

void weight_88_inverse_float(double *block);

void weight_init(void) {
  double temp[64];
  int i;

  W[0] = 1.0;
  W[1] = CS(4) / (4.0 * CS(7) * CS(2));
  W[2] = CS(4) / (2.0 * CS(6));
  W[3] = 1.0 / (2 * CS(5));
  W[4] = 7.0 / 8.0;
  W[5] = CS(4) / CS(3); 
  W[6] = CS(4) / CS(2); 
  W[7] = CS(4) / CS(1);

  for (i=0;i<64;i++)
    temp[i] = 1.0;
  weight_88_inverse_float(temp);

  for (i=0;i<64;i++)
    dv_weight_inverse_88_matrix[i] = (dv_coeff_t)rint(temp[i] * 16.0);
}

void weight_88(dv_coeff_t *block) {
  int x,y;
  dv_coeff_t dc;

  dc = block[0];
  for (y=0;y<8;y++) {
    for (x=0;x<8;x++) {
      block[y*8+x] *= W[x] * W[y] / 2;
    }
  }
  block[0] = dc / 4;
}

void weight_248(dv_coeff_t *block) {
  int x,z;
  dv_coeff_t dc;

  dc = block[0];
  for (z=0;z<8;z++) {
    for (x=0;x<8;x++) {
      block[z*8+x] *= W[x] * W[2*z] / 2;
      block[(z+4)*8+x] *= W[x] * W[2*z] / 2;
    }
  }
  block[0] = dc / 4;
}

void weight_88_inverse_float(double *block) {
  int x,y;
  double dc;

  dc = block[0];
  for (y=0;y<8;y++) {
    for (x=0;x<8;x++) {
      block[y*8+x] /= (W[x] * W[y] / 2.0);
    }
  }
  block[0] = dc * 4.0;
}

void weight_88_inverse(dv_coeff_t *block) {
  int i;
  for (i=0;i<64;i++)
    block[i] *= dv_weight_inverse_88_matrix[i];
}

void weight_248_inverse(dv_coeff_t *block) {
  int x,z;
  dv_coeff_t dc;

  dc = block[0];
  for (z=0;z<4;z++) {
    for (x=0;x<8;x++) {
      block[z*8+x] /= (W[x] * W[2*z] / 2);
      block[(z+4)*8+x] /= (W[x] * W[2*z] / 2);
    }
  }
  block[0] = dc * 4;
}
