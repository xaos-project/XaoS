/*
 * Templated version of edge detection for different pixel depths.
 * Replaces the old preprocessor-based approach with type-safe templates.
 */
#include "pixel_traits.h"

namespace tpl {

template <typename PixelTraits>
static void do_edge(void *data, struct taskinfo */*task*/, int r1, int r2)
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
            up = ((pixel16_t *)f->childimage->currlines[y - 1]) + 2;
        else
            up = ((pixel16_t *)f->childimage->currlines[y]) + 2;
        if (y != f->image->height - 1)
            down = ((pixel16_t *)f->childimage->currlines[y + 1]) + 2;
        else
            down = ((pixel16_t *)f->childimage->currlines[y]) + 2;
        end = p::add((pixel_t *)f->image->currlines[y], f->image->width - 1);
        p::setp(output, -1, 0);
        p::setp(output, f->image->width - 2, 0);
        while (output < end) {
            if (input[1] != input[0] || input[0] != up[0] ||
                input[0] != down[0]) {
                if (output < end - 2) {
                    p::set(output, pixels[input[0]]);
                    p::setp(output, 1, pixels[input[1]]);
                    p::setp(output, 2, pixels[input[2]]);
                    p::inc(output, 3);
                    input += 3;
                    up += 3;
                    down += 3;
                    while (output < end - 1 &&
                           (input[0] != up[-1] || input[0] != down[-1])) {
                        p::set(output, pixels[input[0]]);
                        p::setp(output, 1, pixels[input[1]]);
                        p::inc(output, 2);
                        input += 2;
                        up += 2;
                        down += 2;
                    }
                    if (output < end &&
                        (input[-1] != input[0] || up[-2] != input[0] ||
                         down[-2] != input[0])) {
                        p::set(output, pixels[input[0]]);
                        p::inc(output, 1);
                        input++;
                        up++;
                        down++;
                    }
                } else
                    p::set(output, pixels[*input]), p::inc(output, 1), input++,
                        up++, down++;
            } else
                p::set(output, black), p::inc(output, 1), input++, up++, down++;
        }
    }
}

} // namespace tpl
