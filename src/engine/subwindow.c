#ifndef _plan9_
#include <config.h>
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
#include <zoom.h>
struct subdata
{
  struct filter *second;
  struct image *image;
  pixel_t **currlines;
  int recal;
  int forpversion, forversion;
  number_t pre, pim;
};
void
subwindow_setsecond (struct filter *f, struct filter *f1)
{
  struct subdata *s = (struct subdata *) f->data;
  s->second = f1;
}
static void
myflip (struct image *image)
{
  struct subdata *s = (struct subdata *) image->data;
  flipgeneric (image);
  s->image->flip (s->image);
  s->currlines = s->image->currlines;
}
static int
requirement (struct filter *f, struct requirements *r)
{
  r->nimages = 2;
  r->flags |= IMAGEDATA;
  return (f->next->action->requirement (f->next, r));
}
extern CONST struct filteraction threed_filter;

static int
initialize (struct filter *f, struct initdata *i)
{
  struct subdata *s = (struct subdata *) f->data;
  int x;
  int val = 0;
  pixel_t **lines1, **lines2 = NULL;
  double size;
  int width, height;
  int threed = 0;
  struct filter *f1 = f;

  inhermisc (f, i);
  if (datalost (f, i))
    s->recal = 1;
  while (f1)
    {
      if (f1->action == &threed_filter)
	threed = 1;
      f1 = f1->next;
    }
  f->imageversion = i->image->version;
  if (f->childimage != NULL)
    destroy_image (f->childimage);
  s->image = f->image = i->image;
  s->image->flags |= PROTECTBUFFERS;
  s->currlines = f->image->currlines;
  s->forpversion = f->image->palette->version;
  s->forversion = f->fractalc->version;
  if (f->image->width * f->image->pixelwidth <
      f->image->height * f->image->pixelheight)
    size = f->image->width * f->image->pixelwidth / 2;
  else
    size = f->image->height * f->image->pixelheight / 2;
  width = (int) (size / f->image->pixelwidth);
  height = (int) (size / f->image->pixelheight);
  /*fractalc_resize_to(f->fractalc,size,size); */
  lines1 = (pixel_t **) malloc (sizeof (*lines1) * height);
  if (f->image->nimages == 2)
    lines2 = (pixel_t **) malloc (sizeof (*lines2) * height);
  if (lines1 == NULL)
    return 0;
  if (f->image->nimages == 2 && lines2 == NULL)
    {
      free (lines1);
      return 0;
    }
  for (x = 0; x < height; x++)
    {
      lines1[x] =
	i->image->currlines[x + (threed ? f->image->height / 3 : 0)];
      if (f->image->nimages == 2)
	lines2[x] =
	  i->image->oldlines[x + (threed ? f->image->height / 3 : 0)];
    }
  if (f->image->nimages == 2)
    for (x = 0; x < f->image->height; x++)
      {
	memcpy (f->image->oldlines[x], f->image->currlines[x],
		f->image->width * f->image->bytesperpixel);
      }
  f->childimage = i->image =
    create_image_lines (width, height, f->image->nimages, lines1, lines2,
			i->image->palette, myflip, FREELINES,
			f->image->pixelwidth, f->image->pixelheight);
  if (i->image == NULL)
    {
      free (lines1);
      free (lines2);
      return 0;
    }
  f->childimage->data = s;
  x = f->previous->action->initialize (f->previous, i);
  if (!x)
    return 0;
  if (s->second != NULL)
    {
      i->image = f->image;
      val = s->second->action->initialize (s->second, i);
      if (!val)
	return 0;
    }
  return (x | val);

}
static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct subdata *s = (struct subdata *) calloc (1, sizeof (*s));
  f->name = "Subwindow";
  f->data = s;
  s->second = NULL;
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
  int val = 0, m, vold;
  vinfo vs;
  vrect rs;
  float wwidth, wheight;
  struct subdata *s = (struct subdata *) f->data;
  static int v;
  if (s->second != NULL
      && (s->recal || s->forpversion != f->image->palette->version
	  || s->forversion != f->fractalc->version))
    {
      int x;
      if (s->recal)
	f->fractalc->version++;
      s->forpversion = f->image->palette->version;
      s->forversion = f->fractalc->version;
      s->recal = 1;
      val = (s->second->action->doit (s->second, flags, time));
      if (val & ANIMATION)
	return val;
      s->recal = 0;
      if (f->image->nimages == 2)
	for (x = 0; x < f->image->height; x++)
	  {
	    memcpy (f->image->oldlines[x], f->image->currlines[x],
		    f->image->width * f->image->bytesperpixel);
	  }
    }
  if (s->currlines != f->image->currlines && f->childimage->nimages == 2)
    flipgeneric (f->childimage), s->currlines = f->image->currlines;
  /*FIXME: ugly hack for new julia mode */
  v++;
  wwidth = f->fractalc->windowwidth;
  wheight = f->fractalc->windowheight;
  f->fractalc->windowwidth =
    f->previous->image->width * f->previous->image->pixelwidth;
  f->fractalc->windowheight =
    f->previous->image->height * f->previous->image->pixelheight;
  vs = f->fractalc->s;
  rs = f->fractalc->rs;
  f->fractalc->s = f->fractalc->currentformula->v;
  if (f->fractalc->currentformula->calculate_julia)
    {
      f->fractalc->s.cr = f->fractalc->s.ci = 0;
      f->fractalc->s.rr = f->fractalc->s.ri = 4;	/*FIXME should be set to real formula's bailout */
    }
  update_view (f->fractalc);
  m = f->fractalc->mandelbrot;
  vold = f->fractalc->version;
  if (s->pre != f->fractalc->pre || s->pim != f->fractalc->pim)
    {
      f->fractalc->version = v;
      s->pre = f->fractalc->pre;
      s->pim = f->fractalc->pim;
    }
  f->fractalc->mandelbrot = 0;
  val = f->previous->action->doit (f->previous, flags, time) | val;
  f->fractalc->mandelbrot = m;
  f->fractalc->version = vold;
  f->fractalc->s = vs;
  f->fractalc->rs = rs;
  f->fractalc->windowwidth = wwidth;
  f->fractalc->windowheight = wheight;
  return val;
}
static void
myremove (struct filter *f)
{
  /*fractalc_resize_to(f->fractalc,f->queue->last->image->width*f->queue->last->image->pixelwidth,f->queue->last->image->height*f->queue->last->image->pixelheight); */
}
static void
convertdown (struct filter *f, int *x, int *y)
{
  struct subdata *s = (struct subdata *) f->data;
  if (s->second != NULL)
    s->second->action->convertdown (s->second, x, y);
  if (f->previous != NULL)
    f->previous->action->convertdown (f->previous, x, y);
}


CONST struct filteraction subwindow_filter = {
  "Subwindow",
  "Subwindow",
  0,
  getinstance,
  destroyinstance,
  doit,
  requirement,
  initialize,
  convertupgeneric,
  convertdown,
  myremove
};
