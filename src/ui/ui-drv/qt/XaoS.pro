# -------------------------------------------------
# Project created by QtCreator 2009-10-29T19:21:55
# -------------------------------------------------
TARGET = XaoS
TEMPLATE = app
INCLUDEPATH = ../../../include \
    .
SOURCES += main.cpp \
    mainwindow.cpp \
    fractalwidget.cpp
HEADERS += mainwindow.h \
    fractalwidget.h
include($$PWD/../../../engine/engine.pri)
include($$PWD/../../../filter/filter.pri)
include($$PWD/../../../ui/ui.pri)
include($$PWD/../../../ui/ui-drv/qt/qt.pri)
include($$PWD/../../../ui-hlp/ui-hlp.pri)
include($$PWD/../../../util/util.pri)
include($$PWD/config.pri)
