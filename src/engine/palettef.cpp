#include "config.h"
#include <cstdlib>
#include <cstdio>
#include "filter.h"
#include "fractal.h"
#include "xthread.h"
struct palettedata {
    struct palette *palette;
    int active;
    unsigned int table[256];
};
/* Template functions for palette conversion (moved from paletted.h) */
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

static void mysetcolor(struct palette *p, int /*start*/, int /*end*/, rgb_t */*rgb*/)
{
    p->data = &p;
}

static int requirement(struct filter *f, struct requirements *r)
{
    f->req = *r;
    r->nimages = 1;
    r->flags &= ~(IMAGEDATA);
    r->supportedmask = MASK1BPP | MASK2BPP | MASK3BPP | MASK4BPP;

    return (f->next->action->requirement(f->next, r));
}

static int initialize(struct filter *f, struct initdata *i)
{
    struct palettedata *s = (struct palettedata *)f->data;
    inhermisc(f, i);
    if (i->image->palette->type != C256 ||
        i->image->palette->setpalette == NULL) {
        if (datalost(f, i) || i->image->version != f->imageversion ||
            !s->active) {
            if (!s->active) {
                struct palette *palette;
                palette = clonepalette(i->image->palette);
                restorepalette(s->palette, palette);
                destroypalette(palette);
            }
            s->palette->data = s;
            if (i->image->palette->maxentries < 256)
                s->palette->maxentries = i->image->palette->maxentries;
            else
                s->palette->maxentries = 256;
            s->active = 1;
        }
        if (!inherimage(f, i, TOUCHIMAGE | IMAGEDATA, 0, 0, s->palette, 0, 0))
            return 0;
        setfractalpalette(f, s->palette);
        f->queue->saveimage = f->childimage;
        f->queue->palettechg = f;
    } else {
        if (s->active) {
            f->image = i->image;
            restorepalette(f->image->palette, s->palette);
        }
        s->active = 0;
    }
    return (f->previous->action->initialize(f->previous, i));
}

static struct filter *getinstance(const struct filteraction *a)
{
    struct filter *f = createfilter(a);
    struct palettedata *i = (struct palettedata *)calloc(1, sizeof(*i));
    i->active = 0;
    i->palette =
        createpalette(0, 256, C256, 0, 256, NULL, mysetcolor, NULL, NULL, NULL);
    f->childimage = NULL;
    f->data = i;
    f->name = "Palette emulator";
    return (f);
}

static void destroyinstance(struct filter *f)
{
    struct palettedata *i = (struct palettedata *)f->data;
    destroypalette(i->palette);
    destroyinheredimage(f);
    free(f->data);
    free(f);
}

static int doit(struct filter *f, int flags, int time1)
{
    int val;
    int time = time1;
    struct palettedata *s = (struct palettedata *)f->data;
    if (s->active)
        updateinheredimage(f);
    if (flags & PALETTEONLY)
        val = 0;
    else
        val = f->previous->action->doit(f->previous, flags, time);
    if (s->active) {
        int i;
        if (s->palette->data != NULL) {
            val |= CHANGED;
            restorepalette(f->image->palette, f->childimage->palette);
            for (i = 0; i < 256; i++) {
                s->table[i] =
                    f->image->palette->pixels[i % f->image->palette->size];
            }
            s->palette->data = NULL;
        }
        drivercall(*f->image, xth_function(tpl::cpalette<Pixel8Traits>, f, f->image->height),
                   xth_function(tpl::cpalette<Pixel16Traits>, f, f->image->height),
                   xth_function(tpl::cpalette<Pixel24Traits>, f, f->image->height),
                   xth_function(tpl::cpalette<Pixel32Traits>, f, f->image->height));
        xth_sync();
    }
    return val;
}

static void myremovefilter(struct filter *f)
{
    struct palettedata *s = (struct palettedata *)f->data;
    if (s->active) {
        restorepalette(f->image->palette, s->palette);
    }
}

const struct filteraction palette_filter = {
    "Palette emulator", "palette",       0,
    getinstance,        destroyinstance, doit,
    requirement,        initialize,      convertupgeneric,
    convertdowngeneric, myremovefilter};
