#ifndef _plan9_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#endif
#include <config.h>
#include <misc-f.h>
#include "xshl.h"

#define XSHL_ENDLINE (65536)
#define XSHL_ENDPARAGRAPH (65536*2)
#define XSHL_NEWSTART (65536*4)
#define XSHL_SMALL (65536*8)
#define XSHL_SKIPBLANK (65536*16)
#define XSHL_COMMAND (65536*32)
#define XSHL_BLANK (65536*64)
#define XSHL_CLEARATLINE (65536*128)

#define XSHL_AUTOCLEAN (XSHL_ENDLINE|XSHL_ENDPARAGRAPH|XSHL_NEWSTART|XSHL_SKIPBLANK|XSHL_COMMAND|XSHL_BLANK)


#define ALL (~0)
CONST static struct tag
{
  CONST char *name;
  int andflagenable;
  int orflagenable;
  int andflagdisable;
  int orflagdisable;
}
tags[] =
{
  {
  "p", ALL, XSHL_SKIPBLANK | XSHL_ENDPARAGRAPH, ALL,
      XSHL_SKIPBLANK | XSHL_ENDPARAGRAPH}
  ,
  {
  "head", ALL,
      XSHL_SKIPBLANK | XSHL_BIG | XSHL_CENTERALIGN | XSHL_ENDPARAGRAPH,
      ~(XSHL_BIG | XSHL_CENTERALIGN),
      XSHL_SKIPBLANK | XSHL_ENDPARAGRAPH | XSHL_ENDLINE}
  ,
  {
  "emph", ALL, XSHL_EMPH, ~XSHL_EMPH, 0}
  ,
  {
  "br", ALL, XSHL_SKIPBLANK | XSHL_ENDLINE, ALL, 0}
  ,
  {
  "white", ~XSHL_COLORMASK, XSHL_WHITE, ~XSHL_WHITE, 0}
  ,
  {
  "red", ~XSHL_COLORMASK, XSHL_RED, ~XSHL_RED, 0}
  ,
  {
  "black", ~XSHL_COLORMASK, XSHL_BLACK, ~XSHL_BLACK, 0}
  ,
  {
  "right", ALL, XSHL_SKIPBLANK | XSHL_RIGHTALIGN | XSHL_ENDLINE,
      ~XSHL_RIGHTALIGN, XSHL_SKIPBLANK | XSHL_ENDLINE}
  ,
  {
  "center", ALL, XSHL_SKIPBLANK | XSHL_CENTERALIGN | XSHL_ENDLINE,
      ~XSHL_CENTERALIGN, XSHL_SKIPBLANK | XSHL_ENDLINE}
  ,
  {
  "tt", ALL, XSHL_MONOSPACE, ~XSHL_MONOSPACE, 0}
  ,
  {
  "dl", ALL, XSHL_SKIPBLANK | XSHL_ENDLINE, ~(XSHL_SMALL | XSHL_EMPH),
      XSHL_SKIPBLANK | XSHL_ENDLINE}
  ,
  {
  "dt", ~XSHL_SMALL, XSHL_SKIPBLANK | XSHL_ENDLINE | XSHL_EMPH, ALL, 0}
  ,
  {
  "dd", ~XSHL_EMPH, XSHL_SKIPBLANK | XSHL_NEWSTART | XSHL_SMALL, ALL, 0}
  ,
  {
  NULL}
};

#define MAXINPUT 256

#define SMALLSKIP (4*spacewidth)

struct boxitem
{
  char *text;
  struct xshl_context c;
  int xpos;
  int width;
  int height;
  struct boxitem *next;
};
static void
freebox (struct boxitem *box)
{
  if (box->c.linktext != NULL)
    free (box->c.linktext);
  free (box->text);
  free (box);
}
static struct xshl_item *
pack (struct boxitem *first, struct boxitem *last, int *collectedflags,
      int width)
{
  struct xshl_item *f = NULL;
  struct xshl_item *l = NULL;
  struct xshl_item *item;
  struct boxitem *curr = first, *ncurr;
  int end = 0;
  char text[256];
  int collected = 0;
  while (curr != last)
    {
      if (curr->text[0] == 0)
	{
	  ncurr = curr->next;
	  freebox (curr);
	  curr = ncurr;
	  continue;
	}
      strcpy (text, curr->text);
      item = (struct xshl_item *) malloc (sizeof (struct xshl_item));
      if (item == NULL)
	return NULL;
      item->x = curr->xpos;
      item->width = curr->width;
      item->c = curr->c;
      if (curr->c.linktext != NULL)
	curr->c.linktext = mystrdup (curr->c.linktext);
      collected |= item->c.flags;
      ncurr = curr->next;
      freebox (curr);
      curr = ncurr;
      while (curr != last &&
	     curr->xpos == item->x + item->width &&
	     (curr->c.flags & 0xffff) == (item->c.flags & 0xffff)
	     &&
	     ((curr->c.linktext == NULL && item->c.linktext == NULL) ||
	      (curr->c.linktext != NULL && item->c.linktext != NULL &&
	       !strcmp (curr->c.linktext, item->c.linktext))))

	{
	  strcat (text, curr->text);
	  item->width += curr->width;
	  collected |= item->c.flags;
	  ncurr = curr->next;
	  freebox (curr);
	  curr = ncurr;
	}
      item->text = mystrdup (text);
      item->next = NULL;
      if (l != NULL)
	l->next = item;
      else
	f = item;
      l = item;
      end = item->x + item->width;
    }
  *collectedflags = collected;
  if (collected & (XSHL_CENTERALIGN | XSHL_RIGHTALIGN))
    {
      if (collected & XSHL_RIGHTALIGN)
	end = width - end;
      else
	end = (width - end) / 2;
      item = f;
      while (item != NULL)
	{
	  item->x += end;
	  item = item->next;
	}
    }
  return (f);
}

