/* 
 *  encode_x86.S
 *
 *     Copyright (C) Peter Schlaile - February 2001
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
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

#define	DV_WIDTH	720*2
#define DV_WIDTH_2	720

.data
ALLONE:	.word 1,1,1,1
	
.text

.global classify_mmx
classify_mmx:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi

	movl	12(%ebp), %esi
	movq	(%esi), %mm7            # amp_ofs
	movl	16(%ebp), %esi
	movq	(%esi), %mm6            # amp_cmp

	movl    8(%ebp), %esi          # source

	movq	%mm7, %mm5
	movq	%mm6, %mm4
	
	pxor	%mm3, %mm3
	pxor	%mm2, %mm2

	movq	0*8(%esi), %mm0
	movq	1*8(%esi), %mm1
	
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	2*8(%esi), %mm0
	movq	3*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	4*8(%esi), %mm0
	movq	5*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	6*8(%esi), %mm0
	movq	7*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	8*8(%esi), %mm0
	movq	9*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	10*8(%esi), %mm0
	movq	11*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	12*8(%esi), %mm0
	movq	13*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	14*8(%esi), %mm0
	movq	15*8(%esi), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 
							
	paddw	%mm2, %mm3
	packsswb %mm3, %mm3

	movd	%mm3, %eax

	pop	%esi
	pop	%ebp
	ret

.global copy_y_block_mmx
copy_y_block_mmx:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src
	
	movq	(%esi), %mm0
	movq	8(%esi), %mm1
	movq	%mm0, 0*8(%edi)
	movq	%mm1, 1*8(%edi) 
	movq	DV_WIDTH(%esi), %mm2
	movq	DV_WIDTH+8(%esi), %mm3
	movq	%mm2, 2*8(%edi)
	movq	%mm3, 3*8(%edi) 
	movq	DV_WIDTH*2(%esi), %mm4
	movq	DV_WIDTH*2+8(%esi), %mm5
	movq	%mm4, 4*8(%edi)
	movq	%mm5, 5*8(%edi) 
	movq	DV_WIDTH*3(%esi), %mm6
	movq	DV_WIDTH*3+8(%esi), %mm7
	movq	%mm6, 6*8(%edi)
	movq	%mm7, 7*8(%edi) 

	movq	DV_WIDTH*4(%esi), %mm0
	movq	DV_WIDTH*4+8(%esi), %mm1
	movq	%mm0, 8*8(%edi)
	movq	%mm1, 9*8(%edi) 
	movq	DV_WIDTH*5(%esi), %mm2
	movq	DV_WIDTH*5+8(%esi), %mm3
	movq	%mm2, 10*8(%edi)
	movq	%mm3, 11*8(%edi) 
	movq	DV_WIDTH*6(%esi), %mm4
	movq	DV_WIDTH*6+8(%esi), %mm5
	movq	%mm4, 12*8(%edi)
	movq	%mm5, 13*8(%edi) 
	movq	DV_WIDTH*7(%esi), %mm6
	movq	DV_WIDTH*7+8(%esi), %mm7
	movq	%mm6, 14*8(%edi)
	movq	%mm7, 15*8(%edi) 
	
	pop	%edi
	pop	%esi
	pop	%ebp
	ret
				
		
.global copy_pal_c_block_mmx
copy_pal_c_block_mmx:
				
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	pushl	%ebx
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src

	movl	$4, %ebx	

copy_pal_c_block_mmx_loop:	
	
	movq	          (%esi), %mm0
	movq	DV_WIDTH_2(%esi), %mm1
	movq               8(%esi), %mm2
	movq	DV_WIDTH_2+8(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH, %esi
	addl	$16, %edi

	movq	          (%esi), %mm0
	movq	DV_WIDTH_2(%esi), %mm1
	movq               8(%esi), %mm2
	movq	DV_WIDTH_2+8(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH, %esi
	addl	$16, %edi

	decl	%ebx
	jnz	copy_pal_c_block_mmx_loop
				
	pop	%ebx
	pop	%edi
	pop	%esi
	pop	%ebp
	ret

.global copy_ntsc_c_block_mmx
copy_ntsc_c_block_mmx:
				
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	pushl	%ebx
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src

	movl	$4, %ebx	

copy_ntsc_c_block_mmx_loop:	
	
	movq	(%esi), %mm0
	movq    8(%esi), %mm1
	movq	16(%esi), %mm2
	movq	24(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_2, %esi
	addl	$16, %edi

	movq	(%esi), %mm0
	movq    8(%esi), %mm1
	movq	16(%esi), %mm2
	movq	24(%esi), %mm3

	paddw	%mm0, %mm1
	paddw	%mm2, %mm3
	psraw	$1, %mm1
	psraw	$1, %mm3

	movq	%mm1, 0*8(%edi)
	movq	%mm3, 1*8(%edi)

	addl	$DV_WIDTH_2, %esi
	addl	$16, %edi

	decl	%ebx
	jnz	copy_ntsc_c_block_mmx_loop
				
	pop	%ebx
	pop	%edi
	pop	%esi
	pop	%ebp
	ret

/* FIXME: reorder_block_mmx isn't really _that_ faster than the C version... 
	 don't know why... */
	
.global reorder_block_mmx
reorder_block_mmx:
				
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	pushl	%ebx
	pushl	%ecx
	pushl	%edx

	movl	8(%ebp), %esi      # source
	movl	12(%ebp), %edi     # reorder_table

	xorl	%ebp, %ebp
	xorl	%eax, %eax
	xorl	%ebx, %ebx
	xorl	%ecx, %ecx
	xorl	%edx, %edx

	subl	$128, %esp
	
