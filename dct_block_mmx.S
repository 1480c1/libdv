# Copyright (C) 1995-99 -  Intel Corporation all rights reserved
# esi - input and output data pointer
# the input data each 16 bit element in the 8x8 matrix is left aligned:
# the output data is tranposed and each 16 bit element in the 8x8 matrix is left aligned:
# e.g. in 11...1110000 format
# israelh. 11/11/97 removed emms. moved to stub
# MMX implementation. Using MMX transpose 


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

.global dct_block_mmx
dct_block_mmx:

pushl   %ebp
movl    %esp, %ebp
pushl   %esi
pushl   %edi

movl    8(%ebp), %esi          # source
movl   12(%ebp), %edi          # destination   
#slot

# column 0
movq 16*0(%esi), %mm0          # v0
# psllw $PRESHIFT, %mm0
movq 16*7(%esi), %mm1          # v7
movq  %mm0, %mm2               # duplicate v0 
# psllw $PRESHIFT, %mm1
paddw %mm1, %mm0               # v00: v0+v7  
psubw %mm1, %mm2               # v07: v0-v7  

movq 16*1(%esi), %mm1          # v1
# psllw $PRESHIFT, %mm1
movq 16*6(%esi), %mm3          # v6
movq  %mm1, %mm4               # duplicate v1 
# psllw $PRESHIFT, %mm3
paddw %mm3, %mm1               # v01: v1+v6  
psubw %mm3, %mm4               # v06: v1-v6  

movq 16*2(%esi), %mm3          # v2
# psllw $PRESHIFT, %mm3
movq 16*5(%esi), %mm5          # v5
movq  %mm3, %mm6               # duplicate v2 
# psllw $PRESHIFT, %mm5
paddw %mm5, %mm3               # v02: v2+v5  
psubw %mm5, %mm6               # v05: v2-v5  

movq 16*3(%esi), %mm5          # v3
# psllw $PRESHIFT, %mm5
movq 16*4(%esi), %mm7          # v4
# psllw $PRESHIFT, %mm7
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
# psllw  $16-WA5_SHIFT, %mm0   # va1: compensate the coeefient scale 	
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

addl $8, %esi # point to the next 4 columns. it can be done by adding 8 to immediates but this is nicer

movq (%esi), %mm0                  # v0
# psllw $PRESHIFT, %mm0
movq 16*7(%esi), %mm1              # v7
movq  %mm0, %mm2               # duplicate v0 
# psllw $PRESHIFT, %mm1
paddw %mm1, %mm0               # v00: v0+v7  
psubw %mm1, %mm2               # v07: v0-v7  

movq 16(%esi), %mm1                # v1
# psllw $PRESHIFT, %mm1
movq 16*6(%esi), %mm3              # v6
movq  %mm1, %mm4               # duplicate v1 
# psllw $PRESHIFT, %mm3
paddw %mm3, %mm1               # v01: v1+v6  
psubw %mm3, %mm4               # v06: v1-v6  

movq 16*2(%esi), %mm3         # v2
# psllw $PRESHIFT, %mm3
movq 16*5(%esi), %mm5              # v5
movq  %mm3, %mm6               # duplicate v2 
# psllw $PRESHIFT, %mm5
paddw %mm5, %mm3               # v02: v2+v5  
psubw %mm5, %mm6               # v05: v2-v5  

movq 16*3(%esi), %mm5              # v3
# psllw $PRESHIFT, %mm5
movq 16*4(%esi), %mm7              # v4
# psllw $PRESHIFT, %mm7
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
#psllw  mm4,16-NSHIFT         ; v35: compensate the coeefient scale

movq   %mm4, %mm6            # duplicate v35
paddw  %mm2, %mm4            # v45: v07+v35
psubw  %mm6, %mm2            # v47: v07-v35

paddw  %mm5, %mm3            # v22: v12+v13

pmulhw WA1, %mm3           # v32~: WA3*v15
psllw  $16-NSHIFT, %mm3       # v32: compensate the coeefient scale
#psllw  mm3,16-NSHIFT         ; v32: compensate the coeefient scale
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
# psllw  $16-WA5_SHIFT, %mm0
psllw  $16-WA4_SHIFT, %mm5 # v36: compensate the coeefient scale note that WA4 is shifted 1 bit less tan the others

#psllw  mm5,16-WA4_SHIFT ; v36: compensate the coeefient scale note that WA4 is shifted 1 bit less tan the others
#psllw  $16-NSHIFT, %mm7       # v34: compensate the coeefient scale
#psllw  mm0,16-WA5_SHIFT

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

