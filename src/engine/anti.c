#include <config.h>
#ifndef _plan9_
#ifndef __cplusplus
#include <math.h>
#endif
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include <string.h>
#include <config.h>
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
#include <xerror.h>
#include <filter.h>
#include <fractal.h>
#include <xthread.h>
struct antidata
{
  int shift;
};
static int
requirement (struct filter *f, struct requirements *r)
{
  f->req = *r;
  r->nimages = 1;
  r->supportedmask = TRUECOLOR24 | TRUECOLOR | TRUECOLOR16 | GRAYSCALE;
  return (f->next->action->requirement (f->next, r));
}

static int
initialize (struct filter *f, struct initdata *i)
{
  struct antidata *s = (struct antidata *) f->data;
  if (i->image->width * i->image->height * i->image->bytesperpixel * 2 * 16 >
      15 * 1024 * 1024)
    {
      s->shift = 1;
    }
  else
    {
      s->shift = 2;
    }
  inhermisc (f, i);
  if (!inherimage
      (f, i, TOUCHIMAGE | IMAGEDATA,
       (int) (((unsigned int) i->image->width) << s->shift),
       (int) (((unsigned int) i->image->height) << s->shift), NULL, 0, 0))
    return 0;
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
  struct antidata *i = (struct antidata *) calloc (1, sizeof (*i));
  f->childimage = NULL;
  f->data = i;
  f->name = "Antialiasing";
  return (f);
}
static void
destroyinstance (struct filter *f)
{
  destroyinheredimage (f);
  free (f->data);
  free (f);
}

static void
antigray (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *srci = f->childimage, *desti = f->image;
  struct antidata *s = (struct antidata *) f->data;
  register unsigned char *src;
  unsigned char *destend, *dest;
  unsigned int ystart, y;
  unsigned int xstart;
  register unsigned int sum;
  unsigned int xstep = (1U << (s->shift));
  int i;
  for (i = r1; i < r2; i++)
    {
      dest = (unsigned char *) desti->currlines[i];
      destend = dest + desti->width;
      ystart = ((unsigned int) i) << s->shift;
      xstart = 0;
      for (; dest < destend; dest++)
	{
	  if (xstep > 2)
	    {
	      sum = 0;
	      for (y = 0; y < 4; y++)
		{
		  src =
		    (unsigned char *) srci->currlines[y + ystart] + xstart;
		  sum += (unsigned int) src[0];
		  sum += (unsigned int) src[1];
		  sum += (unsigned int) src[2];
		  sum += (unsigned int) src[3];
		}
	      sum >>= 4;
	    }
	  else
	    {
	      src = (unsigned char *) srci->currlines[ystart] + xstart;
	      sum = (unsigned int) src[0];
	      sum += (unsigned int) src[1];
	      src = (unsigned char *) srci->currlines[ystart + 1] + xstart;
	      sum += (unsigned int) src[0];
	      sum += (unsigned int) src[1];
	      sum >>= 2;
	    }
	  *dest = (pixel8_t) sum;
	  xstart += xstep;
	}
    }
}

