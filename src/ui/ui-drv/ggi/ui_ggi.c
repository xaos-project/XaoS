#include "aconfig.h"
#ifdef GGI_DRIVER
/*includes */
#include <sys/time.h>
#include <string.h>
#include <xerror.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <xmenu.h>
#include <ui.h>

#include <ggi/ggi.h>

#define GGIMASK   (emKey | emPointer)

#define xsize mode.visible.x
#define ysize mode.visible.y
static int mousex, mousey, buttons;
static int smousex, smousey;
static int keys;
static ggi_visual_t vis;
static char *defmode = NULL;
static ggi_mode mode;
static char *buffers[2];
static int current;
static int bpp;
static int vischanged=0;
static char *visname=NULL;
static char *oldvisname=NULL;
static int mousetype;
static int mousevisible;
#define MOUSESIZE 10
static char buffer[MOUSESIZE * MOUSESIZE * 4];
static void
ggi_hidemouse ()
{
  if (mousevisible)
    ggiPutBox (vis, smousex - MOUSESIZE / 2, smousey - MOUSESIZE / 2, MOUSESIZE, MOUSESIZE, buffer), mousevisible = 0;
}
static void
ggi_drawmouse ()
{
  if (!mousevisible && !mousetype)
    {
      int mx=mousex;
      int my=mousey;
      if(mx<=MOUSESIZE/2) mx=MOUSESIZE/2;
      if(my<=MOUSESIZE/2) my=MOUSESIZE/2;
      if(mx>=xsize-MOUSESIZE/2) mx=xsize-MOUSESIZE/2;
      if(my>=ysize-MOUSESIZE/2) my=ysize-MOUSESIZE/2;
      ggiGetBox (vis, mx - MOUSESIZE / 2, my - MOUSESIZE / 2, MOUSESIZE, MOUSESIZE, buffer);
      smousex = mx;
      smousey = my;
      ggiDrawLine (vis, mousex - MOUSESIZE / 2 + 1, mousey - MOUSESIZE / 2 + 1,
		   mousex + MOUSESIZE / 2 - 1, mousey + MOUSESIZE / 2 - 1);
      ggiDrawLine (vis, mousex + MOUSESIZE / 2 - 1, mousey - MOUSESIZE / 2 + 1,
		   mousex - MOUSESIZE / 2 + 1, mousey + MOUSESIZE / 2 - 1);
      mousevisible = 1;
    }
}

static void
ggi_setpalette (ui_palette pal, int start, int end)
{
  int i;
  ggi_color ggipal[256];
  for (i = 0; i < end - start; i++)
    {
      ggipal[i].r = pal[i][0] * 256;
      ggipal[i].g = pal[i][1] * 256;
      ggipal[i].b = pal[i][2] * 256;
    }
  ggiSetPalette (vis, start, end - start, ggipal);
}

static void
ggi_print (int x, int y, CONST char *text)
{
  ggi_hidemouse ();
  ggiPuts (vis, x, y, text);
  ggi_drawmouse ();
}

static void
ggi_flush ()
{
  ggiFlush (vis);
}

static void
ggi_disp ()
{
  ggiPutBox (vis, 0, 0, xsize, ysize, buffers[current]);
  mousevisible = 0;
  ggi_drawmouse ();
}

static void
ggi_flip_buffers ()
{
  current ^= 1;
}

void
ggi_free_buffers (char *b1, char *b2)
{
  free (buffers[0]);
  free (buffers[1]);
}

int
ggi_alloc_buffers (char **b1, char **b2)
{
  int size = xsize * ysize * bpp;
  current = 0;
  if (!bpp)
    size = (xsize + 7) / 8 * ysize;
  *b1 = buffers[0] = malloc (size);
  *b2 = buffers[1] = malloc (size);
  if (!bpp)
    return ((xsize + 7) / 8);
  return xsize * bpp;		/* bytes per scanline */
}

