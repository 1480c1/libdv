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

#define	DV_WIDTH 720*2
	
.data
.align 8
amp_ofs:	.word	32768+255,32768+255,32768+255,32768+255
amp_cmp:	.word	32768+(255+255),32768+(255+255),32768+(255+255),32768+(255+255)
.text

.global amp_valid_mmx
amp_valid_mmx:

	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	
	movl    8(%ebp), %esi          # source

	movq	amp_ofs, %mm7
	movq	amp_cmp, %mm6

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
				
		
.global copy_c_block_mmx
copy_c_block_mmx:
				
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl	%edi
	pushl	%edx
	pushl	%ebx
	
	movl    8(%ebp), %edi          # dest
	movl    12(%ebp), %esi         # src
	movl	16(%ebp), %edx         # ofs
	shll	$2, %edx
	movl	%edx, %ebx
	shll	$1, %ebx

	movq	(%esi), %mm0
	movq	(%esi,%edx), %mm2
	movq	%mm0, %mm1
	movq	%mm2, %mm3
	addl	%ebx, %esi
	punpcklwd %mm0, %mm0
	punpckhwd %mm1, %mm1
	punpcklwd %mm2, %mm2
	punpckhwd %mm3, %mm3
	movq	%mm0, 0*8(%edi)
	movq	%mm1, 1*8(%edi)
	movq	%mm2, 2*8(%edi)
	movq	%mm3, 3*8(%edi)

	movq	(%esi), %mm0
	movq	(%esi,%edx), %mm2
	movq	%mm0, %mm1
	movq	%mm2, %mm3
	addl	%ebx, %esi
	punpcklwd %mm0, %mm0
	punpckhwd %mm1, %mm1
	punpcklwd %mm2, %mm2
	punpckhwd %mm3, %mm3
	movq	%mm0, 4*8(%edi)
	movq	%mm1, 5*8(%edi)
	movq	%mm2, 6*8(%edi)
	movq	%mm3, 7*8(%edi)

	movq	(%esi), %mm0
	movq	(%esi,%edx), %mm2
	movq	%mm0, %mm1
	movq	%mm2, %mm3
	addl	%ebx, %esi
	punpcklwd %mm0, %mm0
	punpckhwd %mm1, %mm1
	punpcklwd %mm2, %mm2
	punpckhwd %mm3, %mm3
	movq	%mm0, 8*8(%edi)
	movq	%mm1, 9*8(%edi)
	movq	%mm2, 10*8(%edi)
	movq	%mm3, 11*8(%edi)

	movq	(%esi), %mm0
	movq	(%esi,%edx), %mm2
	movq	%mm0, %mm1
	movq	%mm2, %mm3
	punpcklwd %mm0, %mm0
	punpckhwd %mm1, %mm1
	punpcklwd %mm2, %mm2
	punpckhwd %mm3, %mm3
	movq	%mm0, 12*8(%edi)
	movq	%mm1, 13*8(%edi)
	movq	%mm2, 14*8(%edi)
	movq	%mm3, 15*8(%edi)

	pop	%ebx
	pop	%edx
	pop	%edi
	pop	%esi
	pop	%ebp
	ret



