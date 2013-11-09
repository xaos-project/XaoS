AC_INIT([XaoS],[3.6],[xaos-devel@lists.sourceforge.net])

AC_PREREQ(2.63)
AC_CONFIG_HEADER(src/include/aconfig.h src/include/version.h)

AC_CANONICAL_HOST
# Force compilation without cygwin dependency
#case $host_os in
# *cygwin*) CC='gcc -mno-cygwin' ;;
#esac

AC_PROG_CC

dnl
dnl GNU gettext related things
dnl

AM_ICONV
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.18.1)
ALL_LINGUAS="cs es hu fr de ro it pt"
if test "x$USE_NLS" = xyes; then
  AC_PATH_PROG([MSGMERGE], [msgmerge], , [$PATH:../bin])
  if test "x$ac_cv_path_MSGMERGE" = x; then
    AC_MSG_ERROR([Install the msgmerge utility first for i18n
                  or re-configure XaoS with --disable-nls])
  fi
  if test "x$POFILES" = x; then
    for lang in $ALL_LINGUAS; do
      GMOFILES="$GMOFILES $lang.gmo"
      POFILES="$POFILES $lang.po"
    done
  fi
fi

dnl
dnl drivers to test (if available)
dnl

x11_linked=no
x_driver_test=yes
gtk_driver_test=no
aa_driver_test=no
win32_driver_test=yes
long_double=yes
pthread=no
png=yes
xshm=yes
gsl=yes
sffe=yes

dnl
dnl Long double is a way to archieve deeper zooms. It is very fast
dnl at intel processors familly but seems to be very slow at almost
dnl all other platforms. This switch tries to determine whether long_double
dnl is optimal
dnl
AC_ARG_WITH(long-double,
        [  --with-long-double=yes/no   enables/disables usage of long double],[
	if test x$withval = xyes; then
	  long_double=yes
	fi
	if test x$withval = xno; then
	  long_double=no
	fi
])
AC_ARG_WITH(mitshm,
        [  --with-mitshm=yes/no    enables/disables MIT shared memory support],[
	if test x$withval = xyes; then
	  xshm=yes
	fi
	if test x$withval = xno; then
	  xshm=no
	fi
])
AC_ARG_WITH(pthread,
        [  --with-pthread=yes      enables SMP support (experimental)],[
	if test x$withval = xyes; then
	  pthread=test
	fi
	if test x$withval = xno; then
	  pthread=no
	fi
])
AC_ARG_WITH(png,
        [  --with-png=yes/no       enables/disables PNG saving support],[
	if test x$withval = xyes; then
	  png=test
	fi
	if test x$withval = xno; then
	  png=no
	fi
])
AC_ARG_WITH(gsl,
	[  --with-gsl=yes/no	enables/disables GSL complex routines],[
	if test x$withval = xyes; then
	  gsl=yes
	fi
	if test x$withval = xno; then
	  gsl=no
	fi
])
AC_ARG_WITH(sffe,
	[  --with-sffe=yes/no	enables/disables SFFE (user formula evaluator) support],[
	if test x$withval = xyes; then
	  sffe=yes
	fi
	if test x$withval = xno; then
	  sffe=no
	fi
])


	

AC_DEFUN([AC_PROG_AALIB_VER_OK],
[AC_CACHE_CHECK(whether aalib version >= 1.4, ac_cv_aalib_ver_ok,
[echo '#include <aalib.h>
#include <aalib.h>
void f(){ 
#if AA_LIB_VERSIONCODE < 104000
#error aa_lib is probably buggy 
/* I am using SuSE 7.3 and 8.0. Both contains version 1.3,
 * however they report 102000 version code.
 */ 
#endif 
}' > conftest.c
if test -z "`${CC-cc} -c conftest.c 2>&1`"; then
    ac_cv_aalib_ver_ok=yes
    else
        ac_cv_aalib_ver_ok=no
    fi
	rm -f conftest*
])])

AC_PROG_MAKE_SET
AC_PROG_RANLIB

dnl CFLAGS="-O"
dnl Checks for programs.

DRIVERDIRS=""
LOCALEDIR="/usr/share/locale"
DRIVERLIBS="lib/libui.a lib/libui-hlp.a lib/libengine.a lib/libutil.a lib/libfilter.a"
AC_PROG_INSTALL

