/* 
 *  dct.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <string.h>
#include "dct.h"
#include "weighting.h"
#include "mmx.h"

static double KC248[8][4][4][8];
static double KC88[8][8][8][8];
static double C[8];

extern dv_coeff_t postSC[64] __attribute__ ((aligned (32)));

static inline short int_val(float f)
{
	return (short) floor(f + 0.5);
}

void dct_init(void) {
	int x, y, z, h, v, u, i;
    
  for (x = 0; x < 8; x++) {
    for (z = 0; z < 4; z++) {
      for (u = 0; u < 4; u++) {
        for (h = 0; h < 8; h++) {
		KC248[x][z][u][h] = 
			cos((M_PI * u * ((2.0 * z) + 1.0)) / 8.0) *
			cos((M_PI * h * ((2.0 * x) + 1.0)) / 16.0);
        }                       /* for h */
      }                         /* for u */
    }                           /* for z */
  }                             /* for x */

  for (x = 0; x < 8; x++) {
    for (y = 0; y < 8; y++) {
      for (v = 0; v < 8; v++) {
        for (h = 0; h < 8; h++) {
          KC88[x][y][h][v] =
            cos((M_PI * v * ((2.0 * y) + 1.0)) / 16.0) *
            cos((M_PI * h * ((2.0 * x) + 1.0)) / 16.0);
        }
      }
    }
  }

  for (i = 0; i < 8; i++) {
	  C[i] = (i == 0 ? 0.5 / sqrt(2.0) : 0.5);
  } /* for i */

}

void idct_block_mmx(gint16 *block);
void dct_block_mmx(gint16* in_block, gint16* out_block);

typedef short var;
//for DCT
#define A1 0.70711
#define A2 0.54120
#define A3 0.70711
#define A4 1.30658
#define A5 0.38268

