/* 
 *  playdv.c
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

#include <stdlib.h>
#include <glib.h>
#include <stdio.h>

#include "dv.h"
#include "display.h"
#include "bitstream.h"

int main(int argc,char *argv[]) {
  static guint8 vsbuffer[80*5]      __attribute__ ((aligned (64))); 
  static dv_videosegment_t videoseg __attribute__ ((aligned (64)));
  dv_display_t *dv_dpy = NULL;
  FILE *f;
  dv_sample_t sampling;
  gboolean isPAL = 0;
  gboolean is61834 = 0;
  gboolean displayinit = FALSE;
  gint numDIFseq;
  gint ds, v;
  gint lost_coeffs;
  guint dif;
  guint offset;
  static gint frame_count;
  guint quality = DV_QUALITY_COLOR | DV_QUALITY_AC_2;

  if (argc >= 2)
    f = fopen(argv[1],"r");
  else
    f = stdin;


  videoseg.bs = bitstream_init();
  dv_init();

  lost_coeffs = 0;
  dif = 0;
  while (!feof(f)) {
    /* Rather read a whole frame at a time, as this gives the
       data-cache a lot longer time to stay warm during decode */
    // start by reading header block, to determine stream parameters
    offset = dif * 80;
    if (fseek(f,offset,SEEK_SET)) break;
    if (fread(vsbuffer,sizeof(vsbuffer),1,f)<1) break;
    isPAL = vsbuffer[3] & 0x80;
    is61834 = vsbuffer[3] & 0x01;
    if(isPAL && is61834)
      sampling = e_dv_sample_420;
    else
      sampling = e_dv_sample_411;
//    printf("video is %s-compliant %s\n",
//           is61834?"IEC61834":"SMPTE314M",isPAL?"PAL":"NTSC");
    numDIFseq = isPAL ? 12 : 10;

    if (!displayinit) {
      displayinit = TRUE;
      dv_dpy = dv_display_init (&argc, &argv, 
				 720, isPAL ? 576: 480, sampling, "playdv", "playdv");
      if(!dv_dpy) goto no_display;
    }

    // each DV frame consists of a sequence of DIF segments 
    for (ds=0;
	 ds<numDIFseq;
	 ds++) { 
      // Each DIF segment conists of 150 dif blocks, 135 of which are video blocks
      dif += 6; // skip the first 6 dif blocks in a dif sequence 
      /* A video segment consists of 5 video blocks, where each video
         block contains one compressed macroblock.  DV bit allocation
         for the VLC stage can spill bits between blocks in the same
         video segment.  So parsing needs the whole segment to decode
         the VLC data */
      // Loop through video segments 
      for (v=0;v<27;v++) {
	// skip audio block - interleaved before every 3rd video segment
	if(!(v % 3)) dif++; 
        // stage 1: parse and VLC decode 5 macroblocks that make up a video segment
	offset = dif * 80;
	if(fseek(f,offset,SEEK_SET)) break;
	if(fread(vsbuffer,sizeof(vsbuffer),1,f)<1) break;
	bitstream_new_buffer(videoseg.bs, vsbuffer, 80*5); 
	videoseg.i = ds;
	videoseg.k = v;
        videoseg.isPAL = isPAL;
        lost_coeffs += dv_parse_video_segment(&videoseg, quality);
        // stage 2: dequant/unweight/iDCT blocks, and place the macroblocks
	dif+=5;
	dv_decode_video_segment(&videoseg,quality);
	if(dv_dpy->color_space == e_dv_color_yuv) 
	  dv_render_video_segment_yuv_mmx(&videoseg, sampling, dv_dpy->pixels, dv_dpy->pitches);
	else
	  dv_render_video_segment_rgb(&videoseg, sampling, dv_dpy->pixels[0], dv_dpy->pitches[0]);
      } // for s
    } // ds
    //fprintf(stderr,"displaying frame (%d coeffs lost in parse)\n", lost_coeffs);
    frame_count++;
#if BENCHMARK_MODE
    if(frame_count >= 450) break;
#endif
    dv_display_show(dv_dpy);
  }
  dv_display_exit(dv_dpy);
  exit(0);
 no_display:
  exit(-1);
}
