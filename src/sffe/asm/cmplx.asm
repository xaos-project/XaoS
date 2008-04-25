	;      COMPLEX NUMBER ARITHMETICS
	;           Mateusz Malczak
	;             NASM version
	;        http://www.malczak.info
	; 	       SFFE libs
	; http://segfaultlabs.com/projects/sffe
	global _sffecabs,_sffecarg,_sffecargs,_sffecargc
	global _sffecinv
	global _sffecexp,_sffecln,_sffeclog2,_sffeclog
	global _sffecsin,_sffeccos,_sffecsincos,_sffectan,_sffeccot
	global _sffecsinh,_sffeccosh,_sffectanh,_sffeccoth
	global _sffeccpow, _sffecpowd, _sffecpowi, _sffecpowc
	global _sffecsqrt, _sffecrtni

	%ifndef DELPHI
	;	section .text use32 class=CODE ;class=CODE - needed for Borlands Compiler

	;	section CODE use32
	; ".text" is more portable than "CODE use32":
	section .text
	%endif

	;; internal use
%ifdef DELPHI
[section expfunc_text use32]
%endif
_sffecexpfnc: 			; exponent of real argument
    fldl2e
	fmulp   st1
	fld     st0
    frndint
    fsub    to st1
    fld1
    fscale
    fstp    st1
    fxch    st1
    f2xm1
    fld1
    faddp   st1
	fmulp   st1	
	ret

	;; globals
%ifdef DELPHI
[section cabs_text use32]
%endif
_sffecabs:
	push 	ebp
	mov 	ebp, esp
	fld     qword [ebp+08h]
	fmul    qword [ebp+08h]
	fld     qword [ebp+10h]
	fmul    qword [ebp+10h]
	faddp	st1
	fsqrt
	leave
	ret
	
%ifdef DELPHI
[section carg_text use32]
%endif
_sffecarg:
	push 	ebp
	mov		ebp, esp
	fld     qword [ebp+08h]   
	fld     qword [ebp+10h]   
	fpatan
	wait
	leave
	ret

%ifdef DELPHI
[section cargs_text use32]
%endif
_sffecargs:
	push	ebp
	mov 	ebp, esp
	fld     qword [ebp+10h]
	fld     qword [ebp+08h]
	fmul    qword [ebp+08h]
	fld     qword [ebp+10h]
	fmul    qword [ebp+10h]
	faddp	st1
	fsqrt
	fdivp	st1
	wait
	leave
	ret

%ifdef DELPHI
[section cargc_text use32]
%endif
_sffecargc:
	push	ebp
	mov 	ebp, esp
	fld     qword [ebp+08h]   
	fld     qword [ebp+08h]   
	fmul    qword [ebp+08h]   
	fld     qword [ebp+10h]   
	fmul    qword [ebp+10h]   
	faddp	st1
	fsqrt
	fdivp	st1
	wait
	leave
	ret

%ifdef DELPHI
[section cinv_text use32]
%endif
_sffecinv:				
	push	ebp
	mov		ebp, esp		
	mov     edx, [ebp+08h] 
	fld     qword [ebp+0ch] 
	fld     qword [ebp+14h] 
	fld     st0
	fmul    to st0
	fld     st2
	fmul    to st0	
	faddp	st1		
	fdiv    to st1
	fdiv    to st2
	fstp    st0
	fchs			
	fstp    qword [edx+08h]
	fstp    qword [edx] 
	wait
	leave
	ret		

%ifdef DELPHI
[section cexp_text use32]
%endif
_sffecexp:	
	push	ebp
	mov		ebp,esp
	mov     eax, [ebp+08h]
	fld     qword [ebp+0ch]	
	call	_sffecexpfnc
	fld     qword [ebp+14h]
	fsincos
	fld     st2
	fmulp   st1
	fstp    qword [eax]
	fmulp   st1
	fstp    qword [eax+08h]
	wait	
	leave
	ret

%ifdef DELPHI
[section cln_text use32]
%endif
_sffecln:	
	push	ebp
	mov		ebp,esp
	mov     eax, [ebp+08h]
	;theta
	fld     qword [ebp+14h] 
	fld     qword [ebp+0ch] 
	fpatan                      
	;z module
	fld     qword [ebp+0ch] 
	fmul    qword [ebp+0ch] 
	fld     qword [ebp+14h] 
	fmul    qword [ebp+14h] 
	faddp	st1                        
	fsqrt                       
	;ln||z||
	fld1                      
	fxch    st1              
	fyl2x                     
	fldl2e                    
	fdivp   st1                      
	fstp    qword [eax]    
	fstp    qword [eax+08h]
	wait
	leave
	ret
	