#if 0	
popl %edi
popl    %esi
popl %ebp

ret     $0
#endif	

subl $8, %esi # point back to the first 4 columns/rows. 
pushl %esi
pushl %edi

jmp      transpose

cont: 


popl    %edi
popl %esi

# first 4 rows 
movq (%esi), %mm0                  # v0
#pinsrw mm0,[esi+16],1             ; (0,1)->1
#pinsrw mm0,[esi+16*2],2                   ; (0,2)->2
#pinsrw mm0,[esi+16*3],3                   ; (0,3)->3
movq 16*7(%esi), %mm1              # v7
#pinsrw mm1,[esi+7*2],0            ; (7,0)->0
#pinsrw mm1,[esi+16+7*2],1                 ; (7,1)->1
#pinsrw mm1,[esi+16*2+7*2],2               ; (7,1)->2
#pinsrw mm1,[esi+16*3+7*2],3               ; (7,1)->3
movq  %mm0, %mm2               # duplicate v0 
paddw %mm1, %mm0               # v00: v0+v7  
psubw %mm1, %mm2               # v07: v0-v7  

movq 16(%esi), %mm1                # v1
#pinsrw mm1,[esi+1*2],0            ; (7,0)->0
#pinsrw mm1,[esi+16+1*2],1                 ; (7,1)->1
#pinsrw mm1,[esi+16*2+1*2],2               ; (7,1)->2
#pinsrw mm1,[esi+16*3+1*2],3               ; (7,1)->3
movq 16*6(%esi), %mm3              # v6
#pinsrw mm3,[esi+6*2],0            ; (7,0)->0
#pinsrw mm3,[esi+16+6*2],1                 ; (7,1)->1
#pinsrw mm3,[esi+16*2+6*2],2               ; (7,1)->2
#pinsrw mm3,[esi+16*3+6*2],3               ; (7,1)->3
movq  %mm1, %mm4               # duplicate v1 
paddw %mm3, %mm1               # v01: v1+v6  
psubw %mm3, %mm4               # v06: v1-v6  

movq 16*2(%esi), %mm3         # v2
#pinsrw mm3,[esi+2*2],0            ; (7,0)->0
#pinsrw mm3,[esi+16+2*2],1                 ; (7,1)->1
#pinsrw mm3,[esi+16*2+2*2],2               ; (7,1)->2
#pinsrw mm3,[esi+16*3+2*2],3               ; (7,1)->3
movq 16*5(%esi), %mm5              # v5
#pinsrw mm5,[esi+5*2],0            ; (7,0)->0
#pinsrw mm5,[esi+16+5*2],1                 ; (7,1)->1
#pinsrw mm5,[esi+16*2+5*2],2               ; (7,1)->2
#pinsrw mm5,[esi+16*3+5*2],3               ; (7,1)->3
movq  %mm3, %mm6               # duplicate v2 
paddw %mm5, %mm3               # v02: v2+v5  
psubw %mm5, %mm6               # v05: v2-v5  

movq 16*3(%esi), %mm5              # v3
#pinsrw mm5,[esi+3*2],0            ; (7,0)->0
#pinsrw mm5,[esi+16+3*2],1                 ; (7,1)->1
#pinsrw mm5,[esi+16*2+3*2],2               ; (7,1)->2
#pinsrw mm5,[esi+16*3+3*2],3               ; (7,1)->3
movq 16*4(%esi), %mm7              # v4
#pinsrw mm7,[esi+4*2],0            ; (7,0)->0
#pinsrw mm7,[esi+16+4*2],1                 ; (7,1)->1
#pinsrw mm7,[esi+16*2+4*2],2               ; (7,1)->2
#pinsrw mm7,[esi+16*3+4*2],3               ; (7,1)->3
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

paddw %mm1, %mm0       # v10+v11
psubw %mm1, %mm6       # v10-v11

movq  %mm0, (%edi)         # out0: v10+v11 
movq  %mm6, 16*4(%edi)          # out4: v10-v11 

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

movq  %mm5, 16*2(%edi)          # out2: v13+v32 
movq  %mm6, 16*6(%edi)          # out6: v13-v32 

paddw  scratch4, %mm7                   # v14n: v04+v05
movq   %mm0, %mm5           # duplicate v16

