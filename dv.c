#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "dv.h"
#include "dct.h"
#include "idct_248.h"
#include "quant.h"
#include "weighting.h"
#include "vlc.h"
#include "parse.h"
#include "place.h"
#include "rgb.h"
#include "YUY2.h"
#include "YV12.h"
#if ARCH_X86
#include "mmx.h"
#endif

static void 
convert_coeffs(dv_block_t *bl) {
  int i;
  for(i=0;
      i<64;
      i++) {
    bl->coeffs248[i] = bl->coeffs[i];
  } // for
} // convert_coeffs

static void 
convert_coeffs_prime(dv_block_t *bl) {
  int i;
  for(i=0;
      i<64;
      i++) {
    bl->coeffs[i] = bl->coeffs248[i];
  } // for 
} // convert_coeffs_prime

void dv_init(dv_decoder_t *dv) {
#if ARCH_X86
  dv->use_mmx = mmx_ok(); 
#endif
  weight_init();  
  dct_init();
  dv_dct_248_init();
  dv_construct_vlc_table();
  dv_parse_init();
  dv_place_init();
  dv_rgb_init();
  dv_YUY2_init();
  dv_YV12_init();
} /* dv_init */

static inline void 
dv_decode_macroblock(dv_decoder_t *dv, dv_macroblock_t *mb, guint quality) {
  dv_block_t *bl;
  gint b;
  for (b=0,bl = mb->b;
       b<((quality & DV_QUALITY_COLOR) ? 6 : 4);
       b++,bl++) {
    if (bl->dct_mode == DV_DCT_248) { 
      quant_248_inverse(bl->coeffs,mb->qno,bl->class_no);
      weight_248_inverse(bl->coeffs);
      convert_coeffs(bl);
      dv_idct_248(bl->coeffs248);
      convert_coeffs_prime(bl);
    } else {
#if ARCH_X86
      quant_88_inverse_x86(bl->coeffs,mb->qno,bl->class_no);
      weight_88_inverse(bl->coeffs);
      idct_88(bl->coeffs);
#else // ARCH_X86
      quant_88_inverse(bl->coeffs,mb->qno,bl->class_no);
      weight_88_inverse(bl->coeffs);
      idct_88(bl->coeffs);
#endif // ARCH_X86
    } // else
  } // for b
} /* dv_decode_macroblock */

void 
dv_decode_video_segment(dv_decoder_t *dv, dv_videosegment_t *seg, guint quality) {
  dv_macroblock_t *mb;
  dv_block_t *bl;
  gint m, b;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    for (b=0,bl = mb->b;
	 b<((quality & DV_QUALITY_COLOR) ? 6 : 4);
	 b++,bl++) {
      if (bl->dct_mode == DV_DCT_248) { 
	quant_248_inverse(bl->coeffs,mb->qno,bl->class_no);
	weight_248_inverse(bl->coeffs);
	convert_coeffs(bl);
	dv_idct_248(bl->coeffs248);
	convert_coeffs_prime(bl);
      } else {
#if ARCH_X86
	quant_88_inverse_x86(bl->coeffs,mb->qno,bl->class_no);
	weight_88_inverse(bl->coeffs);
	idct_88(bl->coeffs);
#else // ARCH_X86
	quant_88_inverse(bl->coeffs,mb->qno,bl->class_no);
	weight_88_inverse(bl->coeffs);
	idct_88(bl->coeffs);
#endif // ARCH_X86
      } // else
    } // for b
  } // for mb
} /* dv_decode_video_segment */

static inline void
dv_render_macroblock_rgb(dv_decoder_t *dv, dv_macroblock_t *mb, guchar *pixels, gint pitch ) {
  if(dv->sampling == e_dv_sample_411) {
    if(mb->x >= 704) {
      dv_mb411_right_rgb(mb, pixels, pitch); // Right edge are 16x16
    } else {
      dv_mb411_rgb(mb, pixels, pitch);
    } // else
  } else {
    dv_mb420_rgb(mb, pixels, pitch);
  } // else
} // dv_render_macroblock_rgb

void
dv_render_video_segment_rgb(dv_decoder_t *dv, dv_videosegment_t *seg, guchar *pixels, gint pitch ) {
  dv_macroblock_t *mb;
  gint m;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_rgb(mb, pixels, pitch); // Right edge are 16x16
      } else {
	dv_mb411_rgb(mb, pixels, pitch);
      } // else
    } else {
      dv_mb420_rgb(mb, pixels, pitch);
    } // else
  } // for   
} /* dv_render_video_segment_rgb */

#if ARCH_X86

static inline void
dv_render_macroblock_yuv(dv_decoder_t *dv, dv_macroblock_t *mb, guchar **pixels, guint16 *pitches) {
  if(dv->use_mmx) {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_YUY2_mmx(mb, pixels[0], pitches[0]); // Right edge are 420!
      } else {
	dv_mb411_YUY2_mmx(mb, pixels[0], pitches[0]);
      } // else
    } else {
      dv_mb420_YV12_mmx(mb, pixels, pitches);
    } // else
  } else {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_YUY2(mb, pixels[0], pitches[0]); // Right edge are 420!
      } else {
	dv_mb411_YUY2(mb, pixels[0], pitches[0]);
      } // else
    } else {
      dv_mb420_YV12(mb, pixels, pitches);
    } // else
  } // else
} /* dv_render_macroblock_yuv */