%ifdef DELPHI
[section clog2_text use32]
%endif
_sffeclog2:	
	push	ebp
	mov		ebp,esp
	mov     eax, [ebp+08h]
	fld1
	;z module
	fld     qword [ebp+0ch]
	fmul    qword [ebp+0ch]
	fld     qword [ebp+14h]
	fmul    qword [ebp+14h]
	faddp	st1
	fsqrt      
	fyl2x      
	;theta
	fld     qword [ebp+14h]   
	fld     qword [ebp+0ch]   
	fpatan                    
	fldln2                    
	fdivp	st1
	fstp    qword [eax+08h]  
	fstp    qword [eax]      
	wait
	leave
	ret

%ifdef DELPHI
[section clog_text use32]
%endif
_sffeclog:	
	push	ebp
	mov		ebp,esp
	mov     eax, [ebp+08h]
	;ln(base) 
	fld1           
	fild    word [ebp+1ch]
	fyl2x      
	fldl2e     
	fdivp	st1
	;z module
	fld     qword [ebp+0ch]
	fmul    qword [ebp+0ch]
	fld     qword [ebp+14h]
	fmul    qword [ebp+14h]
	faddp	st1        
	fsqrt       
	;ln||z||
	fld1        
	fxch    st1 
	fyl2x      
	fldl2e     
	fdivp	st1
	fdiv	st1
	;theta
	fld     qword [ebp+14h]
	fld     qword [ebp+0ch]
	fpatan                 
	fdiv	st2
	fstp    qword [eax+08h]
	fstp    qword [eax]
	fstp    st0
	wait	
	leave
	ret

%ifdef DELPHI
[section csin_text use32]
%endif
_sffecsin:
	push	ebp
	mov		ebp, esp
	mov     eax, [ebp+08h]
	fld1
	fld1
	faddp	st1
	fld     qword [ebp+14h]
	call	_sffecexpfnc
	fld1
	fdiv    st1
	fld     st1;-
	fld     st1;- optimize
	faddp	st1;-
	fdiv    st3
	fxch    st2
	fxch    st1
	fsubp   st1
	fdiv    st2
	fld     qword [ebp+0ch]
	fsincos
	fxch    st3
	fmulp   st1
	fstp    qword [eax]
	fmulp   st1
	fstp    qword [eax+08h]
	fstp    st0
	fwait
	leave
	ret

%ifdef DELPHI
[section ccos_text use32]
%endif
_sffeccos:	
	push	ebp
	mov		ebp,esp
	mov     eax, [ebp+08h]
	fld1
	fld1
	faddp	st1
	fld     qword [ebp+14h] 
	call	_sffecexpfnc
	fld1                    
	fdiv    st1     
	fld     st1
	fld     st1
	faddp   st1
	fdiv    st3
	fxch    st2
	fxch    st1
	fsubp   st1
	fdiv    st2
	fld     qword [ebp+0ch]
	fsincos                       
	fxch    st2
	fmulp   st1
	fchs
	fstp    qword [eax+08h]
	fmulp	st1
	fstp    qword [eax]    
	fstp    st0
	fwait	
	leave
	ret
	
%ifdef DELPHI
[section ctan_text use32]
%endif
_sffectan:	
	push	ebp
	mov		ebp,esp
	mov     eax, [ebp+08h]
	;sinh(2b) cosh(2b) 
	fld1
	fld1
	faddp	st1                   
	fld     qword [ebp+14h]
	fadd    qword [ebp+14h]
	call	_sffecexpfnc
	fld1       
	fdiv    st1
	fld     st1
	fld     st1
	faddp	st1
	fdiv    st3
	fxch    st2
	fxch    st1
	fsubp	st1
	fdivrp  st2 ;fdivrp    st(2),st(0)
	;sin(2b) cos(2b) 
	fld     qword [ebp+0ch]
	fadd    qword [ebp+0ch]
	fsincos            
	faddp   st2
	;check if zero
	fdiv    st1
	fstp    qword [eax]
	fdivp   st1
	fstp    qword [eax+08h]
	fwait	
	leave
	ret
	
