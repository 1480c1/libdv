/* 
 *  encode.c
 *
 *     Copyright (C) James Bowman  - July 2000
 *                   Peter Schlaile - Jan 2001
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

#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "dct.h"
#include "idct_248.h"
#include "quant.h"
#include "weighting.h"
#include "vlc.h"
#include "parse.h"
#include "place.h"
#include "mmx.h"
#include "headers.h"

#define DV_WIDTH       720
#define DV_PAL_HEIGHT  576
#define DV_NTSC_HEIGHT 480
#define BITS_PER_LONG  32

/* FIXME: Just guessed! */
#define DCT_248_THRESHOLD ((8 * 8 * 8) << DCT_YUV_PRECISION)
#define DCT_248_THRESMASK ((~63)       << DCT_YUV_PRECISION)

typedef struct dv_vlc_entry_s {
	unsigned long val; /* Can only be 24 bits wide otherwise it can't be
			      handled by put_bits */
	long len;
} dv_vlc_entry_t;

typedef struct dv_vlc_block_s {
	dv_vlc_entry_t coeffs[128];
	dv_vlc_entry_t * coeffs_end;
	dv_vlc_entry_t * coeffs_start;
	guint bit_offset;
	long bit_budget;
	gboolean can_supply;
} dv_vlc_block_t;

/* #include "ycrcb_to_rgb32.h" */

extern gint     dv_super_map_vertical[5];
extern gint     dv_super_map_horizontal[5];

extern gint    dv_parse_bit_start[6];
extern gint    dv_parse_bit_end[6];

static long cycles_used[15*5*6];
static long classes_used[4];
static long qnos_used[16];
static long dct_used[2];
static long vlc_overflows;

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
	gint mb_num; /* mb number withing the 6 x 5 zig-zag pattern */
	gint mb_num_mod_3, mb_num_div_3; /* temporaries */
	gint mb_row;    /* mb row within sb (de-zigzag) */
	gint mb_col;    /* mb col within sb (de-zigzag) */
	/* Column offset of superblocks in macroblocks. */
	static const guint column_offset[] = {0, 9, 18, 27, 36};  
	
	/* Consider the area spanned super block as 30 element macroblock
	   grid (6 rows x 5 columns).  The macroblocks are laid out in a in
	   a zig-zag down and up the columns of the grid.  Of course,
	   superblocks are not perfect rectangles, since there are only 27
	   blocks.  The missing three macroblocks are either at the start
	   or end depending on the superblock column. */
	
	/* Within a superblock, the macroblocks start at the topleft corner
	   for even-column superblocks, and 3 down for odd-column
	   superblocks. */
	mb_num = mb->k;  
	mb_num_mod_3 = mb_num % 3;
	mb_num_div_3 = mb_num / 3;
	/* Compute superblock-relative row position (de-zigzag) */
	mb_row = ((mb_num_div_3 % 2) == 0) ? mb_num_mod_3 : (2- mb_num_mod_3); 
	/* Compute macroblock's frame-relative column position (in blocks) */
	mb_col = mb_num_div_3 + column_offset[mb->j];
	/* Compute frame-relative byte offset of macroblock's top-left corner
	   Convert from superblock-relative row position to frame relative 
	   (in blocks). */
	mb_row += (mb->i * 3); /* each right-edge macroblock is 
				  2 blocks high, and each superblock is 
				  6 blocks high */
	mb->x = mb_col * 16;
	mb->y = mb_row * 16;
} /* dv_place_420_macroblock */

/* FIXME: Could still be faster... */
static inline guint put_bits(unsigned char *s, guint offset, 
                             guint len, guint value)
{
#if !ARCH_X86
        s += (offset >> 3);
        
        value <<= (24 - len);
        value &= 0xffffff;
        value >>= (offset & 7);
        s[0] |= (value >> 16) & 0xff;
        s[1] |= (value >> 8) & 0xff;
        s[2] |= value & 0xff;
        return offset + len;
#else
        s += (offset >> 3);
        value <<= BITS_PER_LONG - len - (offset & 7);
        __asm__("bswap %0" : "=r" (value) : "0" (value));

        *((unsigned long*) s) |= value;
        return offset + len;

#endif
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
}; /* dv_vlc_test_table */

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
	if (run > 14 || amp > 22) { /* run < -1 || amp < 0 never happens! */
		return NULL;
	} else {
		return vlc_test_lookup[((run + 1) << 5) + amp];
	}
}

static guint vlc_num_bits_orig(int run, int amp, int sign)
{
	dv_vlc_encode_t *hit;
	
	hit = find_vlc_entry(run, amp);
	if (hit != NULL) {
		return (hit->len + ((amp != 0) ? 1 : 0));
	} else {
		if ((62 <= run) && (run <= 63) && (amp == 0)) {
			return    vlc_num_bits_orig(1, 0, 0) 
				+ vlc_num_bits_orig(run - 2, 0, 0);
		} else if ((6 <= run) && (run <= 61) && (amp == 0)) {
			return (7 + 6);
		} else if ((23 <= amp) && (amp <= 255) && (run == 0)) {
			return (7 + 8 + 1);
		} else {
			return    vlc_num_bits_orig(run - 1, 0, 0) 
				+ vlc_num_bits_orig(0, amp, sign);
		}
	}
}

static unsigned char * vlc_num_bits_lookup;

static void init_vlc_num_bits_lookup()
{
	int run,amp,sign;
	vlc_num_bits_lookup = (unsigned char*) malloc(32768);
	for (sign = 0; sign <= 1; sign++) {
		for (run = 0; run < 63; run++) {
			for (amp = 0; amp < 255; amp++) {
				vlc_num_bits_lookup[amp | (run << 8) 
						   | (sign << (8+6))] =
					vlc_num_bits_orig(run, amp, sign);
			}
		}
	}
}

static inline guint vlc_num_bits(int run, int amp, int sign)
{
	return vlc_num_bits_lookup[amp | (run << 8) | (sign << (8+6))];
}


