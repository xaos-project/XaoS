#ifdef _plan9_
#include <u.h>
#include <libc.h>
#else
#include <stdlib.h>
#include <string.h>
#endif
#include <filter.h>
#include <fractal.h>
#include <timers.h>
#include <ui_helper.h>
#include <xerror.h>
#include <misc-f.h>
#include "grlib.h"
#define EXPIRETIME 4000000
static void
getpos (uih_context * c, int *x, int *y, int *w, int *h, void *data)
{
  int n = (int) data;
  if (c->messg.message[n] != NULL)
    {
      int he = xtextheight (c->font);
      *y = c->messg.messagestart + he * n;
      *h = he;
      *w = xtextwidth (c->font, c->messg.message[n]);
      *x = (c->image->width - *w) / 2;
    }
  else
    {
      *w = *h = *x = *y = 0;
    }
}
static void
draw (uih_context * c, void *data)
{
  int x, y, w;
  int n = (int) data;
  if (c->messg.message[n] != NULL)
    {
      int h = xtextheight (c->font);
      y = c->messg.messagestart + h * n;
      w = xtextwidth (c->font, c->messg.message[n]);
      x = (c->image->width - w) / 2;
      if (c->messg.messagetype[n])
	xprint (c->image, c->font, x, y, c->messg.message[n],
		(c->image->flags & AAIMAGE) ? BGCOLOR (c) : SELCOLOR (c),
		BGCOLOR (c), 0);
      else
	xprint (c->image, c->font, x, y, c->messg.message[n],
		(c->image->flags & AAIMAGE) ? BGCOLOR (c) : FGCOLOR (c),
		BGCOLOR (c), 0);
    }
}
void
uih_rmmessage (uih_context * c, int pid)
{
  int i;
  for (i = 0; i < NMESSAGES && c->messg.pid[i] != pid; i++);
  if (i == NMESSAGES)
    return;
  if (c->messg.message[i] == NULL)
    return;
  free (c->messg.message[i]);
  tl_remove_timer (c->messg.messagetimer[i]);
  tl_free_timer (c->messg.messagetimer[i]);
  c->messg.message[i] = NULL;
  for (; i < NMESSAGES - 1; i++)
    {
      c->messg.message[i] = c->messg.message[i + 1];
      c->messg.messagetimer[i] = c->messg.messagetimer[i + 1];
      c->messg.messagetype[i] = c->messg.messagetype[i + 1];
      c->messg.pid[i] = c->messg.pid[i + 1];
    }
  c->messg.message[NMESSAGES - 1] = NULL;
  c->display = 1;
}

void
uih_scrollup (uih_context * c)
{
  uih_rmmessage (c, c->messg.pid[0]);
}

void
uih_clearmessages (uih_context * c)
{
  while (c->messg.message[0] != NULL)
    uih_scrollup (c);
}

void
uih_initmessages (uih_context * c)
{
  int i;
  for (i = 0; i < NMESSAGES; i++)
    {
      c->messg.message[i] = NULL;
      c->messg.w[i] = uih_registerw (c, getpos, draw, (void *) i, 0);
    }
  c->messg.messagestart = 0;
}

void
uih_destroymessages (uih_context * c)
{
  int i;
  uih_clearmessages (c);
  for (i = 0; i < NMESSAGES; i++)
    uih_removew (c, c->messg.w[i]);
}
static int
uih_message1 (uih_context * c, CONST char *message, int type)
{
  static int pid;
  int i;
  for (i = 0; i < NMESSAGES && c->messg.message[i] != NULL; i++);
  if (i == NMESSAGES)
    uih_scrollup (c), i--;
  c->messg.message[i] = mystrdup (message);
  c->messg.messagetype[i] = type;
  c->messg.messagetimer[i] = tl_create_timer ();
  tl_reset_timer (c->messg.messagetimer[i]);
  tl_set_interval (c->messg.messagetimer[i], 1);;
  tl_slowdown_timer (c->messg.messagetimer[i], EXPIRETIME);;
  tl_set_handler (c->messg.messagetimer[i], (void (*)(void *)) uih_scrollup,
		  c);
  tl_add_timer (syncgroup, c->messg.messagetimer[i]);
  /*tl_remove_timer (c->messg.messagetimer[i]); */
  c->messg.pid[i] = ++pid;
  c->display = 1;
  return (pid);
}

int
uih_message (uih_context * c, CONST char *message)
{
  return (uih_message1 (c, message, 0));
}

int
uih_error (uih_context * c, CONST char *error)
{
  char str[256];
  sprintf (str, "Error: %s", error);
  c->errstring = error;
  return (uih_message1 (c, str, 1));
}

void
uih_printmessages (uih_context * c)
{
  int i;
  for (i = 0; i < NMESSAGES && c->messg.message[i] != NULL; i++)
    x_message (c->messg.message[i], stderr);
}
