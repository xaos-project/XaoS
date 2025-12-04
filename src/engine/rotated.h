/*
 * Templated version of image rotation for different pixel depths.
 * Replaces the old preprocessor-based approach with type-safe templates.
 */
#include "pixel_traits.h"

namespace tpl {

template <typename PixelTraits>
static void do_rotate(void *data, struct taskinfo */*task*/, int r1, int r2)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    struct filter *f = (struct filter *)data;
    struct rotatedata *s = (struct rotatedata *)f->data;
    double xstep = (s->x2 - s->x1) * 65536 / f->image->height;
    double ystep = (s->y2 - s->y1) * 65536 / f->image->height;
    double x = (s->x1) * 65536, y = (s->y1) * 65536;

    int ixstep = (int)((s->xx1 - s->x1) * 65536);
    int iystep = (int)((s->yy1 - s->y1) * 65536);
    int i;

    if (x < 0)
        x = 0; /*avoid shifting problems */
    if (y < 0)
        y = 0;

    ixstep /= f->image->width;
    iystep /= f->image->width;

    /* I do floating point dda here since I expect that registers used by dda
     * will not conflict with registers of integer one used by main loop so it
     * will be faster than dda from stack :)
     */
    x += r1 * xstep;
    y += r1 * ystep;
    for (i = r1; i < r2; i++) {

        {
            int ix = (int)x;
            int iy = (int)y;
            pixel_t **vbuff = (pixel_t **)f->childimage->currlines;
            pixel_t *end = p::add((pixel_t *)f->image->currlines[i],
                                  f->image->width),
                    *dest = (pixel_t *)f->image->currlines[i];
            int iixstep = ixstep, iiystep = iystep;

            while (dest < end) {
                p::copy(dest, 0, (pixel_t *)(vbuff[iy >> 16]), (ix >> 16));
                p::inc(dest, 1);
                ix += iixstep;
                iy += iiystep;
            }
        }

        x += xstep;
        y += ystep;
    }
}

} // namespace tpl
