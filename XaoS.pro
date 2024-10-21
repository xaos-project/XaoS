# -------------------------------------------------
# Project created by QtCreator 2009-10-29T19:21:55
# -------------------------------------------------

lessThan(QT_MAJOR_VERSION, 6): error("requires Qt >= 6")
wasm {
lessThan(QT_MINOR_VERSION, 5): error("requires Qt >= 6.5.2")
}

QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic

TEMPLATE = app

QT += widgets

macx {
    TARGET = XaoS
} else {
    TARGET = xaos
}

contains(DEFINES, USE_OPENGL) {
    QT += opengl
    win32:LIBS += -lopengl32
}

contains(DEFINES, USE_FLOAT128) {
    LIBS += -lquadmath
} else {
    DEFINES += USE_LONG_DOUBLE
}

CONFIG(debug, debug|release) {
    DEFINES += DEBUG
    win32:CONFIG += console
}

CONFIG(release, debug|release) {
    QMAKE_POST_LINK=$(STRIP) $(TARGET)
    linux: {
        # This may help in debugging some odd issues under Debian:
        # QMAKE_CFLAGS   *= $(shell dpkg-buildflags --get CFLAGS)
        # QMAKE_CXXFLAGS *= $(shell dpkg-buildflags --get CXXFLAGS)
        # QMAKE_LFLAGS   *= $(shell dpkg-buildflags --get LDFLAGS)
    }
}

isEmpty(QMAKE_LRELEASE) {
    win32 {
        QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
    } else {
        QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
    }
    unix {
        !exists($$QMAKE_LRELEASE) { QMAKE_LRELEASE = lrelease-qt5 }
        !exists($$QMAKE_LRELEASE) { QMAKE_LRELEASE = lrelease }
    } else {
        !exists($$QMAKE_LRELEASE) { QMAKE_LRELEASE = lrelease }
    }
}

CONFIG += optimize_full
CONFIG += c++11

QMAKE_CXXFLAGS += -ffast-math
QMAKE_CFLAGS += -ffast-math

QMAKE_CXXFLAGS += -fpermissive
RESOURCES += XaoS.qrc

DESTDIR = $$PWD/bin

include($$PWD/i18n/i18n.pri)
include($$PWD/src/include/include.pri)
include($$PWD/src/ui/ui.pri)
include($$PWD/src/engine/engine.pri)
include($$PWD/src/ui-hlp/ui-hlp.pri)
include($$PWD/src/util/util.pri)
include($$PWD/src/sffe/sffe.pri)

# Support "make install"
isEmpty(PREFIX) {
    PREFIX = /usr/local
    }
DEFINES += DATAPATH=\\\"$$PREFIX/share/XaoS\\\"
wasm{
    QMAKE_LFLAGS += --preload-file $$PWD/examples@$$DATAPATH/examples
    QMAKE_LFLAGS += --preload-file $$PWD/catalogs@$$DATAPATH/catalogs
    QMAKE_LFLAGS += --preload-file $$PWD/tutorial@$$DATAPATH/tutorial
    QMAKE_LFLAGS += -sASYNCIFY -Os # -sASYNCIFY can help avoiding to get the web application hang when the user presses "s".
    QMAKE_POST_LINK = $$PWD/tools/postprocess-web $$PWD $$PWD/bin
}
executable.files = bin/xaos
executable.path = $$PREFIX/bin
examples.path = $$PREFIX/share/XaoS/examples
examples.extra = find examples -name \'*.xpf\' -exec cp {} $(INSTALL_ROOT)$$PREFIX/share/XaoS/examples \;
catalogs.files = catalogs/*.cat
catalogs.path = $$PREFIX/share/XaoS/catalogs
tutorial.files = tutorial/*.x?f
tutorial.path = $$PREFIX/share/XaoS/tutorial
INSTALLS += executable examples catalogs tutorial
