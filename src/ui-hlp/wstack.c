#ifdef _plan9_
#include <u.h>
#include <libc.h>
#else
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#endif
#include <fconfig.h>
#include <filter.h>
#include <ui_helper.h>
#include <archaccel.h>
#include "grlib.h"

/* This is quite simple and ugly implementation of windows.
 * it redraws all windows every frame and removes them before calculation
 * it should be OK for small windows (like texts) used by ui_helper
 * I plan to abuse it in ugly interface too, in case it will be fast enought
 */

static void
uih_darkrectangle (struct image *image, int x, int y, int width, int height)
{

  int mask = 0;
  unsigned int *current, *end;
  if (x + width < 0 || y + height < 0 || y >= image->height
      || x >= image->width)
    return;
  if (x + width >= image->width)
    width = image->width - x;
  if (x < 0)
    width += x, x = 0;
  if (width <= 0)
    return;
  if (y + height >= image->height)
    height = image->height - y;
  if (y < 0)
    height += y, y = 0;
  if (height <= 0)
    return;
  assert (x >= 0 && y >= 00 && width > 0 && height > 0
	  && x + width <= image->width && y + height <= image->height);
  if (image->bytesperpixel == 2)
    {
      int x1 = x / 2;
      width = (x + width + 1) / 2 - x1;
      x = x1;
    }
  if (image->bytesperpixel == 3)
    {
      int x1 = x * 3 / 4;
      width = (x + width + 2) * 3 / 4 - x1;
      x = x1;
    }
  switch (image->palette->type)
    {
    case TRUECOLOR:
    case TRUECOLOR24:
      mask =
	~((1 <<
	   (image->palette->info.truec.rshift + 7 -
	    image->palette->info.truec.rprec)) | (1 << (image->palette->
							info.truec.gshift +
							7 -
							image->palette->
							info.truec.gprec)) |
	  (1 <<
	   (image->palette->info.truec.bshift + 7 -
	    image->palette->info.truec.bprec)));
      break;
    case TRUECOLOR16:
      mask =
	((1 <<
	  (image->palette->info.truec.rshift + 7 -
	   image->palette->info.truec.rprec)) | (1 << (image->palette->
						       info.truec.gshift + 7 -
						       image->palette->
						       info.truec.gprec)) | (1
									     <<
									     (image->
									      palette->
									      info.
									      truec.
									      bshift
									      +
									      7
									      -
									      image->
									      palette->
									      info.
									      truec.
									      bprec)));
      mask = ~(mask | (mask << 16));
      break;
    }
  height += y;
  while (y < height)
    {
      current = ((unsigned int *) image->currlines[y]) + x;
      end = current + width;
      while (current < end)
	*current = (*current >> 1) & mask, current++;
      y++;
    }

}
void
uih_drawborder (struct uih_context *uih, int x, int y, int width, int height,
		int flags)
{
  int leftcolor;
  int rightcolor;
  int bgcolor;
  if (uih->palette->type & BITMAPS)
    {
      if (flags & BORDER_PRESSED)
	{
	  bgcolor = FGCOLOR (uih);
	  rightcolor = BGCOLOR (uih);
	  leftcolor = BGCOLOR (uih);
	}
      else
	{
	  bgcolor = BGCOLOR (uih);
	  rightcolor = FGCOLOR (uih);
	  leftcolor = FGCOLOR (uih);
	}
    }
  else
    {
      if (flags & BORDER_LIGHT)
	{
	  bgcolor = LIGHTGRAYCOLOR (uih);
	  rightcolor = BGCOLOR (uih);
	  leftcolor = LIGHTGRAYCOLOR2 (uih);
	}
      else
	{
	  bgcolor = DARKGRAYCOLOR (uih);
	  rightcolor = BGCOLOR (uih);
	  leftcolor = LIGHTGRAYCOLOR (uih);
	}
      if (flags & BORDER_PRESSED)
	{
	  int i = leftcolor;
	  leftcolor = rightcolor;
	  rightcolor = i;
	}
      if (uih->image->flags & AAIMAGE)
	bgcolor = BGCOLOR (uih);
    }
  if (uih->image->bytesperpixel > 1 && (flags & BORDER_TRANSPARENT))
    uih_darkrectangle (uih->image, x + 1, y + 1, width - 2,
		       height - 2) /*, leftcolor = GRAYCOLOR (uih) */ ;
  else
    {
      xrectangle (uih->image, x + 1, y + 1, width - 2, height - 2, bgcolor);
    }
  xhline (uih->image, x, y, width - 1, leftcolor);
  xhline (uih->image, x, y + height - 1, width - 1, rightcolor);
  xvline (uih->image, x, y, height - 1, leftcolor);
  xvline (uih->image, x + width - 1, y, height - 1, rightcolor);
}

