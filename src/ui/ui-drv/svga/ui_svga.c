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
#include "aconfig.h"
#ifdef SVGA_DRIVER
#include <stdio.h>
#include <malloc.h>
#include <vga.h>
#include <vgagl.h>
#include <vgamouse.h>
#include <vgakeyboard.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "keytables.h"
#include <ui.h>
#include <xthread.h>
#include <stdlib.h>

extern int euid, egid;
struct ui_driver svga_driver;
static char keys[128];
static int initialised;
static int mode = -1;
static vga_modeinfo *info;
static int width = 640, height = 480;
static int svga_currentbuff = 0;
static double bbytesperpixel;
static int colors;
static int defmode = -1;
static GraphicsContext screen;
static GraphicsContext buffers[2];
static int mousetype = NORMALMOUSE;
static int retrace;
static int grayscale;
void *font = NULL;

static int whitecolor;

static void
svga_mousetype (int m)
{
  mousetype = m;
}
static void
svga_print (int x, int y, CONST char *text)
{
  if (colors != 2)
    {
      gl_setcontext (&screen);
      gl_enableclipping ();
      if (y > height - svga_driver.textheight)
	return;
      gl_write (x, y / svga_driver.textheight * 8, text);
    }
}

static void
draw_mouse (int x, int y, int clear)
{
  static int oldx, oldy;

  if (mousetype != REPLAYMOUSE)
    {
      if (x < 3)
	x = 3;
      if (y < 3)
	y = 3;
      if (x > width - 5)
	x = width - 5;
      if (y > height - 5)
	y = height - 5;
      if ((oldx != x || oldy != y) && clear)
	{
	  gl_setcontext (&buffers[svga_currentbuff]);
	  gl_enableclipping ();
	  if (mode == G320x240x256
	      || mode == G320x400x256 || mode == G360x480x256)
	    gl_copyscreen (&screen);
	  else
	    gl_copyboxtocontext (oldx - 3, oldy - 3, 8, 8, &screen, oldx - 3,
				 oldy - 3);
	}
      oldx = x;
      oldy = y;
    }
}

static void
svga_display (void)
{
  gl_setcontext (&buffers[svga_currentbuff]);
  if (retrace)
    vga_waitretrace ();
  gl_copyscreen (&screen);
  draw_mouse (mouse_getx (), mouse_gety (), 0);
}

static void
svga_setpalette (ui_palette pal, int start, int end)
{
  int i;
  vga_waitretrace ();
  for (i = 0; i < end - start; i++)
    {
      vga_setpalette (i + start,
		      pal[i][0] >> 2, pal[i][1] >> 2, pal[i][2] >> 2);
    }
}
static void
svga_setgrayscale (void)
{
  int i;
  vga_waitretrace ();
  for (i = 0; i < 256; i++)
    {
      vga_setpalette (i, i >> 2, i >> 2, i >> 2);
    }
}


static void
svga_flip_buffers (void)
{
  svga_currentbuff ^= 1;
}

static void
handler (int scancode, int press)
{
  keys[scancode] = press;
  if (press)
    {
      switch (scancode)
	{
	case SCANCODE_CURSORBLOCKUP:
	  ui_key (UIKEY_UP);
	  break;
	case SCANCODE_CURSORBLOCKLEFT:
	  ui_key (UIKEY_LEFT);
	  break;
	case SCANCODE_CURSORBLOCKDOWN:
	  ui_key (UIKEY_DOWN);
	  break;
	case SCANCODE_CURSORBLOCKRIGHT:
	  ui_key (UIKEY_RIGHT);
	  break;
	case 1:
	  ui_key (UIKEY_ESC);
	  break;
	case SCANCODE_BACKSPACE:
	  ui_key (UIKEY_BACKSPACE);
	  break;
	case SCANCODE_HOME:
	  ui_key (UIKEY_HOME);
	  break;
	case SCANCODE_END:
	  ui_key (UIKEY_END);
	  break;
	case SCANCODE_PAGEUP:
	  ui_key (UIKEY_PGUP);
	  break;
	case SCANCODE_PAGEDOWN:
	  ui_key (UIKEY_PGDOWN);
	  break;
	default:
	  if (keys[SCANCODE_LEFTSHIFT] || keys[SCANCODE_RIGHTSHIFT])
	    ui_key (shift_table[scancode]);
	  else
	    ui_key (plain_table[scancode]);
	}
    }
}

#ifdef HAVE_PTHREAD_SIGHANDLER
extern void pthread_sighandler (int sig);
static void
shutdown (int n)
{
  printf ("Task cancelled\n");
  pthread_exit (NULL);
}
#endif

