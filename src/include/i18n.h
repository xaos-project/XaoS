#ifndef I18N_H
#define I18N_H

const char *qt_gettext(const char *context, const char *text);

#define TR(context, text) qt_gettext(context, text)
#endif
