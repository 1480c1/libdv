/* 
 *  enc_input.c
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
 
#include "enc_input.h"
#include "encode.h"
#include "dct.h"
#include "dv_types.h"
#include "mmx.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#if HAVE_LINUX_VIDEODEV_H
#define HAVE_DEV_VIDEO  1
#endif

#if HAVE_DEV_VIDEO
#include <sys/types.h>
#include <linux/videodev.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#endif

#if !ARCH_X86
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
#endif

extern void rgbtoycb_mmx(unsigned char* inPtr, int rows, int columns,
			 short* outyPtr, short* outuPtr, short* outvPtr);

void dv_enc_rgb_to_ycb(unsigned char* img_rgb, int height,
		       short* img_y, short* img_cr, short* img_cb)
{
#if !ARCH_X86
#if 1
       int i;
       int ti;
       unsigned char *ip;
       register long r,g,b ;
       long colr, colb;
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

	       *ty++ =  (( ( (16828 * r) + (33038 * g) + (6416 * b) )
			   >> 16   ) - 128 + 16) << DCT_YUV_PRECISION;

	       colr +=  ( (28784 * r) + (-24121 * g) + (-4663 * b) ) ; 
	       colb +=  ( (-9729 * r) + (-19055 * g) + (28784 * b) ) ;
	       if (! (i % 2)) {
		       *tr++ = colr >> (16 + 1 - DCT_YUV_PRECISION);
		       *tb++ = colb >> (16 + 1 - DCT_YUV_PRECISION);
		       colr = colb = 0;
	       }       
       }
#else
	int x,y;
	/* This is the RGB -> YUV color matrix */
	static const double cm[3][3] = {
		{.299 * 219.0/255.0,.587* 219.0/255.0,.114* 219.0/255.0},
		{.5* 224.0/255.0,-.419* 224.0/255.0,-.081* 224.0/255.0},
		{-.169 * 224.0/255.0,-.331* 224.0/255.0,.5* 224.0/255.0}
	};

	double tmp_cr[DV_PAL_HEIGHT][DV_WIDTH];
	double tmp_cb[DV_PAL_HEIGHT][DV_WIDTH];
	double fac = pow(2, DCT_YUV_PRECISION);
	int i,j;

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
		}
	}
	for (y = 0; y < height; y++) {
		for (x = 0; x < DV_WIDTH / 2; x++) {
			img_cr[y * DV_WIDTH / 2 + x] = 
				f2sb((tmp_cr[y][2*x] +
				      tmp_cr[y][2*x+1]) / 2.0);
			img_cb[y * DV_WIDTH / 2 + x] = 
				f2sb((tmp_cb[y][2*x] +
				      tmp_cb[y][2*x+1]) / 2.0);
		}
	}
#endif
#else
	rgbtoycb_mmx(img_rgb, height, DV_WIDTH, (short*) img_y,
		     (short*) img_cr, (short*) img_cb);
	emms();
#endif
}

extern void yuvtoycb_mmx(unsigned char* inPtr, int rows, int columns,
			 short* outyPtr, short* outuPtr, short* outvPtr);

void dv_enc_yuv_to_ycb(unsigned char* img_yuv, int height,
		       short* img_y, short* img_cr, short* img_cb)
{
#if !ARCH_X86
	/* to be written */
#else
	yuvtoycb_mmx(img_yuv, height, DV_WIDTH, (short*) img_y,
		     (short*) img_cr, (short*) img_cb);
	emms();
#endif
}

extern void ycbtoycb_mmx(unsigned char* inPtr, int rows, int columns,
			 short* outyPtr, short* outuPtr, short* outvPtr);

static void dv_enc_ycb_to_ycb(unsigned char* img_ycb, int height,
			      short* img_y, short* img_cr, short* img_cb)
{
#if !ARCH_X86
	long count = height * DV_WIDTH;
	unsigned char* p = img_ycb;

	while (count--) {
		/* FIXME: - 128 + 16 doesn't work? */
		*img_y++ = (*p++ - 128) << DCT_YUV_PRECISION; 
	}

	count = height * DV_WIDTH / 2;
	while (count--) {
		*img_cb++ = (*p++ - 128) << DCT_YUV_PRECISION;
	}

	count = height * DV_WIDTH / 2;
	while (count--) {
		*img_cr++ = (*p++ - 128) << DCT_YUV_PRECISION;
	}
#else
	ycbtoycb_mmx(img_ycb, height, DV_WIDTH, (short*) img_y,
		     (short*) img_cr, (short*) img_cb);
	emms();
#endif
}

static unsigned char* readbuf = NULL;
static int wrong_interlace = 0;

static int read_ppm_stream(FILE* f, int * isPAL, int * height_)
{
	int height, width;
	char line[200];
	fgets(line, sizeof(line), f);
	if (feof(f)) {
		return -1;
	}
	do {
		fgets(line, sizeof(line), f); /* P6 */
	} while ((line[0] == '#'||(line[0] == '\n')) && !feof(f));
	if (sscanf(line, "%d %d\n", &width, &height) != 2) {
		fprintf(stderr, "Bad PPM file!\n");
		return -1;
	}
	if (width != DV_WIDTH || 
	    (height != DV_PAL_HEIGHT && height != DV_NTSC_HEIGHT)) {
		fprintf(stderr, "Invalid picture size! (%d, %d)\n"
			"Allowed sizes are 720x576 for PAL and "
			"720x480 for NTSC\n"
			"Probably you should try ppmqscale...\n", 
			width, height);
		return -1;
	}
	fgets(line, sizeof(line), f);	/* 255 */
	
	fread(readbuf, 1, 3 * DV_WIDTH * height, f);

	*height_ = height;
	*isPAL = (height == DV_PAL_HEIGHT);

	return 0;
}

