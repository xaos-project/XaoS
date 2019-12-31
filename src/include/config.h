#ifndef CONFIG_H
#define CONFIG_H

#define XaoS_VERSION "4.0pre2"
#define HELP_URL "https://github.com/xaos-project/XaoS/wiki"
#define WEB_URL "http://xaos.sourceforge.net/"

#define HOMEDIR
#define DATAPATH "/usr/share/XaoS"
/* #undef USE_PTHREAD */
#define HAVE_FABSL 1
#define HAVE_FTIME 1
#ifndef _MSC_VER
#define HAVE_LONG_DOUBLE 1
#endif
#define HAVE_GETTIMEOFDAY 1
#ifndef _MSC_VER
#define HAVE_SYS_TIME_H 1
#endif
#ifndef _MSC_VER
#define HAVE_UNISTD_H 1
#endif
#define USE_OPENGL

#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 4

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

#define FPOINT_TYPE  long double
				       /*floating point math type on computers
				          with medium speed floating point math should   
				          use float instead */
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

#ifdef HAVE_MOUSEMASK
#define NCURSESMOUSE
#endif
#define SFIXEDCOLOR
#define STRUECOLOR
#define STRUECOLOR16
#define STRUECOLOR24
#define SMBITMAPS
#define SLBITMAPS
#ifdef HAVE_SELECT
#define COMPILE_PIPE
#endif
#endif				/*CONFIG_H */