static void
init_mode (void)
{
  int i, high;

  while (mode == -1)
    {
      printf ("Choose one of the following video modes: \n");
      high = 0;
      for (i = 1; i <= GLASTMODE; i++)
	if (vga_hasmode (i))
	  {
	    char expl[100];
	    char *cols = NULL;
	    *expl = '\0';
	    info = vga_getmodeinfo (i);
	    if (info->colors != 256 && info->colors != 32768
		&& info->colors != 65536 && info->colors != 16777216)
	      continue;
	    if (i == G320x200x256)
	      strcpy (expl, "packed-pixel");
	    else if (i == G320x240x256
		     || i == G320x400x256 || i == G360x480x256)
	      strcpy (expl, "Mode X");
	    else
	      strcpy (expl, "packed-pixel, banked");
	    if (info->flags & IS_INTERLACED)
	      {
		if (*expl != '\0')
		  strcat (expl, ", ");
		strcat (expl, "interlaced");
	      }
	    if (info->flags & IS_DYNAMICMODE)
	      {
		if (*expl != '\0')
		  strcat (expl, ", ");
		strcat (expl, "dynamically loaded");
	      }
	    high = i;
	    if (defmode == -1)
	      {
		printf ("%5d: %dx%d, ", i, info->width, info->height);
		if (cols == NULL)
		  printf ("%d", info->colors);
		else
		  printf ("%s", cols);
		printf (" colors ");
		if (*expl != '\0')
		  printf ("(%s)", expl);
		printf ("\n");
	      }
	  }
      if (defmode == -1)
	{
	  printf ("Enter mode number (1-%d): ", high);
	  fflush (stdout);
	  scanf ("%d", &mode);
	  getchar ();
	  printf ("\n");
	}
      else
	mode = defmode, defmode = -1;
      if (mode < 1 || mode > GLASTMODE)
	{
	  printf ("Error: Mode number out of range \n");
	  mode = -1;
	  continue;
	}
      if (!vga_hasmode (mode))
	{
	  printf ("Error: Video mode not supported by driver\n");
	  mode = -1;
	  continue;
	}
      info = vga_getmodeinfo (mode);
      if (info->colors != 256 && info->colors != 32768
	  && info->colors != 65536 && info->colors != 16777216)
	{
	  printf
	    ("Error: Only 256,32768,65536 and 16777216 color modes supported\n");
	  mode = -1;
	  continue;
	}
    }

  colors = info->colors;
  bbytesperpixel = info->bytesperpixel;
  if (info->colors == 2)
    bbytesperpixel = 1.0 / 8;
  if (!bbytesperpixel)
    bbytesperpixel = 1;
  width = info->width;
  height = info->height;
#ifdef HAVE_PTHREAD_SIGHANDLER
  if (nthreads != 1)
    {
      printf ("You are starting threaded XaoS. Everything should work well,\n"
	      "except console switching, wich is broken because of SIGUSR\n"
	      "collision between linuxthreads and SVGAlib\n"
	      "\n"
	      "DONT EVEN THINK ABOUT TOUCHING ALT+FKEY COMBINATION!\n"
	      "\n"
	      "Result is blocked keyboard. You need to kill XaoS from the net then\n\n"
	      "Attempts for console switch (from other programs etc) may also cause\n"
	      "confusion in thread synchronization! or even kill some of XaoS processes\n"
	      "and cause lockup in main control loop. So do NOT try to start X or\n"
	      "some other bad thinks. Be curefull.\n\n"
	      "For your own safety switching of consoles will be blocked\n"
	      "Please press enter to continue/ctrl+c to abort\n");
      getc (stdin);
    }
#else
  if (nthreads != 1)
    {
      printf ("WARNING WARNING WARNING WARNING\n"
	      "You are trying to start SVGAlib XaoS with multiple threads. In case\n"
	      "you use linux-thread based thread library (like one in glibc) your system\n"
	      "is going to CRASH! So if you are not sure stop XaoS now. Otherwise press 'y'\n"
	      "to continue\n");
      if (getc (stdin) != 'y')
	exit (1);
    }
#endif
  vga_setmode (mode);
  seteuid (getuid ());		/* Don't need supervisor rights anymore. */
  setegid (getgid ());
  if (mode == G320x240x256 || mode == G320x400x256 || mode == G360x480x256)
    svga_driver.textheight = 8 * 4;
  else
    svga_driver.textheight = 8;
  gl_setcontextvga (mode);
#ifdef HAVE_PTHREAD_SIGHANDLER
  if (nthreads != 1)
    {
#warning With linux thread SVGAlib driver is just partially functional. See comment bellow this line.
      /* bad luck...bots SVGAlib and linuxthreads abuses SIGUSR1/2
       * for their own purposes. It is better to keep pthread code-
       * it disables console switching but don't crashes XaoS.
       * SIGUSR2 is not mapped, since linuxthreads don't export
       * handler. It is used for canceling threads wich is not
       * used by current XaoS thread implementation so everything
       * works (almost) well
       */
      struct sigaction sa;
      sa.sa_handler = pthread_sighandler;
      sigemptyset (&sa.sa_mask);
      sa.sa_flags = SA_RESTART;	/* does not matter for regular threads, but
				   better for the thread manager */
      sigaction (SIGUSR1, &sa, NULL);
      sa.sa_handler = shutdown;	/* Well shutdown isn't exactly what pthread implementation
				   does, but it works:) */
      sigaction (SIGUSR2, &sa, NULL);
      vga_lockvc ();
    }
#endif
  gl_getcontext (&screen);
  keyboard_init ();
  keyboard_translatekeys (0);
#ifdef VGA_KEYEVENT
  keyboard_seteventhandler (handler);
#endif
  gl_setpalettecolor (255, 63, 63, 63);
  gl_setpalettecolor (0, 0, 0, 0);
  if (colors == 256)
    {
      if (grayscale)
	svga_setgrayscale (), svga_driver.imagetype = UI_GRAYSCALE,
	  whitecolor = 255;
      else
	svga_driver.imagetype = UI_C256, whitecolor = 1;
    }
  if (colors == 65536)
    {
      svga_driver.imagetype = UI_TRUECOLOR16;
      svga_driver.rmask = 31 * 64 * 32;
      svga_driver.gmask = 63 * 32;
      svga_driver.bmask = 31;
      whitecolor = colors - 1;
    }
  if (colors == 32768)
    {
      svga_driver.imagetype = UI_TRUECOLOR16;
      svga_driver.rmask = 31 * 32 * 32;
      svga_driver.gmask = 31 * 32;
      svga_driver.bmask = 31;
      whitecolor = colors - 1;
    }
  if (colors == 16777216)
    {
      if (bbytesperpixel == 4)
	{
	  svga_driver.imagetype = UI_TRUECOLOR;
	  svga_driver.rmask = 0xff0000;
	  svga_driver.gmask = 0x00ff00;
	  svga_driver.bmask = 0x0000ff;
	  if (info->flags & RGB_MISORDERED)
	    {
	      svga_driver.rmask = 0x000000ff;
	      svga_driver.gmask = 0x0000ff00;
	      svga_driver.bmask = 0x00ff0000;
	    }
	  whitecolor = 0xffffffffU;
	}
      else
	{
	  svga_driver.rmask = 0xff0000;
	  svga_driver.gmask = 0x00ff00;
	  svga_driver.bmask = 0x0000ff;
	  svga_driver.imagetype = UI_TRUECOLOR24;
	  if (info->flags & RGB_MISORDERED)
	    {
	      svga_driver.rmask = 0x0000ff;
	      svga_driver.gmask = 0x00ff00;
	      svga_driver.bmask = 0xff0000;
	    }
	  whitecolor = 0xffffffU;
	}
    }
  if (font != NULL)
    free (font), font = NULL;
  font = (char *) malloc (256 * 8 * 8 * bbytesperpixel * 2);
  gl_expandfont (8, 8, whitecolor, gl_font8x8, font);
  gl_setfont (8, 8, font);
  draw_mouse (mouse_getx (), mouse_gety (), 0);
}

