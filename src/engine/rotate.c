/* An rotation filter. Uses bressemham algorithm combined with dda to rotate
 * image around center
 * This filter is used internally by XaoS and is unvisible to normal user in
 * 'E' menu.
 * It is used to implement fast rotation mode
 */
#include <config.h>
#ifndef _plan9_
#include <string.h>
#include <limits.h>
#include <archaccel.h>
#ifndef __cplusplus
#include <math.h>
#endif
#ifdef NO_MALLOC_H
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#endif
#define SLARGEITER
#include <xthread.h>
#include <filter.h>

struct rotatedata
{
  number_t angle;
  number_t x1, y1, x2, y2, xx1, yy1, xx2, yy2;
};

#include <c256.h>
#define do_rotate do_rotate8
#include "rotated.c"

#include <truecolor.h>
#define do_rotate do_rotate32
#include "rotated.c"

#include <true24.h>
#define do_rotate do_rotate24
#include "rotated.c"

#include <hicolor.h>
#define do_rotate do_rotate16
#include "rotated.c"


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
  float size, pixelsize;
  struct rotatedata *s = (struct rotatedata *) f->data;
  inhermisc (f, i);
  s->angle = INT_MAX;
  /*in/out coloring modes looks better in iter modes. This also saves some
     memory in truecolor. */
  if (i->image->pixelwidth < i->image->pixelheight)
    pixelsize = i->image->pixelwidth;
  else
    pixelsize = i->image->pixelheight;
  size =
    sqrt (i->image->width * i->image->width * i->image->pixelwidth *
	  i->image->pixelwidth +
	  i->image->height * i->image->height * i->image->pixelheight *
	  i->image->pixelheight);
  if (!inherimage
      (f, i, TOUCHIMAGE | NEWIMAGE, (int) (size / pixelsize + 1),
       (int) (size / pixelsize + 1), NULL, pixelsize, pixelsize))
    return 0;
  return (f->previous->action->initialize (f->previous, i));
}
static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  struct rotatedata *i = (struct rotatedata *) calloc (1, sizeof (*i));
  f->name = "Rotation filter";
  f->data = i;
  return (f);
}
static void
destroyinstance (struct filter *f)
{
  free (f->data);
  destroyinheredimage (f);
  free (f);
}
static int
doit (struct filter *f, int flags, int time)
{
  int val;
  struct rotatedata *s = (struct rotatedata *) f->data;
  number_t angle = f->fractalc->angle;
  number_t wx = f->fractalc->windowwidth, wy = f->fractalc->windowheight;
  number_t rr = f->fractalc->s.rr, ir = f->fractalc->s.ri;
  f->fractalc->windowwidth = f->fractalc->windowheight =
    f->childimage->width * f->childimage->pixelwidth;
  f->fractalc->s.rr *= f->fractalc->windowwidth / wx;
  f->fractalc->s.ri *= f->fractalc->windowheight / wy;
  f->fractalc->windowwidth = f->fractalc->windowheight = 1;
  f->fractalc->angle = 0;
  update_view (f->fractalc);	/*update rotation tables */
  updateinheredimage (f);
  val = f->previous->action->doit (f->previous, flags, time);
  f->fractalc->angle = angle;
  update_view (f->fractalc);	/*update rotation tables */
  f->fractalc->s.rr = rr;
  f->fractalc->s.ri = ir;
  f->fractalc->windowwidth = wx;
  f->fractalc->windowheight = wy;
  if ((val & CHANGED) || s->angle != angle)
    {
      s->xx2 = f->image->width * f->image->pixelwidth / 2;
      s->yy2 = f->image->height * f->image->pixelheight / 2;
      s->x1 = -s->xx2;
      s->y1 = -s->yy2;
      s->x2 = -s->xx2;
      s->y2 = s->yy2;
      s->xx1 = s->xx2;
      s->yy1 = -s->yy2;
      rotateback (*f->fractalc, s->x1, s->y1);
      rotateback (*f->fractalc, s->x2, s->y2);
      rotateback (*f->fractalc, s->xx1, s->yy1);
      rotateback (*f->fractalc, s->xx2, s->yy2);
      s->x1 /= f->childimage->pixelwidth;
      s->x1 += f->childimage->width / 2;
      s->y1 /= f->childimage->pixelwidth;
      s->y1 += f->childimage->width / 2;
      s->xx1 /= f->childimage->pixelwidth;
      s->xx1 += f->childimage->width / 2;
      s->yy1 /= f->childimage->pixelwidth;
      s->yy1 += f->childimage->width / 2;
      s->x2 /= f->childimage->pixelwidth;
      s->x2 += f->childimage->width / 2;
      s->y2 /= f->childimage->pixelwidth;
      s->y2 += f->childimage->width / 2;
      s->xx2 /= f->childimage->pixelwidth;
      s->xx2 += f->childimage->width / 2;
      s->yy2 /= f->childimage->pixelwidth;
      s->yy2 += f->childimage->width / 2;

      drivercall (*f->image,
		  xth_function (do_rotate8, f, f->image->height),
		  xth_function (do_rotate16, f, f->image->height),
		  xth_function (do_rotate24, f, f->image->height),
		  xth_function (do_rotate32, f, f->image->height));
      xth_sync ();
      val |= CHANGED;
    }
  return val;
}

static void
convertup (struct filter *f, int *x, int *y)
{
  number_t xd = (*x - f->childimage->width / 2) * f->childimage->pixelwidth;
  number_t yd = (*y - f->childimage->height / 2) * f->childimage->pixelheight;
  *x = (int) (f->image->width / 2 + xd / f->image->pixelwidth);
  *y = (int) (f->image->height / 2 + yd / f->image->pixelheight);
  if (f->next != NULL)
    f->next->action->convertup (f->next, x, y);
}
static void
convertdown (struct filter *f, int *x, int *y)
{
  number_t xd = (*x - f->image->width / 2) * f->image->pixelwidth;
  number_t yd = (*y - f->image->height / 2) * f->image->pixelheight;
  *x = (int) (f->childimage->width / 2 + xd / f->childimage->pixelwidth);
  *y = (int) (f->childimage->height / 2 + yd / f->childimage->pixelheight);
  if (f->previous != NULL)
    f->previous->action->convertdown (f->previous, x, y);
}



CONST struct filteraction rotate_filter = {
  "Image rotation",
  "rotate",
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
