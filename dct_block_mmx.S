# Copyright (C) 1995-99 -  Intel Corporation all rights reserved
# esi - input and output data pointer
# the input data each 16 bit element in the 8x8 matrix is left aligned:
# the output data is tranposed and each 16 bit element in the 8x8 matrix is left aligned:
# e.g. in 11...1110000 format
# israelh. 11/11/97 removed emms. moved to stub
# MMX implementation. Using MMX transpose 

#define YUV_PRECISION  1
	
#define NSHIFT     15
#define PRESHIFT   1
#define WA4_SHIFT  (NSHIFT-1)
#define WA5_SHIFT  (NSHIFT+1)


.data

.align 8	
			
WA1: .word 23171,23171,23171,23171  /* 0.70711 * 32768 */
WA2: .word 17734,17734,17734,17734  /* 0.54120 * 32768 */
WA3: .word 23171,23171,23171,23171  /* 0.70711 * 32768 */
WA4: .word 21407,21407,21407,21407  /* 1.30658 * 16384 */
WA5: .word 25079,25079,25079,25079  /* 0.38268 * 65536 */

scratch1:       .quad 0
scratch2:       .quad 0
scratch3:       .quad 0
scratch4:       .quad 0

.text

.align 8	
.global dct_88_block_mmx
dct_88_block_mmx:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi

	movl    8(%ebp), %esi          # source

# column 0
	movq 16*0(%esi), %mm0          # v0
	movq 16*7(%esi), %mm1          # v7
	movq  %mm0, %mm2               # duplicate v0 
	paddw %mm1, %mm0               # v00: v0+v7  
	psubw %mm1, %mm2               # v07: v0-v7  

	movq 16*1(%esi), %mm1          # v1
	movq 16*6(%esi), %mm3          # v6
	movq  %mm1, %mm4               # duplicate v1 
	paddw %mm3, %mm1               # v01: v1+v6  
	psubw %mm3, %mm4               # v06: v1-v6  

	movq 16*2(%esi), %mm3          # v2
	movq 16*5(%esi), %mm5          # v5
	movq  %mm3, %mm6               # duplicate v2 
	paddw %mm5, %mm3               # v02: v2+v5  
	psubw %mm5, %mm6               # v05: v2-v5  

	movq 16*3(%esi), %mm5          # v3
	movq 16*4(%esi), %mm7          # v4
	movq  %mm7, scratch1           # scratch1: v4   ; 
	movq  %mm5, %mm7               # duplicate v3 
	paddw scratch1, %mm5           # v03: v3+v4  
	psubw scratch1, %mm7           # v04: v3-v4  
	movq  %mm5, scratch2           # scratch2: v03
	movq  %mm0, %mm5               # mm5: v00

	paddw scratch2, %mm0           # v10: v00+v03   
	psubw scratch2, %mm5           # v13: v00-v03   
	movq  %mm3, scratch3           # scratch3: v02
	movq  %mm1, %mm3               # duplicate v01

	paddw scratch3, %mm1          # v11: v01+v02
	psubw scratch3, %mm3          # v12: v01-v02

	movq  %mm6, scratch4           # scratch4: v05
	movq  %mm0, %mm6               # duplicate v10

	paddw %mm1, %mm0              # v10+v11
	psubw %mm1, %mm6              # v10-v11

	movq  %mm0, (%esi)             # out0: v10+v11 
	movq  %mm6, 16*4(%esi)         # out4: v10-v11 

	movq  %mm4, %mm0               # mm0: v06
	paddw scratch4, %mm4          # v15: v05+v06 
	paddw  %mm2, %mm0             # v16: v07+v06

	pmulhw WA3, %mm4               # v35~: WA3*v15
	psllw  $1, %mm4                # v35: compensate the coeefient scale

	movq   %mm4, %mm6              # duplicate v35
	paddw  %mm2, %mm4             # v45: v07+v35
	psubw  %mm6, %mm2             # v47: v07-v35

	paddw  %mm5, %mm3             # v22: v12+v13

	pmulhw WA1, %mm3               # v32~: WA1*v22
	psllw  $16-NSHIFT, %mm3        # v32: compensate the coeefient scale
	movq   %mm5, %mm6              # duplicate v13

	paddw  %mm3, %mm5             # v13+v32
	psubw  %mm3, %mm6             # v13-v32

	movq  %mm5, 16*2(%esi)         # out2: v13+v32 
	movq  %mm6, 16*6(%esi)         # out6: v13-v32 


	paddw  scratch4, %mm7         # v14n: v04+v05
	movq   %mm0, %mm5              # duplicate v16

	psubw  %mm7, %mm0             # va1: v16-v14n
	pmulhw WA5, %mm0               # va0~:  va1*WA5
	pmulhw WA4, %mm5               # v36~~: v16*WA4
	pmulhw WA2, %mm7               # v34~~: v14n*WA2
	psllw  $16-WA4_SHIFT, %mm5     # v36: compensate the coeefient scale 
	psllw  $16-NSHIFT, %mm7        # v34: compensate the coeefient scale

	psubw  %mm0, %mm5              # v36~: v36~~-va0~
	psubw  %mm0, %mm7              # v34~: v34~~-va0~

	movq   %mm4, %mm0              # duplicate v45
	paddw  %mm5, %mm4              # v45+v36
	psubw  %mm5, %mm0              # v45-v36

	movq  %mm4, 16*1(%esi)         # out1: v45+v36 
	movq  %mm0, 16*7(%esi)         # out7: v45-v36 

	movq   %mm2, %mm5              # duplicate v47
	paddw  %mm7, %mm2              # v47+v34
	psubw  %mm7, %mm5              # v47-v34

	movq  %mm2, 16*5(%esi)         # out5: v47+v34 
	movq  %mm5, 16*3(%esi)         # out3: v47-v34 