%ifdef DELPHI
[section ccot_text use32]
%endif
_sffeccot:	
	push	ebp
	mov		ebp,esp
	mov     eax, [ebp+08h]
	;sinh(2b) cosh(2b) 
	fld1
	fld1
	faddp	st1
	fld     qword [ebp+14h]
	fadd    qword [ebp+14h]
	call	_sffecexpfnc
	fld1                   
	fdiv    st1    
	fld     st1
	fld     st1
	fsubp   st1
	fdiv    st3
	fxch    st2
	fxch    st1
	faddp	st1
	fdivrp  st2	;fdivrp st(2),st(0)
	;sin(2b) cos(2b) 
	fld     qword [ebp+0ch]
	fld     st0
	faddp	st1
	fsincos   
	fsubp   st3
	;check if zero
	fdiv   st2
	fstp   qword [eax]
	fdiv   st1
	fchs
	fstp   qword [eax+08h]
	fstp   st0
	fwait
	leave
	ret
	
%ifdef DELPHI
[section csinh_text use32]
%endif
_sffecsinh:	
	push	ebp
	mov		ebp,esp
	mov     eax, [ebp+08h]
	fld1
	fld1
	faddp	st1
	fld     qword [ebp+0Ch]
	call	_sffecexpfnc
	fld1       
	fdiv    st1
	fld     st1
	fld     st1
	faddp	st1
	fdiv    st3
	fxch    st2
	fxch    st1
	fsubp	st1
	fdivrp  st2 ;jak w tan i cot
	fld     qword [ebp+14h]
	fsincos               
	fxch    st2
	fmulp	st1
	fstp    qword [eax+08h]
	fmulp	st1
	fstp    qword [eax]
	fwait	
	leave
	ret

%ifdef DELPHI
[section ccosh_text use32]
%endif
_sffeccosh:	
	push	ebp
	mov		ebp,esp
	mov     eax, [ebp+08h]
	fld     qword [ebp+0ch]
	call	_sffecexpfnc
	fld     st0
	fld1
	fxch    st1
	fdivp	st1
	fld     st1
	fld     st1
	faddp	st1
	fld1
	fld1
	faddp	st1
	fdivp	st1
	fxch    st2
	fxch    st1
	fsubp	st1
	fld1
	fld1
	faddp	st1
	fdivp	st1
	fld     qword [ebp+14h]
	fsincos                       
	fxch    st2
	fmulp	st1
	fstp    qword [eax+08h]
	fmulp	st1
	fstp    qword [eax]
	fwait
	leave
	ret
	
%ifdef DELPHI
[section ctanh_text use32]
%endif
_sffectanh:	
	push	ebp
	mov		ebp,esp
	mov     eax, [ebp+08h]
	;sinh(2a) cosh(2a)
	fld1
	fld1
	faddp	st1
	fld     qword [ebp+0ch]
	fadd    qword [ebp+0ch]
	call	_sffecexpfnc
	fld1
	fdiv    st1
	fld     st1
	fld     st1
	fsubp	st1
	fdiv    st3
	fxch    st2
	fxch    st1
	faddp	st1
	fdivrp  st2 ;jak wczesniej
	;sin(2b) cos(2b)
	fld     qword [ebp+14h]
	fadd    qword [ebp+14h]
	fsincos               
	faddp   st3
	;check if zero
	fdiv    st2
	fstp    qword [eax+08h]
	fxch    st1
	fdivp	st1
	fstp    qword [eax]
	fwait	
	leave
	ret

%ifdef DELPHI
[section ccoth_text use32]
%endif
_sffeccoth:	
	push	ebp
	mov		ebp,esp
	mov     eax, [ebp+08h]
	;sinh(2b) cosh(2b) 
	fld1
	fld1
	faddp	st1
	fld     qword [ebp+0ch]
	fadd    qword [ebp+0ch]
	call	_sffecexpfnc
	fld1                   
	fdiv    st1
	fld     st1
	fld     st1
	fsubp	st1
	fdiv    st3
	fxch    st2
	fxch    st1
	faddp	st1
	fdivrp  st2 ;jak wczesniej
	;sin(2b) cos(2b)
	fld     qword [ebp+14h]
	fadd    qword [ebp+14h]
	fsincos 
	fsubp   st3
	;check if zero
	fdiv    st2
	fchs
	fstp   	qword [eax+08h]
	fxch    st1
	fdivp	st1
	fstp   	qword [eax]
	fwait
	leave
	ret