void
dv_render_video_segment_yuv(dv_decoder_t *dv, dv_videosegment_t *seg, guchar **pixels, guint16 *pitches) {
  dv_macroblock_t *mb;
  gint m;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(dv->use_mmx) {
      if(dv->sampling == e_dv_sample_411) {
	if(mb->x >= 704) {
	  dv_mb411_right_YUY2_mmx(mb, pixels[0], pitches[0]); // Right edge are 420!
	} else {
	  dv_mb411_YUY2_mmx(mb, pixels[0], pitches[0]);
	} // else
      } else {
	dv_mb420_YV12_mmx(mb, pixels, pitches);
      } // else
    } else {
      if(dv->sampling == e_dv_sample_411) {
	if(mb->x >= 704) {
	  dv_mb411_right_YUY2(mb, pixels[0], pitches[0]); // Right edge are 420!
	} else {
	  dv_mb411_YUY2(mb, pixels[0], pitches[0]);
	} // else
      } else {
	dv_mb420_YV12(mb, pixels, pitches);
      } // else
    } // else
  } // for   
} /* dv_render_video_segment_yuv */

#else // ARCH_X86

static inline void
dv_render_macroblock_yuv(dv_decoder_t *dv, dv_macroblock_t *mb, guchar **pixels, guint16 *pitches) {
  if(dv->sampling == e_dv_sample_411) {
    if(mb->x >= 704) {
      dv_mb411_right_YUY2(mb, pixels[0], pitches[0]); // Right edge are 420!
    } else {
      dv_mb411_YUY2(mb, pixels[0], pitches[0]);
    } // else
  } else {
    dv_mb420_YV12(mb, pixels, pitches);
  } // else
} /* dv_render_macroblock_yuv */

void
dv_render_video_segment_yuv(dv_decoder_t *dv, dv_videosegment_t *seg, guchar **pixels, guint16 *pitches) {
  dv_macroblock_t *mb;
  gint m;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_YUY2(mb, pixels[0], pitches[0]); // Right edge are 420!
      } else {
	dv_mb411_YUY2(mb, pixels[0], pitches[0]);
      } // else
    } else {
      dv_mb420_YV12(mb, pixels, pitches);
    } // else
  } // for   
} /* dv_render_video_segment_yuv */

#endif // ! ARCH_X86

void
dv_decode_full_frame(dv_decoder_t *dv, guchar *buffer, 
		     dv_color_space_t color_space, guchar **pixels, guint16 *pitches) {

  static dv_videosegment_t vs;
  dv_videosegment_t *seg = &vs;
  dv_macroblock_t *mb;
  gint ds, v, m;
  guint offset = 0, dif = 0;

  if(!seg->bs) {
    seg->bs = bitstream_init();
    if(!seg->bs) 
      goto nomem;
  } // if
  seg->isPAL = (dv->system == e_dv_system_625_50);

  // each DV frame consists of a sequence of DIF segments 
  for (ds=0; ds < dv->num_dif_seqs; ds++) { 
    // Each DIF segment conists of 150 dif blocks, 135 of which are video blocks
    /* A video segment consists of 5 video blocks, where each video
       block contains one compressed macroblock.  DV bit allocation
       for the VLC stage can spill bits between blocks in the same
       video segment.  So parsing needs the whole segment to decode
       the VLC data */
    dif += 6;
    // Loop through video segments 
    for (v=0;v<27;v++) {
      // skip audio block - interleaved before every 3rd video segment
      if(!(v % 3)) dif++; 
      // stage 1: parse and VLC decode 5 macroblocks that make up a video segment
      offset = dif * 80;
      bitstream_new_buffer(seg->bs, buffer + offset, 80*5); 
      dv_parse_video_segment(seg, dv->quality);
      // stage 2: dequant/unweight/iDCT blocks, and place the macroblocks
      dif+=5;
      seg->i = ds;
      seg->k = v;
      if(color_space == e_dv_color_yuv) {
	for (m=0,mb = seg->mb;
	     m<5;
	     m++,mb++) {
	  dv_decode_macroblock(dv, mb, dv->quality);
	  dv_place_macroblock(dv, seg, mb, m);
	  dv_render_macroblock_yuv(dv, mb, pixels, pitches);
	} // for m
      } else {
	for (m=0,mb = seg->mb;
	     m<5;
	     m++,mb++) {
	  dv_decode_macroblock(dv, mb, dv->quality);
	  dv_place_macroblock(dv, seg, mb, m);
	  dv_render_macroblock_rgb(dv, mb, pixels[0], pitches[0]);
	} // for m
      } // else color_space
    } // for v
  } // ds
  return;
 nomem:
  fprintf(stderr,"no memory for bitstream!\n");
  exit(-1);
} // dv_decode_full_frame 

