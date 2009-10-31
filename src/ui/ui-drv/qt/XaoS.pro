# -------------------------------------------------
# Project created by QtCreator 2009-10-29T19:21:55
# -------------------------------------------------
TARGET = xaos
DESTDIR = $$PWD/../../../../bin
TEMPLATE = app
INCLUDEPATH += $$PWD \
    $$PWD/../../../include
SOURCES += main.cpp \
    mainwindow.cpp \
    fractalwidget.cpp \
    customdialog.cpp
HEADERS += mainwindow.h \
    fractalwidget.h \
    customdialog.h
include($$PWD/../../../engine/engine.pri)
include($$PWD/../../../filter/filter.pri)
include($$PWD/../../../ui/ui.pri)
include($$PWD/../../../ui/ui-drv/qt/qt.pri)
include($$PWD/../../../ui-hlp/ui-hlp.pri)
include($$PWD/../../../util/util.pri)
include($$PWD/config.pri)
RESOURCES += XaoS.qrc
