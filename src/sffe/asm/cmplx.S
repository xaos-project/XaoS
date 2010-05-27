	.globl _sffecabs,_sffecarg,_sffecargs,_sffecargc
	.globl _sffecinv
	.globl _sffecexp,_sffecln,_sffeclog2,_sffeclog
	.globl _sffecsin,_sffeccos,_sffecsincos,_sffectan,_sffeccot
	.globl _sffecsinh,_sffeccosh,_sffectanh,_sffeccoth
	.globl _sffeccpow, _sffecpowd, _sffecpowi, _sffecpowc
	.globl _sffecsqrt, _sffecrtni

	.text
	
_sffecexpfnc:
	fldl2e 
	fmulp  %st,%st(1)
	fld    %st(0)
	frndint 
	fsubr  %st,%st(1)
	fld1   
	fscale 
	fstp   %st(1)
	fxch   %st(1)
	f2xm1  
	fld1   
	faddp  %st,%st(1)
	fmulp  %st,%st(1)
	ret    

_sffecabs:
	push   %ebp
	mov    %esp,%ebp
	fldl   0x8(%ebp)
	fmull  0x8(%ebp)
	fldl   0x10(%ebp)
	fmull  0x10(%ebp)
	faddp  %st,%st(1)
	fsqrt  
	leave  
	ret    

_sffecarg:
	push   %ebp
	mov    %esp,%ebp
	fldl   0x8(%ebp)
	fldl   0x10(%ebp)
	fpatan 
	fwait
	leave  
	ret    

_sffecargs:
	push   %ebp
	mov    %esp,%ebp
	fldl   0x10(%ebp)
	fldl   0x8(%ebp)
	fmull  0x8(%ebp)
	fldl   0x10(%ebp)
	fmull  0x10(%ebp)
	faddp  %st,%st(1)
	fsqrt  
	fdivrp %st,%st(1)
	fwait
	leave  
	ret    

_sffecargc:
	push   %ebp
	mov    %esp,%ebp
	fldl   0x8(%ebp)
	fldl   0x8(%ebp)
	fmull  0x8(%ebp)
	fldl   0x10(%ebp)
	fmull  0x10(%ebp)
	faddp  %st,%st(1)
	fsqrt  
	fdivrp %st,%st(1)
	fwait
	leave  
	ret    

_sffecinv:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%edx
	fldl   0xc(%ebp)
	fldl   0x14(%ebp)
	fld    %st(0)
	fmul   %st,%st(0)
	fld    %st(2)
	fmul   %st,%st(0)
	faddp  %st,%st(1)
	fdivr  %st,%st(1)
	fdivr  %st,%st(2)
	fstp   %st(0)
	fchs   
	fstpl  0x8(%edx)
	fstpl  (%edx)
	fwait
	leave  
	ret    

_sffecexp:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	fldl   0xc(%ebp)
	call   _sffecexpfnc
	fldl   0x14(%ebp)
	fsincos 
	fld    %st(2)
	fmulp  %st,%st(1)
	fstpl  (%eax)
	fmulp  %st,%st(1)
	fstpl  0x8(%eax)
	fwait
	leave  
	ret    

_sffecln:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	fldl   0x14(%ebp)
	fldl   0xc(%ebp)
	fpatan 
	fldl   0xc(%ebp)
	fmull  0xc(%ebp)
	fldl   0x14(%ebp)
	fmull  0x14(%ebp)
	faddp  %st,%st(1)
	fsqrt  
	fld1   
	fxch   %st(1)
	fyl2x  
	fldl2e 
	fdivrp %st,%st(1)
	fstpl  (%eax)
	fstpl  0x8(%eax)
	fwait
	leave  
	ret    

