	#include "asmoff.h"
.text
	.align 4
.globl dv_decode_vlc 
	.type	 dv_decode_vlc,@function
dv_decode_vlc:
	pushl %ebx

	/* Args are at 8(%esp). */
	movl  8(%esp),%eax		/* %eax is bits */
	movl  12(%esp),%ebx		/* %ebx is maxbits */

	movl  dv_vlc_class_index_mask(,%ebx,4),%edx
	andl  %eax,%edx
	movl  dv_vlc_class_index_rshift(,%ebx,4),%ecx
	sarl  %cl,%edx
	movl  dv_vlc_classes(,%ebx,4),%ecx
	movsbl  (%ecx,%edx,1),%edx	/* %edx is class */
			
	movl  dv_vlc_index_mask(,%edx,4),%ebx
	movl  dv_vlc_index_rshift(,%edx,4),%ecx
	andl  %eax,%ebx
	sarl  %cl,%ebx

	movl  dv_vlc_lookups(,%edx,4),%edx
	movl  (%edx,%ebx,4),%edx

	/* Now %edx holds result, like this:
	   bits 0-7   run
	   bits 8-15  len
	   bits 16-31 amp
	*/
	/* code needs to do this with result:
	   if ((amp > 0) &&
	     if (bits & sign_mask[result->len])
	         amp = -amp;
	   }
	*/

	/* Form a mask from (bits & sign_mask[result->len]) */
	movl  %edx,%ecx
	sarl  $8,%ecx
	andl  $0xff,%ecx
	movl  sign_mask(,%ecx,4),%ebx
	andl  %ebx,%eax
	negl  %eax
	sarl  $31,%eax

	movl  %edx,%ebx
	sarl  $31,%ebx
	xorl  $0xffffffff,%ebx
	andl  $0xffff0000,%ebx

	andl  %ebx,%eax

	/* Now %eax is 0xffff0000 if we want to negate %edx, zero otherwise */
	xorl  %eax,%edx
	subl  %eax,%edx

	/*
	if (maxbits < result->len)
	    *result = broken;
	Note that the 'broken' pattern is all ones (i.e. 0xffffffff)
	*/
	movl  12(%esp),%ebx		/* %ebx is maxbits */
	subl  %ecx,%ebx
	sbbl  %ebx,%ebx
	orl   %ebx,%edx

	movl  16(%esp),%eax
	movl  %edx,(%eax)
	
	popl  %ebx
	ret
	
.text
	.align 4
.globl __dv_decode_vlc 
	.type	 __dv_decode_vlc,@function
__dv_decode_vlc:
	pushl %ebx

	/* Args are at 8(%esp). */
	movl  8(%esp),%eax		/* %eax is bits */
	
	movl  %eax,%edx			/* %edx is class */
	andl  $0xfe00,%edx
	sarl  $9,%edx
	movsbl dv_vlc_class_lookup5(%edx),%edx
	
	movl  dv_vlc_index_mask(,%edx,4),%ebx
	movl  dv_vlc_index_rshift(,%edx,4),%ecx
	andl  %eax,%ebx
	sarl  %cl,%ebx

	movl  dv_vlc_lookups(,%edx,4),%edx
	movl  (%edx,%ebx,4),%edx

	/* Now %edx holds result, like this:
	   bits 0-7   run
	   bits 8-15  len
	   bits 16-31 amp
	*/
	/* code needs to do this with result:
	   if ((amp > 0) &&
	     if ((bits >> sign_rshift[result->len]) & 1)
	         amp = -amp;
	   }
	*/
	/* if (amp < 0) %edx is 0, else 0xffff0000. */
	movl  %edx,%ecx
	sarl  $8,%ecx
	andl  $0xff,%ecx
	movl  sign_mask(,%ecx,4),%ecx
	andl  %ecx,%eax
	negl  %eax
	sarl  $31,%eax

	movl  %edx,%ebx
	sarl  $31,%ebx
	xorl  $0xffffffff,%ebx
	andl  $0xffff0000,%ebx

	andl  %ebx,%eax
	
	xorl  %eax,%edx
	subl  %eax,%edx

	movl  12(%esp),%eax
	movl  %edx,(%eax)
	
	popl  %ebx
	ret

/*	
void dv_parse_ac_coeffs_pass0(bitstream_t *bs,
			      gint m,	 
			      gint b,
			      dv_macroblock_t *mb,
			      dv_block_t *bl)
*/
.text
	.align	4
.globl	dv_parse_ac_coeffs_pass0
dv_parse_ac_coeffs_pass0:
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%ebp

#define ARGn(N)  (20+(4*(N)))(%esp)

	/*
	eax	scratch
	ebx	bl->reorder
	ecx
	edx
	esi	bs->buf
	edi	bl->offset
	ebp	bl
	*/
	movl    ARGn(4),%ebp
	movl	ARGn(0),%esi
	movl	bitstream_t_buf(%esi),%esi
	movl	dv_block_t_offset(%ebp),%edi
	movl	dv_block_t_reorder(%ebp),%ebx

	/* I think it would be better to zero out the coeffs as we're
	copying them into the framebuffer.  But that optimization is
	for another day. */
	
	movq    dv_block_t_coeffs(%ebp),%mm1
	pxor    %mm0,%mm0
	pand    const_f_0_0_0,%mm1
	movq    %mm1,dv_block_t_coeffs(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 8)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 16)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 24)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 32)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 40)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 48)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 56)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 64)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 72)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 80)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 88)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 96)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 104)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 112)(%ebp)
	movq    %mm0,(dv_block_t_coeffs + 120)(%ebp)
	
