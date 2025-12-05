#include "config.h"
#include <cstdlib>
#include <cstdio> /*for NULL */
#include <cmath>
#define SLARGEITER
#include "xthread.h"
#include "filter.h"

struct threeddata {
    struct palette *pal;
    struct palette *savedpalette;
    unsigned int *pixels;
    unsigned int maxiter;
    unsigned int height;
    unsigned int colheight;
    unsigned int midcolor;
    unsigned int darkcolor;
    unsigned int stereogrammode;
};

/* Template functions for 3D conversion (moved from 3dd.h) */
#include "pixel_traits.h"

namespace tpl {

template <typename PixelTraits>
static void convert_3d(struct filter *f, int *x1, int *y1)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    struct threeddata *data = (struct threeddata *)f->data;
    int y;
    int x = *x1;
    unsigned int inp;
    unsigned int height = data->height;
    const pixel16_t *input;
    if (x >= f->childimage->width - 5 || x < 0 || *y1 > f->childimage->height) {
        *x1 += *y1 / 2;
        return;
    }
    if (x < 0)
        x = 0;
    for (y = f->childimage->height - 3; y >= 0; y--) {
        int d;
        input = ((pixel16_t *)f->childimage->currlines[y] + y / 2);
        inp = (input[x] + input[x + 1] + input[x + 2] + input[x + 3] +
               input[x + 4] + input[x + 5]);
        input = ((pixel16_t *)f->childimage->currlines[y + 1] + y / 2);
        inp += (input[x] + input[x + 1] + input[x + 2] + input[x + 3] +
                input[x + 4] + input[x + 5]);
        input = ((pixel16_t *)f->childimage->currlines[y + 2] + y / 2);
        inp += (input[x] + input[x + 1] + input[x + 2] + input[x + 3] +
                input[x + 4] + input[x + 5]);
        d = y - (inp / 16 > height ? height : inp / 16);
        if (d <= *y1) {
            *y1 = y;
            *x1 = x + y / 2;
            return;
        }
    }
    *x1 += *y1 / 2;
    return;
}

template <typename PixelTraits>
static void convertup_3d(struct filter *f, int *x1, int *y1)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;

    struct threeddata *data = (struct threeddata *)f->data;
    int y = *y1;
    int x = *x1;
    unsigned int inp;
    unsigned int height = data->height;
    const pixel16_t *input;
    if (x >= f->childimage->width - 5)
        x = f->childimage->width - 6;
    if (y >= f->childimage->height - 3)
        y = f->childimage->height - 3;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    input = ((pixel16_t *)f->childimage->currlines[y] + y / 2);
    inp = (input[x] + input[x + 1] + input[x + 2] + input[x + 3] +
           input[x + 4] + input[x + 5]);
    input = ((pixel16_t *)f->childimage->currlines[y + 1] + y / 2);
    inp += (input[x] + input[x + 1] + input[x + 2] + input[x + 3] +
            input[x + 4] + input[x + 5]);
    input = ((pixel16_t *)f->childimage->currlines[y + 2] + y / 2);
    inp += (input[x] + input[x + 1] + input[x + 2] + input[x + 3] +
            input[x + 4] + input[x + 5]);
    *x1 -= *y1 / 2;
    *y1 = y - (inp / 16 > height ? height : inp / 16);
}

