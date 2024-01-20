#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include "config.h"
#include "xerror.h"
/*On windows we use message boxes done in the ui_win32.c code*/
void x_message(const char *text, ...)
{
    va_list ap;
    va_start(ap, text);
    vfprintf(stdout, text, ap);
    fprintf(stdout, "\n");
    va_end(ap);
}

void x_error(const char *text, ...)
{
    va_list ap;
    va_start(ap, text);
    vfprintf(stderr, text, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void x_fatalerror(const char *text, ...)
{
    va_list ap;
    va_start(ap, text);
    vfprintf(stderr, text, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}
