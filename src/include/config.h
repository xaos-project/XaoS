#ifndef CONFIG_H
#define CONFIG_H
#define HOMEDIR

#define HELP_URL "https://github.com/xaos-project/XaoS/wiki"
#define WEB_URL "http://xaos.sourceforge.net/"

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
#ifdef _MSC_VER
#define INLINE __inline
#else
#define INLINE inline
#endif

/* Win32 don't support long double IO. Use our replacements if possible */
#ifdef HAVE_LONG_DOUBLE
#ifdef _WIN32
#ifndef __GNUC__
1
/* You need to solve long double IO problems to compile XaoS on non-gcc
 * compiler */
#endif
#define USE_XLDIO
#endif
#endif

#include "gccaccel.h"
#ifdef HAVE_MOUSEMASK
#define NCURSESMOUSE
#endif
#ifdef QT_DRIVER
#define SFIXEDCOLOR
#define STRUECOLOR
#define STRUECOLOR16
#define STRUECOLOR24
#define SMBITMAPS
#define SLBITMAPS
#endif
#ifdef HAVE_SELECT
#define COMPILE_PIPE
#endif
#endif				/*CONFIG_H */
