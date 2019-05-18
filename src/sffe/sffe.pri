DEFINES += SFFE_USING
SOURCES += \
    $$PWD/sffe.c
contains(QT_ARCH, i386) {
    # use 32-bit intel assembly sources if possible
    DEFINES += SFFE_CMPLX_ASM
    SOURCES += \
        $$PWD/sffe_cmplx_asm.c \
        $$PWD/asm/cmplx.S
} else {
    # otherwise, fall back to GSL sources
    DEFINES += SFFE_CMPLX_GSL
    SOURCES += \
        $$PWD/sffe_cmplx_gsl.c \
        $$PWD/gsl_complex_math.c
}
