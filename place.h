/* 
 *  place.h
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

#ifndef __PLACE_H__
#define __PLACE_H__

#include <stddef.h>
#include <glib.h>

extern void dv_place_init(void);
extern void dv_place_macroblock(dv_decoder_t *dv, dv_videosegment_t *seg, dv_macroblock_t *mb, gint m);
extern void dv_place_video_segment(dv_decoder_t *dv, dv_videosegment_t *seg);
extern void dv_place_frame(dv_decoder_t *dv, dv_frame_t *frame);

#endif // __PLACE_H__
