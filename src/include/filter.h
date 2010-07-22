#ifndef IMAGE_H
#define IMAGE_H
#include "fconfig.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char pixel_t;
typedef unsigned char rgb_t[4];	/*4 is better than 3 - makes multiplying easier */
struct truec {
    int rshift, gshift, bshift;	/*the shift ammounts */
    int rprec, gprec, bprec;	/*precisity - 0=8bit, 1=7bit, -1=9bit etc... */
    unsigned int rmask, gmask, bmask;	/*masks */
    unsigned int mask1, mask2, allmask;	/*Mask1 and mask2 are distinc color masks
                                               allmask are mask for all colors */
    int byteexact;		/*When every colors is at one byte */
    int missingbyte;	/*for 32bit truecolor and exact byte places one byte is
                               unused... */
};
union paletteinfo {
    struct truec truec;
};
struct palette {
    int start;
    int end;
    int maxentries;
    int version;
    int type;
    unsigned int *pixels;
    int npreallocated;
    rgb_t *rgb;
    int flags;
    int (*alloccolor) (struct palette * pal, int init, int r, int g,
                       int b);
    void (*setpalette) (struct palette * pal, int start, int end,
                        rgb_t * rgb);
    void (*allocfinished) (struct palette * pal);
    void (*cyclecolors) (struct palette * pal, int direction);
    int size;		/*number of allocated color entries */
    void *data;		/*userdata */
    /*Preallocated palette cells */
    int ncells;
    unsigned int *index;
    const rgb_t *prergb;
    union paletteinfo info;
};

struct image_driver;

struct image {
    float pixelwidth, pixelheight;
    pixel_t **oldlines;
    pixel_t **currlines;
    void (*flip) (struct image * img);
    int width, height, nimages;
    int bytesperpixel;
    int currimage;
    int flags;
    int scanline;
    int version;
    struct palette *palette;
    void *data;		/*userdata */
    struct image_driver *driver;
};

struct image_driver {
    int (*print) (struct image *image, int x, int y,
                const char *text, int fgcolor, int bgcolor, int mode);
    int (*textwidth) (struct image *image, const char *text);
    int (*textheight) (struct image *image);
    int (*charwidth) (struct image *image, const char c);
    const char * (*saveimage) (struct image *image, const char *filename);
};

#define interpol1(i1,i2,n,mask) ((((i1)&(mask))*(n)+((i2)&(mask))*(256-(n)))&((mask)<<8))
#define interpol(i1,i2,n,mr,mg,mb) ((interpol1(i1,i2,n,mr)+interpol1(i1,i2,n,mg)+interpol1(i1,i2,n,mb))>>8)
#define intergray(i1,i2,n) (((i1)*n+(i2)*(256-(n)))>>8)
/*
 * J.B. Langston 3/13/2008
 *
 * The Mac OS X driver requires a 32-bit rgb mask where the most significant
 * byte is on (e.g., 0xffffff00). This exposed a bug in the interpol macro
 * that resulted in distorted colors for the smooth coloring modes.
 * If the interpol macro is applied to such a mask, it causes an overflow
 * of the 32-bit int, and the left-most color byte is lost.
 *
 * I added shiftinterpol macro to handle such masks. It shifts everything 1
 * byte to the right, performs the calculation, and then shifts everything
 * back 1 byte to the left when it is done.
 *
 * I also created the safeinterpol macro which detects if the most
 * signficant byte in the mask is on, and uses the shiftinterpol macro if
 * so, or the orignal interpol macro if not.
 *
 * I then modified the interpoltype macro to use the safeinterpol macro
 * instead of the interpol macro directly.
 */
#define shiftinterpol(i1,i2,n,mr,mg,mb) (interpol((i1)>>8,(i2)>>8,n,(mr)>>8,(mg)>>8,(mb)>>8)<<8)
#define safeinterpol(i1,i2,n,mr,mg,mb) ((((mr)|(mg)|(mb))&0xff000000)?shiftinterpol(i1,i2,n,mr,mg,mb):interpol(i1,i2,n,mr,mg,mb))
#define interpoltype(palette,i1,i2,n) ((palette).type==GRAYSCALE || (palette).type == LARGEITER?intergray(i1,i2,n):safeinterpol(i1,i2,n,(palette).info.truec.rmask,(palette).info.truec.gmask,(palette).info.truec.bmask))
/*palette flags */
#define UNKNOWNENTRIES 1
#define DONOTCHANGE 2
#define FINISHLATER 4
#define UNFINISHED 8
/*image flags */
#define FREELINES 1
#define FREEDATA 2
#define AAIMAGE 4
#define PROTECTBUFFERS 8
/*palette types supported by most of engine*/
#define C256 1
#define GRAYSCALE 2
#define TRUECOLOR16 4
#define TRUECOLOR24 8
#define TRUECOLOR 16
/*special mage types used internaly by XaoS */
#define LARGEITER 32
#define SMALLITER 64