;***************** COMPLEX TO COMPLEX POWER			
%ifdef DELPHI
[section ccpow_text use32]
%endif
_sffeccpow:	
;TODO: wyeliminowac xch po wystepujace po obliczeniu theta
	push	ebp
	mov		ebp,esp
	fld1
	;z module
	fld     qword [ebp+0ch]   
	fmul    qword [ebp+0ch]   
	fld     qword [ebp+14h]   
	fmul    qword [ebp+14h]   
	faddp	st1
	fsqrt
	ftst
	fstsw   ax
	test    ah, 1000000b
	jnz     MZ
	mov     eax, [ebp+08h]
	;ln||z||*/
	fyl2x
	fldl2e
	fdivp	st1
	;theta*/
	fld     qword [ebp+14h]
	fld     qword [ebp+0ch]
	fpatan
	
	fld 	st1 ;ln||z||
	fmul 	qword [ebp+1ch]
	fld		st1 ;theta
	fmul	qword [ebp+24h]
	fsubp 	st1 ; st0-st1 
	fxch	st2 ; w st2 mam teraz a = z2.re * lnz - z2.im * theta w st0 jest ln||z||
	fmul	qword [ebp+24h]
	fxch 	st1
	fmul	qword [ebp+1ch]
	faddp	st1
	fxch	st1
	
	call	_sffecexpfnc
	fxch 	st1
	fsincos                       
	fld     st2                 
	fmulp	st1
	fstp    qword [eax]
	fmulp	st1
	fstp    qword [eax+08h]  
	jmp     END
MZ:
	mov     eax, [ebp+08h]
	fstp    qword [eax+08h]   
	fldz
	fstp    qword [eax]
	fstp	st0
END:
s	fwait
	leave
	ret

;***************** COMPLEX TO REAL POWER
%ifdef DELPHI
[section cpowd_text use32]
%endif
_sffecpowd:
	push	ebp
	mov		ebp,esp
;	fld1                          
	;z module
	fld     qword [ebp+0ch]   
	fmul    qword [ebp+0ch]   
	fld     qword [ebp+14h]   
	fmul    qword [ebp+14h]   
	faddp	st1                          
	fsqrt                         
	ftst                          
	fstsw   ax
	test    ah, 1000000b
	jnz     MZ2
	mov     eax, [ebp+08h]
	fld1                          
	;theta
	fld     qword [ebp+14h]	
	fld     qword [ebp+0ch]
	fpatan     
	fmul    qword [ebp+1ch]
	fxch    st2
	;ln||z||
	fyl2x 
	fldl2e
	fdivp    st1
	fmul     qword [ebp+1ch]
  
	call	_sffecexpfnc
	fxch    st1
	fsincos                   
	fld     st2
	fmulp	st1
	fstp    qword [eax]       
	fmulp	st1
	fstp    qword [eax+08h]   
	jmp     END2
MZ2:
	mov     eax, [ebp+08h]
	fstp    qword [eax+08h]   
	fldz
	fstp    qword [eax]
END2:
	fwait
	leave
	ret

;***************** COMPLEX TO INT POWER		
%ifdef DELPHI
[section cpowi_text use32]
%endif
_sffecpowi:
	push	ebp
	mov		ebp,esp
	;z module
	fild    dword [ebp+1ch]   ;st(0)=n 
	fld     qword [ebp+0ch]   
	fmul    qword [ebp+0ch]   
	fld     qword [ebp+14h]
	fmul    qword [ebp+14h]
	faddp	st1
	fsqrt
	ftst
	fstsw   ax
	test    ah, 1000000b
	jnz     MZ5
	mov     eax, [ebp+08h]	
	;||z||^n 
	fld1
	fxch    st1
	fyl2x
	fmulp 	st1
	fld     st0
	frndint
	fsub	to st1	
	fld1
	fscale
	fstp    st1
	fxch    st1
	f2xm1
	fld1
	faddp	st1
	fmulp	st1
	;theta
	fld     qword [ebp+14h]
	fld     qword [ebp+0ch]
	fpatan
	fild    dword [ebp+1ch]
	fmulp	st1
	fsincos
	fld     st2
	fmulp	st1
	fstp    qword [eax]
	fmulp	st1
	fstp    qword [eax+08h]
	jmp     END53
MZ5:	
	mov     eax,  [ebp+08h]
	fstp    qword [eax+08h]
	fldz
	fstp    qword [eax]
	fstp 	st0
END53:
	fwait
	leave	
	ret

