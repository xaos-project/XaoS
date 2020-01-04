# -------------------------------------------------
# Project created by QtCreator 2009-10-29T19:21:55
# -------------------------------------------------

lessThan(QT_MAJOR_VERSION, 5): error("requires Qt >= 5")
lessThan(QT_MINOR_VERSION, 6): error("requires Qt >= 5.6")

TEMPLATE = app

QT += opengl

win32 {
    LIBS += -lopengl32
    DEFINES += USE_OPENGL
}

macx {
    TARGET = XaoS
    DEFINES += USE_OPENGL
} else {
    TARGET = xaos
}

linux {
    contains(QMAKE_HOST.arch, arm.*): { }
    else {
        DEFINES += USE_OPENGL
    }
}

CONFIG(debug, debug|release) {
    DEFINES += DEBUG
    win32:CONFIG += console
}

RESOURCES += XaoS.qrc

DESTDIR = $$PWD/bin

include($$PWD/i18n/i18n.pri)
include($$PWD/src/include/include.pri)
include($$PWD/src/ui/ui.pri)
include($$PWD/src/engine/engine.pri)
include($$PWD/src/ui-hlp/ui-hlp.pri)
include($$PWD/src/util/util.pri)
include($$PWD/src/sffe/sffe.pri)
