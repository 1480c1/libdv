/* 
 *  YUY2.c
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

/* Most of this file is derived from patch 101018 submitted by Stefan
 * Lucke <lucke@berlin.snafu.de> */

#include <glib.h>
#include <stdlib.h>

#include "dv.h"
#include "YUY2.h"

#if USE_MMX_ASM
#include "mmx.h"
#endif // USE_MMX_ASM

/* Lookup tables for mapping signed to unsigned, and clamping */
static unsigned char	real_uvlut[256], *uvlut;
static unsigned char	real_ylut[768],  *ylut;

#if USE_MMX_ASM
/* Define some constants used in MMX range mapping and clamping logic */
static mmx_t		mmx_0x0010s = (mmx_t) 0x0010001000100010LL,
			mmx_0x0080s = (mmx_t) 0x0080008000800080LL,
			mmx_0x7f24s = (mmx_t) 0x7f247f247f247f24LL,
			mmx_0x7f94s = (mmx_t) 0x7f947f947f947f94LL,
			mmx_0xff00s = (mmx_t) 0xff00ff00ff00ff00LL;
#endif // USE_MMX_ASM

void 
dv_YUY2_init(void) {
  gint i;
  gint value;

  uvlut = real_uvlut + 128; // index from -128 .. 127
  for(i=-128;
      i<128;
      ++i) {
    if(i < (16-128)) value = 16;
    else if(i > (240-128)) value = 240;
    else value = i+128;
    uvlut[i] = value;
  } /* for */
  
  ylut = real_ylut + 256; // index from -256 .. 511
  for(i=-256;
      i<512;
      ++i) {
    if(i < (16-128)) value = 16;
    else if(i > (235-128)) value = 240;
    else value = i+128;
    ylut[i] = value;
  } /* for */
} /* dv_YUY2_init */

void 
dv_mb411_YUY2(dv_macroblock_t *mb, guchar *pixels, gint pitch, gint x, gint y) {
  dv_coeff_t		*Y[4], *cr_frame, *cb_frame;
  unsigned char	        *pyuv, *pwyuv, cb, cr;
  int			i, j, row;

  Y [0] = mb->b[0].coeffs;
  Y [1] = mb->b[1].coeffs;
  Y [2] = mb->b[2].coeffs;
  Y [3] = mb->b[3].coeffs;
  cr_frame = mb->b[4].coeffs;
  cb_frame = mb->b[5].coeffs;

  pyuv = pixels + (x * 2) + (y * pitch);

  for (row = 0; row < 8; ++row) { // Eight rows
    pwyuv = pyuv;

    for (i = 0; i < 4; ++i) {     // Four Y blocks
      dv_coeff_t *Ytmp = Y[i];   // less indexing in inner loop speedup?

      for (j = 0; j < 2; ++j) {   // two 4-pixel spans per Y block

        cb = uvlut[*cb_frame++];
        cr = uvlut[*cr_frame++];

	*pwyuv++ = ylut[*Ytmp++];
	*pwyuv++ = cb;
	*pwyuv++ = ylut[*Ytmp++];
	*pwyuv++ = cr;

	*pwyuv++ = ylut[*Ytmp++];
	*pwyuv++ = cb;
	*pwyuv++ = ylut[*Ytmp++];
	*pwyuv++ = cr;

      } /* for j */

      Y[i] = Ytmp;
    } /* for i */

    pyuv += pitch;
  } /* for row */
} /* dv_mb411_YUY2 */

