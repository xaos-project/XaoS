#ifndef I18N_H
#define I18N_H
#include "aconfig.h"
#if defined(HAVE_GETTEXT)
#include <libintl.h>
#include <locale.h>
#elif defined (QT_GETTEXT)
#include "qt_gettext.h"
#define gettext(STRING) qt_gettext(STRING)
#else
#define gettext(STRING) STRING
#endif
#define _(STRING) gettext(STRING)
#endif
