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
/* 2009-07-30 JB Langston
 * Undefining CONSTF qualifier under GCC 4.x to fix bug #8, where HSV coloring 
 * mode is completely black on Mac and Linux.
 * 
 * This qualifier causes the hsv_to_rgb function in formulas.c not to work 
 * under gcc 4.x--I'm not sure why. The Windows version of XaoS is still 
 * compiled with gcc 3.x and it works there even with CONSTF defined.  Under 
 * gdb, the code in this function gets skipped entirely when qualifier is used.   
 */
#if __GNUC__ < 4
#define CONSTF __attribute__ ((__const__))
#endif
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