_sffeclog2:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	fld1   
	fldl   0xc(%ebp)
	fmull  0xc(%ebp)
	fldl   0x14(%ebp)
	fmull  0x14(%ebp)
	faddp  %st,%st(1)
	fsqrt  
	fyl2x  
	fldl   0x14(%ebp)
	fldl   0xc(%ebp)
	fpatan 
	fldln2 
	fdivrp %st,%st(1)
	fstpl  0x8(%eax)
	fstpl  (%eax)
	fwait
	leave  
	ret    

_sffeclog:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	fld1   
	fild   0x1c(%ebp)
	fyl2x  
	fldl2e 
	fdivrp %st,%st(1)
	fldl   0xc(%ebp)
	fmull  0xc(%ebp)
	fldl   0x14(%ebp)
	fmull  0x14(%ebp)
	faddp  %st,%st(1)
	fsqrt  
	fld1   
	fxch   %st(1)
	fyl2x  
	fldl2e 
	fdivrp %st,%st(1)
	fdiv   %st(1),%st
	fldl   0x14(%ebp)
	fldl   0xc(%ebp)
	fpatan 
	fdiv   %st(2),%st
	fstpl  0x8(%eax)
	fstpl  (%eax)
	fstp   %st(0)
	fwait
	leave  
	ret    

_sffecsin:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	fld1   
	fld1   
	faddp  %st,%st(1)
	fldl   0x14(%ebp)
	call   _sffecexpfnc
	fld1   
	fdiv   %st(1),%st
	fld    %st(1)
	fld    %st(1)
	faddp  %st,%st(1)
	fdiv   %st(3),%st
	fxch   %st(2)
	fxch   %st(1)
	fsubrp %st,%st(1)
	fdiv   %st(2),%st
	fldl   0xc(%ebp)
	fsincos 
	fxch   %st(3)
	fmulp  %st,%st(1)
	fstpl  (%eax)
	fmulp  %st,%st(1)
	fstpl  0x8(%eax)
	fstp   %st(0)
	fwait
	leave  
	ret    

_sffeccos:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	fld1   
	fld1   
	faddp  %st,%st(1)
	fldl   0x14(%ebp)
	call   _sffecexpfnc
	fld1   
	fdiv   %st(1),%st
	fld    %st(1)
	fld    %st(1)
	faddp  %st,%st(1)
	fdiv   %st(3),%st
	fxch   %st(2)
	fxch   %st(1)
	fsubrp %st,%st(1)
	fdiv   %st(2),%st
	fldl   0xc(%ebp)
	fsincos 
	fxch   %st(2)
	fmulp  %st,%st(1)
	fchs   
	fstpl  0x8(%eax)
	fmulp  %st,%st(1)
	fstpl  (%eax)
	fstp   %st(0)
	fwait
	leave  
	ret    

_sffectan:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	fld1   
	fld1   
	faddp  %st,%st(1)
	fldl   0x14(%ebp)
	faddl  0x14(%ebp)
	call   _sffecexpfnc
	fld1   
	fdiv   %st(1),%st
	fld    %st(1)
	fld    %st(1)
	faddp  %st,%st(1)
	fdiv   %st(3),%st
	fxch   %st(2)
	fxch   %st(1)
	fsubrp %st,%st(1)
	fdivp  %st,%st(2)
	fldl   0xc(%ebp)
	faddl  0xc(%ebp)
	fsincos 
	faddp  %st,%st(2)
	fdiv   %st(1),%st
	fstpl  (%eax)
	fdivrp %st,%st(1)
	fstpl  0x8(%eax)
	fwait
	leave  
	ret    

_sffeccot:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	fld1   
	fld1   
	faddp  %st,%st(1)
	fldl   0x14(%ebp)
	faddl  0x14(%ebp)
	call   _sffecexpfnc
	fld1   
	fdiv   %st(1),%st
	fld    %st(1)
	fld    %st(1)
	fsubrp %st,%st(1)
	fdiv   %st(3),%st
	fxch   %st(2)
	fxch   %st(1)
	faddp  %st,%st(1)
	fdivp  %st,%st(2)
	fldl   0xc(%ebp)
	fld    %st(0)
	faddp  %st,%st(1)
	fsincos 
	fsubrp %st,%st(3)
	fdiv   %st(2),%st
	fstpl  (%eax)
	fdiv   %st(1),%st
	fchs   
	fstpl  0x8(%eax)
	fstp   %st(0)
	fwait
	leave  
	ret    