static inline void vlc_encode_r(int run, int amp, int sign, dv_vlc_entry_t * o)
{
	dv_vlc_encode_t *hit = find_vlc_entry(run, amp);

	if (hit != NULL) {
		/* 1111110 */
		o->val = hit->val;
		o->len = hit->len;
		if (amp != 0) {
			o->val <<= 1;
			o->val |= sign;
			o->len++;
		}
	} else {
		if (amp == 0) {
			/* 1111110 */
			o->val = (0x7e << 6) | run;
			o->len = 7 + 6;
		} else {
			/* 1111111 */
			o->val = (0x7f << 9) | (amp << 1) | sign;
			o->len = 7 + 8 + 1;
		}
	}
}

/* FIXME: This still wants to be optimized... */

static inline dv_vlc_entry_t * vlc_encode(int run, int amp, int sign, 
					  dv_vlc_entry_t * o)
{
	dv_vlc_encode_t *hit = find_vlc_entry(run, amp);

	if (hit != NULL) {
		/* 1111110 */
		o->val = hit->val;
		o->len = hit->len;
		
		if (amp != 0) {
			o->val <<= 1;
			o->val |= sign;
			o->len++;
		}
	} else {
		if (amp == 0) {
			if (run < 62) {
				/* 1111110 */
				o->val = (0x7e << 6) | run;
				o->len = 7 + 6;
			} else {
				o->val = (0x7cf << 13) 
					| (0x7e << 6) | (run - 2);
				o->len = 11 + 7 + 6;
			}
		} else if (run == 0) {
			/* 1111111 */
			o->val = (0x7f << 9) | (amp << 1) | sign;
			o->len = 7 + 8 + 1;
		} else {
			vlc_encode_r(run - 1, 0, 0, o);
			++o;
			vlc_encode_r(0, amp, sign, o);
			return ++o;
		}
	}
	return ++o;
}


static unsigned short reorder_88[64] = {
	1, 2, 6, 7,15,16,28,29,
	3, 5, 8,14,17,27,30,43,
	4, 9,13,18,26,31,42,44,
	10,12,19,25,32,41,45,54,
	11,20,24,33,40,46,53,55,
	21,23,34,39,47,52,56,61,
	22,35,38,48,51,57,60,62,
	36,37,49,50,58,59,63,64
};
static unsigned short reorder_248[64] = {
	1, 3, 7,19,21,35,37,51,
	5, 9,17,23,33,39,49,53,
	11,15,25,31,41,47,55,61,
	13,27,29,43,45,57,59,63,

	2, 4, 8,20,22,36,38,52,
	6,10,18,24,34,40,50,54,
	12,16,26,32,42,48,56,62,
	14,28,30,44,46,58,60,64
};

static void prepare_reorder_tables()
{
	int i;
	for (i = 0; i < 64; i++) {
 		reorder_88[i]--;
 		reorder_88[i] *= 2;
		reorder_248[i]--;
		reorder_248[i] *= 2;
	}
}

extern int reorder_block_mmx(dv_coeff_t * a, 
			     const unsigned short* reorder_table);

static void reorder_block(dv_block_t *bl)
{
#if !ARCH_X86
	dv_coeff_t zigzag[64];
	int i;
#endif
	const unsigned short *reorder;

	if (bl->dct_mode == DV_DCT_88)
		reorder = reorder_88;
	else
		reorder = reorder_248;

#if ARCH_X86
	reorder_block_mmx(bl->coeffs, reorder);
	emms();
#else	
	for (i = 0; i < 64; i++) {
		*(unsigned short*) ((char*) zigzag + reorder[i])=bl->coeffs[i];
	}
	memcpy(bl->coeffs, zigzag, 64 * sizeof(dv_coeff_t));
#endif
}

static void vlc_encode_block(dv_block_t* bl, dv_vlc_block_t* out)
{
	dv_coeff_t * z = bl->coeffs + 1; /* First AC coeff */
	dv_coeff_t * z_end = bl->coeffs + 64;
	int run, amp, sign;

	dv_vlc_entry_t * o = out->coeffs;

	do {
		run = 0;
		if (*z == 0) {
			z++; run++;
			while (*z == 0 && z != z_end) {
				z++;
				run++;
			}
			if (z == z_end) {
				break;
			}
		}
		amp = *z++;
		sign = 0;
		if (amp < 0) {
			amp = -amp;
			sign = 1;
		}
		o = vlc_encode(run, amp, sign, o);
	} while (z != z_end);

	o->val = 0x6; /* EOB */
	o->len = 4;
	++o;
	out->coeffs_start = out->coeffs;
	out->coeffs_end = o;
}

static void vlc_make_fit(dv_vlc_block_t * bl, int num_blocks, long bit_budget)
{
	dv_vlc_block_t* b = bl + num_blocks;
	long bits_used = 0;
	for (b = bl; b != bl + num_blocks; b++) {
		dv_vlc_entry_t * begin = b->coeffs_start;
		dv_vlc_entry_t * end = b->coeffs_end;
		while (begin != end) {
			bits_used += begin->len;
			begin++;
		}
	}
	if (bits_used > bit_budget) {
		vlc_overflows++;
	} else {
		return;
	}
	while (bits_used > bit_budget) {
		b--;
		if (b->coeffs_end != b->coeffs + 1) {
			b->coeffs_end--;
			bits_used -= b->coeffs_end->len;
		}
		if (b == bl) {
			b = bl + num_blocks;
		}
	}
	for (b = bl; b != bl + num_blocks; b++) {
		b->coeffs_end[-1].len = 4;
		b->coeffs_end[-1].val = 0x6; /* EOB */
	}
}

static inline void vlc_split_code(dv_vlc_block_t * src, 
				  dv_vlc_block_t * dst,
				  unsigned char *vsbuffer)
{
	dv_vlc_entry_t* e = src->coeffs_start;	
	long val = e->val;
	long len = e->len;
			
	len -= dst->bit_budget; 
	val >>= len;
	dst->bit_offset = put_bits(vsbuffer,
				   dst->bit_offset, dst->bit_budget, val);
	dst->bit_budget = 0;
	e->val = e->val & ((1 << len)-1);
	e->len = len;
}	   

