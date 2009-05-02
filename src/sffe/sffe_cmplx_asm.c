/*/////////////////////////////////////////////////////////////////////////////////////
// project : sFFe ( SegFault (or Segmentation Fault :) ) formula evalutaor )
// author  : Mateusz Malczak ( mateusz@malczak.info )
// wpage   : www.segfaultlabs.com/projects/sffe
///////////////////////////////////////////////////////////////////////////////////////
// special build for XaoS, for more info visit
// http://www.segfaultlabs.com/projects/sfXaos
/////////////////////////////////////////////////////////////////////////////////////*/

#include <config.h>
#ifdef SFFE_CMPLX_ASM

#include <math.h>
#include "sffe.h"
#include "sffe_cmplx_asm.h"

#ifdef __cplusplus
extern "C" {
#endif


    const sffunction sfcmplxfunc[sffnctscount] = {
	/* nie uwzgledniaj w wyszukaniu funkcji */
	{sfpow, 2, "^\0"}, {sfadd, 2, "+\0"}, {sfsub, 2, "-\0"}, {sfmul, 2,
								  "*\0"},
	    {sfdiv, 2, "/\0"},
	/* ponizej uwzgledniaj w wyszukaniu funkcji */
	{sfsin, 1, "SIN\0"}, {sfcos, 1, "COS\0"}, {sftan, 1, "TAN\0"},
	    {sfcot, 1, "COT\0"},
	{sfasin, 1, "ASIN\0"}, {sfacos, 1, "ACOS\0"}, {sfatan, 1,
						       "ATAN\0"}, {sfacot,
								   1,
								   "ACOT\0"},
	{sfatan2, 2, "ATAN2\0"},
	{sfsinh, 1, "SINH\0"}, {sfcosh, 1, "COSH\0"}, {sftanh, 1,
						       "TANH\0"}, {sfcoth,
								   1,
								   "COTH\0"},
	{sfexp, 1, "EXP\0"}, {sflog, 1, "LOG\0"}, {sflog10, 1, "LOG10\0"},
	    {sflog2, 1, "LOG2\0"},
	{sflogN, 2, "LOGN\0"}, {sflogCN, 2, "LOGCN\0"},
	/*power functions */
	{sfpow, 2, "POW\0"}, {sfpowi, 2, "POWI\0"}, {sfpowd, 2, "POWD\0"},
	    {sfpowdc, 2, "POWDC\0"},
	{sfsqr, 1, "SQR\0"}, {sfsqrt, 1, "SQRT\0"}, {sfrtni, 3, "RTNI"},
	    {sfinv, 1, "INV\n"},
	{sfceil, 1, "CEIL\0"}, {sffloor, 1, "FLOOR\0"}, {sfabs, 1,
							 "ABS\0"}, {sfrabs,
								    1,
								    "RABS\0"},
	{sfre, 1, "RE\0"}, {sfim, 1, "IM\0"},
	{NULL, 1, "RAD\0"}, {NULL, 1, "DEG\0"},
	{NULL, 1, "SIGN\0"}, {NULL, 1, "TRUNC\0"}, {sfrand, 1, "RAND\0"}
    };

    const char sfcnames[sfvarscount][5] =
	{ "PI\0", "PI_2\0", "PI2\0", "E\0", "I\0", "RND\0" };

    const cfptr sfcvals[sfvarscount] =
	{ sfcPI, sfcPI2, sfc2PI, sfcE, sfcI, sfcRND };


    cmplx cset(double r, double i) {
	cmplx c;
	c.r = r;
	c.i = i;
	return c;
    };

    cmplx cadd(const cmplx c1, const cmplx c2) {
	cmplx r;
	r.r = c1.r + c2.r;
	r.i = c1.i + c2.i;
	return r;
    };

    cmplx csub(const cmplx c1, const cmplx c2) {
	cmplx r;
	r.r = c1.r - c2.r;
	r.i = c1.i - c2.i;
	return r;
    };

    cmplx cmul(const cmplx c1, const cmplx c2) {
	cmplx r;
	r.r = c1.r * c2.r - c1.i * c2.i;
	r.i = c1.r * c2.i + c1.i * c2.r;
	return r;
    };

    cmplx cdiv(const cmplx c1, const cmplx c2) {
	double d = (c2.r * c2.r + c2.i * c2.i);
	cmplx r;
	r.r = (c1.r * c2.r + c1.i * c2.i) / d;
	r.i = (-c1.i * c2.r + c1.r * c2.i) / d;
	return r;
    };

    sfarg *sfadd(sfarg * const p) {	/* + */
	sfvalue(p) = cadd(sfvalue(sfaram2(p)), sfvalue(sfaram1(p)));
	return sfaram2(p);
    };

    sfarg *sfsub(sfarg * const p) {	/* - */
	sfvalue(p) = csub(sfvalue(sfaram2(p)), sfvalue(sfaram1(p)));
	return sfaram2(p);
    };

    sfarg *sfmul(sfarg * const p) {	/* *  */
	sfvalue(p) = cmul(sfvalue(sfaram2(p)), sfvalue(sfaram1(p)));
	return sfaram2(p);
    };

    sfarg *sfdiv(sfarg * const p) {	/*  /   */
	sfvalue(p) = cdiv(sfvalue(sfaram2(p)), sfvalue(sfaram1(p)));
	return sfaram2(p);
    };


    sfarg *sfsin(sfarg * const p) {	/* sin */
	sfvalue(p) = sffecsin(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };

    sfarg *sfcos(sfarg * const p) {	/* cos */
	sfvalue(p) = sffeccos(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };

    sfarg *sftan(sfarg * const p) {	/* tan */
	sfvalue(p) = sffectan(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };

    sfarg *sfcot(sfarg * const p) {	/* ctan */
	sfvalue(p) = sffeccot(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };


    sfarg *sfasin(sfarg * const p) {	/* asin */
	//sfvalue(p) = asin( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
    };

    sfarg *sfacos(sfarg * const p) {	/* acos */
	//sfvalue(p) = acos( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
    };

    sfarg *sfatan(sfarg * const p) {	/* atan */
// sfvalue(p) = atan( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
    };

    sfarg *sfacot(sfarg * const p) {	/* actan */
// sfvalue(p) = 1.0/atan( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
    };

    sfarg *sfatan2(sfarg * const p) {	/* atan2 */
	//sfvalue(p) = atan2( sfvalue( sfaram2(p) ), sfvalue( sfaram1(p) ) );
	return sfaram2(p);
    };


    sfarg *sfsinh(sfarg * const p) {	/* sinh */
	sfvalue(p) = sffecsinh(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };

    sfarg *sfcosh(sfarg * const p) {	/* cosh */
	sfvalue(p) = sffeccosh(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };

    sfarg *sftanh(sfarg * const p) {	/* tanh */
	sfvalue(p) = sffectanh(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };

    sfarg *sfcoth(sfarg * const p) {	/* ctanh */
	sfvalue(p) = sffeccoth(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };


    sfarg *sfexp(sfarg * const p) {	/* exp */
	sfvalue(p) = sffecexp(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };

    sfarg *sflog(sfarg * const p) {	/* log */
	sfvalue(p) = sffecln(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };

    sfarg *sflog2(sfarg * const p) {	/* log2 */
	sfvalue(p) = sffeclog2(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };

    sfarg *sflog10(sfarg * const p) {	/* log2 */
	sfvalue(p) = sffeclog(sfvalue(sfaram1(p)), 10);
	return sfaram1(p);
    };

    sfarg *sflogN(sfarg * const p) {	/* logN */
	sfvalue(p) = sffeclog(sfvalue(sfaram1(p)), sfvalue(sfaram2(p)).r);
	return sfaram2(p);
    };

    sfarg *sflogCN(sfarg * const p) {	/* logCN */
	sfvalue(p) =
	    cdiv(sffecln(sfvalue(sfaram2(p))),
		 sffecln(sfvalue(sfaram1(p))));
	return sfaram2(p);
    };

    sfarg *sfpow(sfarg * const p) {	/* csflx pow */
	sfvalue(p) = sffeccpow(sfvalue(sfaram2(p)), sfvalue(sfaram1(p)));
	return sfaram2(p);
    };

    sfarg *sfpowi(sfarg * const p) {	/* int pow */
	sfvalue(p) =
	    sffecpowi(sfvalue(sfaram2(p)), (int) (sfvalue(sfaram1(p)).r));
	return sfaram2(p);
    };

    sfarg *sfpowd(sfarg * const p) {	/* double pow */
	sfvalue(p) = sffecpowd(sfvalue(sfaram2(p)), sfvalue(sfaram1(p)).r);
	return sfaram2(p);
    };

    sfarg *sfpowdc(sfarg * const p) {	/* double to csflx pow */
	sfvalue(p) = sffecpowc(sfvalue(sfaram2(p)).r, sfvalue(sfaram1(p)));
	return sfaram2(p);
    };

    sfarg *sfsqr(sfarg * const p) {	/* sqr */
	sfvalue(p) = cmul(sfvalue(sfaram1(p)), sfvalue(sfaram1(p)));
	return sfaram1(p);
    };

    sfarg *sfsqrt(sfarg * const p) {	/* sqrt */
	sfvalue(p) = sffecsqrt(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };

    sfarg *sfrtni(sfarg * const p)	/* rtni *///cos tu nie tak jak powinno byc ;(
    {
	sfvalue(p) =
	    sffecrtni(sfvalue(sfaram3(p)), (int) (sfvalue(sfaram2(p)).r),
		      (int) (sfvalue(sfaram1(p)).r));
	return sfaram3(p);
    };

    sfarg *sfinv(sfarg * const p) {	/* cinv */
	sfvalue(p) = sffecinv(sfvalue(sfaram1(p)));
	return sfaram1(p);
    };

    sfarg *sfceil(sfarg * const p) {	/* ceil */
	//sfvalue(p) = ceil( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
    };

    sfarg *sffloor(sfarg * const p) {	/* floor */
	//sfvalue(p) = floor( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
    };

    sfarg *sfabs(sfarg * const p) {	/* abs - |z| */
	sfvalue(p).r = sffecabs(sfvalue(sfaram1(p)));
	sfvalue(p).i = 0;
	return sfaram1(p);
    };

    sfarg *sfrabs(sfarg * const p) {	/* abs - real numbers */
	sfvalue(p).r = sfvalue(sfaram1(p)).r;
	if (sfvalue(p).r < 0)
	    sfvalue(p).r = -sfvalue(p).r;
	sfvalue(p).i = 0;
	return sfaram1(p);
    };

    sfarg *sfre(sfarg * const p) {	/* RE */
	sfvalue(p).r = sfvalue(sfaram1(p)).r;
	sfvalue(p).i = 0;
	return sfaram1(p);
    };

    sfarg *sfim(sfarg * const p) {	/* IM */
	sfvalue(p).r = sfvalue(sfaram1(p)).i;
	sfvalue(p).i = 0;
	return sfaram1(p);
    };

    sfarg *sfrand(sfarg * const p) {	/* rand */
	sfvalue(p).r =
	    sfvalue(sfaram1(p)).r * (double) rand() / (double) RAND_MAX;
	sfvalue(p).i = 0;
	return sfaram1(p);
    };

//const eval
    void sfcPI(sfNumber * cnst) {
	*cnst = cset(4 * atan(1), 0);
    };
    void sfcPI2(sfNumber * cnst) {
	*cnst = cset(2 * atan(1), 0);
    };
    void sfc2PI(sfNumber * cnst) {
	*cnst = cset(8 * atan(1), 0);
    };
    void sfcE(sfNumber * cnst) {
	*cnst = cset(exp(1), 0);
    };
    void sfcI(sfNumber * cnst) {
	*cnst = cset(0, 1);
    };
    void sfcRND(sfNumber * cnst) {
	*cnst = cset((double) rand() / (double) RAND_MAX, 0);
    };

#ifdef __cplusplus
}
#endif

#endif
