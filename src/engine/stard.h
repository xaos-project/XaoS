/*
 * Templated version of starfield effect for different pixel depths.
 * Replaces the old preprocessor-based approach with type-safe templates.
 */
#include "pixel_traits.h"

namespace tpl {

template <typename PixelTraits>
static void do_starfield(void *data, struct taskinfo */*task*/, int r1, int r2)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;
    using pixeldata_t = typename p::pixeldata_t;

    struct filter *f = (struct filter *)data;
    pixel_t *dest;
    pixel8_t *src, *srcend;
    unsigned int color;
    int y;
    pixeldata_t black = (pixeldata_t)f->image->palette->pixels[0];
    mysrandom((unsigned int)rand());
    for (y = r1; y < r2; y++) {
        src = f->childimage->currlines[y];
        srcend = f->childimage->currlines[y] + f->childimage->width;
        dest = (pixel_t *)f->image->currlines[y];
        while (src < srcend) {
            color = ((unsigned int)myrandom() >> 7) & 15;
            if (!*src ||
                (unsigned int)*src * (unsigned int)*src * (unsigned int)*src >
                    (unsigned int)((unsigned int)myrandom() & (0xffffff))) {
                p::set(dest, (pixeldata_t)f->image->palette->pixels[color]);
            } else
                p::set(dest, black);
            p::inc(dest, 1);
            src++;
        }
    }
}

} // namespace tpl
