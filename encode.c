/* 
 *  encode.c
 *
 *     Copyright (C) James Bowman - July 2000
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

#include <dv_types.h>

#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "bitstream.h"
#include "dct.h"
#include "idct_248.h"
#include "quant.h"
#include "weighting.h"
#include "vlc.h"
#include "parse.h"
#include "place.h"
#include "mmx.h"

#define DV_WIDTH      720
#define DV_PAL_HEIGHT 576

#define MODE_STATISTICS 1
#define YUV_TESTING     0

/* #include "ycrcb_to_rgb32.h" */

extern gint     dv_super_map_vertical[5];
extern gint     dv_super_map_horizontal[5];

extern gint    dv_parse_bit_start[6];
extern gint    dv_parse_bit_end[6];

static inline void
dv_place_411_macroblock(dv_macroblock_t *mb) 
{
	gint mb_num; /* mb number withing the 6 x 5 zig-zag pattern  */
	gint mb_num_mod_6, mb_num_div_6; /* temporaries */
	gint mb_row;    /* mb row within sb (de-zigzag) */
	gint mb_col;    /* mb col within sb (de-zigzag) */
	/* Column offset of superblocks in macroblocks. */
	static const guint column_offset[] = {0, 4, 9, 13, 18};  
	
	/* Consider the area spanned super block as 30 element macroblock
	   grid (6 rows x 5 columns).  The macroblocks are laid out in a in
	   a zig-zag down and up the columns of the grid.  Of course,
	   superblocks are not perfect rectangles, since there are only 27
	   blocks.  The missing three macroblocks are either at the start
	   or end depending on the superblock column.
	
	   Within a superblock, the macroblocks start at the topleft corner
	   for even-column superblocks, and 3 down for odd-column
	   superblocks. */

	mb_num = ((mb->j % 2) == 1) ? mb->k + 3: mb->k;  
	mb_num_mod_6 = mb_num % 6;
	mb_num_div_6 = mb_num / 6;

	/* Compute superblock-relative row position (de-zigzag) */
	mb_row = ((mb_num_div_6 % 2) == 0) ? 
		mb_num_mod_6 : (5 - mb_num_mod_6); 
	/* Compute macroblock's frame-relative column position (in blocks) */
	mb_col = (mb_num_div_6 + column_offset[mb->j]) * 4;
	/* Compute frame-relative byte offset of macroblock's top-left corner
	   with special case for right-edge macroblocks */
	if(mb_col < (22 * 4)) {
		/* Convert from superblock-relative row position 
		   to frame relative (in blocks). */
		mb_row += (mb->i * 6); /* each superblock is 6 blocks high */
		/* Normal case */
	} else { 
		/* Convert from superblock-relative row position to 
		   frame relative (in blocks). */
		mb_row = mb_row * 2 + mb->i * 6; 
                /* each right-edge macroblock is 2 blocks high, 
		   and each superblock is 6 blocks high */
	}
	mb->x = mb_col * 8;
	mb->y = mb_row * 8;
} /* dv_place_411_macroblock */

static inline void 
dv_place_420_macroblock(dv_macroblock_t *mb) 
{
	gint mb_num; // mb number withing the 6 x 5 zig-zag pattern 
	gint mb_num_mod_3, mb_num_div_3; // temporaries
	gint mb_row;    // mb row within sb (de-zigzag)
	gint mb_col;    // mb col within sb (de-zigzag)
	// Column offset of superblocks in macroblocks.  
	static const guint column_offset[] = {0, 9, 18, 27, 36};  
	
	// Consider the area spanned super block as 30 element macroblock
	// grid (6 rows x 5 columns).  The macroblocks are laid out in a in
	// a zig-zag down and up the columns of the grid.  Of course,
	// superblocks are not perfect rectangles, since there are only 27
	// blocks.  The missing three macroblocks are either at the start
	// or end depending on the superblock column.
	
	// Within a superblock, the macroblocks start at the topleft corner
	// for even-column superblocks, and 3 down for odd-column
	// superblocks.
	mb_num = mb->k;  
	mb_num_mod_3 = mb_num % 3;
	mb_num_div_3 = mb_num / 3;
	// Compute superblock-relative row position (de-zigzag)
	mb_row = ((mb_num_div_3 % 2) == 0) ? mb_num_mod_3 : (2 - mb_num_mod_3); 
	// Compute macroblock's frame-relative column position (in blocks)
	mb_col = mb_num_div_3 + column_offset[mb->j];
	// Compute frame-relative byte offset of macroblock's top-left corner
	// Convert from superblock-relative row position to frame relative (in blocks).
	mb_row += (mb->i * 3); // each right-edge macroblock is 2 blocks high, and each superblock is 6 blocks high
	mb->x = mb_col * 16;
	mb->y = mb_row * 16;
} // dv_place_420_macroblock

