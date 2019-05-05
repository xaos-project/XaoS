# -------------------------------------------------
# Project created by QtCreator 2009-10-29T19:21:55
# -------------------------------------------------
TEMPLATE = app

QT += opengl

macx {
    TARGET = XaoS
} else {
    TARGET = xaos
}

CONFIG(debug, debug|release) {
    DEFINES += DEBUG
    win32:CONFIG += console
}

DESTDIR = $$PWD/bin

include($$PWD/src/engine/engine.pri)
include($$PWD/src/filter/filter.pri)
include($$PWD/src/ui/ui.pri)
include($$PWD/src/ui-hlp/ui-hlp.pri)
include($$PWD/src/util/util.pri)
include($$PWD/src/sffe/sffe.pri)
include($$PWD/src/ui/ui-drv/qt/qt.pri)
include($$PWD/src/include/include.pri)