readloop:
	movl	%edi,%ecx
	shrl	$3,%ecx
	movzbl  (%esi,%ecx,1),%eax
	movzbl  1(%esi,%ecx,1),%edx
	movzbl  2(%esi,%ecx,1),%ecx
	shll	$16,%eax
	shll	$8,%edx
	orl	%ecx,%eax
	orl	%edx,%eax
	movl	%edi,%edx
	andl	$7,%edx
	movl	$8,%ecx
	subl	%edx,%ecx
	shrl    %cl,%eax
	
	movl	dv_block_t_end(%ebp),%edx
	subl	%edi,%edx	/* edx is bits_left */
	cmpl	$16,%edx
	
	jl	slowpath
	/* fast path:	 use inlined version of __dv_decode_vlc */
	/* ---------------------- */
	pushl %ebx

	/* %eax is bits */
	
	movl  %eax,%edx			/* %edx is class */
	andl  $0xfe00,%edx
	sarl  $9,%edx
	movsbl dv_vlc_class_lookup5(%edx),%edx
	
	movl  dv_vlc_index_mask(,%edx,4),%ebx
	movl  dv_vlc_index_rshift(,%edx,4),%ecx
	andl  %eax,%ebx
	sarl  %cl,%ebx

	movl  dv_vlc_lookups(,%edx,4),%edx
	movl  (%edx,%ebx,4),%edx

	/* Now %edx holds result, like this:
	   bits 0-7   run
	   bits 8-15  len
	   bits 16-31 amp
	*/
	/* code needs to do this with result:
	   if ((amp > 0) &&
	     if ((bits >> sign_rshift[result->len]) & 1)
	         amp = -amp;
	   }
	*/
	/* if (amp < 0) %edx is 0, else 0xffff0000. */
	movl  %edx,%ecx
	sarl  $8,%ecx
	andl  $0xff,%ecx
	movl  sign_mask(,%ecx,4),%ecx
	andl  %ecx,%eax
	negl  %eax
	sarl  $31,%eax

	movl  %edx,%ebx
	sarl  $31,%ebx
	xorl  $0xffffffff,%ebx
	andl  $0xffff0000,%ebx

	andl  %ebx,%eax
	
	xorl  %eax,%edx
	subl  %eax,%edx

	popl  %ebx
	/* ---------------------- */
	
	jmp	done_decode
slowpath:
	/* slow path:	 use dv_decode_vlc */;
	pushl	$vlc		/* last parameter is &vlc */
	pushl	%edx		/* bits_left */
	pushl	%eax		/* bits */
	call	dv_decode_vlc
	addl	$12,%esp
done_decode:
	test	$0x80,%edx	/* If (vlc.run < 0) break */
	jne	escape
	
	/* bl->offset += vlc.len */
	movl	%edx,%eax
	shrl	$8,%eax
	andl	$255,%eax
	addl	%eax,%edi
	
	/* bl->reorder += vlc.run */
	movl	%edx,%eax
	andl	$255,%eax
	addl	%eax,%ebx
	
	/* SET_COEFF(bl->coeffs, bl->reorder, vlc.amp); */
	movzbl	(%ebx),%eax
	incl	%ebx

	shrl	$16,%edx
	movw	%dx,(dv_block_t_coeffs)(%ebp,%eax,1)
	
	jmp	readloop
	
escape:
	/* if (vlc.amp == 0) */
	test	$0xffff0000,%edx
	jne	ampnonzero
	/* bl->reorder = bl->reorder_sentinel; */
	movl	dv_block_t_reorder_sentinel(%ebp),%ebx
	addl	$4,%edi
	movl	$1,dv_block_t_eob(%ebp)
	movl    ARGn(3),%edx
	incl	dv_macroblock_t_eob_count(%edx)
	jmp	alldone
ampnonzero:
	andl	$0x0000ff00,%edx
	cmpl	$0x0000fe00,%edx	/* VLC_ERROR */
	jne	alldone
	movl    ARGn(3),%edx
	movl	$1,dv_macroblock_t_vlc_error(%edx); 
alldone:
	movl	%ebx,dv_block_t_reorder(%ebp)
	movl	%edi,dv_block_t_offset(%ebp)
	
	popl	%ebp
	popl	%esi
	popl	%edi
	popl	%ebx

	emms
	
	ret

show16:
	movl	%edi,%ecx
	movl	%edi,%edx
	shrl	$3,%ecx
	andl	$7,%edx
	movzbl  (%esi,%ecx,1),%eax
	movzbl  1(%esi,%ecx,1),%ebx
	movzbl  2(%esi,%ecx,1),%ecx
	shll	$16,%eax
	shll	$8,%ebx
	orl	%ecx,%eax
	orl	%ebx,%eax
	movl	$8,%ecx
	subl	%edx,%ecx
	shrl    %cl,%eax
	ret

.data
vlc:	
	.long	0
	
	.align 16
const_f_0_0_0:
	.short	0xffff,0,0,0
