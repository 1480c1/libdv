/* 
 *  vlc.h
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

#ifndef __VLC_H__
#define __VLC_H__

#include <glib.h>
#include "bitstream.h"

#define USE_ASM_FOR_VLC 1

#define VLC_NOBITS (-1)
#define VLC_ERROR (-2)

typedef struct dv_vlc_s {
  gint8 run;
  gint8 len;
  gint16 amp;
} dv_vlc_t;

#if 1
typedef dv_vlc_t dv_vlc_tab_t;
#else
typedef struct dv_vlc_tab_s {
  gint8 run;
  gint8 len;
  gint16 amp;
} dv_vlc_tab_t;
#endif

extern const gint8 *dv_vlc_classes[17];
extern const gint dv_vlc_class_index_mask[17];
extern const gint dv_vlc_class_index_rshift[17];
extern const dv_vlc_tab_t dv_vlc_broken[1];
extern const dv_vlc_tab_t *dv_vlc_lookups[6];
extern const gint dv_vlc_index_mask[6];
extern const gint dv_vlc_index_rshift[6];
extern const gint sign_lookup[2];
extern const gint sign_mask[17];
extern const gint sign_rshift[17];
extern void dv_construct_vlc_table();

// Note we assume bits is right (lsb) aligned, 0 < maxbits < 17
// This may look crazy, but there are no branches here.
#if ! INLINE_DECODE_VLC
extern void dv_decode_vlc(gint bits,gint maxbits, dv_vlc_t *result);
#else
extern __inline__ void dv_decode_vlc(gint bits,gint maxbits, dv_vlc_t *result) {
  static dv_vlc_t vlc_broken = {run: -1, amp: -1, len: VLC_NOBITS};
  dv_vlc_t *results[2] = { &vlc_broken, result };
  gint class, has_sign, amps[2];
  
  bits = bits << (16 - maxbits); // left align input
  class = dv_vlc_classes[maxbits][(bits & (dv_vlc_class_index_mask[maxbits])) >> (dv_vlc_class_index_rshift[maxbits])];
  *result = dv_vlc_lookups[class][(bits & (dv_vlc_index_mask[class])) >> (dv_vlc_index_rshift[class])];
  amps[1] = -(amps[0] = result->amp);
  has_sign = amps[0] > 0;
  result->len += has_sign;
  result->amp = amps[has_sign *  // or vlc not valid
		       ((sign_mask[result->len] & bits) >> sign_rshift[result->len])];
  *result = *results[maxbits >= result->len];
} // dv_decode_vlc
#endif // ! INLINE_DECODE_VLC

#if ! INLINE_DECODE_VLC
extern void __dv_decode_vlc(gint bits, dv_vlc_t *result);
#else
// Fastpath, assumes full 16bits are available, which eleminates left align of input,
// check for enough bits at end, and hardcodes lookups on maxbits.
extern __inline__ void __dv_decode_vlc(gint bits, dv_vlc_t *result) {
  gint class, has_sign, amps[2];
  
  class = dv_vlc_classes[16][(bits & (dv_vlc_class_index_mask[16])) >> (dv_vlc_class_index_rshift[16])];
  *result = dv_vlc_lookups[class][(bits & (dv_vlc_index_mask[class])) >> (dv_vlc_index_rshift[class])];
  amps[1] = -(amps[0] = result->amp);
  has_sign = amps[0] > 0;
  result->len += has_sign;
  result->amp = amps[has_sign *  // or vlc not valid
		    ((sign_mask[result->len] & bits) >> sign_rshift[result->len])];
} // __dv_decode_vlc
#endif // ! INLINE_DECODE_VLC

extern __inline__ void dv_peek_vlc(bitstream_t *bs,gint maxbits, dv_vlc_t *result) {
  if(maxbits < 16)
    dv_decode_vlc(bitstream_show(bs,maxbits),maxbits,result);
  else
    __dv_decode_vlc(bitstream_show(bs,16),result);
} // dv_peek_vlc

#endif // __VLC_H__
