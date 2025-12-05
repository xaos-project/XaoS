/*
 * Templated version of autopilot functions for different pixel depths.
 * Replaces the old preprocessor-based approach with type-safe templates.
 */
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
