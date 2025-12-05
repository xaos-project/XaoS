/*
 *     XaoS, a fast portable realtime fractal zoomer
 *                  Copyright (C) 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <cfloat>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <climits>
#include "config.h"
#define SLARGEITER
#include "filter.h"
#include "zoom.h"
#include "autopilot.h"
#include "ui_helper.h"
#define MINCOUNT 5
#define InSet(i) (i == context->image->palette->pixels[0])

/* Template functions for autopilot (moved from autod.h) */
#include "pixel_traits.h"

namespace tpl {

template <typename PixelTraits>
static inline int look1(uih_context *context, int x, int y, int range, int max)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    pixel_t *vbuff;
    int i, j, c = 0;
    if (range < context->zengine->image->width / 2)
        if (x < 0 || x > context->zengine->image->width || y < 0 ||
            y > context->zengine->image->height)
            return 0;
    do {
        max--;
        c = 0;
        if (range > context->zengine->image->width / 2)
            context->x1 =
                rand() % (context->zengine->image->width - 2 * LOOKSIZE - 1) +
                LOOKSIZE,
            context->y1 =
                rand() % (context->zengine->image->height - 2 * LOOKSIZE - 1) +
                LOOKSIZE;
        else {
            context->x1 = rand() % range - (range >> 1) + x;
            context->y1 = rand() % range - (range >> 1) + y;
            if (context->x1 < LOOKSIZE)
                context->x1 = LOOKSIZE;
            if (context->y1 < LOOKSIZE)
                context->y1 = LOOKSIZE;
            if (context->x1 > context->zengine->image->width - 2 - LOOKSIZE)
                context->x1 = context->zengine->image->width - 2 - LOOKSIZE;
            if (context->y1 > context->zengine->image->height - 2 - LOOKSIZE)
                context->y1 = context->zengine->image->height - 2 - LOOKSIZE;
        }
        for (j = context->y1 - LOOKSIZE; j <= context->y1 + LOOKSIZE; j++) {
            vbuff = (pixel_t *)context->zengine->image->currlines[j];
            for (i = context->x1 - LOOKSIZE; i <= context->x1 + LOOKSIZE; i++)
                if (InSet(p::getp(vbuff, i)))
                    c++;
        }
    } while ((c == 0 || c > LOOKSIZE * LOOKSIZE) && max > 0);
    if (max > 0) {
        context->c1 = BUTTON1, context->interlevel = 1;
        return 1;
    }
    return (0);
}

template <typename PixelTraits>
static inline int look2(uih_context *context, int x, int y, int range, int max)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    pixel_t *vbuff, *vbuff2;
    int i, j, i1, j1, c = 0;
    if (range < context->zengine->image->width / 2)
        if (x < 0 || x > context->zengine->image->width || y < 0 ||
            y > context->zengine->image->height)
            return 0;
    do {
        max--;
        c = 0;

        if (range > context->zengine->image->width / 2)
            context->x1 =
                rand() % (context->zengine->image->width - 2 * LOOKSIZE - 1) +
                LOOKSIZE,
            context->y1 =
                rand() % (context->zengine->image->height - 2 * LOOKSIZE - 1) +
                LOOKSIZE;
        else {
            context->x1 = rand() % range - (range >> 1) + x;
            context->y1 = rand() % range - (range >> 1) + y;
            if (context->x1 < LOOKSIZE)
                context->x1 = LOOKSIZE;
            if (context->y1 < LOOKSIZE)
                context->y1 = LOOKSIZE;
            if (context->x1 > context->zengine->image->width - 2 - LOOKSIZE)
                context->x1 = context->zengine->image->width - 2 - LOOKSIZE;
            if (context->y1 > context->zengine->image->height - 2 - LOOKSIZE)
                context->y1 = context->zengine->image->height - 2 - LOOKSIZE;
        }

        for (j = context->y1 - LOOKSIZE; j < context->y1 + LOOKSIZE; j++) {
            vbuff = (pixel_t *)context->zengine->image->currlines[j];
            for (i = context->x1 - LOOKSIZE; i <= context->x1 + LOOKSIZE; i++)
                for (j1 = j + 1; j1 < context->y1 + LOOKSIZE; j1++) {
                    vbuff2 = (pixel_t *)context->zengine->image->currlines[j1];
                    for (i1 = i + 1; i1 < context->x1 + LOOKSIZE; i1++)
                        if (p::getp(vbuff, i) == p::getp(vbuff2, i1))
                            c++;
                }
        }

    } while ((c > LOOKSIZE * LOOKSIZE / 2) && max > 0);
    if (max > 0) {
        context->c1 = BUTTON1, context->interlevel = 2;
        return 1;
    }
    return 0;
}

} // namespace tpl

#ifdef USE_FLOAT128
#include <quadmath.h>
#define isnan isnanq
#else
#define isnan std::isnan
#endif

void clean_autopilot(uih_context *context)
{
    context->minsize = 1000;
    context->maxsize = 0;
    context->autime = 0;
    context->minlong = 0;
    context->x1 = INT_MAX;
    context->y1 = INT_MAX;
    context->autopilotversion = context->fcontext->version;
}

static void again(uih_context *context)
{
    context->fcontext->s = context->fcontext->currentformula->v;
    context->fcontext->version++;
    clean_autopilot(context);
}

