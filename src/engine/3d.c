#ifndef _plan9_
#include <config.h>
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include <stdio.h>		/*for NULL */
#include <math.h>
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#endif
#define SLARGEITER
#include <xthread.h>
#include <filter.h>

struct threeddata
{
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

#define spixel_t pixel16_t
#include <c256.h>
#define do_3d do_3d8
#define convert_3d convert_3d8
#define convertup_3d convertup_3d8
#include "3dd.c"

#include <truecolor.h>
#define do_3d do_3d32
#define convert_3d convert_3d32
#define convertup_3d convertup_3d32
#include "3dd.c"

#include <true24.h>
#define do_3d do_3d24
#define convert_3d convert_3d24
#define convertup_3d convertup_3d24
#include "3dd.c"

#include <hicolor.h>
#define do_3d do_3d16
#define convert_3d convert_3d16
#define convertup_3d convertup_3d16
#include "3dd.c"

static int
requirement (struct filter *f, struct requirements *r)
{
  f->req = *r;
  r->nimages = 1;
  r->flags &= ~IMAGEDATA;
  r->supportedmask = MASK1BPP | MASK3BPP | MASK2BPP | MASK4BPP;
  return (f->next->action->requirement (f->next, r));
}
extern CONST struct filteraction stereogram_filter;
static int
initialize (struct filter *f, struct initdata *i)
{
  struct threeddata *d = (struct threeddata *) f->data;
  struct filter *f1 = f;
  inhermisc (f, i);
  d->stereogrammode = 0;
  while (f1)
    {
      if (f1->action == &stereogram_filter)
	d->stereogrammode = 1;
      f1 = f1->next;
    }
  d->maxiter = -1;
  d->height = i->image->height / 3;
  if (d->pal != NULL)
    destroypalette (d->pal);
  d->pal =
    createpalette (0, 65536, LARGEITER, 0, 65536, NULL, NULL, NULL, NULL,
		   NULL);
  /*in/out coloring modes looks better in iter modes. This also saves some
     memory in truecolor. */
  if (i->image->palette->type == LARGEITER
      || i->image->palette->type == SMALLITER)
    {
    }
  else
    {
      if (d->savedpalette == NULL)
	d->savedpalette = clonepalette (i->image->palette);

      mkgraypalette (i->image->palette);
    }
  if (d->pixels != NULL)
    {
      free (d->pixels);
      d->pixels = NULL;
    }
  if (!inherimage
      (f, i, TOUCHIMAGE | NEWIMAGE,
       i->image->width + 6 + (i->image->height + d->height + 6) / 2,
       i->image->height + d->height + 6, d->pal, i->image->pixelwidth,
       i->image->pixelheight * 2))
    return 0;
  setfractalpalette (f, d->savedpalette);
  fractalc_resize_to (f->fractalc,
		      f->childimage->pixelwidth * f->childimage->width,
		      f->childimage->pixelheight * f->childimage->height);
  f->fractalc->version++;
  return (f->previous->action->initialize (f->previous, i));
}
static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct threeddata *d = calloc (sizeof (*d), 1);
  f->data = d;
  f->name = "3d";
  return (f);
}
static void
destroyinstance (struct filter *f)
{
  struct threeddata *d = (struct threeddata *) f->data;
  if (d->pal != NULL)
    destroypalette ((struct palette *) d->pal);
  if (d->savedpalette != NULL)
    destroypalette (d->savedpalette);
  if (d->pixels)
    {
      d->pixels = 0;
      free (d->pixels);
    }
  free (d);
  destroyinheredimage (f);
  free (f);
}
static int
doit (struct filter *f, int flags, int time)
{
  int val;
  int size = f->childimage->palette->type == SMALLITER ? 240 : 65520;
  struct threeddata *d = (struct threeddata *) f->data;
  if (f->image->palette->size < size)
    size = f->image->palette->size;

  /* Update logarithmic scale palette.  */
  if (f->fractalc->maxiter != d->maxiter)
    {
      unsigned int i;
      int palsize = f->fractalc->maxiter;
      if (palsize >= 65536)
	palsize = 65535;
      d->colheight = d->height * (64 + 32) / 64;
      d->midcolor = d->height * 60 / 100;
      d->darkcolor = d->height * 30 / 100;
      d->pal->size = palsize;
      for (i = 0; i < (unsigned int) palsize; i++)
	{
	  unsigned int y;
	  y =
	    (log10 (1 + 10.0 * (i ? i : palsize) / palsize)) * d->colheight /
	    9.0 * 16.0 / 2.0;
	  /*y = (i ? i : palsize) * d->colheight / 9.0 / 2.0 * 16.0 / palsize; */
	  if (y != d->pal->pixels[i])
	    f->fractalc->version++;
	  d->pal->pixels[i] = y;
	}
      d->maxiter = f->fractalc->maxiter;
      if (d->pixels)
	free (d->pixels);
      i = 0;
      if (d->stereogrammode)
	{
	  d->pixels = malloc ((f->childimage->height) * sizeof (*d->pixels));
	  for (i = 0; i < (unsigned int) f->childimage->height; i++)
	    {
	      d->pixels[i] =
		(f->childimage->height - i) * 255 / f->childimage->height;
	    }
	}
      else
	{
	  d->pixels = malloc ((d->colheight + 5) * sizeof (*d->pixels));
	  for (; i < d->colheight; i++)
	    {
	      int c = i * (f->image->palette->size) / d->colheight;
	      if (c > f->image->palette->size - 1)
		c = f->image->palette->size - 1;
	      d->pixels[i] = f->image->palette->pixels[c];
	    }
	  d->pixels[i] = f->image->palette->pixels[0];
	}
    }
  updateinheredimage (f);
  val = f->previous->action->doit (f->previous, flags, time);
  drivercall (*f->image,
	      xth_function (do_3d8, f, f->image->width),
	      xth_function (do_3d16, f, f->image->width),
	      xth_function (do_3d24, f, f->image->width),
	      xth_function (do_3d32, f, f->image->width));
  xth_sync ();
  return val;
}
static void
myremove (struct filter *f)
{
  struct threeddata *d = (struct threeddata *) f->data;
  fractalc_resize_to (f->fractalc, f->image->width * f->image->pixelwidth,
		      f->image->height * f->image->pixelheight);
  if (d->savedpalette != NULL)
    {
      restorepalette (f->image->palette, d->savedpalette);
      destroypalette (d->savedpalette);
      d->savedpalette = NULL;
    }

}

static void
convertup (struct filter *f, int *x, int *y)
{
  drivercall (*f->image,
	      convertup_3d8 (f, x, y),
	      convertup_3d16 (f, x, y),
	      convertup_3d24 (f, x, y), convertup_3d32 (f, x, y));
  f->next->action->convertup (f->next, x, y);
}
static void
convertdown (struct filter *f, int *x, int *y)
{
  drivercall (*f->image,
	      convert_3d8 (f, x, y),
	      convert_3d16 (f, x, y),
	      convert_3d24 (f, x, y), convert_3d32 (f, x, y));
  if (f->previous != NULL)
    f->previous->action->convertdown (f->previous, x, y);
}

CONST struct filteraction threed_filter = {
  "Pseudo 3d",
  "threed",
  0,
  getinstance,
  destroyinstance,
  doit,
  requirement,
  initialize,
  convertup,
  convertdown,
  myremove
};