# column 1

	addl $8, %esi                  # point to the next 4 columns. 
		                       # it can be done by adding 8 to 
		                       # immediates but this is nicer

	movq (%esi), %mm0                  # v0
	movq 16*7(%esi), %mm1              # v7
	movq  %mm0, %mm2               # duplicate v0 
	paddw %mm1, %mm0               # v00: v0+v7  
	psubw %mm1, %mm2               # v07: v0-v7  

	movq 16(%esi), %mm1                # v1
	movq 16*6(%esi), %mm3              # v6
	movq  %mm1, %mm4               # duplicate v1 
	paddw %mm3, %mm1               # v01: v1+v6  
	psubw %mm3, %mm4               # v06: v1-v6  

	movq 16*2(%esi), %mm3         # v2
	movq 16*5(%esi), %mm5              # v5
	movq  %mm3, %mm6               # duplicate v2 
	paddw %mm5, %mm3               # v02: v2+v5  
	psubw %mm5, %mm6               # v05: v2-v5  

	movq 16*3(%esi), %mm5              # v3
	movq 16*4(%esi), %mm7              # v4
	movq  %mm7, scratch1                    # scratch1: v4   ; 
	movq  %mm5, %mm7               # duplicate v3 
	paddw scratch1, %mm5           # v03: v3+v4  
	psubw scratch1, %mm7           # v04: v3-v4  
	movq  %mm5, scratch2        # scratch2: v03
	movq  %mm0, %mm5               # mm5: v00

	paddw scratch2, %mm0           # v10: v00+v03   
	psubw scratch2, %mm5           # v13: v00-v03   
	movq  %mm3, scratch3         # scratc3: v02
	movq  %mm1, %mm3               # duplicate v01

	paddw scratch3, %mm1           # v11: v01+v02
	psubw scratch3, %mm3           # v12: v01-v02

	movq  %mm6, scratch4         # scratc4: v05
	movq  %mm0, %mm6               # duplicate v10

	paddw %mm1, %mm0                            # v10+v11
	psubw %mm1, %mm6                        # v10-v11

	movq  %mm0, (%esi)         # out0: v10+v11 
	movq  %mm6, 16*4(%esi)          # out4: v10-v11 

	movq  %mm4, %mm0             # mm0: v06
	paddw scratch4, %mm4         # v15: v05+v06 
	paddw  %mm2, %mm0                       # v16: v07+v06

	pmulhw WA3, %mm4           # v35~: WA3*v15
	psllw  $16-NSHIFT, %mm4       # v35: compensate the coeefient scale

	movq   %mm4, %mm6            # duplicate v35
	paddw  %mm2, %mm4            # v45: v07+v35
	psubw  %mm6, %mm2            # v47: v07-v35

	paddw  %mm5, %mm3            # v22: v12+v13

	pmulhw WA1, %mm3           # v32~: WA3*v15
	psllw  $16-NSHIFT, %mm3       # v32: compensate the coeefient scale
	movq   %mm5, %mm6            # duplicate v13

	paddw  %mm3, %mm5            # v13+v32
	psubw  %mm3, %mm6            # v13-v32

	movq  %mm5, 16*2(%esi)          # out2: v13+v32 
	movq  %mm6, 16*6(%esi)          # out6: v13-v32 

	paddw  scratch4, %mm7                           # v14n: v04+v05
	movq   %mm0, %mm5                               # duplicate v16

	psubw  %mm7, %mm0                               # va1: v16-v14n
	pmulhw WA2, %mm7                # v34~~: v14n*WA2
	pmulhw WA5, %mm0                # va0~:  va1*WA5
	pmulhw WA4, %mm5                        # v36~~: v16*WA4
	psllw  $16-NSHIFT, %mm7
	psllw  $16-WA4_SHIFT, %mm5      # v36: compensate the coeffient 
		# scale note that WA4 is shifted 1 bit less than the others
	
	psubw  %mm0, %mm5   # v36~: v36~~-va0~
	psubw  %mm0, %mm7   # v34~: v34~~-va0~

	movq   %mm4, %mm0    # duplicate v45
	paddw  %mm5, %mm4    # v45+v36
	psubw  %mm5, %mm0    # v45-v36

	movq  %mm4, 16*1(%esi)          # out1: v45+v36 
	movq  %mm0, 16*7(%esi)          # out7: v45-v36 

	movq   %mm2, %mm5    # duplicate v47
	paddw  %mm7, %mm2    # v47+v34
	psubw  %mm7, %mm5    # v47-v34

	movq  %mm2, 16*5(%esi)          # out5: v47+v34 
	movq  %mm5, 16*3(%esi)          # out3: v47-v34 

	popl    %esi
	popl %ebp

	ret     $0

