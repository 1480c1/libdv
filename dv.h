/* 
 *  dv.h
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

#ifndef DV_H
#define DV_H


#include <stdlib.h>
#include <stdio.h>
#include <bitstream.h>

#include "dv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if HAVE_LIBPOPT
extern inline void
dv_opt_usage(poptContext con, struct poptOption *opt, gint num)
{
  struct poptOption *o = opt + num;
  if(o->shortName && o->longName) {
    fprintf(stderr,"-%c, --%s", o->shortName, o->longName);
  } else if(o->shortName) {
    fprintf(stderr,"-%c", o->shortName);
  } else if(o->longName) {
    fprintf(stderr,"--%s", o->longName);
  } // if
  if(o->argDescrip) {
    fprintf(stderr, "=%s\n", o->argDescrip);
  } else {
    fprintf(stderr, ": invalid usage\n");
  } // else
  exit(-1);
} // dv_opt_usage
#endif // HAVE_LIBPOPT

/* Main API */
extern dv_decoder_t *dv_decoder_new(void);
extern void dv_init(dv_decoder_t *dv);
extern gint dv_parse_header(dv_decoder_t *dv, guchar *buffer);
extern void dv_decode_full_frame(dv_decoder_t *dv, guchar *buffer, 
				 dv_color_space_t color_space, guchar **pixels, guint16 *pitches);

extern gint dv_decode_full_audio(dv_decoder_t *dv, guchar *buffer, gint16 **outbufs);

/* Low level API */
extern gint dv_parse_video_segment(dv_videosegment_t *seg, guint quality);
extern void dv_decode_video_segment(dv_decoder_t *dv, dv_videosegment_t *seg, guint quality);

extern void dv_render_video_segment_rgb(dv_decoder_t *dv, dv_videosegment_t *seg, 
					guchar *pixels, gint pitch );

extern void dv_render_video_segment_yuv(dv_decoder_t *dv, dv_videosegment_t *seg, 
					guchar **pixels, guint16 *pitches);
#ifdef __cplusplus
}
#endif

#endif // DV_H 
