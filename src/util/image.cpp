#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdio>

#include "config.h"
#include "filter.h"
#include "fractal.h"
#include "ui_helper.h"
#include "grlib.h"
#include "c256.h"

void flipgeneric(struct image *img)
{
    pixel_t **line;
    assert(img->nimages == 2);
    img->currimage ^= 1;
    line = img->currlines;
    img->currlines = img->oldlines;
    img->oldlines = line;
}

int bytesperpixel(int type)
{
    switch (type) {
        case MBITMAP:
        case LBITMAP:
        case LIBITMAP:
        case MIBITMAP:
            return 0;
        case SMALLITER:
        case FIXEDCOLOR:
        case GRAYSCALE:
        case C256:
            return 1;
        case LARGEITER:
        case TRUECOLOR16:
            return 2;
        case TRUECOLOR24:
            return 3;
        case TRUECOLOR:
            return 4;
        default:
            assert(0);
            return 0;
    }
}

struct image *create_image_lines(int width, int height, int nimages,
                                 pixel_t **lines1, pixel_t **lines2,
                                 struct palette *palette,
                                 void (*flip)(struct image *img), int flags,
                                 float pixelwidth, float pixelheight)
{
    int i;
    static int version = 1;
    struct image *img = (struct image *)calloc(1, sizeof(*img));
    if (img == NULL)
        return NULL;
    if (flip == NULL)
        flip = flipgeneric;
    img->width = width;
    img->height = height;
    img->nimages = nimages;
    img->bytesperpixel = bytesperpixel(palette->type);
    img->palette = palette;
    img->currimage = 0;
    img->flip = flip;
    img->flags = flags;
    img->version = version;
    version += 65535;
    img->currlines = lines1;
    img->oldlines = lines2;
    img->pixelwidth = pixelwidth;
    img->pixelheight = pixelheight;
    if (lines1 != NULL && (nimages != 2 || lines2 != NULL)) {
        img->scanline = (int)(lines1[1] - lines1[0]);
        if (img->scanline < 0)
            img->scanline = -1;
        else {
            for (i = 0; i < height; i++) {
                if (lines1[0] - lines1[i] != img->scanline * i) {
                    img->scanline = -1;
                    break;
                }
                if (nimages == 2 &&
                    lines2[0] - lines2[i] != img->scanline * i) {
                    img->scanline = -1;
                    break;
                }
            }
        }
    } else
        img->scanline = -1;
    return (img);
}

struct image *create_image_cont(int width, int height, int scanlinesize,
                                int nimages, pixel_t *buf1, pixel_t *buf2,
                                struct palette *palette,
                                void (*flip)(struct image *img), int flags,
                                float pixelwidth, float pixelheight)
{
    struct image *img =
        create_image_lines(width, height, nimages, NULL, NULL, palette, flip,
                           flags, pixelwidth, pixelheight);
    int i;
    if (img == NULL) {
        return NULL;
    }
    if ((img->currlines =
             (pixel_t **)malloc(sizeof(*img->currlines) * height)) == NULL) {
        free(img);
        return NULL;
    }
    if (nimages == 2) {
        if ((img->oldlines =
                 (pixel_t **)malloc(sizeof(*img->oldlines) * height)) == NULL) {
            free(img->currlines);
            free(img);
            return NULL;
        }
    }
    for (i = 0; i < img->height; i++) {
        img->currlines[i] = buf1;
        buf1 += scanlinesize;
    }
    if (nimages == 2)
        for (i = 0; i < img->height; i++) {
            img->oldlines[i] = buf2;
            buf2 += scanlinesize;
        }
    img->flags |= FREELINES;
    img->scanline = scanlinesize;
    img->data = NULL;
    img->free = NULL;
    return (img);
}

