#include "aconfig.h"
#ifdef DGA_DRIVER
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>

#include <ui.h>
#include <xmenu.h>
#include <xerror.h>
#include "ui.h"
#include "xio.h"
#include <cursor.h>
#include <misc-f.h>


#define MOUSEWIDTH 16
#define MOUSEHEIGHT 16


/* Mouse */
static int mouseX, mouseY, oldmouseX, oldmouseY;
static CONST char *mousepointer = mouse_pointer_data;
static char *storeddata = NULL;
static unsigned int mousebuttons = 0;
static CONST char *defmode="320x200";
static char *defdisplay=NULL;

static int first_time=1;
struct ui_driver DGA_driver;
static XF86VidModeModeInfo *mode;
static int current;
static Display *display;
static Window window;
static char *buffers[2];
static int linewidth;
static void DGA_uninitialise (void);
static struct
  {
    int screen;
    Visual *visual;
    int depth;
    unsigned char *addr;
    int grabbed_keybd;
    int grabbed_mouse;
    char *base_addr;
    int linewidth;
    int width;
    int height;
    int bank_size;
    int ram_size;
    Colormap cmap;
    unsigned int *pixels;
    XF86VidModeModeInfo orig_mode;
    XF86VidModeModeInfo *mode;
    int orig_viewport_x;
    int orig_viewport_y;
    int vidmode_changed;
    int modecount;
    XF86VidModeModeInfo **modes;
  }
xf86ctx =
{
  -1, NULL, -1, NULL, 0, 0, NULL,
    -1, -1, -1, -1, -1, 0, NULL, 
  {
  }
  ,0, 0, 0
};

static char *
store (char *data, int depth, int lpitch, int width, int height, int xpos, int ypos)
{
  int d = depth / 8;
  char *store = malloc (d * MOUSEWIDTH * MOUSEHEIGHT);
  int y;
  if (xpos + MOUSEWIDTH > width)
    xpos = width - MOUSEWIDTH;
  if (ypos + MOUSEHEIGHT > height)
    ypos = height - MOUSEHEIGHT;
  if (xpos < 0)
    xpos = 0;
  if (ypos < 0)
    ypos = 0;
  for (y = 0; y < MOUSEHEIGHT; y++)
    memcpy (store + d * MOUSEWIDTH * y, data + xpos * d + (ypos + y) * lpitch, MOUSEWIDTH * d);
  return store;
}
static void 
restore (char *data, char *store, int depth, int lpitch, int width, int height, int xpos, int ypos)
{
  int y;
  int d = depth / 8;
  if (xpos + MOUSEWIDTH > width)
    xpos = width - MOUSEWIDTH;
  if (ypos + MOUSEHEIGHT > height)
    ypos = height - MOUSEHEIGHT;
  if (xpos < 0)
    xpos = 0;
  if (ypos < 0)
    ypos = 0;
  for (y = 0; y < MOUSEHEIGHT; y++)
    memcpy (data + xpos * d + (ypos + y) * lpitch, store + d * MOUSEWIDTH * y, MOUSEWIDTH * d);
}

static void 
drawmouse (char *data, CONST char *mouse, int depth, int lpitch, int width, int height, int xpos, int ypos)
{
  int x, y, z, c;
  int d = depth / 8;
  for (y = 0; y < MOUSEWIDTH; y++)
    for (x = 0; x < MOUSEWIDTH; x++)
      if (mouse[x + MOUSEWIDTH * y] && x + xpos > 0 && (x + xpos) < width && y + ypos > 0 && y + ypos < height)
	{
	  c = mouse[x + MOUSEWIDTH * y] == 2 ? (d == 1 ? 1 : 255) : 0;
	  for (z = 0; z < d; z++)
	    data[z + d * (x + xpos) + (y + ypos) * lpitch] = c;
	}
}
static char **names;
static menudialog uih_resizedialog[] =
{
    DIALOGCHOICE ("Mode", NULL, 0),
    {NULL}
};

static menudialog *
DGA_resizedialog (struct uih_context *c)
{
  int i;
  for (i = 0; i < xf86ctx.modecount; i++)
    {
      if (xf86ctx.modes[i]->hdisplay == xf86ctx.width &&
	  xf86ctx.modes[i]->vdisplay == xf86ctx.height)
	break;
    }
  if (i==xf86ctx.modecount) i=0;
  uih_resizedialog[0].defint = i;
  uih_resizedialog[0].defstr = (char *)names;
  return uih_resizedialog;
}

