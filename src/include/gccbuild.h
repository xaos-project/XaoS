/* BUGS:
 * bellow gcc 2.7.2 builtins are not supported
 * gcc 2.7.2.x don't inline long doubles
 * egcc 1.0.1 crash in constant propagation when certain builtins are enabled
 */


#ifndef __GCCBUILD
#define __GCCBUILD
#if defined(__GLIBC__)
#include "math.h"
#else
#if defined(__GNUC__) && defined(__i386__) && defined(__OPTIMIZE__)
#if 0
#include "i386/__math.h"
#endif
#endif
#endif

/* We really want to use builtins. Avoid any defines */
#if defined __GNUC__ && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7))
#define __HAVE_FABSL
#define __fabsl(x) __builtin_fabsl(x)
#define __fabs(x) __builtin_fabsl(x)
#define __fabsf(x) __builtin_fabsf(x)
#define sin(x) __builtin_sin(x)
#define cos(x) __builtin_cos(x)
#endif


#include "aconfig.h"

#ifndef __HAVE_FABSL
#ifdef HAVE_FABSL
#define __fabsl fabsl
#define __HAVE_FABSL
#else
#ifdef HAVE__FABSL
#define __fabsl _fabsl
#define __HAVE_FABSL
#else
#ifdef HAVE___FABSL
#define __HAVE___FABSL
#endif
#endif
#endif
#endif

/* GCC 2.7.x have problems with long double inlines. Disable them! */
#if defined __GNUC__ && (__GNUC__ == 2 && __GNUC_MINOR__ <= 7)
#undef __HAVE_FABSL
#endif

#ifdef HAVE_LONG_DOUBLE
#ifndef __HAVE_FABSL
#define myabs(x) ((x)>0?(x):-(x))
#else
#define myabs(x) __fabsl((number_t)(x))
#endif
#else
#define myabs(x) fabs((number_t)(x))
#endif


#endif /*gccbuild */