_sffecsinh:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	fld1   
	fld1   
	faddp  %st,%st(1)
	fldl   0xc(%ebp)
	call   _sffecexpfnc
	fld1   
	fdiv   %st(1),%st
	fld    %st(1)
	fld    %st(1)
	faddp  %st,%st(1)
	fdiv   %st(3),%st
	fxch   %st(2)
	fxch   %st(1)
	fsubrp %st,%st(1)
	fdivp  %st,%st(2)
	fldl   0x14(%ebp)
	fsincos 
	fxch   %st(2)
	fmulp  %st,%st(1)
	fstpl  0x8(%eax)
	fmulp  %st,%st(1)
	fstpl  (%eax)
	fwait
	leave  
	ret    

_sffeccosh:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	fldl   0xc(%ebp)
	call   _sffecexpfnc
	fld    %st(0)
	fld1   
	fxch   %st(1)
	fdivrp %st,%st(1)
	fld    %st(1)
	fld    %st(1)
	faddp  %st,%st(1)
	fld1   
	fld1   
	faddp  %st,%st(1)
	fdivrp %st,%st(1)
	fxch   %st(2)
	fxch   %st(1)
	fsubrp %st,%st(1)
	fld1   
	fld1   
	faddp  %st,%st(1)
	fdivrp %st,%st(1)
	fldl   0x14(%ebp)
	fsincos 
	fxch   %st(2)
	fmulp  %st,%st(1)
	fstpl  0x8(%eax)
	fmulp  %st,%st(1)
	fstpl  (%eax)
	fwait
	leave  
	ret    

_sffectanh:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	fld1   
	fld1   
	faddp  %st,%st(1)
	fldl   0xc(%ebp)
	faddl  0xc(%ebp)
	call   _sffecexpfnc
	fld1   
	fdiv   %st(1),%st
	fld    %st(1)
	fld    %st(1)
	fsubrp %st,%st(1)
	fdiv   %st(3),%st
	fxch   %st(2)
	fxch   %st(1)
	faddp  %st,%st(1)
	fdivp  %st,%st(2)
	fldl   0x14(%ebp)
	faddl  0x14(%ebp)
	fsincos 
	faddp  %st,%st(3)
	fdiv   %st(2),%st
	fstpl  0x8(%eax)
	fxch   %st(1)
	fdivrp %st,%st(1)
	fstpl  (%eax)
	fwait
	leave  
	ret    

_sffeccoth:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	fld1   
	fld1   
	faddp  %st,%st(1)
	fldl   0xc(%ebp)
	faddl  0xc(%ebp)
	call   _sffecexpfnc
	fld1   
	fdiv   %st(1),%st
	fld    %st(1)
	fld    %st(1)
	fsubrp %st,%st(1)
	fdiv   %st(3),%st
	fxch   %st(2)
	fxch   %st(1)
	faddp  %st,%st(1)
	fdivp  %st,%st(2)
	fldl   0x14(%ebp)
	faddl  0x14(%ebp)
	fsincos 
	fsubrp %st,%st(3)
	fdiv   %st(2),%st
	fchs   
	fstpl  0x8(%eax)
	fxch   %st(1)
	fdivrp %st,%st(1)
	fstpl  (%eax)
	fwait
	leave  
	ret    

