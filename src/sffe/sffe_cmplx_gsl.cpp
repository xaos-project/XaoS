/*/////////////////////////////////////////////////////////////////////////////////////
// project : sFFe ( SegFault (or Segmentation Fault :) ) formula evalutaor )
// author  : Mateusz Malczak (mateusz@malczak.info)
// wpage   : www.segfaultlabs.com/projects/sffe
///////////////////////////////////////////////////////////////////////////////////////
// special build for XaoS, for more info visit
// http://www.segfaultlabs.com/projects/sfXaos
/////////////////////////////////////////////////////////////////////////////////////*/

#ifdef SFFE_CMPLX_GSL

#include "sffe.h"
#include "sffe_cmplx_gsl.h"
#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#include <math.h>

const sffunction sfcmplxfunc[sffnctscount] = {
    /* operators */
    {sfpow, 2, "^\0"},
    {sfadd, 2, "+\0"},
    {sfsub, 2, "-\0"},
    {sfmul, 2, "*\0"},
    {sfdiv, 2, "/\0"},

    /* functions */
    {sfsin, 1, "sin\0"},
    {sfcos, 1, "cos\0"},
    {sftan, 1, "tan\0"},
    {sfcot, 1, "cot\0"},
    {sfasin, 1, "asin\0"},
    {sfacos, 1, "acos\0"},
    {sfatan, 1, "atan\0"},
    {sfacot, 1, "acot\0"},
    {sfatan2, 2, "atan2\0"},
    {sfsinh, 1, "sinh\0"},
    {sfcosh, 1, "cosh\0"},
    {sftanh, 1, "tanh\0"},
    {sfcoth, 1, "coth\0"},
    {sfexp, 1, "exp\0"},
    {sflog, 1, "log\0"},
    {sflog10, 1, "log10\0"},
    {sflog2, 1, "log2\0"},
    {sflogN, 2, "logn\0"},
    {sflogN, 2, "logcn\0"},
    /*power functions */
    {sfpow, 2, "pow\0"},
    {sfpowd, 2, "powd\0"},
    {sfpow, 2, "powi\0"},
    {sfpow, 2, "powdc\0"},
    {sfsqr, 1, "sqr\0"},
    {sfsqrt, 1, "sqrt\0"},
    {sfrtni, 3, "rtni"},
    {sfinv, 1, "inv\n"},
    {sfceil, 1, "ceil\0"},
    {sffloor, 1, "floor\0"},
    {sfabs, 1, "abs\0"},
    {sfrabs, 1, "rabs\0"},
    {sfre, 1, "re\0"},
    {sfim, 1, "im\0"},

    {NULL, 1, "rad\0"},
    {NULL, 1, "deg\0"},
    {NULL, 1, "sign\0"},
    {NULL, 1, "trunc\0"},
    {sfrand, 1, "rand\0"}};

const char sfcnames[sfvarscount][6] = {"pi\0", "pi_2\0", "pi2\0",
                                       "e\0",  "i\0",    "rnd\0"};

const cfptr sfcvals[sfvarscount] = {sfcPI, sfcPI2, sfc2PI, sfcE, sfcI, sfcRND};

sfarg *sfadd(sfarg *const p)
{ /* + */
    sfvalue(p) = gsl_complex_add(sfvalue(sfaram2(p)), sfvalue(sfaram1(p)));
    return sfaram2(p);
}

sfarg *sfsub(sfarg *const p)
{ /* - */
    sfvalue(p) = gsl_complex_sub(sfvalue(sfaram2(p)), sfvalue(sfaram1(p)));
    return sfaram2(p);
}

sfarg *sfmul(sfarg *const p)
{ /* *  */
    sfvalue(p) = gsl_complex_mul(sfvalue(sfaram2(p)), sfvalue(sfaram1(p)));
    return sfaram2(p);
}

sfarg *sfdiv(sfarg *const p)
{ /*  /   */
    sfvalue(p) = gsl_complex_div(sfvalue(sfaram2(p)), sfvalue(sfaram1(p)));
    return sfaram2(p);
}

