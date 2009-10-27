# -------------------------------------------------
# Project created by QtCreator 2009-08-13T18:42:55
# -------------------------------------------------
TARGET = XaoS
TEMPLATE = app
SOURCES += main.cpp \
    fractalwidget.cpp \
    misc.c \
    ../engine/formulas.c \
    ../engine/fractal.c \
    ../engine/btrace.c \
    ../engine/palettef.c \
    ../engine/emboss.c \
    ../engine/star.c \
    ../engine/anti.c \
    ../engine/dither.c \
    ../engine/edge.c \
    ../engine/edge2.c \
    ../engine/rotate.c \
    ../engine/zoom.c \
    ../engine/blur.c \
    ../engine/interlace.c \
    ../engine/itersmall.c \
    ../engine/stereogram.c \
    ../engine/3d.c \
    ../engine/subwindow.c \
    ../engine/plane.c \
    ../engine/julia.c \
    ../engine/i386.c \
    ../filter/image.c \
    ../filter/palette.c \
    ../filter/random.c \
    ../filter/grlib.c \
    ../filter/font.c \
    ../filter/filter.c \
    ../ui-hlp/autopilot.c \
    ../ui-hlp/ui_helper.c \
    ../ui-hlp/menu.c \
    ../ui-hlp/play.c \
    ../ui-hlp/render.c \
    ../ui-hlp/playtext.c \
    ../ui-hlp/save.c \
    ../ui-hlp/wstack.c \
    ../util/png.c \
    ../util/catalog.c \
    ../util/thread.c \
    ../util/xstring.c \
    ../util/help.c \
    ../util/xerror.c \
    ../util/xshl.c \
    ../util/xldio.c \
    ../util/xstdio.c \
    ../util/xmenu.c \
    ../util/timers.c \
    mainwindow.cpp \
    fractalparameters.cpp \
    fractalmodel.cpp \
    abstractfractalmodel.cpp
HEADERS += fractalwidget.h \
    mainwindow.h \
    fractalparameters.h \
    fractalmodel.h \
    abstractfractalmodel.h
INCLUDEPATH = ../include
FORMS += fractalparameters.ui