/*palette types handled by the dithering filter*/
#define LBITMAP 256
#define MBITMAP 512
#define LIBITMAP 1024
#define MIBITMAP 2048
#define FIXEDCOLOR 4096

#define ALLMASK (C256|TRUECOLOR16|TRUECOLOR24|LARGEITER|GRAYSCALE)
#define BITMAPS (LBITMAP|MBITMAP|LIBITMAP|MIBITMAP)
#define MASK1BPP (SMALLITER|C256|GRAYSCALE)
#define MASK2BPP (TRUECOLOR16|LARGEITER)
#define MASK3BPP (TRUECOLOR24)
#define MASK4BPP (TRUECOLOR)

/*flags for requirements */
#define IMAGEDATA 1
#define TOUCHIMAGE 2
#define NEWIMAGE 4
/*flags for initdata */
#define DATALOST 1
/*flags for doit */
#define INTERRUPTIBLE 1
#define PALETTEONLY 2
/*return flags */
#define INEXACT 1
#define CHANGED 2
#define ANIMATION 4
#define UNCOMPLETTE (1<<29)
/*flags for filters */
#define ALLOCEDIMAGE 1		/*used by inherimage mechanizm */
#define SHAREDDATA 2

#define PALGORITHMS 3
#ifdef _plan9_
#undef pixel32_t
#undef pixel8_t
#undef pixel16_t
#define pixel32_t unsigned int
#define pixel16_t unsigned short
#define pixel8_t unsigned char
#undef ppixel8_t
#undef ppixel16_t
#undef ppixel24_t
#undef ppixel32_t
#define ppixel8_t pixel8_t *
#define ppixel16_t pixel16_t *
#define ppixel24_t unsigned char *
#define ppixel32_t pixel32_t *
#else
#include <pixel_t.h>		/*avoid problems with plan9-it ignores #if
                               So code must be separated into another file */
#endif
#define imgetpixel(image,x,y) ((image)->bytesperpixel==1?(image)->currlines[y][x]:((image)->bytesperpixel==4?((pixel32_t*)(image)->currlines[y])[x]:(image)->bytesperpixel==3?(((pixel16_t *)(image)->currlines[y])[x]+((image)->currlines[y][3*(x)+2]<<16)):(((pixel16_t*)(image)->currlines[y])[x])))
struct requirements {
    int nimages;
    int supportedmask;
    int flags;
};
struct filter {
    struct filter *next, *previous;
    struct queue *queue;
    const struct filteraction *action;
    struct image *image, *childimage;
    struct requirements req;
    struct fractal_context *fractalc;
    void *data;
    const char *name;
    int flags;
    int imageversion;	/*For detection whether image changed or not */
    void (*wait_function) (struct filter * f);
    /*stuff for wait_function */
    int pos, max, incalculation, readyforinterrupt, interrupt;
    const char *pass;
};
struct initdata {
    void (*wait_function) (struct filter * f);
    struct image *image;
    struct fractal_context *fractalc;
    int flags;
};
struct filteraction {
    const char *name;
    const char *shortname;
    int flags;
    struct filter *(*getinstance) (const struct filteraction * a);
    void (*destroyinstance) (struct filter * f);
    int (*doit) (struct filter * f, int flags, int time);
    int (*requirement) (struct filter * f, struct requirements * r);
    int (*initialize) (struct filter * f, struct initdata * i);
    void (*convertup) (struct filter * f, int *x, int *y);
    void (*convertdown) (struct filter * f, int *x, int *y);
    void (*removefilter) (struct filter * f);
};
struct queue {
    struct filter *first, *last;
    int isinitialized;
    struct filter *palettechg;
    struct image *saveimage;
};