static guint put_bits(unsigned char *s, guint offset, guint len, guint value)
{
	s += (offset >> 3);
	value <<= (24 - len);
	value &= 0xffffff;
	value >>= (offset & 7);
	s[0] |= (value >> 16) & 0xff;
	s[1] |= (value >> 8) & 0xff;
	s[2] |= value & 0xff;
	return offset + len;
}

inline gint f2b(float f)
{
	int b = rint(f);
	if (b < 0)
		b = 0;
	if (b > 255)
		b = 255;
	
	return b;
}


inline gint f2sb(float f)
{
	int b = rint(f);
	
	return b;
}

static void dump_block(  char *tag, dv_block_t *bl)
{
	int i, j;
	printf("%s\n", tag);
	for (j = 0; j < 8; j++) {
		printf("{ ");
		for (i = 0; i < 8; i++) {
			printf("%4d, ", bl->coeffs[8 * j + i]);
		}
		printf(" },\n");
	}
}

typedef struct {
	gint8 run;
	gint16 amp;
	guint16 val;
	guint8 len;
} dv_vlc_encode_t;

static dv_vlc_encode_t dv_vlc_test_table[89] = {
	{ 0, 1, 0x0, 2 },
	{ 0, 2, 0x2, 3 },
	{-1, 0, 0x6, 4 },
	{ 1, 1, 0x7, 4 },
	{ 0, 3, 0x8, 4 },
	{ 0, 4, 0x9, 4 },
	{ 2, 1, 0x14, 5 },
	{ 1, 2, 0x15, 5 },
	{ 0, 5, 0x16, 5 },
	{ 0, 6, 0x17, 5 },
	{ 3, 1, 0x30, 6 },
	{ 4, 1, 0x31, 6 },
	{ 0, 7, 0x32, 6 },
	{ 0, 8, 0x33, 6 },
	{ 5, 1,  0x68, 7 },
	{ 6, 1,  0x69, 7 },
	{ 2, 2,  0x6a, 7 },
	{ 1, 3,  0x6b, 7 },
	{ 1, 4,  0x6c, 7 },
	{ 0, 9,  0x6d, 7 },
	{ 0, 10, 0x6e, 7 },
	{ 0, 11, 0x6f, 7 },
	{ 7,  1,  0xe0, 8 },
	{ 8,  1,  0xe1, 8 },
	{ 9,  1,  0xe2, 8 },
	{ 10, 1,  0xe3, 8 },
	{ 3,  2,  0xe4, 8 },
	{ 4,  2,  0xe5, 8 },
	{ 2,  3,  0xe6, 8 },
	{ 1,  5,  0xe7, 8 },
	{ 1,  6,  0xe8, 8 },
	{ 1,  7,  0xe9, 8 },
	{ 0,  12, 0xea, 8 },
	{ 0,  13, 0xeb, 8 },
	{ 0,  14, 0xec, 8 },
	{ 0,  15, 0xed, 8 },
	{ 0,  16, 0xee, 8 },
	{ 0,  17, 0xef, 8 },
	{ 11, 1,  0x1e0, 9 },
	{ 12, 1,  0x1e1, 9 },
	{ 13, 1,  0x1e2, 9 },
	{ 14, 1,  0x1e3, 9 },
	{ 5,  2,  0x1e4, 9 },
	{ 6,  2,  0x1e5, 9 },
	{ 3,  3,  0x1e6, 9 },
	{ 4,  3,  0x1e7, 9 },
	{ 2,  4,  0x1e8, 9 },
	{ 2,  5,  0x1e9, 9 },
	{ 1,  8,  0x1ea, 9 },
	{ 0,  18, 0x1eb, 9 },
	{ 0,  19, 0x1ec, 9 },
	{ 0,  20, 0x1ed, 9 },
	{ 0,  21, 0x1ee, 9 },
	{ 0,  22, 0x1ef, 9 },
	{ 5, 3,  0x3e0, 10 },
	{ 3, 4,  0x3e1, 10 },
	{ 3, 5,  0x3e2, 10 },
	{ 2, 6,  0x3e3, 10 },
	{ 1, 9,  0x3e4, 10 },
	{ 1, 10, 0x3e5, 10 },
	{ 1, 11, 0x3e6, 10 },
	{ 0, 0,  0x7ce, 11 },
	{ 1, 0,  0x7cf, 11 },
	{ 6, 3,  0x7d0, 11 },
	{ 4, 4,  0x7d1, 11 },
	{ 3, 6,  0x7d2, 11 },
	{ 1, 12, 0x7d3, 11 },
	{ 1, 13, 0x7d4, 11 },
	{ 1, 14, 0x7d5, 11 },
	{ 2, 0, 0xfac, 12 },
	{ 3, 0, 0xfad, 12 },
	{ 4, 0, 0xfae, 12 },
	{ 5, 0, 0xfaf, 12 },
	{ 7,  2,  0xfb0, 12 },
	{ 8,  2,  0xfb1, 12 },
	{ 9,  2,  0xfb2, 12 },
	{ 10, 2,  0xfb3, 12 },
	{ 7,  3,  0xfb4, 12 },
	{ 8,  3,  0xfb5, 12 },
	{ 4,  5,  0xfb6, 12 },
	{ 3,  7,  0xfb7, 12 },
	{ 2,  7,  0xfb8, 12 },
	{ 2,  8,  0xfb9, 12 },
	{ 2,  9,  0xfba, 12 },
	{ 2,  10, 0xfbb, 12 },
	{ 2,  11, 0xfbc, 12 },
	{ 1,  15, 0xfbd, 12 },
	{ 1,  16, 0xfbe, 12 },
	{ 1,  17, 0xfbf, 12 },
}; // dv_vlc_test_table

