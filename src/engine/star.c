#include <config.h>
#ifndef _plan9_
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include <config.h>
#include <limits.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <stdlib.h>
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#endif
#include <filter.h>
#include <xthread.h>

struct starfielddata
{
  struct palette *palette;
  struct palette *savedpalette;
};

static unsigned int state;
static INLINE void
mysrandom (unsigned int x)
{
  state = x;
}

#define MYLONG_MAX 0xffffff	/*this is enought for me. */
static INLINE unsigned int
myrandom (void)
{
  state = ((state * 1103515245) + 12345) & MYLONG_MAX;
  return state;
}

#define IMAGETYPE SMALLITER
#include <c256.h>
#define do_starfield do_starfield8
#include "stard.c"
#include <hicolor.h>
#define do_starfield do_starfield16
#include "stard.c"
#include <true24.h>
#define do_starfield do_starfield24
#include "stard.c"
#include <truecolor.h>
#define do_starfield do_starfield32
#include "stard.c"
static int
requirement (struct filter *f, struct requirements *r)
{
  f->req = *r;
  r->nimages = 1;
  r->flags &= ~IMAGEDATA;
  r->supportedmask = C256 | TRUECOLOR24 | TRUECOLOR | TRUECOLOR16 | GRAYSCALE;
  return (f->next->action->requirement (f->next, r));
}
static int
initialize (struct filter *f, struct initdata *i)
{
  struct starfielddata *s = (struct starfielddata *) f->data;
  inhermisc (f, i);
  if (s->savedpalette == NULL)
    s->savedpalette = clonepalette (i->image->palette);
  mkstarfieldpalette (i->image->palette);
  if (!inherimage (f, i, TOUCHIMAGE, 0, 0, s->palette, 0, 0))
    {
      return 0;
    }
  setfractalpalette (f, s->savedpalette);
  return (f->previous->action->initialize (f->previous, i));
}
static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct starfielddata *i = (struct starfielddata *) calloc (1, sizeof (*i));
  i->savedpalette = NULL;
  i->palette =
    createpalette (0, 65536, IMAGETYPE, 0, 65536, NULL, NULL, NULL, NULL,
		   NULL);
  f->data = i;
  f->name = "Starfield";
  return (f);
}
static void
destroyinstance (struct filter *f)
{
  struct starfielddata *i = (struct starfielddata *) f->data;
  if (i->savedpalette != NULL)
    destroypalette (i->savedpalette);
  destroypalette (i->palette);
  destroyinheredimage (f);
  free (f);
  free (i);
}
static int
doit (struct filter *f, int flags, int time)
{
  int val;
  val = f->previous->action->doit (f->previous, flags, time);
  drivercall (*f->image,
	      xth_function (do_starfield8, f, f->image->height),
	      xth_function (do_starfield16, f, f->image->height),
	      xth_function (do_starfield24, f, f->image->height),
	      xth_function (do_starfield32, f, f->image->height));
  xth_sync ();
  return val | CHANGED;
}

static void
myremovefilter (struct filter *f)
{
  struct starfielddata *s = (struct starfielddata *) f->data;
  if (s->savedpalette != NULL)
    {
      restorepalette (f->image->palette, s->savedpalette);
      destroypalette (s->savedpalette);
      s->savedpalette = NULL;
    }
}

CONST struct filteraction starfield_filter = {
  "Starfield",
  "starfield",
  0,
  getinstance,
  destroyinstance,
  doit,
  requirement,
  initialize,
  convertupgeneric,
  convertdowngeneric,
  myremovefilter
};