#ifndef isspace
#define isspace(c)  ((c)==' '||(c)=='\t'||(c)=='\n')
#endif
static char xshllink[32];
static int flags;
static struct boxitem *
xshl_readbox (void *data, int (*get) (void *))
{
  char inputbuf[256];
  int c;
  int i;
  char command[16];
  char parameter[32];
  if (flags & XSHL_BLANK)
    {
      struct boxitem *box = (struct boxitem *) malloc (sizeof (*box));
      box->width = 0;
      box->next = NULL;
      box->height = flags & XSHL_BIG ? 0 : 1;
      box->text = mystrdup (" ");
      box->c.flags = flags | XSHL_CLEARATLINE;
      box->c.linktext = NULL;
      if (xshllink[0] != 0)
	box->c.linktext = mystrdup (xshllink);
      flags &= ~XSHL_AUTOCLEAN;
      flags |= XSHL_SKIPBLANK;
      return (box);
    }
  if (flags & XSHL_COMMAND)
    {
      int i = 0;
      flags &= ~XSHL_COMMAND;
      parameter[0] = 0;
      do
	{
	  c = command[i] = get (data);
	  if (i < 15)
	    i++;
	}
      while (c && c != '>' && !isspace (c));
      command[i - 1] = 0;
      if (c != '>' && c)
	{
	  do
	    {
	      c = get (data);
	    }
	  while (c && c != '>' && isspace (c));
	  if (c && c != '>')
	    {
	      i = 1;
	      parameter[0] = c;
	      do
		{
		  c = parameter[i] = get (data);
		  if (i < 31)
		    i++;
		}
	      while (c && c != '>' && !isspace (c));
	      parameter[i - 1] = 0;
	      if (isspace (c))
		{
		  do
		    {
		      c = get (data);
		    }
		  while (c && c != '>' && isspace (c));
		}
	    }
	}
      {
	int i;
	int disabled = 0;
	if (command[0] == '/')
	  disabled = 1;
	for (i = 0;
	     tags[i].name != NULL
	     && strcmp (tags[i].name, command + disabled); i++);
	if (tags[i].name != NULL)
	  {
	    if (disabled)
	      {
		flags &= tags[i].andflagdisable;
		flags |= tags[i].orflagdisable;
	      }
	    else
	      {
		flags &= tags[i].andflagenable;
		flags |= tags[i].orflagenable;
	      }
	  }
	else
	  {
	    if (!strcmp (command + disabled, "a")
		|| !strcmp (command + disabled, "tutor"))
	      {
		if (disabled)
		  xshllink[0] = 0;
		else
		  strcpy (xshllink, parameter);
	      }
	    else
	      while (c != '>')
		c = get (data);
	  }
      }
      if (c == '>')
	c = get (data);
    }
  else
    c = get (data);
  if (!c)
    {
      return NULL;
    }
  if (flags & XSHL_SKIPBLANK)
    {
      while (isspace (c))
	{
	  c = get (data);
	  if (!c)
	    {
	      return NULL;
	    }
	}
      flags &= ~XSHL_SKIPBLANK;
    }
  i = 0;
  inputbuf[i++] = c;
  while (c && c != '<' && !isspace (c))
    {
      c = get (data);
      inputbuf[i++] = c;
      if (i > 255)
	i = 255;
    }
  inputbuf[i - 1] = 0;
  if (i == 1 && !c)
    {
      return NULL;
    }
  if (i == 1 && isspace (c))
    {
      flags |= XSHL_BLANK;
      return xshl_readbox (data, get);
    }
  if (i == 1 && c == '<')
    {
      flags |= XSHL_COMMAND;
      return xshl_readbox (data, get);
    }
  {
    struct boxitem *box = (struct boxitem *) malloc (sizeof (*box));
    box->width = 0;
    box->next = NULL;
    box->height = flags & XSHL_BIG ? 0 : 1;
    box->text = mystrdup (inputbuf);
    box->c.flags = flags | XSHL_CLEARATLINE;
    box->c.linktext = NULL;
    if (xshllink[0] != 0)
      box->c.linktext = mystrdup (xshllink);
    flags &= ~XSHL_AUTOCLEAN;
    if (isspace (c))
      flags |= XSHL_BLANK;
    if (c == '<')
      flags |= XSHL_COMMAND;
    return (box);
  }
  /*We are at the first word */
}