AC_ARG_WITH(x11-driver,
        [  --with-x11-driver=no    disables x11 driver],[
	  x_driver_test=$withval
])
AC_ARG_WITH(gtk-driver,
        [  --with-gtk-driver=no    disables GTK driver],[
	  gtk_driver_test=$withval
])
AC_ARG_WITH(win32-driver,
        [  --with-win32-driver=no  disables Win32 driver],[
	  win32_driver_test=$withval
])
AC_ARG_WITH(aa-driver,
        [  --with-aa-driver=no     disables aa-lib driver],[
	  aa_driver_test=$withval
])

if test "$x_driver_test" = yes -o "$aa_driver_test" = yes; then
  AC_PATH_XTRA
fi


if test  "$win32_driver_test" = yes; then
  win32_driver=no
  case $host_os in
    *mingw* | *cygwin*) win32_driver=yes
    LIBS="$LIBS  -lgdi32  -lmsvcrt -luser32 -lversion -lmingwex"
    CFLAGS="$CFLAGS -mwindows"
    ;;
  esac
fi

AH_TEMPLATE(USE_PTHREAD, [Define to use posix threads])
if test x$pthread = xtest; then
  AC_CHECK_FUNCS(pthread_create)
  if test "x$ac_cv_func_pthread_create" = xyes; then
    AC_DEFINE(USE_PTHREAD)
    CFLAGS="$CFLAGS -D_REENTRANT"
    AC_CHECK_FUNCS(pthread_sighandler)
  else
  AC_CHECK_LIB(pthread,pthread_create)
  if test "x$ac_cv_lib_pthread_pthread_create" = xyes; then
    AC_DEFINE(USE_PTHREAD)
    CFLAGS="$CFLAGS -D_REENTRANT"
    AC_CHECK_FUNCS(pthread_sighandler)
  else
    echo "No pthread library available! Cannot compile threaded version"
    exit
  fi
  fi
fi

AC_DEFUN([AC_AALIB], [
    if $search_aalib
    then
        if test -f $1/aalib.h
        then
            LIBS1="$LIBS"
            if test "x$no_x" = x; then 
	      LIBS="$LIBS $X_EXTRA_LIBS $X_LIBS -lX11"
	    fi
            LIBS="$LIBS $2"
            unset ac_cv_lib_aa_aa_init
            AC_CHECK_LIB(aa, aa_init)
            if test "x$ac_cv_lib_aa_aa_init" = xyes; then
              AC_MSG_RESULT(Found aalib on $1/aalib.h)
              AC_PROG_AALIB_VER_OK
 	      if test $ac_cv_aalib_ver_ok = yes; then
               #LIBS="$LIBS $2"
	       x11_linked=yes
               CFLAGS="$CFLAGS $3"
               search_aalib=false
               aa_driver=true
              else
               unset ac_cv_lib_aa_aa_init
               LIBS="$LIBS1"
              fi
            else
             unset ac_cv_lib_aa_aa_init
             LIBS="$LIBS1"
            fi
        fi
    fi
])


if test "$aa_driver_test" = yes; then
    OLDLIBS="$LIBS"
    OLDCFLAGS="$CFLAGS"

    search_aalib=true;
    if test "x$no_x" = x; then
       LIBS="$LIBS $X_EXTRA_LIBS $X_LIBS"
       CFLAGS="$CFLAGS $X_CFLAGS"
    fi
    AC_AALIB(/usr/include, , )
    AC_AALIB(/usr/local/include, -L/usr/local/lib, -I/usr/local/include)
    for name in ../aalib-*.*
    do
      AC_AALIB($name,"-L$name","-I$name")
    done
    if $search_aalib
    then
      aa_driver=false
      LIBS="$OLDLIBS"
      CFLAGS="$OLDCFLAGS"
    else
      aa_driver=true
    fi
fi


dnl Checks for header files.

