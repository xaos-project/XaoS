#ifdef cpixel_t
#undef UNSUPPORTED
#undef cpixel_t
#undef cpixeldata_t
#undef cppixel_t
#undef bpp
#undef bpp1
#endif
#define cpixel_t pixel32_t
#define cpixeldata_t pixel32_t
#define cppixel_t ppixel32_t
#define bpp 4
#include <generic.h>
