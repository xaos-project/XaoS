#ifndef X_LDIO_H
#define X_LDIO_H
#ifdef USE_XLDIO
#include "xio.h"
#ifdef __cplusplus
extern "C"
{
#endif
  void x_ldout (long double param, int prec, xio_file stream);
  long double x_strtold (CONST char *s, CONST char **sret);
#ifdef __cplusplus
}
#endif

#endif				/*USE_XLDIO */

#endif				/*X_LDIO_H */