struct uih_window *
uih_registerw (struct uih_context *uih, uih_getposfunc
	       getpos, uih_drawfunc draw, void *data, int flags)
{
  struct uih_window *w =
    (struct uih_window *) calloc (1, sizeof (struct uih_window));
  struct uih_window *w1;
  assert (uih != NULL && getpos != NULL && draw != NULL && flags >= 0);
  if (w == NULL)
    return NULL;
  uih_clearwindows (uih);
  w1 = uih->wtop;
  w->getpos = getpos;
  w->draw = draw;
  w->data = data;
  w->flags = flags;
  w->savedline = -1;
  w->saveddata = NULL;
  w->flags |= BORDER_TRANSPARENT | BORDER_PRESSED | BORDER_LIGHT;
  w->next = NULL;
  if (w1 == NULL)
    {
      uih->wtop = w;
    }
  else
    {
      while (w1->next != NULL)
	w1 = w1->next;
      w1->next = w;
    }
  w->previous = w1;
  w->x = -65536;
  return w;
}

void
uih_setline (struct uih_context *uih, struct uih_window *w, int color, int x1,
	     int y1, int x2, int y2)
{
  if (w->savedline != color || w->x != x1 || w->y != y1 || w->width != x2 - x1
      || w->height != y2 - y1)
    {
      uih_clearwindows (uih);
      uih->display = 1;
      w->savedline = color;
      w->x = x1;
      w->y = y1;
      w->width = x2 - x1;
      w->height = y2 - y1;
    }
}
struct uih_window *
uih_registerline (struct uih_context *uih, int color, int x1, int y1, int x2,
		  int y2)
{
  struct uih_window *w =
    (struct uih_window *) calloc (1, sizeof (struct uih_window));
  struct uih_window *w1;
  if (w == NULL)
    return NULL;
  uih_clearwindows (uih);
  w1 = uih->wtop;
  uih->display = 1;
  w->getpos = NULL;
  w->savedline = color;
  w->flags = 0;
  w->x = x1;
  w->y = y1;
  w->width = x2 - x1;
  w->height = y2 - y1;
  w->saveddata = NULL;
  w->next = NULL;
  if (w1 == NULL)
    {
      uih->wtop = w;
    }
  else
    {
      while (w1->next != NULL)
	w1 = w1->next;
      w1->next = w;
    }
  w->previous = w1;
  return w;
}

void
uih_removew (struct uih_context *uih, struct uih_window *w)
{
  uih_clearwindows (uih);
  assert (uih->wtop != NULL);
  assert (w != NULL);
  uih->display = 1;

  if (w->previous == NULL)
    {
      assert (uih->wtop == w);
      uih->wtop = w->next;
    }
  else
    {
      w->previous->next = w->next;
    }
  if (w->next != NULL)
    {
      w->next->previous = w->previous;
    }
  free (w);
}

