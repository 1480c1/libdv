/*
 *  YV12.h
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
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

#ifndef DV_YV12_H
#define DV_YV12_H

#include "dv_types.h"

/* Convert output of decoder to YV12 conforming layout.  YV12 is a
 * format supported directly by many display adaptors.  See
 * the following website for details of YV12:
 *
 *    http://www.webartz.com/fourcc/fccyuv.htm#YV12
 * 
 * The conversion entails going from 16bit to 8bit and properly
 * clamping YUV values.
 * 
 * These conversions make sense to use if the HW supports YV12 and the
 * DV is IEC 61834 PAL.  SMPTE 314M PAL uses 411, so it is better to
 * upsample that to YUY2.  */

#ifdef __cplusplus
extern "C" {
#endif

extern void dv_YV12_init(dv_decoder_t *decoder, int clamp_luma, int clamp_chroma);

#ifdef __cplusplus
}
#endif

#endif /* DV_YV12_H */