static void vlc_encode_block_pass_1(dv_vlc_block_t * bl, 
				    unsigned char *vsbuffer,
				    int vlc_encode_passes)
{
	while (bl->coeffs_start != bl->coeffs_end && 
		bl->bit_budget >= bl->coeffs_start->len) {
		bl->bit_offset = put_bits(vsbuffer, bl->bit_offset,
					  bl->coeffs_start->len,
					  bl->coeffs_start->val);
		bl->bit_budget -= bl->coeffs_start->len;
		bl->coeffs_start++;
	}
	if (vlc_encode_passes > 1) {
		if (bl->coeffs_start == bl->coeffs_end) {
			bl->can_supply = 1;
		} else {
			vlc_split_code(bl, bl, vsbuffer);
			bl->can_supply = 0;
		}
	}
}

static void vlc_encode_block_pass_n(dv_vlc_block_t * blocks, 
				    unsigned char *vsbuffer,
				    int vlc_encode_passes, int current_pass)
{
	dv_vlc_block_t * supplier[30];
	dv_vlc_block_t * receiver[30];
	int b,max_b;
	dv_vlc_block_t ** s_;
	dv_vlc_block_t ** r_;
	dv_vlc_block_t ** s_last = supplier;
	dv_vlc_block_t ** r_last = receiver;

	if (current_pass > vlc_encode_passes) {
		return;
	}

	max_b = (current_pass == 2) ? 6 : 30;

	for (b = 0; b < max_b; b++) {
		dv_vlc_block_t * bl = blocks + b;
		if (!bl->can_supply) {
			if (bl->coeffs_start != bl->coeffs_end) {
				*r_last++ = bl;
			}
		} else if (bl->bit_budget) {
			*s_last++ = bl;
		}
	}

	s_ = supplier;
	r_ = receiver;

	for (; r_ != r_last && s_ != s_last; r_++) {
		dv_vlc_block_t* r = *r_;
		for (; s_ != s_last; s_++) {
			dv_vlc_block_t * s = *s_;
			while (r->coeffs_start != r->coeffs_end && 
			       s->bit_budget >= r->coeffs_start->len) {
				unsigned long val;
				long len;
				val = r->coeffs_start->val;
				len = r->coeffs_start->len;
				s->bit_offset = put_bits(vsbuffer, 
							 s->bit_offset,
							 len, val);
				r->coeffs_start++;
				s->bit_budget -= len;
			}
			
			if (r->coeffs_start == r->coeffs_end) {
				break;
			}
			if (s->bit_budget) {
				vlc_split_code(r, s, vsbuffer);
			}
		}
	}

	return;
}

guint vlc_num_bits_block(dv_coeff_t *bl)
{
	dv_coeff_t * z = bl + 1; /* First AC coeff */
	dv_coeff_t * z_end = bl + 64;
	int run, amp, sign;
	guint num_bits = 0;

	do {
		run = 0;
		amp = *z;
		if (amp == 0) {
			z++; run++;
			while (*z == 0 && z != z_end) {
				z++;
				run++;
			}
			if (z == z_end) {
				break;
			}
			amp = *z;
		}
		z++;
		sign = 0;
		if (amp < 0) {
			amp = -amp;
			sign = 1;
		}
			
		num_bits += vlc_num_bits(run, amp, sign);
	} while (z != z_end);
	
	return num_bits;
}

extern int classify_mmx(dv_coeff_t * a, unsigned short* amp_ofs,
			unsigned short* amp_cmp);

inline int classify(dv_coeff_t * bl)
{
#if ARCH_X86
	static unsigned short amp_ofs[3][4] = { 
		{ 32768+35,32768+35,32768+35,32768+35 },
		{ 32768+23,32768+23,32768+23,32768+23 },
		{ 32768+11,32768+11,32768+11,32768+11 }
	};
	static unsigned short amp_cmp[3][4] = { 
		{ 32768+(35+35),32768+(35+35),32768+(35+35),32768+(35+35) },
		{ 32768+(23+23),32768+(23+23),32768+(23+23),32768+(23+23) },
		{ 32768+(11+11),32768+(11+11),32768+(11+11),32768+(11+11) }
	};
	int i,dc;

	dc = bl[0];
	bl[0] = 0;
	for (i = 0; i < 3; i++) {
		if (classify_mmx(bl, amp_ofs[i], amp_cmp[i])) {
			bl[0] = dc;
			emms();
			return 3-i;
		}
	}
	bl[0] = dc;
	emms();
	return 0;
#else
	int rval = 0;

	dv_coeff_t* p = bl + 1;
	dv_coeff_t* p_end = p + 64;

	while (p != p_end) {
		int a = *p++;
		int b = (a >> 15);
		a ^= b;
		a -= b;
		if (rval < a) {
			rval = a;
		}
	}

	if (rval > 35) {
		rval = 3;
	} else if (rval > 23) {
		rval = 2;
	} else if (rval > 11) {
		rval = 1;
	} else {
		rval = 0;
	}

	return rval;
#endif
}


void read_ppm_stream(FILE* f, unsigned char* readbuf, int * isPAL,
		     int * height_)
{
	int height, width;
	char line[200];
	fgets(line, sizeof(line), f);
	if (feof(f)) {
		exit(0);
	}
	do {
		fgets(line, sizeof(line), f); /* P6 */
	} while ((line[0] == '#'||(line[0] == '\n')) && !feof(f));
	if (sscanf(line, "%d %d\n", &width, &height) != 2) {
		fprintf(stderr, "Bad PPM file!\n");
		exit(1);
	}
	if (width != DV_WIDTH || 
	    (height != DV_PAL_HEIGHT && height != DV_NTSC_HEIGHT)) {
		fprintf(stderr, "Invalid picture size! (%d, %d)\n"
			"Allowed sizes are 720x576 for PAL and "
			"720x480 for NTSC\n"
			"Probably you should try ppmqscale...\n", 
			width, height);
		exit(1);
	}
	fgets(line, sizeof(line), f);	/* 255 */
	
	fread(readbuf, 1, 3 * DV_WIDTH * height, f);

	*height_ = height;
	*isPAL = (height == DV_PAL_HEIGHT);
}

