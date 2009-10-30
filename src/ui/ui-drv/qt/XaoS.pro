# -------------------------------------------------
# Project created by QtCreator 2009-10-29T19:21:55
# -------------------------------------------------
TARGET = XaoS
TEMPLATE = app
INCLUDEPATH = ../../../include \
    .
SOURCES += ../../../engine/formulas.c \
    ../../../engine/fractal.c \
    ../../../engine/btrace.c \
    ../../../engine/palettef.c \
    ../../../engine/emboss.c \
    ../../../engine/star.c \
    ../../../engine/anti.c \
    ../../../engine/dither.c \
    ../../../engine/edge.c \
    ../../../engine/edge2.c \
    ../../../engine/rotate.c \
    ../../../engine/zoom.c \
    ../../../engine/blur.c \
    ../../../engine/interlace.c \
    ../../../engine/itersmall.c \
    ../../../engine/stereogram.c \
    ../../../engine/3d.c \
    ../../../engine/subwindow.c \
    ../../../engine/plane.c \
    ../../../engine/julia.c \
    ../../../engine/i386.c \
    ../../../filter/image.c \
    ../../../filter/palette.c \
    ../../../filter/random.c \
    ../../../filter/grlib.c \
    ../../../filter/font.c \
    ../../../filter/filter.c \
    ../../../ui/drivers.c \
    ../../../ui/ui.c \
    ../../../ui/uihelp.c \
    ../../../ui/param.c \
    ../../../ui/fparams.c \
    ../../../ui/filesel.c \
    ../../../ui/uimenu.c \
    ../../../ui/pipecmd.c \
    ../../../ui/dialog.c \
    ../../../ui-hlp/autopilot.c \
    ../../../ui-hlp/ui_helper.c \
    ../../../ui-hlp/menu.c \
    ../../../ui-hlp/messg.c \
    ../../../ui-hlp/play.c \
    ../../../ui-hlp/render.c \
    ../../../ui-hlp/playtext.c \
    ../../../ui-hlp/save.c \
    ../../../ui-hlp/wstack.c \
    ../../../util/png.c \
    ../../../util/catalog.c \
    ../../../util/thread.c \
    ../../../util/xstring.c \
    ../../../util/help.c \
    ../../../util/xerror.c \
    ../../../util/xshl.c \
    ../../../util/xldio.c \
    ../../../util/xstdio.c \
    ../../../util/xmenu.c \
    ../../../util/timers.c \
    main.cpp \
    mainwindow.cpp \
    fractalwidget.cpp
HEADERS += mainwindow.h \
    config.h \
    aconfig.h \
    version.h \
    fractalwidget.h
