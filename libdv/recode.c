/* 
 *  recode.c
 *
 *     Copyright (C) Dan Dennedy, Peter Schlaile
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

#include <dv.h>
#include <dv_types.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int read_frame(FILE* in_vid, unsigned char* frame_buf, int * isPAL)
{
        if (fread(frame_buf, 1, 120000, in_vid) != 120000) {
                return 0;
        }

        *isPAL = (frame_buf[3] & 0x80);

        if (*isPAL) {
                if (fread(frame_buf + 120000, 1, 144000 - 120000, in_vid) !=
                    144000 - 120000) {
                        return 0;
                }
        }
        return 1;
}

int main( int argc, char **argv)
{
	int infile = 0;
	unsigned char dv_buffer[144000];
	unsigned char video_buffer[720 * 576 * 3];
	gint16 *audio_bufs[4];
	dv_decoder_t *decoder = NULL;
	int pitches[3];
	unsigned char *pixels[3];
	int i = 0;
	int isPAL = 0;

	pitches[0] = 720 * 3;
	pixels[0] = video_buffer;
	
	for(i = 0; i < 4; i++) {
		audio_bufs[i] = malloc(DV_AUDIO_MAX_SAMPLES*sizeof(gint16));
	}

	decoder = dv_decoder_new();
	
	dv_init();
	decoder->quality = DV_QUALITY_BEST;

	i = 0;
	while (read_frame(stdin, dv_buffer, &isPAL)) {
		dv_parse_header(decoder, dv_buffer);
		dv_decode_full_frame(decoder, dv_buffer, e_dv_color_rgb, 
				     pixels, pitches);
		dv_decode_full_audio(decoder, dv_buffer, audio_bufs);

		dv_encode_full_frame(pixels, dv_buffer, e_dv_color_rgb,
			isPAL, 0, 3, 0, DV_DCT_AUTO);
		dv_encode_full_audio(dv_buffer, 0, audio_bufs, 2, 48000);
		fwrite(dv_buffer, 1, (isPAL ? 144000 : 120000), stdout);
	}

	close(infile);
	
	for(i=0; i < 4; i++) free(audio_bufs[i]);
	if (decoder->video) free(decoder->video);
	if (decoder->audio) free(decoder->audio);

	return 0;
}