#if 0
static void dct_aan_line(short* in, short* out)
{
	var v0, v1, v2, v3, v4, v5, v6, v7;
	var v00,v01,v02,v03,v04,v05,v06,v07;   
	var v10,v11,v12,v13,v14,v15,v16;   
	var v20,v21,v22;   
	var v32,v34,v35,v36;   
	var v42,v43,v45,v47;   
	var v54,v55,v56,v57,va0;   

	var v32b,v34b,v35b,v36b;   
	var v42b,v43b,v45b,v47b;   
	var v54b,v55b,v56b,v57b,va0b;   

	int i,n;
	v0 = in[0];
	v1 = in[1];
	v2 = in[2];
	v3 = in[3];
	v4 = in[4];
	v5 = in[5];
	v6 = in[6];
	v7 = in[7];
	
	// first butterfly stage 
	v00 = v0+v7;	  //0
	v07 = v0-v7;	  //7
	v01 = v1+v6;	  //1
	v06 = v1-v6;	  //6
	v02 = v2+v5;	  //2
	v05 = v2-v5;	  //5
	v03 = v3+v4;	  //3
	v04 = v3-v4;	  //4

	fprintf(stderr, "\nv0?: %d %d %d %d %d %d %d %d\n", 
		v00, v01, v02, v03, v04, v05, v06, v07);
	
	//second low butterfly
	v10=v00+v03;	     //0
	v13=v00-v03;		 //3
	v11=v01+v02;		 //1
	v12=v01-v02;		 //2
	
	//second high
	v16=v06+v07;		 //6
	v15=v05+v06;		 //5
	v14=-(v04+v05);	 //4
	//7 v77 without change

	fprintf(stderr, "v1?: %d %d %d %d %d %d %d %d -- %d\n", 
		v10, v11, v12, v13, v14, v15, v16, v07, v14+v16);
	
	//third	(only 3 real new terms)
	v20=v10+v11;			    //0
	v21=v10-v11;				//1
	v22=v12+v13;   			//2
	va0=(v14+v16)*A5; // temporary for A5 multiply
	va0b=((v14+v16)*25079) >> 16;	 // temporary for A5 multiply

	fprintf(stderr, "v2?: %d %d %d va0: %d\n", 
		v20, v21, v22, va0);

	fprintf(stderr, "v2?: %d %d %d va0: %d\n", 
		v20, v21, v22, va0b);
	
	//fourth
	v32=v22*A1;                // 2
	v34=-(v14*A2+va0);      // 4 ?
	v36=v16*A4-va0;         // 6 ?
	v35=v15*A3;                // 5

	v32b=(v22*23171) >> 15;                // 2
	v34b=-(((v14*17734) >> 15)+va0b);      // 4 ?
	v36b=((v16*21407) >> 14)-va0b;         // 6 ?
	v35b=(v15*23171) >> 15;                // 5

	fprintf(stderr, "v3?: -- -- %d -- %d %d %d\n", 
		v32, v34, v35, v36);
	fprintf(stderr, "v3?: -- -- %d -- %d %d %d\n", 
		v32b, v34b, v35b, v36b);
	
	//fifth
	v42=v32+v13;                    //2
	v43=v13-v32;                    //3
	v45=v07+v35;                    //5
	v47=v07-v35;                    //7

	v42b=v32b+v13;                    //2
	v43b=v13-v32b;                    //3
	v45b=v07+v35b;                    //5
	v47b=v07-v35b;                    //7

	fprintf(stderr, "v4?: -- -- %d %d -- %d -- %d\n", 
		v42, v43, v45, v47);

	fprintf(stderr, "v4?: -- -- %d %d -- %d -- %d\n", 
		v42b, v43b, v45b, v47b);
	
	//last
	v54=v34+v47;                         //4
	v57=v47-v34;                         //7
	v55=v45+v36;                         //5
	v56=v45-v36;                         //5

	v54b=v34b+v47b;                         //4
	v57b=v47b-v34b;                         //7
	v55b=v45b+v36b;                         //5
	v56b=v45b-v36b;                         //5

	fprintf(stderr, "v5?: -- -- -- -- %d %d %d %d\n", 
		v54, v55, v56, v57);
	fprintf(stderr, "v5?: -- -- -- -- %d %d %d %d\n", 
		v54b, v55b, v56b, v57b);
	
	// output butterfly
	
	out[0] = v20;
	out[1] = v55;
	out[2] = v42;
	out[3] = v57;
	out[4] = v21;
	out[5] = v54;
	out[6] = v43;
	out[7] = v56;
}
#else
static void dct_aan_line(short* in, short* out)
{
	var v0, v1, v2, v3, v4, v5, v6, v7;
	var v00,v01,v02,v03,v04,v05,v06,v07;   
	var v10,v11,v12,v13,v14,v15,v16;   
	var v20,v21,v22;   
	var v32,v34,v35,v36;   
	var v42,v43,v45,v47;   
	var v54,v55,v56,v57,va0;   

	v0 = in[0];
	v1 = in[1];
	v2 = in[2];
	v3 = in[3];
	v4 = in[4];
	v5 = in[5];
	v6 = in[6];
	v7 = in[7];
	
	// first butterfly stage 
	v00 = v0+v7;	  //0
	v07 = v0-v7;	  //7
	v01 = v1+v6;	  //1
	v06 = v1-v6;	  //6
	v02 = v2+v5;	  //2
	v05 = v2-v5;	  //5
	v03 = v3+v4;	  //3
	v04 = v3-v4;	  //4

	//second low butterfly
	v10=v00+v03;	     //0
	v13=v00-v03;		 //3
	v11=v01+v02;		 //1
	v12=v01-v02;		 //2
	
	//second high
	v16=v06+v07;		 //6
	v15=v05+v06;		 //5
	v14=-(v04+v05);	 //4
	//7 v77 without change

	//third	(only 3 real new terms)
	v20=v10+v11;			    //0
	v21=v10-v11;				//1
	v22=v12+v13;   			//2
#if 0
	va0=(v14+v16)*A5; // temporary for A5 multiply
#endif
	va0=((v14+v16)*25079) >> 16;	 // temporary for A5 multiply

	//fourth
#if 0
	v32=v22*A1;                // 2
	v34=-(v14*A2+va0);      // 4 ?
	v36=v16*A4-va0;         // 6 ?
	v35=v15*A3;                // 5
#endif

	v32=(v22*23171) >> 15;                // 2
	v34=-(((v14*17734) >> 15)+va0);      // 4 ?
	v36=((v16*21407) >> 14)-va0;         // 6 ?
	v35=(v15*23171) >> 15;                // 5

	//fifth
	v42=v32+v13;                    //2
	v43=v13-v32;                    //3
	v45=v07+v35;                    //5
	v47=v07-v35;                    //7

	//last
	v54=v34+v47;                         //4
	v57=v47-v34;                         //7
	v55=v45+v36;                         //5
	v56=v45-v36;                         //5

	// output butterfly
	
	out[0] = v20;
	out[1] = v55;
	out[2] = v42;
	out[3] = v57;
	out[4] = v21;
	out[5] = v54;
	out[6] = v43;
	out[7] = v56;
}
#endif

static inline void postscale(var v[64])
{
	int i;
	int factor = pow(2, 16 + DCT_YUV_PRECISION);
	for( i=0; i<64; i++ ) {
		v[i] = ( v[i] * postSC[i] ) / factor;
	}
}