#ifdef STRUECOLOR24
static void
anti24 (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *srci = f->childimage, *desti = f->image;
  struct antidata *s = (struct antidata *) f->data;
  register unsigned char *src;
  unsigned char *destend, *dest;
  unsigned int ystart, y;
  unsigned int xstart;
  register unsigned int sum;
  unsigned int xstep = ((1U << s->shift) - 1) * 3;
  int c = 0;
  int i;
  if (!srci->palette->info.truec.byteexact)
    {
      x_fatalerror
	("Antialiasing filter:Unsupported colormask! Ask authors to add support for this :)");
    }
  for (i = r1; i < r2; i++)
    {
      dest = (unsigned char *) desti->currlines[i];
      destend = dest + desti->width * 3;
      ystart = ((unsigned int) i) << s->shift;
      xstart = 0;
      c = 1;
      for (; dest < destend; dest++)
	{
	  if (s->shift > 1)
	    {
	      sum = 0;
	      for (y = 0; y < 4; y++)
		{
		  src =
		    (unsigned char *) srci->currlines[y + ystart] + xstart;
		  sum += (unsigned int) src[0];
		  sum += (unsigned int) src[3];
		  sum += (unsigned int) src[6];
		  sum += (unsigned int) src[9];
		}
	      sum >>= 4;
	    }
	  else
	    {
	      src = (unsigned char *) srci->currlines[ystart] + xstart;
	      sum = (unsigned int) src[0];
	      sum += (unsigned int) src[3];
	      src = (unsigned char *) srci->currlines[ystart + 1] + xstart;
	      sum += (unsigned int) src[0];
	      sum += (unsigned int) src[3];
	      sum >>= 2;
	    }
	  *dest = (unsigned char) sum;
	  if (c == 3)
	    c = 0, xstart += xstep;
	  c++;
	  xstart++;
	}
    }
}
#endif
#ifdef SUPPORT16
#define MASKR1 ((unsigned int)((31+31744)+(31*65536*32)))
#define MASKR2 ((unsigned int)((31+31744)*(65536/32)+31))

#define MASKRH1 (31+31744)
#define MASKRH2 (31*32)
static void
anti16 (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *srci = f->childimage, *desti = f->image;
  struct antidata *s = (struct antidata *) f->data;
  register unsigned int *src;
  unsigned short *destend, *dest;
  int ystart, y;
  int xstart;
  register unsigned int sum1 = 0, sum2 = 0, sum;
  unsigned int xstep = 1U << (s->shift - 1);
  int i;
  unsigned int mask1 =
    (srci->palette->info.
     truec.mask2 | (srci->palette->info.truec.mask1 << 16)) >> 4;
  unsigned int mask2 =
    srci->palette->info.truec.mask1 | (srci->palette->info.truec.mask2 << 16);
  for (i = r1; i < r2; i++)
    {
      dest = (unsigned short *) desti->currlines[i];
      destend = dest + desti->width;
      ystart = ((unsigned int) i) << s->shift;
      xstart = 0;
      for (; dest < destend; dest++)
	{
	  if (xstep > 2)
	    {
	      sum1 = sum2 = 0;
	      for (y = 0; y < 4; y++)
		{
		  src = (unsigned int *) srci->currlines[y + ystart] + xstart;
		  sum1 += ((unsigned int) src[0] >> 4) & mask1;
		  sum2 += ((unsigned int) src[0] >> 4) & mask2;
		  sum1 += (unsigned int) src[1] & mask1;
		  sum2 += (unsigned int) src[1] & mask2;
		}
	      sum = ((sum1 >> 4) + (sum2 >> 16)) >> 4;
	      sum1 = (sum2 + (sum1 >> 12)) >> 4;
	    }
	  else
	    {
	      src = (unsigned int *) srci->currlines[ystart] + xstart;
	      sum1 = ((unsigned int) src[0] >> 4) & mask1;
	      sum2 = (unsigned int) src[0] & mask2;
	      src = (unsigned int *) srci->currlines[ystart + 1] + xstart;
	      sum1 += ((unsigned int) src[0] >> 4) & mask1;
	      sum2 += (unsigned int) src[0] & mask2;
	      sum = ((sum1 << 4) + (sum2 >> 16)) >> 2;
	      sum1 = (sum2 + (sum1 >> 12)) >> 2;
	    }
	  *dest =
	    (sum & srci->palette->info.truec.mask2) | (sum1 & srci->
						       palette->info.truec.
						       mask1);
	  xstart += xstep;
	}
    }
}
#endif