;***************** INT/DOUBLE TO COMPLEX POWER	
%ifdef DELPHI
[section cpowc_text use32]
%endif
_sffecpowc:
	push	ebp
	mov		ebp,esp
	fld     qword [ebp+0ch]
	ftst
	fstsw   ax
	test    ah, 1000000b
	jnz     MZ3
	mov     eax, [ebp+08h]	
	;n^a
	fld     qword [ebp+14h]
	fxch    st1
	fabs                          
	fld1
	fxch    st1
	fyl2x                         
	fmulp	st1
	fld     st0
	frndint    
	fsub	to st1
	fld1
	fscale  
	fstp    st1
	fxch    st1
	f2xm1
	fld1       
	faddp	st1
	fmulp	st1
	;ln||n||
	fld     qword [ebp+0ch]
	fabs
	fld1    
	fxch    st1
	fyl2x      
	fldl2e     
	fdivp	st1
	fld     qword [ebp+1ch]
	fmulp	st1
	fsincos    
	fld     st2
	fmulp	st1
	fstp    qword [eax]
	fmulp	st1
	fstp    qword [eax+08h]
	jmp     END3
MZ3:
	mov     eax, [ebp+08h]	
	fstp    qword [eax+08h]
	fldz
	fstp    qword [eax]    
END3:
	fwait 
	leave
	ret
	
;***************** SQRT
%ifdef DELPHI
[section csqrt_text use32]
%endif
_sffecsqrt:
	push	ebp
	mov		ebp,esp
	push 	ebx
	mov     ebx, [ebp+08h]
	;z module
	fld     qword [ebp+0ch]   
	fmul    qword [ebp+0ch]   
	fld     qword [ebp+14h]
	fmul    qword [ebp+14h]
	faddp	st1
	fsqrt      
	fld 	st0	;duplicate |z|
	;real
	fadd	qword [ebp+0ch]
	fld1
	fld1
	faddp	st1
	fdivp 	st1
	fsqrt	
	fstp 	qword [ebx]
	fwait
	;imag
	fsub	qword [ebp+0ch]
	fld1
	fld1
	faddp	st1
	fdivp 	st1
	fsqrt	
	;imag sign check
	push	eax
	fld 	qword [ebp+14h]
	ftst
	fstp 	st0
	fstsw   ax	
	test    ah, 1b
	jz     IMAGPOS
	fchs	
IMAGPOS:
	pop		eax
	fwait
	fstp 	qword [ebx+08h]		
	pop		ebx
	leave
	ret

;***************** Nth ORDER ROOT	
%ifdef DELPHI
[section crtni_text use32]
%endif
_sffecrtni:
	push	ebp
	mov		ebp,esp
	;z module
	fld     qword [ebp+0ch]   
	fmul    qword [ebp+0ch]   
	fld     qword [ebp+14h]
	fmul    qword [ebp+14h]
	faddp	st1
	fsqrt      
	ftst       
	fstsw   ax
	test    ah, 1000000b
	jnz     MZ4
	mov     eax, [ebp+08h]	
	;n-th root if ||z||
	fld1
	fild 	word  [ebp+1ch] 
	fdivp	st1				;st0=1/n
	fxch    st1				;st0=||z|| st1=1/n
	fyl2x                   ;log2(||z||/n)	
	fld     st0				;duplikuj st0
	frndint    
	fsub	to st1			;st0=int( log2(||z||/n) ) st1=frac( log2(||z||/n) )
	fld1					;st0=1
	fscale  				;st0=2^int( log2(||z||/n) ) st1=frac( log2(||z||/n) )
	fstp    st1
	fxch    st1
	f2xm1					;st1=2^frac( log2(||z||/n) )-1
	fld1       
	faddp	st1
	fmulp	st1				;sqrt(||z||)
	;theta
	fld     qword [ebp+14h]
	fld     qword [ebp+0ch]
	fpatan                 	;st0=theta st1=sqrN(||z||)
	;theta_i
	fldpi
	fldpi
	faddp	st1			    ;st0=2Pi st1=theta st2=sqrN(||z||)
	fimul	word  [ebp+20h]
	faddp	st1			    ;st1=theta+i2Pi	
	fidiv 	word  [ebp+1ch] ;st0=theta_i
	;re/im	
	fsincos    

	fld     st2
	fmulp	st1
	fstp    qword [eax]
	fmulp	st1
	fstp    qword [eax+08h]
	jmp     END4
MZ4:
	fldz
	fstp    qword [eax]
	fstp    qword [eax+08h]   
END4:
	leave
	ret

%ifndef DELPHI	
_sffecfunc:	
	push	ebp
	mov		ebp,esp
	mov     eax, [ebp+08h]
	leave
	ret
%endif