struct ui_driver ggi_driver;
static void
ggi_getsize (int *w, int *h)
{
  ggi_color red =
  {65535, 0, 0};
  ggi_color green =
  {0, 65535, 0};
  ggi_color blue =
  {0, 0, 65535};
  ggi_color white =
  {65535, 65535, 65535};

  keys = 0;
  buttons = 0;
  mousevisible = 0;

  if (vischanged) {
    ggiClose (vis);
    if ((vis = ggiOpen (visname, NULL)) == NULL)
      if ((vis = ggiOpen (oldvisname, NULL)) == NULL)
         x_fatalerror("Can not re-initialize GGI");
    vischanged=0;
  }
  if (defmode != NULL)
    ggiParseMode (defmode, &mode);
  else {
    ggiParseMode ("479x379", &mode);
    if (ggiCheckMode (vis, &mode)) {
       ggiParseMode ("320x200", &mode);
       if (ggiCheckMode (vis, &mode)) {
          ggiParseMode ("320x240", &mode);
          if (ggiCheckMode (vis, &mode)) {
             ggiParseMode ("640x480", &mode);
	  }
       }
    }
#if 0
    if (ggiCheckMode (vis, &mode)) {
      ggiParseMode ("479x379#479x379[32]", &mode);
      if (ggiCheckMode (vis, &mode)) {
        ggiParseMode ("479x379#479x379[16]", &mode);
         if (ggiCheckMode (vis, &mode)) {
            ggiParseMode ("479x379#479x379[24]", &mode);
            if (ggiCheckMode (vis, &mode)) {
               ggiParseMode ("479x379#479x379[8]", &mode);
	    }
	 }
      }
    }
#endif
  }

  if (ggiSetMode (vis, &mode))
    {
      if(defmode) ggiParseMode (defmode, &mode); else ggiParseMode ("479x379", &mode);
      if (ggiCheckMode (vis, &mode)) {
         ggiParseMode ("320x200", &mode);
         if (ggiCheckMode (vis, &mode)) {
            ggiParseMode ("320x240", &mode);
            if (ggiCheckMode (vis, &mode)) {
               ggiParseMode ("640x480", &mode);
	    }
         }
      }
      if (ggiSetMode (vis, &mode)) {
        ggiClose (vis);
        ggiExit ();
        x_fatalerror("Can not initialize graphics mode!");
      }
    }

  ggi_driver.width = mode.size.x / 10.0;
  ggi_driver.height = mode.size.y / 10.0;
  switch (mode.graphtype)
    {
    case GT_1BIT:
      bpp = 1;
      ggi_driver.imagetype = UI_LBITMAP;
      ggiSetGCForeground (vis, ggiMapColor (vis, &white));
      break;
    case GT_4BIT:
      ggiClose (vis);
      ggiExit ();
      x_fatalerror ("4bit modes are marked as obsolette thus unsupported by XaoS engine. Sorry :)\n");
    case GT_8BIT:
      bpp = 1;
      ggi_driver.imagetype = UI_C256;
      ggiSetGCForeground (vis, 1);
      break;
    case GT_15BIT:
      bpp = 2;
      ggi_driver.imagetype = UI_TRUECOLOR16;
      ggiSetGCForeground (vis, ggiMapColor (vis, &white));
      break;
    case GT_16BIT:
      bpp = 2;
      ggi_driver.imagetype = UI_TRUECOLOR16;
      ggiSetGCForeground (vis, ggiMapColor (vis, &white));
      break;
    case GT_24BIT:
      bpp = 3;
      ggi_driver.imagetype = UI_TRUECOLOR16;
      ggiSetGCForeground (vis, ggiMapColor (vis, &white));
      break;
    case GT_32BIT:
      bpp = 4;
      ggi_driver.imagetype = UI_TRUECOLOR16;
      ggiSetGCForeground (vis, ggiMapColor (vis, &white));
      break;
    default:
      ggiClose (vis);
      x_fatalerror ("GGI driver: unknown type!\n");
    }
  ggi_driver.rmask = ggiMapColor (vis, &red);
  ggi_driver.gmask = ggiMapColor (vis, &green);
  ggi_driver.bmask = ggiMapColor (vis, &blue);
  *w = xsize;
  *h = ysize;
  current = 0;
}

static void
ggi_processevents (int wait, int *mx, int *my, int *mb, int *k)
{
  ggi_event ev;
  struct timeval ti =
  {0, 0};

  while (wait || ggiEventPoll (vis, GGIMASK, &ti))
    {
      int type;
      type = ggiEventRead (vis, &ev, GGIMASK);
      wait = 0;
      switch (ev.any.type)
	{
	case evKeyRelease:
	  switch (ev.key.sym)
	    {
	    case GIIK_Up:
	      keys &= ~4;
	      break;
	    case GIIK_Down:
	      keys &= ~8;
	      break;
	    case GIIK_Left:
	      keys &= ~1;
	      break;
	    case GIIK_Right:
	      keys &= ~2;
	      break;
	    }
	  break;
	case evKeyPress:
	case evKeyRepeat:
#define KEY_ESC         0xf01b
	  switch (ev.key.sym)
	    {
	    case GIIK_Up:
	      ui_key (UIKEY_UP);
	      keys |= 4;
	      break;
	    case GIIK_Down:
	      ui_key (UIKEY_DOWN);
	      keys |= 8;
	      break;
	    case GIIK_Left:
	      ui_key (UIKEY_LEFT);
	      keys |= 1;
	      break;
	    case GIIK_Right:
	      ui_key (UIKEY_RIGHT);
	      keys |= 2;
	      break;
	    case GIIK_PageUp:
	      ui_key (UIKEY_PGUP);
	      break;
	    case GIIK_PageDown:
	      ui_key (UIKEY_PGDOWN);
	      break;
	    /*case U (KEY_ESC):*/
	    case GIIK_Break:
	    case GIIUC_Escape:
	      ui_key (UIKEY_ESC);
	      break;
#ifdef GIIK_Delete
	    case GIIK_Delete:
#endif
#ifdef GIIUC_Delete
	    case GIIUC_Delete:
#endif
	    case GIIUC_BackSpace:
	      ui_key (UIKEY_BACKSPACE);
	      break;
	    case GIIK_Enter:
	      ui_key ('\n');
	      break;
	    default:
	      ui_key (ev.key.sym & 255);
	    }
	  break;
	case evPtrButtonPress:
	  if (ev.pbutton.button & 1)
	    buttons |= BUTTON1;
	  if (ev.pbutton.button & 4)
	    buttons |= BUTTON2;
	  if (ev.pbutton.button & 2)
	    buttons |= BUTTON3;
	  break;
	case evPtrButtonRelease:
	  if (ev.pbutton.button & 1)
	    buttons &= ~BUTTON1;
	  if (ev.pbutton.button & 4)
	    buttons &= ~BUTTON2;
	  if (ev.pbutton.button & 2)
	    buttons &= ~BUTTON3;
	  break;
	case evPtrAbsolute:
	  ggi_hidemouse ();
	  mousex = ev.pmove.x;
	  mousey = ev.pmove.y;
	  ggi_drawmouse ();
	  break;
	case evPtrRelative:
	  ggi_hidemouse ();
	  mousex += ev.pmove.x;
	  mousey += ev.pmove.y;
	  if(mousex<0) mousex=0;
	  if(mousey<0) mousey=0;
	  if(mousex>xsize) mousex=xsize;
	  if(mousey>ysize) mousey=ysize;
	  ggi_drawmouse ();
	  break;
	}
    }
  *mx = mousex;
  *my = mousey;
  *mb = buttons;
  *k = keys;
}

