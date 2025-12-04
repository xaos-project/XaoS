/*
 * Templated versions of boundary trace functions for different pixel depths.
 * Replaces the old preprocessor-based approach with type-safe templates.
 */
#include "pixel_traits.h"

namespace tpl {

template <typename PixelTraits>
static void tracecolor(int xstart, int ystart, int xend, int yend, int x, int y)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;
    using ppixel_t = typename p::ppixel_t;
    using pixeldata_t = typename p::pixeldata_t;

    int dir = RIGHT, fill = 0;
    unsigned char *calc;
    int peri = 0;
    pixeldata_t c = (pixeldata_t)calculatepixel(x, y, 0);
    pixeldata_t w = (pixeldata_t)0;
    pixeldata_t inset = (pixeldata_t)cpalette.pixels[0];
    p::setp((ppixel_t)cimage.currlines[y], x, c);
    calc = calculated + x + y * CALCWIDTH;
    *calc = (unsigned char)1;
    while (x > xstart && p::getp((ppixel_t)cimage.currlines[y], x - 1) == c)
        x--, calc--;
    *calc = (unsigned char)2;
    if (c == inset)
        peri = 1;
    do {
        if (!fill && !*calc) {
            *calc = (unsigned char)1;
            p::setp((ppixel_t)cimage.currlines[y], x, c);
        }
        switch (dir) {
            case RIGHT:
                if (y > ystart) {
                    if (!*(calc - CALCWIDTH)) {
                        w = (pixeldata_t)calculatepixel(x, y - 1, peri);
                        p::setp((ppixel_t)cimage.currlines[y - 1], x, w);
                        *(calc - CALCWIDTH) = (unsigned char)1;
                    } else
                        w = p::getp((ppixel_t)cimage.currlines[y - 1], x);
                    if (w == c) {
                        dir = UP;
                        calc -= CALCWIDTH;
                        y--;
                        break;
                    }
                }

                if (x < xend) {
                    if (!*(calc + 1)) {
                        w = (pixeldata_t)calculatepixel(x + 1, y, peri);
                        p::setp((ppixel_t)cimage.currlines[y], x + 1, w);
                        *(calc + 1) = (unsigned char)1;
                    } else
                        w = p::getp((ppixel_t)cimage.currlines[y], x + 1);
                    if (w == c) {
                        calc++;
                        x++;
                        break;
                    }
                }

                if (y < yend) {
                    if (!*(calc + CALCWIDTH)) {
                        w = (pixeldata_t)calculatepixel(x, y + 1, peri);
                        p::setp((ppixel_t)cimage.currlines[y + 1], x, w);
                        *(calc + CALCWIDTH) = (unsigned char)1;
                    } else
                        w = p::getp((ppixel_t)cimage.currlines[y + 1], x);
                    if (w == c) {
                        dir = DOWN;
                        calc += CALCWIDTH;
                        y++;
                        break;
                    }
                }

                if (*calc == (unsigned char)2) {
                    *calc = (unsigned char)1;
                    return;
                }

                dir = LEFT;
                x--;
                calc--;
                break;

            case LEFT:
                if (y < yend) {
                    if (!*(calc + CALCWIDTH)) {
                        w = (pixeldata_t)calculatepixel(x, y + 1, peri);
                        p::setp((ppixel_t)cimage.currlines[y + 1], x, w);
                        *(calc + CALCWIDTH) = (unsigned char)1;
                    } else
                        w = p::getp((ppixel_t)cimage.currlines[y + 1], x);
                    if (w == c) {
                        dir = DOWN;
                        calc += CALCWIDTH;
                        y++;
                        break;
                    }
                }

                if (x > xstart) {
                    if (!*(calc - 1)) {
                        w = (pixeldata_t)calculatepixel(x - 1, y, peri);
                        p::setp((ppixel_t)cimage.currlines[y], x - 1, w);
                        *(calc - 1) = (unsigned char)1;
                    } else
                        w = p::getp((ppixel_t)cimage.currlines[y], x - 1);
                    if (w == c) {
                        calc--;
                        x--;
                        break;
                    }
                }

                if (y > ystart) {
                    if (!*(calc - CALCWIDTH)) {
                        w = (pixeldata_t)calculatepixel(x, y - 1, peri);
                        p::setp((ppixel_t)cimage.currlines[y - 1], x, w);
                        *(calc - CALCWIDTH) = (unsigned char)1;
                    } else
                        w = p::getp((ppixel_t)cimage.currlines[y - 1], x);
                    if (w == c) {
                        dir = UP;
                        calc -= CALCWIDTH;
                        y--;
                        break;
                    }
                }

                dir = RIGHT;
                x++;
                calc++;
                break;

            case UP:
                if (fill) {
                    unsigned char *calc1;
                    pixel_t *pixel1;
                    calc1 = calc + 1;
                    pixel1 = p::add((pixel_t *)cimage.currlines[y], x + 1);
                    while (pixel1 <=
                           p::add((pixel_t *)cimage.currlines[y], xend)) {
                        if (!*calc1)
                            *calc1 = (unsigned char)1, p::set(pixel1, c);
                        else if (p::get(pixel1) != c)
                            break;
                        p::inc(pixel1, 1);
                        calc1++;
                    }
                }
                if (x > xstart) {
                    if (!*(calc - 1)) {
                        w = (pixeldata_t)calculatepixel(x - 1, y, peri);
                        p::setp((ppixel_t)cimage.currlines[y], x - 1, w);
                        *(calc - 1) = (unsigned char)1;
                    }
                    w = p::getp((ppixel_t)cimage.currlines[y], x - 1);
                    if (w == c) {
                        dir = LEFT;
                        calc--;
                        x--;
                        break;
                    }
                }

                if (y > ystart) {
                    if (!*(calc - CALCWIDTH)) {
                        w = (pixeldata_t)calculatepixel(x, y - 1, peri);
                        p::setp((ppixel_t)cimage.currlines[y - 1], x, w);
                        *(calc - CALCWIDTH) = (unsigned char)1;
                    }
                    w = p::getp((ppixel_t)cimage.currlines[y - 1], x);
                    if (w == c) {
                        calc -= CALCWIDTH;
                        y--;
                        break;
                    }
                }

                if (x < xend) {
                    if (!*(calc + 1)) {
                        w = (pixeldata_t)calculatepixel(x + 1, y, peri);
                        p::setp((ppixel_t)cimage.currlines[y], x + 1, w);
                        *(calc + 1) = (unsigned char)1;
                    } else
                        w = p::getp((ppixel_t)cimage.currlines[y], x + 1);
                    if (w == c) {
                        dir = RIGHT;
                        calc++;
                        x++;
                        break;
                    }
                }

                dir = DOWN;
                y++;
                calc += CALCWIDTH;
                break;
            case DOWN:
                if (x < xend) {
                    if (!*(calc + 1)) {
                        w = (pixeldata_t)calculatepixel(x + 1, y, peri);
                        p::setp((ppixel_t)cimage.currlines[y], x + 1, w);
                        *(calc + 1) = (unsigned char)1;
                    } else
                        w = p::getp((ppixel_t)cimage.currlines[y], x + 1);
                    if (w == c) {
                        dir = RIGHT;
                        calc++;
                        x++;
                        break;
                    }
                }

                if (y < yend) {
                    if (!*(calc + CALCWIDTH)) {
                        w = (pixeldata_t)calculatepixel(x, y + 1, peri);
                        p::setp((ppixel_t)cimage.currlines[y + 1], x, w);
                        *(calc + CALCWIDTH) = (unsigned char)1;
                    } else
                        w = p::getp((ppixel_t)cimage.currlines[y + 1], x);
                    if (w == c) {
                        dir = DOWN;
                        calc += CALCWIDTH;
                        y++;
                        break;
                    }
                }

                if (x > xstart) {
                    if (!*(calc - 1)) {
                        w = (pixeldata_t)calculatepixel(x - 1, y, peri);
                        p::setp((ppixel_t)cimage.currlines[y], x - 1, w);
                        *(calc - 1) = (unsigned char)1;
                    } else
                        w = p::getp((ppixel_t)cimage.currlines[y], x - 1);
                    if (w == c) {
                        dir = LEFT;
                        calc--;
                        x--;
                        break;
                    }
                }

                dir = UP;
                calc -= CALCWIDTH;
                y--;
                break;
        }
        if (*calc == (unsigned char)2) {
            if (fill) {
                *calc = (unsigned char)1;
                return;
            }
            fill = 1;
            dir = RIGHT;
        }
    } while (1);
}