template <typename PixelTraits>
static void do_3d(void *dataptr, struct taskinfo */*task*/, int r1, int r2)
{
    using p = PixelTraits;
    using pixel_t = typename p::pixel_t;
    using ppixel_t = typename p::ppixel_t;
    using pixeldata_t = typename p::pixeldata_t;

    struct filter *f = (struct filter *)dataptr;
    unsigned int y;
    int maxinp = 0;
    unsigned int x;
    unsigned int end;
    unsigned int sum;
    pixel16_t const *input;
    unsigned int *lengths;
    unsigned int *sums;
    unsigned int *relsums;
    struct threeddata *data = (struct threeddata *)f->data;

    /* Copy to local variables to improve cse and memory references.  */
    unsigned int height = data->height;
    unsigned int stereogrammode = data->stereogrammode;
    unsigned int colheight = data->colheight;
    unsigned int midcolor = data->midcolor;
    unsigned int darkcolor = data->darkcolor;
    const unsigned int *pixels = data->pixels;
    ppixel_t *currlines = (ppixel_t *)f->image->currlines;
    struct inp {
        int max;
        unsigned int down;
    } * inpdata;

    lengths = (unsigned int *)malloc(sizeof(unsigned int) * f->image->width);
    inpdata = (struct inp *)malloc(sizeof(struct inp) * (f->image->width + 2));
    sums = (unsigned int *)malloc(sizeof(unsigned int) * (f->image->width + 2) *
                                  2);
    for (x = 0; x < (unsigned int)f->image->width; x++)
        lengths[x] = f->image->height - 1, sums[x * 2 + 0] = 0,
        sums[x * 2 + 1] = 0, inpdata[x].max = 0;
    sums[x * 2 + 0] = 0, sums[x * 2 + 1] = 0, inpdata[x].max = 0;
    inpdata[x + 1].max = 0;
    end = r2;
    for (y = f->childimage->height - 2; y > 0;) {
        y--;
        input = ((pixel16_t *)f->childimage->currlines[y] + y / 2);
        x = r1;
        relsums = sums + (y & 1);

        /* Fix boundary cases.  */
        /*relsums[0] = relsums[1];
           relsums[end*2-1] = relsums[end*2-2]; */
        inpdata[end + 1] = inpdata[end] = inpdata[end - 1];
        sum = input[x] + input[x + 1] + input[x + 2] + input[x + 3] +
              input[x + 4] + input[x + 5];

        while (x < end) {
            unsigned int inp;
            unsigned int d;

            /* Average pixel values of 5*3 square to get nicer shapes.  */
            sum += input[x + 6] - input[x];
            inp = sum + sums[x * 2 + 1] + sums[x * 2];
            relsums[x * 2] = sum;
            inpdata[x].down = inp;

            /* Calculate shades.  */
            maxinp = inpdata[x + 2].max;
            if ((int)inp > maxinp)
                inpdata[x].max = inp - 32;
            else
                inpdata[x].max = maxinp - 32;

            /* calculate top of mountain.  */
            d = inp / 16;
            d = y - (d > height ? height : d);

            /* Underflow */
            if (d > 65535U)
                d = 0;
            if (d < lengths[x]) {
                int y1;
                unsigned int color;
                if (stereogrammode)
                    color = pixels[y];
                else if (inp / 16 > height)
                    /*Red thinks on the top.  */
                    color =
                        pixels[inp / 16 >= colheight ? colheight : inp / 16];
                else {
                    int c;
                    /* Simple shading model.
                       Depends only on the preceding voxel.  */

                    c = ((int)inpdata[x + 2].down - (int)inp) / 8;

                    /* Get shades.  */
                    color = ((int)inp > maxinp ? midcolor : darkcolor) - c;
                    color =
                        pixels[color < 65535 ? (color < height ? color : height)
                                             : 0];
                }
                for (y1 = lengths[x]; y1 >= (int)d; y1--) {
                    p::setp(currlines[y1], x, color);
                }
                lengths[x] = d;
            }
            x++;
        }
    }
    free(lengths);
    free(inpdata);
    free(sums);
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
    struct threeddata *d = (struct threeddata *)f->data;
    struct filter *f1 = f;
    inhermisc(f, i);
    d->stereogrammode = 0;
    while (f1) {
        if (f1->action == &stereogram_filter)
            d->stereogrammode = 1;
        f1 = f1->next;
    }
    d->maxiter = -1;
    d->height = i->image->height / 3;
    if (d->pal != NULL)
        destroypalette(d->pal);
    d->pal = createpalette(0, 65536, LARGEITER, 0, 65536, NULL, NULL, NULL,
                           NULL, NULL);
    /*in/out coloring modes looks better in iter modes. This also saves some
       memory in truecolor. */
    if (i->image->palette->type == LARGEITER ||
        i->image->palette->type == SMALLITER) {
    } else {
        if (d->savedpalette == NULL)
            d->savedpalette = clonepalette(i->image->palette);

        mkgraypalette(i->image->palette);
    }
    if (d->pixels != NULL) {
        free(d->pixels);
        d->pixels = NULL;
    }
    if (!inherimage(f, i, TOUCHIMAGE | NEWIMAGE,
                    i->image->width + 6 +
                        (i->image->height + d->height + 6) / 2,
                    i->image->height + d->height + 6, d->pal,
                    i->image->pixelwidth, i->image->pixelheight * 2))
        return 0;
    setfractalpalette(f, d->savedpalette);
    fractalc_resize_to(f->fractalc,
                       f->childimage->pixelwidth * f->childimage->width,
                       f->childimage->pixelheight * f->childimage->height);
    f->fractalc->version++;
    return (f->previous->action->initialize(f->previous, i));
}

static struct filter *getinstance(const struct filteraction *a)
{
    struct filter *f = createfilter(a);
    struct threeddata *d = (struct threeddata *)calloc(sizeof(*d), 1);
    f->data = d;
    f->name = "3d";
    return (f);
}