static void
svga_uninitialise (void)
{
  keyboard_close ();
  vga_setmode (TEXT);
  if (font != NULL)
    free (font), font = NULL;

}


static int
svga_init (void)
{
  int i;
  int fd;
  struct stat chkbuf;
  seteuid (euid);		/* We need supervisor rights to open mouse. */
  setegid (egid);
  if ((fd = open ("/dev/console", O_RDONLY)) < 0)
    {
      seteuid (getuid ());	/* Don't need supervisor rights anymore. */
      setegid (getgid ());
      return (0);
    }
  close (fd);
  fstat (2, &chkbuf);
  if (chkbuf.st_rdev >> 8 != 4 || (chkbuf.st_rdev & 0xff) >= 64)
    {
      seteuid (getuid ());	/* Don't need supervisor rights anymore. */
      setegid (getgid ());
      return (0);
    }
  if ((fd = open ("/dev/mem", O_RDWR)) < 0)
    {
      seteuid (getuid ());	/* Don't need supervisor rights anymore. */
      setegid (getgid ());
      return (0);
    }
  close (fd);
  if (!initialised)
    {
      vga_setmousesupport (1);
      vga_init ();
      initialised = 1;
    }
  for (i = 1; i <= GLASTMODE; i++)
    if (vga_hasmode (i))
      {
	info = vga_getmodeinfo (i);
	if (info->colors != 256 && info->colors != 32768
	    && info->colors != 65536 && info->colors != 16777216)
	  continue;
      }
  if (i == GLASTMODE + 1)
    {
      svga_uninitialise ();
      return 0;
    }
  seteuid (getuid ());		/* Don't need supervisor rights anymore. */
  setegid (getgid ());
  return (1);
}
static void
svga_get_size (int *x, int *y)
{
  int m = 0;
  if (mode != -1)
    {
      m = 1;
      svga_uninitialise ();
      mode = -1;
    }
  init_mode ();
  *x = width;
  *y = height;
}
static void
svga_getmouse (int *x, int *y, int *buttons)
{
  mouse_update ();
  *x = mouse_getx ();
  *y = mouse_gety ();
  *buttons = 0;
  if (mouse_getbutton () == MOUSE_LEFTBUTTON)
    *buttons |= BUTTON1;
  if (mouse_getbutton () == MOUSE_RIGHTBUTTON)
    *buttons |= BUTTON3;
  if (mouse_getbutton () & MOUSE_MIDDLEBUTTON)
    *buttons |= BUTTON2;
  draw_mouse (mouse_getx (), mouse_gety (), 1);
}
static void
svga_processevents (int wait, int *x, int *y, int *buttons, int *k)
{
  if (wait)
    {
#ifdef VGA_KEYEVENT
      struct timeval timeout;
      fd_set inputs;
      fd_set outputs;
      fd_set except;
      FD_ZERO (&inputs);
      FD_ZERO (&outputs);
      FD_ZERO (&except);
      timeout.tv_sec = 999999;
      timeout.tv_usec = 999999;
      mouse_update ();
      keyboard_update ();
      vga_waitevent (VGA_KEYEVENT | VGA_MOUSEEVENT, &inputs, &outputs,
		     &except, &timeout);
#else
#warning Your SVGALIB is old. Please update to latest one to get better XaoS
      mouse_update ();
      keyboard_update ();
#endif
    }
  else
    {
      mouse_update ();
      keyboard_update ();
    }
#ifndef VGA_KEYEVENT
  {
    int i;
    for (i = 0; i < 128; i++)
      if (keys[i] && !keyboard_keypressed (i))
	handler (i, 0);
      else
	{
	  if (!keys[i] && keyboard_keypressed (i))
	    handler (i, 1);
	}
  }
#endif
  *x = mouse_getx ();
  *y = mouse_gety ();
  *buttons = 0;
  if (mouse_getbutton () & MOUSE_LEFTBUTTON)
    *buttons |= BUTTON1;
  if (mouse_getbutton () & MOUSE_RIGHTBUTTON)
    *buttons |= BUTTON3;
  if (mouse_getbutton () & MOUSE_MIDDLEBUTTON)
    *buttons |= BUTTON2;
  *k = (keys[SCANCODE_CURSORBLOCKLEFT] != 0) +
    2 * (keys[SCANCODE_CURSORBLOCKRIGHT] != 0) +
    4 * (keys[SCANCODE_CURSORBLOCKUP] != 0) +
    8 * (keys[SCANCODE_CURSORBLOCKDOWN] != 0);
  draw_mouse (mouse_getx (), mouse_gety (), 1);
}
static int
svga_alloc_buffers (char **buffer1, char **buffer2)
{
  svga_currentbuff = 0;
  gl_setcontextvgavirtual (mode);
  gl_getcontext (&buffers[0]);
  gl_setcontextvgavirtual (mode);
  gl_getcontext (&buffers[1]);
  *buffer1 = buffers[0].vbuf;
  *buffer2 = buffers[1].vbuf;
  return (info->linewidth >=
	  width ? info->linewidth : width * bbytesperpixel);
}
static void
svga_free_buffers (char *buffer1, char *buffer2)
{
  gl_freecontext (&buffers[0]);
  gl_freecontext (&buffers[1]);
}
static CONST struct params params[] = {
  {"", P_HELP, NULL, "SVGA driver options:"},
  {"-mode", P_NUMBER, &defmode,
   "Select graphics mode(same number as in interactive menu)"},
  {"-retrace", P_SWITCH, &retrace, "Wait for retrace before displaying"},
  {"-grayscale", P_SWITCH, &grayscale,
   "Use grayscale mode instead of palette"},
  {NULL, 0, NULL, NULL}
};

struct ui_driver svga_driver = {
  "SVGAlib",
  svga_init,
  svga_get_size,
  svga_processevents,
  svga_getmouse,
  svga_uninitialise,
  NULL,
  svga_setpalette,
  svga_print,
  svga_display,
  svga_alloc_buffers,
  svga_free_buffers,
  svga_flip_buffers,
  svga_mousetype,
  NULL,
  8,
  8,
  params,
  FULLSCREEN | UPDATE_AFTER_RESIZE | PALETTE_ROTATION |
    ROTATE_INSIDE_CALCULATION | RESIZE_COMMAND,
  0.0, 0.0,
  0, 0,
  UI_C256,
  0, 256, 255
};

#endif
