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

void dv_init(void) {
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

void 
dv_decode_video_segment(dv_videosegment_t *seg, guint quality) {
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
	quant_88_inverse(bl->coeffs,mb->qno,bl->class_no);
	weight_88_inverse(bl->coeffs);
	idct_88(bl->coeffs);
      } // else
    } // for b
  } // for mb
} /* dv_decode_video_segment */

void
dv_render_video_segment_rgb(dv_videosegment_t *seg, dv_sample_t sampling, 
			    guchar *pixels, gint pitch ) {
  dv_macroblock_t *mb;
  gint m, x, y;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(sampling == e_dv_sample_411) {
      dv_place_411_macroblock(mb,&x,&y);
      if((mb->j == 4) && (mb->k > 23)) {
	dv_mb420_rgb(mb, pixels, pitch, x, y); // Right edge are 420!
      } else {
	dv_mb411_rgb(mb, pixels, pitch, x, y);
      } // else
    } else {
      dv_place_420_macroblock(mb,&x,&y);
      dv_mb420_rgb(mb, pixels, pitch, x, y);
    } // else
  } // for   
} /* dv_render_video_segment_rgb */


void
dv_render_video_segment_yuv(dv_videosegment_t *seg, dv_sample_t sampling, 
			    guchar **pixels, guint16 *pitches) {
  dv_macroblock_t *mb;
  gint m, x, y;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(sampling == e_dv_sample_411) {
      dv_place_411_macroblock(mb,&x,&y);
      if((mb->j == 4) && (mb->k > 23)) {
	dv_mb420_YUY2(mb, pixels[0], pitches[0], x, y); // Right edge are 420!
      } else {
	dv_mb411_YUY2(mb, pixels[0], pitches[0], x, y);
      } // else
    } else {
      dv_place_420_macroblock(mb,&x,&y);
      dv_mb420_YV12(mb, pixels, pitches, x, y);
    } // else
  } // for   
} /* dv_render_video_segment_rgb */

#if USE_MMX_ASM
void
dv_render_video_segment_yuv_mmx(dv_videosegment_t *seg, dv_sample_t sampling, 
				guchar **pixels, guint16 *pitches) {
  dv_macroblock_t *mb;
  gint m, x, y;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(sampling == e_dv_sample_411) {
      dv_place_411_macroblock(mb,&x,&y);
      if((mb->j == 4) && (mb->k > 23)) {
	dv_mb420_YUY2_mmx(mb, pixels[0], pitches[0], x, y); // Right edge are 420!
      } else {
	dv_mb411_YUY2_mmx(mb, pixels[0], pitches[0], x, y);
      } // else
    } else {
      dv_place_420_macroblock(mb,&x,&y);
      dv_mb420_YV12_mmx(mb, pixels, pitches, x, y);
    } // else
  } // for   
} /* dv_render_video_segment_rgb */
#endif