sfarg *sfsin(sfarg *const p)
{ /* sin */
    sfvalue(p) = gsl_complex_sin(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfcos(sfarg *const p)
{ /* cos */
    sfvalue(p) = gsl_complex_cos(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sftan(sfarg *const p)
{ /* tan */
    sfvalue(p) = gsl_complex_tan(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfcot(sfarg *const p)
{ /* ctan */
    sfvalue(p) = gsl_complex_cot(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfasin(sfarg *const p)
{ /* asin */
    sfvalue(p) = gsl_complex_arcsin(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfacos(sfarg *const p)
{ /* acos */
    sfvalue(p) = gsl_complex_arccos(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfatan(sfarg *const p)
{ /* atan */
    sfvalue(p) = gsl_complex_arctan(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfacot(sfarg *const p)
{ /* actan */
    sfvalue(p) = gsl_complex_arccot(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfatan2(sfarg *const p)
{ /* atan2 */
    return sfaram2(p);
}

sfarg *sfsinh(sfarg *const p)
{ /* sinh */
    sfvalue(p) = gsl_complex_sinh(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfcosh(sfarg *const p)
{ /* cosh */
    sfvalue(p) = gsl_complex_cosh(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sftanh(sfarg *const p)
{ /* tanh */
    sfvalue(p) = gsl_complex_tanh(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfcoth(sfarg *const p)
{ /* ctanh */
    sfvalue(p) = gsl_complex_coth(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfexp(sfarg *const p)
{ /* exp */
    sfvalue(p) = gsl_complex_exp(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sflog(sfarg *const p)
{ /* log */
    sfvalue(p) = gsl_complex_log(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sflog10(sfarg *const p)
{ /* log10 */
    sfvalue(p) = gsl_complex_log10(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sflog2(sfarg *const p)
{ /* log2 */
    sfNumber base;
    real(base) = 2;
    imag(base) = 0;
    sfvalue(p) = gsl_complex_log_b(sfvalue(sfaram1(p)), base);
    return sfaram1(p);
}

sfarg *sflogN(sfarg *const p)
{ /* logN */
    sfvalue(p) = gsl_complex_log_b(sfvalue(sfaram1(p)), sfvalue(sfaram2(p)));
    return sfaram2(p);
}

sfarg *sfpow(sfarg *const p)
{ /* cmplx pow */
    sfvalue(p) = gsl_complex_pow(sfvalue(sfaram2(p)), sfvalue(sfaram1(p)));
    return sfaram2(p);
}

sfarg *sfpowd(sfarg *const p)
{ /* int pow */
    sfvalue(p) = gsl_complex_pow_real(sfvalue(sfaram2(p)),
                                      GSL_REAL(sfvalue(sfaram1(p))));
    return sfaram2(p);
}

sfarg *sfsqr(sfarg *const p)
{ /* sqr */
    sfvalue(p) = gsl_complex_pow(sfvalue(sfaram1(p)), sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfsqrt(sfarg *const p)
{ /* sqrt */
    sfvalue(p) = gsl_complex_sqrt(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfrtni(sfarg *const p)
{ /* rtni */
    double nrz = pow(gsl_complex_abs(sfvalue(sfaram3(p))),
                     1.0 / (double)(int)real(sfvalue(sfaram2(p))));
    double alfi = (gsl_complex_arg(sfvalue(sfaram3(p))) +
                   8 * atan(1.0) * (double)(int)real(sfvalue(sfaram1(p)))) /
                  (double)(int)real(sfvalue(sfaram2(p)));

    cmplxset(sfvalue(sfaram3(p)), nrz * cos(alfi), nrz * sin(alfi));
    return sfaram3(p);
}

sfarg *sfinv(sfarg *const p)
{ /* cinv */
    sfvalue(p) = gsl_complex_inverse(sfvalue(sfaram1(p)));
    return sfaram1(p);
}

sfarg *sfceil(sfarg *const p)
{ /* ceil */
    // sfvalue(p) = ceil( sfvalue( sfaram1(p) ) );
    return sfaram1(p);
}

sfarg *sffloor(sfarg *const p)
{ /* floor */
    // sfvalue(p) = floor( sfvalue( sfaram1(p) ) );
    return sfaram1(p);
}

sfarg *sfabs(sfarg *const p)
{ /* abs - |z| */
    GSL_REAL(sfvalue(p)) = gsl_complex_abs(sfvalue(sfaram1(p)));
    GSL_IMAG(sfvalue(p)) = 0.0;
    return sfaram1(p);
}

sfarg *sfrabs(sfarg *const p)
{ /* abs - real numbers */
    GSL_REAL(sfvalue(p)) = GSL_REAL(sfvalue(sfaram1(p)));
    if (GSL_REAL(sfvalue(p)) < 0)
        GSL_REAL(sfvalue(p)) = -GSL_REAL(sfvalue(p));
    GSL_IMAG(sfvalue(p)) = 0;
    return sfaram1(p);
}

sfarg *sfre(sfarg *const p)
{ /* RE */
    GSL_REAL(sfvalue(p)) = GSL_REAL(sfvalue(sfaram1(p)));
    GSL_IMAG(sfvalue(p)) = 0.0;
    return sfaram1(p);
}

sfarg *sfim(sfarg *const p)
{ /* IM */
    GSL_REAL(sfvalue(p)) = GSL_IMAG(sfvalue(sfaram1(p)));
    GSL_IMAG(sfvalue(p)) = 0.0;
    return sfaram1(p);
}

sfarg *sfrand(sfarg *const p)
{ /* rand */
    GSL_REAL(sfvalue(p)) =
        GSL_REAL(sfvalue(sfaram1(p))) * (double)rand() / (double)RAND_MAX;
    GSL_IMAG(sfvalue(p)) = 0;
    return sfaram1(p);
}

// const eval
void sfcPI(sfNumber *cnst) { GSL_SET_COMPLEX(cnst, 4 * atan(1), 0); }

void sfcPI2(sfNumber *cnst) { GSL_SET_COMPLEX(cnst, 2 * atan(1), 0); }

void sfc2PI(sfNumber *cnst) { GSL_SET_COMPLEX(cnst, 8 * atan(1), 0); }

void sfcE(sfNumber *cnst) { GSL_SET_COMPLEX(cnst, exp(1), 0); }

void sfcI(sfNumber *cnst) { GSL_SET_COMPLEX(cnst, 0, 1); }

void sfcRND(sfNumber *cnst) { GSL_SET_COMPLEX(cnst, rand(), 0); }

#endif
