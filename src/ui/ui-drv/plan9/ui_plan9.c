/* 
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright (C) 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <config.h>
#ifdef PLAN9_DRIVER
#include <u.h>
#include <libc.h>
#ifdef _plan9v2_
#include <libg.h>
#else
#include <draw.h>
#include <event.h>
#include <cursor.h>
#endif
#include "zoom.h"
#include "ui.h"

#ifdef _plan9v2_
static Bitmap *mybitmap;
#else
static Image *mybitmap;
#endif
static char *buffers[2];
static Rectangle rect, rect1;
static int width, height;
static int current = 0;
static int nopalette;
static int ldepth;

static RGB cmap[256];
static int ncolors = 255;
static int mousex, mousey, mousebuttons;

#ifdef _plan9v2_
static void
plan9_setrange (ui_palette palette, int start, int end)
{
  int i;
  if (nopalette)
    {
      rdcolmap (&screen, cmap);
      for (i = start; i < end; i++)
	{
	  palette[i - start][0] = cmap[i].red / 256UL / 256UL / 256UL;
	  palette[i - start][1] = cmap[i].green / 256UL / 256UL / 256UL;
	  palette[i - start][2] = cmap[i].blue / 256UL / 256UL / 256UL;
	}
    }
  else
    {
      cmap[255].red = 0;
      cmap[255].green = 0;
      cmap[255].blue = 0;
      cmap[0].red = 0xffffffffUL;
      cmap[0].green = 0xffffffffUL;
      cmap[0].blue = 0xffffffffUL;
      for (i = start; i < end; i++)
	{
	  cmap[i].red =
	    (unsigned long) palette[i - start][0] * 256UL * 256UL * 256UL;
	  cmap[i].green =
	    (unsigned long) palette[i - start][1] * 256UL * 256UL * 256UL;
	  cmap[i].blue =
	    (unsigned long) palette[i - start][2] * 256UL * 256UL * 256UL;
	}
      wrcolmap (&screen, cmap);
    }
}
#else
static void
plan9_setrange (ui_palette palette, int start, int end)
{
  int i;
  readcolmap (display, cmap);
  for (i = start; i < end; i++) {
	/* old 8 bit only palette
	  palette[i - start][0] = cmap[i].red / 256UL / 256UL / 256UL;
	  palette[i - start][1] = cmap[i].green / 256UL / 256UL / 256UL;
	  palette[i - start][2] = cmap[i].blue / 256UL / 256UL / 256UL;
	*/
	  palette[i - start][0] = ((cmap2rgb(i) >> 16) & 0xff);
 	  palette[i - start][1] = ((cmap2rgb(i) >> 8) & 0xff);
 	  palette[i - start][2] = ((cmap2rgb(i) >> 8) & 0xff) ;

  }
}
#endif

static void
plan9_print (int x, int y, CONST char *text)
{
  Point p = { rect.min.x + x, rect.min.y + y };
#ifdef _plan9v2_
  string (&screen, p, font, text, S);
#else
  string (screen, p, display->black,ZP, display->defaultfont, text);
#endif
}
static void
 plan9_flush (void)
{
#ifdef _plan9v2_
   bflush ();
#else
  flushimage(display,1);
#endif
}
static void
plan9_getmouse (int *x, int *y, int *buttons)
{
  *x = mousex;
  *y = mousey;
  *buttons = mousebuttons;
}

static void
plan9_display (void)
{
#ifdef _plan9v2_
  wrbitmap (mybitmap, 0, height, (unsigned char *) buffers[current]);
  bitblt (&screen, rect.min, mybitmap, rect1, S);
#else
  loadimage(mybitmap, rect1,  (unsigned char *) buffers[current],width * (height + 1));
  draw(screen, screen->r, mybitmap, nil, ZP);
#endif
}

static void
plan9_flip_buffers (void)
{
  current ^= 1;
}
#ifndef _plan9v2_
void
eresized(int new)
{	
	if(new && getwindow(display, Refnone) < 0) {
		fprint(2, "XaoS: can't reattach to window: %r\n");
		exits("resized");
	}
	ui_resize();
}
#endif