AC_CHECK_HEADERS([alloca.h ddraw.h htmlhelp.h errno.h fcntl.h float.h libintl.h limits.h locale.h malloc.h stddef.h stdlib.h string.h sys/time.h sys/timeb.h unistd.h libgen.h dirent.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
if test "x$long_double" = xyes; then
AC_TYPE_LONG_DOUBLE  
else
echo Long double disabled, since it would be too slow on this CPU
fi
AC_HEADER_TIME
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_TYPE_PID_T
AC_TYPE_SIZE_T
dnl AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_STRUCT_TM

test "x$prefix" = xNONE && prefix=$ac_default_prefix
eval datadir2=$datadir
eval datadir3=$datadir2
# This is a madness! eval can evaluate only one level of variable...
AH_TEMPLATE(DATAPATH, [Define path to XaoS data files])
AC_DEFINE_UNQUOTED(DATAPATH,"${datadir3}/XaoS")

AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)

dnl Checks for library functions.
dnl AC_FUNC_ERROR_AT_LINE
dnl AC_FUNC_FORK
dnl AC_FUNC_MALLOC
dnl AC_FUNC_MEMCMP
dnl AC_FUNC_STAT
dnl AC_FUNC_VPRINTF

AC_CHECK_LIB(m, pow)

AH_TEMPLATE(USE_PNG, [Define to use libpng for image saving])
if test $png = yes; then
  AC_CHECK_LIB(z, inflate)
  if test x"$ac_cv_lib_z_inflate" = xno; then
   AC_CHECK_LIB(gz, inflate)
   if test x"$ac_cv_lib_gz_inflate" = xno; then
    png=no
   fi
  fi
  AC_CHECK_LIB(png, png_create_info_struct)
  if test x"$ac_cv_lib_png_png_create_info_struct" = xno; then
    png=no
  fi
  if test "$png" = yes; then
    AC_DEFINE(USE_PNG)
  fi
fi
AC_FUNC_ALLOCA
AC_CHECK_FUNCS([ftime getcwd gettimeofday memchr memmove memset pow select setlocale sqrt strdup strerror strstr sleep setitimer ftime finite fabsl _fabsl __fabsl])

AH_TEMPLATE(HAVE_MITSHM_H, [Define if mitshm.h is present])
AH_TEMPLATE(HAVE_IPC_H, [Define if ipc.h is present])
if test  "$x_driver_test" = yes; then
dnl X checks
if test "x$no_x" = x; then 
  AC_CHECK_HEADER(sys/ipc.h, AC_DEFINE(HAVE_IPC_H), no_sys_ipc=yes)
  AC_CHECK_HEADER(sys/shm.h, AC_DEFINE(HAVE_IPC_H), no_sys_shm=yes)

  if test "x$xshm" = xyes; then
  AC_MSG_CHECKING(X11/extensions/XShm.h)
  if eval "test -f $x_includes/X11/extensions/XShm.h"; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_MITSHM_H)
  else
    AC_MSG_RESULT(no)
    xshm=no
  fi
  fi

  dnl AC_CHECK_LIB(Xext,XShmEventBase,LIBS="$LIBS -lXext", no_ext_lib=yes, $X_LIBS)
  AC_CHECK_LIB(Xext,XShmAttach,LIBS="$LIBS -lXext", no_ext_lib=yes, $X_LIBS )

fi
fi

if test  "$gtk_driver_test" = yes; then
  PKG_CHECK_MODULES([DEPS], gtk+-2.0 >= 2.2 glib-2.0 >= 2.2, gtk_driver=true, gtk_driver=false)
  AC_SUBST(DEPS_CFLAGS)
  AC_SUBST(DEPS_LIBS)
fi

AH_TEMPLATE(NO_MALLOC_H, [Define on systems without malloc.h])
if test "$ac_cv_header_malloc_h" = "no"; then
 AC_DEFINE(NO_MALLOC_H)
fi

AH_TEMPLATE(HTML_HELP, [Define to use Microsoft HTML Help Library])
if test "$ac_cv_header_htmlhelp_h" = "yes"; then
 AC_DEFINE(HTML_HELP)
 LIBS="$LIBS -lhtmlhelp"
fi

AH_TEMPLATE(DDRAW_DRIVER, [Define to compile DirectDraw driver])
if test "$ac_cv_header_ddraw_h" = "yes"; then
 AC_DEFINE(DDRAW_DRIVER)
fi


