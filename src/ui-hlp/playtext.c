#ifndef _plan9_
#include <stdlib.h>
#else
#include <u.h>
#include <libc.h>
#endif
#include <fconfig.h>
#include <filter.h>
#include <fractal.h>
#include <ui_helper.h>
#include <xerror.h>
#include <grlib.h>

static inline void
prepare (struct uih_context *c, char *string, int *xmax, int *nr)
{
  int xm = 0;
  int n = 1;
  int pos = 0;
  int tmp;
  while (1)
    {
      tmp = 0;
      while (*string != '\n')
	{
	  if (*string == 0)
	    {
	      tmp++;
	      if (tmp > xm)
		xm = tmp;
	      *xmax = xm;
	      *nr = n;
	      return;
	    }
	  else
	    tmp += xtextcharw (c->font, *string);
	  if (pos > 255)
	    break;
	  string++;
	  pos++;
	}
      tmp++;
      if (tmp > xm)
	xm = tmp;
      n++;
      if (n > 30)
	{
	  n = 30;
	  *xmax = tmp;
	  *nr = n;
	  return;
	}
      pos = 0;
      string++;
    }
}
static void
getpos (uih_context * c, int *x, int *y, int *w, int *h, void *data)
{
  int num = (int) data;
  int xmax, nr;
  if (c->text[num] == NULL)
    {
      *x = *y = *h = *w;
      return;
    }
  prepare (c, c->text[num], &xmax, &nr);
  nr *= xtextheight (c->font);
  switch (num)
    {
    case 0:
      *y = 0;
      break;
    case 1:
      *y = (c->image->height - nr) / 2;
      break;
    case 2:
      *y = c->image->height - nr;
      break;
    }
  *h = nr;
  switch (c->textpos[num])
    {
    case 0:
      *x = 0;
      break;
    case 1:
      *x = (c->image->width - xmax) / 2;
      break;
    case 2:
      *x = c->image->width - xmax;
      break;
    }
  *w = xmax;
}
static void
draw (uih_context * c, void *data)
{
  int num = (int) data;
  int flags = 0;
  int xmax, n, nr, i;
  int x = 0, y = 0;
  char *string;
  int fgcolor = 0, bgcolor = 0;
  if (c->text[num] == NULL)
    return;
  prepare (c, c->text[num], &xmax, &n);
  nr = n * xtextheight (c->font);
  switch (c->textcolor[num])
    {
    case 0:
      fgcolor = FGCOLOR (c);
      bgcolor = BGCOLOR (c);
      break;
    case 1:
      fgcolor = BGCOLOR (c);
      bgcolor = BGCOLOR (c);
      flags = TEXT_PRESSED;
      break;
    case 2:
      fgcolor = SELCOLOR (c);
      bgcolor = BGCOLOR (c);
      break;
    default:
      x_fatalerror ("playtext:unknown color\n");
    }
  if (c->image->flags & AAIMAGE)
    fgcolor = BGCOLOR (c);
  switch (num)
    {
    case 0:
      y = 0;
      break;
    case 1:
      y = (c->image->height - nr) / 2;
      break;
    case 2:
      y = c->image->height - nr;
      break;
    }
  string = c->text[num];
  for (i = 0; i < n; i++)
    {
      xmax = xtextwidth (c->font, string);
      switch (c->textpos[num])
	{
	case 0:
	  x = 0;
	  break;
	case 1:
	  x = (c->image->width - xmax) / 2;
	  break;
	case 2:
	  x = c->image->width - xmax;
	  break;
	}
      string +=
	xprint (c->image, c->font, x, y, string, fgcolor, bgcolor, flags) + 1;
      y += xtextheight (c->font);
    }
}
void
uih_inittext (uih_context * c)
{
  c->text[0] = c->text[1] = c->text[2] = NULL;
  c->textpos[0] = c->textpos[1] = c->textpos[2] = 0;
  c->textwindow[0] = uih_registerw (c, getpos, draw, (void *) 0, 0);
  c->textwindow[1] = uih_registerw (c, getpos, draw, (void *) 1, 0);
  c->textwindow[2] = uih_registerw (c, getpos, draw, (void *) 2, 0);
}

void
uih_destroytext (uih_context * c)
{
  if (c->text[0] != NULL)
    free (c->text[0]), c->text[0] = NULL;
  if (c->text[1] != NULL)
    free (c->text[1]), c->text[1] = NULL;
  if (c->text[2] != NULL)
    free (c->text[2]), c->text[2] = NULL;
  uih_removew (c, c->textwindow[0]);
  uih_removew (c, c->textwindow[1]);
  uih_removew (c, c->textwindow[2]);
}