void
ereshaped (Rectangle rect1)
{
  ui_resize ();
}
static void
plan9_processevent (int wait, int *mx, int *my, int *b, int *k)
{
  static int keys;
  Event E;
  while (wait || ecanread (Emouse | Ekeyboard))
    {
      wait = 0;
      switch (event (&E))
	{
	case Emouse:
	  mousex = E.mouse.xy.x - rect.min.x;
	  mousey = E.mouse.xy.y - rect.min.y;
	  mousebuttons = 0;
	  if (E.mouse.buttons & 1)
	    mousebuttons = BUTTON1;
	  if (E.mouse.buttons & 2)
	    mousebuttons |= BUTTON2;
	  if (E.mouse.buttons & 4)
	    mousebuttons |= BUTTON3;
	  break;
	case Ekeyboard:
	  {
	    if (E.kbdc == '[')
	      keys ^= 1;
	    if (E.kbdc == ']')
	      keys ^= 2;
	    if (E.kbdc == '\'')
	      keys ^= 4;
	    if (E.kbdc == ';')
	      keys ^= 8;
	    ui_key (tolower (E.kbdc));
	  }
	  break;
	}

    }

  *mx = mousex;
  *my = mousey;
  *b = mousebuttons;
  *k = keys;
}
static void
plan9_getsize (int *w, int *h)
{
#ifdef _plan9v2_
  bscreenrect (&rect);
#else
	rect = screen->r;
#endif
  width = rect.max.x - rect.min.x;
  height = rect.max.y - rect.min.y;
  rect1.min.x = 0;
  rect1.min.y = 0;
  rect1.max.x = width;
  rect1.max.y = height;
  *w = width;
  *h = height;
}
static int
plan9_allocbuffers (char **b1, char **b2)
{
  int w = width;
#ifdef _plan9v2_
  if (screen.ldepth == 0)
      w = (w + 7) / 8;
  mybitmap = balloc (rect1, ldepth);
#else
    mybitmap = allocimage(display,rect1,CMAP8,1,DCyan); 
#endif
  if (screen.ldepth == 0)
    w = (w + 7) / 8;
  current = 0;
  *b1 = buffers[0] = (char *) malloc (w * (height + 1));
  *b2 = buffers[1] = (char *) malloc (w * (height + 1));
  return (w);
}
static void
plan9_freebuffers (char *b1, char *b2)
{
  free (buffers[0]);
  free (buffers[1]);
#ifdef _plan9v2_
  bfree (mybitmap);
#else
  freeimage (mybitmap);
#endif
}
struct ui_driver plan9_driver;

#ifdef _plan9v2_
static int
plan9_init (void)
{
  binit (NULL, NULL, "XaoS");
  einit (Ekeyboard | Emouse);
  ldepth = screen.ldepth;
  switch (screen.ldepth)
    {
    case 0:
      plan9_driver.imagetype = UI_MIBITMAP;
      break;
    case 1:
      plan9_driver.flags |= UPDATE_AFTER_PALETTE;
      plan9_driver.imagetype = UI_FIXEDCOLOR;
      plan9_driver.palettestart = 0;
      plan9_driver.paletteend = 4;
      plan9_driver.maxentries = 4;
      ldepth = 3;
      break;
    case 2:
      plan9_driver.flags |= UPDATE_AFTER_PALETTE;
      plan9_driver.imagetype = UI_FIXEDCOLOR;
      plan9_driver.palettestart = 0;
      plan9_driver.paletteend = 16;
      plan9_driver.maxentries = 16;
      ldepth = 3;
      break;
    case 3:
      if (nopalette)
	{
	  plan9_driver.flags |= UPDATE_AFTER_PALETTE;
	  plan9_driver.imagetype = UI_FIXEDCOLOR;
	  plan9_driver.palettestart = 0;
	  plan9_driver.paletteend = 256;
	  plan9_driver.maxentries = 256;
	}
      else
	{
	  plan9_driver.flags |= PALETTE_ROTATION | ROTATE_INSIDE_CALCULATION;
	}
      break;
    default:
      printf ("Unsupported bitmap depth %i. Please contact author!\n",
	      screen.ldepth);
      return 0;
    }
  return 1;
}
#else
static int
  plan9_init (void)
{
  initdraw (nil, nil, "XaoS");
  einit (Ekeyboard | Emouse);
  ldepth = 3;

  plan9_driver.flags |= UPDATE_AFTER_PALETTE;
  plan9_driver.imagetype = UI_FIXEDCOLOR;
  plan9_driver.palettestart = 0;
  plan9_driver.paletteend = 256;
  plan9_driver.maxentries = 256;

  return 1;
}
#endif
static void
plan9_uninit ()
{
#ifdef _plan9v2_
  bexit ();
#else
  ;
#endif
}

static CONST struct params params[] = {
  {"", P_HELP, NULL, "plan9 driver options:"},
  {"-nopalette", P_SWITCH, &nopalette,
   "Disable palette allocating. Use ugly looking rgbpixel instead"},
  {NULL, 0, NULL, NULL}
};

struct ui_driver plan9_driver = {
  "plan9",
  plan9_init,
  plan9_getsize,
  plan9_processevent,
  plan9_getmouse,
  plan9_uninit,
  NULL,
  plan9_setrange,
  plan9_print,
  plan9_display,
  plan9_allocbuffers,
  plan9_freebuffers,
  plan9_flip_buffers,
  NULL,
  plan9_flush,
  8,
  14,
  params,
  RESOLUTION,
  0.0, 0.0,
  800, 600,
  UI_C256,
  1, 255, 256 - 2
};

#endif
