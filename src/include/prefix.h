#ifndef __BEOS__

#ifndef _MAC
#define _MAC
#endif

// These options should match the settings in the MW Pref panel.
//#define MAC68K_INT_SIZE               4
//#define MAC68K_DFLOAT_SIZE    10


#include <MacHeaders.h>

#define NDEBUG			// Comment out to enable debugging.

#else // __BEOS__

// Surprise!  The BeBox looks almost like a Mac :-)

// You need a precompiled BeHeaders file.
#include <BeHeaders>
#define myfabs(x) ((x)>0?(x):-(x))

#define NDEBUG

#endif // __BEOS__
