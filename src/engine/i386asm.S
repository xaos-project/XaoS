/* Image fields */
#define oldlines  SYMBOL(cimage)+8
#define currlines SYMBOL(cimage)+12
#define width	  SYMBOL(cimage)+20
#define height	  SYMBOL(cimage)+24
#define bytesperpixel SYMBOL(cimage)+32

/* SYMBOL(czoomc) fields */
#define reallocy  SYMBOL(czoomc)+24
#define reallocysize 6 /*64*/
#define dirty	44
#define plus	28

#define FRAMESIZE 0
#define r1	28+FRAMESIZE(%esp)
#define r2	32+FRAMESIZE(%esp)
#define printf(what) pusha ; pushl what ; pushl $.STR ; call SYMBOL(printf) ; addl $8, %esp ; popa
#ifndef YES_UNDERSCORES
#define SYMBOL(x) x
#else
#define SYMBOL(x) _##x
#endif
.section .rodata
.STR:
	.string "%i\n"
.text
	.align 4
.globl SYMBOL(moveoldpoints)
SYMBOL(moveoldpoints):
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	cld

	movl	r1,	%ebp
	.p2align 7,,4
.radky:
	/* reallocy->dirty */
	movl	%ebp, %ebx
	sall	$reallocysize, %ebx
	addl	reallocy, %ebx
	cmpl	$0,dirty(%ebx)
	jne	.hotovo 

	/* reallocy->plus -> esi */
	movl	plus(%ebx), %esi

	/* ebx = size */
	movl	SYMBOL(tmpdata), %ebx

	pushl	%ebp

	/* Vline -> esi */
	movl	oldlines, %edi
	movl	(%edi,%esi,4), %esi

	/* edi -> currlines[ebp] */
	movl	currlines, %edi
	movl	(%edi,%ebp,4), %edi

	cmpl	$4, bytesperpixel
	je .smallloop4
	cmpl	$10, SYMBOL(avgsize)
	jl	.smallloop	

	/* Vline -> ebp */
	movl	%esi, %ebp
	/* size -> esi */
	movl	(%ebx), %eax

	.p2align 7,,4
.loop:
	movl	%ebp, %esi	/* U */
	nop			/* V */
	addl	4(%ebx), %esi	/* U */
	addl	8(%ebx), %edi	/* V */

	cmpl	$7, %eax	/* U */
	jl	.small		/* V */

	movl	%edi, %ecx	/* V */
	negl	%ecx		/* Non pairable */
	andl	$3, %ecx	/* U */

	subl	%ecx, %eax	/* U */
	rep ; movsb

	movl	%eax, %ecx	/* U */
	shrl	$2, %ecx	/* U */
	rep ; movsl

	andl	$3, %eax
.small:
	movl	%eax, %ecx	/* V */

	rep ; movsb
	movl	12(%ebx), %eax	/* U */
	addl	$12, %ebx	/* V */
	cmpl	$0, %eax	/* U */
	jne .loop		/* V */
	
	popl	%ebp
	jmp .hotovo
.smallloop:
	movl	%esi, %ebp
	movl	(%ebx), %ecx
	cmpl	$2, bytesperpixel
	je .smallloop2
	.p2align 7,,4
.sloop:
	movl	%ebp, %esi	/* U */
	nop			/* V */
	addl	4(%ebx), %esi	/* U */
	addl	8(%ebx), %edi	/* V */
	/* Finish addl, mask prefix */
	rep ; movsb
	movl	12(%ebx), %ecx	/* U */
	addl	$12, %ebx	/* V */
	cmpl	$0, %ecx	/* U */
	jne .sloop		/* V */
	popl	%ebp
	jmp .hotovo
.smallloop2:
	.p2align 7,,4
.sloop2:
	movl	%ebp, %esi	/* U */
	sarl	$1, %ecx	/* V */
	addl	4(%ebx), %esi	/* U */
	addl	8(%ebx), %edi	/* V */

	/* Finish addl, mask prefix */
	rep ; movsw
	movl	12(%ebx), %ecx	/* U */
	addl	$12, %ebx	/* V */
	cmpl	$0, %ecx	/* U */
	jne .sloop2		/* V */
	popl	%ebp
	jmp .hotovo
.smallloop4:
	movl	%esi, %ebp
	movl	(%ebx), %ecx
	.p2align 7,,4
.sloop4:
	movl	%ebp, %esi	/* U */
	sarl	$2, %ecx	/* V */
	addl	4(%ebx), %esi	/* U */
	addl	8(%ebx), %edi	/* V */

	/* Finish addl, mask prefix */
	rep ; movsl
	movl	12(%ebx), %ecx	/* U */
	addl	$12, %ebx	/* V */
	cmpl	$0, %ecx	/* U */
	jne .sloop2		/* V */
	popl	%ebp
