#ifndef LIBINTL_H
#define LIBINTL_H
#define bindtextdomain(x, y) (y)
#define textdomain(x)
const char *qt_gettext(const char *text);
#endif
