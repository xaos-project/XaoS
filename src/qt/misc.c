#define SFFE_CMPLX_ASM

#include <config.h>
#include <sffe.h>
#include <xio.h>
#include <filter.h>
#include <ui_helper.h>

cmplx Z, C, pZ;
xio_pathdata configfile;

#ifdef _WIN32
void x_fatalerror(CONST char *text, ...)
{
}
void x_error(CONST char *text, ...)
{
}
void x_message(CONST char *text, ...)
{
}


#endif
