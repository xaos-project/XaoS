#include <config.h>
#ifndef _plan9_
#ifdef NO_MALLOC_H
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#else
#include <u.h>
#include <libc.h>
#endif
#include <stdio.h>
#include <filter.h>
/*#define DEBUG */
struct queue *
create_queue (struct filter *f)
{
  struct queue *q = (struct queue *) calloc (1, sizeof (struct queue));
  q->first = q->last = f;
  f->queue = q;
  f->next = f->previous = NULL;
  return (q);
}

void
insertfilter (struct filter *f1, struct filter *f2)
{
  f1->next = f2;
  f1->queue = f2->queue;
  f1->queue->isinitialized = 0;
  f1->previous = f2->previous;
  if (f2->previous != NULL)
    f2->previous->next = f1;
  else
    f2->queue->first = f1;
  f2->previous = f1;
}

void
addfilter (struct filter *f1, struct filter *f2)
{
  f1->previous = f2;
  f1->queue = f2->queue;
  f1->queue->isinitialized = 0;
  f1->next = f2->next;
  if (f2->next != NULL)
    f2->next->previous = f1;
  else
    f2->queue->last = f1;
  f2->next = f1;
}

void
removefilter (struct filter *f)
{
  if (f->action->removefilter != NULL)
    f->action->removefilter (f);
  if (f->previous != NULL)
    f->previous->next = f->next;
  else
    f->queue->first = f->next;
  if (f->next != NULL)
    f->next->previous = f->previous;
  else
    f->queue->last = f->previous;
  f->queue->isinitialized = 0;
}

int
initqueue (struct queue *q)
{
  struct requirements noreq = { 0, ALLMASK, 0 };
  struct initdata init = { NULL, 0 };
#ifdef DEBUG
  printf ("\n\nInitializing queue\n");
#endif
  q->palettechg = NULL;
  if (!q->first->action->requirement (q->first, &noreq))
    return 0;
  if (!q->last->action->initialize (q->last, &init))
    return 0;
  q->isinitialized = 1;
#ifdef DEBUG
  printf ("Correctly initialized\n");
#endif
  return 1;
}

int
reqimage (struct filter *f, struct requirements *req, int supportedmask,
	  int flags)
{
  f->req = *req;
  req->supportedmask &= supportedmask;
  if (!req->supportedmask)
    return 0;
  if (flags & TOUCHIMAGE && req->flags & IMAGEDATA)
    {
      req->flags = flags;
    }
  else
    req->flags &= flags;
  return 1;
}

/* An function helping to filter create new image.
 * It should be called by filter in inicialization. Filter passes
 * width,height,pixelwidth, pixelheight
 * and palette he wants to pass to his child and flags defining how it works
 * with image(IMAGEDATA if it requires data from previous frames (like blur
 * filter, TOUCHIMAGE if it changes data in image(like blur or stereogram
 * filter but unlike interlace and NEWIMAGE if it strictly requires to create
 * new image)
 * As palette he should pass NULL to keep parents palette. Same as
 * (pixel)width/height should be passed 0;
 *
 * Function then aplies some heruistic in order to minimize memory
 * requirements. So it should share image, create image that shares image data
 * or create new image)
 *
 * fills f->image, f->childimage and returns 1 if sucess and 0 if fail(usually
 * out of memory or it is unable to fit child's requirements)
 * and prepares data for child call.
 */
