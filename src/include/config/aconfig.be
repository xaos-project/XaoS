/* aconfig.h for the BeOS version.  */

#ifndef ACONFIG_H
#define ACONFIG_H

#define MAIN_FUNCTION be_main
#define DATAPATH "/boot/apps/XaoS"

#define NDEBUG

/* #define USE_PNG 1                          uncomment if you haven't got libpng & libz */
/* #define HAVE_ALLOCA 1              ### would like to, but stack overflows */
/* #define HAVE_ALLOCA_H 1            ### maybe later ... */
#define HAVE_FINITE 1
#define STDC_HEADERS 1
#define TIME_WITH_SYS_TIME 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_LIMITS_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_UNISTD_H 1

#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 4

#ifdef __POWERPC__
/* #  define SLOWFUNCPTR              ### causes mwcc to thrash :-( */
#  define SLOWCACHESYNC
#  ifdef __MWERKS__
#    define INLINEFABS(x) __fabs(x)
#  endif
#endif

#endif /* ACONFIG_H */
