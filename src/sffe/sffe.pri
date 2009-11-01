DEFINES += SFFE_USING SFFE_CMPLX_ASM

SOURCES += \
    $$PWD/sffe.c \
    $$PWD/sffe_cmplx_asm.c \
    $$PWD/sffe_cmplx_gsl.c

ASM_SOURCES += \
    $$PWD/asm/cmplx.asm

nasm.output = $$PWD/asm/${QMAKE_FILE_BASE}.o
nasm.commands = nasm -f coff ${QMAKE_FILE_NAME}
nasm.input = ASM_SOURCES

QMAKE_EXTRA_COMPILERS += nasm
