#include "config.h"
#include "config.h"
#include <climits>
#include <cstdlib>
#include "filter.h"
#include "xthread.h"

struct starfielddata {
    struct palette *palette;
    struct palette *savedpalette;
};

static unsigned int state;
static inline void mysrandom(unsigned int x) { state = x; }

#define MYLONG_MAX 0xffffff /*this is enough for me. */
static inline unsigned int myrandom(void)
{
    state = ((state * 1103515245) + 12345) & MYLONG_MAX;
    return state;
}

#define IMAGETYPE SMALLITER
/* Template functions for starfield effect (moved from stard.h) */
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
static int requirement(struct filter *f, struct requirements *r)
{
    f->req = *r;
    r->nimages = 1;
    r->flags &= ~IMAGEDATA;
    r->supportedmask = C256 | TRUECOLOR24 | TRUECOLOR | TRUECOLOR16 | GRAYSCALE;
    return (f->next->action->requirement(f->next, r));
}

static int initialize(struct filter *f, struct initdata *i)
{
    struct starfielddata *s = (struct starfielddata *)f->data;
    inhermisc(f, i);
    if (s->savedpalette == NULL)
        s->savedpalette = clonepalette(i->image->palette);
    mkstarfieldpalette(i->image->palette);
    if (!inherimage(f, i, TOUCHIMAGE, 0, 0, s->palette, 0, 0)) {
        return 0;
    }
    setfractalpalette(f, s->savedpalette);
    return (f->previous->action->initialize(f->previous, i));
}

static struct filter *getinstance(const struct filteraction *a)
{
    struct filter *f = createfilter(a);
    struct starfielddata *i = (struct starfielddata *)calloc(1, sizeof(*i));
    i->savedpalette = NULL;
    i->palette = createpalette(0, 65536, IMAGETYPE, 0, 65536, NULL, NULL, NULL,
                               NULL, NULL);
    f->data = i;
    f->name = "Starfield";
    return (f);
}

static void destroyinstance(struct filter *f)
{
    struct starfielddata *i = (struct starfielddata *)f->data;
    if (i->savedpalette != NULL)
        destroypalette(i->savedpalette);
    destroypalette(i->palette);
    destroyinheredimage(f);
    free(f);
    free(i);
}

static int doit(struct filter *f, int flags, int time)
{
    int val;
    val = f->previous->action->doit(f->previous, flags, time);
    drivercall(*f->image, xth_function(tpl::do_starfield<Pixel8Traits>, f, f->image->height),
               xth_function(tpl::do_starfield<Pixel16Traits>, f, f->image->height),
               xth_function(tpl::do_starfield<Pixel24Traits>, f, f->image->height),
               xth_function(tpl::do_starfield<Pixel32Traits>, f, f->image->height));
    xth_sync();
    return val | CHANGED;
}

static void myremovefilter(struct filter *f)
{
    struct starfielddata *s = (struct starfielddata *)f->data;
    if (s->savedpalette != NULL) {
        restorepalette(f->image->palette, s->savedpalette);
        destroypalette(s->savedpalette);
        s->savedpalette = NULL;
    }
}

const struct filteraction starfield_filter = {
    "Starfield",        "starfield",     0,
    getinstance,        destroyinstance, doit,
    requirement,        initialize,      convertupgeneric,
    convertdowngeneric, myremovefilter};