static inline void dct_aan(short *s_in, short *s_out)
{
	int i,j,r,c;

	for( c=0; c<64; c += 8 ) // columns
		dct_aan_line(s_in + c, s_in + c);

	for ( i=0; i < 8; i++) {
		for (j=0; j < 8; j++) {
			s_out[i * 8 + j] = s_in[j * 8 + i];
		}
	}

	for( r=0; r<64; r+= 8 ) // then rows
		dct_aan_line(s_out + r, s_out + r);

}

// #define BRUTE_FORCE_DCT_88 1

extern short mmx_va0[4];
extern short mmx_v16[4];
extern short mmx_v32[4];
extern short mmx_v34[4];
extern short mmx_v35[4];
extern short mmx_v36[4];

/* Input has to be transposed !!! */

void dct_88(dv_coeff_t *block, dv_coeff_t *block_out) {
#if !ARCH_X86
#if BRUTE_FORCE_DCT_88
	int v,h,y,x,i;
	double temp[64];

	memset(temp,0,sizeof(temp));
	for (v=0;v<8;v++) {
		for (h=0;h<8;h++) {
			for (y=0;y<8;y++) {
				for (x=0;x<8;x++) {
					temp[v*8+h] += block[y*8+x] 
						* KC88[x][y][h][v];
				}
			}
			temp[v*8+h] *= (C[h] * C[v]);
		}
	}

	for (i=0;i<64;i++) {
		block_out[i] = temp[i];
	}
#else
	dct_aan(block, block_out);
	postscale(block_out);
#endif

#else
#if 0
	int i,j;
	short tester[64];
	short blcopy[64];

	memcpy(blcopy, block, sizeof(blcopy));

	for ( i=0; i < 8; i++) {
		for (j=0; j < 8; j++) {
			tester[i * 8 + j] = block[j * 8 + i];
		}
	}
#endif
#if 0
	int i,j;
	int err = 0;
	short tester[64];
	short blcopy[64];
	short blcopy2[64];

	memcpy(blcopy, block, sizeof(blcopy));
	memcpy(blcopy2, block, sizeof(blcopy));
	dct_aan(blcopy2, tester);
	dct_block_mmx(block, block);
	for ( i=0; i < 8; i++) {
		for (j=0; j < 8; j++) {
			block_out[i * 8 + j] = block[j * 8 + i];
		}
	}
	dct_block_mmx(block_out, block_out);
	emms();

	err = 0;
	for ( i = 0; i < 64; i++) {
		err += abs(block_out[i] - tester[i]);
	}

	if (err > 0) {
		fprintf(stderr, "DCT tester: (Input / MMX / correct)\n");
		for ( j=0; j < 8; j++) {
			for ( i=0; i < 8; i++) {
				fprintf(stderr, "%d\t", blcopy[i * 8 + j]);
			}
			fprintf(stderr, "\t");
			for ( i=0; i < 8; i++) {
				fprintf(stderr, "%d\t", block_out[i * 8 + j]);
			}
			fprintf(stderr, "\t");
			for ( i=0; i < 8; i++) {
				fprintf(stderr, "%d\t", tester[i * 8 + j]);
			}
			fprintf(stderr, "\n");
		}
		fprintf(stderr, "\n");
	}
#endif
	dct_block_mmx(block, block_out);
	emms();
	postscale(block_out);

#if 0

	for( i=0; i<64; i += 8 ) // columns
		dct_aan_line(tester + i, tester + i);
#endif
#if 0
	dct_block_mmx(block, block_out);
	emms();
	postscale(block_out);
#endif

#if 0
	fprintf(stderr, "\nmmx_v32: %d %d %d %d\n", 
		mmx_v32[0],mmx_v32[1],mmx_v32[2],mmx_v32[3]);
	fprintf(stderr, "\nmmx_v16: %d %d %d %d\n", 
		mmx_v16[0],mmx_v16[1],mmx_v16[2],mmx_v16[3]);
	fprintf(stderr, "\nmmx_va0: %d %d %d %d\n", 
		mmx_va0[0],mmx_va0[1],mmx_va0[2],mmx_va0[3]);
	fprintf(stderr, "\nmmx_v34: %d %d %d %d\n", 
		mmx_v34[0],mmx_v34[1],mmx_v34[2],mmx_v34[3]);
	fprintf(stderr, "\nmmx_v35: %d %d %d %d\n", 
		mmx_v35[0],mmx_v35[1],mmx_v35[2],mmx_v35[3]);
	fprintf(stderr, "\nmmx_v36: %d %d %d %d\n", 
		mmx_v36[0],mmx_v36[1],mmx_v36[2],mmx_v36[3]);
	fprintf(stderr, "DCT tester: (Input / MMX / correct)\n");
	for ( j=0; j < 8; j++) {
		for ( i=0; i < 8; i++) {
			fprintf(stderr, "%d\t", blcopy[j * 8 + i]);
		}
		fprintf(stderr, "\t");
		for ( i=0; i < 8; i++) {
			fprintf(stderr, "%d\t", block[j * 8 + i]);
		}
		fprintf(stderr, "\t");
		for ( i=0; i < 8; i++) {
			fprintf(stderr, "%d\t", tester[i * 8 + j]);
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");

	for ( i=0; i < 8; i++) {
		for (j=0; j < 8; j++) {
			block_out[i * 8 + j] = block[j * 8 + i];
		}
	}
	dct_block_mmx(block_out, block_out);

	emms();
#endif

	// postscale_transpose(block_out);
	
	// idct_block_mmx(tblock, get_preSC_inverse());
	
	// transpose(block_out);

#if 0
	for (y=0; y<8;y++) {
		for (x=0; x<8;x++) {
			block[y*8 + x] = tblock[x*8 + y];
		}
	}
#endif
#endif

#if 0
	for (i = 0; i < 64; i++) {
		fprintf(stderr, "%g %d\n", temp[i], block_out[i]); 
	}
#endif

}

void dct_248(dv_coeff_t *block) {
  int u,h,z,x,i;
  double temp[64];
  int factor = pow(2, DCT_YUV_PRECISION);

  memset(temp,0,sizeof(temp));
  for (u=0;u<4;u++) {
    for (h=0;h<8;h++) {
      for (z=0;z<4;z++) {
        for (x=0;x<8;x++) {
          temp[u*8+h] += (block[x*8+2*z] + block[x*8+(2*z+1)]) *
                            KC248[x][z][u][h];
          temp[(u+4)*8+h] += (block[x*8+2*z] - block[x*8+(2*z+1)]) *
                              KC248[x][z][u][h];
        }
      }
      temp[u*8+h] *= (C[h] * C[u]);
      temp[(u+4)*8+h] *= (C[h] * C[u]);
    }
  }


  for (i=0;i<64;i++)
    block[i] = temp[i] / factor;
}

void idct_88(dv_coeff_t *block) {
#if !ARCH_X86
  int v,h,y,x,i;
  double temp[64];

  memset(temp,0,sizeof(temp));
  for (v=0;v<8;v++) {
    for (h=0;h<8;h++) {
      for (y=0;y<8;y++){ 
        for (x=0;x<8;x++) {
          temp[y*8+x] += C[v] * C[h] * block[v*8+h] * KC88[x][y][h][v];
        }
      }
    }
  }

  for (i=0;i<64;i++)
    block[i] = temp[i];
#else
  idct_block_mmx(block);
  emms();
#endif
}

#if BRUTE_FORCE_248

void idct_248(double *block) {
  int u,h,z,x,i;
  double temp[64];
  double temp2[64];
  double b,c;
  double (*in)[8][8], (*out)[8][8]; /* don't really need storage -- fixup later */

#if 0
  // This is to identify visually where 248 blocks are...
  for(i=0;i<64;i++) {
    block[i] = 235 - 128;
  }
  return;
#endif

  memset(temp,0,sizeof(temp));

  out = &temp;
  in = &temp2;

  
  for(z=0;z<8;z++) {
    for(x=0;x<8;x++)
      (*in)[z][x] = block[z*8+x];
  }

    for (x = 0; x < 8; x++) {
      for (z = 0; z < 4; z++) {
	for (u = 0; u < 4; u++) {
	  for (h = 0; h < 8; h++) {
	    b = (double)(*in)[u][h];  
	    c = (double)(*in)[u+4][h];
	    (*out)[2*z][x] += C[u] * C[h] * (b + c) * KC248[x][z][u][h];
	    (*out)[2*z+1][x] += C[u] * C[h] * (b - c) * KC248[x][z][u][h];
	  }                       /* for h */
	}                         /* for u */
      }                           /* for z */
    }                             /* for x */

#if 0
  for (u=0;u<4;u++) {
    for (h=0;h<8;h++) {
      for (z=0;z<4;z++) {
        for (x=0;x<8;x++) {
          b = block[u*8+h];
          c = block[(u+4)*8+h];
          temp[(2*u)*8+h] += C[h] * C[u] * (b + c) * KC248[x][z][h][u];
          temp[(2*u+1)*8+h] += C[h] * C[u] * (b - c) * KC248[x][z][h][u];
        }
      }
    }
  }
#endif

  for(z=0;z<8;z++) {
    for(x=0;x<8;x++)
      block[z*8+x] = (*out)[z][x];
  }
}


#endif // BRUTE_FORCE_248
