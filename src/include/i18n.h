#ifndef I18N_H
#define I18N_H

const char *qt_gettext(const char *context, const char *text);
void setLanguage(const char *lang);
const char *getLanguage();
const char *lang1(int i);
const char *lang2(int i);

#define TR(context, text) qt_gettext(context, text)
#endif
