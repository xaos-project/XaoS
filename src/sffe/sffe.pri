DEFINES += SFFE_USING SFFE_CMPLX_ASM

SOURCES += \
    $$PWD/sffe.c \
    $$PWD/sffe_cmplx_asm.c \
    $$PWD/sffe_cmplx_gsl.c

ASM_SOURCES += \
    $$PWD/asm/cmplx.asm

nasm.input = ASM_SOURCES
nasm.output = $$PWD/${QMAKE_FILE_BASE}.o

win32 {
    nasm.commands = nasm -f coff -o $$PWD/${QMAKE_FILE_BASE}.o ${QMAKE_FILE_NAME}
} else:macx {
    nasm.commands = nasm -f macho -o $$PWD/${QMAKE_FILE_BASE}.o ${QMAKE_FILE_NAME}
} else {
    nasm.commands = nasm -f elf -o $$PWD/${QMAKE_FILE_BASE}.o ${QMAKE_FILE_NAME}
}


QMAKE_EXTRA_COMPILERS += nasm