static void
DGA_resize (struct uih_context *c, int p)
{
  static char s[256];
  sprintf (s, "%ix%i", xf86ctx.modes[p]->hdisplay, xf86ctx.modes[p]->vdisplay);
  defmode=s;
  ui_call_resize();
}

static CONST menuitem menuitems[] =
{
  MENUCDIALOG ("ui", "=", "Resize", "resize", 0, DGA_resize, DGA_resizedialog),
};


static int
DGA_getmodeinfo (XF86VidModeModeInfo * modeinfo)
{
  XF86VidModeModeLine modeline;
  int dotclock;
  Bool err;

  err = XF86VidModeGetModeLine (display, xf86ctx.screen, &dotclock, &modeline);

  modeinfo->dotclock = dotclock;
  modeinfo->hdisplay = modeline.hdisplay;
  modeinfo->hsyncstart = modeline.hsyncstart;
  modeinfo->hsyncend = modeline.hsyncend;
  modeinfo->htotal = modeline.htotal;
  modeinfo->vdisplay = modeline.vdisplay;
  modeinfo->vsyncstart = modeline.vsyncstart;
  modeinfo->vsyncend = modeline.vsyncend;
  modeinfo->vtotal = modeline.vtotal;
  modeinfo->flags = modeline.flags;
  modeinfo->privsize = modeline.privsize;
  modeinfo->private = modeline.private;

  return err;
}



static void
DGA_setpalette (ui_palette pal, int start, int end)
{
  int i;
  for(i=start;i<end;i++)
  {
    XColor color;
    color.pixel=i;
    color.flags = DoRed | DoBlue | DoGreen;
    color.red = pal[i-start][0]*256;
    color.green = pal[i-start][1]*256;
    color.blue = pal[i-start][2]*256;
    XStoreColor (display, xf86ctx.cmap, &color);
  }
  XF86DGAInstallColormap(display,xf86ctx.screen,xf86ctx.cmap);
}

static void
DGA_print (int x, int y, CONST char *text)
{
}

static void
DGA_display (void)
{
  int i;
  if (storeddata)
    free (storeddata), storeddata = NULL;
  storeddata = store (buffers[current], xf86ctx.depth, linewidth, xf86ctx.width, xf86ctx.height, mouseX, mouseY);
  drawmouse (buffers[current], mousepointer, xf86ctx.depth, linewidth, xf86ctx.width, xf86ctx.height, mouseX, mouseY);

  for (i = 0; i < xf86ctx.height; i++)
    {
      memcpy (xf86ctx.base_addr + xf86ctx.linewidth * i * xf86ctx.depth / 8, buffers[current] + linewidth * i, linewidth);
    }

  restore (buffers[current], storeddata, xf86ctx.depth, linewidth, xf86ctx.width, xf86ctx.height, mouseX, mouseY);
  oldmouseX = mouseX;
  oldmouseY = mouseY;
#if 0
  sleep (1);
  exit (1);
#endif
}

static void
DGA_flip_buffers (void)
{
  current^=1;
}

static void
DGA_free_buffers (char *b1, char *b2)
{
  free(buffers[0]);
  free(buffers[1]);
}

static int
DGA_alloc_buffers (char **b1, char **b2)
{
  buffers[0] = malloc (linewidth * xf86ctx.height * xf86ctx.depth / 8);
  current=0;
  if (buffers[0] == NULL)
    {
      x_error ("Couldn't alloc enough memory\n");
      return 0;
    }
  buffers[1] = malloc (linewidth * xf86ctx.height * xf86ctx.depth / 8);
  if (buffers[1] == NULL)
    {
      free(buffers[0]);
      x_error("Couldn't alloc enough memory\n");
      return 0;
    }
  *b1=buffers[0];
  *b2=buffers[1];
  XSync(display,0);
  return linewidth;			/* bytes per scanline */
}

static void 
DGA_vidmode_restoremode (Display * disp)
{
  XF86VidModeSwitchToMode (disp, xf86ctx.screen, &xf86ctx.orig_mode);
  XF86VidModeSetViewPort (disp, xf86ctx.screen, xf86ctx.orig_viewport_x, xf86ctx.orig_viewport_y);
  /* 'Mach64-hack': restores screen when screwed up */
  XF86VidModeSwitchMode (disp, xf86ctx.screen, -1);
  XF86VidModeSwitchMode (disp, xf86ctx.screen, 1);
/**************************************************/
  XSync (disp, False);
}