.align 8	
.global dct_block_mmx_postscale
dct_block_mmx_postscale:

	pushl	 %ebp
	movl	 %esp,%ebp
	pushl	 %esi
	pushl	 %edi

	movl	 8(%ebp),%esi		# source matrix 
	movl    12(%ebp), %edi          # postscale matrix 

	movq	0*8(%esi), %mm0
	movq	1*8(%esi), %mm1
	movq	2*8(%esi), %mm2
	movq	3*8(%esi), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	0*8(%edi), %mm0
	pmulhw	1*8(%edi), %mm1
	pmulhw	2*8(%edi), %mm2
	pmulhw	3*8(%edi), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 0*8(%esi)
	movq	%mm1, 1*8(%esi)
	movq	%mm2, 2*8(%esi)
	movq	%mm3, 3*8(%esi)

	
	movq	4*8(%esi), %mm0
	movq	5*8(%esi), %mm1
	movq	6*8(%esi), %mm2
	movq	7*8(%esi), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	4*8(%edi), %mm0
	pmulhw	5*8(%edi), %mm1
	pmulhw	6*8(%edi), %mm2
	pmulhw	7*8(%edi), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 4*8(%esi)
	movq	%mm1, 5*8(%esi)
	movq	%mm2, 6*8(%esi)
	movq	%mm3, 7*8(%esi)
	

	movq	8*8(%esi), %mm0
	movq	9*8(%esi), %mm1
	movq	10*8(%esi), %mm2
	movq	11*8(%esi), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	8*8(%edi), %mm0
	pmulhw	9*8(%edi), %mm1
	pmulhw	10*8(%edi), %mm2
	pmulhw	11*8(%edi), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 8*8(%esi)
	movq	%mm1, 9*8(%esi)
	movq	%mm2, 10*8(%esi)
	movq	%mm3, 11*8(%esi)

		
	movq	12*8(%esi), %mm0
	movq	13*8(%esi), %mm1
	movq	14*8(%esi), %mm2
	movq	15*8(%esi), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	12*8(%edi), %mm0
	pmulhw	13*8(%edi), %mm1
	pmulhw	14*8(%edi), %mm2
	pmulhw	15*8(%edi), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 12*8(%esi)
	movq	%mm1, 13*8(%esi)
	movq	%mm2, 14*8(%esi)
	movq	%mm3, 15*8(%esi)
			
	popl	 %edi
	popl	 %esi
	popl	 %ebp
	ret