void do_autopilot(uih_context *context, int *x, int *y, int *controls,
                  void (*changed)(void), int times)
{
    int c = 0;
    volatile number_t step =
        (context->fcontext->rs.mc - context->fcontext->rs.nc) /
        context->zengine->image->width / 10;
    volatile number_t pos = context->fcontext->rs.mc;
    volatile number_t pos1 = context->fcontext->rs.mc;
    volatile number_t ystep =
        (context->fcontext->rs.mi - context->fcontext->rs.ni) /
        context->zengine->image->height / 10;
    volatile number_t ypos = context->fcontext->rs.mi;
    volatile number_t ypos1 = context->fcontext->rs.mi;
    pos += step; /*out of precisity check */
    ypos += ystep;
    pos1 -= step; /*out of precisity check */
    ypos1 -= ystep;
    *x = context->x1;
    *y = context->y1;
    uih_clearwindows(context);
    context->zengine->action->convertup(context->zengine, x, y);
    if ((context->minlong > MINCOUNT && context->c1 == BUTTON3) ||
        !(pos > context->fcontext->rs.mc) ||
        !(ypos > context->fcontext->rs.mi) ||
        (pos1 >= context->fcontext->rs.mc) ||
        (ypos1 >= context->fcontext->rs.mi) ||
        context->fcontext->rs.mc - context->fcontext->rs.nc > 100.0 ||
        isnan(pos) || isnan(ypos) || isnan(context->fcontext->s.cr) ||
        isnan(context->fcontext->s.ci) ||
        isnan(context->fcontext->s.rr - context->fcontext->s.ri) ||
        context->fcontext->s.rr == 0 || context->fcontext->s.ri == 0 ||
        isnan(context->fcontext->rs.mc - context->fcontext->rs.mi) ||
        isnan(context->fcontext->rs.nc - context->fcontext->rs.ni)) {
        again(context);
        changed();
    }
    /*Are we waiting for better qualitty? */
    if (!context->c1 && context->zengine->flags & INCOMPLETE) {
        return;
    }
    assert(changed != NULL);
    if (context->fcontext->version != context->autopilotversion)
        clean_autopilot(context);
    if (context->fcontext->rs.mc - context->fcontext->rs.nc <
        context->minsize) {
        context->minsize = context->fcontext->rs.mc - context->fcontext->rs.nc;
        context->minlong = 0;
    } /*Oscillating prevention */
    if (context->fcontext->rs.mc - context->fcontext->rs.nc >
        context->maxsize) {
        context->minsize = context->fcontext->rs.mc - context->fcontext->rs.nc;
        context->maxsize = context->fcontext->rs.mc - context->fcontext->rs.nc;
        context->minlong = 0;
    }
    if (context->autime <= 0) {
        context->minlong++;
        context->autime = rand() % MAXTIME;
        if (context->zengine->flags & LOWQUALITY) {
            context->c1 = 0;
        } else {
            switch (context->zengine->image->bytesperpixel) {
                case 1:
                    c = tpl::look1<Pixel8Traits>(context, *x, *y, RANGE1, NGUESSES);
                    if (!c)
                        c = tpl::look2<Pixel8Traits>(context, *x, *y, RANGE1, NGUESSES);
                    if (!(rand() % 30))
                        c = 0;
                    if (!c)
                        c = tpl::look1<Pixel8Traits>(context, *x, *y, 10000, NGUESSES1);
                    if (!c)
                        c = tpl::look1<Pixel8Traits>(context, *x, *y, 10000, NGUESSES2);
                    break;
#ifdef SUPPORT16
                case 2:
                    c = tpl::look1<Pixel16Traits>(context, *x, *y, RANGE1, NGUESSES);
                    if (!c)
                        c = tpl::look2<Pixel16Traits>(context, *x, *y, RANGE1, NGUESSES);
                    if (!(rand() % 30))
                        c = 0;
                    if (!c)
                        c = tpl::look1<Pixel16Traits>(context, *x, *y, 10000, NGUESSES1);
                    if (!c)
                        c = tpl::look2<Pixel16Traits>(context, *x, *y, 10000, NGUESSES1);
                    break;
#endif
#ifdef STRUECOLOR24
                case 3:
                    c = tpl::look1<Pixel24Traits>(context, *x, *y, RANGE1, NGUESSES);
                    if (!c)
                        c = tpl::look2<Pixel24Traits>(context, *x, *y, RANGE1, NGUESSES);
                    if (!(rand() % 30))
                        c = 0;
                    if (!c)
                        c = tpl::look1<Pixel24Traits>(context, *x, *y, 10000, NGUESSES1);
                    if (!c)
                        c = tpl::look2<Pixel24Traits>(context, *x, *y, 10000, NGUESSES1);
                    break;
#endif
                case 4:
                    c = tpl::look1<Pixel32Traits>(context, *x, *y, RANGE1, NGUESSES);
                    if (!c)
                        c = tpl::look2<Pixel32Traits>(context, *x, *y, RANGE1, NGUESSES);
                    if (!(rand() % 30))
                        c = 0;
                    if (!c)
                        c = tpl::look1<Pixel32Traits>(context, *x, *y, 10000, NGUESSES1);
                    if (!c)
                        c = tpl::look2<Pixel32Traits>(context, *x, *y, 10000, NGUESSES1);
            }
            if (!c) {
                if ((context->zengine->flags & INCOMPLETE)) {
                    context->c1 = 0;
                } else
                    context->c1 = BUTTON3, context->autime >>= 1;
            }
        }
    }
    context->autime -= times;
    *x = context->x1;
    *y = context->y1;
    context->zengine->action->convertup(context->zengine, x, y);
    /*    printf("%i %i\n",*x,*y); */
    *controls = context->c1;
}
