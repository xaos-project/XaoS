#include <config.h>
#ifndef _plan9_
#include <limits.h>
#ifdef NO_MALLOC_H
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <string.h>
#else
#include <u.h>
#include <libc.h>
#endif
#include <fconfig.h>
#include <filter.h>
#include <fractal.h>
#include <ui_helper.h>
#include <catalog.h>
#include <xmenu.h>
#include <grlib.h>
#include "play.h"

#ifdef HAVE_GETTEXT
#include <libintl.h>
#else
#define gettext(STRING) STRING
#endif

#define nextchar() (uih->playstring==NULL?xio_getc(FD):uih->playstring[uih->playpos++])
#define ungetchar(c) (uih->playstring==NULL?xio_ungetc(c,FD):uih->playpos--)
#define endoffile() (uih->playstring==NULL?xio_feof(FD):uih->playstring[uih->playpos]==0)
static int nonblockmode;
static catalog_t *catalog;	/*The message catalog should be "session wide" */
CONST static char *errstring;
#define seterr(str) {if(errstring==NULL) errstring=str;}
#define FD uih->playc->file
static char token[1024];
static int first;
static int last;
static int parsenext;

static CONST char *CONST animroot = "animroot";

static inline struct uih_line *
uih_findkey (uih_context * c, int key)
{
  struct uih_line *l = c->playc->lines.first;
  while (l != NULL)
    {
      if (l->key == key)
	return l;
      l = l->next;
    }
  return NULL;
}
static inline void
uih_removeline (uih_context * c, struct uih_line *l)
{
  if (l == NULL)
    return;
  uih_removew (c, l->w);
  if (l->prev)
    l->prev->next = l->next;
  else
    c->playc->lines.first = l->next;
  if (l->next)
    l->next->prev = l->prev;
  free (l);
}

void
uih_line (uih_context * c, dialogparam * d)
{
  struct uih_window *w;
  struct uih_line *l;
  if (c->playstring != NULL)
    {
      seterr (gettext ("line available only in animation replay"));
      return;
    }
  w = uih_registerline (c, 0, -1, -1, -1, -1);
  uih_removeline (c, uih_findkey (c, c->playc->lines.currkey));
  l = (struct uih_line *) calloc (1, sizeof (*l));
  l->posmode = d[0].dint;
  l->w = w;
  l->x1 = d[1].dcoord[0];
  l->y1 = d[1].dcoord[1];
  l->x2 = d[2].dcoord[0];
  l->y2 = d[2].dcoord[1];
  l->color = c->color;
  l->morph = 0;
  l->key = c->playc->lines.currkey++;
  l->prev = NULL;
  l->next = c->playc->lines.first;
  c->playc->lines.first = l;
}

void
uih_morphline (uih_context * c, dialogparam * d)
{
  struct uih_line *l;
  l = uih_findkey (c, c->playc->lines.currkey);
  if (l == NULL)
    {
      seterr (gettext ("Morphing non existing line!"));
      return;
    }
  c->playc->lines.currkey++;
  l->mposmode = d[0].dint;
  l->mx1 = d[1].dcoord[0];
  l->my1 = d[1].dcoord[1];
  l->mx2 = d[2].dcoord[0];
  l->my2 = d[2].dcoord[1];
  l->morph = 1;
  c->playc->lines.morphing = 1;
}

void
uih_morphlastline (uih_context * c, dialogparam * d)
{
  c->playc->lines.currkey--;
  uih_morphline (c, d);
}