static dv_vlc_encode_t * vlc_test_lookup[512];

static void init_vlc_test_lookup()
{
	int i;
	memset(vlc_test_lookup, 0, 512 * sizeof(dv_vlc_encode_t*));
	for (i = 0; i < 89; i++) {
		dv_vlc_encode_t *pvc = &dv_vlc_test_table[i];
		vlc_test_lookup[((pvc->run + 1) << 5) + pvc->amp] = pvc;
	}
}

static inline dv_vlc_encode_t * find_vlc_entry(int run, int amp)
{
	if (run < -1 || run > 14 || amp < 0 || amp > 22) {
		return NULL;
	} else {
		return vlc_test_lookup[((run + 1) << 5) + amp];
	}
}

static guint vlc_encode(unsigned char *vsbuffer, guint bit_offset,
			int run, int amp, int sign)
{
	dv_vlc_encode_t *hit = find_vlc_entry(run, amp);
	
	if (hit != NULL) {
		/* 1111110 */
		bit_offset = put_bits(vsbuffer, bit_offset,hit->len,hit->val);
		
		if (amp != 0)
			bit_offset = put_bits(vsbuffer, bit_offset, 1, sign);
	} else {
		if ((62 <= run) && (run <= 63) && (amp == 0)) {
			bit_offset = vlc_encode(vsbuffer, bit_offset, 1, 0, 0);
			bit_offset = vlc_encode(vsbuffer, bit_offset, 
						run - 2, 0, 0);
		} else if ((6 <= run) && (run <= 61) && (amp == 0)) {
			/* 1111110 */
			bit_offset = put_bits(vsbuffer, bit_offset, 7, 0x7e); 
			bit_offset = put_bits(vsbuffer, bit_offset, 6, run);
		} else if ((23 <= amp) && (amp <= 255) && (run == 0)) {
			/* 1111111 */
			bit_offset = put_bits(vsbuffer, bit_offset, 7, 0x7f); 
			bit_offset = put_bits(vsbuffer, bit_offset, 8, amp);
			bit_offset = put_bits(vsbuffer, bit_offset, 1, sign);
		} else {
			bit_offset = vlc_encode(vsbuffer, bit_offset, 
						run - 1, 0, 0);
			bit_offset = vlc_encode(vsbuffer, bit_offset, 
						0, amp, sign);
		}
	}
	return bit_offset;
}