.align 8	
.global dct_248_block_mmx
dct_248_block_mmx:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl   %edi

	movl    8(%ebp), %esi          # source

# column 0

	movq 16*0(%esi), %mm0          # v00
	movq 16*2(%esi), %mm1          # v01
	movq 16*4(%esi), %mm2          # v02
	movq 16*6(%esi), %mm3          # v03

	movq %mm0, %mm4                # v00
	movq %mm1, %mm5                # v01
	
	paddw %mm3, %mm0               # v10: v00+v03
	psubw %mm3, %mm4	       # v13: v00-v03
	
	paddw %mm2, %mm1	       # v11: v01+v02
	psubw %mm2, %mm5	       # v12: v01-v02
	
	movq %mm0, %mm3                # v10

	paddw %mm4, %mm5               # v22: v12+v13	
	paddw %mm1, %mm0	       # v20: v10+v11
	psubw %mm1, %mm3	       # v21: v10-v11

	pmulhw WA1, %mm5               # v32~: WA1*v22
	movq  %mm4, %mm2	
	psllw  $16-NSHIFT, %mm5        # v32: compensate the coeffient scale

	movq  %mm0, 16*0(%esi)
	
	psubw %mm5, %mm2	       # v43: v13-v32	
	paddw %mm4, %mm5	       # v42: v32+v13

	movq  %mm2, 16*6(%esi)
	movq  %mm3, 16*4(%esi)
	movq  %mm5, 16*2(%esi)

# column 1

	addl $8, %esi # point to the next 4 columns. 
	              # it can be done by adding 8 to immediates 
	              # but this is nicer

	movq 16*0(%esi), %mm0          # v00
	movq 16*2(%esi), %mm1          # v01
	movq 16*4(%esi), %mm2          # v02
	movq 16*6(%esi), %mm3          # v03

	movq %mm0, %mm4
	movq %mm1, %mm5
	
	paddw %mm3, %mm0               # v10: v00+v03
	psubw %mm3, %mm4	       # v13: v00-v03
	
	paddw %mm2, %mm1	       # v11: v01+v02
	psubw %mm2, %mm5	       # v12: v01-v02
	
	movq %mm0, %mm3

	paddw %mm4, %mm5               # v22: v12+v13	
	paddw %mm1, %mm0	       # v20: v10+v11
	psubw %mm1, %mm3	       # v21: v10-v11

	pmulhw WA1, %mm5               # v32~: WA1*v22
	movq  %mm4, %mm2	
	psllw  $16-NSHIFT, %mm5        # v32: compensate the coeffient scale

	movq  %mm0, 16*0(%esi)
	
	psubw %mm5, %mm2	       # v43: v13-v32	
	paddw %mm4, %mm5	       # v42: v32+v13

	movq  %mm2, 16*6(%esi)
	movq  %mm3, 16*4(%esi)
	movq  %mm5, 16*2(%esi)

# column 0 ... second line		

	addl $8, %esi

	movq 16*0(%esi), %mm0          # v00
	movq 16*2(%esi), %mm1          # v01
	movq 16*4(%esi), %mm2          # v02
	movq 16*6(%esi), %mm3          # v03

	movq %mm0, %mm4
	movq %mm1, %mm5
	
	paddw %mm3, %mm0               # v10: v00+v03
	psubw %mm3, %mm4	       # v13: v00-v03
	
	paddw %mm2, %mm1	       # v11: v01+v02
	psubw %mm2, %mm5	       # v12: v01-v02
	
	movq %mm0, %mm3

	paddw %mm4, %mm5               # v22: v12+v13	
	paddw %mm1, %mm0	       # v20: v10+v11
	psubw %mm1, %mm3	       # v21: v10-v11

	pmulhw WA1, %mm5               # v32~: WA1*v22
	movq  %mm4, %mm2	
	psllw  $16-NSHIFT, %mm5        # v32: compensate the coeffient scale

	movq  %mm0, 16*0(%esi)
	
	psubw %mm5, %mm2	       # v43: v13-v32	
	paddw %mm4, %mm5	       # v42: v32+v13

	movq  %mm2, 16*6(%esi)
	movq  %mm3, 16*4(%esi)
	movq  %mm5, 16*2(%esi)