#ifndef bthreads
#define ethreads 1

template <typename PixelTraits>
static inline void tracepoint(int xp, int yp, int dir, unsigned int color,
                              int xstart, int xend, int ystart, int yend)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;
    using ppixel_t = typename p::ppixel_t;
    using pixeldata_t = typename p::pixeldata_t;

    unsigned char *calc;
    pixeldata_t mycolor;
    int i, lookdir;
    unsigned int c;
    int x, y;
    int periodicity = (dir & 8) != 0;
    dir &= ~8;
    calc = calculated + xp + yp * CALCWIDTH;

    if (!(*calc & (CALCULATED | CALCULATING))) {
        *calc |= CALCULATING;
        mycolor = (pixeldata_t)calculatepixel(xp, yp, periodicity);
        p::setp((ppixel_t)cimage.currlines[yp], xp, mycolor);
        *calc |= CALCULATED;
        *calc &= ~CALCULATING;
    } else {
        if (*calc & CALCULATING) {
            /*Bad luck..some other procesor is working with out pixel :) try
             *later.*/
            addstack(xp, yp, dir, color, periodicity);
            return;
        }
        mycolor = p::getp((ppixel_t)cimage.currlines[yp], xp);
    }

    while (1) {
        periodicity = (mycolor == inset || color == inset);
        lookdir = turnright(dir);
        for (i = 0; i < 3; i++) {
            x = xp + dirrections[lookdir][0];
            y = yp + dirrections[lookdir][1];
            if (x >= xstart && x <= xend && y >= ystart && y <= yend) {
                calc = calculated + x + y * CALCWIDTH;
                if (!(*calc & (CALCULATED | CALCULATING))) {
                    *calc |= CALCULATING;
                    c = calculatepixel(x, y, periodicity);
                    p::setp((ppixel_t)cimage.currlines[y], x, c);
                    *calc |= CALCULATED;
                    *calc &= ~CALCULATING;
                } else {
                    if (*calc & CALCULATING) {
                        /*Bad luck..some other procesor is working with out
                         *pixel :) try later.*/
                        addstack(xp, yp, dir, color, periodicity);
                        return;
                    }
                    c = p::getp((ppixel_t)cimage.currlines[y], x);
                }
                if (c == mycolor)
                    break;
                if (c != color) {
                    int dir2 = turnright(lookdir);
                    int mask = (1 << dir2) + (1 << turnright(dir2));
                    if (!(*calc & mask)) {
                        addstack(x, y, dir2, mycolor, periodicity);
                    }
                    color = c;
                }
            }
            lookdir = turnleft(lookdir);
        }
        x = xp + dirrections[lookdir][0];
        y = yp + dirrections[lookdir][1];
        if (x >= xstart && x <= xend && y >= ystart && y <= yend) {
            calc = calculated + x + y * CALCWIDTH;
            if (!(*calc & (1 << lookdir))) {
                *calc |= (1 << lookdir);
                if (size < 10) {
                    addstack(x, y, lookdir, color, periodicity);
                    return;
                } else {
                    xp = x;
                    yp = y;
                    dir = lookdir;
                    calc = calculated + xp + yp * CALCWIDTH;
                }
            } else
                return;
        } else
            return;
    }
}