static int ppm_init(int wrong_interlace_) 
{
	wrong_interlace = wrong_interlace_;
	readbuf = (unsigned char*) calloc(DV_WIDTH * (DV_PAL_HEIGHT + 1), 3);

	return 0;
}

static void ppm_finish()
{
	free(readbuf);
}

static int ppm_load(const char* filename, int * isPAL, 
		    short* img_y, short * img_cr, short * img_cb)
{
	FILE* ppm_in = NULL;
	int rval = -1;
	unsigned char* rbuf = readbuf;
	int height;

	if (strcmp(filename, "-") == 0) {
		ppm_in = stdin;
	} else {
		ppm_in = fopen(filename, "r");
		if (ppm_in == NULL) {
			return -1;
		}
	}

	rval = read_ppm_stream(ppm_in, isPAL, &height);
	if (ppm_in != stdin) {
		fclose(ppm_in);
	}
	if (rval != -1) {
		if (wrong_interlace) {
			rbuf += DV_WIDTH * 3;
		}
		dv_enc_rgb_to_ycb(rbuf, height, img_y, img_cr, img_cb);
	}
	return rval;
}



#if HAVE_DEV_VIDEO
#define ioerror(msg, res) \
        if (res == -1) { \
                perror(msg); \
                return(-1); \
        }

/* This is a work around for bugs in the bttv driver... */

static int vid_in = -1;
static unsigned char * vid_map;
static struct video_mmap gb_frames[VIDEO_MAX_FRAME];
static struct video_mbuf gb_buffers;
static int frame_counter = 0;

static void close_capture()
{
	int i;
	if (vid_in != -1) {
		/* Be sure, that DMA is OFF now! */
		for (i = 0; i < gb_buffers.frames; i++) {
			ioctl(vid_in, VIDIOCSYNC, &gb_frames[i]);
		}
		/* No one capturing? Leave... */
		munmap(vid_map, gb_buffers.size);
		close(vid_in);
	}
}

static void sigterm_handler(int signum)
{
	fprintf(stderr, "Shutting down capture on signal %d\n", signum);
	close_capture();
	exit(-1);
}

static int init_vid_device(const char* filename)
{
	int width = DV_WIDTH;
	int height = DV_PAL_HEIGHT;
	long i;

	vid_in = open(filename, O_RDWR);

	ioerror("open", vid_in);
	
	ioerror("VIDIOCGMBUF", ioctl(vid_in, VIDIOCGMBUF, &gb_buffers));
	vid_map = (unsigned char*) mmap(0, gb_buffers.size, 
					PROT_READ | PROT_WRITE,
					MAP_SHARED, vid_in, 0);
	ioerror("mmap", (long) vid_map);

	fprintf(stderr, "encodedv-capture: found %d buffers\n", 
		gb_buffers.frames);

	for (i = 0; i < gb_buffers.frames; i++) {
		gb_frames[i].frame = i;
		gb_frames[i].width = width;
		gb_frames[i].height = height;
		gb_frames[i].format = VIDEO_PALETTE_YUV422P;
	}

	for (i = 0; i < gb_buffers.frames; i++) {
		ioerror("VIDIOCMCAPTURE", 
			ioctl(vid_in, VIDIOCMCAPTURE, &gb_frames[i]));
	}
	return 0;
}

static int video_init(int wrong_interlace)
{
	signal(SIGINT, sigterm_handler);
	signal(SIGTERM, sigterm_handler);
	signal(SIGSEGV, sigterm_handler);
	return 0;
}

static void video_finish()
{
	close_capture();
}

static int video_load(const char* filename, int * isPAL, 
		       short* img_y, short * img_cr, short * img_cb)
{
	int fnumber;

	if (vid_in == -1) {
		if (init_vid_device(filename) < 0) {
			return -1;
		}
	}

	fnumber = (frame_counter++) % gb_buffers.frames;

	ioerror("VIDIOCSYNC", ioctl(vid_in, VIDIOCSYNC, &gb_frames[fnumber]));
	
	dv_enc_ycb_to_ycb(vid_map + gb_buffers.offsets[fnumber],
			  DV_PAL_HEIGHT, img_y, img_cr, img_cb);
	*isPAL = 1;

	ioerror("VIDIOCMCAPTURE",
		ioctl(vid_in, VIDIOCMCAPTURE, &gb_frames[fnumber]));
	return 0;
}

#endif


static dv_enc_input_filter_t filters[DV_ENC_MAX_INPUT_FILTERS] = {
	{ ppm_init, ppm_finish, ppm_load, "ppm" },
#if HAVE_DEV_VIDEO
	{ video_init, video_finish, video_load, "video" },
#endif
#if 0
	{ pgm_init, pgm_finish, pgm_load, "pgm" },
#endif
	{ NULL, NULL, NULL, NULL }};

void dv_enc_register_input_filter(dv_enc_input_filter_t filter)
{
	dv_enc_input_filter_t * p = filters;
	while (p->filter_name) p++;
	*p = filter;
}

int get_dv_enc_input_filters(dv_enc_input_filter_t ** filters_,
			     int * count)
{
	dv_enc_input_filter_t * p = filters;

	*count = 0;
	while (p->filter_name) p++, (*count)++;

	*filters_ = filters;
	return 0;
}
