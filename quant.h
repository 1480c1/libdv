/* Copyright (C) 2000 Oregon Graduate Institute of Science & Technology
 *
 * See the file COPYRIGHT, which should have been supplied with this
 * package, for licensing details.  We may be contacted through email
 * at <quasar-help@cse.ogi.edu>.
 */

#ifndef DV_QUANT_H
#define DV_QUANT_H

#include <dv_types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void quant(dv_coeff_t *block,int qno,int class);
extern void quant_88_inverse(dv_coeff_t *block,int qno,int class);
extern void (*quant_248_inverse) (dv_coeff_t *block,int qno,int class,
                                  dv_248_coeff_t *co);
extern void quant_88_inverse_x86(dv_coeff_t *block,int qno,int class);
extern void dv_quant_init (dv_decoder_t *dv);
#ifdef __cplusplus
}
#endif

#endif // DV_QUANT_H