# column 1 ... second line		

	addl $8, %esi
	
	movq 16*0(%esi), %mm0          # v00
	movq 16*2(%esi), %mm1          # v01
	movq 16*4(%esi), %mm2          # v02
	movq 16*6(%esi), %mm3          # v03

	movq %mm0, %mm4
	movq %mm1, %mm5
	
	paddw %mm3, %mm0               # v10: v00+v03
	psubw %mm3, %mm4	       # v13: v00-v03
	
	paddw %mm2, %mm1	       # v11: v01+v02
	psubw %mm2, %mm5	       # v12: v01-v02
	
	movq %mm0, %mm3

	paddw %mm4, %mm5               # v22: v12+v13	
	paddw %mm1, %mm0	       # v20: v10+v11
	psubw %mm1, %mm3	       # v21: v10-v11

	pmulhw WA1, %mm5               # v32~: WA1*v22
	movq  %mm4, %mm2	
	psllw  $16-NSHIFT, %mm5        # v32: compensate the coeffient scale

	movq  %mm0, 16*0(%esi)
	
	psubw %mm5, %mm2	       # v43: v13-v32	
	paddw %mm4, %mm5	       # v42: v32+v13

	movq  %mm2, 16*6(%esi)
	movq  %mm3, 16*4(%esi)
	movq  %mm5, 16*2(%esi)
	
	popl	 %edi
	popl	 %esi
	popl	 %ebp
	ret

.align 8	
.global dct_248_block_mmx_post_sum
dct_248_block_mmx_post_sum:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi

	movl    8(%ebp), %esi          # source

	movq	16*0(%esi), %mm0
	movq	16*1(%esi), %mm1	
	movq	16*2(%esi), %mm2
	movq	16*3(%esi), %mm3	
	movq	16*4(%esi), %mm4
	movq	16*5(%esi), %mm5	
	movq	16*6(%esi), %mm6	
	movq	16*7(%esi), %mm7
	
	psubw	%mm1, %mm0
	paddw	16*0(%esi), %mm1 
	movq	%mm0, 16*4(%esi)
	movq	%mm1, 16*0(%esi)

	movq	%mm4, %mm1
	movq	%mm2, %mm0
	paddw	%mm5, %mm1
	psubw	%mm5, %mm4		
	paddw	%mm3, %mm0
	psubw	%mm3, %mm2

	movq	%mm0, 16*1(%esi)
	movq	%mm2, 16*5(%esi)			
	movq	%mm1, 16*2(%esi)
	movq	%mm4, 16*6(%esi)			
		
	paddw	%mm6, %mm7
	psubw	16*7(%esi), %mm6

	movq	%mm7, 16*3(%esi)
	movq	%mm6, 16*7(%esi)		
				
	addl	$8, %esi

	movq	16*0(%esi), %mm0
	movq	16*1(%esi), %mm1	
	movq	16*2(%esi), %mm2
	movq	16*3(%esi), %mm3	
	movq	16*4(%esi), %mm4
	movq	16*5(%esi), %mm5	
	movq	16*6(%esi), %mm6	
	movq	16*7(%esi), %mm7
	
	psubw	%mm1, %mm0
	paddw	16*0(%esi), %mm1
	movq	%mm0, 16*4(%esi)
	movq	%mm1, 16*0(%esi)

	movq	%mm4, %mm1
	movq	%mm2, %mm0
	paddw	%mm5, %mm1
	psubw	%mm5, %mm4		
	paddw	%mm3, %mm0
	psubw	%mm3, %mm2

	movq	%mm0, 16*1(%esi)
	movq	%mm2, 16*5(%esi)			
	movq	%mm1, 16*2(%esi)
	movq	%mm4, 16*6(%esi)			
		
	paddw	%mm6, %mm7
	psubw	16*7(%esi), %mm6

	movq	%mm7, 16*3(%esi)
	movq	%mm6, 16*7(%esi)		
	
	popl	%esi
	popl	%ebp
	ret		
