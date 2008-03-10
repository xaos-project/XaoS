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

#ifdef linux
 #define cabs   _cabs
 #define carg   _carg
 #define cargs  _cargs
 #define cargc  _cargc
 #define cinv   _cinv
 #define cexp   _cexp
 #define cln    _cln
 #define clog2  _clog2
 #define clog   _clog
 #define csin   _csin
 #define ccos   _ccos
 #define ctan   _ctan
 #define ccot   _ccot
 #define csinh  _csinh
 #define ccosh  _ccosh
 #define ctanh  _ctanh
 #define ccoth  _ccoth
 #define ccpow  _ccpow
 #define cpowd  _cpowd
 #define cpowi  _cpowi
 #define cpowc  _cpowc
 #define csqrt  _csqrt
 #define crtni  _crtni
#endif

 #ifdef __cplusplus
  extern "C" {
 #endif

 /* written with asm in file cmplx.asm, compile with NASM */
extern double cabs( const cmplx c );
extern double carg( const cmplx c );
extern double cargs( const cmplx c );
extern double cargc( const cmplx c );
extern cmplx cinv( const cmplx c );
extern cmplx cexp( const cmplx c );
extern cmplx cln( const cmplx c );
extern cmplx clog2( const cmplx c );
extern cmplx clog( const cmplx c, unsigned int base );
extern cmplx csin( const cmplx c );
extern cmplx ccos( const cmplx c );
extern cmplx ctan( const cmplx c );
extern cmplx ccot( const cmplx c );
extern cmplx csinh( const cmplx c );
extern cmplx ccosh( const cmplx c );
extern cmplx ctanh( const cmplx c );
extern cmplx ccoth( const cmplx c );
 /* power functions */
extern cmplx ccpow( const cmplx b, const cmplx exp );
extern cmplx cpowd( const cmplx b, double exp );
extern cmplx cpowi( const cmplx b, int exp );
extern cmplx cpowc( double b, const cmplx exp );  
extern cmplx csqrt(const cmplx b ); /* square root */
extern cmplx crtni( const cmplx b, int n, int i ); /* i-th solution of N-th order root of a CN*/
/*complex numbers for mparser*/
cmplx cset( double r, double i );
cmplx cadd(const cmplx c1, const cmplx c2);
cmplx csub(const cmplx c1, const cmplx c2);
cmplx cmul(const cmplx c1, const cmplx c2);
cmplx cdiv(const cmplx c1, const cmplx c2);
sfarg* sfadd( sfarg * const p ); /*  +  */
sfarg* sfsub( sfarg * const p ); /*  -  */
sfarg* sfmul( sfarg * const p ); /*  *  */
sfarg* sfdiv( sfarg * const p ); /*  /  */
sfarg* sfsin( sfarg * const p ); /* sin */
sfarg* sfcos( sfarg * const p ); /* cos */
sfarg* sftan( sfarg * const p ); /* tan */
sfarg* sfcot( sfarg * const p ); /* ctan */
sfarg* sfasin( sfarg * const p ); /* asin */
sfarg* sfacos( sfarg * const p ); /* acos */
sfarg* sfatan( sfarg * const p ); /* atan */
sfarg* sfacot( sfarg * const p ); /* actan */
sfarg* sfatan2( sfarg * const p ); /* atan2 */
sfarg* sfsinh( sfarg * const p ); /* sinh */
sfarg* sfcosh( sfarg * const p ); /* cosh */
sfarg* sftanh( sfarg * const p ); /* tanh */
sfarg* sfcoth( sfarg * const p ); /* ctanh */
sfarg* sfexp( sfarg * const p ); /* exp */
sfarg* sflog( sfarg * const p ); /* log */
sfarg* sflog2( sfarg * const p ); /* log2 */
sfarg* sflog10( sfarg * const p ); /* log2 */
sfarg* sflogN( sfarg * const p ); /* logN */
sfarg* sflogCN( sfarg * const p ); /* logCN */
sfarg* sfpow( sfarg * const p ); /* csflx pow */
sfarg* sfpowi( sfarg * const p ); /* int pow */
sfarg* sfpowd( sfarg * const p ); /* double pow */
sfarg* sfpowdc( sfarg * const p ); /* double to csflx pow */
sfarg* sfsqr( sfarg * const p ); /* sqr */
sfarg* sfsqrt( sfarg * const p ); /* sqrt */
sfarg* sfrtni( sfarg * const p ); /* rtni */ /*cos tu nie tak jak powinno byc ;(*/
sfarg* sfinv( sfarg * const p ); /* cinv */
sfarg* sfceil( sfarg * const p ); /* ceil */
sfarg* sffloor( sfarg * const p ); /* floor */
sfarg* sfabs( sfarg * const p ); /* abs - |z|*/
sfarg* sfre( sfarg * const p ); /* RE */
sfarg* sfim( sfarg * const p ); /* IM */
sfarg* sfrabs( sfarg * const p ); /* abs - real numbers*/
sfarg* sfrand( sfarg * const p ); /* rand */
/*const eval*/
void sfcPI( sfNumber *cnst );
void sfcPI2( sfNumber *cnst );
void sfc2PI( sfNumber *cnst );
void sfcE( sfNumber *cnst );
void sfcI( sfNumber *cnst );
void sfcRND( sfNumber *cnst );
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
