
#include <stdio.h>

#include "dv_types.h"
#include "bitstream.h"

#define offsetof(S, M) \
    ((int)&(((S*)NULL)->M))

#define declare(S, M) \
    printf("#define %-40s %d\n", #S "_" #M, offsetof(S, M))
#define declaresize(S) \
    printf("#define %-40s %d\n", #S "_size", sizeof(S))
#define export(S) \
    printf("#define %-40s %d\n", #S, S)

int main(int argc, char *argv[])
{
  declare(dv_videosegment_t,	i);
  declare(dv_videosegment_t,	k);
  declare(dv_videosegment_t,	bs);
  declare(dv_videosegment_t,	mb);
  declare(dv_videosegment_t,	isPAL);

  declaresize(dv_macroblock_t);
  declare(dv_macroblock_t,	b);
  declare(dv_macroblock_t,	eob_count);
  declare(dv_macroblock_t,	vlc_error);
  declare(dv_macroblock_t,	qno);
  declare(dv_macroblock_t,	sta);
  declare(dv_macroblock_t,	i);
  declare(dv_macroblock_t,	j);
  declare(dv_macroblock_t,	k);

  declaresize(dv_block_t);
  declare(dv_block_t,		coeffs);
  declare(dv_block_t,		dct_mode);
  declare(dv_block_t,		class_no);
  declare(dv_block_t,		reorder);
  declare(dv_block_t,		reorder_sentinel);
  declare(dv_block_t,		offset);
  declare(dv_block_t,		end);
  declare(dv_block_t,		eob);
  declare(dv_block_t,		mark);

  declare(bitstream_t,	buf);

  export(DV_QUALITY_BEST);
  export(DV_QUALITY_FASTEST);
  export(DV_QUALITY_COLOR);
  export(DV_QUALITY_AC_MASK);
  export(DV_QUALITY_DC);
  export(DV_QUALITY_AC_1);
  export(DV_QUALITY_AC_2);
  export(DV_WEIGHT_BIAS);

  return 0;
}