void read_ppm(const char* filename, unsigned char* readbuf, int * isPAL,
	      int * height_)
{
	FILE *f;
	/* Read the PPM */
	f = fopen(filename, "r");
	if (f == NULL) {
		perror(filename);
		exit(1);
	}
	read_ppm_stream(f, readbuf, isPAL, height_);
	fclose(f);
}

static inline void do_blur(unsigned char* src, unsigned char* dst, 
			   int x, int y, int width)
{
	int r =   src[x - 3 + y*width] 
		+ src[x + 3 + y*width]
		+ src[x + (y-1)*width]
		+ src[x + (y+1)*width];
	r /= 8;
	r += src[x + y * width] / 2;
	dst[x + y*width] = r;
}

void blur(unsigned char * readbuf, unsigned char * writebuf,
	  int width, int height)
{
	int x = 0;
	int y = 0;

	for (x = 1; x < width - 1; x++) {
		for (y = 1; y < height - 1; y++) {
			do_blur(readbuf, writebuf, x*3 + 0, y, width * 3);
			do_blur(readbuf, writebuf, x*3 + 1, y, width * 3);
			do_blur(readbuf, writebuf, x*3 + 2, y, width * 3);
		}
	}
}

extern void rgbtoyuv_mmx(unsigned char* inPtr, int rows, int columns,
			 short* outyPtr, short* outuPtr, short* outvPtr);

static void convert_to_yuv(unsigned char* img_rgb, int height,
			   short* img_y, short* img_cr, short* img_cb)
{
#if !ARCH_X86
	int i;
	int ti;
	unsigned char *ip;
	register int r,g,b ;
	int colr, colb;
	register short int *ty, *tr, *tb;
	ip =  img_rgb;	
	colr = colb =  0;
	ty = img_y;	
	tr = img_cr;
	tb = img_cb;
	ti = height * DV_WIDTH ;
	for (i = 0; i < ti; i++) {
			r = *ip++;
			g = *ip++; 
			b = *ip++;

			*ty++ =  (( ( (16828 * r) + (33038 * g) + (6416 * b) )  >> 16   ) - 128 + 16) << 1  ; 

			colr +=   ( (28784 * r) + (-24121 * g) + (-4663 * b) ) ; 
			colb +=   ( (-9729 * r) + (-19055 * g) + (28784 * b) ) ;
			if (! (i % 4)){
				*tr++ = colr >> 17 ;
				*tb++ = colb >> 17 ;
				colr = colb = 0;
				}	
	}
#else
	rgbtoyuv_mmx(img_rgb, height, DV_WIDTH, (short*) img_y,
		     (short*) img_cr, (short*) img_cb);
	emms();
#endif
}

int need_dct_248_transposed(dv_coeff_t * bl)
{
	int res = 0;
	int i,j;
	
	for (j = 0; j < 7; j ++) {
		for (i = 0; i < 8; i++) {
			int a = bl[i * 8 + j] - bl[i * 8 + j + 1];
			int b = (a >> 15);
			a ^= b;
			a -= b;
			a &= DCT_248_THRESMASK;
			res += a;
		}
	}
	return (res > DCT_248_THRESHOLD);
}

#if ARCH_X86
extern int need_dct_248_mmx(dv_coeff_t * bl, short* thres_mask);

int need_dct_248_mmx_normal(dv_coeff_t * bl)
{
	int res;
	short thres_mask[4] = {DCT_248_THRESMASK, DCT_248_THRESMASK, 
			       DCT_248_THRESMASK, DCT_248_THRESMASK };
	res = need_dct_248_mmx(bl, thres_mask);
	return (res > DCT_248_THRESHOLD);
}


extern void transpose_mmx(short * dst);
extern void copy_y_block_mmx(short * dst, short * src);
extern void copy_pal_c_block_mmx(short * dst, short * src);
extern void copy_ntsc_c_block_mmx(short * dst, short * src);
#endif /* ARCH_X86 */

void build_coeff(short* img_y, short* img_cr, short* img_cb,
		 dv_macroblock_t *mb, int isPAL)
{
	int y = mb->y;
	int x = mb->x;
	int b;

#if !ARCH_X86
	if (isPAL || mb->x == DV_WIDTH- 16) { /* PAL or rightmost NTSC block */
		int i,j;
		for (j = 0; j < 8; j++) {
			for (i = 0; i < 8; i++) {
				mb->b[0].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH +  x + i];
				mb->b[1].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH +  x + 8 + i];
				mb->b[2].coeffs[8 * i + j] = 
					img_y[(y + 8 + j) * DV_WIDTH + x + i];
				mb->b[3].coeffs[8 * i + j] = 
					img_y[(y + 8 + j) * DV_WIDTH 
					     + x + 8 + i];
				mb->b[4].coeffs[8 * i + j] = 
					(img_cr[(y + 2*j) * DV_WIDTH/2 
					       + x / 2 + i]
					+ img_cr[(y + 2*j + 1) * DV_WIDTH/2
						+ x / 2 + i]) >> 1;
				mb->b[5].coeffs[8 * i + j] = 
					(img_cb[(y + 2*j) * DV_WIDTH/2
					      + x / 2 + i]
					+ img_cb[(y + 2*j + 1) * DV_WIDTH/2
						+ x / 2 + i]) >> 1;
			}
		}
	} else {                        /* NTSC */
		int i,j;
		for (j = 0; j < 8; j++) {
			for (i = 0; i < 8; i++) {
				mb->b[0].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH +  x + i];
				mb->b[1].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH +  x + 8 + i];
				mb->b[2].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH + x + 16 + i];
				mb->b[3].coeffs[8 * i + j] = 
					img_y[(y + j) * DV_WIDTH + x + 24 + i];
				mb->b[4].coeffs[8 * i + j] = 
					(img_cr[(y + j) * DV_WIDTH/2
					       + x / 2 + i*2]
					 + img_cr[(y + j) * DV_WIDTH/2 
						 + x / 2 + 1 + i*2]) >> 1;
				mb->b[5].coeffs[8 * i + j] = 
					(img_cb[(y + j) * DV_WIDTH/2
					       + x / 2 + i*2]
					 + img_cb[(y + j) * DV_WIDTH/2 
						 + x / 2 + 1 + i*2]) >> 1;
			}
		}
	}
	for (b = 0; b < 6; b++) {
		mb->b[b].dct_mode = need_dct_248_transposed(mb->b[b].coeffs) 
			? DV_DCT_248 : DV_DCT_88;
	}