struct image *create_image_mem(int width, int height, int nimages,
                               struct palette *palette, float pixelwidth,
                               float pixelheight)
{
    unsigned char *data = (unsigned char *)calloc(((width + 3) & ~3) * height,
                                                  bytesperpixel(palette->type));
    unsigned char *data1 =
        (unsigned char *)(nimages == 2 ? calloc(((width + 3) & ~3) * height,
                                                bytesperpixel(palette->type))
                                       : NULL);
    struct image *img;
    if (data == NULL) {
#ifdef DEBUG
        printf("Image:out of memory\n");
#endif
        return (NULL);
    }
    if (nimages == 2 && data1 == NULL) {
        free(data);
#ifdef DEBUG
        printf("Image:out of memory2\n");
#endif
        return NULL;
    }
    img = create_image_cont(
        width, height, ((width + 3) & ~3) * bytesperpixel(palette->type),
        nimages, data, data1, palette, NULL, 0, pixelwidth, pixelheight);
    if (img == NULL) {
        free(data);
        if (data1 != NULL)
            free(data1);
        return NULL;
    }
    img->flags |= FREEDATA;
    return (img);
}

struct image *create_subimage(struct image *simg, int width, int height,
                              int nimages, struct palette *palette,
                              float pixelwidth, float pixelheight)
{
    int size = height * bytesperpixel(palette->type);
    int i;
    int shift1 = 0, shift2 = 0;
    struct image *img;
    if (size > simg->height * simg->bytesperpixel || height > simg->height ||
        (nimages == 2 && simg->nimages == 1))
        return (create_image_mem(width, height, nimages, palette, pixelwidth,
                                 pixelheight));
    nimages = simg->nimages;
    img = create_image_lines(width, height, nimages, NULL, NULL, palette, NULL,
                             0, pixelwidth, pixelheight);
    if (img == NULL)
        return NULL;
    if ((img->currlines =
             (pixel_t **)malloc(sizeof(*img->currlines) * height)) == NULL) {
        free(img);
        return NULL;
    }
    if (nimages == 2) {
        if ((img->oldlines =
                 (pixel_t **)malloc(sizeof(*img->oldlines) * height)) == NULL) {
            free(img->currlines);
            free(img);
            return NULL;
        }
    }
    shift1 = simg->height - img->height;
    shift2 =
        simg->width * simg->bytesperpixel - img->width * img->bytesperpixel;
    for (i = 0; i < img->height; i++) {
        img->currlines[i] = simg->currlines[i + shift1] + shift2;
    }
    if (nimages == 2)
        for (i = 0; i < img->height; i++) {
            img->oldlines[i] = simg->oldlines[i + shift1] + shift2;
        }
    img->flags |= FREELINES;
    img->currimage = simg->currimage;
    return (img);
}

void destroy_image(struct image *img)
{
    if (img->free) {
        img->free(img);
        return;
    }
    if (img->flags & FREEDATA) {
        free(*img->currlines);
        if (img->nimages == 2)
            free(*img->oldlines);
    }
    if (img->flags & FREELINES) {
        free(img->currlines);
        if (img->nimages == 2)
            free(img->oldlines);
    }
    free(img);
}

void clear_image(struct image *img)
{
    int i;
    int color = img->palette->pixels[0];
    int width = img->width * img->bytesperpixel;
    if (img->palette->npreallocated)
        color = img->palette->index[0];
    if (!width) {
        width = (img->width + 7) / 8;
        if (color)
            color = 255;
    }
    for (i = 0; i < img->height; i++)
        memset(img->currlines[i], color, width);
}

#define drawchar drawchar8
#define hline hline8
#define vline vline8
#define rectangle rectangle8
#define savevline savevline8
#define restorevline restorevline8
#define line line8
#define saveline saveline8
#define restoreline restoreline8
#include "grlibd.h"
#include "hicolor.h"
#define drawchar drawchar16
#define hline hline16
#define vline vline16
#define rectangle rectangle16
#define savevline savevline16
#define restorevline restorevline16
#define line line16
#define saveline saveline16
#define restoreline restoreline16
#include "grlibd.h"
#include "true24.h"
#define drawchar drawchar24
#define hline hline24
#define vline vline24
#define rectangle rectangle24
#define savevline savevline24
#define restorevline restorevline24
#define line line24
#define saveline saveline24
#define restoreline restoreline24
#include "grlibd.h"
#include "truecolor.h"
#define drawchar drawchar32
#define hline hline32
#define vline vline32
#define rectangle rectangle32
#define savevline savevline32
#define restorevline restorevline32
#define line line32
#define saveline saveline32
#define restoreline restoreline32
#include "grlibd.h"