void 
dv_mb420_YUY2(dv_macroblock_t *mb, guchar *pixels, gint pitch, gint x, gint y) {

  dv_coeff_t		*Y[4], *Ytmp0, *Ytmp1, *cr_frame, *cb_frame;
  unsigned char	        *pyuv, *pwyuv0, *pwyuv1, cb, cr;
  int			i, j, col, row;


  Y[0] = mb->b[0].coeffs;
  Y[1] = mb->b[1].coeffs;
  Y[2] = mb->b[2].coeffs;
  Y[3] = mb->b[3].coeffs;
  cr_frame = mb->b[4].coeffs;
  cb_frame = mb->b[5].coeffs;

  pyuv = pixels + (x * 2) + (y * pitch);

  for (j = 0; j < 4; j += 2) { // Two rows of blocks j, j+1

    for (row = 0; row < 8; row+=2) { // 4 pairs of two rows
      pwyuv0 = pyuv;
      pwyuv1 = pyuv + pitch;

      for (i = 0; i < 2; ++i) { // Two columns of blocks
        Ytmp0 = Y[j + i];  // even Y row
	Ytmp1 = Ytmp0 + 8; // odd Y row

        for (col = 0; col < 4; ++col) {  // 4 spans of 2x2 pixels

          cb = uvlut[*cb_frame++]; 
          cr = uvlut[*cr_frame++]; 

	  *pwyuv0++ = ylut[*Ytmp0++];
	  *pwyuv0++ = cb;
	  *pwyuv0++ = ylut[*Ytmp0++];
	  *pwyuv0++ = cr;

	  *pwyuv1++ = ylut[*Ytmp1++];
	  *pwyuv1++ = cb;
	  *pwyuv1++ = ylut[*Ytmp1++];
	  *pwyuv1++ = cr;
        } /* for col */
        Y[j + i] = Ytmp1;
      } /* for i */

      pyuv += (pitch * 2);
    } /* for row */

  } /* for j */
} /* dv_mb420_YUY2 */

#if USE_MMX_ASM

/* TODO (by Buck): 
 *
 *   When testing YV12.c, I discovered that my video card (RAGE 128)
 *   doesn't care that pixel components are strictly clamped to Y
 *   (16..235), UV (16..240).  So I was able to use MMX pack with
 *   unsigned saturation to go from 16 to 8 bit representation.  This
 *   clamps to (0..255).  Applying this to the code here might allow
 *   us to reduce instruction count below.
 *    
 *   Question:  do other video cards behave the same way?
 * */
void 
dv_mb411_YUY2_mmx(dv_macroblock_t *mb, guchar *pixels, gint pitch, gint x, gint y) {
    dv_coeff_t		*Y[4], *cr_frame, *cb_frame;
    unsigned char	*pyuv, *pwyuv;
    int			i, row;

    Y[0] = mb->b[0].coeffs;
    Y[1] = mb->b[1].coeffs;
    Y[2] = mb->b[2].coeffs;
    Y[3] = mb->b[3].coeffs;
    cr_frame = mb->b[4].coeffs;
    cb_frame = mb->b[5].coeffs;

    pyuv = pixels + (x * 2) + (y * pitch);

    movq_m2r (mmx_0x7f94s, mm6);
    movq_m2r (mmx_0x7f24s, mm5);

    for (row = 0; row < 8; ++row) { // Eight rows
      pwyuv = pyuv;
      for (i = 0; i < 4; ++i) {     // Four Y blocks
	dv_coeff_t *Ytmp = Y [i];   // less indexing in inner loop speedup?
	/* ---------------------------------------------------------------------
	 */
	movq_m2r (*cb_frame, mm2);    // cb0 cb1 cb2 cb3
	paddw_m2r (mmx_0x0080s, mm2); // add 128

	psllw_i2r (8, mm2);           // move into high byte
	movq_m2r (*cr_frame, mm3);    // cr0 cr1 cr2 cr3

	paddw_m2r (mmx_0x0080s, mm3); // add 128
	psllw_i2r (8, mm3);           // move into high byte

	movq_r2r (mm2, mm4);
	punpcklwd_r2r (mm3, mm4); // cb0cr0 cb1cr1

	punpckldq_r2r (mm4, mm4); // cb0cr0 cb0cr0
	pand_m2r (mmx_0xff00s, mm4); /* Buck wants to know:  is this just for pairing?
					low bytes are zero already-right? */

	/* ---------------------------------------------------------------------
	 */
	movq_m2r (*Ytmp, mm0);
	paddsw_r2r (mm6, mm0);         // clamp hi

	psubusw_r2r (mm5, mm0);        // clamp low
	paddw_m2r (mmx_0x0010s, mm0);  // adjust back into 16-235 range

	por_r2r (mm4, mm0);            // interleave with 4 bytes of crcb with 4 Ys
	movq_r2m (mm0, *pwyuv);        

	Ytmp += 4;
	pwyuv += 8;

	/* ---------------------------------------------------------------------
	 */
	movq_r2r (mm2, mm4);          
	punpcklwd_r2r (mm3, mm4); 
	punpckhdq_r2r (mm4, mm4); 

	/* ---------------------------------------------------------------------
	 */
	movq_m2r (*Ytmp, mm0);
	paddsw_r2r (mm6, mm0);
	psubusw_r2r (mm5, mm0);
	paddw_m2r (mmx_0x0010s, mm0);
	por_r2r (mm4, mm0);
	movq_r2m (mm0, *pwyuv);
	Ytmp += 4;
	pwyuv += 8;

	cr_frame += 2;
	cb_frame += 2;
	Y [i] = Ytmp;
      } /* for i */
      pyuv += pitch;
    } /* for j */
    emms ();
} /* dv_mb411_YUY2_mmx */