psubw  %mm7, %mm0                       # va1: v16-v14n
pmulhw WA4, %mm5                        # v36~~: v16*WA4
pmulhw WA2, %mm7                # v34~~: v14n*WA2
pmulhw WA5, %mm0                # va0~:  va1*WA5
psllw  $16-WA4_SHIFT, %mm5    # v36: compensate the coeefient scale. Note that WA$ is scaled one bit less
psllw  $16-NSHIFT, %mm7       # v34: compensate the coeefient scale
#psllw  mm0,16-WA5_SHIFT

psubw  %mm0, %mm5   # v36~: v36~~-va0~
psubw  %mm0, %mm7   # v34~: v34~~-va0~

movq   %mm4, %mm0    # duplicate v45
paddw  %mm5, %mm4    # v45+v36
psubw  %mm5, %mm0    # v45-v36

movq  %mm4, 16*1(%edi)          # out1: v45+v36 
movq  %mm0, 16*7(%edi)          # out7: v45-v36 

movq   %mm2, %mm5    # duplicate v47
paddw  %mm7, %mm2    # v47+v34
psubw  %mm7, %mm5    # v47-v34

movq  %mm2, 16*5(%edi)          # out5: v47+v34 
movq  %mm5, 16*3(%edi)          # out3: v47-v34 


# second 4 rows 
addl $8, %esi #16*4  ; source - point to the second 4 rows. 
addl $8, %edi             # destination - point to the second 4 columns (transposed)

movq (%esi), %mm0                  # v0
#pinsrw mm0,[esi+16],1             ; (0,1)->1
#pinsrw mm0,[esi+16*2],2                   ; (0,2)->2
#pinsrw mm0,[esi+16*3],3                   ; (0,3)->3
movq 16*7(%esi), %mm1              # v7
#pinsrw mm1,[esi+7*2],0            ; (7,0)->0
#pinsrw mm1,[esi+16+7*2],1                 ; (7,1)->1
#pinsrw mm1,[esi+16*2+7*2],2               ; (7,1)->2
#pinsrw mm1,[esi+16*3+7*2],3               ; (7,1)->3
movq  %mm0, %mm2               # duplicate v0 
paddw %mm1, %mm0               # v00: v0+v7  
psubw %mm1, %mm2               # v07: v0-v7  

movq 16(%esi), %mm1                # v1
#pinsrw mm1,[esi+1*2],0            ; (7,0)->0
#pinsrw mm1,[esi+16+1*2],1                 ; (7,1)->1
#pinsrw mm1,[esi+16*2+1*2],2               ; (7,1)->2
#pinsrw mm1,[esi+16*3+1*2],3               ; (7,1)->3
movq 16*6(%esi), %mm3              # v6
#pinsrw mm3,[esi+6*2],0            ; (7,0)->0
#pinsrw mm3,[esi+16+6*2],1                 ; (7,1)->1
#pinsrw mm3,[esi+16*2+6*2],2               ; (7,1)->2
#pinsrw mm3,[esi+16*3+6*2],3               ; (7,1)->3
movq  %mm1, %mm4               # duplicate v1 
paddw %mm3, %mm1               # v01: v1+v6  
psubw %mm3, %mm4               # v06: v1-v6  

movq 16*2(%esi), %mm3         # v2
#pinsrw mm3,[esi+2*2],0            ; (7,0)->0
#pinsrw mm3,[esi+16+2*2],1                 ; (7,1)->1
#pinsrw mm3,[esi+16*2+2*2],2               ; (7,1)->2
#pinsrw mm3,[esi+16*3+2*2],3               ; (7,1)->3
movq 16*5(%esi), %mm5              # v5
#pinsrw mm5,[esi+5*2],0            ; (7,0)->0
#pinsrw mm5,[esi+16+5*2],1                 ; (7,1)->1
#pinsrw mm5,[esi+16*2+5*2],2               ; (7,1)->2
#pinsrw mm5,[esi+16*3+5*2],3               ; (7,1)->3
movq  %mm3, %mm6               # duplicate v2 
paddw %mm5, %mm3               # v02: v2+v5  
psubw %mm5, %mm6               # v05: v2-v5  

movq 16*3(%esi), %mm5              # v3
#pinsrw mm5,[esi+3*2],0            ; (7,0)->0
#pinsrw mm5,[esi+16+3*2],1                 ; (7,1)->1
#pinsrw mm5,[esi+16*2+3*2],2               ; (7,1)->2
#pinsrw mm5,[esi+16*3+3*2],3               ; (7,1)->3
movq 16*4(%esi), %mm7              # v4
#pinsrw mm7,[esi+4*2],0            ; (7,0)->0
#pinsrw mm7,[esi+16+4*2],1                 ; (7,1)->1
#pinsrw mm7,[esi+16*2+4*2],2               ; (7,1)->2
#pinsrw mm7,[esi+16*3+4*2],3               ; (7,1)->3
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