static int
DGA_vidmode_setup_mode_restore (void)
{
  Display *disp;
  int status;
  pid_t pid;

  if (!DGA_getmodeinfo (&xf86ctx.orig_mode))
    {
      x_error ("XF86VidModeGetModeLine failed\n");
      return 1;
    }


  if (!XF86VidModeGetViewPort (display, xf86ctx.screen,
			&xf86ctx.orig_viewport_x, &xf86ctx.orig_viewport_y))
    {
      x_error ("XF86VidModeGetViewPort failed\n");
      return 1;
    }

  pid = fork ();
  if (pid > 0)
    {
      waitpid (pid, &status, 0);
      disp = XOpenDisplay (defdisplay);
      DGA_vidmode_restoremode (disp);
      XCloseDisplay (disp);
      _exit (!WIFEXITED (status));
    }

  if (pid < 0)
    {
      perror ("fork");
      return 1;
    }

  return 0;
}
static XF86VidModeModeInfo *
DGA_findmode (void)
{
  int mindist = 65536 * 30000;
  int width = 640, height = 480;
  int i;
  XF86VidModeModeInfo *best = NULL;
  if (sscanf (defmode, "%ix%i", &width, &height) < 2)
    return NULL;
  for (i = 0; i < xf86ctx.modecount; i++)
    {
      int dist;
      dist = (width - xf86ctx.modes[i]->hdisplay) * (width - xf86ctx.modes[i]->hdisplay) +
	(height - xf86ctx.modes[i]->vdisplay) * (height - xf86ctx.modes[i]->vdisplay);
      if (dist < mindist)
	mindist = dist, best = xf86ctx.modes[i];
    }
  return best;
}


static void
DGA_getsize (int *w, int *h)
{
  window = RootWindow (display, xf86ctx.screen);
  DGA_driver.rmask = xf86ctx.visual->red_mask;
  DGA_driver.gmask = xf86ctx.visual->green_mask;
  DGA_driver.bmask = xf86ctx.visual->blue_mask;
  XF86DGADirectVideo (display, xf86ctx.screen, 0);
  if (xf86ctx.cmap)
    {
      XFreeColormap (display, xf86ctx.cmap);
      xf86ctx.cmap = 0;
    }
  switch (xf86ctx.depth)
    {
    case 8:
      xf86ctx.cmap = XCreateColormap (display, window, xf86ctx.visual, AllocAll);
      if (xf86ctx.visual->class != PseudoColor)
	{
	  DGA_uninitialise ();
	  x_fatalerror ("Only pseudocolor visual supported on 256 color cards\n");
	}
      DGA_driver.imagetype = UI_C256;
      break;
    case 15:
    case 16:
      DGA_driver.imagetype = UI_TRUECOLOR16;
      break;
    case 24:
      DGA_driver.imagetype = UI_TRUECOLOR24;
      break;
    case 32:
      DGA_driver.imagetype = UI_TRUECOLOR;
      break;
    default:
      {
	DGA_uninitialise ();
	x_fatalerror ("Unsupported visual!\n");
      }

    }

  xf86ctx.addr = (unsigned char *) xf86ctx.base_addr;

  mode = DGA_findmode ();


  if (mode)
    {
      /*xf86_dga_setup_graphics (bestmode); */
      xf86ctx.vidmode_changed = 1;
      xf86ctx.mode = mode;
    }
  else
    xf86ctx.mode = &xf86ctx.orig_mode;
  *w = xf86ctx.width = xf86ctx.mode->hdisplay;
  *h = xf86ctx.height = xf86ctx.mode->vdisplay;
  linewidth = ((xf86ctx.width * (xf86ctx.depth / 8)) + 63) & ~63;


  if (first_time)
    {
      if (DGA_vidmode_setup_mode_restore ())
	{
	  DGA_uninitialise ();
	  exit (1);
	}
    }
  if (!XF86VidModeSwitchToMode (display, xf86ctx.screen, xf86ctx.mode))
    {
      DGA_uninitialise ();
      x_fatalerror ("XF86VidModeSwitchToMode failed\n");
    }
  xf86ctx.vidmode_changed = 1;
  if (!XF86DGASetViewPort (display, xf86ctx.screen, 0, 0))
    {
      DGA_uninitialise ();
      x_fatalerror ("XF86DGASetViewPort failed\n");
    }
  if (!xf86ctx.grabbed_keybd)
    {
      if (XGrabKeyboard (display, window, True, GrabModeAsync, GrabModeAsync, CurrentTime))
	{
	  DGA_uninitialise ();
	  x_fatalerror ("XGrabKeyboard failed\n");
	}
      xf86ctx.grabbed_keybd = 1;
    }

  if (!xf86ctx.grabbed_mouse)
    {
      if (XGrabPointer (display, window, True,
		    PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
		     GrabModeAsync, GrabModeAsync, None, None, CurrentTime))
	{
	  DGA_uninitialise ();
	  x_fatalerror ("XGrabPointer failed\n");
	}
      xf86ctx.grabbed_mouse = 1;
    }

  if (first_time)
    {
      if (XF86DGAForkApp (xf86ctx.screen))
	{
	  perror ("fork");
	  DGA_uninitialise ();
	  exit (1);
	}
      first_time = 0;
    }

  if (!XF86DGADirectVideo (display, xf86ctx.screen,
	    XF86DGADirectGraphics | XF86DGADirectMouse | XF86DGADirectKeyb))
    {
      DGA_uninitialise ();
      x_fatalerror ("XF86DGADirectVideo failed\n");
    }

  if (!XF86DGASetViewPort (display, xf86ctx.screen, 0, 0))
    {
      DGA_uninitialise ();
      x_fatalerror ("XF86DGASetViewPort failed\n");
    }

  /*memset (xf86ctx.base_addr, 0, xf86ctx.bank_size); */

}


