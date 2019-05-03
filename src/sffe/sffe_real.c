#ifdef SFFE_DOUBLE
#include "sffe.h"

#include <math.h>
#include <string.h>

#define sfEPSILON 1e-16
 
/*! IMPLEMENTACJA WSZSYTKICH DOOSTEPNYCH W STANDARDZIE FUNKCJI RZECZYWISTYCH !*/
#ifdef __cplusplus
 extern "C" {
#endif

const sffunction sfcmplxfunc[sffnctscount] =  {
		/* basic real arithmetics */
		{sfpow,2,"^\0"}, 
		{sfadd,2,"+\0"}, 
		{sfsub,2,"-\0"}, 
		{sfmul,2,"*\0"}, 
		{sfdiv,2,"/\0"},

		/* function */
		{sfsin,1,"SIN\0"}, 
		{sfcos,1,"COS\0"}, 
		{sftan,1,"TAN\0"}, 
		{sfcot,1,"COT\0"},
		{sfasin,1,"ASIN\0"}, 
		{sfacos,1,"ACOS\0"}, 
		{sfatan,1,"ATAN\0"}, 
		{sfacot,1,"ACOT\0"},
		{sfatan2,2,"ATAN2\0"}, 
		{sfsinh,1,"SINH\0"}, 
		{sfcosh,1,"COSH\0"}, 
		{sftanh,1,"TANH\0"}, 
		{sfcoth,1,"COTH\0"},
		{sfexp,1,"EXP\0"}, 
		{sflog,1,"LOG\0"}, 
		{sflog10,1,"LOG10\0"}, 
		{sflogN,2,"LOGN\0"},
		{sfpow,2,"POW\0"},
		{sfsqr,1,"SQR\0"},
		{sfsqrt,1,"SQRT\0"},
		{sfceil,1,"CEIL\0"}, 
		{sffloor,1,"FLOOR\0"},
		{sfabs,1,"ABS\0"}
};

const char sfcnames[sfvarscount][5] = 
{"PI\0","PI_2\0","PI2\0","E\0","EPS\0","RND\0"};

const cfptr sfcvals[sfvarscount] = 
{ sfcPI, sfcPI2, sfc2PI, sfcE, sfcEPS, sfcRND };


sfarg *sfadd( sfarg * const p, void *payload ) /* + */
{
	sfvalue(p) = sfvalue( sfaram2(p) ) + sfvalue( sfaram1(p) );
	return sfaram2(p);
};

sfarg *sfsub( sfarg * const p, void *payload ) /* - */
{
	sfvalue(p) = sfvalue( sfaram2(p) ) - sfvalue( sfaram1(p) );
	return sfaram2(p);
};

sfarg *sfmul( sfarg * const p, void *payload ) /* *  */
{
	sfvalue(p) = sfvalue( sfaram2(p) ) * sfvalue( sfaram1(p) );
	return sfaram2(p);
};

sfarg *sfdiv( sfarg * const p, void *payload ) /*  / */
{
	sfvalue(p) = sfvalue( sfaram2(p) ) / sfvalue( sfaram1(p) );
	return sfaram2(p);
};

sfarg *sfsin( sfarg * const p, void *payload ) /* sin */
{
	sfvalue(p) = sin( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sfcos( sfarg * const p, void *payload ) /* cos */
{
	sfvalue(p) = cos( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sftan( sfarg * const p, void *payload ) /* tan */
{
	sfvalue(p) = tan( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sfcot( sfarg * const p, void *payload ) /* ctan */
{
	sfvalue(p) = 1.0/tan( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sfasin( sfarg * const p, void *payload ) /* asin */
{
	sfvalue(p) = asin( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sfacos( sfarg * const p, void *payload ) /* acos */
{
	sfvalue(p) = acos( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sfatan( sfarg * const p, void *payload ) /* atan */
{
	sfvalue(p) = atan( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sfacot( sfarg * const p, void *payload ) /* actan */
{
	sfvalue(p) = 1.0/atan( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sfatan2( sfarg * const p, void *payload ) /* atan2 */
{
	sfvalue(p) = atan2( sfvalue( sfaram2(p) ), sfvalue( sfaram1(p) ) );
	return sfaram2(p);
};


sfarg *sfsinh( sfarg * const p, void *payload ) /* sinh */
{
	sfvalue(p) = sinh( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sfcosh( sfarg * const p, void *payload ) /* cosh */
{
	sfvalue(p) = cosh( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sftanh( sfarg * const p, void *payload ) /* tanh */
{
	sfvalue(p) = tanh( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sfcoth( sfarg * const p, void *payload ) /* ctanh */
{
	sfvalue(p) = 1.0/tanh( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};


sfarg *sfexp( sfarg * const p, void *payload ) /* exp */
{
	sfvalue(p) = exp( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sflog( sfarg * const p, void *payload ) /* log */
{
	sfvalue(p) = log( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sflog10( sfarg * const p, void *payload ) /* log10 */
{
	sfvalue(p) = log10( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sflogN( sfarg * const p, void *payload ) /* logN */
{
	sfvalue(p) = log( sfvalue( sfaram1(p) ) )/log( sfvalue( sfaram2(p) ) );;
	return sfaram2(p);
};

sfarg *sfpow( sfarg * const p, void *payload ) /* pow */
{
	sfvalue(p) = pow( sfvalue( sfaram2(p) ), sfvalue( sfaram1(p) ) );
	return sfaram2(p);
};

sfarg *sfsqr( sfarg * const p, void *payload ) /* sqr */
{
	sfvalue(p) = sfvalue( sfaram1(p) )*sfvalue( sfaram1(p) );
	return sfaram1(p);
};

sfarg *sfsqrt( sfarg * const p, void *payload ) /* sqrt */
{
	sfvalue(p) = sqrt( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sfceil( sfarg * const p, void *payload ) /* ceil */
{
	sfvalue(p) = ceil( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sffloor( sfarg * const p, void *payload ) /* floor */
{
	sfvalue(p) = floor( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

sfarg *sfabs( sfarg * const p, void *payload ) /* abs */
{
	sfvalue(p) = fabs( sfvalue( sfaram1(p) ) );
	return sfaram1(p);
};

//const eval
void sfcPI( sfNumber *cnst ) 
{ 
	*cnst=4*atan(1); 
};

void sfcPI2( sfNumber *cnst )
{ 
	*cnst=2*atan(1); 
};

void sfc2PI( sfNumber *cnst ) 
{ 
	*cnst=8*atan(1); 
};

void sfcE( sfNumber *cnst ) 
{
	*cnst=exp(1); 
};

void sfcEPS( sfNumber *cnst ) 
{ 
	*cnst=sfEPSILON; 
};

void sfcRND( sfNumber *cnst ) 
{ 
	*cnst=(double)rand()/(double)RAND_MAX; 
};

#ifdef __cplusplus
 }
#endif

#endif