void xhline(struct image *image, int x, int y, int width, int fgcolor)
{
    /*Do some clipping */
    if (x + width < 0 || y < 0 || y >= image->height || x >= image->width)
        return;
    if (x + width >= image->width - 1)
        width = image->width - x - 2;
    if (x < 0)
        width += x, x = 0;
    if (width < 0)
        return;
    switch (image->bytesperpixel) {
#ifdef SBITMAPS
        case 0:
            hline1(image, x, y, width, fgcolor);
            break;
#endif
        case 1:
            hline8(image, x, y, width, fgcolor);
            break;
#ifdef SUPPORT16
        case 2:
            hline16(image, x, y, width, fgcolor);
            break;
#endif
#ifdef STRUECOLOR24
        case 3:
            hline24(image, x, y, width, fgcolor);
            break;
#endif
        case 4:
            hline32(image, x, y, width, fgcolor);
            break;
    }
}

void xvline(struct image *image, int x, int y, int height, int fgcolor)
{
    /*Do some clipping */
    if (x < 0 || y + height < 0 || y >= image->height || x >= image->width)
        return;
    if (y + height >= image->height - 1)
        height = image->height - y - 2;
    if (y < 0)
        height += y, y = 0;
    if (height < 0)
        return;
    switch (image->bytesperpixel) {
#ifdef SBITMAPS
        case 0:
            vline1(image, x, y, height, fgcolor);
            break;
#endif
        case 1:
            vline8(image, x, y, height, fgcolor);
            break;
#ifdef SUPPORT16
        case 2:
            vline16(image, x, y, height, fgcolor);
            break;
#endif
#ifdef STRUECOLOR24
        case 3:
            vline24(image, x, y, height, fgcolor);
            break;
#endif
        case 4:
            vline32(image, x, y, height, fgcolor);
            break;
    }
}

void xrectangle(struct image *image, int x, int y, int width, int height,
                int fgcolor)
{
    /*Do some clipping */
    if (x + width < 0 || y + height < 0 || y >= image->height ||
        x >= image->width)
        return;
    if (x + width >= image->width)
        width = image->width - x;
    if (x < 0)
        width += x, x = 0;
    if (width < 0)
        return;
    if (y + height >= image->height)
        height = image->height - y;
    if (y < 0)
        height += y, y = 0;
    if (height < 0)
        return;
    switch (image->bytesperpixel) {
#ifdef SBITMAPS
        case 0:
            rectangle1(image, x, y, width, height, fgcolor);
            break;
#endif
        case 1:
            rectangle8(image, x, y, width, height, fgcolor);
            break;
#ifdef SUPPORT16
        case 2:
            rectangle16(image, x, y, width, height, fgcolor);
            break;
#endif
#ifdef STRUECOLOR24
        case 3:
            rectangle24(image, x, y, width, height, fgcolor);
            break;
#endif
        case 4:
            rectangle32(image, x, y, width, height, fgcolor);
            break;
    }
}

static inline char *savehline(struct image *i, int x1, int y, int x2)
{
    int start, end;
    char *c;
    if (!i->bytesperpixel)
        start = (x1) / 8, end = (x2 + 1 + 7) / 8;
    else
        start = x1 * i->bytesperpixel, end = (x2 + 1) * i->bytesperpixel;
    c = (char *)malloc(end - start);
    if (c == NULL)
        return NULL;
    memcpy(c, i->currlines[y] + start, end - start);
    return c;
}

static inline void restorehline(struct image *i, char *c, int x1, int y, int x2)
{
    int start, end;
    if (!i->bytesperpixel)
        start = (x1) / 8, end = (x2 + 1 + 7) / 8;
    else
        start = x1 * i->bytesperpixel, end = (x2 + 1) * i->bytesperpixel;
    memcpy(i->currlines[y] + start, c, end - start);
}