_sffeccpow:
	push   %ebp
	mov    %esp,%ebp
	fld1   
	fldl   0xc(%ebp)
	fmull  0xc(%ebp)
	fldl   0x14(%ebp)
	fmull  0x14(%ebp)
	faddp  %st,%st(1)
	fsqrt  
	ftst   
	fstsw  %ax
	test   $0x40,%ah
	jne    MZ
	mov    0x8(%ebp),%eax
	fyl2x  
	fldl2e 
	fdivrp %st,%st(1)
	fldl   0x14(%ebp)
	fldl   0xc(%ebp)
	fpatan 
	fld    %st(1)
	fmull  0x1c(%ebp)
	fld    %st(1)
	fmull  0x24(%ebp)
	fsubrp %st,%st(1)
	fxch   %st(2)
	fmull  0x24(%ebp)
	fxch   %st(1)
	fmull  0x1c(%ebp)
	faddp  %st,%st(1)
	fxch   %st(1)
	call   _sffecexpfnc
	fxch   %st(1)
	fsincos 
	fld    %st(2)
	fmulp  %st,%st(1)
	fstpl  (%eax)
	fmulp  %st,%st(1)
	fstpl  0x8(%eax)
	jmp    END

MZ:
	mov    0x8(%ebp),%eax
	fstpl  0x8(%eax)
	fldz   
	fstpl  (%eax)
	fstp   %st(0)

END:
	fwait
	leave  
	ret    

_sffecpowd:
	push   %ebp
	mov    %esp,%ebp
	fldl   0xc(%ebp)
	fmull  0xc(%ebp)
	fldl   0x14(%ebp)
	fmull  0x14(%ebp)
	faddp  %st,%st(1)
	fsqrt  
	ftst   
	fstsw  %ax
	test   $0x40,%ah
	jne    MZ2
	mov    0x8(%ebp),%eax
	fld1   
	fldl   0x14(%ebp)
	fldl   0xc(%ebp)
	fpatan 
	fmull  0x1c(%ebp)
	fxch   %st(2)
	fyl2x  
	fldl2e 
	fdivrp %st,%st(1)
	fmull  0x1c(%ebp)
	call   _sffecexpfnc
	fxch   %st(1)
	fsincos 
	fld    %st(2)
	fmulp  %st,%st(1)
	fstpl  (%eax)
	fmulp  %st,%st(1)
	fstpl  0x8(%eax)
	jmp    END2

MZ2:
	mov    0x8(%ebp),%eax
	fstpl  0x8(%eax)
	fldz   
	fstpl  (%eax)

END2:
	fwait
	leave  
	ret    

_sffecpowi:
	push   %ebp
	mov    %esp,%ebp
	fildl  0x1c(%ebp)
	fldl   0xc(%ebp)
	fmull  0xc(%ebp)
	fldl   0x14(%ebp)
	fmull  0x14(%ebp)
	faddp  %st,%st(1)
	fsqrt  
	ftst   
	fstsw  %ax
	test   $0x40,%ah
	jne    MZ5
	mov    0x8(%ebp),%eax
	fld1   
	fxch   %st(1)
	fyl2x  
	fmulp  %st,%st(1)
	fld    %st(0)
	frndint 
	fsubr  %st,%st(1)
	fld1   
	fscale 
	fstp   %st(1)
	fxch   %st(1)
	f2xm1  
	fld1   
	faddp  %st,%st(1)
	fmulp  %st,%st(1)
	fldl   0x14(%ebp)
	fldl   0xc(%ebp)
	fpatan 
	fildl  0x1c(%ebp)
	fmulp  %st,%st(1)
	fsincos 
	fld    %st(2)
	fmulp  %st,%st(1)
	fstpl  (%eax)
	fmulp  %st,%st(1)
	fstpl  0x8(%eax)
	jmp    END53

MZ5:
	mov    0x8(%ebp),%eax
	fstpl  0x8(%eax)
	fldz   
	fstpl  (%eax)
	fstp   %st(0)

END53:
	fwait
	leave  
	ret    

