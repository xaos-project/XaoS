#include <config.h>
#ifndef _plan9_
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include <config.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <stdlib.h>
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#endif
#define SLARGEITER
#include <filter.h>
#include <xthread.h>
#define NCOLORS 256
#define IMAGETYPE SMALLITER
#define spixel_t pixel8_t
struct siterdata
{
  struct palette *palette;
};
static int
requirement (struct filter *f, struct requirements *r)
{
  f->req = *r;
  r->nimages = 1;
  r->flags &= ~IMAGEDATA;
  r->supportedmask = MASK1BPP | MASK2BPP | MASK3BPP | MASK4BPP;
  return (f->next->action->requirement (f->next, r));
}
static int
initialize (struct filter *f, struct initdata *i)
{
  struct siterdata *s = (struct siterdata *) f->data;
  inhermisc (f, i);
  if (!inherimage (f, i, TOUCHIMAGE, 0, 0, s->palette, 0, 0))
    return 0;
  return (f->previous->action->initialize (f->previous, i));
}
static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct siterdata *i = (struct siterdata *) calloc (1, sizeof (*i));
  i->palette =
    createpalette (0, 65536, IMAGETYPE, 0, 65536, NULL, NULL, NULL, NULL,
		   NULL);
  f->data = i;
  f->name = "Smalliter image convertor";
  return (f);
}
static void
convert8 (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *img1 = f->childimage, *img2 = f->image;
  unsigned char *src, *srcend;
  unsigned int *pixels = img2->palette->pixels;
  pixel8_t *dest;
  int i;
  for (i = r1; i < r2; i++)
    {
      src = img1->currlines[i];
      dest = img2->currlines[i];
      srcend = src + img1->width;
      for (; src < srcend; src++, dest++)
	*dest = pixels[*src];
    }
}
static void
convert16 (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *img1 = f->childimage, *img2 = f->image;
  unsigned char *src, *srcend;
  unsigned int *pixels = img2->palette->pixels;
  pixel16_t *dest;
  int i;
  for (i = r1; i < r2; i++)
    {
      src = img1->currlines[i];
      dest = (pixel16_t *) img2->currlines[i];
      srcend = src + img1->width;
      for (; src < srcend; src++, dest++)
	{
	  *dest = pixels[*src];
	}
    }
}

#ifdef STRUECOLOR24
#include <true24.h>
static void
convert24 (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *img1 = f->childimage, *img2 = f->image;
  unsigned char *src, *srcend;
  unsigned int *pixels = img2->palette->pixels;
  cpixel_t *dest;
  int i;
  for (i = r1; i < r2; i++)
    {
      src = img1->currlines[i];
      dest = (cpixel_t *) img2->currlines[i];
      srcend = src + img1->width;
      for (; src < srcend; src++, dest += 3)
	p_set (dest, pixels[*src]);
    }
}
#endif
static void
convert32 (void *data, struct taskinfo *task, int r1, int r2)
{
  struct filter *f = (struct filter *) data;
  struct image *img1 = f->childimage, *img2 = f->image;
  unsigned char *src, *srcend;
  unsigned int *pixels = img2->palette->pixels;
  pixel32_t *dest;
  int i;
  for (i = r1; i < r2; i++)
    {
      src = img1->currlines[i];
      dest = (pixel32_t *) img2->currlines[i];
      srcend = src + img1->width;
      for (; src < srcend; src++, dest++)
	*dest = pixels[*src];
    }
}
static void
destroyinstance (struct filter *f)
{
  struct siterdata *i = (struct siterdata *) f->data;
  destroypalette (i->palette);
  free (f->data);
  destroyinheredimage (f);
  free (f);
}
static int
doit (struct filter *f, int flags, int time)
{
  int val;
  int size;
  updateinheredimage (f);
  if (f->image->palette->size < 256)
    size = f->image->palette->size;
  else
    size = 256;
  if (size != f->childimage->palette->size)
    f->childimage->palette->size = size, f->childimage->palette->version++;
  val = f->previous->action->doit (f->previous, flags, time);
  if (f->image->palette->type != SMALLITER
      || f->image->currlines[0] != f->childimage->currlines[0])
    {
    drivercall (*f->image,
		  xth_function (convert8, f, f->image->height),
		  xth_function (convert16, f, f->image->height),
		  xth_function (convert24, f, f->image->height),
		  xth_function (convert32, f, f->image->height))}
  xth_sync ();
  return val;
}

CONST struct filteraction smalliter_filter = {
  "Smalliter image convertor",
  "smalliter",
  0,
  getinstance,
  destroyinstance,
  doit,
  requirement,
  initialize,
  convertupgeneric,
  convertdowngeneric,
  NULL
};