#define datalost(f,i) (((i)->flags&DATALOST)||((f)->imageversion&&(f)->imageversion!=(i)->image->version))
/*filter actions */

extern unsigned int col_diff[3][512];
struct filter *createfilter(const struct filteraction *fa);
struct queue *create_queue(struct filter *f);
void insertfilter(struct filter *f1, struct filter *f2);
void removefilter(struct filter *f);
void addfilter(struct filter *f1, struct filter *f2);
int initqueue(struct queue *q);

/*Filter utility functions */
int reqimage(struct filter *f, struct requirements *req,
             int supportedmask, int flags);
int inherimage(struct filter *f, struct initdata *data, int flags,
               int width, int height, struct palette *palette,
               float pixelwidth, float pixelheight);
void destroyinheredimage(struct filter *f);
void updateinheredimage(struct filter *f);

void inhermisc(struct filter *f, const struct initdata *i);

/*image actions */

void flipgeneric(struct image *img);
struct image *create_image_lines(int width, int height,
                                 int nimages, pixel_t ** lines1,
                                 pixel_t ** lines2,
                                 struct palette *palette,
                                 void (*flip) (struct image * img),
                                 int flags, float pixelwidth,
                                 float pixelheight);
struct image *create_image_cont(int width, int height,
                                int scanlinesize, int nimages,
                                pixel_t * buf1, pixel_t * buf2,
                                struct palette *palette,
                                void (*flip) (struct image * img),
                                int flags, float pixelwidth,
                                float pixelheight);
struct image *create_image_mem(int width, int height, int nimages,
                               struct palette *palette,
                               float pixelwidth, float pixelheight);
struct image *create_subimage(struct image *simg, int width,
                              int height, int nimages,
                              struct palette *palette,
                              float pixelwidth, float pixelheight);

void destroy_image(struct image *img);
void clear_image(struct image *img);

/*palette */

int bytesperpixel(int type) CONSTF;
void bestfit_init(void);
struct palette *createpalette(int start, int end, int type, int flags,
                              int maxentries,
                              int (*alloccolor) (struct palette * pal,
                                                 int init, int r,
                                                 int g, int b),
                              void (*setcolor) (struct palette * pal,
                                                int start, int end,
                                                rgb_t * rgb),
                              void (*allocfinished) (struct palette *
                                                     pal),
                              void (*cyclecolors) (struct palette *
                                                   pal, int direction),
                              union paletteinfo *info);
void destroypalette(struct palette *palette);
int mkdefaultpalette(struct palette *palette);
int mkstereogrampalette(struct palette *palette);
int mkstarfieldpalette(struct palette *palette);
int mkblurpalette(struct palette *palette);
int mkgraypalette(struct palette *palette);
int mkrgb(struct palette *palette);
int mkpalette(struct palette *palette, int seed, int algorithm);
int shiftpalette(struct palette *palette, int n);
void preallocpalette(struct palette *pal);
struct palette *clonepalette(struct palette *palette);
void restorepalette(struct palette *dest, struct palette *src);
void convertupgeneric(struct filter *f, int *x, int *y);
void convertdowngeneric(struct filter *f, int *x, int *y);
int fixedalloccolor(struct palette *palette, int init, int r, int g,
                    int b) CONSTF;

#define setfractalpalette(f,p) if((f)->fractalc->palette==(f)->image->palette) (f)->fractalc->palette=(p)

#ifdef STRUECOLOR24
#define TRUECOLOR24CASE(x) case 3:x;break;
#else
#define TRUECOLOR24CASE(x)
#endif

#ifdef STRUECOLOR16
#define SUPPORT16
#endif
#ifdef SUPPORT16
#define TRUECOLOR16CASE(x) case 2:x;break;
#else
#define TRUECOLOR16CASE(x)
#endif

#define drivercall(i,x1,x2,x3,x4) switch((i).bytesperpixel) { \
TRUECOLOR24CASE(x3); \
        TRUECOLOR16CASE(x2); \
case 1:x1;break; \
case 4:x4; \
}
#ifdef SMBITMAPS
#define SBITMAPS
#else
#ifdef SLBITMAPS
#define SBITMAPS
#endif
#endif

#ifdef SBITMAPS
#define SCONVERTORS
#else
#ifdef SFIXEDCOLOR
#define SCONVERTORS
#endif
#endif

#ifdef __cplusplus
}
#endif
#include "formulas.h"
#endif
