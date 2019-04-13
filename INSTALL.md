How to Compile and Install XaoS
===============================

NOTE: This is the source code package for XaoS. You cannot use XaoS from 
this package until you compile it.  For end users, we recommend using a 
pre-built binary instead. 

If you would like to install a binary version for Windows or Mac OS X, 
download the appropriate package for your operating system from 
http://xaos.sf.net.

For Ubuntu and Debian users, type the following from a shell prompt to 
automatically install XaoS:

```
 sudo apt-get install xaos
```

For other GNU/Linux users, refer to your distribution's instructions for
installing packages from its repositories.  

Compilation from source is only necessary if:

 * you are a packager for an operating system distribution,
 * you want to try new features which are not yet included in the
   binary packages available for your distribution,
 * you want to try some changes/enhancements on XaoS source code.

You should be comfortable compiling software and working on a Unix command
line before attempting these instructions.
  


Unix-Like Systems
=================

Build Requirements
------------------

XaoS should compile on most modern Unix-like systems.  It has been most
recently tested on Ubuntu 9.04.

The dependencies are broken down by the functionality that requires them. If 
you are not trying to compile an official binary and only want to test or
develop a certain subset of XaoS's features, you can skip the dependencies
that are not relevant to those features.

To compile XaoS you need:

 * A modern version of gcc (3.x or higher) from http://gcc.gnu.org/
 * Other optimizing C compilers may work but have not been tested

To build the X11 UI, you need: 

 * Xlib library and headers from http://www.x.org/.

For the GTK UI you need:

 * GTK+ 2.x (tested with 2.16) from http://www.gtk.org/.

XaoS can display fractals as high quality ASCII art. For this, you need:

 * AA-lib 1.4 or higher from http://aa-project.sf.net

To compile XaoS with support for saving images you need:

 * zlib from http://www.zlib.net/
 * libpng from http://www.libpng.org/pub/png/libpng.html

To use internationalization (i18n) you need:

 * gettext from http://www.gnu.org/software/gettext/
 * libiconv from http://www.gnu.org/software/libiconv/

To support user-defined formulas, you need ONE of the following:

 * nasm from http://www.nasm.us/
 * gsl from http://www.gnu.org/software/gsl/

 NOTE: Nasm will only work on x86 architectures, but GSL should work on any
       modern platform.

These dependencies may be available pre-compiled through your operating
system's package repository.  See your operating system's documentation for
more details.

NOTE: Many GNU/Linux distributions separate the run-time and development
components of their libraries into different packages.  If you install
these dependencies from your distribution's repository, please make sure to
install the development packages as well (usually marked -dev or -devel).

Users of Ubuntu and Debian can automatically install all the necessary 
build dependencies from above by typing:

```
 sudo apt-get build-dep xaos
```

However, GTK+ is not configured as a build dependency yet, so you will
need to install it manually by typing:

```
 sudo apt-get install libgtk2.0-dev
```


Building XaoS
-------------

To build XaoS, type the following commands from the XaoS source directory:

```
 ./configure
 make
```

XaoS will automatically detect which dependencies you have installed and
configure itself to support whatever features are available.  If you find
that some feature is missing, please verify that you have installed the
corresponding dependency (including the developer libraries).

Configure will try to choose the best optimization switches for your
architecture, but it may not work well for less common ones.  You can 
provide optimization flags by specifying them before running configure:

```
 CFLAGS=(your best optimizing switches)
 export CFLAGS
```

You can also customize what optional features to build by passing options
to the configure script.

- To enable experimental SMP support use `--with-pthread=yes`
- To enable the experimental GTK UI, use: `--with-gtk-driver=yes --with-x11-driver=no`
- To enable the aa-lib driver, use: `--with-aa-driver=yes`

Run `./configure --help` for a full list of options.

Once you have successfully compiled XaoS, type the following to install it:

```
 sudo make install
```