void
uih_setkey (uih_context * c, int line)
{
  if (!c->play)
    {
      seterr (gettext ("linekey not available in this context!"));
      return;
    }
  c->playc->lines.currkey = line;
}
static void
uih_stopmorphing (uih_context * c)
{
  struct uih_line *l = c->playc->lines.first;
  while (l)
    {
      if (l->morph)
	{
	  l->x1 = l->mx1;
	  l->y1 = l->my1;
	  l->x2 = l->mx2;
	  l->y2 = l->my2;
	  l->posmode = l->mposmode;
	  l->morph = 0;
	  c->playc->lines.morphing = 1;
	}
      l = l->next;
    }
}
void
uih_update_lines (uih_context * c)
{
  int m = 0;
  int co;
  struct uih_line *l = c->playc->lines.first;
  int x1, y1, x2, y2;
  number_t x, y;
  int timer = tl_lookup_timer (c->playc->timer) - c->playc->starttime;
  number_t mmul =		/*(tl_lookup_timer (c->playc->timer) - c->playc->starttime) / (number_t) (c->playc->frametime - c->playc->starttime); */
    MORPHVALUE (timer, c->playc->frametime - c->playc->starttime,
		c->playc->morphlinetimes[0], c->playc->morphlinetimes[1]);


  while (l)
    {
      switch (l->posmode)
	{
	case 0:
	  x1 = (int) (c->image->width * l->x1);
	  y1 = (int) (c->image->height * l->y1);
	  x2 = (int) (c->image->width * l->x2);
	  y2 = (int) (c->image->height * l->y2);
	  break;
	case 1:
	  x = c->image->width * c->image->pixelwidth;
	  y = c->image->height * c->image->pixelheight;
	  if (x > y)
	    x = y;
	  x1 =
	    (int) (c->image->width / 2 +
		   (l->x1 - 0.5) * x / c->image->pixelwidth);
	  y1 =
	    (int) (c->image->height / 2 +
		   (l->y1 - 0.5) * x / c->image->pixelheight);
	  x2 =
	    (int) (c->image->width / 2 +
		   (l->x2 - 0.5) * x / c->image->pixelwidth);
	  y2 =
	    (int) (c->image->height / 2 +
		   (l->y2 - 0.5) * x / c->image->pixelheight);
	  break;
	case 2:
	  x = l->x1;
	  y = l->y1;
	  rotate (*(c->fcontext), x, y);
	  x =
	    (x - c->fcontext->rs.nc) / (c->fcontext->rs.mc -
					c->fcontext->rs.nc) *
	    c->zengine->image->width;
	  y =
	    (y - c->fcontext->rs.ni) / (c->fcontext->rs.mi -
					c->fcontext->rs.ni) *
	    c->zengine->image->height;
	  x1 = (int) x;
	  y1 = (int) y;
	  c->zengine->action->convertup (c->zengine, &x1, &y1);
	  x = l->x2;
	  y = l->y2;
	  rotate (*(c->fcontext), x, y);
	  x =
	    (x - c->fcontext->rs.nc) / (c->fcontext->rs.mc -
					c->fcontext->rs.nc) *
	    c->zengine->image->width;
	  y =
	    (y - c->fcontext->rs.ni) / (c->fcontext->rs.mi -
					c->fcontext->rs.ni) *
	    c->zengine->image->height;
	  x2 = (int) x;
	  y2 = (int) y;
	  c->zengine->action->convertup (c->zengine, &x2, &y2);
	  break;
	}
      if (l->morph)
	{
	  int mx1, mx2, my1, my2;
	  m = 1;
	  switch (l->mposmode)
	    {
	    case 0:
	      mx1 = (int) (c->image->width * l->mx1);
	      my1 = (int) (c->image->height * l->my1);
	      mx2 = (int) (c->image->width * l->mx2);
	      my2 = (int) (c->image->height * l->my2);
	      break;
	    case 1:
	      x = c->image->width * c->image->pixelwidth;
	      y = c->image->height * c->image->pixelheight;
	      if (x > y)
		x = y;
	      mx1 =
		(int) (c->image->width / 2 +
		       (l->mx1 - 0.5) * x / c->image->pixelwidth);
	      my1 =
		(int) (c->image->height / 2 +
		       (l->my1 - 0.5) * x / c->image->pixelheight);
	      mx2 =
		(int) (c->image->width / 2 +
		       (l->mx2 - 0.5) * x / c->image->pixelwidth);
	      my2 =
		(int) (c->image->height / 2 +
		       (l->my2 - 0.5) * x / c->image->pixelheight);
	      break;
	    default:
	      x = l->mx1;
	      y = l->my1;
	      rotate (*(c->fcontext), x, y);
	      x =
		(x - c->fcontext->rs.nc) / (c->fcontext->rs.mc -
					    c->fcontext->rs.nc) *
		c->zengine->image->width;
	      y =
		(y - c->fcontext->rs.ni) / (c->fcontext->rs.mi -
					    c->fcontext->rs.ni) *
		c->zengine->image->height;
	      mx1 = (int) x;
	      my1 = (int) y;
	      c->zengine->action->convertup (c->zengine, &mx1, &my1);
	      x = l->mx2;
	      y = l->my2;
	      rotate (*(c->fcontext), x, y);
	      x =
		(x - c->fcontext->rs.nc) / (c->fcontext->rs.mc -
					    c->fcontext->rs.nc) *
		c->zengine->image->width;
	      y =
		(y - c->fcontext->rs.ni) / (c->fcontext->rs.mi -
					    c->fcontext->rs.ni) *
		c->zengine->image->height;
	      mx2 = (int) x;
	      my2 = (int) y;
	      c->zengine->action->convertup (c->zengine, &mx2, &my2);
	      break;
	    }
	  x1 = (int) (x1 + (mx1 - x1) * mmul);
	  y1 = (int) (y1 + (my1 - y1) * mmul);
	  x2 = (int) (x2 + (mx2 - x2) * mmul);
	  y2 = (int) (y2 + (my2 - y2) * mmul);
	}
      switch (l->color)
	{
	case 1:
	  co = BGCOLOR (c);
	  break;
	case 0:
	  co = FGCOLOR (c);
	  break;
	default:
	  co = SELCOLOR (c);
	  break;
	}
      uih_setline (c, l->w, co, x1, y1, x2, y2);

      l = l->next;
    }
  c->playc->lines.morphing = m;
  if (m)
    c->display = 1;

}

