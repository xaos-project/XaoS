#if SIZEOF_INT==4
typedef unsigned int pixel32_t;
#elif SIZEOF_LONG==4
typedef unsigned long pixel32_t;
#else
/*#error define size for pixel32 please */
00
#endif
#if SIZEOF_SHORT==2
typedef unsigned short pixel16_t;
#elif SIZEOF_INT==2
typedef unsigned int pixel16_t;
#else
/*#error define size for pixel16 please */
  00
#endif
typedef unsigned char pixel8_t;
typedef unsigned char ppixel24_t[3];
typedef pixel8_t *ppixel8_t;
typedef pixel16_t *ppixel16_t;
typedef pixel32_t *ppixel32_t;
