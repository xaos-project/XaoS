#ifndef I18N_H
#define I18N_H

const char *qt_gettext(const char *text);

#define gettext(STRING) qt_gettext(STRING)
#define _(STRING) qt_gettext(STRING)
#endif