reorder_loop:	
	
	movw	 (%esi, %ebp), %ax
	movw	2(%esi, %ebp), %bx

	movw	 (%edi, %ebp), %cx
	movw	2(%edi, %ebp), %dx

	movw	 %ax, (%esp,%ecx)
	movw	 %bx, (%esp,%edx)

	movw	4(%esi, %ebp), %ax
	movw	6(%esi, %ebp), %bx

	movw	4(%edi, %ebp), %cx
	movw	6(%edi, %ebp), %dx

	movw	 %ax, (%esp,%ecx)
	movw	 %bx, (%esp,%edx)

	movw	 8(%esi, %ebp), %ax
	movw	10(%esi, %ebp), %bx

	movw	8(%edi, %ebp), %cx
	movw	10(%edi, %ebp), %dx

	movw	 %ax, (%esp,%ecx)
	movw	 %bx, (%esp,%edx)

	movw	12(%esi, %ebp), %ax
	movw	14(%esi, %ebp), %bx

	movw	12(%edi, %ebp), %cx
	movw	14(%edi, %ebp), %dx

	movw	 %ax, (%esp,%ecx)
	movw	 %bx, (%esp,%edx)

	addl	$16, %ebp
	
	cmpl	$128, %ebp
	jne	reorder_loop
	
	movq	(%esp)  , %mm0
	movq	8(%esp) , %mm1
	movq	16(%esp), %mm2
	movq	24(%esp), %mm3

	movq	%mm0, (%esi)
	movq	%mm1, 8(%esi)
	movq	%mm2, 16(%esi)
	movq	%mm3, 24(%esi)

	movq	32(%esp)   , %mm0
	movq	32+8(%esp) , %mm1
	movq	32+16(%esp), %mm2
	movq	32+24(%esp), %mm3

	movq	%mm0, 32(%esi)
	movq	%mm1, 32+8(%esi)
	movq	%mm2, 32+16(%esi)
	movq	%mm3, 32+24(%esi)

	movq	64(%esp)   , %mm0
	movq	64+8(%esp) , %mm1
	movq	64+16(%esp), %mm2
	movq	64+24(%esp), %mm3

	movq	%mm0, 64(%esi)
	movq	%mm1, 64+8(%esi)
	movq	%mm2, 64+16(%esi)
	movq	%mm3, 64+24(%esi)

	movq	96(%esp)   , %mm0
	movq	96+8(%esp) , %mm1
	movq	96+16(%esp), %mm2
	movq	96+24(%esp), %mm3

	addl	$128, %esp
	
	movq	%mm0, 96(%esi)
	movq	%mm1, 96+8(%esi)
	movq	%mm2, 96+16(%esi)
	movq	%mm3, 96+24(%esi)
	
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret

.global need_dct_248_mmx
need_dct_248_mmx:
	
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi

	movl	8(%ebp), %esi      # source
	movl	12(%ebp), %edi	   # thres_mask

	movq	(0*8+0)*2(%esi), %mm0
	movq	(0*8+4)*2(%esi), %mm1
	psubw	(1*8+0)*2(%esi), %mm0
	psubw	(1*8+4)*2(%esi), %mm1
	movq	%mm0, %mm2
	movq	%mm1, %mm3
	psraw	$15, %mm2
	psraw	$15, %mm3
	pxor	%mm2, %mm0
	pxor	%mm3, %mm1
	psubw	%mm2, %mm0
	psubw	%mm3, %mm1
	pand	(%edi), %mm0
	pand	(%edi), %mm1
	
	movq	(1*8+0)*2(%esi), %mm4
	movq	(1*8+4)*2(%esi), %mm5
	psubw	(2*8+0)*2(%esi), %mm4
	psubw	(2*8+4)*2(%esi), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	pand	(%edi), %mm4
	pand	(%edi), %mm5
	
	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(2*8+0)*2(%esi), %mm4
	movq	(2*8+4)*2(%esi), %mm5
	psubw	(3*8+0)*2(%esi), %mm4
	psubw	(3*8+4)*2(%esi), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	pand	(%edi), %mm4
	pand	(%edi), %mm5
	
	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(3*8+0)*2(%esi), %mm4
	movq	(3*8+4)*2(%esi), %mm5
	psubw	(4*8+0)*2(%esi), %mm4
	psubw	(4*8+4)*2(%esi), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	pand	(%edi), %mm4
	pand	(%edi), %mm5
	
	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(4*8+0)*2(%esi), %mm4
	movq	(4*8+4)*2(%esi), %mm5
	psubw	(5*8+0)*2(%esi), %mm4
	psubw	(5*8+4)*2(%esi), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	pand	(%edi), %mm4
	pand	(%edi), %mm5
	
	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(5*8+0)*2(%esi), %mm4
	movq	(5*8+4)*2(%esi), %mm5
	psubw	(6*8+0)*2(%esi), %mm4
	psubw	(6*8+4)*2(%esi), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	pand	(%edi), %mm4
	pand	(%edi), %mm5
	
	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(6*8+0)*2(%esi), %mm4
	movq	(6*8+4)*2(%esi), %mm5
	psubw	(7*8+0)*2(%esi), %mm4
	psubw	(7*8+4)*2(%esi), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	pand	(%edi), %mm4
	pand	(%edi), %mm5
	
	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	paddw	%mm1, %mm0
	
	pmaddwd	ALLONE, %mm0	
	movq	%mm0, %mm1
	psrlq	$32, %mm1
	paddd	%mm1, %mm0
	
	movd	%mm0, %eax
	
	popl	%edi
	popl	%esi
	popl	%ebp

	ret




