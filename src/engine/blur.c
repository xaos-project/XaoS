#include <config.h>
#ifndef _plan9_
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include <string.h>
#include <math.h>
#include <stdio.h>
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
#include <fractal.h>
#include <xthread.h>
#include <xerror.h>
#include <archaccel.h>
#define SIZE 64
#define SIZE2 8
#define AMOUNT 0.005
#define DIV 1000.0
#define MAXFRAMES 800*1000	/*after 800 frames should be OK */
struct blurdata
{
  int bckuptime;
  int counter;
  struct palette *savedpalette, *palette;
  unsigned char (*table)[256];	/*Used by blur routines */
  int n;
};
static int
requirement (struct filter *f, struct requirements *r)
{
  f->req = *r;
  r->nimages = 1;
  r->flags |= IMAGEDATA;
  r->supportedmask = C256 | TRUECOLOR24 | TRUECOLOR | TRUECOLOR16 | GRAYSCALE;
  return (f->next->action->requirement (f->next, r));
}
static void
blur8 (struct filter *f)
{
  struct image *desti = f->image;
  struct blurdata *s = (struct blurdata *) f->data;
  unsigned char (*table)[256] = s->table;
  unsigned int i, i1, im, im1, ipl, ii;
  if (f->image->palette->type == C256)
    for (i = im = 0; i < SIZE; i++, im += 256 - s->n)
      {
	for (i1 = im1 = 0; i1 < SIZE2; i1++, im1 += s->n * (SIZE / SIZE2))
	  {
	    ipl = (im + im1 + 128) >> 8;
	    ii = i1 * (SIZE / SIZE2);
	    if (ipl == i && i != ii)
	      {
		if (i < ii)
		  ipl++;
		else
		  ipl--;
	      }
	    ii = desti->palette->pixels[i];
	    table[i1][ii] = (unsigned char) desti->palette->pixels[ipl];
	  }
      }
  else
    for (i = im = desti->palette->start, im *= 256 - s->n;
	 i < (unsigned int) desti->palette->end; i++, im += 256 - s->n)
      {
	for (i1 = im1 = desti->palette->start, im1 *= s->n;
	     i1 < (unsigned int) desti->palette->end; i1++, im1 += s->n)
	  {
	    ipl = (im + im1 + 128) >> 8;
	    if (ipl == i && i != i1)
	      {
		if (i < i1)
		  ipl++;
		else
		  ipl--;
	      }
	    table[i1][i] = (unsigned char) ipl;
	  }
      }
}
static void
blurtruecolor (struct filter *f)
{
  struct blurdata *s = (struct blurdata *) f->data;
  unsigned int i, i1, im, im1;
  unsigned char (*table)[256] = s->table;
  for (i = im = 0; i < 256; i++, im += 256 - s->n)
    {
      for (i1 = im1 = 0; i1 < 256; i1++, im1 += s->n)
	table[i1][i] = (unsigned char) ((im + im1) >> 8);
    }
}
static void
clear_image2 (struct image *img)
{
  int i;
  int color = img->palette->pixels[0];
  int width = img->width * img->bytesperpixel;
  if (!width)
    width = (img->width + 7) / 8;
  for (i = 0; i < img->height; i++)
    memset_long (img->currlines[i], color, (size_t) width);
}