#else
	if (isPAL || mb->x == DV_WIDTH- 16) { /* PAL or rightmost NTSC block */
		short* start_y = img_y + y * DV_WIDTH + x;
		copy_y_block_mmx(mb->b[0].coeffs, start_y);
		copy_y_block_mmx(mb->b[1].coeffs, start_y + 8);
		copy_y_block_mmx(mb->b[2].coeffs, start_y + 8 * DV_WIDTH);
		copy_y_block_mmx(mb->b[3].coeffs, start_y + 8 * DV_WIDTH + 8);
		copy_pal_c_block_mmx(mb->b[4].coeffs,
				     img_cr+y * DV_WIDTH/2+ x/2);
		copy_pal_c_block_mmx(mb->b[5].coeffs,
				     img_cb+y * DV_WIDTH/2+ x/2);
	} else {                              /* NTSC */
		short* start_y = img_y + y * DV_WIDTH + x;
		copy_y_block_mmx(mb->b[0].coeffs, start_y);
		copy_y_block_mmx(mb->b[1].coeffs, start_y + 8);
		copy_y_block_mmx(mb->b[2].coeffs, start_y + 16);
		copy_y_block_mmx(mb->b[3].coeffs, start_y + 24);
		copy_ntsc_c_block_mmx(mb->b[4].coeffs,
				      img_cr + y*DV_WIDTH/2 + x/2);
		copy_ntsc_c_block_mmx(mb->b[5].coeffs,
				      img_cb + y*DV_WIDTH/2 + x/2);
	}
	for (b = 0; b < 6; b++) {
		mb->b[b].dct_mode = need_dct_248_mmx_normal(mb->b[b].coeffs) 
			? DV_DCT_248 : DV_DCT_88;
	}
	transpose_mmx(mb->b[0].coeffs);
	transpose_mmx(mb->b[1].coeffs);
	transpose_mmx(mb->b[2].coeffs);
	transpose_mmx(mb->b[3].coeffs);
	transpose_mmx(mb->b[4].coeffs);
	transpose_mmx(mb->b[5].coeffs);
	emms();
#endif
}

static void do_dct(dv_macroblock_t *mb)
{
	guint b;

	for (b = 0; b < 6; b++) {
		dv_block_t *bl = &mb->b[b];
		
		if (bl->dct_mode == DV_DCT_88) {
			dct_88(bl->coeffs);
#if BRUTE_FORCE_DCT_88
			weight_88(bl->coeffs);
#endif
		} else {
			dct_248(bl->coeffs);
#if BRUTE_FORCE_DCT_248
			weight_248(bl->coeffs);
#endif
		}
		dct_used[bl->dct_mode]++;
		reorder_block(bl);
	}
}

static int classes[3][4] = {
	{ 0, 1, 2, 3},
	{ 1, 2, 3, 3},
	{ 2, 3, 3, 3}
};

static int qnos[4][16] = {
	{ 15,                  8,    6,    4,    2, 0},
	{ 15,         11, 10,  8,    6,    4,    2, 0},
	{ 15, 14, 13, 11,      8,    6,    4,    2, 0},
	{ 15,     13, 12, 10,  8,    6,    4,    2, 0}
};

static int qno_start[4][16];

static void init_qno_start()
{
	int qno;
	int klass;
	int i;
	for (qno = 0; qno < 16; qno++) {
		for (klass = 0; klass < 4; klass++) {
			i = 0;
			while (qnos[klass][i] > qno) {
				i++;
			}
			qno_start[klass][qno] = i;
		}
	}
}

static void do_classify(dv_macroblock_t * mb)
{
	int b;
	dv_block_t *bl;

	for (b = 0; b < 4; b++) {
		bl = &mb->b[b];
		bl->class_no = classes[0][classify(bl->coeffs)];
		classes_used[bl->class_no]++;
	}
	bl = &mb->b[4];
	bl->class_no = classes[1][classify(bl->coeffs)];
	classes_used[bl->class_no]++;
	bl = &mb->b[5];
	bl->class_no = classes[2][classify(bl->coeffs)];
	classes_used[bl->class_no]++;
	
}

static void quant_1_pass(dv_videosegment_t* videoseg, dv_vlc_block_t * vblocks)
{
	dv_macroblock_t *mb;
	gint m;
	int b;
	int smallest_qno = 15;
	int qno_index;
	int cycles = 0;

	dv_coeff_t bb[6][64];

	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		for (b = 0; b < 6; b++) {
			dv_block_t *bl = &mb->b[b];
			guint ac_coeff_budget = (((b < 4) ? 100 : 68) - 4); 
			qno_index = qno_start[bl->class_no][smallest_qno];
			while (smallest_qno > 0) {
				memcpy(bb[b], bl->coeffs, 
				       64 *sizeof(dv_coeff_t));
				quant(bb[b], smallest_qno, bl->class_no);
				if (vlc_num_bits_block(bb[b])<=ac_coeff_budget)
					break;
				qno_index++;
				cycles++;
				smallest_qno = qnos[bl->class_no][qno_index];
			}
			if (smallest_qno == 0) {
				break;
			}
		}

		mb->qno = smallest_qno;
		cycles_used[cycles]++;
		qnos_used[smallest_qno]++;
		if (smallest_qno == 15) { /* Things are cheap these days ;-) */
			for (b = 0; b < 6; b++) {
				dv_block_t *bl = &mb->b[b];
				memcpy(bl->coeffs, bb[b], 
				       64 *sizeof(dv_coeff_t));
				vlc_encode_block(bl, vblocks + b);
			}
		} else {
			for (b = 0; b < 6; b++) {
				dv_block_t *bl = &mb->b[b];
				quant(bl->coeffs, smallest_qno, bl->class_no);
				vlc_encode_block(bl, vblocks + b);
			}
			
			if (smallest_qno == 0) {
				for (b = 0; b < 6; b++) {
					vlc_make_fit(vblocks + b, 1,
						     ((b < 4) ? 100 : 68));
				}
			}

		}
		vblocks += 6;
	}
}

