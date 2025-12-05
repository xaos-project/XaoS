/* An edge detection filter.
 * This is very simple filter - it initializes smalliter image and then
 * does an simple edge detection algo on it.
 */
#include "config.h"
#include <cstdlib>
#include <cstdio> /*for NULL */
#define SLARGEITER
#include "xthread.h"
#include "filter.h"

/* Template functions for edge detection (moved from edged.h) */
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

static int requirement(struct filter *f, struct requirements *r)
{
    f->req = *r;
    r->nimages = 1;
    r->flags &= ~IMAGEDATA;
    r->supportedmask = MASK1BPP | MASK3BPP | MASK2BPP | MASK4BPP;
    return (f->next->action->requirement(f->next, r));
}

static int initialize(struct filter *f, struct initdata *i)
{
    inhermisc(f, i);
    /*in/out coloring modes looks better in iter modes. This also saves some
       memory in truecolor. */
    if (f->data != NULL)
        destroypalette((struct palette *)f->data);
    f->data = createpalette(
        0, 65536, i->image->bytesperpixel <= 1 ? SMALLITER : LARGEITER, 0,
        65536, NULL, NULL, NULL, NULL, NULL);
    if (!inherimage(f, i, TOUCHIMAGE | NEWIMAGE, 0, 0,
                    (struct palette *)f->data, 0, 0))
        return 0;
    return (f->previous->action->initialize(f->previous, i));
}

static struct filter *getinstance(const struct filteraction *a)
{
    struct filter *f = createfilter(a);
    f->name = "Edge detection";
    return (f);
}

static void destroyinstance(struct filter *f)
{
    if (f->data != NULL)
        destroypalette((struct palette *)f->data);
    destroyinheredimage(f);
    free(f);
}

static int doit(struct filter *f, int flags, int time)
{
    int val;
    int size = f->childimage->palette->type == SMALLITER ? 240 : 65520;
    if (f->image->palette->size < size)
        size = f->image->palette->size;
    if (((struct palette *)f->data)->size != size)
        ((struct palette *)f->data)->size = size,
                         ((struct palette *)f->data)->version++;
    updateinheredimage(f);
    val = f->previous->action->doit(f->previous, flags, time);
    drivercall(*f->image, xth_function(tpl::do_edge<Pixel8Traits>, f, f->image->height),
               xth_function(tpl::do_edge<Pixel16Traits>, f, f->image->height),
               xth_function(tpl::do_edge<Pixel24Traits>, f, f->image->height),
               xth_function(tpl::do_edge<Pixel32Traits>, f, f->image->height));
    xth_sync();
    return val;
}

const struct filteraction edge_filter = {"Edge detection",
                                         "edge",
                                         0,
                                         getinstance,
                                         destroyinstance,
                                         doit,
                                         requirement,
                                         initialize,
                                         convertupgeneric,
                                         convertdowngeneric,
                                         NULL};
