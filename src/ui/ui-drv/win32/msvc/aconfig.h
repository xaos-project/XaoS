/* src/include/aconfig.h.  Generated from aconfig.h.in by configure.  */
#ifndef ACONFIG_H
#define ACONFIG_H
/* #undef HAVE_PTHREAD_SIGHANDLER */
#define DATAPATH "/usr/local/share/XaoS"
/* #undef C_ALLOCA */
/* #undef const */
/* #undef USE_PTHREAD */
/*Avoid stack frame explosion on Windoze*/
#ifndef _WIN32
/*BeOS crashes badly when large amounts of stack are consumed */
#ifndef __BEOS__
#define HAVE_ALLOCA 1
/* #undef HAVE_ALLOCA_H */
#endif
#endif
#define HAVE_FABSL 1
/* #undef HAVE__FABSL */
/* #undef HAVE___FABSL */
#define HAVE_FTIME 1
/* #undef USE_PNG */
#define HAVE_FINITE 1
/* #undef HAVE_SELECT */
//#define HAVE_LONG_DOUBLE 1
/* #undef HAVE_REDRAWWIN */
/* #undef HAVE_WREDRAWLN */
/* #undef USE_NCURSES */
/* #undef inline */
/* #undef _POSIX_SOURCE */
#define STDC_HEADERS 1
/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1
#define HAVE_GETTIMEOFDAY 1
/* #undef HAVE_USLEEP */
/* #undef HAVE_TERMATTRS */
/* #undef HAVE_MOUSEMASK */
/* #undef HAVE_SETITIMER */
/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1
/* Define if you have the <sys/time.h> header file.  */
//#define HAVE_SYS_TIME_H 1
/* Define if you have the <unistd.h> header file.  */
//#define HAVE_UNISTD_H 1
/* #undef MITSHM */
/* #undef CURSES_DRIVER */
/* #undef BEOS_DRIVER */
/* #undef AA_DRIVER */
/* #undef GTK_DRIVER */
/* #undef GGI_DRIVER */
/* #undef X11_DRIVER */
/* #undef DGA_DRIVER */
/* #undef SVGA_DRIVER */
#define WIN32_DRIVER 1
/* #undef DDRAW_DRIVER */
/* #undef HTML_HELP */
#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
/* #undef HAVE_GETTEXT */
/* #undef NO_MALLOC_H */
#endif
