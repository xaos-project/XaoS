#include <config.h>
#ifndef _plan9_
#ifdef NO_MALLOC_H
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <stdio.h>		/*for NULL */
#include <string.h>		/*for memcpy */
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#endif
#include <filter.h>
struct intdata
{
  unsigned char *lastent;
  int changed;
  int first;
};
static int
requirement (struct filter *f, struct requirements *r)
{
  r->nimages = 1;
  r->flags |= IMAGEDATA;
  return (f->next->action->requirement (f->next, r));
}
static int
initialize (struct filter *f, struct initdata *i)
{
  int x;
  struct intdata *d = (struct intdata *) f->data;
  pixel_t **lines1 =
    (pixel_t **) malloc (sizeof (*lines1) * i->image->height / 2), **lines2 =
    (pixel_t **) malloc (sizeof (*lines2) * i->image->height / 2);
  if (lines1 == NULL)
    return 0;
  inhermisc (f, i);
  d->first = 1;
  if (lines2 == NULL)
    {
      free (lines1);
      return 0;
    }
  if (f->childimage != NULL)
    destroy_image (f->childimage);
  f->image = i->image;
  f->image->flags |= PROTECTBUFFERS;
  i->flags |= DATALOST;
  for (x = 0; x < (i->image->height) / 2; x++)
    {
      lines1[x] = i->image->currlines[x * 2];
      lines2[x] = i->image->currlines[x * 2 + 1];
    }
  f->childimage = i->image =
    create_image_lines (i->image->width, (i->image->height) / 2, 2, lines1,
			lines2, i->image->palette, NULL,
			FREELINES | PROTECTBUFFERS, f->image->pixelwidth,
			f->image->pixelheight * 2);
  if (i->image == NULL)
    {
      free (lines1);
      free (lines2);
      return 0;
    }
  return (f->previous->action->initialize (f->previous, i));
}
static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct intdata *i = (struct intdata *) calloc (1, sizeof (*i));
  f->data = i;
  f->name = "Interlace filter";
  return (f);
}
static void
destroyinstance (struct filter *f)
{
  free (f->data);
  if (f->childimage != NULL)
    destroy_image (f->childimage);
  free (f);
}
static int
doit (struct filter *f, int flags, int time)
{
  struct intdata *i = (struct intdata *) f->data;
  int val;
  if (!(f->req.flags & IMAGEDATA)
      && f->childimage->currlines[0] == i->lastent)
    f->childimage->flip (f->childimage);
  i->lastent = f->childimage->currlines[0];
  val = f->previous->action->doit (f->previous, flags, time);
  if (i->first)
    {
      int y;
      for (y = 0; y < f->childimage->height; y++)
	memcpy (f->childimage->oldlines[y], f->childimage->currlines[y],
		f->childimage->width * f->childimage->bytesperpixel);
      i->first = 0;
    }
  if (val & CHANGED)
    i->changed = 1, val |= ANIMATION;
  else
    {
      if (i->changed)
	val |= CHANGED;
      i->changed = 0;
    }
  return (val);
}
static void
convertup (struct filter *f, int *x, int *y)
{
  *y *= 2;
  f->next->action->convertup (f->next, x, y);
}
static void
convertdown (struct filter *f, int *x, int *y)
{
  *y /= 2;
  f->previous->action->convertdown (f->previous, x, y);
}

CONST struct filteraction interlace_filter = {
  "Interlace filter",
  "interlace",
  0,
  getinstance,
  destroyinstance,
  doit,
  requirement,
  initialize,
  convertup,
  convertdown,
  NULL
};