void
uih_clear_line (uih_context * c)
{
  if (c->playstring != NULL)
    {
      seterr (gettext ("clear_line available only in animation replay"));
      return;
    }
  uih_removeline (c, uih_findkey (c, c->playc->lines.currkey++));
}

void
uih_clear_lines (uih_context * c)
{
  if (c->playstring != NULL)
    {
      seterr (gettext ("clear_lines available only in animation replay"));
      return;
    }
  while (c->playc->lines.first != NULL)
    uih_removeline (c, c->playc->lines.first);
  c->playc->lines.currkey = 0;
}

void
uih_freecatalog (uih_context * c)
{
  if (catalog != NULL)
    free_catalog (catalog), catalog = NULL;
}

void
uih_setfont (struct uih_context *uih)
{
  if (catalog != NULL && find_text (catalog, "encoding")
      && find_text (catalog, "encoding")[0] == '2')
    uih->encoding = 2;
  else
    uih->encoding = 1;
  if (uih->image->flags & AAIMAGE)
    uih->font = &xaafont;
  else
    {
      if (uih->encoding == 2)
	{
	  if (uih->image->pixelheight < 0.06)
	    uih->font = &xbigfont;
	  else
	    uih->font = &xsmallfont;
	}
      else
	{
	  if (uih->image->pixelheight < 0.04)
	    uih->font = &xbigfontil1;
	  else if (uih->image->pixelheight < 0.07)
	    uih->font = &xmedfontil1;
	  else
	    uih->font = &xsmallfontil1;
	}
    }
}

int
uih_loadcatalog (uih_context * c, CONST char *name)
{
  static int firsttime = 1;
  static CONST char *str;
  xio_file f = xio_getcatalog (name);
  if (f == XIO_FAILED)
    {
      if (firsttime)
	{
	  firsttime = 0;
	  return 0;
	}			/*Let XaoS work as stand alone executable */
      uih_error (c, gettext ("Catalog file not found"));
      return 0;
    }
  firsttime = 0;
  if (catalog != NULL)
    free_catalog (catalog);
  catalog = load_catalog (f, &str);
  if (str != NULL)
    uih_error (c, str);
  uih_setfont (c);
  return (catalog != NULL);
}
static void
handler (void *userdata)
{
  struct uih_context *uih = (struct uih_context *) userdata;
  uih->playc->playframe++;
  uih->inanimation = 2;
  if (uih->playc->timerin)
    tl_remove_timer (uih->playc->timer);
  uih->playc->timerin = 0;
}
static void
handler1 (void *userdata)
{
  struct uih_context *uih = (struct uih_context *) userdata;
  uih->playc->playframe++;
  uih->inanimation = 2;
  tl_update_time ();
  tl_reset_timer (uih->playc->timer);
  uih_setcomplettehandler (uih, NULL, NULL);
}

