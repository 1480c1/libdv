/* 
 *  encode.h
 *
 *     Copyright (C) Peter Schlaile - Feb 2001
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
 
#ifndef DV_ENCODE_H
#define DV_ENCODE_H

#include "dct.h"

#define DV_WIDTH       720
#define DV_PAL_HEIGHT  576
#define DV_NTSC_HEIGHT 480

/* FIXME: Just guessed! */
#define DCT_248_THRESHOLD ((8 * 8 * 8) << DCT_YUV_PRECISION)
#define DCT_248_THRESMASK ((~63)       << DCT_YUV_PRECISION)

#endif
