/*
 * Templated version of graphics library functions for different pixel depths.
 * Replaces the old preprocessor-based approach with type-safe templates.
 */
#include "pixel_traits.h"

namespace tpl {

template <typename PixelTraits>
static inline void hline(struct image *img, int x, int y, int length,
                         int fgcolor)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    pixel_t *current = (pixel_t *)img->currlines[y],
            *end = (pixel_t *)img->currlines[y];
    p::inc(current, x);
    p::inc(end, x + length);
    if (sizeof(pixel_t) == 1) {
        memset(current, fgcolor, end - current + 1);
    } else {
        while (current <= end) {
            p::set(current, fgcolor);
            p::inc(current, 1);
        }
    }
}

template <typename PixelTraits>
static inline void vline(struct image *img, int x, int y, int length,
                         int fgcolor)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    length += y;
    while (y <= length) {
        pixel_t *current = (pixel_t *)img->currlines[y];
        p::inc(current, x);
        p::set(current, fgcolor);
        y++;
    }
}

template <typename PixelTraits>
static inline void rectangle(struct image *img, int x, int y, int width,
                             int height, int fgcolor)
{
    height += y;
    while (y < height)
        hline<PixelTraits>(img, x, y, width - 1, fgcolor), y++;
}

template <typename PixelTraits>
static inline char *savevline(struct image *img, int x, int y, int length)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    pixel_t *saved = (pixel_t *)malloc(length * p::bpp + p::bpp), *s = saved;
    length += y;
    while (y <= length) {
        pixel_t *current = (pixel_t *)img->currlines[y];
        p::copy(s, 0, current, x);
        p::inc(s, 1);
        y++;
    }
    return (char *)saved;
}

template <typename PixelTraits>
static inline void restorevline(struct image *img, char *saved, int x, int y,
                                int length)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    pixel_t *s = (pixel_t *)saved;
    length += y;
    while (y <= length) {
        pixel_t *current = (pixel_t *)img->currlines[y];
        p::copy(current, x, s, 0);
        p::inc(s, 1);
        y++;
    }
}

template <typename PixelTraits>
static inline char *saveline(struct image *img, int x, int y, int x2, int y2)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    int dx = x2 - x;
    int dy = y2 - y;
    int ady = abs(dy);
    if (dx < ady) {
        pixel_t *saved = (pixel_t *)malloc((ady + 1) * p::bpp * 2), *s = saved;
        int plus = (dx << 16) / ady;
        if (dy < 0) {
            int dy = (x << 16) /*| (65536 / 2) */;
            ady = y;
            while (ady >= y2) {
                pixel_t *current = (pixel_t *)img->currlines[ady];
                p::inc(current, (dy >> 16));
                p::copy(s, 0, current, 0);
                p::copy(s, 1, current, 1);
                p::inc(s, 2);
                dy += plus;
                ady--;
            }
        } else {
            int dy = (x << 16) /*| (65536 / 2) */;
            ady = y;
            while (ady <= y2) {
                pixel_t *current = (pixel_t *)img->currlines[ady];
                p::inc(current, (dy >> 16));
                p::copy(s, 0, current, 0);
                p::copy(s, 1, current, 1);
                p::inc(s, 2);
                dy += plus;
                ady++;
            }
        }
        return ((char *)saved);
    } else {
        pixel_t *saved = (pixel_t *)malloc((dx + 1) * p::bpp * 2), *s = saved;
        int plus = (dy << 16) / dx;
        ady = x;
        dy = (y << 16);
        while (ady <= x2) {
            pixel_t *current = (pixel_t *)img->currlines[dy >> 16];
            p::copy(s, 0, current, ady);
            current = (pixel_t *)img->currlines[(dy >> 16) + 1];
            p::copy(s, 1, current, ady);
            p::inc(s, 2);
            dy += plus;
            ady++;
        }
        return ((char *)saved);
    }
}