#define __clipx1 0
#define __clipy1 0
#define __clipx2 (img->width - 2)
#define __clipy2 (img->height - 2)
static inline int regioncode(struct image *img, const int x, const int y)
{
    int dx1, dx2, dy1, dy2;
    int result;
    result = 0;
    dy2 = __clipy2 - y;
    if (dy2 < 0)
        result++;
    result <<= 1;
    dy1 = y - __clipy1;
    if (dy1 < 0)
        result++;
    result <<= 1;
    dx2 = __clipx2 - x;
    if (dx2 < 0)
        result++;
    result <<= 1;
    dx1 = x - __clipx1;
    if (dx1 < 0)
        result++;
    return result;
}

#define swap(x, y)                                                             \
    {                                                                          \
        int temp = x;                                                          \
        x = y;                                                                 \
        y = temp;                                                              \
    }
#define doclip(ret)                                                            \
    for (;;) {                                                                 \
        int r1 = regioncode(img, x1, y1);                                      \
        int r2 = regioncode(img, x2, y2);                                      \
        if (!(r1 | r2))                                                        \
            break; /* completely inside */                                     \
        if (r1 & r2)                                                           \
            ret; /* completely outside */                                      \
        if (r1 == 0) {                                                         \
            swap(x1, x2); /* make sure first */                                \
            swap(y1, y2); /* point is outside */                               \
            r1 = r2;                                                           \
        }                                                                      \
        if (r1 & 1) { /* left */                                               \
            y1 += (long)(__clipx1 - x1) * (long)(y2 - y1) / (long)(x2 - x1);   \
            x1 = __clipx1;                                                     \
        } else if (r1 & 2) { /* right */                                       \
            y1 += (long)(__clipx2 - x1) * (long)(y2 - y1) / (long)(x2 - x1);   \
            x1 = __clipx2;                                                     \
        } else if (r1 & 4) { /* top */                                         \
            x1 += (long)(__clipy1 - y1) * (long)(x2 - x1) / (long)(y2 - y1);   \
            y1 = __clipy1;                                                     \
        } else if (r1 & 8) { /* bottom */                                      \
            x1 += (long)(__clipy2 - y1) * (long)(x2 - x1) / (long)(y2 - y1);   \
            y1 = __clipy2;                                                     \
        }                                                                      \
    }                                                                          \
    if (x2 < x1) {                                                             \
        swap(x1, x2);                                                          \
        swap(y1, y2);                                                          \
    }

void xline(struct image *img, int x1, int y1, int x2, int y2, int color)
{
    doclip(return );
    if (x1 == x2) {
        if (y2 < y1) {
            swap(y1, y2);
        }
        switch (img->bytesperpixel) {
#ifdef SBITMAPS
            case 0:
                vline1(img, x1, y1, y2 - y1, color);
                break;
#endif
            case 1:
                vline8(img, x1, y1, y2 - y1, color);
                break;
#ifdef SUPPORT16
            case 2:
                vline16(img, x1, y1, y2 - y1, color);
                break;
#endif
#ifdef STRUECOLOR24
            case 3:
                vline24(img, x1, y1, y2 - y1, color);
                break;
#endif
            case 4:
                vline32(img, x1, y1, y2 - y1, color);
                break;
        }
    } else if (y1 == y2) {
        switch (img->bytesperpixel) {
#ifdef SBITMAPS
            case 0:
                hline1(img, x1, y1, x2 - x1, color);
                break;
#endif
            case 1:
                hline8(img, x1, y1, x2 - x1, color);
                break;
#ifdef SUPPORT16
            case 2:
                hline16(img, x1, y1, x2 - x1, color);
                break;
#endif
#ifdef STRUECOLOR24
            case 3:
                hline24(img, x1, y1, x2 - x1, color);
                break;
#endif
            case 4:
                hline32(img, x1, y1, x2 - x1, color);
                break;
        }
    } else {
        switch (img->bytesperpixel) {
#ifdef SBITMAPS
            case 0:
                line1(img, x1, y1, x2, y2, color);
                break;
#endif
            case 1:
                line8(img, x1, y1, x2, y2, color);
                break;
#ifdef SUPPORT16
            case 2:
                line16(img, x1, y1, x2, y2, color);
                break;
#endif
#ifdef STRUECOLOR24
            case 3:
                line24(img, x1, y1, x2, y2, color);
                break;
#endif
            case 4:
                line32(img, x1, y1, x2, y2, color);
                break;
        }
    }
}

