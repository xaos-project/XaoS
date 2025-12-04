/*
 * Templated version of edge detection (algorithm 2) for different pixel depths.
 * Replaces the old preprocessor-based approach with type-safe templates.
 */
#include "pixel_traits.h"

namespace tpl {

template <typename PixelTraits>
static void do_edge2(void *data, struct taskinfo */*task*/, int r1, int r2)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    struct filter *f = (struct filter *)data;
    int y;
    unsigned int *pixels = f->image->palette->pixels;
    unsigned int black = f->image->palette->pixels[0];
    pixel_t *output, *end;
    pixel16_t *up, *down, *input;

    for (y = r1; y < r2; y++) {
        output = p::add((pixel_t *)f->image->currlines[y], 1);
        input = ((pixel16_t *)f->childimage->currlines[y]) + 1;

        if (y != 0)
            up = ((pixel16_t *)f->childimage->currlines[y - 1]) + 1;
        else
            up = ((pixel16_t *)f->childimage->currlines[y]) + 1;

        if (y != f->image->height - 1)
            down = ((pixel16_t *)f->childimage->currlines[y + 1]) + 1;
        else
            down = ((pixel16_t *)f->childimage->currlines[y]) + 1;

        end = p::add((pixel_t *)f->image->currlines[y], f->image->width - 1);
        p::setp(output, -1, 0);
        p::setp(output, f->image->width - 2, 0);

        while (output < end) {
            if (input[0] > up[0] || input[0] > down[0]) {
                p::set(output, pixels[input[0]]);
            } else if (input[0] != input[1]) {
                if (input[0] < input[1]) {
                    p::set(output, black);
                    p::inc(output, 1);
                    input++;
                    up++;
                    down++;
                }
                p::set(output, pixels[input[0]]);
            } else
                p::set(output, black);
            p::inc(output, 1);
            input++;
            up++;
            down++;
        }
    }
}

} // namespace tpl