template <typename PixelTraits>
static void queue(void *data, struct taskinfo *task, int r1, int r2)
{
    using p = PixelTraits;

    int x, y, d, c;
    int pos = 0;

    while (1) {
        int nstack;
        xth_lock(0);
        while (!size) {    /*Well stack is empty. */
            if (exitnow) { /*Possibly everything is done now.. */
                xth_unlock(0);
                return;
            }
            if (nwaiting == bthreads - 1) { /*We are last working CPU */
                exitnow = 1;                /*So we should exit now */
                xth_wakeup(0);              /*Wake up all waiting tasks */
                xth_unlock(0);
                return; /*and exit :) */
            }
            nwaiting++;      /*We are not latest task. */
            xth_sleep(0, 0); /*Wait until other task will push some data */
            nwaiting--;
            if (exitnow) { /*Everything is done now? */
                xth_unlock(0);
                return;
            }
        }
        nstack = xth_nthread(task);
        while (!sizes[nstack])
            if (nstack != bthreads - 1)
                nstack++;
            else
                nstack = 0;
        sizes[nstack]--;
        size--;
        pos++;
        if (pos >= sizes[nstack])
            pos = 0;
        x = starts[nstack][pos >> PAGESHIFT][pos & (PAGESIZE - 1)].x;
        y = starts[nstack][pos >> PAGESHIFT][pos & (PAGESIZE - 1)].y;
        d = starts[nstack][pos >> PAGESHIFT][pos & (PAGESIZE - 1)].direction;
        c = starts[nstack][pos >> PAGESHIFT][pos & (PAGESIZE - 1)].color;
        /* Well stack currently is queue. Should have better results at
         * SMP, since has tendency trace all ways at time, so (I believe)
         * should avoid some cache conflict and situation where queue is
         * empty. At the other hand, makes queue bigger and needs following
         * copy:
         */
        starts[nstack][pos >> PAGESHIFT][pos & (PAGESIZE - 1)] =
            starts[nstack][sizes[nstack] >> PAGESHIFT]
                  [sizes[nstack] & (PAGESIZE - 1)];
        xth_unlock(0);
        tpl::tracepoint<PixelTraits>(x, y, d, c, xstart, xend, ystart, yend);
    }
}

