/* 
 *  oss.h
 *
 *     Copyright (C) Charles 'Buck' Krasic - January 2001
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

#ifndef DV_OSS_H
#define DV_OSS_H

#include <libdv/dv_types.h>
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

extern dv_oss_t *dv_oss_new(void);
extern gboolean  dv_oss_init (dv_audio_t *audio, dv_oss_t *oss);
extern gboolean  dv_oss_play (dv_audio_t *audio, dv_oss_t *oss, gint16 **out);
extern void      dv_oss_close(dv_oss_t *oss);

#ifdef __cplusplus
}
#endif

#endif /* DV_OSS_H */
