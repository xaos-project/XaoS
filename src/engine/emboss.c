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
struct embossdata
{
  struct palette *savedpalette, *palette;
  int xdist, ydist;
  unsigned int table[512];
};
static int
requirement (struct filter *f, struct requirements *r)
{
  f->req = *r;
  r->nimages = 1;
  r->flags &= ~(IMAGEDATA);
  r->supportedmask = GRAYSCALE | C256 | TRUECOLOR24 | TRUECOLOR | TRUECOLOR16;

  return (f->next->action->requirement (f->next, r));
}
static int
initialize (struct filter *f, struct initdata *i)
{
  int x;
  struct embossdata *s = (struct embossdata *) f->data;
  inhermisc (f, i);
  s->palette->size = 256 / 32;
  for (x = 0; x < 256 / 32; x++)
    s->palette->pixels[x] = x * 32;
#define SSTEP (32*8/64)
#define SSTEP2 (32*8/256)
  if (datalost (f, i) || i->image->version != f->imageversion)
    {
      if (s->savedpalette == NULL)
	s->savedpalette = clonepalette (i->image->palette);
      mkgraypalette (i->image->palette);
      if (i->image->palette->type & (C256 | GRAYSCALE))
	{
	  for (x = 0; x < 256; x++)
	    {
	      int dist = (x + SSTEP - 1) / SSTEP;
	      dist += 32;
	      if (dist > 63)
		dist = 63;
	      s->table[x] = i->image->palette->pixels[dist];
	    }
	  for (x = 256; x < 512; x++)
	    {
	      int dist = -(512 - x + SSTEP - 1) / SSTEP;
	      dist += 32;
	      if (dist < 0)
		dist = 0;
	      s->table[x] = i->image->palette->pixels[dist];
	    }
	}
      else
	{
	  for (x = 0; x < 256; x++)
	    {
	      int dist = (x + SSTEP2 - 1) / SSTEP2;
	      dist += 128;
	      if (dist > 255)
		dist = 255;
	      s->table[x] =
		((dist >> i->image->palette->info.truec.rprec) << i->
		 image->palette->info.truec.rshift) | ((dist >> i->image->
							palette->info.truec.
							gprec) << i->image->
						       palette->info.
						       truec.gshift) | ((dist
									 >>
									 i->
									 image->
									 palette->
									 info.
									 truec.
									 bprec)
									<< i->
									image->
									palette->
									info.
									truec.
									bshift);

	    }
	  for (x = 256; x < 512; x++)
	    {
	      int dist = -(512 - x + SSTEP2 - 1) / SSTEP2;
	      dist += 128;
	      if (dist < 0)
		dist = 0;
	      s->table[x] =
		((dist >> i->image->palette->info.truec.rprec) << i->
		 image->palette->info.truec.rshift) | ((dist >> i->image->
							palette->info.truec.
							gprec) << i->image->
						       palette->info.
						       truec.gshift) | ((dist
									 >>
									 i->
									 image->
									 palette->
									 info.
									 truec.
									 bprec)
									<< i->
									image->
									palette->
									info.
									truec.
									bshift);
	    }
	}
    }
  s->xdist = (int) (0.1 / i->image->pixelwidth);
  s->ydist = (int) (0.1 / i->image->pixelwidth);
  if (s->xdist < 1)
    s->xdist = 1;
  if (s->ydist < 1)
    s->ydist = 1;
  if (!inherimage
      (f, i, TOUCHIMAGE, i->image->width + s->xdist,
       i->image->height + s->ydist, s->palette, 0, 0))
    return 0;
  clear_image (f->image);
  setfractalpalette (f, s->savedpalette);
  return (f->previous->action->initialize (f->previous, i));
}
static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct embossdata *i = (struct embossdata *) calloc (1, sizeof (*i));
  i->savedpalette = NULL;
  i->palette =
    createpalette (0, 256, GRAYSCALE, 0, 256, NULL, NULL, NULL, NULL, NULL);
  f->childimage = NULL;
  f->data = i;
  f->name = "Emboss";
  return (f);
}
static void
emboss8 (void *data, struct taskinfo *task, int r1, int r2)
{
  pixel8_t *src, *srcend, *src2;
  pixel8_t *dest;
  struct filter *f = (struct filter *) data;
  struct embossdata *s = (struct embossdata *) f->data;
  int i;
  unsigned int *table = s->table;
  for (i = r1; i < r2; i++)
    {
      src = f->childimage->currlines[i];
      src2 = f->childimage->currlines[i + s->ydist] + s->xdist;
      srcend = src + f->image->width;
      dest = f->image->currlines[i];
      while (src < srcend)
	{
	  *dest = table[((int) *src2 - (int) *src) & 511];
	  src++;
	  src2++;
	  dest++;
	}
    }
}