.hotovo:
	incl	%ebp
	cmpl	r2, %ebp
	jl	.radky

	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	.align 4

#define line 12(%esp)
.globl SYMBOL(fillline_8)
SYMBOL(fillline_8):
	pushl	%edi
	pushl	%ebx
	cld

	/* ebx <- vbuff */
	movl	currlines, %ebx
	movl	line, %eax
	movl	(%ebx,%eax,4), %ebx

	movl	SYMBOL(tmpdata), %edx
	movl	8(%edx), %ecx
	cmpl	$0, %ecx
	je	.done

	/* eax <- from */
	movl	(%edx), %eax

	.p2align 7,,4
.setloop:
	movl	4(%edx), %edi		/* U */
	movb	(%eax, %ebx), %al	/* V */
	addl	%ebx, %edi		/* U */
	addl	$16, %edx		/* V */
	rep ; stosb
/*
	movb	%al, %ah
	shrl	$1, %ecx
	rep ; stosw
	jnc 1f
	movb %al, (%edi)
1:*/
	movl	8(%edx), %ecx		/* U */
	movl	(%edx), %eax 		/* V */
	cmp	$0, %ecx		/* U */
	jne	.setloop		/* V */
.done:
	popl	%ebx
	popl	%edi
	ret
.globl SYMBOL(fillline_16)
SYMBOL(fillline_16):
	pushl	%edi
	pushl	%ebx
	cld

	/* ebx <- vbuff */
	movl	currlines, %ebx
	movl	line, %eax
	movl	(%ebx,%eax,4), %ebx

	movl	SYMBOL(tmpdata), %edx
	movl	8(%edx), %ecx
	cmpl	$0, %ecx
	je	.done16

	/* eax <- from */
	movl	(%edx), %eax

	.p2align 7,,4
.setloop16:
	movw	(%eax, %ebx), %ax	/* prefix + U */
	movl	4(%edx), %edi		/* V */
	addl	%ebx, %edi		/* U */
	addl	$16, %edx		/* V */
	rep ; stosw
	movl	8(%edx), %ecx		/* U */
	movl	(%edx), %eax 		/* V */
	cmp	$0, %ecx		/* U */
	jne	.setloop16		/* V */
.done16:
	popl	%ebx
	popl	%edi
	ret
.globl SYMBOL(fillline_32)
SYMBOL(fillline_32):
	pushl	%edi
	pushl	%ebx
	cld

	/* ebx <- vbuff */
	movl	currlines, %ebx
	movl	line, %eax
	movl	(%ebx,%eax,4), %ebx

	movl	SYMBOL(tmpdata), %edx
	movl	8(%edx), %ecx
	cmpl	$0, %ecx
	je	.done32

	/* eax <- from */
	movl	(%edx), %eax

	.p2align 7,,4
.setloop32:
	movl	4(%edx), %edi		/* U */
	movl	(%eax, %ebx), %eax	/* V */
	addl	%ebx, %edi		/* U */
	addl	$16, %edx		/* V */
	rep ; stosl
	movl	8(%edx), %ecx		/* U */
	movl	(%edx), %eax 		/* V */
	cmp	$0, %ecx		/* U */
	jne	.setloop32		/* V */
.done32:
	popl	%ebx
	popl	%edi
	ret
#undef line
#define line 16(%esp)
.globl SYMBOL(fillline_24)
SYMBOL(fillline_24):
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	/* ebx <- vbuff */
	movl	currlines, %ebx
	movl	line, %eax
	movl	(%ebx,%eax,4), %ebx

	movl	SYMBOL(tmpdata), %edx
	movl	8(%edx), %ecx
	cmpl	$0, %ecx
	je	.done24

	/* eax <- from */
	movl	(%edx), %eax

	.p2align 7,,4
.setloop24:
	movw	(%eax, %ebx), %si	/* U */
	movl	4(%edx), %edi		/* V */
	movb	2(%eax, %ebx), %ah	/* U */
	addl	%ebx, %edi		/* V */
	addl	$16, %edx		/* U */
.insetloop:
	movw	%si, (%edi)		/* U */
	movb	%ah, 2(%edi)		/* V */
	addl	$3, %edi		/* U */
	dec   %ecx			/* V */
	jnz   .insetloop		/* U */

	movl	8(%edx), %ecx		/* U */
	movl	(%edx), %eax 		/* V */
	cmp	$0, %ecx		/* U */
	jne	.setloop24		/* V */
.done24:
	popl	%ebx
	popl	%esi
	popl	%edi
	ret
