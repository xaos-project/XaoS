#ifndef I18N_H
#define I18N_H
#include "aconfig.h"

#ifdef QT_GETTEXT
// Emulate gettext call using Qt's translation framework
const char *qt_gettext(const char *text);
const char *qt_locale();
#define gettext(STRING) qt_gettext(STRING)
#else
// No I18N available, no-op gettext function
#define gettext(STRING) STRING
#endif
// Define _ as alias for gettext
#define _(STRING) gettext(STRING)
#endif