static inline guint vlc_num_bits(int run, int amp, int sign)
{
	dv_vlc_encode_t *hit;
	
	assert((0 <= amp) && (amp <= 255));
	assert((0 <= run) && (run <= 63));

	hit = find_vlc_entry(run, amp);
	if (hit != NULL) {
		return (hit->len + ((amp != 0) ? 1 : 0));
	} else {
		if ((62 <= run) && (run <= 63) && (amp == 0)) {
			return    vlc_num_bits(1, 0, 0) 
				+ vlc_num_bits(run - 2, 0, 0);
		} else if ((6 <= run) && (run <= 61) && (amp == 0)) {
			return (7 + 6);
		} else if ((23 <= amp) && (amp <= 255) && (run == 0)) {
			return (7 + 8 + 1);
		} else {
			return    vlc_num_bits(run - 1, 0, 0) 
				+ vlc_num_bits(0, amp, sign);
		}
	}
}


static const int reorder_88[64] = {
	1, 2, 6, 7,15,16,28,29,
	3, 5, 8,14,17,27,30,43,
	4, 9,13,18,26,31,42,44,
	10,12,19,25,32,41,45,54,
	11,20,24,33,40,46,53,55,
	21,23,34,39,47,52,56,61,
	22,35,38,48,51,57,60,62,
	36,37,49,50,58,59,63,64
};
static const int reorder_248[64] = {
	1, 3, 7,19,21,35,37,51,
	5, 9,17,23,33,39,49,53,
	11,15,25,31,41,47,55,61,
	13,27,29,43,45,57,59,63,

	2, 4, 8,20,22,36,38,52,
	6,10,18,24,34,40,50,54,
	12,16,26,32,42,48,56,62,
	14,28,30,44,46,58,60,64
};

guint vlc_encode_block(unsigned char *vsbuffer, guint bit_offset, 
		       dv_block_t *bl, guint bit_budget)
{
	const int *reorder;
	dv_coeff_t zigzag[64];
	int i;
	int run, amp, sign;
	guint start_offset;
	gint bits_left = bit_budget;
	
	start_offset = bit_offset;
	
	if (bl->dct_mode == DV_DCT_88)
		reorder = reorder_88;
	else
		reorder = reorder_248;
	
	for (i = 0; i < 64; i++)
		zigzag[reorder[i] - 1] = bl->coeffs[i];
	
	i = 1;			/* first AC coeff */
	
	do {
		run = 0;
		while ((zigzag[i] == 0) && (i < 64)) {
			i++;
			run++;
		}
		if (i < 64) {
			if (zigzag[i] < 0) {
				amp = -zigzag[i];
				sign = 1;
			} else {
				amp = zigzag[i];
				sign = 0;
			}
			i++;
			
			//printf("run=%d, amp=%d, sign=%d\n", run, amp, sign);
			bits_left -= vlc_num_bits(run, amp, sign);
			if (bits_left < 4)
				break;
			
			bit_offset = vlc_encode(vsbuffer, bit_offset, 
						run, amp, sign);
		}
	} while (i < 64);
	
	bit_offset = put_bits(vsbuffer, bit_offset, 4, 0x6); /* EOB */
	
	assert((bit_offset - start_offset) <= bit_budget);
	
	return bit_offset;
}