static CONST char * CONST names[] =
{"1",
 "8",
 "15",
 "16",
 "24",
 "32",
 NULL};
static CONST menudialog uih_resizedialog[] =
{
  DIALOGINT ("X:", 0),
  DIALOGINT ("Y:", 0),
  DIALOGCHOICE ("Depth", names, 0),
  DIALOGSTR ("Display", ""),
  {NULL}
};
static menudialog *
ggi_resizedialog (struct uih_context *c)
{
  uih_resizedialog[0].defint = xsize;
  uih_resizedialog[1].defint = ysize;
  switch (mode.graphtype)
    {
    case GT_1BIT:
      uih_resizedialog[2].defint = 0;
      break;
    case GT_4BIT:
    case GT_8BIT:
      uih_resizedialog[2].defint = 1;
      break;
    case GT_15BIT:
      uih_resizedialog[2].defint = 2;
      break;
    case GT_16BIT:
      uih_resizedialog[2].defint = 3;
      break;
    case GT_24BIT:
      uih_resizedialog[2].defint = 4;
      break;
    case GT_32BIT:
      uih_resizedialog[2].defint = 5;
      break;
    default:
      uih_resizedialog[2].defint = 6;
    }
  uih_resizedialog[3].defstr = visname;
  return (uih_resizedialog);
}
static void
ggi_resize (struct uih_context *c, dialogparam * p)
{
  static char s[100];
  sprintf (s, "%ix%i#%ix%i[%s]", p[0].dint, p[1].dint, p[0].dint, p[1].dint, names[p[2].dint]);
  defmode = s;
  /*printf ("%s\n", s);*/
  if (strcmp (visname, p[3].dstring))
    {
      if(oldvisname) free (oldvisname);
      oldvisname=visname;
      visname=p[3].dstring;
      vischanged=1;
      visname = strdup (p[3].dstring);
    }
  ui_call_resize ();
}

static menuitem menuitems[] =
{
MENUCDIALOG ("ui", "=", "Resize", "resize", 0, ggi_resize, ggi_resizedialog),
};


static int
ggi_init ()
{
  if (ggiInit () != 0)
    return 0;


  if ((vis = ggiOpen (visname, NULL)) == NULL)
    return 0;
  if(visname!=NULL) visname=strdup(visname); else visname=strdup("");
  menu_add (menuitems, NITEMS (menuitems));

  return (1);
}

static void
ggi_uninitialise ()
{
  menu_delete (menuitems, NITEMS (menuitems));
  ggiClose (vis);
  ggiExit ();
}

static void
ggi_getmouse (int *x, int *y, int *b)
{
  *x = mousex;
  *y = mousey;
  *b = buttons;
}


static void
ggi_mousetype (int type)
{
  mousetype = type;
}

static CONST struct params params[] =
{
  {"", P_HELP, NULL, "GGI driver options:"},
  {"-defmode", P_STRING, &defmode, "Graphics defmode"},
  {"-ggidisplay", P_STRING, &visname, "Display"},
  {NULL, 0, NULL, NULL}
};

struct ui_driver ggi_driver =
{
  "GGI",
  ggi_init,
  ggi_getsize,
  ggi_processevents,
  ggi_getmouse,
  ggi_uninitialise,
  NULL,
  ggi_setpalette,
  ggi_print,
  ggi_disp,
  ggi_alloc_buffers,
  ggi_free_buffers,
  ggi_flip_buffers,
  ggi_mousetype,		/*This should be NULL */
  ggi_flush,			/*flush */
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

#endif
