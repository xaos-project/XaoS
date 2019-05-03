/*/////////////////////////////////////////////////////////////////////////////////////
// project : sFFe ( SegFault (or Segmentation Fault :) ) formula evalutaor )
// author  : Mateusz Malczak ( mateusz@malczak.info )
// wpage   :
///////////////////////////////////////////////////////////////////////////////////////
// possible config definitions
//   general
//	SFFE_DOUBLE - real math parser
//	SFFE_COMPLEX - complex math parser
//	SFFE_DEVEL - print extra info to stdout
//	SFFE_DIRECT_FPTR - use direct function pointers (!!!) ommits payload
//	SFFE_DLL - Windows DLL
//	
//   complex numbers (for SFFE_COMPLEX)
//	SFFE_CMPLX_GSL - uses GSL complex number routines
//	SFFE_CMPLX_ASM - uses my asm complex unit (compile it with NASM)
/////////////////////////////////////////////////////////////////////////////////////*/

#ifndef SFFE_H
#define SFFE_H
#include <stdlib.h>

#define SFFE_DIRECT_FPTR 1

#ifdef SFFE_REAL
 #define SFFE_DOUBLE 1
#endif	

/* --- */
/*TODO long double needed*/
#ifdef SFFE_CMPLX_ASM
	#define SFFE_COMPLEX 1
	typedef struct cmpx__ {
    	double r, i;
	} cmplx;
	#define sfNumber 		cmplx
#elif SFFE_CMPLX_GSL
	#define SFFE_COMPLEX 1
	#include <gsl/gsl_complex.h>
	typedef gsl_complex cmplx;
	#define sfNumber 		gsl_complex
#elif SFFE_DOUBLE
	#define sfNumber		double	
#endif

typedef enum {
    sfvar_type_ptr,
    sfvar_type_managed_ptr
} sfvartype;


/* basic sffe argument 'stack' */
typedef struct sfargument__
{
    struct sfargument__ *parg;
    sfvartype type;
    sfNumber *value;
} sfarg;

/* sffe function prototype, parameters order is right-to-left (cdecl) */
typedef sfarg *(*sffptr)(sfarg * const a, void *payload);

/* constats eval functions */
typedef void (*cfptr) (sfNumber * cnst);

/* function type structure */
typedef struct
{
    sffptr fptr;
    unsigned char parcnt;
    /*FIXME changed from char* to char[20] to get rid of warnings during compilation */
    char *name;
    void *payload; // unmanaged opaque memory pointer
} sffunction;

/* basic sffe 'stack' operation ( function + result slot ) */
typedef struct
{
    sfarg*		arg;
#ifdef SFFE_DIRECT_FPTR
    sffptr fnc;
#else
    sffunction*	fnc;
#endif
} sfopr;

typedef struct
{
    char *name;
    sfvartype type;
    sfNumber *value;
} sfvariable;

typedef struct sfcontext__
{
    unsigned int funcsCount; /* number of default / user functions */
    sffunction *functions;
    
    unsigned int constsCount;
    cfptr *constants;
} sffe_context;

/* SFFE main structure */
typedef struct
{
/*public*/
    const char *expression;		/* parsed expression (read-only) */
    char *errormsg;         /* parser errors (read-only) */
    sfNumber *result;		/* evaluation result (read-only) */
    
/* protected/private */
    unsigned int argCount;	/* number of arguments in use */
    sfarg *args;
    
    unsigned int oprCount;	/* number of operations in use */
    sfopr *oprs;
    
    unsigned int varCount;	/* number of used variables */
    sfvariable *variables;
    
    unsigned int userfCount; /* number of user functions */
    sffunction *userf;
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
#define sfparamN(p,N) struct sfargument__ *r = p->parg; while((--N)>0) r = r->parg; return r;
/* and so on */


#ifdef __cplusplus
extern "C" {
#endif

/* create formula evaluator structure */
sffe *sffe_alloc(void);
/* free fe structure */
void sffe_free(sffe ** parser);
    
/* parse expression 'expression' and strore result in 'parser' struct, error (if any) returned */
int sffe_parse(sffe ** parser, const char *expression);
    
/* evaulate function and return evaluation result */
sfNumber sffe_eval(sffe * const parser);
    
/* register user function with name 'vname', with 'parcnt' parameters and defined with function pointed by 'funptr'*/
void* sffe_regfunc(sffe ** parser, const char *vname, unsigned int parcnt, sffptr funptr, void *payload);
    
/* get already registered variable pointer, NULL if variable was not registered */
sfvariable* sffe_var(sffe *const parser, const char* name);
    
/* register single variable 'vptrs' identified by name 'vchars' */
//void *sffe_regvar(sffe ** parser, sfNumber * vptrs, char vchars);
sfvariable* sffe_regvar(sffe ** parser, sfNumber * vptrs, const char* name);
    
/* register multiple variables */
void sffe_regvars(sffe ** parser, unsigned int cN, sfNumber ** vptrs, char* const* names);
    
//sffunction *sffe_function_alloc(char *name, sffptr function_pointer, unsigned char paramsCount, void *payload);

//void sffe_function_free(sffunction* function);
    
/* set 'vptrs' as 'vchars' variable  */
sfNumber* sffe_setvar(sffe ** parser, sfNumber vptrs, const char* name);

#ifdef __cplusplus
}
#endif

#ifdef SFFE_CMPLX_ASM
	#include "sffe_cmplx_asm.h"
#elif SFFE_CMPLX_GSL
	#include "sffe_cmplx_gsl.h"
#elif SFFE_DOUBLE
	#include "sffe_real.h"
#endif


#endif