void
uih_skipframe (struct uih_context *uih)
{
  if (uih->play && uih->playc->timerin)
    handler (uih), tl_reset_timer (uih->playc->timer);
}

int
uih_replayenable (struct uih_context *uih, xio_file f, xio_constpath filename,
		  int animr)
{
  struct uih_playcontext *p;
  CONST char *s;
  if (uih->play)
    {
      uih_error (uih, gettext ("Replay is already active"));
      return 0;
    }
  if (f == XIO_FAILED)
    {
      uih_error (uih, gettext ("File open failed"));
      return 0;
    }
  p = (struct uih_playcontext *) calloc (1, sizeof (*p));
  if (p == NULL)
    {
      uih_error (uih, gettext ("Out of memory"));
      return 0;
    }
  if (animr)
    {
      uih->menuroot = animroot;
      uih_updatemenus (uih, NULL);
    }
  p->file = f;
  p->playframe = 1;
  p->timer = tl_create_timer ();
  p->frametime = 0;
  p->morph = 0;
  p->morphjulia = 0;
  p->lines.first = NULL;
  p->lines.morphing = 0;
  p->lines.currkey = 0;
  tl_update_time ();
  tl_set_handler (p->timer, handler, uih);
  uih_stoptimers (uih);
  if (uih->stoppedtimers)
    tl_stop_timer (p->timer);
  uih->playc = p;
  uih->play = 1;
  uih_emulatetimers (uih);
  tl_reset_timer (p->timer);
  uih->playc->line = 1;
  if (filename != NULL)
    {
      uih->playc->directory = xio_getdirectory (filename);
    }
  else
    {
      uih->playc->directory = xio_getdirectory (XIO_EMPTYPATH);
    }
  uih->playc->level = 0;
  s = uih->playstring;
  uih->playstring = NULL;
  uih_playupdate (uih);
  uih->playstring = s;
  return 1;
}