#ifdef SUPPORT16
static void
emboss16 (void *data, struct taskinfo *task, int r1, int r2)
{
  pixel8_t *src, *srcend, *src2;
  pixel16_t *dest;
  struct filter *f = (struct filter *) data;
  struct embossdata *s = (struct embossdata *) f->data;
  int i;
  unsigned int *table = s->table;
  for (i = r1; i < r2; i++)
    {
      src = f->childimage->currlines[i];
      src2 = f->childimage->currlines[i + s->ydist] + s->xdist;
      srcend = src + f->image->width;
      dest = (pixel16_t *) f->image->currlines[i];
      while (src < srcend)
	{
	  *dest = table[((int) *src2 - (int) *src) & 511];
	  src++;
	  src2++;
	  dest++;
	}
    }
}
#endif
#ifdef STRUECOLOR24
static void
emboss24 (void *data, struct taskinfo *task, int r1, int r2)
{
  pixel8_t *src, *srcend, *src2;
  pixel8_t *dest;
  struct filter *f = (struct filter *) data;
  struct embossdata *s = (struct embossdata *) f->data;
  int i;
  unsigned int *table = s->table;
  for (i = r1; i < r2; i++)
    {
      src = f->childimage->currlines[i];
      src2 = f->childimage->currlines[i + s->ydist] + s->xdist;
      srcend = src + f->image->width;
      dest = (pixel8_t *) f->image->currlines[i];
      while (src < srcend)
	{
	  *dest = *(dest + 1) = *(dest + 2) =
	    table[((int) *src2 - (int) *src) & 511];
	  src++;
	  src2++;
	  dest += 3;
	}
    }
}
#endif
static void
emboss32 (void *data, struct taskinfo *task, int r1, int r2)
{
  pixel8_t *src, *srcend, *src2;
  pixel32_t *dest;
  struct filter *f = (struct filter *) data;
  struct embossdata *s = (struct embossdata *) f->data;
  int i;
  unsigned int *table = s->table;
  for (i = r1; i < r2; i++)
    {
      src = f->childimage->currlines[i];
      src2 = f->childimage->currlines[i + s->ydist] + s->xdist;
      srcend = src + f->image->width;
      dest = (pixel32_t *) f->image->currlines[i];
      while (src < srcend)
	{
	  *dest = table[((int) *src2 - (int) *src) & 511];
	  src++;
	  src2++;
	  dest++;
	}
    }
}
static void
destroyinstance (struct filter *f)
{
  struct embossdata *i = (struct embossdata *) f->data;
  if (i->savedpalette != NULL)
    destroypalette (i->savedpalette);
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
  updateinheredimage (f);
  val = f->previous->action->doit (f->previous, flags, time);
  drivercall (*f->image,
	      xth_function (emboss8, f, f->image->height),
	      xth_function (emboss16, f, f->image->height),
	      xth_function (emboss24, f, f->image->height),
	      xth_function (emboss32, f, f->image->height));
  xth_sync ();
  return val;
}
static void
myremovefilter (struct filter *f)
{
  struct embossdata *s = (struct embossdata *) f->data;
  if (s->savedpalette != NULL)
    {
      restorepalette (f->image->palette, s->savedpalette);
      destroypalette (s->savedpalette);
      s->savedpalette = NULL;
    }
}

CONST struct filteraction emboss_filter = {
  "Emboss",
  "emboss",
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