static int
initialize (struct filter *f, struct initdata *i)
{
  struct blurdata *s = (struct blurdata *) f->data;
  unsigned int x;
  inhermisc (f, i);
  s->counter = 0;
  s->palette->size = SIZE2;
  for (x = 0; x < SIZE2; x++)
    s->palette->pixels[x] = x;
  if (datalost (f, i) || i->image->version != f->imageversion)
    {
      s->bckuptime = MAXFRAMES;
      s->counter = MAXFRAMES;
      if (i->image->palette->type == C256)
	{
	  if (s->savedpalette == NULL)
	    s->savedpalette = clonepalette (i->image->palette);
	  mkblurpalette (i->image->palette);
	}
      else
	{
	  if (s->savedpalette != NULL)
	    {
	      restorepalette (i->image->palette, s->savedpalette);
	      destroypalette (s->savedpalette);
	      s->savedpalette = NULL;
	    }
	}
      clear_image2 (i->image);
    }
  if (!inherimage
      (f, i, TOUCHIMAGE | IMAGEDATA, 0, 0,
       i->image->palette->type == C256 ? s->palette : NULL, 0, 0))
    return 0;
  if (f->image->palette->type == C256)
    {
      setfractalpalette (f, s->savedpalette);
    }
  if (i->image == NULL)
    {
      return 0;
    }
  return (f->previous->action->initialize (f->previous, i));
}
static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct blurdata *i = (struct blurdata *) calloc (1, sizeof (*i));
  i->savedpalette = NULL;
  i->palette =
    createpalette (0, 256, SMALLITER, 0, 256, NULL, NULL, NULL, NULL, NULL);
  i->palette->size = SIZE2;
  i->palette->end = SIZE2;
  i->table = NULL;
  f->childimage = NULL;
  f->data = i;
  f->name = "Motionblur";
  return (f);
}
static void
destroyinstance (struct filter *f)
{
  struct blurdata *i = (struct blurdata *) f->data;
  if (i->table != NULL)
    free (i->table);
  if (i->savedpalette != NULL)
    destroypalette (i->savedpalette);
  destroypalette (i->palette);
  destroyinheredimage (f);
  free (f->data);
  free (f);
}

/* An part of blur function that should be done paraely */
static void
blur82 (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *srci = f->childimage, *desti = f->image;
  struct blurdata *s = (struct blurdata *) f->data;
  unsigned char (*table)[256];
  int i, im;
  unsigned char *src, *dest, *srcend;

  im = srci->width;
  table = s->table;
  for (i = r1; i < r2; i++)
    {
      src = srci->currlines[i];
      srcend = src + im;
      dest = desti->currlines[i];
      for (; src < srcend; src++, dest++)
	{
	  dest[0] = table[src[0]][dest[0]];
	}
    }
}

