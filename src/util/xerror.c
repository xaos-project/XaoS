#ifdef _plan9_
#include <u.h>
#include <libc.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "xerror.h"
#include "aconfig.h"
/*BeOS driver have its own routines in the C++ code */
#ifndef BEOS_DRIVER
/*On windows we use message boxes done in the ui_win32.c code*/
#ifndef WIN32_DRIVER
void
x_message (CONST char *text, ...)
{
  va_list ap;
  va_start (ap, text);
  vfprintf (stdout, text, ap);
  fprintf (stdout, "\n");
  va_end (ap);
}

void
x_error (CONST char *text, ...)
{
  va_list ap;
  va_start (ap, text);
  vfprintf (stderr, text, ap);
  fprintf (stderr, "\n");
  va_end (ap);
}

void
x_fatalerror (CONST char *text, ...)
{
  va_list ap;
  va_start (ap, text);
  vfprintf (stderr, text, ap);
  fprintf (stderr, "\n");
  va_end (ap);
  exit (1);
}
#endif
#endif
