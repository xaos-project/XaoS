#ifndef _plan9_
#include <config.h>
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <stdlib.h>
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#endif
#include <xthread.h>
#include <filter.h>

#define PIXELWIDTH (f->image->pixelwidth)	/*all distances in cm */
#define PIXELHEIGHT (f->image->pixelheight)

#define USER_DIST  (60.0)
#define INDEX_DIST (0.3)
#define EYE_DIST   (8.5)
#define START1 (60.0)
#define FNC(x) x

#define NCOLORS 256
#define IMAGETYPE SMALLITER
#define spixel_t pixel8_t
static int *table;
struct stereogramdata
{
  int minc;
  struct palette *palette;
  struct palette *savedpalette;
};

#include <c256.h>
#define do_stereogram do_stereogram8
#include "stereod.c"

#include <hicolor.h>
#define do_stereogram do_stereogram16
#include "stereod.c"

#include <true24.h>
#define do_stereogram do_stereogram24
#include "stereod.c"

#include <truecolor.h>
#define do_stereogram do_stereogram32
#include "stereod.c"

static int
requirement (struct filter *f, struct requirements *r)
{
  f->req = *r;
  r->nimages = 1;
  r->flags &= ~IMAGEDATA;
  r->supportedmask = C256 | TRUECOLOR | TRUECOLOR24 | TRUECOLOR16 | GRAYSCALE;
  return (f->next->action->requirement (f->next, r));
}
static int
initialize (struct filter *f, struct initdata *i)
{
  struct stereogramdata *s = (struct stereogramdata *) f->data;
  inhermisc (f, i);
  if (s->savedpalette == NULL)
    s->savedpalette = clonepalette (i->image->palette);
  mkstereogrampalette (i->image->palette);
  if (!inherimage
      (f, i, TOUCHIMAGE, i->image->width / 2, (i->image->height) / 2,
       s->palette, i->image->pixelwidth * 2, i->image->pixelheight * 2))
    return 0;
  setfractalpalette (f, s->savedpalette);
  return (f->previous->action->initialize (f->previous, i));
}
static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct stereogramdata *i =
    (struct stereogramdata *) calloc (1, sizeof (*i));
  i->minc = 0;
  i->savedpalette = NULL;
  i->palette =
    createpalette (0, 65536, IMAGETYPE, 0, 65536, NULL, NULL, NULL, NULL,
		   NULL);
  f->data = i;
  f->name = "Random dot stereogram";
  return (f);
}
static void
destroyinstance (struct filter *f)
{
  struct stereogramdata *i = (struct stereogramdata *) f->data;
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
  struct stereogramdata *s = (struct stereogramdata *) f->data;
  int i, y;
  double start, maxdist, dist;
  updateinheredimage (f);
  if (f->fractalc->maxiter < NCOLORS)
    s->palette->size = f->fractalc->maxiter;
  else
    s->palette->size = NCOLORS;
  val = f->previous->action->doit (f->previous, flags, time);
#ifdef HAVE_ALLOCA
  table = (int *) alloca (sizeof (int) * NCOLORS);
#else
  table = (int *) malloc (sizeof (int) * NCOLORS);
#endif
  dist = (f->fractalc->s.rr) / 2;
  maxdist = INDEX_DIST * FNC (f->fractalc->maxiter) + START1;
  do
    {
      start = dist * maxdist - INDEX_DIST * FNC (s->minc);
      maxdist *= 5;
    }
  while (start + INDEX_DIST * (FNC (s->minc)) < 25.0);
  if (f->fractalc->maxiter < NCOLORS)
    y = f->fractalc->maxiter;
  else
    y = NCOLORS;
  if (y < 256)
    y = 256;
  for (i = 0; i < y; i++)
    {
      double dist;
      if (i != 0)
	dist = i;
      else
	dist = y - 1;
      dist = INDEX_DIST * (FNC (dist)) + start;
      table[i] = (int) (EYE_DIST * dist / (dist + USER_DIST) / PIXELWIDTH);
    }
  drivercall (*f->image,
	      xth_function (do_stereogram8, f, f->childimage->height),
	      xth_function (do_stereogram16, f, f->childimage->height),
	      xth_function (do_stereogram24, f, f->childimage->height),
	      xth_function (do_stereogram32, f, f->childimage->height));
  xth_sync ();
#ifndef HAVE_ALLOCA
  free (table);
#endif
  return val;
}
static void
convertup (struct filter *f, int *x, int *y)
{
  *y *= 2;
  *x *= 2;
  f->next->action->convertup (f->next, x, y);
}
static void
convertdown (struct filter *f, int *x, int *y)
{
  *y /= 2;
  *x /= 2;
  if (f->previous != NULL)
    f->previous->action->convertdown (f->previous, x, y);
}
static void
myremovefilter (struct filter *f)
{
  struct stereogramdata *s = (struct stereogramdata *) f->data;
  if (s->savedpalette != NULL)
    {
      restorepalette (f->image->palette, s->savedpalette);
      destroypalette (s->savedpalette);
      s->savedpalette = NULL;
    }
}

CONST struct filteraction stereogram_filter = {
  "Random dot stereogram",
  "stereogram",
  0,
  getinstance,
  destroyinstance,
  doit,
  requirement,
  initialize,
  convertup,
  convertdown,
  myremovefilter
};