void 
dv_mb420_YUY2_mmx(dv_macroblock_t *mb, guchar *pixels, gint pitch, gint x, gint y) {
    dv_coeff_t		*Y [4], *Ytmp0, *cr_frame, *cb_frame;
    unsigned char	*pyuv, *pwyuv0, *pwyuv1;
    int			i, j, row;

    Y[0] = mb->b[0].coeffs;
    Y[1] = mb->b[1].coeffs;
    Y[2] = mb->b[2].coeffs;
    Y[3] = mb->b[3].coeffs;
    cr_frame = mb->b[4].coeffs;
    cb_frame = mb->b[5].coeffs;

    pyuv = pixels + (x * 2) + (y * pitch);

    movq_m2r (mmx_0x7f94s, mm6);
    movq_m2r (mmx_0x7f24s, mm5);

    for (j = 0; j < 4; j += 2) { // Two rows of blocks j, j+1
      for (row = 0; row < 8; row+=2) { // 4 pairs of two rows
	pwyuv0 = pyuv;
	pwyuv1 = pyuv + pitch;
	for (i = 0; i < 2; ++i) { // Two columns of blocks
	  Ytmp0 = Y[j + i];

	  /* -------------------------------------------------------------------
	   */
	  movq_m2r (*cb_frame, mm2);
	  paddw_m2r (mmx_0x0080s, mm2);

	  psllw_i2r (8, mm2);
	  movq_m2r (*cr_frame, mm3);

	  paddw_m2r (mmx_0x0080s, mm3);
	  psllw_i2r (8, mm3);

	  movq_r2r (mm2, mm4);
	  punpcklwd_r2r (mm3, mm4);

	  pand_m2r (mmx_0xff00s, mm4);

	  /* -------------------------------------------------------------------
	   */
	  movq_m2r (*(Ytmp0 + 0), mm0);
	  paddsw_r2r (mm6, mm0);
	  psubusw_r2r (mm5, mm0);
	  paddw_m2r (mmx_0x0010s, mm0);
	  por_r2r (mm4, mm0);

	  movq_m2r (*(Ytmp0 + 8), mm1);
	  paddsw_r2r (mm6, mm1);

	  movq_r2m (mm0, *pwyuv0);

	  psubusw_r2r (mm5, mm1);
	  paddw_m2r (mmx_0x0010s, mm1);
	  por_r2r (mm4, mm1);

	  movq_r2m (mm1, *pwyuv1);

	  movq_r2r (mm2, mm4);

	  Ytmp0 += 4;
	  punpckhwd_r2r (mm3, mm4);

	  pwyuv0 += 8;
	  pand_m2r (mmx_0xff00s, mm4);

	  pwyuv1 += 8;

	  movq_m2r (*(Ytmp0 + 0), mm0);
	  paddsw_r2r (mm6, mm0);
	  psubusw_r2r (mm5, mm0);
	  paddw_m2r (mmx_0x0010s, mm0);
	  por_r2r (mm4, mm0);

	  movq_m2r (*(Ytmp0 + 8), mm1);
	  paddsw_r2r (mm6, mm1);

	  movq_r2m (mm0, *pwyuv0);

	  psubusw_r2r (mm5, mm1);
	  paddw_m2r (mmx_0x0010s, mm1);
	  por_r2r (mm4, mm1);

	  movq_r2m (mm1, *pwyuv1);

	  movq_r2r (mm2, mm4);


	  Ytmp0 += 4;
	  pwyuv0 += 8;
	  pwyuv1 += 8;
	  cb_frame += 4;
	  cr_frame += 4;
	  Y[j + i] = Ytmp0 + 8;
	} /* for i */
	pyuv += (pitch * 2);
      } /* for row */
    } /* for j */
    emms ();
} /* dv_mb420_YUY2_mmx */

#endif // USE_MMX_ASM