void
uih_replaydisable (struct uih_context *uih)
{
  if (uih->play)
    {
      int i;
      uih->play = 0;
      tl_free_timer (uih->playc->timer);
      if (uih->menuroot == animroot)
	{
	  uih->menuroot = "root";
	  uih_updatemenus (uih, NULL);
	}
      xio_close (uih->playc->file);
      for (i = 0; i < uih->playc->level; i++)
	xio_close (uih->playc->prevfiles[i]);
      uih->display = 1;
      uih->nonfractalscreen = 0;
      uih_setcomplettehandler (uih, NULL, NULL);
      uih_clear_lines (uih);
      free (uih->playc->directory);
      free (uih->playc);
      uih_display (uih);
    }
}
static void
skipblank (struct uih_context *uih)
{
  int c;
  if (uih->playstring != NULL)
    {
      while ((uih->playstring[uih->playpos] == ' ' ||
	      uih->playstring[uih->playpos] == '\t' ||
	      uih->playstring[uih->playpos] == '\r' ||
	      uih->playstring[uih->playpos] == '\n'))
	uih->playpos++;
      return;
    }
  do
    {
      c = xio_getc (FD);
      if (c == '\n')
	uih->playc->line++;
      if (c == ';')
	while (c != '\n' && !xio_feof (FD))
	  {
	    c = xio_getc (FD);
	    if (c == '\n')
	      uih->playc->line++;
	  }
      while (xio_feof (FD) && uih->playc->level)
	{
	  c = XIO_EOF + 1;
	  xio_close (FD);
	  uih->playc->file = uih->playc->prevfiles[--uih->playc->level];
	  uih->playc->line = 1;
	}
    }
  while (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == XIO_EOF + 1);
  if (c != XIO_EOF)
    xio_ungetc (c, FD);
}
static int
gettoken (struct uih_context *uih)
{
  int c;
  int i = 0;
  skipblank (uih);
  if (first && ((c = nextchar ()) != '('))
    {
      if (c && !endoffile ())
	{
	  seterr ("'(' expected");
	}
      last = 1;
      return -1;
    }
  if (first)
    skipblank (uih), first = 0;
  if (endoffile ())
    {
      if (uih->playstring)
	{
	  seterr (gettext ("Missing parameter"));
	}
      else
	seterr (gettext ("Unexpected end of file"));
      return 0;
    }
  if ((c = nextchar ()) == '"')
    {
      while (c == '\r')
	c = nextchar ();
      token[i] = '"';
      i++;
      do
	{
	  c = nextchar ();
	  while (c == '\r')
	    c = nextchar ();
	  if (c == XIO_EOF || c == 0)
	    {
	      if (uih->playstring)
		{
		  seterr (gettext ("Missing parameter"));
		}
	      else
		seterr (gettext ("Unexpected end of file"));
	      return 0;
	    }
	  if (c == '\n' && uih->playstring == NULL)
	    uih->playc->line++;
	  if (c == '\\')
	    token[i] = nextchar ();
	  else
	    token[i] = c;
	  i++;
	  if (i >= 1024)
	    {
	      seterr (gettext ("Token is too long"));
	      i = 0;
	    }
	}
      while (c != '"');
    }
  else
    ungetchar (c);
  do
    {
      c = nextchar ();
      if (c == XIO_EOF || c == 0)
	{
	  if (uih->playstring)
	    {
	      seterr (gettext ("Missing parameter"));
	    }
	  else
	    seterr (gettext ("Unexpected end of file"));
	  return 0;
	}
      token[i] = c;
      i++;
      if (i >= 1024)
	{
	  seterr (gettext ("Token is too long"));
	  i = 0;
	}
    }
  while (c != ' ' && c != '\t' && c != ')' && c != '\n' && c != '\r');
  i--;
  token[i] = 0;
  skipblank (uih);
  if (c == ')')
    {
      last = 1;
      return i;
    }
  c = nextchar ();
  if (uih->playstring == NULL)
    {
      while (xio_feof (FD) && uih->playc->level)
	uih->playc->file =
	  uih->playc->prevfiles[--uih->playc->level], uih->playc->line = 1;
    }
  if (c == XIO_EOF || c == 0)
    {
      if (uih->playstring)
	{
	  seterr (gettext ("Missing parameter"));
	}
      else
	seterr (gettext ("Unexpected end of file"));
      return 0;
    }
  if (c == ')')
    {
      last = 1;
      return i;
    }
  ungetchar (c);
  return i;
}
static char *
gettokenwr (struct uih_context *c)
{
  if (last)
    return NULL;
  if (gettoken (c) < 0)
    return NULL;
  if (errstring)
    return NULL;
  return (token);
}

void
uih_play_formula (struct uih_context *uih, char *fname)
{
  int i;
  for (i = 0; i < nformulas; i++)
    {
      if (!strcmp (formulas[i].shortname, fname))
	{
	  set_formula (uih->fcontext, i);
	  uih_newimage (uih);
	  return;
	}
    }
  seterr (gettext ("Unknown formula type"));
}

void
uih_playmorph (struct uih_context *uih, dialogparam * d)
{
  if (uih->playstring != NULL)
    {
      seterr (gettext ("morph available only in animation replay"));
      return;
    }
  if (d[2].number <= 0 || d[3].number <= 0)
    {
      seterr (gettext ("morphview: Invalid viewpoint"));
      uih->playc->destination = uih->fcontext->currentformula->v;
    }
  uih->playc->source = uih->fcontext->s;
  uih->playc->destination.cr = d[0].number;
  uih->playc->destination.ci = d[1].number;
  uih->playc->destination.rr = d[2].number;
  uih->playc->destination.ri = d[3].number;
  uih->playc->morph = 1;
}

void
uih_playmove (struct uih_context *uih, number_t x, number_t y)
{
  if (uih->playstring != NULL)
    {
      seterr (gettext ("move available only in animation replay"));
      return;
    }
  uih->playc->source = uih->fcontext->s;
  uih->playc->destination.cr = x;
  uih->playc->destination.ci = y;
  uih->playc->destination.rr = uih->fcontext->s.rr;
  uih->playc->destination.ri = uih->fcontext->s.ri;
  uih->playc->morph = 1;
}