guint vlc_num_bits_block(dv_block_t *bl)
{
	const int *reorder;
	dv_coeff_t zigzag[64];
	int i;
	int run, amp, sign;
	guint num_bits = 0;
	
	if (bl->dct_mode == DV_DCT_88)
		reorder = reorder_88;
	else
		reorder = reorder_248;
	
	for (i = 0; i < 64; i++)
		zigzag[reorder[i] - 1] = bl->coeffs[i];
	
	i = 1;			/* first AC coeff */
	
	do {
		run = 0;
		while ((zigzag[i] == 0) && (i < 64)) {
			i++;
			run++;
		}
		if (i < 64) {
			if (zigzag[i] < 0) {
				amp = -zigzag[i];
				sign = 1;
			} else {
				amp = zigzag[i];
				sign = 0;
			}
			i++;
			
			num_bits += vlc_num_bits(run, amp, sign);
		}
	} while (i < 64);
	
	return num_bits;
}

static int maxamp(dv_block_t *bl)
{
	int i;
	int a, max = 0;
	
	for (i = 0; i < 64; i++) {
		a = abs(bl->coeffs[i]);
		if (a > max)
			max = a;
	}
	return max;
}

void read_ppm(const char* filename, unsigned char* readbuf, int * isPAL,
	      int * height_)
{
	FILE *f;
	int height, width;
	char line[200];
	/* Read the PPM */
	f = fopen(filename, "r");
	if (f == NULL) {
		perror(filename);
		exit(1);
	}
	fgets(line, sizeof(line), f);
	do {
		fgets(line, sizeof(line), f); /* P6 */
	} while ((line[0] == '#') && !feof(f));
	if (sscanf(line, "%d %d\n", &width, &height) != 2) {
		fprintf(stderr, "%s: Bad PPM file\n", filename);
		exit(1);
	}
	if (width != DV_WIDTH || height > DV_PAL_HEIGHT) {
		fprintf(stderr, "%s: width != DV_WIDTH "
			"|| height > DV_PAL_HEIGHT!\n", filename);
		exit(1);
	}
	fgets(line, sizeof(line), f);	/* 255 */
	
	fread(readbuf, 1, 3 * DV_WIDTH * height, f);
	fclose(f);

	*height_ = height;
	*isPAL = (height == DV_PAL_HEIGHT);
}

extern void rgbtoyuv_mmx(unsigned char* inPtr, int rows, int columns,
			 short* outyPtr, short* outuPtr, short* outvPtr);

#if YUV_TESTING
static void test_yuv(short* img, const char* name, int width, int height)
{
	int x,y,v;
	int min_v, max_v;

	fprintf(stderr, "\n%s testing:\nOuch: ", name);
	min_v = 32767;
	max_v = -32767;
	for (y = 0;y < height; y++) {
		for (x=0; x < width; x++) {
			v = img[y * width + x];
			if (v > 2040 || v < -2040) {
				fprintf(stderr, "%d,%d ", x, y);
			}
			if (v < min_v) min_v = v;
			if (v > max_v) max_v = v;
		}
	}
	fprintf(stderr, "\nMinmax: %d, %d\n", min_v, max_v);
}
#endif

