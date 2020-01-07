#ifndef IUTIL_H
#define IUTIL_H
#include "xio.h"
#include "config.h"
struct image;

const char *writepng(xio_constpath filename, const struct image *image);
void XaoS_srandom(unsigned int x);
long int XaoS_random(void);
char *mystrdup(const char *);

number_t xstrtonum(const char *s, char **sp);
#endif
