/* 
 *  place.c
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

#include "dv.h"
#include "place.h"

#define DEBUG_HIGHLIGHT_MBS 0

static inline guint8 dv_clamp_c(gint d) { return d; }
static inline guint8 dv_clamp_y(gint d) {
  if(d < (16-128)) d = (16-128);
  else if(d > (235-128)) d = (235-128);
  return d;
  //  return d;
}

#define Y_POS(ROW,COL) ((ROW * 180 * 4 * 8) + (COL * 4 * 8))
#define C_POS(ROW,COL) ((ROW * 180 * 8) + (COL * 8))

static guint32 dv_411_sb_y_offset[12][5] = {
  { Y_POS(0, 0), Y_POS(0, 4), Y_POS(0, 9), Y_POS(0, 13), Y_POS(0, 18) },
  { Y_POS(6, 0), Y_POS(6, 4), Y_POS(6, 9), Y_POS(6, 13), Y_POS(6, 18) },
  { Y_POS(12,0), Y_POS(12,4), Y_POS(12,9), Y_POS(12,13), Y_POS(12,18) },
  { Y_POS(18,0), Y_POS(18,4), Y_POS(18,9), Y_POS(18,13), Y_POS(18,18) },
  { Y_POS(24,0), Y_POS(24,4), Y_POS(24,9), Y_POS(24,13), Y_POS(24,18) },
  { Y_POS(30,0), Y_POS(30,4), Y_POS(30,9), Y_POS(30,13), Y_POS(30,18) },
  { Y_POS(36,0), Y_POS(36,4), Y_POS(36,9), Y_POS(36,13), Y_POS(36,18) },
  { Y_POS(42,0), Y_POS(42,4), Y_POS(42,9), Y_POS(42,13), Y_POS(42,18) },
  { Y_POS(48,0), Y_POS(48,4), Y_POS(48,9), Y_POS(48,13), Y_POS(48,18) },
  { Y_POS(54,0), Y_POS(54,4), Y_POS(54,9), Y_POS(54,13), Y_POS(54,18) },
  { Y_POS(60,0), Y_POS(60,4), Y_POS(60,9), Y_POS(60,13), Y_POS(60,18) },
  { Y_POS(66,0), Y_POS(66,4), Y_POS(66,9), Y_POS(66,13), Y_POS(66,18) },
}; // dv_411_sb_y_offset

static guint16 dv_411_mb_y_offset[3][27] = {
  // cols 0, 2 
  { 
    Y_POS(0,0), Y_POS(1,0), Y_POS(2,0), Y_POS(3,0), Y_POS(4,0), Y_POS(5,0),
    Y_POS(5,1), Y_POS(4,1), Y_POS(3,1), Y_POS(2,1), Y_POS(1,1), Y_POS(0,1),
    Y_POS(0,2), Y_POS(1,2), Y_POS(2,2), Y_POS(3,2), Y_POS(4,2), Y_POS(5,2),
    Y_POS(5,3), Y_POS(4,3), Y_POS(3,3), Y_POS(2,3), Y_POS(1,3), Y_POS(0,3),
    Y_POS(0,4), Y_POS(1,4), Y_POS(2,4)
  },
  // cols 1, 3
  { 
    Y_POS(3,0), Y_POS(4,0), Y_POS(5,0),
    Y_POS(5,1), Y_POS(4,1), Y_POS(3,1), Y_POS(2,1), Y_POS(1,1), Y_POS(0,1),
    Y_POS(0,2), Y_POS(1,2), Y_POS(2,2), Y_POS(3,2), Y_POS(4,2), Y_POS(5,2),
    Y_POS(5,3), Y_POS(4,3), Y_POS(3,3), Y_POS(2,3), Y_POS(1,3), Y_POS(0,3),
    Y_POS(0,4), Y_POS(1,4), Y_POS(2,4), Y_POS(3,4), Y_POS(4,4), Y_POS(5,4)
  }, 
  // col 4
  { 
    Y_POS(0,0), Y_POS(1,0), Y_POS(2,0), Y_POS(3,0), Y_POS(4,0), Y_POS(5,0),
    Y_POS(5,1), Y_POS(4,1), Y_POS(3,1), Y_POS(2,1), Y_POS(1,1), Y_POS(0,1),
    Y_POS(0,2), Y_POS(1,2), Y_POS(2,2), Y_POS(3,2), Y_POS(4,2), Y_POS(5,2),
    Y_POS(5,3), Y_POS(4,3), Y_POS(3,3), Y_POS(2,3), Y_POS(1,3), Y_POS(0,3),
    Y_POS(0,4), Y_POS(2,4), Y_POS(4,4) // "alternate" format macroblocks on right edge
  }
}; // dv_411_mb_y_offset

static guint32 dv_411_sb_c_offset[12][5] = {
  { C_POS(0, 0), C_POS(0, 4), C_POS(0, 9), C_POS(0, 13), C_POS(0, 18) },
  { C_POS(6, 0), C_POS(6, 4), C_POS(6, 9), C_POS(6, 13), C_POS(6, 18) },
  { C_POS(12,0), C_POS(12,4), C_POS(12,9), C_POS(12,13), C_POS(12,18) },
  { C_POS(18,0), C_POS(18,4), C_POS(18,9), C_POS(18,13), C_POS(18,18) },
  { C_POS(24,0), C_POS(24,4), C_POS(24,9), C_POS(24,13), C_POS(24,18) },
  { C_POS(30,0), C_POS(30,4), C_POS(30,9), C_POS(30,13), C_POS(30,18) },
  { C_POS(36,0), C_POS(36,4), C_POS(36,9), C_POS(36,13), C_POS(36,18) },
  { C_POS(42,0), C_POS(42,4), C_POS(42,9), C_POS(42,13), C_POS(42,18) },
  { C_POS(48,0), C_POS(48,4), C_POS(48,9), C_POS(48,13), C_POS(48,18) },
  { C_POS(54,0), C_POS(54,4), C_POS(54,9), C_POS(54,13), C_POS(54,18) },
  { C_POS(60,0), C_POS(60,4), C_POS(60,9), C_POS(60,13), C_POS(60,18) },
  { C_POS(66,0), C_POS(66,4), C_POS(66,9), C_POS(66,13), C_POS(66,18) },
}; // dv_411_sb_c_offset

static guint16 dv_411_mb_c_offset[3][27] = {
  // cols 0, 2 
  { 
    C_POS(0,0), C_POS(1,0), C_POS(2,0), C_POS(3,0), C_POS(4,0), C_POS(5,0),
    C_POS(5,1), C_POS(4,1), C_POS(3,1), C_POS(2,1), C_POS(1,1), C_POS(0,1),
    C_POS(0,2), C_POS(1,2), C_POS(2,2), C_POS(3,2), C_POS(4,2), C_POS(5,2),
    C_POS(5,3), C_POS(4,3), C_POS(3,3), C_POS(2,3), C_POS(1,3), C_POS(0,3),
    C_POS(0,4), C_POS(1,4), C_POS(2,4)
  },
  // cols 1, 3
  { 
    C_POS(3,0), C_POS(4,0), C_POS(5,0),
    C_POS(5,1), C_POS(4,1), C_POS(3,1), C_POS(2,1), C_POS(1,1), C_POS(0,1),
    C_POS(0,2), C_POS(1,2), C_POS(2,2), C_POS(3,2), C_POS(4,2), C_POS(5,2),
    C_POS(5,3), C_POS(4,3), C_POS(3,3), C_POS(2,3), C_POS(1,3), C_POS(0,3),
    C_POS(0,4), C_POS(1,4), C_POS(2,4), C_POS(3,4), C_POS(4,4), C_POS(5,4)
  }, 
  // col 4
  { 
    C_POS(0,0), C_POS(1,0), C_POS(2,0), C_POS(3,0), C_POS(4,0), C_POS(5,0),
    C_POS(5,1), C_POS(4,1), C_POS(3,1), C_POS(2,1), C_POS(1,1), C_POS(0,1),
    C_POS(0,2), C_POS(1,2), C_POS(2,2), C_POS(3,2), C_POS(4,2), C_POS(5,2),
    C_POS(5,3), C_POS(4,3), C_POS(3,3), C_POS(2,3), C_POS(1,3), C_POS(0,3),
    C_POS(0,4), C_POS(2,4), C_POS(4,4) // "alternate" format macroblocks on right edge
  }
}; // dv_411_mb_c_offset

#undef Y_POS
#undef C_POS

#define Y_POS(ROW,COL) ((ROW * 180 * 4 * 8 * 2) + (COL * 2 * 8))
#define C_POS(ROW,COL) ((ROW * 360 * 8) + (COL * 8))

static guint32 dv_420_sb_y_offset[12][5] = {
  { Y_POS(0, 0), Y_POS(0,  9), Y_POS(0,  18), Y_POS(0,  27), Y_POS(0,  36) },
  { Y_POS(3, 0), Y_POS(3,  9), Y_POS(3,  18), Y_POS(3,  27), Y_POS(3,  36) },
  { Y_POS(6, 0), Y_POS(6,  9), Y_POS(6,  18), Y_POS(6,  27), Y_POS(6,  36) },
  { Y_POS(9, 0), Y_POS(9,  9), Y_POS(9,  18), Y_POS(9,  27), Y_POS(9,  36) },
  { Y_POS(12,0), Y_POS(12, 9), Y_POS(12, 18), Y_POS(12, 27), Y_POS(12, 36) },
  { Y_POS(15,0), Y_POS(15, 9), Y_POS(15, 18), Y_POS(15, 27), Y_POS(15, 36) },
  { Y_POS(18,0), Y_POS(18, 9), Y_POS(18, 18), Y_POS(18, 27), Y_POS(18, 36) },
  { Y_POS(21,0), Y_POS(21, 9), Y_POS(21, 18), Y_POS(21, 27), Y_POS(21, 36) },
  { Y_POS(24,0), Y_POS(24, 9), Y_POS(24, 18), Y_POS(24, 27), Y_POS(24, 36) },
  { Y_POS(27,0), Y_POS(27, 9), Y_POS(27, 18), Y_POS(27, 27), Y_POS(27, 36) },
  { Y_POS(30,0), Y_POS(30, 9), Y_POS(30, 18), Y_POS(30, 27), Y_POS(30, 36) },
  { Y_POS(33,0), Y_POS(33, 9), Y_POS(33, 18), Y_POS(33, 27), Y_POS(33, 36) },
}; // dv_420_sb_y_offset

static guint16 dv_420_mb_y_offset[27] = {
  Y_POS(0,0), Y_POS(1,0), Y_POS(2,0), Y_POS(2,1), Y_POS(1,1), Y_POS(0,1),
  Y_POS(0,2), Y_POS(1,2), Y_POS(2,2), Y_POS(2,3), Y_POS(1,3), Y_POS(0,3),
  Y_POS(0,4), Y_POS(1,4), Y_POS(2,4), Y_POS(2,5), Y_POS(1,5), Y_POS(0,5),
  Y_POS(0,6), Y_POS(1,6), Y_POS(2,6), Y_POS(2,7), Y_POS(1,7), Y_POS(0,7),
  Y_POS(0,8), Y_POS(1,8), Y_POS(2,8)
}; // dv_420_mb_y_offset

static guint32 dv_420_sb_c_offset[12][5] = {
  { C_POS(0, 0), C_POS(0,  9), C_POS(0,  18), C_POS(0,  27), C_POS(0,  36) },
  { C_POS(3, 0), C_POS(3,  9), C_POS(3,  18), C_POS(3,  27), C_POS(3,  36) },
  { C_POS(6, 0), C_POS(6,  9), C_POS(6,  18), C_POS(6,  27), C_POS(6,  36) },
  { C_POS(9, 0), C_POS(9,  9), C_POS(9,  18), C_POS(9,  27), C_POS(9,  36) },
  { C_POS(12,0), C_POS(12, 9), C_POS(12, 18), C_POS(12, 27), C_POS(12, 36) },
  { C_POS(15,0), C_POS(15, 9), C_POS(15, 18), C_POS(15, 27), C_POS(15, 36) },
  { C_POS(18,0), C_POS(18, 9), C_POS(18, 18), C_POS(18, 27), C_POS(18, 36) },
  { C_POS(21,0), C_POS(21, 9), C_POS(21, 18), C_POS(21, 27), C_POS(21, 36) },
  { C_POS(24,0), C_POS(24, 9), C_POS(24, 18), C_POS(24, 27), C_POS(24, 36) },
  { C_POS(27,0), C_POS(27, 9), C_POS(27, 18), C_POS(27, 27), C_POS(27, 36) },
  { C_POS(30,0), C_POS(30, 9), C_POS(30, 18), C_POS(30, 27), C_POS(30, 36) },
  { C_POS(33,0), C_POS(33, 9), C_POS(33, 18), C_POS(33, 27), C_POS(33, 36) },
}; // dv_420_sb_c_offset

static guint16 dv_420_mb_c_offset[27] = {
  C_POS(0,0), C_POS(1,0), C_POS(2,0), C_POS(2,1), C_POS(1,1), C_POS(0,1),
  C_POS(0,2), C_POS(1,2), C_POS(2,2), C_POS(2,3), C_POS(1,3), C_POS(0,3),
  C_POS(0,4), C_POS(1,4), C_POS(2,4), C_POS(2,5), C_POS(1,5), C_POS(0,5),
  C_POS(0,6), C_POS(1,6), C_POS(2,6), C_POS(2,7), C_POS(1,7), C_POS(0,7),
  C_POS(0,8), C_POS(1,8), C_POS(2,8)
}; // dv_420_mb_c_offset

void dv_place_init(void) {
  return;
} // dv_place_init

void dv_place_411_macroblock(dv_macroblock_t *mb,guint8 *y_frame,guint8 *cr_frame,guint8 *cb_frame) {
  dv_block_t *bl;
  gint b, row, col, sb_shape;
  dv_coeff_t *coeff;
  guint8 *y_base, *y;
  guint8 *cr_base, *cb_base, *c;
  guint32 c_offset;

  // First calculate the offset of the top left corner of the superblock
  y_base = y_frame + dv_411_sb_y_offset[mb->i][mb->j];
  c_offset = dv_411_sb_c_offset[mb->i][mb->j];
  cr_base = cr_frame + c_offset;
  cb_base = cb_frame + c_offset;

  // Second, add the offset for the macroblock position within the superblock
  if((mb->j == 0) || (mb->j == 2)) 
    sb_shape = 0;
  else if((mb->j == 1) || (mb->j == 3)) 
    sb_shape = 1;
  else
    sb_shape = 2;
  y_base += dv_411_mb_y_offset[sb_shape][mb->k];
  c_offset = dv_411_mb_c_offset[sb_shape][mb->k];
  cr_base += c_offset;
  cb_base += c_offset;

  // Normal macroblocks
  if((mb->j < 4) || (mb->k < 24)) {
#if DEBUG_HIGHLIGHT_MBS  // highlight MB borders
    for(b=0,bl=mb->b;
	b<4;
	b++,bl++) {
      for(col=0;
	  col<8;
	  col++) {
	bl->coeffs[col] = 235 - 128;
	bl->coeffs[7*8+col] = 235 - 128;
      }
      if(b==0) {
	for(row=1; 
	    row<7; 
	    row++) 
	  bl->coeffs[row*8] = 235 - 128;
      } else if(b==3) {
	for(row=1; 
	    row<7; 
	    row++)
	  bl->coeffs[row*8+7] = 235 - 128;
      } // else
    } // for 
#endif
    // Copy four Y blocks
    for(b=0,bl=mb->b,y=y_base;
	b<4;
	b++,bl++,y_base+=8) {
      coeff = bl->coeffs;
      y = y_base;
      for(row=0; 
	  row<8; 
	  row++, y+=(720-8)) 
	for(col=0; 
	    col<8; 
	    col++) 
	  *y++ = dv_clamp_y(*coeff++);
    } // for 
    // Copy Cb block
    coeff = bl->coeffs;
    c = cb_base;
    for(row=0; 
	row<8; 
	row++, c+=(180-8)) 
      for(col=0; 
	  col<8; 
	  col++) 
	*c++ = dv_clamp_c(*coeff++);
    // Copy Cr block
    bl++;
    coeff = bl->coeffs;
    c = cr_base;
    for(row=0; 
	row<8; 
	row++, c+=(180-8)) 
      for(col=0; 
	  col<8; 
	  col++) 
	*c++ = dv_clamp_c(*coeff++);
  } else {     // Right side macroblocks
    // Copy four Y blocks - first two 
    for(b=0,bl=mb->b;
	b<2;
	b++,bl++) {
      coeff = bl->coeffs;
      y = y_base + b * 8; 
      for(row=0; 
	  row<8; 
	  row++, y+=(720-8)) 
	for(col=0; 
	    col<8; 
	    col++) 
	  *y++ = dv_clamp_y(*coeff++);
    }
    // Y blocks - second two one row down
    for(b=0,bl=mb->b+2;
	b<2;
	b++,bl++) {
      coeff = bl->coeffs;
      y = y_base + 720 * 8 + b * 8; 
      for(row=0; 
	  row<8; 
	  row++, y+=(720-8))
	for(col=0; 
	    col<8; 
	    col++) 
	  *y++ = dv_clamp_y(*coeff++);
    } // for b
    // Copy Cb block
    coeff = bl->coeffs;
    c = cb_base;
    for(row=0; 
	row<8; 
	row++, c+=(180-4), coeff += 4) 
      for(col=0; 
	  col<4; 
	  col++) 
	*c++ = dv_clamp_c(*coeff++);
    coeff = bl->coeffs + 4;
    for(row=0; 
	row<8; 
	row++, c+=(180-4), coeff += 4) 
      for(col=0; 
	  col<4; 
	  col++) 
	*c++ = dv_clamp_c(*coeff++);
    // Copy Cr block
    bl++;
    coeff = bl->coeffs;
    c = cr_base;
    for(row=0; 
	row<8; 
	row++, c+=(180-4), coeff += 4) 
      for(col=0; 
	  col<4; 
	  col++) 
	*c++ = dv_clamp_c(*coeff++);
    coeff = bl->coeffs + 4;
    for(row=0; 
	row<8; 
	row++, c+=(180-4), coeff += 4) 
      for(col=0; 
	  col<4; 
	  col++) 
	*c++ = dv_clamp_c(*coeff++);
  } // else right side 
} // dv_place_411_macroblock

void dv_place_420_macroblock(dv_macroblock_t *mb,guint8 *y_frame,guint8 *cr_frame,guint8 *cb_frame) {
  dv_block_t *bl;
  gint b, row, col;
  dv_coeff_t *coeff;
  guint8 *y_base, *y;
  guint8 *cr_base, *cb_base, *c;
  guint32 c_offset;

  // First calculate the offset of the top left corner of the superblock
  y_base = y_frame + dv_420_sb_y_offset[mb->i][mb->j];
  c_offset = dv_420_sb_c_offset[mb->i][mb->j];
  cr_base = cr_frame + c_offset;
  cb_base = cb_frame + c_offset;

  // Second, add the offset for the macroblock position within the superblock
  y_base += dv_420_mb_y_offset[mb->k];
  c_offset = dv_420_mb_c_offset[mb->k];
  cr_base += c_offset;
  cb_base += c_offset;

#if DEBUG_HIGHLIGHT_MBS  // highlight MB borders
  for(b=0,bl=mb->b;
      b<4;
      b++,bl++) {
    for(col=0;
	col<8;
	col++) {
      bl->coeffs[col] = 235 - 128;
      bl->coeffs[7*8+col] = 235 - 128;
    }
    if(b==0) {
      for(row=1; 
	  row<7; 
	  row++) 
	bl->coeffs[row*8] = 235 - 128;
    } else if(b==3) {
      for(row=1; 
	  row<7; 
	  row++)
	bl->coeffs[row*8+7] = 235 - 128;
    } // else
  } // for 
#endif
  // Copy four Y blocks
  for(b=0,bl=mb->b,y=y_base;
      b<2;
      b++,bl++,y_base+=8) {
    coeff = bl->coeffs;
    y = y_base;
    for(row=0; 
	row<8; 
	row++, y+=(720-8)) 
      for(col=0; 
	  col<8; 
	  col++) 
	*y++ = dv_clamp_y(*coeff++);
  } // for 
  for(b=2,y_base+=(720*8-16);
      b<4;
      b++,bl++,y_base+=8) {
    coeff = bl->coeffs;
    y = y_base;
    for(row=0; 
	row<8; 
	row++, y+=(720-8)) 
      for(col=0; 
	  col<8; 
	  col++) 
	*y++ = dv_clamp_y(*coeff++);
  } // for 
  // Copy Cb block
  coeff = bl->coeffs;
  c = cb_base;
  for(row=0; 
      row<8; 
      row++, c+=(360-8)) 
    for(col=0; 
	col<8; 
	col++) 
      *c++ = dv_clamp_c(*coeff++);
  // Copy Cr block
  bl++;
  coeff = bl->coeffs;
  c = cr_base;
  for(row=0; 
      row<8; 
      row++, c+=(360-8)) 
    for(col=0; 
	col<8; 
	col++) 
      *c++ = dv_clamp_c(*coeff++);
} // dv_place_420_macroblock

