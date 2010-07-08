/*/////////////////////////////////////////////////////////////////////////////////////
// project : sFFe ( SegFault (or Segmentation Fault :) ) formula evalutaor )
// author  : Mateusz Malczak ( mateusz@malczak.info )
// wpage   : www.segfaultlabs.com/projects/sffe
///////////////////////////////////////////////////////////////////////////////////////
// possible config definitions
//   general
//	SFFE_DEVEL - print extra info to stdout
//	SFFE_DLL - Windows DLL
//   complex numbers
//	SFFE_CMPLX_GSL - uses GSL complex number routines
//	SFFE_CMPLX_ASM - uses my asm complex unit (compile it with NASM)
///////////////////////////////////////////////////////////////////////////////////////
// special build for XaoS, for more info visit
// http://www.segfaultlabs.com/projects/sfXaos
/////////////////////////////////////////////////////////////////////////////////////*/

#ifndef SFFE_H
#define SFFE_H
#include <config.h>
#include <stdlib.h>

/* dont udef next line. removing this definitions can lead to compile and/or runtime errors */
#define SFFE_COMPLEX 1

/* --- */
/*TODO long double needed*/
#ifdef SFFE_CMPLX_ASM
typedef struct cmpx__ {
    double r, i;
} cmplx;
#define sfNumber 		cmplx
#elif SFFE_CMPLX_GSL
#include <gsl/gsl_complex.h>
typedef gsl_complex cmplx;
#define sfNumber 		gsl_complex
#endif

/* basic sffe argument 'stack' */
typedef struct sfargument__ {
    struct sfargument__ *parg;
    sfNumber *value;
} sfarg;

/* sffe function prototype, parameters order is right-to-left (cdecl) */
typedef sfarg *(*sffptr) (sfarg * const a);

/* constats eval functions */
typedef void (*cfptr) (sfNumber * cnst);

/* function type structure */
typedef struct sffunction__ {
    sffptr fptr;
    unsigned char parcnt;
    /*FIXME changed from char* to char[20] to get rid of warnings during compilation */
    char name[20];
} sffunction;

/* basic sffe 'stack' operation ( function + result slot ) */
typedef struct sfoperation__ {
    sfarg *arg;
    sffptr f;
} sfopr;

/* SFFE main structure */
typedef struct sffe__ {
/*public*/
    char *expression;		/* parsed expression (read-only) */
    char *errormsg;		/* parser errors (read-only) */
    sfNumber *result;		/* evaluation result (read-only) */
/* protected/private */
    unsigned int argCount;	/* number of argument in use */
    sfarg *args;
    unsigned int oprCount;	/* number of operations in use */
    sfopr *oprs;
    unsigned int varCount;	/* number of used variables */
    char *varChars;
    sfNumber **varPtrs;
    unsigned int userfCount;	/* number of user functions */
    sffunction *userf;
/* not used 
  unsigned int	stCount;	    
  sfNumber*		statics;
*/
} sffe;

#define SFFE sffe
#define sffeparser sffe
#define sfparser sffe
#define SFFEPARSER sffe

/* 'stack' slot value */
#define sfvalue(p) (*((p)->value))

/* function parameters */
#define sfaram1(p) ((p)->parg)
#define sfaram2(p) ((p)->parg->parg)
#define sfaram3(p) ((p)->parg->parg->parg)
#define sfaram4(p) ((p)->parg->parg->parg->parg)
#define sfaram5(p) ((p)->parg->parg->parg->parg->parg)
/* and so on */


#ifdef __cplusplus
extern "C" {
#endif

/* create formula evaluator structure */
sffe *sffe_alloc(void);
/* free fe structure */
void sffe_free(sffe ** parser);
/* parse expression 'expression' and strore result in 'parser' struct, error (if any) returned */
int sffe_parse(sffe ** parser, char *expression);
/* evaulate function and return evaluation result */
sfNumber sffe_eval(sffe * const parser);
/* evaluate without returnig result */
//void sffe_eval2(sffe *const parser);
/* register user function with name 'vname', with 'parcnt' parameters and defined with function pointed by 'funptr'*/
void *sffe_regfunc(sffe ** parser, char *vname, unsigned int parcnt,
                   sffptr funptr);
/* register single variable 'vptrs' identified by name 'vchars' */
void *sffe_regvar(sffe ** parser, sfNumber * vptrs, char vchars);
/* register multiple variables */
void *sffe_regvars(sffe ** parser, unsigned int cN, sfNumber ** vptrs,
                   char *vchars);
/* get variable 'vchar' pointer */
sfNumber *sffe_varptr(sffe * const parser, char vchar);
/* set 'vptrs' as 'vchars' variable  */
sfNumber *sffe_setvar(sffe ** parser, sfNumber * vptrs, char vchars);

#ifdef __cplusplus
}
#endif
#ifdef SFFE_CMPLX_ASM
#include "sffe_cmplx_asm.h"
#elif SFFE_CMPLX_GSL
#include "sffe_cmplx_gsl.h"
#endif
#endif
