/*/////////////////////////////////////////////////////////////////////////////////////
// project : sFFe ( SegFault (or Segmentation Fault :) ) formula evalutaor )
// author  : Mateusz Malczak ( mateusz@malczak.info )
// wpage   : www.segfaultlabs.com/projects/sffe
///////////////////////////////////////////////////////////////////////////////////////
// special build for XaoS, for more info visit
// http://www.segfaultlabs.com/projects/sfXaos
/////////////////////////////////////////////////////////////////////////////////////*/
#ifndef SFFE_CMPLX_ASM_H
#define SFFE_CMPLX_ASM_H

#include <config.h>
#ifdef SFFE_CMPLX_ASM

#include "sffe.h"

#define sffnctscount 43
#define sfvarscount 6
#define cmplxset(c,r,i) ( c = cset(r,i) )
#define real(c) (c).r
#define imag(c) (c).i

#ifdef __ELF__
#define sffecabs   _sffecabs
#define sffecarg   _sffecarg
#define sffecargs  _sffecargs
#define sffecargc  _sffecargc
#define sffecinv   _sffecinv
#define sffecexp   _sffecexp
#define sffecln    _sffecln
#define sffeclog2  _sffeclog2
#define sffeclog   _sffeclog
#define sffecsin   _sffecsin
#define sffeccos   _sffeccos
#define sffectan   _sffectan
#define sffeccot   _sffeccot
#define sffecsinh  _sffecsinh
#define sffeccosh  _sffeccosh
#define sffectanh  _sffectanh
#define sffeccoth  _sffeccoth
#define sffeccpow  _sffeccpow
#define sffecpowd  _sffecpowd
#define sffecpowi  _sffecpowi
#define sffecpowc  _sffecpowc
#define sffecsqrt  _sffecsqrt
#define sffecrtni  _sffecrtni
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* written with asm in file cmplx.asm, compile with NASM */
extern double sffecabs(const cmplx c);
extern double sffecarg(const cmplx c);
extern double sffecargs(const cmplx c);
extern double sffecargc(const cmplx c);
extern cmplx sffecinv(const cmplx c);
extern cmplx sffecexp(const cmplx c);
extern cmplx sffecln(const cmplx c);
extern cmplx sffeclog2(const cmplx c);
extern cmplx sffeclog(const cmplx c, unsigned int base);
extern cmplx sffecsin(const cmplx c);
extern cmplx sffeccos(const cmplx c);
extern cmplx sffectan(const cmplx c);
extern cmplx sffeccot(const cmplx c);
extern cmplx sffecsinh(const cmplx c);
extern cmplx sffeccosh(const cmplx c);
extern cmplx sffectanh(const cmplx c);
extern cmplx sffeccoth(const cmplx c);
/* power functions */
extern cmplx sffeccpow(const cmplx b, const cmplx exp);
extern cmplx sffecpowd(const cmplx b, double exp);
extern cmplx sffecpowi(const cmplx b, int exp);
extern cmplx sffecpowc(double b, const cmplx exp);
extern cmplx sffecsqrt(const cmplx b);	/* square root */
extern cmplx sffecrtni(const cmplx b, int n, int i);	/* i-th solution of N-th order root of a CN */
/*complex numbers for mparser*/
cmplx cset(double r, double i);
cmplx cadd(const cmplx c1, const cmplx c2);
cmplx csub(const cmplx c1, const cmplx c2);
cmplx cmul(const cmplx c1, const cmplx c2);
cmplx cdiv(const cmplx c1, const cmplx c2);
sfarg *sfadd(sfarg * const p);	/*  +  */
sfarg *sfsub(sfarg * const p);	/*  -  */
sfarg *sfmul(sfarg * const p);	/*  *  */
sfarg *sfdiv(sfarg * const p);	/*  /  */
sfarg *sfsin(sfarg * const p);	/* sin */
sfarg *sfcos(sfarg * const p);	/* cos */
sfarg *sftan(sfarg * const p);	/* tan */
sfarg *sfcot(sfarg * const p);	/* ctan */
sfarg *sfasin(sfarg * const p);	/* asin */
sfarg *sfacos(sfarg * const p);	/* acos */
sfarg *sfatan(sfarg * const p);	/* atan */
sfarg *sfacot(sfarg * const p);	/* actan */
sfarg *sfatan2(sfarg * const p);	/* atan2 */
sfarg *sfsinh(sfarg * const p);	/* sinh */
sfarg *sfcosh(sfarg * const p);	/* cosh */
sfarg *sftanh(sfarg * const p);	/* tanh */
sfarg *sfcoth(sfarg * const p);	/* ctanh */
sfarg *sfexp(sfarg * const p);	/* exp */
sfarg *sflog(sfarg * const p);	/* log */
sfarg *sflog2(sfarg * const p);	/* log2 */
sfarg *sflog10(sfarg * const p);	/* log2 */
sfarg *sflogN(sfarg * const p);	/* logN */
sfarg *sflogCN(sfarg * const p);	/* logCN */
sfarg *sfpow(sfarg * const p);	/* csflx pow */
sfarg *sfpowi(sfarg * const p);	/* int pow */
sfarg *sfpowd(sfarg * const p);	/* double pow */
sfarg *sfpowdc(sfarg * const p);	/* double to csflx pow */
sfarg *sfsqr(sfarg * const p);	/* sqr */
sfarg *sfsqrt(sfarg * const p);	/* sqrt */
sfarg *sfrtni(sfarg * const p);	/* rtni *//*cos tu nie tak jak powinno byc ;( */
sfarg *sfinv(sfarg * const p);	/* cinv */
sfarg *sfceil(sfarg * const p);	/* ceil */
sfarg *sffloor(sfarg * const p);	/* floor */
sfarg *sfabs(sfarg * const p);	/* abs - |z| */
sfarg *sfre(sfarg * const p);	/* RE */
sfarg *sfim(sfarg * const p);	/* IM */
sfarg *sfrabs(sfarg * const p);	/* abs - real numbers */
sfarg *sfrand(sfarg * const p);	/* rand */
/*const eval*/
void sfcPI(sfNumber * cnst);
void sfcPI2(sfNumber * cnst);
void sfc2PI(sfNumber * cnst);
void sfcE(sfNumber * cnst);
void sfcI(sfNumber * cnst);
void sfcRND(sfNumber * cnst);
#ifdef __cplusplus
}
#endif
/* all available function (function pointer, number of parameters, name )*/
    extern const sffunction sfcmplxfunc[sffnctscount];
/* all available buildin variables */
extern const char sfcnames[sfvarscount][5];
/* available variables function pointers */
extern const cfptr sfcvals[sfvarscount];

#endif
#endif