static int
DGA_init (void)
{
  int major, minor, event_base, error_base, flags;
  int i, count;
  XPixmapFormatValues *pixmaps;
  mousebuttons = 0;

  if (!(display = XOpenDisplay (defdisplay)))
    {
      return 0;
    }
  xf86ctx.screen = DefaultScreen (display);
  if (geteuid ())
    {
      x_error ("DGA requires root rights\n");
      XCloseDisplay (display);
      return 0;
    }
  if (!XF86DGAQueryVersion (display, &major, &minor))
    {
      x_error ("XF86DGAQueryVersion failed \n");
      XCloseDisplay (display);
      return 0;
    }
  if (!XF86DGAQueryExtension (display, &event_base, &error_base))
    {
      x_error ("XF86DGAQueryExtension failed\n");
      XCloseDisplay (display);
      return 0;
    }
  if (!XF86DGAQueryDirectVideo (display, xf86ctx.screen, &flags))
    {
      x_error ("XF86DGAQueryDirectVideo failed\n");
      XCloseDisplay (display);
      return 0;
    }
  if (!(flags & XF86DGADirectPresent))
    {
      x_error ("XF86DGADirectVideo support is not present\n");
      XCloseDisplay (display);
      return 0;
    }
  if (!XF86DGAGetVideo (display, xf86ctx.screen,
			&xf86ctx.base_addr, &xf86ctx.linewidth,
			&xf86ctx.bank_size, &xf86ctx.ram_size))
    {
      x_error ("XF86DGAGetVideo failed\n");
      XCloseDisplay (display);
      return 0;
    }
  xf86ctx.visual = DefaultVisual (display, xf86ctx.screen);
  if (xf86ctx.bank_size != (xf86ctx.ram_size * 1024))
    {
      x_error ("Banked graphics modes not supported\n");
      XCloseDisplay (display);
      return 0;
    }

  if (!XF86VidModeQueryVersion (display, &major, &minor))
    {
      x_error ("XF86VidModeQueryVersion failed\n");
      XCloseDisplay (display);
      return 0;
    }

  if (!XF86VidModeQueryExtension (display, &event_base, &error_base))
    {
      x_error ("XF86VidModeQueryExtension failed\n");
      XCloseDisplay (display);
      return 0;
    }


  /* dirty hack 24bpp can be either 24bpp packed or 32 bpp sparse */
  pixmaps = XListPixmapFormats (display, &count);
  if (!pixmaps)
    {
      x_error ("X11-Error: Couldn't list pixmap formats.\n"
	       "Probably out of memory.\n");
      XCloseDisplay (display);
      return 0;
    }
  for (i = 0; i < count; i++)
    {
      if (pixmaps[i].depth == DefaultDepth (display, xf86ctx.screen))
	{
	  xf86ctx.depth = pixmaps[i].bits_per_pixel;
	  break;
	}
    }
  free (pixmaps);
  if (i == count)
    {
      x_error ("Couldn't find a zpixmap with the defaultcolordepth\nThis should not happen!\n");
      XCloseDisplay (display);
      return 0;
    }
  DGA_getmodeinfo (&xf86ctx.orig_mode);

  if (!XF86VidModeGetAllModeLines (display, xf86ctx.screen,
				   &xf86ctx.modecount, &xf86ctx.modes))
    {
      x_error ("XF86VidModeGetAllModeLines failed\n");
      XCloseDisplay (display);
      return 0;
    }
  names=calloc(sizeof (*names), xf86ctx.modecount+1);
  for(i=0;i<xf86ctx.modecount;i++)
  {
    char c[256];
    sprintf(c,"%ix%i", xf86ctx.modes[i]->hdisplay, xf86ctx.modes[i]->vdisplay);
    names[i]=mystrdup(c);
  }

  menu_add (menuitems, NITEMS (menuitems));

  DGA_driver.width = ( XDisplayWidthMM (display, xf86ctx.screen)) / 10.0;
  DGA_driver.height = ( XDisplayHeightMM (display, xf86ctx.screen)) / 10.0;


  return ( /*1 for sucess 0 for fail */ 1);
}