For Win32
=========

Build Requirements
------------------

XaoS supports Windows XP or later.  To compile a fully-featured XaoS binary
on Windows, you need the following dependencies.

The dependencies are broken down by the functionality that requires them. If 
you are not trying to compile an official binary and only want to test or
develop a certain subset of XaoS's features, you can skip the dependencies
that are not relevant to those features.

- To compile XaoS, you need MSYS2 from https://www.msys2.org/
- To build the XaoS installer you need NSIS from http://nsis.sourceforge.net/


Preparation
-----------

Install the latest version of MSYS2 and follow the instructions to update it. 

Start the "MSYS2 MinGW 32-bit" shell and install GCC and XaoS build dependencies:

```
pacman -S make nasm mingw-w64-i686-gcc mingw-w64-i686-libpng
```

Extract the XaoS source files:

```
tar xfz XaoS-$VERSION.tar.gz   # replace $VERSION with actual version
```

Or, check out the latest sources from GitHub:

```
git clone https://github.com/xaos-project/XaoS.git
```

NOTE: If your Windows username contains spaces, do not extract the sources in
your home directory.  Instead create a directory called, for example, /build
and extract them there.



Building XaoS
-------------

To build XaoS, type the following commands from the XaoS source root:

```
./configure
make
```

XaoS will automatically detect what dependencies you have installed and
configure itself to support whatever features are available.  If you find
that some feature is missing, please verify that you have installed the
corresponding dependency installed.

To install XaoS to a staging directory, type the following commands:

```
export DESTDIR=<installdir>
make install-win
```

where `<installdir>` should be the absolute path of the folder in which XaoS 
should be installed.

To build the XaoS installer, install XaoS to
`XaoS/src/ui/ui-drv/win32/installer/XaoS`. Use NSIS to compile the script XaoS
Installer.nsi in the directory `XaoS/src/ui/ui-drv/win32/installer`.


Mac OS X
========

Build Dependencies
------------------

XaoS runs on Mac OS X 10.4 or later but compilation has only been tested 
recently on 10.5.

Xcode 3.0 or later is required to compile XaoS for Mac OS X. You can find 
it on the Developer Tools disc that came with your computer or download it
from: http://developer.apple.com/mac/

Pre-compiled binaries for additional Mac OS X third-party build dependencies
can be downloaded from: 
http://downloads.sourceforge.net/sourceforge/xaos/xaos-thirdparty-cocoa-20090714.tar.gz

Exract this file within the `src/ui/ui-drv/cocoa/thirdparty` directory of your
XaoS source distribution.

Alternatively, to compile third party dependencies from source, download:

 * libpng from http://www.libpng.org/pub/png/libpng.html
 * gettext from http://www.gnu.org/software/gettext/
 * gsl from http://www.gnu.org/software/gsl/

Extract each library and build them using the following commands:

```
 env CFLAGS="-O -g -isysroot /Developer/SDKs/MacOSX10.4u.sdk \
             -arch i386 -arch ppc -mmacosx-version-min=10.4" \
     LDFLAGS="-arch i386 -arch ppc" \
 ./configure --prefix=$XAOS_ROOT/src/ui/ui-drv/cocoa/thirdparty \
             --disable-dependency-tracking

 make
 make install
```

Be sure to replace $XAOS_ROOT with the root of your XaoS folder. This 
will install the libraries and headers in src/ui/ui-drv/cocoa/thirdparty
where the Xcode project will be able to find them.
	   
For more information about building open source libraries as universal 
binaries see: http://developer.apple.com/technotes/tn2005/tn2137.html
	

Building XaoS
-------------

The Mac OS X version of XaoS is not compiled using the configure scripts in
the root directory.  Instead, use the Xcode project located in:
`src/ui/ui-drv/cocoa/XaoS.xcodeproj`.

If the libraries were installed properly, you should be able to build XaoS
from the Xcode project.