_sffecpowc:
	push   %ebp
	mov    %esp,%ebp
	fldl   0xc(%ebp)
	ftst   
	fstsw  %ax
	test   $0x40,%ah
	jne    MZ3
	mov    0x8(%ebp),%eax
	fldl   0x14(%ebp)
	fxch   %st(1)
	fabs   
	fld1   
	fxch   %st(1)
	fyl2x  
	fmulp  %st,%st(1)
	fld    %st(0)
	frndint 
	fsubr  %st,%st(1)
	fld1   
	fscale 
	fstp   %st(1)
	fxch   %st(1)
	f2xm1  
	fld1   
	faddp  %st,%st(1)
	fmulp  %st,%st(1)
	fldl   0xc(%ebp)
	fabs   
	fld1   
	fxch   %st(1)
	fyl2x  
	fldl2e 
	fdivrp %st,%st(1)
	fldl   0x1c(%ebp)
	fmulp  %st,%st(1)
	fsincos 
	fld    %st(2)
	fmulp  %st,%st(1)
	fstpl  (%eax)
	fmulp  %st,%st(1)
	fstpl  0x8(%eax)
	jmp    END3

MZ3:
	mov    0x8(%ebp),%eax
	fstpl  0x8(%eax)
	fldz   
	fstpl  (%eax)

END3:
	fwait
	leave  
	ret    

_sffecsqrt:
	push   %ebp
	mov    %esp,%ebp
	push   %ebx
	mov    0x8(%ebp),%ebx
	fldl   0xc(%ebp)
	fmull  0xc(%ebp)
	fldl   0x14(%ebp)
	fmull  0x14(%ebp)
	faddp  %st,%st(1)
	fsqrt  
	fld    %st(0)
	faddl  0xc(%ebp)
	fld1   
	fld1   
	faddp  %st,%st(1)
	fdivrp %st,%st(1)
	fsqrt  
	fstpl  (%ebx)
	fwait
	fsubl  0xc(%ebp)
	fld1   
	fld1   
	faddp  %st,%st(1)
	fdivrp %st,%st(1)
	fsqrt  
	push   %eax
	fldl   0x14(%ebp)
	ftst   
	fstp   %st(0)
	fstsw  %ax
	test   $0x1,%ah
	je     IMAGPOS
	fchs   

IMAGPOS:
	pop    %eax
	fwait
	fstpl  0x8(%ebx)
	pop    %ebx
	leave  
	ret    

_sffecrtni:
	push   %ebp
	mov    %esp,%ebp
	fldl   0xc(%ebp)
	fmull  0xc(%ebp)
	fldl   0x14(%ebp)
	fmull  0x14(%ebp)
	faddp  %st,%st(1)
	fsqrt  
	ftst   
	fstsw  %ax
	test   $0x40,%ah
	jne    MZ4
	mov    0x8(%ebp),%eax
	fld1   
	fild   0x1c(%ebp)
	fdivrp %st,%st(1)
	fxch   %st(1)
	fyl2x  
	fld    %st(0)
	frndint 
	fsubr  %st,%st(1)
	fld1   
	fscale 
	fstp   %st(1)
	fxch   %st(1)
	f2xm1  
	fld1   
	faddp  %st,%st(1)
	fmulp  %st,%st(1)
	fldl   0x14(%ebp)
	fldl   0xc(%ebp)
	fpatan 
	fldpi  
	fldpi  
	faddp  %st,%st(1)
	fimul  0x20(%ebp)
	faddp  %st,%st(1)
	fidiv  0x1c(%ebp)
	fsincos 
	fld    %st(2)
	fmulp  %st,%st(1)
	fstpl  (%eax)
	fmulp  %st,%st(1)
	fstpl  0x8(%eax)
	jmp    END4

MZ4:
	fldz   
	fstpl  (%eax)
	fstpl  0x8(%eax)

END4:
	leave  
	ret    

_sffecfunc:
	push   %ebp
	mov    %esp,%ebp
	mov    0x8(%ebp),%eax
	leave  
	ret    
