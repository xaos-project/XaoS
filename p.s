	.file	"p.c"
	.version	"01.01"
gcc2_compiled.:
.text
	.align 16
.globl test
	.type	 test,@function
test:
	pushl	%ebp
	movl	$blb, %eax
	movl	%esp, %ebp
	movl	%ebp, %esp
	popl	%ebp
	ret
.Lfe1:
	.size	 test,.Lfe1-test
	.ident	"GCC: (GNU) 2.96 19990902 (experimental)"