void
uih_playmorphjulia (struct uih_context *uih, number_t x, number_t y)
{
  if (uih->playstring != NULL)
    {
      seterr (gettext ("morphjulia available only in animation replay"));
      return;
    }
  uih->playc->sr = uih->fcontext->pre;
  uih->playc->si = uih->fcontext->pim;
  uih->playc->dr = x;
  uih->playc->di = y;
  uih->playc->morphjulia = 1;
}

void
uih_playmorphangle (struct uih_context *uih, number_t angle)
{
  if (uih->playstring != NULL)
    {
      seterr (gettext ("morphangle available only in animation replay"));
      return;
    }
  uih->playc->morphangle = 1;
  uih->playc->srcangle = uih->fcontext->angle;
  uih->playc->destangle = angle;
}

void
uih_playautorotate (struct uih_context *uih, int mode)
{
  if (mode)
    {
      uih_fastrotateenable (uih);
      uih_rotatemode (uih, ROTATE_CONTINUOUS);
    }
  else
    uih_rotatemode (uih, ROTATE_NONE);
}

void
uih_playfilter (struct uih_context *uih, dialogparam * p)
{
  CONST char *fname = p[0].dstring;
  int mode;
  int i;
  for (i = 0; i < uih_nfilters; i++)
    {
      if (!strcmp (uih_filters[i]->shortname, fname))
	{
	  mode = p[1].dint;
	  if (mode)
	    uih_enablefilter (uih, i);
	  else
	    uih_disablefilter (uih, i);
	  return;
	}
    }
  seterr (gettext ("Unknown filter"));
}

void
uih_playdefpalette (struct uih_context *uih, int shift)
{
  if (uih->zengine->fractalc->palette == NULL)
    return;
  if (mkdefaultpalette (uih->zengine->fractalc->palette) != 0)
    {
      uih_newimage (uih);
    }
  uih->palettetype = 0;
  uih->palettechanged = 1;
  if (shiftpalette (uih->zengine->fractalc->palette, shift))
    {
      uih_newimage (uih);
    }
  uih->manualpaletteshift = 0;
  uih->paletteshift = shift;
}

void
uih_zoomcenter (struct uih_context *uih, number_t x, number_t y)
{
  uih->xcenter = x;
  uih->ycenter = y;
  uih->xcenterm = INT_MAX;
  uih->ycenterm = INT_MAX;
}
extern char *xtextposnames[];
extern char *ytextposnames[];
void
uih_playtextpos (struct uih_context *uih, dialogparam * p)
{
  int x, y;
  x = p[0].dint;
  y = p[1].dint;
  uih_settextpos (uih, x, y);
}

void
uih_playusleep (struct uih_context *uih, int time)
{
  parsenext = 0;
  if (uih->playstring != NULL)
    {
      seterr (gettext ("sleep available only in animation replay"));
      return;
    }
  uih->playc->frametime = time;
  if (time < tl_lookup_timer (uih->playc->timer)	/*&&((!uih->step)||(!uih->zoomactive)) */
    )
    {
      tl_slowdown_timer (uih->playc->timer, time);
      uih->playc->playframe++;
    }
  else
    {
      tl_set_interval (uih->playc->timer, time);
      if (!uih->playc->timerin)
	{
	  uih->playc->timerin = 1;
	  tl_add_timer (syncgroup, uih->playc->timer);
	}
      else
	printf (gettext ("Internal program error #12 %i\n"),
		uih->playc->playframe);
    }
  uih->playc->starttime = tl_lookup_timer (uih->playc->timer);
}

void
uih_playtextsleep (struct uih_context *uih)
{
  uih_playusleep (uih,
		  500000 + 1000000 * (uih->nletters +
				      uih->todisplayletters) /
		  uih->letterspersec);
  uih->nletters = 0;
  uih->todisplayletters = 0;
}


void
uih_playwait (struct uih_context *uih)
{
  parsenext = 0;
  if (uih->playstring != NULL)
    {
      seterr (gettext ("wait available only in animation replay"));
      return;
    }
  if (!uih->uncomplette && !uih->display && !uih->recalculatemode
      && !uih->displaytext && !uih->clearscreen)
    {
      uih->playc->playframe++;
    }
  else
    {
      uih_setcomplettehandler (uih, handler1, uih);
    }
}
void
uih_playjulia (struct uih_context *uih, int julia)
{
  julia = !julia;
  if (julia != uih->fcontext->mandelbrot)
    {
      uih->fcontext->mandelbrot = julia;
      uih->fcontext->version++;
      uih_updatemenus (uih, "uimandelbrot");
      uih_newimage (uih);
    }
}
void
uih_playcalculate (struct uih_context *uih)
{
  uih_newimage (uih);
}