echo "------------------------------------------------------------"
echo "Available drivers:"
drivers=no
AH_TEMPLATE(AA_DRIVER, [Define to build aalib driver])
if test "$aa_driver_test" = yes; then
if test "x$aa_driver" = xtrue; then
  drivers=yes
  AC_DEFINE(AA_DRIVER)
  DRIVERDIRS="$DRIVERDIRS ui/ui-drv/aa"
  DRIVERLIBS="$DRIVERLIBS lib/libuiaa.a"
  echo "   aa-lib"
fi
fi

AH_TEMPLATE(GTK_DRIVER, [Define to build GTK+ driver])
if test "$gtk_driver_test" = yes; then
if test "x$gtk_driver" = xtrue; then
  drivers=yes
  AC_DEFINE(GTK_DRIVER)
  DRIVERDIRS="$DRIVERDIRS ui/ui-drv/gtk"
  DRIVERLIBS="$DRIVERLIBS lib/libuigtk.a"
  CFLAGS="$CFLAGS $DEPS_CFLAGS"
  LIBS="$LIBS $DEPS_LIBS"
  echo "   gtk+"
fi
fi

AH_TEMPLATE(X11_DRIVER, [Define to build X11 driver])
AH_TEMPLATE(MITSHM, [Define to use MIT shared memory extensions])
if test  "$x_driver_test" = yes; then
if test "x$no_x" = x; then 
  drivers=yes
  AC_DEFINE(X11_DRIVER)
  DRIVERDIRS="$DRIVERDIRS ui/ui-drv/x11"
  DRIVERLIBS="$DRIVERLIBS lib/libuix11.a"
  if test "x$no_sys_ipc$no_sys_shm y$ac_cv_lib_Xext_XShmAttach z$xshm" = "x yyes zyes"; then
    echo "   X11       with MIT shared memory support"
    AC_DEFINE(MITSHM)
  else
    echo "   X11"
  fi
  if test "$x11_linked" = no; then 
    LIBS="$LIBS $X_EXTRA_LIBS $X_LIBS -lX11"
    x11_linked=yes
  fi
  CFLAGS="$CFLAGS $X_CFLAGS"
fi
fi

AH_TEMPLATE(WIN32_DRIVER, [Define to build Windows driver])
if test  "$win32_driver" = yes; then
  drivers=yes
  AC_DEFINE(WIN32_DRIVER)
  DRIVERDIRS="$DRIVERDIRS ui/ui-drv/win32"
  DRIVERLIBS="ui/ui-drv/win32/ui_win32.o lib/libuiwin32.a ui/ui-drv/win32/xaos.res $DRIVERLIBS"
  LOCALEDIR="../../locale"
  echo -n "   Win32"
  if test "$ac_cv_header_ddraw_h" = yes; then
   echo " (with DirectDraw support)"
  else
   echo ""
  fi 
fi

REALTOPDIR=`nolinks=1; pwd`
AC_SUBST(REALTOPDIR)
BINPATH="$REALTOPDIR/bin"
AC_SUBST(BINPATH)
AR="ar"
AC_SUBST(AR)
SRCPATH="$REALTOPDIR/src"
AC_SUBST(SRCPATH)
LIBPATH="$SRCPATH/lib"
AC_SUBST(LIBPATH)
# does this work if you do make -C <some dir>
TOPDIR=`nolinks=1;pwd`
AC_SUBST(TOPDIR)
AC_SUBST(LOCALEDIR)
# sffe cmplx detection (flow is : GLS -> sffeASM -> none)
if test $sffe = yes; then
 echo "------------------------------------------------------------" 
 echo "Complex math parser:"
 echo "try GSL: $gsl"
 
 if test $gsl = yes; then
  GSL_LIBS=""
  AC_PATH_PROG(GSL_CONFIG, gsl-config, no)
  AC_MSG_CHECKING(for GSL)
  if test "$GSL_CONFIG" = "no" ; then
    gsl=no
    AC_MSG_RESULT(no)
  else
    AC_MSG_RESULT(yes)
    echo "using GSL for complex numbers"
    GSL_LIBS=`$GSL_CONFIG --libs`
    GSL_CFLAGS=`$GSL_CONFIG --cflags`
    CFLAGS="$CFLAGS $GSL_CFLAGS -fomit-frame-pointer -DSFFE_USING -DSFFE_CMPLX_GSL"
    DRIVERLIBS="$DRIVERLIBS lib/libsffe.a"
    ASM_CMPLX_O=""
  fi 
 fi

 if test $gsl = no; then 
  AC_CHECK_PROG([NASM],[nasm],[yes])
  if test "$NASM" = yes; then
    echo "using NASM for complex numbers"
    CFLAGS="$CFLAGS -DSFFE_USING -DSFFE_CMPLX_ASM"
    DRIVERLIBS="$DRIVERLIBS lib/libsffe.a"
    ASM_CMPLX_O="asm/cmplx.o"
    case $host_os in
      *cygwin* | *mingw*) CMPLX_O_TARGET=coff;;
      *darwin*) CMPLX_O_TARGET=macho;;
      *) CMPLX_O_TARGET=elf;;
    esac
  else
    sffe=no
  fi
 fi
 echo "------------------------------------------------------------"