static void convert_to_yuv(unsigned char* img_rgb, int height,
			   short* img_y, short* img_cr, short* img_cb)
{
#if !ARCH_X86
	int x,y;
	/* This is the RGB -> YUV color matrix */
	static const float cm[3][3] = {
		{ 0.256951160416, 0.504420728197,  0.0977346405969    },
		{ 0.439165945662, -0.367885794249, -0.0712801514127   },
		{ -0.148211670329, -0.290954275333,  0.439165945662  }
	};

	double tmp_cr[DV_PAL_HEIGHT][DV_WIDTH];
	double tmp_cb[DV_PAL_HEIGHT][DV_WIDTH];
	double fac = pow(2, DCT_YUV_PRECISION);
	for (y = 0; y < height; y++) {
		for (x = 0; x < DV_WIDTH; x++) {
			register double cy, cr, cb;
			register int r = img_rgb[(y * DV_WIDTH + x)*3 + 0];
			register int g = img_rgb[(y * DV_WIDTH + x)*3 + 1];
			register int b = img_rgb[(y * DV_WIDTH + x)*3 + 2];
			cy =    (cm[0][0] * r) + (cm[0][1] * g) +
				(cm[0][2] * b);
			cr =    (cm[1][0] * r) + (cm[1][1] * g) +
				(cm[1][2] * b);
			cb =    (cm[2][0] * r) + (cm[2][1] * g) +
				(cm[2][2] * b);
			
			img_y[y * DV_WIDTH + x] = 
				(f2sb(cy) - 128 + 16) * fac;
			tmp_cr[y][x] = cr * fac;
			tmp_cb[y][x] = cb * fac;
			//printf("%02x\n", img_y[y][x]);
		}
	}
	for (y = 0; y < height; y++) {
		for (x = 0; x < DV_WIDTH / 4; x++) {
			img_cr[y * DV_WIDTH/4  + x] = 
				f2sb((tmp_cr[y][4*x] +
				      tmp_cr[y][4*x+1] +
				      tmp_cr[y][4*x+2] +
				      tmp_cr[y][4*x+3]) / 4.0);
			img_cb[y * DV_WIDTH/4 + x] = 
				f2sb((tmp_cb[y][4*x] +
				      tmp_cb[y][4*x+1] +
				      tmp_cb[y][4*x+2] +
				      tmp_cb[y][4*x+3]) / 4.0);
		}
	}
#else
#if YUV_TESTING
	int x,y;
#endif
	rgbtoyuv_mmx(img_rgb, height, DV_WIDTH, (short*) img_y,
		     (short*) img_cr, (short*) img_cb);
	emms();
#if YUV_TESTING
	test_yuv(img_y, "Y", DV_WIDTH, height);
	test_yuv(img_cr, "CR", DV_WIDTH/4, height);
	test_yuv(img_cb, "CB", DV_WIDTH/4, height);

	fprintf(stderr, "\n");
	fprintf(stderr, "Y: 300-320\n");
	for (y = 300; y < 320; y++) {
		for (x = 200; x < 200+50; x++) {
			fprintf(stderr, "%d\t", img_y[y * DV_WIDTH + x]);
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "CR: 300-320\n");
	for (y = 300; y < 320; y++) {
		for (x = 50; x < 100; x++) {
			fprintf(stderr, "%d\t", img_cr[y * DV_WIDTH/4 + x]);
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "CB: 300-320\n");

	for (y = 300; y < 320; y++) {
		for (x = 50; x < 100; x++) {
			fprintf(stderr, "%d\t", img_cb[y * DV_WIDTH/4 + x]);
		}
		fprintf(stderr, "\n");
	}
#endif
#endif
}

static void build_coeff(short* img_y, short* img_cr, short* img_cb,
			       dv_macroblock_t *mb)
{
	int i,j;
	int y = mb->y;
	int x = mb->x;
		
	if((mb->j == 4) && (mb->k > 23)) {
		for (j = 0; j < 8; j++)
			for (i = 0; i < 8; i++) {
				mb->b[0].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH +  x + i];
				mb->b[1].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH +  x + 8 + i];
				mb->b[2].coeffs[8 * i + j] = 
					img_y[(y + 8 + j) * DV_WIDTH + x + i];
				mb->b[3].coeffs[8 * i + j] = 
					img_y[(y + 8 + j) * DV_WIDTH + x + 8 + i];
				mb->b[4].coeffs[8 * i + j] = 
					img_cr[(y + 2 * j) * DV_WIDTH/4 + x / 4 + i / 2];
				mb->b[5].coeffs[8 * i + j] = 
					img_cb[(y + 2 * j) * DV_WIDTH/4 + x / 4 + i / 2];
			}
	} else {
		for (j = 0; j < 8; j++)
			for (i = 0; i < 8; i++) {
				mb->b[0].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH + x + i];
				mb->b[1].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH + x + 8 + i];
				mb->b[2].coeffs[8 * i + j] = 
					img_y[(y + 8 + j) * DV_WIDTH + x +  i];
				mb->b[3].coeffs[8 * i + j] = 
					img_y[(y + 8 + j) * DV_WIDTH + x + 8 + i];
				mb->b[4].coeffs[8 * i + j] = 
					img_cr[(y + j) * DV_WIDTH/4 + x / 4 + i];
				mb->b[5].coeffs[8 * i + j] = 
					img_cb[(y + j) * DV_WIDTH/4 + x / 4 + i];
			}
	}
}