#define MASK1 0x00ff00ff
static void
anti32 (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *srci = f->childimage, *desti = f->image;
  struct antidata *s = (struct antidata *) f->data;
  register unsigned int *src;
  unsigned int *destend, *dest;
  unsigned int ystart, y;
  unsigned int xstart;
  register unsigned int sum1 = 0, sum2 = 0;
  unsigned int xstep = 1U << s->shift;
  int i;
  if (!srci->palette->info.truec.byteexact)
    {
      x_fatalerror
	("Antialiasing filter:Unsupported colormask2!  ask authors to add support for this :)");
    }
  for (i = r1; i < r2; i++)
    {
      dest = (unsigned int *) desti->currlines[i];
      destend = dest + desti->width;
      ystart = ((unsigned int) i) << s->shift;
      xstart = 0;
      for (; dest < destend; dest++)
	{
	  if (xstep > 2)
	    {
	      sum1 = sum2 = 0;
	      for (y = 0; y < 4; y++)
		{
		  src = (unsigned int *) srci->currlines[y + ystart] + xstart;
		  sum1 += (unsigned int) src[0] & MASK1;
		  sum2 += ((unsigned int) src[0] >> 8) & MASK1;
		  sum1 += (unsigned int) src[1] & MASK1;
		  sum2 += ((unsigned int) src[1] >> 8) & MASK1;
		  sum1 += (unsigned int) src[2] & MASK1;
		  sum2 += ((unsigned int) src[2] >> 8) & MASK1;
		  sum1 += (unsigned int) src[3] & MASK1;
		  sum2 += ((unsigned int) src[3] >> 8) & MASK1;
		}
	      sum1 >>= 4;
	      sum2 >>= 4;
	    }
	  else
	    {
	      src = (unsigned int *) srci->currlines[ystart] + xstart;
	      sum1 = (unsigned int) src[0] & MASK1;
	      sum2 = ((unsigned int) src[0] >> 8) & MASK1;
	      sum1 += (unsigned int) src[1] & MASK1;
	      sum2 += ((unsigned int) src[1] >> 8) & MASK1;
	      src = (unsigned int *) srci->currlines[ystart + 1] + xstart;
	      sum1 += (unsigned int) src[0] & MASK1;
	      sum2 += ((unsigned int) src[0] >> 8) & MASK1;
	      sum1 += (unsigned int) src[1] & MASK1;
	      sum2 += ((unsigned int) src[1] >> 8) & MASK1;
	      sum1 >>= 2;
	      sum2 >>= 2;
	    }
	  *dest = (sum1 & MASK1) | ((sum2 & MASK1) << 8);
	  xstart += xstep;
	}
    }
}
static int
doit (struct filter *f, int flags, int time1)
{
  int val;
  updateinheredimage (f);
  val = f->previous->action->doit (f->previous, flags, time1);
  switch (f->image->palette->type)
    {
    case GRAYSCALE:
      xth_function (antigray, f, f->image->height);
      break;
#ifdef STRUECOLOR24
    case TRUECOLOR24:
      xth_function (anti24, f, f->image->height);
      break;
#endif
#ifdef SUPPORT16
    case TRUECOLOR16:
      xth_function (anti16, f, f->image->height);
      break;
#endif
    case TRUECOLOR:
      xth_function (anti32, f, f->image->height);
      break;
    }
  xth_sync ();
  return val;
}
static void
convertup (struct filter *f, int *x, int *y)
{
  struct antidata *s = (struct antidata *) f->data;
  *x >>= s->shift;
  *y >>= s->shift;
  f->next->action->convertup (f->next, x, y);
}
static void
convertdown (struct filter *f, int *x, int *y)
{
  struct antidata *s = (struct antidata *) f->data;
  *x <<= s->shift;
  *y <<= s->shift;
  f->previous->action->convertdown (f->previous, x, y);
}


CONST struct filteraction antialias_filter = {
  "Antialiasing",
  "anti",
  0,
  getinstance,
  destroyinstance,
  doit,
  requirement,
  initialize,
  convertup,
  convertdown,
  NULL,
};