static void destroyinstance(struct filter *f)
{
    struct threeddata *d = (struct threeddata *)f->data;
    if (d->pal != NULL)
        destroypalette((struct palette *)d->pal);
    if (d->savedpalette != NULL)
        destroypalette(d->savedpalette);
    if (d->pixels) {
        d->pixels = 0;
        free(d->pixels);
    }
    free(d);
    destroyinheredimage(f);
    free(f);
}

static int doit(struct filter *f, int flags, int time)
{
    int val;
    int size = f->childimage->palette->type == SMALLITER ? 240 : 65520;
    struct threeddata *d = (struct threeddata *)f->data;
    if (f->image->palette->size < size)
        size = f->image->palette->size;

    /* Update logarithmic scale palette.  */
    if (f->fractalc->maxiter != d->maxiter) {
        unsigned int i;
        int palsize = f->fractalc->maxiter;
        if (palsize >= 65536)
            palsize = 65535;
        d->colheight = d->height * (64 + 32) / 64;
        d->midcolor = d->height * 60 / 100;
        d->darkcolor = d->height * 30 / 100;
        d->pal->size = palsize;
        for (i = 0; i < (unsigned int)palsize; i++) {
            unsigned int y;
            y = (log10(1 + 10.0 * (i ? i : palsize) / palsize)) * d->colheight /
                9.0 * 16.0 / 2.0;
            /*y = (i ? i : palsize) * d->colheight / 9.0 / 2.0 * 16.0 / palsize;
             */
            if (y != d->pal->pixels[i])
                f->fractalc->version++;
            d->pal->pixels[i] = y;
        }
        d->maxiter = f->fractalc->maxiter;
        if (d->pixels)
            free(d->pixels);
        i = 0;
        if (d->stereogrammode) {
            d->pixels = (unsigned int *)malloc((f->childimage->height) *
                                               sizeof(*d->pixels));
            for (i = 0; i < (unsigned int)f->childimage->height; i++) {
                d->pixels[i] =
                    (f->childimage->height - i) * 255 / f->childimage->height;
            }
        } else {
            d->pixels =
                (unsigned int *)malloc((d->colheight + 5) * sizeof(*d->pixels));
            for (; i < d->colheight; i++) {
                int c = i * (f->image->palette->size) / d->colheight;
                if (c > f->image->palette->size - 1)
                    c = f->image->palette->size - 1;
                d->pixels[i] = f->image->palette->pixels[c];
            }
            d->pixels[i] = f->image->palette->pixels[0];
        }
    }
    updateinheredimage(f);
    val = f->previous->action->doit(f->previous, flags, time);
    drivercall(*f->image, xth_function(tpl::do_3d<Pixel8Traits>, f, f->image->width),
               xth_function(tpl::do_3d<Pixel16Traits>, f, f->image->width),
               xth_function(tpl::do_3d<Pixel24Traits>, f, f->image->width),
               xth_function(tpl::do_3d<Pixel32Traits>, f, f->image->width));
    xth_sync();
    return val;
}

static void myremove(struct filter *f)
{
    struct threeddata *d = (struct threeddata *)f->data;
    fractalc_resize_to(f->fractalc, f->image->width * f->image->pixelwidth,
                       f->image->height * f->image->pixelheight);
    if (d->savedpalette != NULL) {
        restorepalette(f->image->palette, d->savedpalette);
        destroypalette(d->savedpalette);
        d->savedpalette = NULL;
    }
}

static void convertup(struct filter *f, int *x, int *y)
{
    drivercall(*f->image, tpl::convertup_3d<Pixel8Traits>(f, x, y),
               tpl::convertup_3d<Pixel16Traits>(f, x, y),
               tpl::convertup_3d<Pixel24Traits>(f, x, y),
               tpl::convertup_3d<Pixel32Traits>(f, x, y));
    f->next->action->convertup(f->next, x, y);
}

static void convertdown(struct filter *f, int *x, int *y)
{
    drivercall(*f->image, tpl::convert_3d<Pixel8Traits>(f, x, y),
               tpl::convert_3d<Pixel16Traits>(f, x, y),
               tpl::convert_3d<Pixel24Traits>(f, x, y),
               tpl::convert_3d<Pixel32Traits>(f, x, y));
    if (f->previous != NULL)
        f->previous->action->convertdown(f->previous, x, y);
}

const struct filteraction threed_filter = {
    "Pseudo 3d", "threed",   0,         getinstance, destroyinstance, doit,
    requirement, initialize, convertup, convertdown, myremove};