static void quant_2_passes(dv_videosegment_t* videoseg, 
			   dv_vlc_block_t * vblocks)
{
	dv_macroblock_t *mb;
	gint m;
	int b;
	int smallest_qno;
	int qno_index;
	dv_coeff_t bb[6][64];
	const guint ac_coeff_budget = 4*100+2*68-6*4;

	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		int cycles = 0;
		smallest_qno = 15;
		qno_index = 0;
		for (;;) {
			guint bits_used = 0;

			for (b = 0; b < 6; b++) {
				dv_block_t *bl = &mb->b[b];
				memcpy(bb[b], bl->coeffs, 
				       64 *sizeof(dv_coeff_t));
				quant(bb[b], smallest_qno, bl->class_no);
				bits_used += vlc_num_bits_block(bb[b]);
			}
			if (bits_used <= ac_coeff_budget) {
				break;
			}
			qno_index++;
			for (b = 0; b < 6; b++) {
				dv_block_t *bl = &mb->b[b];
				int qno = qnos[bl->class_no][qno_index];
				if (qno < smallest_qno) {
					smallest_qno = qno;
					qno_index = qno_start[bl->class_no]
						[smallest_qno];
				}
			}
			cycles++;
			if (smallest_qno == 0) {
				break;
			}
		}
		mb->qno = smallest_qno;
		cycles_used[cycles]++;
		qnos_used[smallest_qno]++;
		if (smallest_qno == 15) { /* Things are cheap these days ;-) */
			for (b = 0; b < 6; b++) {
				dv_block_t *bl = &mb->b[b];
				memcpy(bl->coeffs, bb[b], 
				       64 *sizeof(dv_coeff_t));
				vlc_encode_block(bl, vblocks + b);
			}
		} else {
			for (b = 0; b < 6; b++) {
				dv_block_t *bl = &mb->b[b];
				quant(bl->coeffs, smallest_qno, bl->class_no);
				vlc_encode_block(bl, vblocks + b);
			}
			if (smallest_qno == 0) {
				vlc_make_fit(vblocks, 6, 4*100+2*68);
			}
		}
		vblocks += 6;
	}
}

static void quant_3_passes(dv_videosegment_t* videoseg, 
			   dv_vlc_block_t * vblocks)
{
	dv_macroblock_t *mb;
	gint m;
	int b;
	int smallest_qno[5];
	int qno_index[5];
	int cycles = 0;
	dv_coeff_t bb[5][6][64];
	const guint ac_coeff_budget = 5*(4*100+2*68-6*4);
	guint bits_used[5];
	guint bits_used_total;

	for (m = 0; m < 5; m++) {
		smallest_qno[m] = 15;
		qno_index[m] = 0;
	}

	bits_used_total = 0;
	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		bits_used[m] = 0;
		for (b = 0; b < 6; b++) {
			dv_block_t *bl = &mb->b[b];
			memcpy(bb[m][b], bl->coeffs, 64 * sizeof(dv_coeff_t));
			quant(bb[m][b], smallest_qno[m], bl->class_no);
			bits_used[m] += vlc_num_bits_block(bb[m][b]);
		}
		bits_used_total += bits_used[m];
	}

	while (bits_used_total > ac_coeff_budget) {
		int m_max = 0;
		int bits_used_ = 0;
		int smallest_qno_;
		int qno_index_;

		for (m = 1; m < 5; m++) {
			if (bits_used[m] > bits_used[m_max]) {
				m_max = m;
			}
		}
		m = m_max;
		mb = videoseg->mb + m;
		qno_index_ = qno_index[m]; 
		smallest_qno_ = smallest_qno[m];

		qno_index_++;
		cycles++;

		for (b = 0; b < 6; b++) {
			dv_block_t *bl = &mb->b[b];
			int qno = qnos[bl->class_no][qno_index_];
			if (qno < smallest_qno_) {
				smallest_qno_ = qno;
				qno_index_ = qno_start[bl->class_no]
					[smallest_qno_];
			}
		}
		qno_index[m] = qno_index_;
		smallest_qno[m] = smallest_qno_;
		if (smallest_qno_ == 0) {
			break;
		}
		
		for (b = 0; b < 6; b++) {
			dv_block_t *bl = &mb->b[b];
			memcpy(bb[m][b], bl->coeffs, 
			       64 *sizeof(dv_coeff_t));
			quant(bb[m][b], smallest_qno_, bl->class_no);
			bits_used_ += vlc_num_bits_block(bb[m][b]);
		}
		
		bits_used_total -= bits_used[m];
		bits_used_total += bits_used_;
		bits_used[m] = bits_used_;
	}
	cycles_used[cycles]++;
	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		mb->qno = smallest_qno[m];
		qnos_used[smallest_qno[m]]++;
		if (smallest_qno[m] == 15) { 
			for (b = 0; b < 6; b++) {
				dv_block_t *bl = &mb->b[b];
				memcpy(bl->coeffs, bb[m][b], 
				       64 *sizeof(dv_coeff_t));
				vlc_encode_block(bl, vblocks + 6 * m + b);
			}
		} else {
			for (b = 0; b < 6; b++) {
				dv_block_t *bl = &mb->b[b];
				quant(bl->coeffs, smallest_qno[m], 
				      bl->class_no);
				vlc_encode_block(bl, vblocks + 6 * m + b);
			}
		}
	}
	if (bits_used_total > ac_coeff_budget) {
		vlc_make_fit(vblocks, 30, 5*(4*100+2*68));
	}
}

