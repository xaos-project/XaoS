/* An edge detection filter.
 * This is very simple filter - it initializes smalliter image and then
 * does an simple edge detection algo on it.
 */
#include <config.h>
#ifndef _plan9_
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include <stdio.h>		/*for NULL */
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#endif
#define SLARGEITER
#include <xthread.h>
#include <filter.h>

#define spixel_t pixel8_t
#include <c256.h>
#define do_edge do_edge8
#include "edged.c"

#undef spixel_t
#define spixel_t pixel16_t
#include <truecolor.h>
#define do_edge do_edge32
#include "edged.c"

#include <true24.h>
#define do_edge do_edge24
#include "edged.c"

#include <hicolor.h>
#define do_edge do_edge16
#include "edged.c"

static int
requirement (struct filter *f, struct requirements *r)
{
  f->req = *r;
  r->nimages = 1;
  r->flags &= ~IMAGEDATA;
  r->supportedmask = MASK1BPP | MASK3BPP | MASK2BPP | MASK4BPP;
  return (f->next->action->requirement (f->next, r));
}
static int
initialize (struct filter *f, struct initdata *i)
{
  inhermisc (f, i);
  /*in/out coloring modes looks better in iter modes. This also saves some
     memory in truecolor. */
  if (f->data != NULL)
    destroypalette ((struct palette *) f->data);
  f->data =
    createpalette (0, 65536,
		   i->image->bytesperpixel <= 1 ? SMALLITER : LARGEITER, 0,
		   65536, NULL, NULL, NULL, NULL, NULL);
  if (!inherimage
      (f, i, TOUCHIMAGE | NEWIMAGE, 0, 0, (struct palette *) f->data, 0, 0))
    return 0;
  return (f->previous->action->initialize (f->previous, i));
}
static struct filter *
getinstance (CONST struct filteraction *a)
{
  struct filter *f = createfilter (a);
  f->name = "Edge detection";
  return (f);
}
static void
destroyinstance (struct filter *f)
{
  if (f->data != NULL)
    destroypalette ((struct palette *) f->data);
  destroyinheredimage (f);
  free (f);
}
static int
doit (struct filter *f, int flags, int time)
{
  int val;
  int size = f->childimage->palette->type == SMALLITER ? 240 : 65520;
  if (f->image->palette->size < size)
    size = f->image->palette->size;
  if (((struct palette *) f->data)->size != size)
    ((struct palette *) f->data)->size =
      size, ((struct palette *) f->data)->version++;
  updateinheredimage (f);
  val = f->previous->action->doit (f->previous, flags, time);
  drivercall (*f->image,
	      xth_function (do_edge8, f, f->image->height),
	      xth_function (do_edge16, f, f->image->height),
	      xth_function (do_edge24, f, f->image->height),
	      xth_function (do_edge32, f, f->image->height));
  xth_sync ();
  return val;
}

CONST struct filteraction edge_filter = {
  "Edge detection",
  "edge",
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