template <typename PixelTraits>
static inline void restoreline(struct image *img, char *saved, int x, int y,
                               int x2, int y2)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    int dx = x2 - x;
    int dy = y2 - y;
    int ady = abs(dy);
    if (dx < ady) {
        pixel_t *s = (pixel_t *)saved;
        int plus = (dx << 16) / ady;
        if (dy < 0) {
            int dy = (x << 16) /*| (65536 / 2) */;
            ady = y;
            while (ady >= y2) {
                pixel_t *current = (pixel_t *)img->currlines[ady];
                p::inc(current, (dy >> 16));
                p::copy(current, 0, s, 0);
                p::copy(current, 1, s, 1);
                p::inc(s, 2);
                dy += plus;
                ady--;
            }
        } else {
            int dy = (x << 16) /*| (65536 / 2) */;
            ady = y;
            while (ady <= y2) {
                pixel_t *current = (pixel_t *)img->currlines[ady];
                p::inc(current, (dy >> 16));
                p::copy(current, 0, s, 0);
                p::copy(current, 1, s, 1);
                p::inc(s, 2);
                dy += plus;
                ady++;
            }
        }
    } else {
        pixel_t *s = (pixel_t *)saved;
        int plus = (dy << 16) / dx;
        ady = x;
        dy = (y << 16);
        while (ady <= x2) {
            pixel_t *current = (pixel_t *)img->currlines[dy >> 16];
            p::copy(current, ady, s, 0);
            current = (pixel_t *)img->currlines[(dy >> 16) + 1];
            p::copy(current, ady, s, 1);
            p::inc(s, 2);
            dy += plus;
            ady++;
        }
    }
}

template <typename PixelTraits>
static inline void line(struct image *img, int x, int y, int x2, int y2,
                        int color)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    int dx = x2 - x;
    int dy = y2 - y;
    int ady = abs(dy);
    int rmask = 0, gmask = 0, bmask = 0;

    if (sizeof(pixel_t) > 1) {
        rmask = img->palette->info.truec.rmask;
        gmask = img->palette->info.truec.gmask;
        bmask = img->palette->info.truec.bmask;
    }

    // Interpolation function selection based on pixel depth
    auto myinterpol = [&](int a, int b, int n) -> int {
        if (sizeof(pixel_t) == 1) {
            return intergray(a, b, n);
        } else {
            return interpol(a, b, n, rmask, gmask, bmask);
        }
    };

    if (sizeof(pixel_t) == 1 && (img->palette->type & (C256 | FIXEDCOLOR))) {
        if (dx < ady) {
            int plus = (dx << 16) / ady;
            if (dy < 0) {
                int dy = (x << 16) | (65536 / 2);
                ady = y;
                while (ady >= y2) {
                    pixel_t *current = (pixel_t *)img->currlines[ady];
                    p::inc(current, (dy >> 16));
                    p::set(current, color);
                    dy += plus;
                    ady--;
                }
            } else {
                int dy = (x << 16) | (65536 / 2);
                ady = y;
                while (ady <= y2) {
                    pixel_t *current = (pixel_t *)img->currlines[ady];
                    p::inc(current, (dy >> 16));
                    p::set(current, color);
                    dy += plus;
                    ady++;
                }
            }
        } else {
            int plus = (dy << 16) / dx;
            ady = x;
            dy = (y << 16) | (65536 / 2);
            while (ady <= x2) {
                pixel_t *current = (pixel_t *)img->currlines[dy >> 16];
                p::setp(current, ady, color);
                dy += plus;
                ady++;
            }
        }
        return;
    }

    if (dx < ady) {
        int plus = (dx << 16) / ady;
        if (dy < 0) {
            int dy = (x << 16);
            ady = y;
            while (ady >= y2) {
                pixel_t *current = (pixel_t *)img->currlines[ady];
                p::inc(current, (dy >> 16));
                p::set(current,
                      myinterpol(p::get(current), color, ((dy & 65535) >> 8)));
                p::setp(
                    current, 1,
                    myinterpol(color, p::getp(current, 1), ((dy & 65535) >> 8)));
                dy += plus;
                ady--;
            }
        } else {
            int dy = (x << 16);
            ady = y;
            while (ady <= y2) {
                pixel_t *current = (pixel_t *)img->currlines[ady];
                p::inc(current, (dy >> 16));
                p::set(current,
                      myinterpol(p::get(current), color, ((dy & 65535) >> 8)));
                p::setp(
                    current, 1,
                    myinterpol(color, p::getp(current, 1), ((dy & 65535) >> 8)));
                dy += plus;
                ady++;
            }
        }
    } else {
        int plus = (dy << 16) / dx;
        ady = x;
        dy = (y << 16);
        while (ady <= x2) {
            pixel_t *current = (pixel_t *)img->currlines[dy >> 16];
            p::setp(
                current, ady,
                myinterpol(p::getp(current, ady), color, ((dy & 65535) >> 8)));
            current = (pixel_t *)img->currlines[(dy >> 16) + 1];
            p::setp(
                current, ady,
                myinterpol(color, p::getp(current, ady), ((dy & 65535) >> 8)));
            dy += plus;
            ady++;
        }
    }
}

} // namespace tpl