static void process_videosegment(short* img_y, short* img_cr, short* img_cb,
				 dv_videosegment_t* videoseg,
				 guint8 * vsbuffer, int vlc_encode_passes)
{
	dv_macroblock_t *mb;
	gint m;
	guint b;
	dv_vlc_block_t vlc_block[5*6];

	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		mb->vlc_error = 0;
		mb->eob_count = 0;
		mb->i = (videoseg->i+ dv_super_map_vertical[m]) 
			% (videoseg->isPAL ? 12 : 10);
		mb->j = dv_super_map_horizontal[m];
		mb->k = videoseg->k;
		
		if (videoseg->isPAL) {
			dv_place_420_macroblock(mb);
		} else {
			dv_place_411_macroblock(mb);
		}
		build_coeff(img_y, img_cr, img_cb, mb, videoseg->isPAL);
		do_dct(mb);
		do_classify(mb);
	}

	for (m = 0; m < 5; m++) {
		dv_vlc_block_t *v_bl = vlc_block;
		for (b = 0; b < 6; b++) {
			v_bl->bit_offset = (8* (80 * m))+dv_parse_bit_start[b];
			v_bl->bit_budget = (b < 4) ? 100 : 68;
			v_bl++;
		}
	}

	switch (vlc_encode_passes) {
	case 1:
		quant_1_pass(videoseg, vlc_block);
		break;
	case 2:
		quant_2_passes(videoseg, vlc_block);
		break;
	case 3:
		quant_3_passes(videoseg, vlc_block);
		break;
	default:
		fprintf(stderr, "Invalid value for vlc_encode_passes "
			"specified: %d!\n", vlc_encode_passes);
		exit(-1);
	}
		
	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		put_bits(vsbuffer, (8 * (80 * m)) + 28, 4, mb->qno);
		
		for (b = 0; b < 6; b++) {
			dv_block_t *bl = &mb->b[b];
			dv_vlc_block_t *v_bl = vlc_block + m*6 + b;

			v_bl->bit_offset = (8* (80 * m))+dv_parse_bit_start[b];
			v_bl->bit_budget = (b < 4) ? 100 : 68;

			put_bits(vsbuffer, v_bl->bit_offset - 12,
				 12, (bl->coeffs[0] << 3) | (bl->dct_mode << 2)
				 | bl->class_no);
			vlc_encode_block_pass_1(v_bl, vsbuffer, 
						vlc_encode_passes);
		}
		vlc_encode_block_pass_n(vlc_block + m * 6, vsbuffer, 
					vlc_encode_passes, 2);
	}
	vlc_encode_block_pass_n(vlc_block, vsbuffer, vlc_encode_passes, 3);
}

static void encode(short* img_y, short* img_cr, short* img_cb, 
		   int isPAL, unsigned char* target, int vlc_encode_passes)
{
	static dv_videosegment_t videoseg ALIGN64;

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
		dif += 6; /* skip the first 6 dif blocks in a dif sequence */
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
					     &videoseg, target + offset,
					     vlc_encode_passes);
			
			dif += 5;
		} 
	} 
}

void encode_picture(unsigned char* readbuf, int isPAL, int height, time_t now,
		    int do_blur, int vlc_encode_passes)
{
	static int frame_counter = 0;
	unsigned char blurred[DV_PAL_HEIGHT * DV_WIDTH * 3];
	unsigned char target[144000];
	short img_y[DV_PAL_HEIGHT * DV_WIDTH];
	short img_cr[DV_PAL_HEIGHT * DV_WIDTH / 2];
	short img_cb[DV_PAL_HEIGHT * DV_WIDTH / 2];

	if (do_blur) {
		blur(readbuf, blurred, DV_WIDTH, height);
		convert_to_yuv(blurred, height, img_y, img_cr, img_cb);
	} else {
		convert_to_yuv(readbuf, height, img_y, img_cr, img_cb);
	}
	encode(img_y, img_cr, img_cb, isPAL, target, vlc_encode_passes);
	write_info_blocks(target, frame_counter, isPAL, &now);
	fwrite(target, 1, isPAL ? 144000 : 120000, stdout);
	frame_counter++;
}

void do_show_statistics()
{
	int i = 0;
	fprintf(stderr, "\n\nFinal statistics:\n"
		"========================================================\n"
		"\n  |CYCLES    |QNOS     |CLASS    |VLC OVERF|DCT\n"
		"========================================================\n");
	fprintf(stderr,"%2d: %8ld |%8ld |%8ld |%8ld |%8ld (DCT88)\n", 
		i, cycles_used[i], qnos_used[i],
		classes_used[i], vlc_overflows, dct_used[DV_DCT_88]);
	i++;
	fprintf(stderr,"%2d: %8ld |%8ld |%8ld |         |%8ld (DCT248)\n", 
		i, cycles_used[i], qnos_used[i],
		classes_used[i], 
		dct_used[DV_DCT_248]);
	i++;
	for (;i < 4; i++) {
		fprintf(stderr, "%2d: %8ld |%8ld |%8ld |         |\n", 
			i, cycles_used[i], qnos_used[i],
			classes_used[i]);
	}
	for (;i < 16; i++) {
		fprintf(stderr, "%2d: %8ld |%8ld |         |         |\n", 
			i, cycles_used[i], qnos_used[i]);
	}
}

#define DV_ENCODER_OPT_VERSION         0
#define DV_ENCODER_OPT_START_FRAME     1
#define DV_ENCODER_OPT_END_FRAME       2
#define DV_ENCODER_OPT_WRONG_INTERLACE 3
#define DV_ENCODER_OPT_BLUR            4
#define DV_ENCODER_OPT_ENCODE_PASSES   5
#define DV_ENCODER_OPT_STATISTICS      6
#define DV_ENCODER_OPT_AUTOHELP        7
#define DV_ENCODER_NUM_OPTS            8

