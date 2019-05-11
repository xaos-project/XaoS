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

TRANSLATIONS = $$files($$PWD/i18n/*.po)
updateqm.input = TRANSLATIONS
updateqm.output = $$PWD/bin/XaoS_${QMAKE_FILE_BASE}.qm
updateqm.commands = lrelease ${QMAKE_FILE_NAME} -qm ${QMAKE_FILE_OUT}
updateqm.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += updateqm

DESTDIR = $$PWD/bin

include($$PWD/src/include/include.pri)
include($$PWD/src/ui/ui.pri)
include($$PWD/src/engine/engine.pri)
include($$PWD/src/filter/filter.pri)
include($$PWD/src/ui-hlp/ui-hlp.pri)
include($$PWD/src/util/util.pri)
#include($$PWD/src/sffe/sffe.pri)