void
uih_playzoom (struct uih_context *uih)
{
  uih->zoomactive = 1;
}

void
uih_playunzoom (struct uih_context *uih)
{
  uih->zoomactive = -1;
}

void
uih_playstop (struct uih_context *uih)
{
  uih->zoomactive = 0;
}

void
uih_playmessage (struct uih_context *uih, char *name)
{
  char *message;
  if (catalog == NULL)
    {
      uih_text (uih, gettext ("No catalog file loaded"));
      return;
    }
  message = find_text (catalog, name);
  if (message == NULL)
    {
      uih_text (uih, gettext ("Message not found in catalog file"));
      return;
    }
  uih_text (uih, message);
}

void
uih_playload (struct uih_context *uih, xio_path file)
{
  xio_file f;
  xio_pathdata tmp;
  if (uih->playstring != NULL)
    {
      seterr (gettext ("load available only in animation replay"));
      return;
    }
  if (uih->playc->level == MAXLEVEL)
    {
      seterr (gettext ("Include level overflow"));
      return;
    }


  xio_addfname (tmp, uih->playc->directory, file);
  f = xio_ropen (tmp);

  if (f == XIO_FAILED)
    {
      seterr (gettext ("File not found"));
      return;
    }
  uih->playc->prevfiles[uih->playc->level] = uih->playc->file;
  uih->playc->level++;
  uih->playc->file = f;
  uih->playc->line = 1;
}
static void
uih_processcommand (struct uih_context *uih, int flags)
{
  CONST char *error;
  first = 1;
  last = 0;
  error = menu_processcommand (uih, gettokenwr, 1, flags, uih->menuroot);
  if (error != NULL)
    seterr (error);
  if (!last)
    {
      seterr (gettext ("Too many parameters"));
    }
}
void
uih_playupdate (struct uih_context *uih)
{
  static char errtext[1024];
  errstring = NULL;
  while (uih->play && uih->playc->playframe && errstring == NULL)
    {
      parsenext = 1;
      uih->playc->playframe--;
      if (uih->playc->lines.morphing)
	{
	  uih_stopmorphing (uih);
	  uih->display = 1;
	}
      if (uih->playc->morph)
	{
	  uih->fcontext->s = uih->playc->destination;
	  uih_animate_image (uih);
	  uih->playc->morph = 0;
	}
      if (uih->playc->morphangle)
	{
	  uih_angle (uih, uih->playc->destangle);
	  uih->playc->morphangle = 0;
	}
      if (uih->playc->morphjulia)
	{
	  uih_setjuliaseed (uih, uih->playc->dr, uih->playc->di);
	  uih->playc->morphjulia = 0;
	}
      while (!xio_feof (FD) && parsenext && errstring == NULL)
	{
	  uih_processcommand (uih, MENUFLAG_NOPLAY);
	}			/*while parsenext */
      uih_update_lines (uih);
      if (errstring != NULL)
	{
	  uih_error (uih, errstring);
	  if (uih->play)
	    {
	      sprintf (errtext, gettext ("Replay disabled at line %i"),
		       uih->playc->line);
	      uih_message (uih, errtext);
	    }
	  /*errstring[255]=0; */
	}
      if ((xio_feof (FD) && parsenext) || errstring)
	{
	  uih_replaydisable (uih);
	}
    }				/*while play&&playframe */
}
void
uih_load (struct uih_context *uih, xio_file f, xio_constpath filename)
{
  nonblockmode = 1;
  uih_replayenable (uih, f, filename, 0);
  uih_replaydisable (uih);
  nonblockmode = 0;
}

void
uih_command (struct uih_context *uih, CONST char *command)
{
  errstring = NULL;
  uih->playpos = 0;
  uih->playstring = command;
  uih_processcommand (uih, (uih->play ? MENUFLAG_NOMENU : 0));
  uih->playstring = NULL;
  if (errstring != NULL)
    {
      uih_error (uih, errstring);
    }
}
