SOURCES += \
    $$PWD/ui.c \
    $$PWD/param.c \
    $$PWD/fparams.c \
    $$PWD/uimenu.c \
    $$PWD/pipecmd.c \
    $$PWD/main.cpp \
    $$PWD/mainwindow.cpp \
    $$PWD/fractalwidget.cpp \
    $$PWD/customdialog.cpp

HEADERS += \
    $$PWD/uiint.h \
    $$PWD/mainwindow.h \
    $$PWD/fractalwidget.h \
    $$PWD/customdialog.h

INCLUDEPATH += $$PWD

macx:ICON = $$PWD/XaoS.icns
win32:RC_FILE = $$PWD/xaos.rc