static void do_dct(dv_macroblock_t *mb)
{
	guint b;

	for (b = 0; b < 6; b++) {
		dv_block_t *bl = &mb->b[b];
		static short tblock[64] __attribute__ ((aligned (64)));
		
		memcpy(tblock, &bl->coeffs, 64*sizeof(short));
		
		bl->dct_mode = DV_DCT_88;
		if (bl->dct_mode == DV_DCT_88) {
			dct_88(tblock, bl->coeffs);
			weight_88(bl->coeffs);
		} else {
			dct_248(bl->coeffs);
			weight_248(bl->coeffs);
		}
	}
}

#if MODE_STATISTICS
long modes_used[50];
#endif

static void do_quant(dv_macroblock_t * mb)
{
	guint b;
	gint i, j;
	gint class;

	struct {
		int qno, class;
	} modes[] = {
#if 0	/* XXX - this table should give better results; 
	   don't know why it doesn't */
	   { 15, 0 },	/* 1 1 1 1 */
	   { 8, 0 },	/* 1 1 1 2 */
	   { 7, 0 },	/* 1 1 2 2 */
	   { 5, 0 },	/* 1 2 2 4 */
	   { 15, 3 },	/* 2 2 2 2 */
	   { 13, 3 },	/* 2 2 2 4 */
	   { 3, 0 },	/* 2 2 4 4 */
	   { 1, 0 },	/* 2 4 4 8 */
	   { 2, 1 },	/* 4 4 8 8 */
	   { 0, 1 },	/* 4 8 8 16 */
	   { 1, 2 },	/* 8 8 16 16 */
	   { 2, 3 },	/* 8 16 16 32 */
	   { 0, 3 }	/* 16 16 32 32 */
#else
	   { 15, 0 },	/* 1 1 1 1 */
	   { 15, 3 },
	   { 14, 3 },
	   { 13, 3 },
	   { 12, 3 },
	   { 11, 3 },
	   { 10, 3 },
	   { 9, 3 },
	   { 8, 3 },
	   { 7, 3 },
	   { 6, 3 },
	   { 5, 3 },
	   { 4, 3 },
	   { 3, 3 },
	   { 2, 3 },
	   { 1, 3 },
	   { 0, 3 },
#endif
	};
	int num_modes = sizeof(modes) / sizeof(modes[0]);
	int highest_mode = 0;
	
	for (b = 0; b < 6; b++) {
		dv_block_t *bl = &mb->b[b], bb;
		int mode;
		guint ac_coeff_budget = ((b < 4) ? 100 : 68) - 4;
		
		for (mode = 0; mode < num_modes; mode++) {
			bb = *bl;
#if ARCH_X86
			quant_88_x86(bb.coeffs, modes[mode].qno,
				     modes[mode].class);
#else
			quant_88(bb.coeffs, modes[mode].qno,modes[mode].class);
#endif
			if ((maxamp(&bb) < 256) &&
			    (vlc_num_bits_block(&bb) < ac_coeff_budget))
				break;
		}
		if (mode == num_modes) {
			/* Nothing good enough found: make coeffs as small as
			   possible */
			mode = num_modes - 1;
		}
		if (mode > highest_mode )
			highest_mode = mode;
	}
#if MODE_STATISTICS
	modes_used[highest_mode]++;
#endif
	mb->qno = modes[highest_mode].qno;
	class = modes[highest_mode].class;

	for (b = 0; b < 6; b++) {
		dv_block_t *bl = &mb->b[b];
		
		bl->class_no = class;
		if (bl->dct_mode == DV_DCT_88)
#if ARCH_X86
			quant_88_x86(bl->coeffs, mb->qno, bl->class_no);
#else
			quant_88(bl->coeffs, mb->qno, bl->class_no);
#endif
		else
			quant_248(bl->coeffs, mb->qno, bl->class_no);
	}
}

