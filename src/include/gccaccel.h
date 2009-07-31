/* GNU C accelerators */
#if defined (__GNUC__)
#define myfabs(x) fabs(x)
#if __GNUC__ >= 2 && (__GNUC__ > 2 || __GNUC_MINOR__ >= 95)
#define __GNUC__EGCS
#endif
#if __GNUC__ >= 2 && (__GNUC__ > 2 || __GNUC_MINOR__ > 7)
#if __GNUC__ >= 2 && (__GNUC__ > 2 || __GNUC_MINOR__ >= 95)
#define RESTRICT __restrict__
#endif
#define CONSTF __attribute__ ((__const__))
#if __GNUC__ >= 3
#define PUREF __attribute__ ((__pure__))
#endif
#define NORETURN __attribute__ ((__noreturn__))
#ifdef __i386__
#ifndef NOREGISTERS
#define REGISTERS(n) __attribute__ ((__regparm__(n)))
#endif /*NOREGISTERS*/
#endif /*__i386__ */
#endif				/*version */
#endif /*__GNUC__*/
#ifndef RESTRICT
#define RESTRICT
#endif
#ifndef CONSTF
#define CONSTF
#endif
#ifndef PUREF
#define PUREF
#endif
#ifndef NORETURN
#define NORETURN
#endif
#ifndef REGISTERS
#define REGISTERS(n)
#endif
#ifndef myfabs
#define myfabs(x) ((x)>0?(x):-(x))
#endif
