#ifndef CONFIG_H
#define CONFIG_H
#define HOMEDIR

#ifdef __BEOS__
#define MAIN_FUNCTION be_main
#ifdef __POWERPC__
#  define SLOWCACHESYNC
#  ifdef __MWERKS__
#    define INLINEFABS(x) __fabs(x)
#  endif
#endif
#endif

#ifdef _WIN32
#define CONFIGFILE "XaoS.cfg"
#else
#define CONFIGFILE ".XaoSrc"
#endif

/*For compilers that don't support nameless unions, do a
#define NONAMELESSUNION
before #include <ddraw.h>*/
#ifdef _WIN32
#define NONAMELESSUNION
#endif

/*#define I_WAS_HERE */      /*uncoment this to disable changing of parameters by atoconf */

#define FPOINT_TYPE  long double
				       /*floating point math type on computers
				          with medium speed floating point math should   
				          use float instead */
#include <aconfig.h>
#define USE_STDIO
#if !defined(HAVE_LONG_DOUBLE)&&!defined(I_WAS_HERE)
#undef FPOINT_TYPE
#define FPOINT_TYPE double
#endif
#define CONST const
#define INLINE inline

/* BeOS have broken long double IO routines on i386. Use our replacements */
#ifdef __BEOS__
#ifdef __i386__
#define USE_XLDIO
#endif
#endif

/* Win32 don't support long double IO. Use our replacements if possible */
#ifdef _WIN32
#ifndef __GNUC__
1
/* You need to solve long double IO problems to compile XaoS on non-gcc
 * compiler */
#endif
#define USE_XLDIO
#endif

#include "gccaccel.h"
#ifdef HAVE_MOUSEMASK
#define NCURSESMOUSE
#endif
#ifndef HAVE_LIMITS_H
#define INT_MAX 2127423647
#endif
#ifdef SVGA_DRIVER
#define DESTICKY
#endif
#ifdef X11_DRIVER
#define SFIXEDCOLOR
#define STRUECOLOR
#define STRUECOLOR16
#define STRUECOLOR24
#define SMBITMAPS
#define SLBITMAPS
#endif
#ifdef SVGA_DRIVER
#undef STRUECOLOR16
#define STRUECOLOR16
#undef STRUECOLOR24
#define STRUECOLOR24
#endif
#ifdef GGI_DRIVER
#undef STRUECOLOR16
#define STRUECOLOR16
#undef STRUECOLOR24
#define STRUECOLOR24
#undef SLBITMAPS
#define SLBITMAPS
#endif
#ifdef BEOS_DRIVER
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __GNUC__
void be_exit_xaos(int i) __attribute__ ((__noreturn__));
#else
void be_exit_xaos(int i);
#endif
#ifdef __cplusplus
}
#endif
#define exit_xaos(i) be_exit_xaos(i)
#undef STRUECOLOR16
#define STRUECOLOR16
#undef SFIXEDCOLOR
#define SFIXEDCOLOR
#undef SMBITMAPS
#define SMBITMAPS
#undef SLBITMAPS
#define SLBITMAPS
#endif
#ifdef WIN32_DRIVER
#define MAIN_FUNCTION XaoS_main
#undef STRUECOLOR16
#define STRUECOLOR16
#undef STRUECOLOR24
#define STRUECOLOR24
#endif
#ifdef HAVE_SELECT
#define COMPILE_PIPE
#endif
#endif				/*CONFIG_H */