paddw %mm1, %mm0       # v10+v11
psubw %mm1, %mm6       # v10-v11

movq  %mm0, (%edi)         # out0: v10+v11 
movq  %mm6, 16*4(%edi)          # out4: v10-v11 

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

movq  %mm5, 16*2(%edi)          # out2: v13+v32 
movq  %mm6, 16*6(%edi)          # out6: v13-v32 

paddw  scratch4, %mm7                   # v14n: v04+v05
movq   %mm0, %mm5           # duplicate v16

psubw  %mm7, %mm0                       # va1: v16-v14n
pmulhw WA4, %mm5                        # v36~~: v16*WA4
pmulhw WA2, %mm7                # v34~~: v14n*WA2
pmulhw WA5, %mm0                # va0~:  va1*WA5
psllw  $16-WA4_SHIFT, %mm5  # v36: compensate the coeefient scale. Note that WA$ is scaled one bit less
psllw  $16-NSHIFT, %mm7       # v34: compensate the coeefient scale
#psllw  mm0,16-WA5_SHIFT

psubw  %mm0, %mm5   # v36~: v36~~-va0~
psubw  %mm0, %mm7   # v34~: v34~~-va0~

movq   %mm4, %mm0    # duplicate v45
paddw  %mm5, %mm4    # v45+v36
psubw  %mm5, %mm0    # v45-v36

movq  %mm4, 16*1(%edi)          # out1: v45+v36 
movq  %mm0, 16*7(%edi)          # out7: v45-v36 

movq   %mm2, %mm5    # duplicate v47
paddw  %mm7, %mm2    # v47+v34
psubw  %mm7, %mm5    # v47-v34

movq  %mm2, 16*5(%edi)          # out5: v47+v34 
movq  %mm5, 16*3(%edi)          # out3: v47-v34 

popl %edi
popl    %esi
popl %ebp

ret     $0

transpose: 


                pushl %ebx
                pushl %ecx
                pushl %edx

        movl            $8, %ebx   # ebx is x_size
        movl            %ebx, %ecx
        movl            %esi, %edi      # pointer to the matrix

        sall            $2, %ecx
        movl            %ebx, %eax
        addl            %ebx, %ecx
        subl            $4, %eax        # eax is the inner loop variable

        addl            %ebx, %ecx      # ecx is 6*row size
        movl            %eax, %edx      # edx is the outer loop variable

do_4x4_block_where_x_equals_y: 

        movq    (%esi), %mm0            # m03:m02|m01:m00 - first line

        movq    (%esi,%ebx,4), %mm2     # m23:m22|m21:m20 - third line
        movq    %mm0, %mm6              # copy first line

        punpcklwd (%esi,%ebx,2), %mm0
        # m11:m01|m10:m00 - interleave first and second lines
        movq    %mm2, %mm7              # copy third line

        punpcklwd (%esi,%ecx,), %mm2
        # m31:m21|m30:m20 - interleave third and fourth lines
        movq    %mm0, %mm4              # copy first intermediate result

        movq    (%esi,%ebx,2), %mm1     # m13:m12|m11:m10 - second line
        punpckldq %mm2, %mm0
        # m30:m20|m10:m00 - interleave to produce result 1

        movq    (%esi,%ecx,), %mm3      # m33:m32|m31:m30 - fourth line
        punpckhdq %mm2, %mm4
        # m31:m21|m11:m01 - interleave to produce result 2

        movq    %mm0, (%esi)            # write result 1
        punpckhwd %mm1, %mm6
        # m13:m03|m12:m02 - interleave first and second lines

        movq    %mm4, (%esi,%ebx,2)     # write result 2
        punpckhwd %mm3, %mm7
        # m33:m23|m32:m22 - interleave third and fourth lines

        movq    %mm6, %mm5              # copy first intermediate result
        punpckldq %mm7, %mm6
        # m32:m22|m12:m02 - interleave to produce result 3

        leal    (%edi,%ebx,8), %edi
        # reload edi to point to a 4x4 set 4 rows down
        punpckhdq %mm7, %mm5
        # m33:m23|m13:m03 - interleave to produce result 4

        movq    %mm6, (%esi,%ebx,4)     # write result 3

        movq    %mm5, (%esi,%ecx,)      # write result 4

        cmpl    $0, %edx
        # check to see if the number of rows left is zero
        je      all_done_ready_to_exit
        #last time through you are done and ready to exit

