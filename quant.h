/* Copyright (C) 2000 Oregon Graduate Institute of Science & Technology
 *
 * See the file COPYRIGHT, which should have been supplied with this
 * package, for licensing details.  We may be contacted through email
 * at <quasar-help@cse.ogi.edu>.
 */

#ifndef DV_QUANT_H
#define DV_QUANT_H

#include "dv_types.h"

extern void quant_88(dv_coeff_t *block,int qno,int class);
extern void quant_248(dv_coeff_t *block,int qno,int class);
extern void quant_88_inverse(dv_coeff_t *block,int qno,int class);
extern void quant_248_inverse(dv_coeff_t *block,int qno,int class);
extern void quant_88_inverse_x86(dv_coeff_t *block,int qno,int class);

#endif // DV_QUANT_H
