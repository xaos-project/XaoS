#ifndef ACCEL_H
#define ACCEL_H
#ifdef __GNUC__
#ifdef __i386__
#ifdef __OPTIMIZE__
#include <i386/sstring.h>
#endif
#endif
#endif
#endif

#ifndef memset_long
#define memset_long(x,y,z) memset(x,y,z)
#endif
