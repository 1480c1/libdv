/* 
 *  ppmqscale.c
 *
 *     Copyright (C) Peter Schlaile - February 2001
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

#include <dv_types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char* read_ppm_stream(FILE* f, int * width, int * height)
{
	char line[200];
	unsigned char* res;
	fgets(line, sizeof(line), f);
	do {
		fgets(line, sizeof(line), f); /* P6 */
	} while ((line[0] == '#') && !feof(f));
	if (feof(f)) {
		exit(0);
	}
	if (sscanf(line, "%d %d\n", width, height) != 2) {
		fprintf(stderr, "Bad PPM file!\n");
		exit(1);
	}
	fgets(line, sizeof(line), f);	/* 255 */
	
	res = (unsigned char*) malloc(*width * *height * 3);

	fread(res, 1, 3 * *width * *height, f);
	return res;
}

void write_ppm(FILE * f, unsigned char* outbuf, int width, int height)
{
	fprintf(f, "P6\n# CREATOR: ppmqscale\n%d %d\n255\n", 
		width, height);
	fwrite(outbuf, 3, width * height, f);
}


void enlarge_picture(unsigned char* src, unsigned char* dst, int src_width, 
		     int src_height, int dst_width, int dst_height)
{
	double ratiox = (double) (dst_width) / (double) (src_width);
	double ratioy = (double) (dst_height) / (double) (src_height);
	double ratio;
	unsigned long x_src, dx_src, x_dst;
	unsigned long y_src, dy_src, y_dst;
	unsigned long dst_scaled_width;
	unsigned long dst_scaled_height;

	if (ratiox > ratioy) {
		ratio = ratioy;
		dst += (int) ((dst_width - ratio * src_width) * 3 / 2);
#if 0
		fprintf(stderr, "Choosing Y ratio: %g, %d\n", ratio,
			(int) ((dst_width - ratio * src_width) * 3 / 2));
#endif
	} else {
		ratio = ratiox;
		dst += (int) ((dst_height - ratio * src_height) * 3 
			      * dst_width / 2);
#if 0
		fprintf(stderr, "Choosing X ratio: %g, %d\n", ratio,
			(int) ((dst_height - ratio * src_height) * 3 
			      * dst_width / 2));
#endif
	}

	dx_src = 65536.0 / ratio;
	dy_src = 65536.0 / ratio;
	dst_scaled_width = ratio * src_width;
	dst_scaled_height = ratio * src_height;
	y_src = 0;
	for (y_dst = 0; y_dst < dst_scaled_height; y_dst++) {
		unsigned char* line1 = src + (y_src >> 16) * 3 * src_width;
		unsigned char* line2 = line1 + 3 * src_width;
		unsigned long weight1y = 65536 - (y_src & 0xffff);
		unsigned long weight2y = 65536 - weight1y;
		       
		x_src = 0;
		for (x_dst = 0; x_dst < dst_scaled_width; x_dst++) {
			unsigned long weight1x = 65536 - (x_src & 0xffff);
			unsigned long weight2x = 65536 - weight1x;

			unsigned long x = (x_src >> 16) * 3;

#if 0			
			*dst++ = line1[x];
			*dst++ = line1[x + 1];
			*dst++ = line1[x + 2];
#else
			*dst++ = ((((line1[x] * weight1y) >> 16) 
				   * weight1x) >> 16)
				+ ((((line2[x] * weight2y) >> 16) 
				    * weight1x) >> 16)
				+ ((((line1[3 + x] * weight1y) >> 16) 
				   * weight2x) >> 16)
				+ ((((line2[3 + x] * weight2y) >> 16) 
				    * weight2x) >> 16);
			*dst++ = ((((line1[x + 1] * weight1y) >> 16) 
				   * weight1x) >> 16)
				+ ((((line2[x + 1] * weight2y) >> 16) 
				    * weight1x) >> 16)
				+ ((((line1[3 + x + 1] * weight1y) >> 16) 
				   * weight2x) >> 16)
				+ ((((line2[3 + x + 1] * weight2y) >> 16) 
				    * weight2x) >> 16);
			*dst++ = ((((line1[x + 2] * weight1y) >> 16) 
				   * weight1x) >> 16)
				+ ((((line2[x + 2] * weight2y) >> 16) 
				    * weight1x) >> 16)
				+ ((((line1[3 + x + 2] * weight1y) >> 16) 
				   * weight2x) >> 16)
				+ ((((line2[3 + x + 2] * weight2y) >> 16) 
				    * weight2x) >> 16);
#endif
			x_src += dx_src;
		}
		y_src += dy_src;
		dst += (dst_width - dst_scaled_width) * 3;
	}
}


void fast_scale(unsigned char* in, unsigned char* out, int in_width, 
		int in_height, int dst_width, int dst_height)
{
	memset(out, 0, dst_height * dst_width * 3);

	if (dst_width > in_width && dst_height > in_height) {
		enlarge_picture(in, out, in_width, in_height,
				dst_width, dst_height);
	} else if (dst_width == in_width && dst_height == in_height) {
		memcpy(out, in, dst_height * dst_width * 3);
	} else {
#if 0
		shrink_picture(in, out, in_width, in_height,
			       dst_width, dst_height);
#endif
	}
}

int main(int argc, const char** argv)
{
	int out_width, out_height;
	int in_width, in_height;
	unsigned char* in_pic;
	unsigned char* out_pic;

	if (argc < 3) {
		fprintf(stderr, "Usage: ppmqscale out_width out_height\n");
		exit(-1);
	}

	out_width = atoi(argv[1]);
	out_height = atoi(argv[2]);

	out_pic = (unsigned char*) malloc(out_width * out_height * 3);
	
	for (;;) {
		in_pic = read_ppm_stream(stdin, &in_width, &in_height);

		fast_scale(in_pic, out_pic, in_width, in_height,
			   out_width, out_height);
		write_ppm(stdout, out_pic, out_width, out_height);
		free(in_pic);
	}
}


