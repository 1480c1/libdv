/* 
 *  rgb.h
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

#ifndef __DV_RGB_H__
#define __DV_RGB_H__

#include "dv.h"

/* Convert output of decoder to RGB layout.  
 * 
 * In addition to YUV to RGB, the conversion first takes care going
 * from 16bit to 8bit and properly clamping YUV values.
 * 
 * This conversions make sense to use if the HW doesn't support YUV.
 * Because of the need to pack 16 to 8, and clamp, it doesn't make sense
 * to allow SW emulation of YUV (e.g. SDL YUV overlays). 
 * 
 * Cache behaviour is particularly relevent here.  These routines
 * ideally would take advantage of cache hint mechanisms (eg. SSE and
 * MMX-2) to avoid a huge footprint from the last mile of the
 * rendering process.
 * */

extern void dv_rgb_init(void);

/* scalar version */
extern void dv_mb411_rgb(dv_macroblock_t *mb, guchar *pixels, gint pitch, gint x, gint y); 
extern void dv_mb420_rgb(dv_macroblock_t *mb, guchar *pixels, gint pitch, gint x, gint y); 

#if USE_MMX_ASM
/* pentium architecture mmx version */
extern void dv_mb411_rgb_mmx(dv_macroblock_t *mb, guchar *pixels, gint pitch, gint x, gint y); 
extern void dv_mb420_rgb_mmx(dv_macroblock_t *mb, guchar *pixels, gint pitch, gint x, gint y); 
#endif

#endif /* __DV_RGB_H__ */
