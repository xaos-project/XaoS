#ifndef I18N_H
#define I18N_H

#ifdef __cplusplus
extern "C" {
#endif

const char *qt_gettext(const char *text);

#ifdef __cplusplus
}
#endif

#define gettext(STRING) qt_gettext(STRING)
#define _(STRING) qt_gettext(STRING)
#endif