static void
DGA_uninitialise ()
{
  int i;
  if (xf86ctx.cmap)
    {
      XFreeColormap (display, xf86ctx.cmap);
      xf86ctx.cmap = 0;
    }
  if (xf86ctx.pixels)
    {
      free (xf86ctx.pixels);
      xf86ctx.pixels = NULL;
    }
  if (xf86ctx.grabbed_mouse)
    {
      XUngrabPointer (display, CurrentTime);
      xf86ctx.grabbed_mouse = 0;
    }
  if (xf86ctx.grabbed_keybd)
    {
      XUngrabKeyboard (display, CurrentTime);
      xf86ctx.grabbed_keybd = 0;
    }
  XF86DGADirectVideo (display, xf86ctx.screen, 0);
  if (xf86ctx.vidmode_changed)
    {
      DGA_vidmode_restoremode (display);
      xf86ctx.vidmode_changed = 0;
    }
  menu_delete (menuitems, NITEMS (menuitems));
  for (i = 0; i < xf86ctx.modecount; i++)
    free (names[i]);
  free (names);
}


static void
DGA_updatemouse (int x, int y)
{
  mouseX=x;
  mouseY=y;
  if (storeddata)
    {
      restore (xf86ctx.base_addr, storeddata, xf86ctx.depth, xf86ctx.depth/8*xf86ctx.linewidth, xf86ctx.width, xf86ctx.height, oldmouseX, oldmouseY);
      free (storeddata);
    }
  storeddata = store (xf86ctx.base_addr, xf86ctx.depth, xf86ctx.depth/8*xf86ctx.linewidth, xf86ctx.width, xf86ctx.height, mouseX, mouseY);
  drawmouse (xf86ctx.base_addr, mousepointer, xf86ctx.depth, xf86ctx.depth/8*xf86ctx.linewidth, xf86ctx.width, xf86ctx.height, mouseX, mouseY);
  oldmouseX = mouseX;
  oldmouseY = mouseY;
}
static void
DGA_processevents (int wait, int *mx, int *my, int *mb, int *k)
{
  static int iflag = 0;
  XEvent ev;


  if (XPending (display) || wait)
    {
      do
	{
          int mousex = 0, mousey = 0;
	  XNextEvent (display, &ev);
	  switch (ev.type)
	    {
	    case ButtonRelease:
	      switch (ev.xbutton.button)
		{
		case 1:
		  mousebuttons &= ~BUTTON1;
		  break;
		case 2:
		  mousebuttons &= ~BUTTON2;
		  break;
		case 3:
		  mousebuttons &= ~BUTTON3;
		  break;
		}
	      break;
	    case ButtonPress:
		  switch (ev.xbutton.button)
		    {
		    case 1:
		      mousebuttons |= BUTTON1;
		      break;
		    case 2:
		      mousebuttons |= BUTTON2;
		      break;
		    case 3:
		      mousebuttons |= BUTTON3;
		      break;
		    }
	      break;
	    case MotionNotify:
	      mousex = ev.xmotion.x_root;
	      mousey = ev.xmotion.y_root;
	      break;
	    case KeyRelease:
	      {
		switch (XLookupKeysym (&ev.xkey, 0))
		  {
		  case XK_Left:
		    iflag &= ~1;
		    break;
		  case XK_Right:
		    iflag &= ~2;
		    break;
		  case XK_Up:
		    iflag &= ~4;
		    break;
		  case XK_Down:
		    iflag &= ~8;
		    break;
		  }
	      }
	      break;
	    case KeyPress:
	      {
		KeySym ksym;
		switch (ksym = XLookupKeysym (&ev.xkey, 0))
		  {
		  case XK_Left:
		    iflag |= 1;
		    ui_key (UIKEY_LEFT);
		    break;
		  case XK_Right:
		    iflag |= 2;
		    ui_key (UIKEY_RIGHT);
		    break;
		  case XK_Up:
		    iflag |= 4;
		    ui_key (UIKEY_UP);
		    break;
		  case XK_Down:
		    iflag |= 8;
		    ui_key (UIKEY_DOWN);
		    break;
#ifdef XK_Page_Up
		  case XK_Page_Up:
		    iflag |= 4;
		    ui_key (UIKEY_PGUP);
		    break;
		  case XK_Page_Down:
		    iflag |= 8;
		    ui_key (UIKEY_PGDOWN);
		    break;
#endif
		  case XK_Escape:
		    ui_key (UIKEY_ESC);
		  case XK_BackSpace:
		    ui_key (UIKEY_BACKSPACE);
		  default:
		    {
		      CONST char *name;
		      char buff[256];
		      if (ksym == XK_F1)
			name = "h";
		      else
			{
			  name = buff;
			  buff[XLookupString (&ev.xkey, buff, 256, &ksym, NULL)] = 0;
			}
		      if (strlen (name) == 1)
			{
			  if (ui_key (*name) == 2)
			    {
			      return;
			    }
			}
		    }
		  }
	      }
	      break;
	    }
	  if (mousex || mousey)
	    {
	      mousex += mouseX;
	      mousey += mouseY;
	      if (mousex < 0)
		mousex = 0;
	      if (mousey < 0)
		mousey = 0;
	      if (mousex > xf86ctx.width)
		mousex = xf86ctx.width;
	      if (mousey > xf86ctx.height)
		mousey = xf86ctx.height;
	      DGA_updatemouse (mousex, mousey);
	    }
	}
      while (XPending (display));
    }
  *mx = mouseX;
  *my = mouseY;
  *mb = mousebuttons;
  *k = iflag;
}
static void
DGA_getmouse (int *x, int *y, int *b)
{
  *x=mouseX;
  *y=mouseY;
  *b=mousebuttons;
}