struct xshl_line *
xshl_interpret (void *data, int (*get) (void *), int width,
		int getwidth (void *, int flags, CONST char *text), int
		startflags, int smallheight, int bigheight)
{
  int spacewidth = getwidth (data, 0, " ");
  int cflags;
  struct boxitem *first = NULL;
  struct boxitem *last = NULL;
  struct boxitem *item;
  struct boxitem *lastword = NULL;
  int maxlines = 200;
  struct xshl_line *lines =
    (struct xshl_line *) malloc (maxlines * sizeof (*lines));
  int nlines = 0;
  int ypos = 0;
  flags = startflags | XSHL_SKIPBLANK;
  xshllink[0] = 0;
  while ((item = xshl_readbox (data, get)) != NULL)
    {
      if (last == NULL)
	{
	  if (item->text[0] == ' ' && !item->text[1])
	    {
	      freebox (item);
	      continue;
	    }
	  lastword = NULL;
	  first = item, item->xpos =
	    item->c.flags & XSHL_SMALL ? SMALLSKIP : 0;
	}
      else
	{
	  if (item->text[0] == ' ' && !item->text[1])
	    {
	      lastword = item;
	    }
	  last->next = item, item->xpos = last->xpos + last->width;
	}
      last = item;
      item->width = getwidth (data, item->c.flags, item->text);
      if (item->c.flags & (XSHL_ENDLINE | XSHL_ENDPARAGRAPH)
	  || ((item->c.flags & XSHL_NEWSTART)
	      && item->xpos + spacewidth > SMALLSKIP))
	{
	  if (nlines > maxlines - 1)
	    maxlines *= 2, lines =
	      (struct xshl_line *) realloc (lines,
					    (maxlines) * sizeof (*lines));
	  if (first != NULL)
	    {
	      lines[nlines].first = pack (first, last, &cflags, width);
	      lines[nlines].y = ypos;
	      nlines++;
	      if (ypos & XSHL_BIG)
		ypos += bigheight;
	      else
		ypos += smallheight;
	    }
	  if (item->c.flags & (XSHL_ENDPARAGRAPH))
	    ypos += smallheight;
	  first = last;
	  item->xpos =
	    item->c.flags & item->c.flags & XSHL_SMALL ? SMALLSKIP : 0;
	  lastword = NULL;
	}
      else if (item->c.flags & XSHL_NEWSTART)
	item->xpos = SMALLSKIP;
      if (item->xpos + item->width > width)
	{
	  if (lastword == NULL)
	    {
	      item = first;
	      while (item != NULL)
		{
		  struct boxitem *c = item->next;
		  freebox (item);
		  item = c;
		}
	      first = last = NULL;
	    }
	  else
	    {
	      int xpos;
	      if (nlines > maxlines - 1)
		maxlines *= 2, lines =
		  (struct xshl_line *) realloc (lines,
						(maxlines) * sizeof (*lines));
	      lines[nlines].first = pack (first, lastword, &cflags, width);
	      lines[nlines].y = ypos;
	      nlines++;
	      if (ypos & XSHL_BIG)
		ypos += bigheight;
	      else
		ypos += smallheight;
	      if (lastword != NULL)
		first = lastword->next;
	      else
		first = NULL;
	      item = first;
	      if (item != NULL)
		{
		  xpos =
		    item->c.flags & item->
		    c.flags & XSHL_SMALL ? SMALLSKIP : 0;
		  while (item != NULL)
		    {
		      item->xpos = xpos;
		      xpos += item->width;
		      item = item->next;
		    }
		}
	      freebox (lastword);
	      lastword = NULL;
	      if (first == NULL)
		last = NULL;
	    }
	}
    }
  if (last != NULL)
    {
      lines[nlines].y = ypos;
      lines[nlines].first = pack (first, last->next, &cflags, width);
    }
  nlines++;
  lines[nlines].y = -1;
  lines =
    (struct xshl_line *) realloc (lines, (nlines + 2) * sizeof (*lines));
  return (lines);
}


int
xshl_textlen (void *data, int flags, CONST char *text)
{
  return ((int) strlen (text));
}

void
xshl_print (int startskip, struct xshl_line *lines)
{
  int i = 0;
  int y = 0;
  while (lines[i].y >= 0)
    {
      int xpos = -startskip;
      struct xshl_item *item;
      while (y < lines[i].y)
	{
	  putc ('\n', stdout), y++;
	}
      item = lines[i].first;
      while (item)
	{
	  while (xpos < item->x)
	    {
	      putc (' ', stdout), xpos++;
	    }
	  xpos += item->width;
	  item = item->next;
	}
      i++;
    }
}
void
xshl_free (struct xshl_line *lines)
{
  int i = 0;
  while (lines[i].y >= 0)
    {
      struct xshl_item *item, *nextitem;
      item = lines[i].first;
      while (item)
	{
	  nextitem = item->next;
	  if (item->c.linktext != NULL)
	    free (item->c.linktext);
	  free (item->text);
	  free (item);
	  item = nextitem;
	}
      i++;
    }
  free (lines);
}
