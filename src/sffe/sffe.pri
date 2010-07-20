!win32-msvc2008 {
    DEFINES += SFFE_USING SFFE_CMPLX_ASM

    SOURCES += \
        $$PWD/sffe.c \
        $$PWD/sffe_cmplx_asm.c \
        $$PWD/sffe_cmplx_gsl.c \
        $$PWD/asm/cmplx.S
}
