#include <config.h>
#include <stdlib.h>
#include <string.h>

#include <fconfig.h>
#include <filter.h>
#include <fractal.h>
#include <ui_helper.h>
#include <grlib.h>
#include <c256.h>
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
#include <hicolor.h>
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
#include <true24.h>
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
#include <truecolor.h>
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
#ifdef SBITMAPS

static void hline1(struct image *img, int x, int y, int l, int color)
{
    int x2 = x + l;
    int c1 = 255;
    int c2 = 255;
    unsigned char *current = img->currlines[y] + x / 8;
    unsigned char *currend = img->currlines[y] + x2 / 8;
    if (img->palette->type & (LBITMAP | LIBITMAP)) {
        c2 >>= x2 & 7;
        c1 <<= 8 - (x & 7);
    } else {
        c1 >>= x & 7;
        c2 <<= 8 - (x2 & 7);
    }
    if (current == currend) {
        if (color)
            *current |= c1 & c2;
        else
            *current &= ~(c1 & c2);
    } else {
        if (color) {
            *current |= c1;
            *currend |= c2;
            memset(current + 1, (char)255, currend - current - 1);
        } else {
            *current &= ~c1;
            *currend &= ~c2;
            memset(current + 1, 0, currend - current - 1);
        }
    }
}

static void vline1(struct image *img, int x, int y, int l, int color)
{
    unsigned char c = 128 >> (x & 7);
    l += y;
    x /= 8;
    if (img->palette->type & (LBITMAP | LIBITMAP)) {
        c = ((c >> 1) & 0x55) | ((c << 1) & 0xaa);
        c = ((c >> 2) & 0x33) | ((c << 2) & 0xcc);
        c = ((c >> 4) & 0x0f) | ((c << 4) & 0xf0);
    }
    if (color)
        while (y <= l) {
            unsigned char *current = img->currlines[y] + x;
            *current |= c;
            y++;
        }
    else {
        c = ~c;
        while (y <= l) {
            unsigned char *current = img->currlines[y] + x;
            *current &= c;
            y++;
        }
    }
}

static INLINE void rectangle1(struct image *img, int x, int y, int width,
                              int height, int fgcolor)
{
    height += y;
    while (y < height)
        hline1(img, x, y, width - 1, fgcolor), y++;
}

static INLINE void line1(struct image *img, int x, int y, int x2, int y2,
                         int color)
{
    int dx = x2 - x;
    int dy = y2 - y;
    int ady = abs(dy);
#ifdef SLBITMAPS
    int type = img->palette->type;
#endif
    if (dx < ady) {
        int plus = (dx << 16) / ady;
        if (dy < 0) {
            int dy = (x << 16) | (65536 / 2);
            ady = y;
#ifdef SLBITMAPS
            if (type & (LBITMAP | LIBITMAP))
                if (!color)
                    while (ady >= y2) {
                        unsigned char *current =
                            img->currlines[ady] + (dy >> 19);
                        *current &= ~(1 << ((dy >> 16) & 7));
                        dy += plus;
                        ady--;
                    }
                else
                    while (ady >= y2) {
                        unsigned char *current =
                            img->currlines[ady] + (dy >> 19);
                        *current |= (1 << ((dy >> 16) & 7));
                        dy += plus;
                        ady--;
                    }
            else
#endif
                if (!color)
                while (ady >= y2) {
                    unsigned char *current = img->currlines[ady] + (dy >> 19);
                    *current &= ~(128 >> ((dy >> 16) & 7));
                    dy += plus;
                    ady--;
                }
            else
                while (ady >= y2) {
                    unsigned char *current = img->currlines[ady] + (dy >> 19);
                    *current |= (128 >> ((dy >> 16) & 7));
                    dy += plus;
                    ady--;
                }
        } else {
            int dy = (x << 16) | (65536 / 2);
            ady = y;
#ifdef SLBITMAPS
            if (type & (LBITMAP | LIBITMAP))
                if (!color)
                    while (ady <= y2) {
                        unsigned char *current =
                            img->currlines[ady] + (dy >> 19);
                        *current &= ~(1 << ((dy >> 16) & 7));
                        dy += plus;
                        ady++;
                    }
                else
                    while (ady <= y2) {
                        unsigned char *current =
                            img->currlines[ady] + (dy >> 19);
                        *current |= (1 << ((dy >> 16) & 7));
                        dy += plus;
                        ady++;
                    }
            else
#endif
                if (!color)
                while (ady <= y2) {
                    unsigned char *current = img->currlines[ady] + (dy >> 19);
                    *current &= ~(128 >> ((dy >> 16) & 7));
                    dy += plus;
                    ady++;
                }
            else
                while (ady <= y2) {
                    unsigned char *current = img->currlines[ady] + (dy >> 19);
                    *current |= (128 >> ((dy >> 16) & 7));
                    dy += plus;
                    ady++;
                }
        }
    } else {
        int plus = (dy << 16) / dx;
        ady = x;
        dy = (y << 16) | (65536 / 2);
#ifdef SLBITMAPS
        if (type & (LBITMAP | LIBITMAP))
            if (!color)
                while (ady <= x2) {
                    unsigned char *current =
                        img->currlines[dy >> 16] + (ady >> 3);
                    *current &= ~(1 << (ady & 7));
                    dy += plus;
                    ady++;
                }
            else
                while (ady <= x2) {
                    unsigned char *current =
                        img->currlines[dy >> 16] + (ady >> 3);
                    *current |= (1 << (ady & 7));
                    dy += plus;
                    ady++;
                }
        else
#endif
            if (!color)
            while (ady <= x2) {
                unsigned char *current = img->currlines[dy >> 16] + (ady >> 3);
                *current &= ~(128 >> (ady & 7));
                dy += plus;
                ady++;
            }
        else
            while (ady <= x2) {
                unsigned char *current = img->currlines[dy >> 16] + (ady >> 3);
                *current |= (128 >> (ady & 7));
                dy += plus;
                ady++;
            }
    }
    return;
}
#endif

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

static INLINE char *savehline(struct image *i, int x1, int y, int x2)
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

static INLINE void restorehline(struct image *i, char *c, int x1, int y, int x2)
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
static INLINE int regioncode(struct image *img, const int x, const int y)
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

void xprepareimage(struct image *img) {}

void xdrawcursor(struct image *img, int x, int y, int color, int height)
{
    xvline(img, x, y, height, color);
    xhline(img, x - 1, y - 1, 1, color);
    xhline(img, x + 1, y - 1, 1, color);
    xhline(img, x - 1, y + height, 1, color);
    xhline(img, x + 1, y + height, 1, color);
}

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
