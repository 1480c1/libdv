/* 
 *  parse.h
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

#ifndef __PARSE_H__
#define __PARSE_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#if HAVE_LIBPOPT
#include <popt.h>
#endif // HAVE_LIBPOPT

#include <glib.h>
#include "dv.h"

#define DV_VIDEO_OPT_BLOCK_QUALITY 0
#define DV_VIDEO_OPT_MONOCHROME    1
#define DV_VIDEO_OPT_CALLBACK      2
#define DV_VIDEO_NUM_OPTS          3

typedef struct {
  guint              quality;        
  gint               arg_block_quality; // default 3
  gint               arg_monochrome;

#ifdef HAVE_LIBPOPT
  struct poptOption  option_table[DV_VIDEO_NUM_OPTS+1]; 
#endif // HAVE_LIBPOPT

} dv_video_t;

extern dv_video_t *dv_video_new(void);

extern void dv_parse_init(void);

#endif // __PARSE_H__
