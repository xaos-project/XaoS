#ifdef cpixel_t
#undef cpixel_t
#undef cpixeldata_t
#undef cppixel_t
#undef bpp
#endif
#define cpixel_t pixel8_t
#define cppixel_t ppixel8_t
#define cpixeldata_t pixel8_t
#define bpp 1
#include <generic.h>
#define bpp1
