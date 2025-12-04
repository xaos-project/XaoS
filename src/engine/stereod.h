/*
 * Templated version of stereogram generation for different pixel depths.
 * Replaces the old preprocessor-based approach with type-safe templates.
 */
#include "pixel_traits.h"

namespace tpl {

template <typename PixelTraits>
static void do_stereogram(void *data, struct taskinfo */*task*/, int r1, int r2)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    struct filter *f = (struct filter *)data;
    int i, y, lc;
    struct stereogramdata *s = (struct stereogramdata *)f->data;
    pixel_t *cs, *c, *src, *src1, *ce;
    pixel8_t *c1;
    unsigned int *pixels = f->image->palette->pixels;
    s->minc = NCOLORS;
    for (i = r1; i < r2; i++) {
        int i1;
        for (i1 = 0; i1 < 2; i1++) {
            c1 = (pixel8_t *)f->childimage->currlines[i];
            c = cs = (pixel_t *)f->image->currlines[2 * i + i1];
            ce = p::add(cs, f->image->width);
            src = src1 = c;
            lc = 1024;
            while (c < ce) {
                y = *c1;
                if (y == lc)
                    p::inc(src, 2);
                else {
                    lc = y;
                    if (y < s->minc && y != 0)
                        s->minc = y;
                    y = table[y];
                    src = p::add(c, -y);
                }
                if (src < src1) {
                    p::set(c, pixels[(rand() & 15)]);
                    p::setp(c, 1, pixels[(rand() & 15)]);
                } else {
                    if (src <= cs) {
                        p::set(c, pixels[(rand() & 15)]);
                        p::setp(c, 1, pixels[(rand() & 15)]);
                    } else {
                        p::copy(c, 0, src, 0);
                        p::copy(c, 1, src, 1);
                    }
                    src1 = src;
                }
                p::inc(c, 2);
                c1++;
            }
        }
    }
}

} // namespace tpl
