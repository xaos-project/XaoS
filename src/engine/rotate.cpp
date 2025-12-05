/* An rotation filter. Uses bressemham algorithm combined with dda to rotate
 * image around center
 * This filter is used internally by XaoS and is unvisible to normal user in
 * 'E' menu.
 * It is used to implement fast rotation mode
 */
#include "config.h"
#include <cstring>
#include <climits>

#include <cmath>
#include <cstdlib>
#define SLARGEITER
#include "xthread.h"
#include "filter.h"

struct rotatedata {
    number_t angle;
    number_t x1, y1, x2, y2, xx1, yy1, xx2, yy2;
};

/* Template functions for image rotation (moved from rotated.h) */
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

static int requirement(struct filter *f, struct requirements *r)
{
    f->req = *r;
    r->nimages = 1;
    r->flags &= ~IMAGEDATA;
    r->supportedmask = MASK1BPP | MASK2BPP | MASK3BPP | MASK4BPP;
    return (f->next->action->requirement(f->next, r));
}

static int initialize(struct filter *f, struct initdata *i)
{
    float size, pixelsize;
    struct rotatedata *s = (struct rotatedata *)f->data;
    inhermisc(f, i);
    s->angle = INT_MAX;
    /*in/out coloring modes looks better in iter modes. This also saves some
       memory in truecolor. */
    if (i->image->pixelwidth < i->image->pixelheight)
        pixelsize = i->image->pixelwidth;
    else
        pixelsize = i->image->pixelheight;
    size = sqrt(i->image->width * i->image->width * i->image->pixelwidth *
                    i->image->pixelwidth +
                i->image->height * i->image->height * i->image->pixelheight *
                    i->image->pixelheight);
    if (!inherimage(f, i, TOUCHIMAGE | NEWIMAGE, (int)(size / pixelsize + 1),
                    (int)(size / pixelsize + 1), NULL, pixelsize, pixelsize))
        return 0;
    return (f->previous->action->initialize(f->previous, i));
}

static struct filter *getinstance(const struct filteraction *a)
{
    struct filter *f = createfilter(a);
    struct rotatedata *i = (struct rotatedata *)calloc(1, sizeof(*i));
    f->name = "Rotation filter";
    f->data = i;
    return (f);
}

static void destroyinstance(struct filter *f)
{
    free(f->data);
    destroyinheredimage(f);
    free(f);
}

static int doit(struct filter *f, int flags, int time)
{
    int val;
    struct rotatedata *s = (struct rotatedata *)f->data;
    number_t angle = f->fractalc->angle;
    number_t wx = f->fractalc->windowwidth, wy = f->fractalc->windowheight;
    number_t rr = f->fractalc->s.rr, ir = f->fractalc->s.ri;
    f->fractalc->windowwidth = f->fractalc->windowheight =
        f->childimage->width * f->childimage->pixelwidth;
    f->fractalc->s.rr *= f->fractalc->windowwidth / wx;
    f->fractalc->s.ri *= f->fractalc->windowheight / wy;
    f->fractalc->windowwidth = f->fractalc->windowheight = 1;
    f->fractalc->angle = 0;
    update_view(f->fractalc); /*update rotation tables */
    updateinheredimage(f);
    val = f->previous->action->doit(f->previous, flags, time);
    f->fractalc->angle = angle;
    update_view(f->fractalc); /*update rotation tables */
    f->fractalc->s.rr = rr;
    f->fractalc->s.ri = ir;
    f->fractalc->windowwidth = wx;
    f->fractalc->windowheight = wy;
    if ((val & CHANGED) || s->angle != angle) {
        s->xx2 = f->image->width * f->image->pixelwidth / 2;
        s->yy2 = f->image->height * f->image->pixelheight / 2;
        s->x1 = -s->xx2;
        s->y1 = -s->yy2;
        s->x2 = -s->xx2;
        s->y2 = s->yy2;
        s->xx1 = s->xx2;
        s->yy1 = -s->yy2;
        rotateback(*f->fractalc, s->x1, s->y1);
        rotateback(*f->fractalc, s->x2, s->y2);
        rotateback(*f->fractalc, s->xx1, s->yy1);
        rotateback(*f->fractalc, s->xx2, s->yy2);
        s->x1 /= f->childimage->pixelwidth;
        s->x1 += f->childimage->width / 2;
        s->y1 /= f->childimage->pixelwidth;
        s->y1 += f->childimage->width / 2;
        s->xx1 /= f->childimage->pixelwidth;
        s->xx1 += f->childimage->width / 2;
        s->yy1 /= f->childimage->pixelwidth;
        s->yy1 += f->childimage->width / 2;
        s->x2 /= f->childimage->pixelwidth;
        s->x2 += f->childimage->width / 2;
        s->y2 /= f->childimage->pixelwidth;
        s->y2 += f->childimage->width / 2;
        s->xx2 /= f->childimage->pixelwidth;
        s->xx2 += f->childimage->width / 2;
        s->yy2 /= f->childimage->pixelwidth;
        s->yy2 += f->childimage->width / 2;

        drivercall(*f->image, xth_function(tpl::do_rotate<Pixel8Traits>, f, f->image->height),
                   xth_function(tpl::do_rotate<Pixel16Traits>, f, f->image->height),
                   xth_function(tpl::do_rotate<Pixel24Traits>, f, f->image->height),
                   xth_function(tpl::do_rotate<Pixel32Traits>, f, f->image->height));
        xth_sync();
        val |= CHANGED;
    }
    return val;
}

static void convertup(struct filter *f, int *x, int *y)
{
    number_t xd = (*x - f->childimage->width / 2) * f->childimage->pixelwidth;
    number_t yd = (*y - f->childimage->height / 2) * f->childimage->pixelheight;
    *x = (int)(f->image->width / 2 + xd / f->image->pixelwidth);
    *y = (int)(f->image->height / 2 + yd / f->image->pixelheight);
    if (f->next != NULL)
        f->next->action->convertup(f->next, x, y);
}

static void convertdown(struct filter *f, int *x, int *y)
{
    number_t xd = (*x - f->image->width / 2) * f->image->pixelwidth;
    number_t yd = (*y - f->image->height / 2) * f->image->pixelheight;
    *x = (int)(f->childimage->width / 2 + xd / f->childimage->pixelwidth);
    *y = (int)(f->childimage->height / 2 + yd / f->childimage->pixelheight);
    if (f->previous != NULL)
        f->previous->action->convertdown(f->previous, x, y);
}

const struct filteraction rotate_filter = {
    "Image rotation", "rotate",    0,           getinstance,
    destroyinstance,  doit,        requirement, initialize,
    convertup,        convertdown, NULL};
