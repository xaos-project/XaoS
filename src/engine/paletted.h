/*
 * Templated version of palette conversion for different pixel depths.
 * Replaces the old preprocessor-based approach with type-safe templates.
 */
#include "pixel_traits.h"

namespace tpl {

template <typename PixelTraits>
static void cpalette(void *data, struct taskinfo */*task*/, int r1, int r2)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    pixel8_t *src, *srcend;
    pixel_t *dest;
    struct filter *f = (struct filter *)data;
    struct palettedata *s = (struct palettedata *)f->data;
    int i;
    unsigned int *table = s->table;
    for (i = r1; i < r2; i++) {
        src = f->childimage->currlines[i];
        srcend = src + f->image->width;
        dest = (pixel_t *)f->image->currlines[i];
        while (src < srcend) {
            p::set(dest, table[*src]);
            src++;
            p::inc(dest, 1);
        }
    }
}

} // namespace tpl
