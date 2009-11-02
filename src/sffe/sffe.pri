DEFINES += SFFE_USING SFFE_CMPLX_ASM

SOURCES += \
    $$PWD/sffe.c \
    $$PWD/sffe_cmplx_asm.c \
    $$PWD/sffe_cmplx_gsl.c

ASM_SOURCES += \
    $$PWD/asm/cmplx.asm

win32 {
    nasm.commands = nasm -f coff -o ${OBJECTS_DIR}${QMAKE_FILE_BASE}.o ${QMAKE_FILE_NAME}
} else:macx {
    nasm.commands = nasm -f macho -o ${OBJECTS_DIR}${QMAKE_FILE_BASE}.o ${QMAKE_FILE_NAME}
} else {
    nasm.commands = nasm -f elf -o ${OBJECTS_DIR}${QMAKE_FILE_BASE}.o ${QMAKE_FILE_NAME}
}

nasm.input = ASM_SOURCES
nasm.output = ${QMAKE_FILE_BASE}.o

QMAKE_EXTRA_COMPILERS += nasm
