#include <glib.h>
#include <stdio.h>

#include "bitstream.h"
#include "dv.h"
#include "vlc.h"
#include "parse.h"

#define offsetof(S, M) \
    ((int)&(((S*)NULL)->M))

#define declare(S, M) \
    printf("#define %-40s %d\n", #S "_" #M, offsetof(S, M))
#define declaresize(S) \
    printf("#define %-40s %d\n", #S "_size", sizeof(S))

int main(int argc, char *argv[])
{
    declaresize(dv_macroblock_t);
    declare(dv_macroblock_t,	eob_count);

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

    return 0;
}
