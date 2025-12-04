/*
 * Templated versions of zoom functions for different pixel depths.
 * Replaces the old preprocessor-based approach with type-safe templates.
 */
#include "pixel_traits.h"

namespace tpl {

/*  These two routines implements solid guessing. They are almost same. One
 *  calculates lines, second rows.
 *
 *  The heruistic is as follows:
 *
 *  ---1------6------5-------  (vbuffu)
 *     |      |      |
 *  ===7======X======8=======  (vbuff1)
 *     |      |      |
 *  ---2------3------4-------  (vbuffd)
 *  distdown  rx   distup
 *
 *  -- and | means calculated lines. == is current line, names are pointers to
 *  them. Note that naming is quite confusing, because it is same in lines and
 *  rows.
 *
 *  we do solid guessing as folows:
 *  |distl-vbuff1| < range
 *  |distr-vbuff1| < range
 *  the distance of distup and distdown is not limited, because we already
 *  have exact enough guesses 3 and 6
 *
 *  points 1 2 3 4 5 6 8 must be the same (point 8 is not yet calculated)
 *
 */
template <typename PixelTraits>
static void calcline(realloc_t *ry)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;
    using ppixel_t = typename p::ppixel_t;
    using pixeldata_t = typename p::pixeldata_t;

    number_t y;
    int range = cfractalc.range;
    realloc_t *rx, *rend, *rend1, *ryl, *ryr;
    int distl, distr, distup, distdown;
    pixel_t *vbuff, *vbuffu, *vbuffd;
    pixeldata_t inset = (pixeldata_t)cpalette.pixels[0];
    pixeldata_t c;
    ppixel_t *vbuff1 = (ppixel_t *)cimage.currlines + (ry - czoomc.reallocy);
    assert(ry >= czoomc.reallocy);
    assert(ry < czoomc.reallocy + cimage.height);
    y = ry->position;
    rend = ry - range - 1;
    if (czoomc.reallocy > rend)
        rend = czoomc.reallocy;
    for (ryl = ry - 1; rend <= ryl && ryl->dirty; ryl--)
        ;
    distl = (int)(ryl - ry);
    rend = ry + range;
    if (czoomc.reallocy + cimage.height < rend)
        rend = czoomc.reallocy + cimage.height;
    for (ryr = ry + 1; rend > ryr && ryr->dirty; ryr++)
        ;
    distr = (int)(ryr - ry);
    rend = czoomc.reallocy + cimage.height;
    if (ryr == czoomc.reallocy + cimage.height || ryl < czoomc.reallocy ||
        ryr->dirty || ryl->dirty) {
        for (rx = czoomc.reallocx, vbuff = *vbuff1,
            rend1 = czoomc.reallocx + cimage.width;
             rx < rend1; rx++) {
            if (!rx->dirty) {
                STAT(tocalculate++);
                p::set(vbuff, (pixeldata_t)calculate(rx->position, y,
                                                     cfractalc.periodicity));
            }
            p::inc(vbuff, 1);
        }
    } else {
        distup = INT_MAX / 2;
        distdown = 0;
        for (rx = czoomc.reallocx, vbuff = vbuff1[0], vbuffu = vbuff1[distl],
            vbuffd = vbuff1[distr], rend1 = czoomc.reallocx + cimage.width;
             rx < rend1; rx++) {
            assert(rx < czoomc.reallocx + cimage.width);
            assert(rx >= czoomc.reallocx);
            if (!rx->dirty) {
                STAT(tocalculate++);
                if (distdown <= 0) {
                    for (ryr = rx + 1; ryr < rend1 && ryr->dirty; ryr++)
                        ;
                    distdown = (int)(ryr - rx);
                    if (ryr == rend1)
                        distdown = INT_MAX / 2;
                }
                if (distdown < INT_MAX / 4 && distup < INT_MAX / 4 &&
                    (p::get(vbuffu) == (c = p::get(vbuffd)) &&
                     c == p::getp(vbuff, -distup) &&
                     c == p::getp(vbuffu, -distup) &&
                     c == p::getp(vbuffu, distdown) &&
                     c == p::getp(vbuffd, distdown) &&
                     c == p::getp(vbuffd, -distup))) {
                    p::set(vbuff, c);
                    STAT(avoided++);
                } else {
                    if (cfractalc.periodicity && distdown < INT_MAX / 4 &&
                        distup < INT_MAX / 4 &&
                        (p::get(vbuffu) != inset && p::get(vbuffd) != inset &&
                         p::getp(vbuff, -distup) != inset &&
                         p::getp(vbuffu, -distup) != inset &&
                         p::getp(vbuffu, +distdown) != inset &&
                         p::getp(vbuffd, -distup) != inset &&
                         p::getp(vbuffd, +distdown) != inset))
                        p::set(vbuff,
                              (pixeldata_t)calculate(rx->position, y, 0));
                    else
                        p::set(vbuff,
                              (pixeldata_t)calculate(rx->position, y,
                                                      cfractalc.periodicity));
                }
                distup = 0;
            }
            p::inc(vbuff, 1);
            p::inc(vbuffu, 1);
            p::inc(vbuffd, 1);
            distdown--;
            distup++;
        }
    }
    ry->recalculate = 0;
    ry->dirty = 0;
}