template <typename PixelTraits>
static void bfill(void *dat, struct taskinfo *task, int r1, int r2)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    int y;
    pixel_t *pos, *end;
    unsigned char *data;
    r1 += ystart + 1;
    r2 += ystart + 1;
    for (y = r1; y < r2; y++) {
        pos = p::add((pixel_t *)cimage.currlines[y], xstart + 1);
        end = p::add((pixel_t *)cimage.currlines[y], xend);
        data = calculated + xstart + y * CALCWIDTH + 1;
        for (; pos < end; p::inc(pos, 1), data++) {
            if (!*data)
                p::copy(pos, 0, pos, -1);
        }
    }
}

#undef ethreads
#endif

template <typename PixelTraits>
static void dosymmetries(int x1, int x2, int y1, int y2, int xsym, int cx1,
                         int cx2)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    if (cx1 != x1) {
        int y;
        pixel_t *xx1, *xx2;
        for (y = y1; y <= y2; y++) {
            xx1 = p::add((pixel_t *)cimage.currlines[y], x1);
            xx2 = p::add((pixel_t *)cimage.currlines[y], 2 * xsym - x1);
            while (xx1 < xx2) {
                p::copy(xx1, 0, xx2, 0);
                p::inc(xx1, 1);
                p::inc(xx2, -1);
            }
        }
    }
    if (cx2 != x2) {
        int y;
        pixel_t *xx1, *xx2;
        for (y = y1; y <= y2; y++) {
            xx1 = p::add((pixel_t *)cimage.currlines[y], x2);
            xx2 = p::add((pixel_t *)cimage.currlines[y], 2 * xsym - x2);
            while (xx1 > xx2) {
                p::copy(xx1, 0, xx2, 0);
                p::inc(xx1, -1);
                p::inc(xx2, 1);
            }
        }
    }
}

} // namespace tpl