int
inherimage (struct filter *f, struct initdata *data, int flags, int width,
	    int height, struct palette *palette, float pixelwidth,
	    float pixelheight)
{
  int newimage = 0;
  int subimage = 1;
  int sharedimage = 1;
  struct image *i;

  int ddatalost = 0;
  if (width == 0)
    width = data->image->width;
  if (height == 0)
    height = data->image->height;
#ifdef DEBUG
  printf ("Inherimage:%s %i %i imagedata:%i %i\n", f->name, width, height,
	  flags & IMAGEDATA, flags & PROTECTBUFFERS);
#endif
  if (pixelwidth == 0)
    pixelwidth = data->image->pixelwidth;
  if (pixelheight == 0)
    pixelheight = data->image->pixelheight;
  if (palette == NULL)
    palette = data->image->palette;
  if (!(palette->type & f->req.supportedmask))
    {
#ifdef DEBUG
      printf
	("Initalization of filter %s failed due to unsupported type by child %s-%i,%i\n",
	 f->name, f->previous->name, f->req.supportedmask, palette->type);
#endif
      f->image = data->image;
      return 0;
    }

  if (flags & NEWIMAGE)
    newimage = 1, sharedimage = 0, subimage = 0;
  if ((flags & IMAGEDATA) /*|| (data->image->flags & PROTECTBUFFERS) */ )
    subimage = 0, sharedimage = 0, newimage = 1;
  /*if filter touches data but child requires them, create separated image */
  if ((flags & TOUCHIMAGE)
      && ((f->req.flags & IMAGEDATA)
	  || (data->image->flags & PROTECTBUFFERS)))
    subimage = 0, newimage = 1, sharedimage = 0;
  /*if required image differs in size or so */
  if (width != data->image->width || height != data->image->height ||
      palette != data->image->palette)
    newimage = 1, sharedimage = 0;

  if (f->childimage != NULL && (f->flags & ALLOCEDIMAGE))
    {
      /*is an old child image still useable for us purposes? if not burn it it! */
      /*We should share image? Why alloc new?? */
      if (!newimage && (f->flags & ALLOCEDIMAGE))
	destroyinheredimage (f), ddatalost = 1;
      /*We should share data? but child image dont do that! */
      if (subimage && !(f->flags & SHAREDDATA))
	destroyinheredimage (f), ddatalost = 1;
      /*We can't share data but child image does that? */
      if (!subimage && (f->flags & SHAREDDATA))
	destroyinheredimage (f), ddatalost = 1;
      /*When image changed, child image must be recreated too */
      if (f->flags & SHAREDDATA
	  && ((data->flags & DATALOST)
	      || f->imageversion != data->image->version))
	destroyinheredimage (f), ddatalost = 1;
      /*We should share image with filter? Why keep created new one? */
      if (sharedimage)
	destroyinheredimage (f), ddatalost = 1;
      /*When child image don't fit out needs */
      if (f->childimage != NULL
	  && (f->childimage->width != width || f->childimage->height != height
	      || f->childimage->palette != palette
	      || f->childimage->bytesperpixel != bytesperpixel (palette->type)
	      || f->childimage->nimages < f->req.nimages))
	destroyinheredimage (f), ddatalost = 1;
      /*Well now child image seems to be heavily probed */
    }
  i = f->childimage;
  if (newimage)
    {				/*Create new image when required */
      if (!(f->flags & ALLOCEDIMAGE))
	{
	  if (subimage)
	    {
	      i =
		create_subimage (data->image, width, height, f->req.nimages,
				 palette, pixelwidth, pixelheight);
	      f->flags |= ALLOCEDIMAGE | SHAREDDATA;
	      ddatalost = 1;
	    }
	  else
	    {
	      i =
		create_image_mem (width, height, f->req.nimages, palette,
				  pixelwidth, pixelheight);
	      f->flags |= ALLOCEDIMAGE;
	      ddatalost = 1;
	    }
	}
    }
#ifdef DEBUG
  printf ("Filter:%s newimage:%i subimage:%i sharedimage:%i\n", f->name,
	  newimage, subimage, sharedimage);
#endif
  if (i == NULL)
    {
      f->image = data->image;
      return 0;
    }
  if (sharedimage)
    i = data->image, ddatalost = (data->flags & DATALOST)
      || (f->childimage != data->image);
  if (sharedimage && datalost (f, data))
    ddatalost = 1;
  else if ((f->flags | SHAREDDATA) && datalost (f, data)
	   && !(i->flags & FREEDATA))
    ddatalost = 1;
  if (ddatalost)
    data->flags |= DATALOST;
  else
    data->flags &= ~DATALOST;
  f->image = data->image;
  f->childimage = i;
  f->imageversion = data->image->version;
  data->image = i;
#ifdef DEBUG
  printf ("OK %i datalost:%i\n", f->flags, ddatalost);
#endif
#ifdef DEBUG
  printf ("Inherimage2:%s %i %i\n", f->name, width, height);
#endif
  return 1;
}

void
destroyinheredimage (struct filter *f)
{
  if (f->flags & ALLOCEDIMAGE)
    destroy_image (f->childimage), f->flags &=
      ~(ALLOCEDIMAGE | SHAREDDATA), f->childimage = NULL;
}

void
updateinheredimage (struct filter *f)
{
  if ((f->flags & SHAREDDATA) && f->childimage)
    {
      if (f->childimage->nimages == 2
	  && f->image->currimage != f->childimage->currimage)
	f->childimage->flip (f->childimage);	/*Hack for interlace filter */
    }
}
void
inhermisc (struct filter *f, CONST struct initdata *data)
{
  f->wait_function = data->wait_function;
  f->fractalc = data->fractalc;
}
struct filter *
createfilter (CONST struct filteraction *fa)
{
  struct filter *f = (struct filter *) calloc (1, sizeof (struct filter));
  if (f == NULL)
    return NULL;
  f->queue = NULL;
  f->next = NULL;
  f->childimage = NULL;
  f->flags = 0;
  f->previous = NULL;
  f->action = fa;
  f->image = NULL;
  f->req.nimages = 1;
  f->data = NULL;
  return (f);
}

void
convertupgeneric (struct filter *f, int *x, int *y)
{
  if (f->next != NULL)
    f->next->action->convertup (f->next, x, y);
}

void
convertdowngeneric (struct filter *f, int *x, int *y)
{
  if (f->previous != NULL)
    f->previous->action->convertdown (f->previous, x, y);
}