template <typename PixelTraits>
static void calccolumn(realloc_t *rx)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;
    using ppixel_t = typename p::ppixel_t;
    using pixeldata_t = typename p::pixeldata_t;

    number_t x;
    int range = cfractalc.range;
    realloc_t *ry, *rend, *rend1, *rxl, *rxr;
    int pos, distl, distr, distup, distdown;
    pixeldata_t c;
    pixeldata_t inset = (pixeldata_t)cpalette.pixels[0];
    ppixel_t *vbuff;
    pos = (int)(rx - czoomc.reallocx);
    assert(pos >= 0);
    assert(pos < cimage.width);
    rend = rx - range + 1;
    if (czoomc.reallocx > rend)
        rend = czoomc.reallocx;
    for (rxl = rx - 1; rend <= rxl && rxl->dirty; rxl--)
        ;
    distl = (int)(rx - rxl);
    rend = rx + range;
    if (czoomc.reallocx + cimage.width < rend)
        rend = czoomc.reallocx + cimage.width;
    for (rxr = rx + 1; rxr < rend && rxr->dirty; rxr++)
        ;
    distr = (int)(rxr - rx);
    x = rx->position;
    rend = czoomc.reallocx + cimage.width;
    if (rxr >= czoomc.reallocx + cimage.width || rxl < czoomc.reallocx ||
        rxr->dirty || rxl->dirty) {
        for (ry = czoomc.reallocy, vbuff = (ppixel_t *)cimage.currlines,
            rend1 = czoomc.reallocy + cimage.height;
             ry < rend1; ry++, vbuff++) {
            if (!ry->dirty) {
                STAT(tocalculate++);
                p::setp((*vbuff), pos,
                       (pixeldata_t)calculate(x, ry->position,
                                               cfractalc.periodicity));
            }
        }
    } else {
        distl = pos - distl;
        distr = pos + distr;
        assert(distl >= 0);
        assert(distr < cimage.width);
        distup = INT_MAX / 2;
        distdown = 0;
        for (ry = czoomc.reallocy, vbuff = (ppixel_t *)cimage.currlines,
            rend1 = czoomc.reallocy + cimage.height;
             ry < rend1; ry++) {
            /*if (ry->symto == -1) { */
            assert(ry < czoomc.reallocy + cimage.height);
            if (!ry->dirty) {
                STAT(tocalculate++);
                if (distdown <= 0) {
                    for (rxr = ry + 1; rxr < rend1 && rxr->dirty; rxr++)
                        ;
                    distdown = (int)(rxr - ry);
                    if (rxr == rend1)
                        distdown = INT_MAX / 2;
                }
                if (distdown < INT_MAX / 4 && distup < INT_MAX / 4 &&
                    (p::getp(vbuff[0], distl) == (c = p::getp(vbuff[0], distr)) &&
                     p::getp(vbuff[-distup], distl) == c &&
                     p::getp(vbuff[-distup], distr) == c &&
                     p::getp(vbuff[-distup], pos) == c &&
                     p::getp(vbuff[distdown], distr) == c &&
                     p::getp(vbuff[distdown], distl) == c)) {
                    STAT(avoided++);
                    p::setp(vbuff[0], pos, c);
                } else {
                    if (cfractalc.periodicity && distdown < INT_MAX / 4 &&
                        distup < INT_MAX / 4 &&
                        (p::getp(vbuff[0], distl) != inset &&
                         p::getp(vbuff[0], distr) != inset &&
                         p::getp(vbuff[distdown], distr) != inset &&
                         p::getp(vbuff[distdown], distl) != inset &&
                         p::getp(vbuff[-distup], distl) != inset &&
                         p::getp(vbuff[-distup], pos) != inset &&
                         p::getp(vbuff[-distup], distr) != inset))
                        p::setp(vbuff[0], pos,
                               (pixeldata_t)calculate(x, ry->position, 0));
                    else
                        p::setp(vbuff[0], pos,
                               (pixeldata_t)calculate(x, ry->position,
                                                       cfractalc.periodicity));
#ifdef DRAW
                    vga_setcolor(0xffffff);
                    vga_drawpixel(rx - czoomc.reallocx, ry - czoomc.reallocy);
#endif
                }
                distup = 0;
            }
            vbuff++;
            distdown--;
            distup++;
        }
    }
    rx->recalculate = 0;
    rx->dirty = 0;
}

template <typename PixelTraits>
static inline void dosymmetry2(void * /*data*/, struct taskinfo * /*task*/,
                               int r1, int r2)
{
    using p = PixelTraits;
    using ppixel_t = typename p::ppixel_t;

    ppixel_t *vbuff = (ppixel_t *)cimage.currlines;
    realloc_t *rx, *rend;
    ppixel_t *vend = (ppixel_t *)cimage.currlines + cimage.height;
    for (rx = czoomc.reallocx + r1, rend = czoomc.reallocx + r2; rx < rend;
         rx++) {
        assert(rx->symto >= 0 || rx->symto == -1);
        if (rx->symto >= 0) {
            assert(rx->symto < cimage.width);
            if (!czoomc.reallocx[rx->symto].dirty) {
                int pos = (int)(rx - czoomc.reallocx);
                int pos1 = rx->symto;
                vbuff = (ppixel_t *)cimage.currlines;
                for (; vbuff < vend; vbuff++)
                    p::copy(vbuff[0], pos, vbuff[0], pos1);
                rx->dirty = 0;
            }
        }
    }
}

template <typename PixelTraits>
static inline void fillline(int line)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;
    using pixeldata_t = typename p::pixeldata_t;

    unsigned char *vbuff = cimage.currlines[line];
    const struct filltable *table = (struct filltable *)tmpdata;
    while (table->length) {
        pixeldata_t s = p::get((pixel_t *)(vbuff + table->from));
        pixel_t *vcurr = (pixel_t *)(vbuff + table->to);
        pixel_t *vend = (pixel_t *)(vbuff + table->end);
        while (vcurr < vend) {
            p::set(vcurr, s);
            p::inc(vcurr, 1);
        }
        table++;
    }
}

} // namespace tpl