static void
DGA_mousetype (int type)
{
  switch (type)
    {
    default:
    case 0:
      mousepointer = mouse_pointer_data;
      break;
    case 1:
      mousepointer = wait_pointer_data;
      break;
    case 2:
      mousepointer = replay_pointer_data;
      break;
    }
  DGA_updatemouse (mouseX, mouseY);
}

static CONST struct params params[] =
{
  {"", P_HELP, NULL,"DGA driver options:"},
  {"-display", P_STRING, &defdisplay, "Select display"},
  {"-defmode", P_STRING, &mode, "Select videomode nearest to specified mode"},
  {NULL, 0, NULL, NULL}
};

struct ui_driver DGA_driver =
{
  "DGA",
  DGA_init,
  DGA_getsize,
  DGA_processevents,
  DGA_getmouse,
  DGA_uninitialise,
  NULL,
  DGA_setpalette,
  DGA_print,
  DGA_display,
  DGA_alloc_buffers,
  DGA_free_buffers,
  DGA_flip_buffers,
  DGA_mousetype,		/*This should be NULL */
  NULL,				/*flush */
  8,				/*text width */
  8,				/*text height */
  params,
  FULLSCREEN | UPDATE_AFTER_RESIZE,	/*flags...see ui.h */
  0.0, 0.0,			/*width/height of screen in centimeters */
  0, 0,				/*resolution of screen for windowed systems */
  UI_C256,			/*Image type */
  0, 255, 255			/*start, end of palette and maximum allocatable */
				/*entries */
};

/* DONT FORGET TO ADD DOCUMENTATION ABOUT YOUR DRIVER INTO xaos.hlp FILE!*/

#endif


