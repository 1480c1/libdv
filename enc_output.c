/* 
 *  enc_output_filters
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
 
#include "enc_output.h"
#include "headers.h"

#include <stdio.h>
#include <stdlib.h>

static int raw_init()
{
	return 0;
}

static void raw_finish()
{

}

static int frame_counter = 0;

static int raw_store(unsigned char* encoded_data, int isPAL, time_t now)
{
	write_info_blocks(encoded_data, frame_counter, isPAL, &now);
	fwrite(encoded_data, 1, isPAL ? 144000 : 120000, stdout);
	frame_counter++;
	return 0;
}

static dv_enc_output_filter_t filters[DV_ENC_MAX_OUTPUT_FILTERS] = {
	{ raw_init, raw_finish, raw_store, "raw" },
	{ NULL, NULL, NULL, NULL }};

void dv_enc_register_output_filter(dv_enc_output_filter_t filter)
{
	dv_enc_output_filter_t * p = filters;
	while (p->filter_name) p++;
	*p = filter;
}

int get_dv_enc_output_filters(dv_enc_output_filter_t ** filters_,
			      int * count)
{
	dv_enc_output_filter_t * p = filters;

	*count = 0;
	while (p->filter_name) p++, (*count)++;

	*filters_ = filters;
	return 0;
}