#ifdef SUPPORT16
static void
blur16 (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *srci = f->childimage, *desti = f->image;
  struct blurdata *s = (struct blurdata *) f->data;
  struct truec *info = &srci->palette->info.truec;
  unsigned int rmask = info->rmask;
  unsigned int gmask = info->gmask;
  unsigned int bmask = info->bmask;
  unsigned int n = (unsigned int) s->n;
  int i;
  pixel16_t *src, *dest, *srcend;
  for (i = r1; i < r2; i++)
    {
      src = (pixel16_t *) srci->currlines[i];
      srcend = src + srci->width;
      dest = (pixel16_t *) desti->currlines[i];
      for (; src < srcend; src++, dest++)
	{
	  *dest = interpol (*src, *dest, n, rmask, gmask, bmask);
	}
    }
}
#endif
#ifdef STRUECOLOR24
static void
blur24 (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *srci = f->childimage, *desti = f->image;
  struct blurdata *s = (struct blurdata *) f->data;
  unsigned char (*table)[256] = s->table;
  unsigned char *src, *dest, *srcend;
  int i, im;
  im = srci->width * 3;
  if (!srci->palette->info.truec.byteexact)
    {
      x_fatalerror
	("Blur filter:unsupported color configuration! Please contact authors.");
    }
  for (i = r1; i < r2; i++)
    {
      src = srci->currlines[i];
      srcend = src + im;
      dest = desti->currlines[i];
      for (; src < srcend; src += 3, dest += 3)
	{
	  dest[0] = table[src[0]][dest[0]];
	  dest[1] = table[src[1]][dest[1]];
	  dest[2] = table[src[2]][dest[2]];
	}
    }
}
#endif
static void
blur32 (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *srci = f->childimage, *desti = f->image;
  struct blurdata *s = (struct blurdata *) f->data;
  unsigned char (*table)[256] = s->table;
  unsigned char *src, *dest, *srcend;
  int i, im;
  im = srci->width * 4;
  if (!srci->palette->info.truec.byteexact)
    {
      x_fatalerror
	("Blur filter:unsupported color configuration! Please contact authors.");
    }
  for (i = r1; i < r2; i++)
    {
      src = srci->currlines[i];
      srcend = src + im;
      dest = desti->currlines[i];
      switch (f->image->palette->info.truec.missingbyte)
	{
	case 3:
	  for (; src < srcend; src += 4, dest += 4)
	    {
	      dest[0] = table[src[0]][dest[0]];
	      dest[1] = table[src[1]][dest[1]];
	      dest[2] = table[src[2]][dest[2]];
	    }
	  break;
	case 2:
	  for (; src < srcend; src += 4, dest += 4)
	    {
	      dest[0] = table[src[0]][dest[0]];
	      dest[1] = table[src[1]][dest[1]];
	      dest[3] = table[src[2]][dest[2]];
	    }
	  break;
	case 1:
	  for (; src < srcend; src += 4, dest += 4)
	    {
	      dest[0] = table[src[0]][dest[0]];
	      dest[2] = table[src[1]][dest[1]];
	      dest[3] = table[src[2]][dest[2]];
	    }
	  break;
	case 0:
	  for (; src < srcend; src += 4, dest += 4)
	    {
	      dest[1] = table[src[1]][dest[1]];
	      dest[2] = table[src[2]][dest[2]];
	      dest[3] = table[src[3]][dest[3]];
	    }
	  break;
	default:
	  for (; src < srcend; src += 4, dest += 4)
	    {
	      dest[1] = table[src[1]][dest[1]];
	      dest[2] = table[src[2]][dest[2]];
	      dest[3] = table[src[3]][dest[3]];
	      dest[4] = table[src[4]][dest[4]];
	    }
	}
    }
}
static int
doit (struct filter *f, int flags, int time1)
{
  int val, n;
  int time = time1;
  struct blurdata *s = (struct blurdata *) f->data;
  updateinheredimage (f);
  val = f->previous->action->doit (f->previous, flags, time);
  s->counter += time;
  if (val & CHANGED)
    s->counter = 0;
  n = (int) ((1 - pow (1.0 - AMOUNT, (time + s->bckuptime) / DIV)) * 256);
  if (s->counter >= 2 * MAXFRAMES)
    {
      return val;
    }
  if (n < 10)
    {
      s->bckuptime += time;
      return val | ANIMATION;
    }
  s->bckuptime = 0;
  if (s->counter >= MAXFRAMES)
    n = 256, s->counter = 2 * MAXFRAMES;
  if (s->n != n)
    {
      s->n = n;
      switch (f->image->bytesperpixel)
	{
	case 1:
	  if (s->table == NULL)
	    s->table = (unsigned char (*)[256]) malloc (256 * 256);
	  blur8 (f);
	  break;
	case 3:
	case 4:
	  if (s->table == NULL)
	    s->table = (unsigned char (*)[256]) malloc (256 * 256);
	  blurtruecolor (f);
	  break;
	default:
	  if (s->table != NULL)
	    free (s->table), s->table = NULL;
	}
    }
  switch (f->image->palette->type)
    {
    case C256:
    case GRAYSCALE:
      xth_function (blur82, f, f->image->height);
      break;
#ifdef SUPPORT16
    case TRUECOLOR16:
      xth_function (blur16, f, f->image->height);
      break;
#endif
    case TRUECOLOR:
      xth_function (blur32, f, f->image->height);
      break;
#ifdef STRUECOLOR24
    case TRUECOLOR24:
      xth_function (blur24, f, f->image->height);
      break;
#endif
    }
  xth_sync ();
  if (s->counter == 2 * MAXFRAMES)
    {
      return val | CHANGED;
    }
  return val | CHANGED | ANIMATION;
}
static void
myremovefilter (struct filter *f)
{
  struct blurdata *s = (struct blurdata *) f->data;
  if (s->savedpalette != NULL)
    {
      restorepalette (f->image->palette, s->savedpalette);
      destroypalette (s->savedpalette);
      s->savedpalette = NULL;
    }
}

CONST struct filteraction blur_filter = {
  "Motionblur",
  "blur",
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
