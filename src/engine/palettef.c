#include <config.h>
#ifndef _plan9_
#ifdef NO_MALLOC_H
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <stdio.h>
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#endif
#include <filter.h>
#include <fractal.h>
#include <xthread.h>
struct palettedata
{
  struct palette *palette;
  int active;
  unsigned int table[256];
};
#include <c256.h>
#define cpalette palette8
#include "paletted.c"

#include <truecolor.h>
#define cpalette palette32
#include "paletted.c"

#include <true24.h>
#define cpalette palette24
#include "paletted.c"

#include <hicolor.h>
#define cpalette palette16
#include "paletted.c"

static void
mysetcolor (struct palette *p, int start, int end, rgb_t * rgb)
{
  p->data = &p;
}

static int
requirement (struct filter *f, struct requirements *r)
{
  f->req = *r;
  r->nimages = 1;
  r->flags &= ~(IMAGEDATA);
  r->supportedmask = MASK1BPP | MASK2BPP | MASK3BPP | MASK4BPP;

  return (f->next->action->requirement (f->next, r));
}
static int
initialize (struct filter *f, struct initdata *i)
{
  struct palettedata *s = (struct palettedata *) f->data;
  inhermisc (f, i);
  if (i->image->palette->type != C256
      || i->image->palette->setpalette == NULL)
    {
      if (datalost (f, i) || i->image->version != f->imageversion
	  || !s->active)
	{
	  if (!s->active)
	    {
	      struct palette *palette;
	      palette = clonepalette (i->image->palette);
	      restorepalette (s->palette, palette);
	      destroypalette (palette);
	    }
	  s->palette->data = s;
	  if (i->image->palette->maxentries < 256)
	    s->palette->maxentries = i->image->palette->maxentries;
	  else
	    s->palette->maxentries = 256;
	  s->active = 1;
	}
      if (!inherimage (f, i, TOUCHIMAGE | IMAGEDATA, 0, 0, s->palette, 0, 0))
	return 0;
      setfractalpalette (f, s->palette);
      f->queue->saveimage = f->childimage;
      f->queue->palettechg = f;
    }
  else
    {
      if (s->active)
	{
	  f->image = i->image;
	  restorepalette (f->image->palette, s->palette);
	}
      s->active = 0;
    }
  return (f->previous->action->initialize (f->previous, i));
}
static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct palettedata *i = (struct palettedata *) calloc (1, sizeof (*i));
  i->active = 0;
  i->palette =
    createpalette (0, 256, C256, 0, 256, NULL, mysetcolor, NULL, NULL, NULL);
  f->childimage = NULL;
  f->data = i;
  f->name = "Palette emulator";
  return (f);
}
static void
destroyinstance (struct filter *f)
{
  struct palettedata *i = (struct palettedata *) f->data;
  destroypalette (i->palette);
  destroyinheredimage (f);
  free (f->data);
  free (f);
}

static int
doit (struct filter *f, int flags, int time1)
{
  int val;
  int time = time1;
  struct palettedata *s = (struct palettedata *) f->data;
  if (s->active)
    updateinheredimage (f);
  if (flags & PALETTEONLY)
    val = 0;
  else
    val = f->previous->action->doit (f->previous, flags, time);
  if (s->active)
    {
      int i;
      if (s->palette->data != NULL)
	{
	  val |= CHANGED;
	  restorepalette (f->image->palette, f->childimage->palette);
	  for (i = 0; i < 256; i++)
	    {
	      s->table[i] =
		f->image->palette->pixels[i % f->image->palette->size];
	    }
	  s->palette->data = NULL;
	}
      drivercall (*f->image,
		  xth_function (palette8, f, f->image->height),
		  xth_function (palette16, f, f->image->height),
		  xth_function (palette24, f, f->image->height),
		  xth_function (palette32, f, f->image->height));
      xth_sync ();
    }
  return val;
}
static void
myremovefilter (struct filter *f)
{
  struct palettedata *s = (struct palettedata *) f->data;
  if (s->active)
    {
      restorepalette (f->image->palette, s->palette);
    }
}

CONST struct filteraction palette_filter = {
  "Palette emulator",
  "palette",
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