static void process_videosegment(
	short* img_y, short* img_cr, short* img_cb,
	dv_videosegment_t* videoseg,
	guint8 * vsbuffer)
{
	dv_macroblock_t *mb;
	gint m;

	/* stage 2: dequant/unweight/iDCT blocks, 
	   and place the macroblocks */
	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		guint b;
		gint i, j;
		gint class;
		
		mb->vlc_error = 0;
		mb->eob_count = 0;
		mb->i = (videoseg->i+ dv_super_map_vertical[m]) 
			% (videoseg->isPAL ? 12 : 10);
		mb->j = dv_super_map_horizontal[m];
		mb->k = videoseg->k;
		
		dv_place_420_macroblock(mb);
		build_coeff(img_y, img_cr, img_cb, mb);
		do_dct(mb);
		do_quant(mb);
		
		put_bits(vsbuffer, (8 * (80 * m)) + 28, 4, mb->qno);
		
		for (b = 0; b < 6; b++) {
			dv_block_t *bl = &mb->b[b];
			guint bit_offset = (8 * (80 * m)) 
				+ dv_parse_bit_start[b] - 12;
			guint ac_coeff_budget = (b < 4) ? 100 : 68;
			
			bit_offset = put_bits(vsbuffer, bit_offset, 9, bl->coeffs[0]);
			bit_offset = put_bits(vsbuffer, bit_offset, 1, bl->dct_mode);
			bit_offset = put_bits(vsbuffer, bit_offset, 2, bl->class_no);
			bit_offset = vlc_encode_block(vsbuffer, bit_offset, bl, ac_coeff_budget);
		}
	}
}

static void encode(short* img_y, short* img_cr, short* img_cb, 
		   int isPAL, unsigned char* target)
{
	static dv_videosegment_t videoseg __attribute__ ((aligned (64)));

	gint numDIFseq;
	gint ds;
	gint v;
	guint dif;
	guint offset;

	memset(target, 0, 144000);

	dif = 0;
	offset = dif * 80;
	if (isPAL) {
		target[offset + 3] |= 0x80;
	}

	numDIFseq = isPAL ? 12 : 10;

	for (ds = 0; ds < numDIFseq; ds++) { 
		/* Each DIF segment conists of 150 dif blocks, 
		   135 of which are video blocks */
		dif += 6; // skip the first 6 dif blocks in a dif sequence 
		/* A video segment consists of 5 video blocks, where each video
		   block contains one compressed macroblock.  DV bit allocation
		   for the VLC stage can spill bits between blocks in the same
		   video segment.  So parsing needs the whole segment to decode
		   the VLC data */
		/* Loop through video segments */
		for (v = 0; v < 27; v++) {
			/* skip audio block - 
			   interleaved before every 3rd video segment
			*/

			if(!(v % 3)) dif++; 

			offset = dif * 80;
			
			videoseg.i = ds;
			videoseg.k = v;
			videoseg.isPAL = isPAL;

			process_videosegment(img_y, img_cr, img_cb, 
					     &videoseg, target + offset);
			
			dif += 5;
		} 
	} 
}




int main(int argc, char *argv[])
{
	int i;
	gint arg_index;
	
	weight_init();  
	dct_init();
	dv_dct_248_init();
	dv_construct_vlc_table();
	dv_parse_init();
	dv_place_init();
	init_vlc_test_lookup();

#if MODE_STATISTICS
	memset(modes_used,0,sizeof(modes_used));
#endif
	
	for (arg_index = 1; arg_index < argc; arg_index++) {
		unsigned char readbuf[DV_PAL_HEIGHT * DV_WIDTH * 3];
		unsigned char target[144000];
		int height;
		int isPAL;
		
		short img_y[DV_PAL_HEIGHT * DV_WIDTH];
		short img_cr[DV_PAL_HEIGHT * DV_WIDTH / 4];
		short img_cb[DV_PAL_HEIGHT * DV_WIDTH / 4];

		read_ppm(argv[arg_index], readbuf, &isPAL, &height);
		convert_to_yuv(readbuf, height, img_y, img_cr, img_cb);
		encode(img_y, img_cr, img_cb, isPAL, target);
		fwrite(target, 1, isPAL ? 144000 : 120000, stdout);
	}
  
#if MODE_STATISTICS
	fprintf(stderr, "MODE_STATISTICS:\n");
	for (i = 0; i < 50; i++) {
		fprintf(stderr, "%d: %d\n", i, modes_used[i]);
	}
#endif

	return 0;
}