do_4x4_blocks_x_and_y_not_equal: 

# transpose the two mirror image 4x4 sets so that the writes 
# can be done without overwriting unused data 

        movq    8(%esi), %mm0           # m03:m02|m01:m00 - first line

        movq    8(%esi,%ebx,4), %mm2     # m23:m22|m21:m20 - third line
        movq    %mm0, %mm6              # copy first line

        punpcklwd 8(%esi,%ebx,2), %mm0
        # m11:m01|m10:m00 - interleave first and second lines
        movq    %mm2, %mm7              # copy third line

        punpcklwd 8(%esi,%ecx,), %mm2
        # m31:m21|m30:m20 - interleave third and fourth lines
        movq    %mm0, %mm4              # copy first intermediate result
# all references for second 4 x 4 block are referred by "n" instead of "m"
        movq    (%edi), %mm1            # n03:n02|n01:n00 - first line 
        punpckldq %mm2, %mm0
        # m30:m20|m10:m00 - interleave to produce first result

        movq    (%edi,%ebx,4), %mm3     # n23:n22|n21:n20 - third line
        punpckhdq %mm2, %mm4
        # m31:m21|m11:m01 - interleave to produce second result

        punpckhwd 8(%esi,%ebx,2), %mm6
        # m13:m03|m12:m02 - interleave first and second lines
        movq    %mm1, %mm2              # copy first line

        punpckhwd 8(%esi,%ecx,), %mm7
        # m33:m23|m32:m22 - interleave third and fourth lines
        movq    %mm6, %mm5              # copy first intermediate result

        movq    %mm0, (%edi)            # write result 1
        punpckhdq %mm7, %mm5
        # m33:m23|m13:m03 - produce third result

        punpcklwd (%edi,%ebx,2), %mm1
        # n11:n01|n10:n00 - interleave first and second lines
        movq    %mm3, %mm0              # copy third line

        punpckhwd (%edi,%ebx,2), %mm2
        # n13:n03|n12:n02 - interleave first and second lines

        movq    %mm4, (%edi,%ebx,2)     # write result 2 out
        punpckldq %mm7, %mm6
        # m32:m22|m12:m02 - produce fourth result

        punpcklwd (%edi,%ecx,), %mm3
        # n31:n21|n30:n20 - interleave third and fourth lines
        movq    %mm1, %mm4              # copy first intermediate result

        movq    %mm6, (%edi,%ebx,4)     # write result 3 out
        punpckldq %mm3, %mm1
        # n30:n20|n10:n00 - produce first result

        punpckhwd (%edi,%ecx,), %mm0
        # n33:n23|n32:n22 - interleave third and fourth lines
        movq    %mm2, %mm6              # copy second intermediate result

        movq    %mm5, (%edi,%ecx,)      # write result 4 out
        punpckhdq %mm3, %mm4
        # n31:n21|n11:n01- produce second result

        movq    %mm1, 8(%esi)
        # write result 5 out - (first result for other 4 x 4 block)
        punpckldq %mm0, %mm2
        # n32:n22|n12:n02- produce third result

        movq    %mm4, 8(%esi,%ebx,2)     # write result 6 out
        punpckhdq %mm0, %mm6
        # n33:n23|n13:n03 - produce fourth result

        movq    %mm2, 8(%esi,%ebx,4)     # write result 7 out

        movq    %mm6, 8(%esi,%ecx,)     # write result 8 out

        addl    $8, %esi
        # increment esi to point to next 4 x 4 block in same row
        leal    (%edi,%ebx,8), %edi
        # increment edi to point to next 4 x 4 block below current one

        subl    $4, %eax                # decrement inner loop variable
        jnz     do_4x4_blocks_x_and_y_not_equal
        # edi points to start of the second row in block we just finished

        sall    %edx
        leal    8(%esi,%ebx,8), %esi     # reload edi to point four rows down

        subl    %edx, %esi
        # subtract the number of bytes in last row 
        # now we point to spot where row = col 
        subl    $8, %edx                # sub 4 from row number

        sarl    %edx
        movl    %esi, %edi

        movl    %edx, %eax
        # reset x_size to outer loop variable to start new row
        jmp     do_4x4_block_where_x_equals_y

all_done_ready_to_exit: 
                popl %edx
                popl %ecx
                popl %ebx


                jmp cont


#_dct8x8aan_mmx ENDP




