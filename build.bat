@echo off
cls
echo Welcome to XaoS building script
echo.
echo To build XaoS you need:
echo.
echo o Djgpp 2.0 or later (gcc-GNU c compiler)
echo o Make utility
echo o Allegro graphics library version 2.2+WIP or later
echo   allegro.h/liballeg.a in your searching path
echo o Djp executable file packer (archive mlp*.zip at simtel)
echo o PNG library
echo o Zlib
echo o An aalib graphics library
echo o Libraries text and vga required by aalib
echo o Gettext and libiconv for internationalization (optional)
echo.
echo See file INSTALL.DOS if you get into trouble.
echo.
echo Binary version also available at XaoS homepage.
echo.
echo Press CTRL+C to interrupt or enter to continue.
echo.
pause >nul
cd src
make -f Makefile.dos
cd ..