char *xsaveline(struct image *img, int x1, int y1, int x2, int y2)
{
    doclip(return (NULL));
    if (y1 == y2) {
        return (savehline(img, x1, y1, x2));
    } else if (x1 == x2) {
        if (y2 < y1) {
            swap(y1, y2);
        }
        switch (img->bytesperpixel) {
#ifdef SBITMAPS
            case 0:
                return (savevline8(img, x1 / 8, y1, y2 - y1));
#endif
            case 1:
                return (savevline8(img, x1, y1, y2 - y1));
#ifdef SUPPORT16
            case 2:
                return (savevline16(img, x1, y1, y2 - y1));
#endif
#ifdef STRUECOLOR24
            case 3:
                return (savevline24(img, x1, y1, y2 - y1));
#endif
            case 4:
                return (savevline32(img, x1, y1, y2 - y1));
        }
    } else {
        switch (img->bytesperpixel) {
#ifdef SBITMAPS
            case 0:
                if (x2 > img->height - 15)
                    x2 = img->height - 15;
                if (x1 > img->height - 15)
                    x1 = img->height - 15;
                return (saveline8(img, (x1 / 8), y1, (x2 / 8), y2));
#endif
            case 1:
                return (saveline8(img, x1, y1, x2, y2));
#ifdef SUPPORT16
            case 2:
                return (saveline16(img, x1, y1, x2, y2));
#endif
#ifdef STRUECOLOR24
            case 3:
                return (saveline24(img, x1, y1, x2, y2));
#endif
            case 4:
                return (saveline32(img, x1, y1, x2, y2));
        }
    }
    return NULL;
}

void xprepareimage(struct image */*img*/) {}

void xrestoreline(struct image *img, char *data, int x1, int y1, int x2, int y2)
{
    doclip(return );
    if (y1 == y2) {
        restorehline(img, data, x1, y1, x2);
        return;
    } else if (x1 == x2) {
        if (y2 < y1) {
            swap(y1, y2);
        }
        switch (img->bytesperpixel) {
#ifdef SBITMAPS
            case 0:
                restorevline8(img, data, x1 / 8, y1, y2 - y1);
                break;
#endif
            case 1:
                restorevline8(img, data, x1, y1, y2 - y1);
                break;
#ifdef SUPPORT16
            case 2:
                restorevline16(img, data, x1, y1, y2 - y1);
                break;
#endif
#ifdef STRUECOLOR24
            case 3:
                restorevline24(img, data, x1, y1, y2 - y1);
                break;
#endif
            case 4:
                restorevline32(img, data, x1, y1, y2 - y1);
                break;
        }
    } else {
        switch (img->bytesperpixel) {
#ifdef SBITMAPS
            case 0:
                if (x2 > img->height - 15)
                    x2 = img->height - 15;
                if (x1 > img->height - 15)
                    x1 = img->height - 15;
                restoreline8(img, data, x1 / 8, y1, x2 / 8, y2);
                break;
#endif
            case 1:
                restoreline8(img, data, x1, y1, x2, y2);
                break;
#ifdef SUPPORT16
            case 2:
                restoreline16(img, data, x1, y1, x2, y2);
                break;
#endif
#ifdef STRUECOLOR24
            case 3:
                restoreline24(img, data, x1, y1, x2, y2);
                break;
#endif
            case 4:
                restoreline32(img, data, x1, y1, x2, y2);
                break;
        }
    }
    return;
}
