#ifdef cpixel_t
#undef cpixel_t
#undef cpixeldata_t
#undef cppixel_t
#undef bpp
#undef UNSUPPORTED
#undef bpp1
#endif
#ifndef SUPPORT16
#define UNSUPPORTED
#endif
#define cpixel_t pixel16_t
#define cpixeldata_t pixel16_t
#define cppixel_t ppixel16_t
#define bpp 4
#include <generic.h>
