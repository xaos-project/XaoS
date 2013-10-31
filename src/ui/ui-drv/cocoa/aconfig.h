/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright (C) 1996 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 *    Cocoa Driver by J.B. Langston III (jb-langston@austin.rr.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef ACONFIG_H
#define ACONFIG_H
/* #undef HAVE_PTHREAD_SIGHANDLER */


//#define DEBUG
//#define VIDEATOR_SUPPORT

/* Triggers Mac OS X-specific behavior in common sources */
#define MACOSX

/* Cocoa driver needs to do some pre-initialization in its own main function */
#define MAIN_FUNCTION XaoS_main

/* XaoS xio library uses \01 to indicate paths relative to the executable */
#define DATAPATH "\01/../Resources"
#define USE_LOCALEPATH 1

/* Using alloca causes stack overflows on large images */
/* #undef C_ALLOCA */
/* #define HAVE_ALLOCA 1 */
/* #define HAVE_ALLOCA_H 1 */

/* #undef const */
#define USE_PTHREAD 1

#define HAVE_FABSL 1
/* #undef HAVE__FABSL */
/* #undef HAVE___FABSL */
#define HAVE_FTIME 1
#define USE_PNG 1
#define HAVE_FINITE 1
#define HAVE_SELECT 1

/* Long double is too slow on PowerPC; only enable for i386 build */
#ifdef __i386__
#define HAVE_LONG_DOUBLE 1
#endif

/* #undef HAVE_REDRAWWIN */
/* #undef HAVE_WREDRAWLN */
#define USE_NCURSES 1
/* #undef inline */
/* #undef _POSIX_SOURCE */
#define STDC_HEADERS 1
/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1
#define HAVE_GETTIMEOFDAY 1
/* #undef HAVE_USLEEP */
/* #undef HAVE_TERMATTRS */
/* #undef HAVE_MOUSEMASK */
#define HAVE_SETITIMER 1
/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1
/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1
/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1
/* #define MITSHM 1 */
/* #undef CURSES_DRIVER */
/* #undef BEOS_DRIVER */
/* #undef AA_DRIVER */
/* #undef GGI_DRIVER */
/* #undef X11_DRIVER */
/* #undef DGA_DRIVER */
/* #undef SVGA_DRIVER */
/* #undef WIN32_DRIVER */
/* #undef DDRAW_DRIVER */
/* #undef OSX_DRIVER */
#define COCOA_DRIVER 1
#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define HAVE_GETTEXT 1
#define NO_MALLOC_H 1

/* Enable SFFE; use ASM for i386 and GSL for PowerPC */
#define SFFE_USING 1
#ifdef __i386__
#define SFFE_CMPLX_ASM 1
#else
#define SFFE_CMPLX_GSL 1
#endif

/* Define colors based on hardware endianness */
#if __BIG_ENDIAN__
#define RMASK 0xff0000
#define GMASK 0x00ff00
#define BMASK 0x0000ff
#else
#define RMASK 0x0000ff
#define GMASK 0x00ff00
#define BMASK 0xff0000
#endif

/* Use platform-provided text rendering instead of built-in */
#define PLATFORM_TEXT_RENDERING 1


#endif