int main(int argc, char *argv[])
{
	time_t now;
	unsigned long start = 0;
	unsigned long end = 0xffffffff;
	int wrong_interlace = 0;
	int do_blur = 0;
	const char* filename = NULL;
	int vlc_encode_passes = 3;
	int show_statistics = 0;

#if HAVE_LIBPOPT
	struct poptOption option_table[DV_ENCODER_NUM_OPTS+1]; 
	int rc;             /* return code from popt */
	poptContext optCon; /* context for parsing command-line options */
	option_table[DV_ENCODER_OPT_VERSION] = (struct poptOption) {
		longName: "version", 
		shortName: 'v', 
		val: 'v', 
		descrip: "show encode version number"
	}; /* version */

	option_table[DV_ENCODER_OPT_START_FRAME] = (struct poptOption) {
		longName:   "start-frame", 
		shortName:  's', 
		argInfo:    POPT_ARG_INT, 
		arg:        &start,
		argDescrip: "count",
		descrip:    "start at <count> frame (defaults to 0)"
	}; /* start-frame */

	option_table[DV_ENCODER_OPT_END_FRAME] = (struct poptOption) {
		longName:   "end-frame", 
		shortName:  'e', 
		argInfo:    POPT_ARG_INT, 
		arg:        &end,
		argDescrip: "count",
		descrip:    "end at <count> frame (defaults to unlimited)"
	}; /* end-frames */

	option_table[DV_ENCODER_OPT_WRONG_INTERLACE] = (struct poptOption) {
		longName:   "wrong-interlace", 
		shortName:  'i', 
		arg:        &wrong_interlace,
		descrip:    "flip lines to compensate for wrong interlacing"
	}; /* wrong-interlace */

	option_table[DV_ENCODER_OPT_BLUR] = (struct poptOption) {
		longName:   "blur", 
		shortName:  'b', 
		arg:        &do_blur,
		descrip:    "apply a 3x3 blur filter on the "
		"images before encoding"
	}; /* blur */

	option_table[DV_ENCODER_OPT_ENCODE_PASSES] = (struct poptOption) {
		longName:   "vlc-passes", 
		shortName:  'p', 
		argInfo:    POPT_ARG_INT, 
		arg:        &vlc_encode_passes,
		descrip:    "vlc code distribution passes (1-3) "
		"greater values = better quality but "
		"not necessarily slower encoding!"
	}; /* vlc encoder passes */

	option_table[DV_ENCODER_OPT_STATISTICS] = (struct poptOption) {
		longName:   "show-statistics", 
		shortName:  't', 
		arg:        &show_statistics,
		descrip:    "show encoder statistics "
	}; /* statistics */
	
	option_table[DV_ENCODER_OPT_AUTOHELP] = (struct poptOption) {
		argInfo: POPT_ARG_INCLUDE_TABLE,
		arg:     poptHelpOptions,
		descrip: "Help options",
	}; /* autohelp */

	option_table[DV_ENCODER_NUM_OPTS] = (struct poptOption) { 
		NULL, 0, 0, NULL, 0 };

	optCon = poptGetContext(NULL, argc, 
				(const char **)argv, option_table, 0);
	poptSetOtherOptionHelp(optCon, "<filename pattern or - for stdin>");

	while ((rc = poptGetNextOpt(optCon)) > 0) {
		switch (rc) {
		case 'v':
			fprintf(stderr,"encode: version %s, "
				"http://libdv.sourceforge.net/\n",
				"CVS 01/14/2001");
			exit(0);
			break;
		default:
			break;
		} /* switch */
	} /* while */

	if (rc < -1) {
		/* an error occurred during option processing */
		fprintf(stderr, "%s: %s\n",
			poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
			poptStrerror(rc));
		exit(-1);
	}

	filename = poptGetArg(optCon);
	if((filename == NULL) || !(poptPeekArg(optCon) == NULL)) {
		poptPrintUsage(optCon, stderr, 0);
		fprintf(stderr, 
			"\nSpecify a single <filename pattern> argument; "
			"e.g. pond%%05d.ppm or - for stdin\n");
		exit(-1);
	}
	poptFreeContext(optCon);

#else
	/* No popt, no usage and no options!  HINT: get popt if you don't
	 * have it yet, it's at: ftp://ftp.redhat.com/pub/redhat/code/popt 
	 */
	filename = argv[1];
#endif /* HAVE_LIBPOPT */

	weight_init();  
	dct_init();
	dv_dct_248_init();
	dv_construct_vlc_table();
	dv_parse_init();
	dv_place_init();
	init_vlc_test_lookup();
	init_vlc_num_bits_lookup();
	init_qno_start();
	prepare_reorder_tables();

	memset(cycles_used,0,sizeof(cycles_used));
	memset(classes_used,0,sizeof(classes_used));
	memset(qnos_used,0,sizeof(qnos_used));
	memset(dct_used,0,sizeof(dct_used));
	vlc_overflows = 0;

	now = time(NULL);
	if (strcmp(filename, "-") == 0) {
		int i;
		for (i = start; i <= end; i++) {
			unsigned char readbuf[(DV_PAL_HEIGHT+1)* DV_WIDTH * 3];
			unsigned char* rbuf = readbuf;
			int height;
			int isPAL;
		
			read_ppm_stream(stdin, readbuf, &isPAL, &height);
			if (wrong_interlace) {
				memset(readbuf + height * DV_WIDTH * 3, 0,
				       DV_WIDTH * 3);
				rbuf += DV_WIDTH * 3;
			}
			encode_picture(rbuf, isPAL, height, now, do_blur,
				       vlc_encode_passes);
			fprintf(stderr, "[%d] ", i);
		}
	} else {
		unsigned long i;
		for (i = start; i <= end; i++) {
			unsigned char readbuf[(DV_PAL_HEIGHT+1)* DV_WIDTH * 3];
			unsigned char* rbuf = readbuf;
			int height;
			int isPAL;
			unsigned char fbuf[1024];

			snprintf(fbuf, 1024, filename, i);
		
			read_ppm(fbuf, readbuf, &isPAL, &height);
			if (wrong_interlace) {
				memset(readbuf + height * DV_WIDTH * 3, 0,
				       DV_WIDTH * 3);
				rbuf += DV_WIDTH * 3;
			}
			encode_picture(rbuf, isPAL, height, now, do_blur,
				       vlc_encode_passes);
			fprintf(stderr, "[%ld] ", i);
		}
	}
  
	if (show_statistics) {
		do_show_statistics();
	}

	return 0;
}


