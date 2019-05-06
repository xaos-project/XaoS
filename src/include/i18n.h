#ifndef I18N_H
#define I18N_H
#include "aconfig.h"

#if defined(HAVE_GETTEXT)
// Use gettext library
#include <libintl.h>
#include <locale.h>

#elif defined (QT_GETTEXT)
// Emulate gettext call using Qt's translation framework
#define bindtextdomain(x, y) (y)
#define textdomain(x)
const char *qt_gettext(const char *text);
#define gettext(STRING) qt_gettext(STRING)

#else
// No I18N available, just pass original string
#define gettext(STRING) STRING
#endif

// Define _ as alias for gettext
#define _(STRING) gettext(STRING)

#endif
