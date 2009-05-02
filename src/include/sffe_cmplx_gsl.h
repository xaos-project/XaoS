/*/////////////////////////////////////////////////////////////////////////////////////
// project : sFFe ( SegFault (or Segmentation Fault :) ) formula evalutaor )
// author  : Mateusz Malczak ( mateusz@malczak.info )
// wpage   : www.segfaultlabs.com/projects/sffe
///////////////////////////////////////////////////////////////////////////////////////
// special build for XaoS, for more info visit
// http://www.segfaultlabs.com/projects/sfXaos
/////////////////////////////////////////////////////////////////////////////////////*/
#ifndef SFFE_CMPLX_GSL_H
#define SFFE_CMPLX_GSL_H

#include <config.h>
#ifdef SFFE_CMPLX_GSL

#include "sffe.h"

#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>

#define sffnctscount 43
#define sfvarscount 6
#define cmplxset(c,r,i) GSL_SET_COMPLEX(&c,r,i)
#define real(c) GSL_REAL((c))
#define imag(c) GSL_IMAG((c))

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
sfarg *sflog10(sfarg * const p);	/* log10 */
sfarg *sflog2(sfarg * const p);	/* log2 */
sfarg *sflogN(sfarg * const p);	/* logN */
sfarg *sfpow(sfarg * const p);	/* csflx pow */
sfarg *sfpowd(sfarg * const p);	/* double pow */
sfarg *sfpowi(sfarg * const p);	/* double pow */
sfarg *sfpowdc(sfarg * const p);	/* double to csflx pow */
sfarg *sfsqr(sfarg * const p);	/* sqr */
sfarg *sfsqrt(sfarg * const p);	/* sqrt */
sfarg *sfrtni(sfarg * const p);	/* rtni */
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