else
	CFLAGS="$CFLAGS -fomit-frame-pointer"
fi


LIBS="$LIBS $GSL_LIBS"
CFLAGS="$CFLAGS -I${SRCPATH}/include"
AC_SUBST(DRIVERLIBS)
AC_SUBST(DRIVERDIRS)
AC_SUBST(CMPLX_O_TARGET)
AC_SUBST(ASM_CMPLX_O)
AC_SUBST(POFILES)

AC_CONFIG_FILES([src/i18n/Makefile.in
    Makefile src/Makefile src/i18n/Makefile src/engine/Makefile 
    src/util/Makefile src/ui/Makefile src/filter/Makefile src/ui-hlp/Makefile
    src/ui/ui-drv/aa/Makefile
    src/ui/ui-drv/x11/Makefile src/ui/ui-drv/win32/Makefile
    doc/Makefile help/Makefile src/ui/ui-drv/gtk/Makefile src/sffe/Makefile
  ])
AC_OUTPUT

echo "------------------------------------------------------------"
echo "Compiler options:"
echo " CC          : $CC"
echo " CFLAGS      : $CFLAGS"
echo " LDLAGS      : $LDLAGS"
echo " LIBS        : $LIBS"
echo " X_LIBS      : $X_LIBS"
echo "------------------------------------------------------------"
echo " "
echo "READ THE FOLLOWING WARNINGS CAREFULLY"
echo " "
if test "x$ac_cv_header_stdc" = xno; then
  AC_MSG_WARN(There are no ANSI C headers avaiable. XaoS will have troubles with compiling.)
  config_problem=yes
fi
if test "x$ac_cv_c_compiler_gnu" = xyes; then
  echo 
else
  AC_MSG_WARN(The GNU C compiler is recommended to get the best performance. XaoS crashes many compilers so your compiler can cause troubles.)
fi
if test "x$ac_cv_c_long_double" = xyes; then
  if test "x$long_double" = xyes; then
  AC_MSG_WARN([Long double enabled. This works well for x86 architectures. On other systems this may slow down much. Especially on SUN workstations. If XaoS works incredibly slowly, try to rebuild it using:
   configure --with-long-double=no
   make clean
   make])
  fi
fi
if test "x$drivers" = xno; then
  AC_MSG_WARN(There is no driver available. XaoS will not start.)
  config_problem=yes
fi
if test "x$config_problem" = xyes; then
  AC_MSG_WARN([There was a problem with automatic configuration.])
  AC_MSG_WARN([Read INSTALL file for troubleshooting tips.])
fi
if test x"$ac_cv_lib_z_inflate" = xno; then
 if test x"$ac_cv_lib_gz_inflate" = xno; then
  AC_MSG_WARN([You need zlib to compile XaoS with image saving support.])
 fi
fi
if test x"$ac_cv_lib_png_png_create_info_struct" = xno; then
  AC_MSG_WARN([You need pnglib to compile XaoS with image saving support.])
fi
if test x"$sffe" = xno; then
  AC_MSG_WARN([You need GSL or NASM to comple XaoS with user formulas.])
fi
echo " "
echo "END OF WARNINGS"
echo " "
echo "In case you have trouble with compiling or running"
echo "XaoS please read the INSTALL file."
echo "------------------------------------------------------------"

