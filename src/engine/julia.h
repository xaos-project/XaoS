#ifndef JULIA_H
#define JULIA_H
void init_julia (struct image *img, number_t rangep, number_t range,
		 number_t xdelta, number_t ystep);
#define SAG			/*solid anti-guessing */
#define NOT_CALCULATED (unsigned char)0
#define INSET (unsigned char)0
#define INPROCESS (unsigned char)255
#define RMIN -range
#define RMAX range
#define IMIN -range
#define IMAX range
#define QMAX 1000
#ifdef STATISTICS
extern int iters2, guessed2, unguessed2, total2, frames2;
#endif

#endif