/*Remove all drawed windows from screen */
void
uih_clearwindows (struct uih_context *uih)
{
  struct uih_window *w = uih->wtop;
  int savedline = 0;
  int savedpos = 0;
  int destwidth = uih->image->width * uih->image->bytesperpixel;
  if (!uih->wdisplayed)
    return;
  if (!uih->image->bytesperpixel)
    {
      destwidth = (w->x + uih->image->width + 7) / 8;
    }
  uih->wdisplayed = 0;
  if (uih->wflipped)
    uih->image->flip (uih->image), uih->wflipped = 0;
  while (w)
    {
      if (w->getpos == NULL)
	{
	  if (w->saveddata != NULL)
	    {
	      xrestoreline (uih->image, w->saveddata, w->x, w->y,
			    w->width + w->x, w->height + w->y);
	      free (w->saveddata);
	      w->saveddata = NULL;
	    }
	}
      else
	{
	  if (w->savedline != -1 || w->saveddata != NULL)
	    {
	      int i;
	      int xskip = w->x * uih->image->bytesperpixel;
	      int width = w->width * uih->image->bytesperpixel;
	      if (!uih->image->bytesperpixel)
		{
		  xskip = w->x / 8;
		  width = (w->x + w->width + 7) / 8 - xskip;
		}
	      assert (w->width);
	      assert (w->height);
	      assert (w->x >= 0);
	      assert (w->y >= 0);
	      assert (w->x + w->width <= uih->image->width);
	      assert (w->y + w->height <= uih->image->height);
	      if (w->savedline != -1)
		{
		  savedline = w->savedline;
		  savedpos = w->savedpos;
		  for (i = w->y; i < w->y + w->height; i++)
		    {
		      unsigned char *data = uih->image->currlines[i] + xskip;
		      assert (savedline < uih->image->height);
		      assert (savedline >= 0);
		      assert (savedpos >= 0 && savedpos <= destwidth);
		      if (width + savedpos > destwidth)
			{
			  int width1;
			  memcpy (data,
				  uih->image->oldlines[savedline] + savedpos,
				  destwidth - savedpos);
			  savedline++;
			  width1 = width - destwidth + savedpos;
			  memcpy (data + (destwidth - savedpos),
				  uih->image->oldlines[savedline], width1);
			  savedpos = width1;
			}
		      else
			memcpy (data,
				uih->image->oldlines[savedline] + savedpos,
				width), savedpos += width;
		    }
		  w->savedline = -1;
		}
	      else
		{
		  assert (w->saveddata);
		  for (i = w->y; i < w->y + w->height; i++)
		    {
		      unsigned char *data = uih->image->currlines[i] + xskip;
		      memcpy (data, w->saveddata + (i - w->y) * width, width);
		    }
		  free (w->saveddata);
		  w->saveddata = NULL;
		}
	    }
	}
      w = w->next;
    }
}
void
uih_drawwindows (struct uih_context *uih)
{
  struct uih_window *w = uih->wtop;
  struct image *img = uih->image;
  int size = 0;
  int nocopy = 0;
  int savedline = 0;
  int savedpos = 0;
  int destwidth = uih->image->width * uih->image->bytesperpixel;
  if (!uih->image->bytesperpixel)
    {
      destwidth = (w->x + uih->image->width + 7) / 8;
    }
  if (uih->wdisplayed)
    return;
  uih->wdisplayed = 1;
  while (w)
    {
      if (w->getpos != NULL)
	{
	  int test = w->x == -65536;
	  w->getpos (uih, &w->x, &w->y, &w->width, &w->height, w->data);
	  if (w->x < 0)
	    w->width -= w->x, w->x = 0;
	  if (w->y < 0)
	    w->height -= w->y, w->y = 0;
	  if (w->x > img->width)
	    w->width = 0, w->height = 0, w->x = 0;
	  if (w->y > img->height)
	    w->width = 0, w->height = 0, w->y = 0;
	  if (w->x + w->width > img->width)
	    w->width = img->width - w->x;
	  if (w->y + w->height > img->height)
	    w->height = img->height - w->y;
	  if (w->width < 0)
	    w->width = 0;
	  if (w->height < 0)
	    w->height = 0;
	  if (test)
	    {
	      struct uih_window *w1 = uih->wtop;
	      while (w1)
		{
		  if (w != w1 && (w1->flags & DRAWBORDER) &&
		      ((((w1->x > w->x + 5 && w1->x + 5 < w->x + w->width) ||
			 (w->x > w1->x + 5 && w->x + 5 < w1->x + w1->width))
			&& ((w1->y > w->y + 5 && w1->y + 5 < w->y + w->height)
			    || (w->y > w1->y + 5
				&& w->y + 5 < w1->y + w1->height)))
		       ||
		       (((w1->x + w1->width > w->x + 5
			  && w1->x + w1->width + 5 < w->x + w->width)
			 || (w->x + w->width > w1->x + 5
			     && w->x + w->width + 5 < w1->x + w1->width))
			&&
			((w1->y + w1->height > w->y + 5
			  && w1->y + w1->height + 5 < w->y + w->height)
			 || (w->y + w->height > w1->y + 5
			     && w->y + w->height + 5 < w1->y + w1->height)))))
		    {
		      w->flags &= ~BORDER_TRANSPARENT;
		      break;
		    }
		  w1 = w1->next;
		}
	    }
	  size += w->width * w->height;
	  if (w->x == 0 && w->y == 0 && w->width == img->width
	      && w->height == img->height)
	    nocopy = 1;
	  assert (w->width >= 0);
	  assert (w->height >= 0);
	  assert (w->x >= 0);
	  assert (w->y >= 0);
	  assert (w->x + w->width <= uih->image->width);
	  assert (w->y + w->height <= uih->image->height);
	}
      w = w->next;
    }
  if (size > img->width * img->height / 2)
    {
      int i;
      int width = img->width * img->bytesperpixel;
      if (!width)
	width = (img->width + 7) / 8;
      uih->wflipped = 1;
      if (!nocopy)
	for (i = 0; i < img->height; i++)
	  memcpy (img->oldlines[i], img->currlines[i], width);
      uih->image->flip (uih->image);
    }
  else
    {
      int savedminx = -1;
      int savedmaxx = -1;
      int savedminy = -1;
      int savedmaxy = -1;
      uih->wflipped = 0;
      w = uih->wtop;
      while (w)
	{
	  int i;
	  assert (w->saveddata == NULL);
	  if (w->getpos == NULL)
	    {
	      if ((w->x < savedminx || w->y < savedminy
		   || w->x + w->width > savedmaxx
		   || w->x + w->height > savedmaxy
		   || w->x + w->width < savedminx
		   || w->y + w->height < savedminy || w->x > savedmaxx
		   || w->y > savedmaxy))
		{
		  w->saveddata =
		    xsaveline (uih->image, w->x, w->y, w->width + w->x,
			       w->height + w->y);
		}
	    }
	  else
	    {
	      assert (w->savedline == -1);
	      if (w->width && w->height
		  && (w->x < savedminx || w->y < savedminy
		      || w->x + w->width > savedmaxx
		      || w->y + w->height > savedmaxy))
		{
		  int xskip = w->x * uih->image->bytesperpixel;
		  int width = w->width * uih->image->bytesperpixel;
		  savedminx = w->x;
		  savedminy = w->y;
		  savedmaxx = w->x + w->width;
		  savedmaxy = w->y + w->height;
		  if (!uih->image->bytesperpixel)
		    {
		      xskip = w->x / 8;
		      width = (w->x + w->width + 7) / 8 - xskip;
		    }
		  if (uih->image->flags & PROTECTBUFFERS || 1)
		    {
		      w->saveddata = (char *) malloc (width * w->height + 1);
		      if (w->saveddata != NULL)
			for (i = w->y; i < w->y + w->height; i++)
			  {
			    unsigned char *data = img->currlines[i] + xskip;
			    memcpy (w->saveddata + (i - w->y) * width, data,
				    width);
			  }

		    }
		  else
		    {
		      w->savedline = savedline;
		      w->savedpos = savedpos;
		      for (i = w->y; i < w->y + w->height; i++)
			{
			  unsigned char *data = img->currlines[i] + xskip;
			  if (width + savedpos > destwidth)
			    {
			      int width1;
			      memcpy (uih->image->oldlines[savedline] +
				      savedpos, data, destwidth - savedpos);
			      savedline++;
			      width1 = width - destwidth + savedpos;
			      memcpy (uih->image->oldlines[savedline],
				      data + (destwidth - savedpos), width1);
			      savedpos = width1;
			    }
			  else
			    memcpy (uih->image->oldlines[savedline] +
				    savedpos, data, width), savedpos += width;
			}
		    }
		}
	    }
	  w = w->next;
	}
    }
  w = uih->wtop;
  while (w)
    {
      if (w->getpos == NULL)
	xline (uih->image, w->x, w->y, w->width + w->x, w->height + w->y,
	       w->savedline);
      else if (w->width && w->height)
	{
	  if (w->flags & DRAWBORDER)
	    uih_drawborder (uih, w->x, w->y, w->width, w->height,
			    (BORDER_TRANSPARENT) & w->flags);
	  w->draw (uih, w->data);
	}
      w = w->next;
    }
}
