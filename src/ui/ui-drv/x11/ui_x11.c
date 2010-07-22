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
#ifdef X11_DRIVER
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include "xlib.h"
#ifdef MITSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif
#include <fconfig.h>
#include <ui.h>
xlibparam xparams = { 0, 0, 0, 0, NULL, -1 };

static int allocated;
Cursor normal, xwait, replay;

struct ui_driver x11_driver;
static xdisplay *d;
static char *size;
static int Xsync;
static int busy;
static int sharedcolormap;
#if 0
static char *selection;
#endif
#ifdef MITSHM
static int Completion;
#endif
#ifdef AMIGA
#define XFlush(x) while(0)
#endif

static void x11_setpaletterange(ui_palette p, int s, int e)
{
    xsetpaletterange(d, p, s, e);
}

static int x11_set_color(int r, int g, int b, int init)
{
    if (init)
	xfree_colors(d);
    return (xalloc_color(d, r * 256, g * 256, b * 256, init));
}

static void x11_print(int x, int y, CONST char *text)
{
    xmoveto(d, x, y + x11_driver.textheight - 2);
    xouttext(d, text);

}

static void x11_flush(void)
{
    XFlush(d->display);
}

static int display;
static int flipped = 0;
static void x11_display(void)
{
    XFlush(d->display);
#ifdef MITSHM
    if (d->SharedMemFlag) {
	if (busy) {
	    display = 1;
	    return;
	}
	busy++;
	display = 0;
    }
#endif
    if (Xsync)
	XSync(d->display, 0);
    if (flipped)
	xflip_buffers(d), flipped = 0;
    draw_screen(d);
#ifdef MITSHM
    if (d->SharedMemFlag) {
	XSync(d->display, 0);
    }
#endif
}

static void x11_flip_buffers(void)
{
    flipped ^= 1;
}

static void x11_free_buffers(char *b1, char *b2)
{
    if (allocated) {
	XSync(d->display, 0);
	allocated = 0;
	free_image(d);
    }
}

static int x11_alloc_buffers(char **b1, char **b2, void **data)
{
    if (!allocated) {
	XSync(d->display, 0);
	allocated = 1;
	if (!alloc_image(d)) {
	    return (0);
	}
	xflip_buffers(d);
    }
    *b1 = d->vbuff;
    *b2 = d->back;
    flipped = 0;
#if 0
    if (d->SharedMemFlag) {
	x11_driver.flags |= UI_KEEP_BUFFER;
    }
#endif
    return (d->linewidth);
}

static void x11_getsize(int *w, int *h)
{
    XSync(d->display, 0);
    xupdate_size(d);
    *w = d->width;
    *h = d->height;
}

static void x11_processevents(int wait, int *mx, int *my, int *mb, int *k)
{
    static int mousex = 100, mousey = 0;
    static int iflag = 0;
    static unsigned int mousebuttons = 0;
    static int resized;
    XEvent ev;


    if (XPending(d->display) || busy >= 2 || wait) {
	do {
	    XNextEvent(d->display, &ev);
	    switch (ev.type) {
	    case ClientMessage:
		if ((int) ev.xclient.format == 32
		    && ev.xclient.data.l[0] == wmDeleteWindow)
		    ui_quit();
		break;
	    case ButtonRelease:
		mousex = ev.xbutton.x;
		mousey = ev.xbutton.y;
		switch (ev.xbutton.button) {
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
		mousex = ev.xbutton.x;
		mousey = ev.xbutton.y;
		if (!
		    (mousex < 0 || mousey < 0 || mousex > (int) d->width
		     || mousey > (int) d->height)) {
		    switch (ev.xbutton.button) {
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
		}
		break;
	    case MotionNotify:
		mousex = ev.xmotion.x;
		mousey = ev.xmotion.y;
		mousebuttons =
		    ev.xmotion.state & (BUTTON1 | BUTTON2 | BUTTON3);
		break;
	    case Expose:
		if (resized)
		    break;
		x11_display();
		break;
	    case ResizeRequest:
		XResizeWindow(d->display, d->window,
			      ev.xresizerequest.width,
			      ev.xresizerequest.height);
		XResizeWindow(d->display, d->parent_window,
			      ev.xresizerequest.width,
			      ev.xresizerequest.height);
		XSync(d->display, 0);
		resized = 2;
		ui_resize();
		resized = 0;
		break;

	    case ConfigureNotify:
		{
		    int oldw = d->width, oldh = d->height;
		    XSync(d->display, 0);
		    xupdate_size(d);
		    if ((int) d->width != oldw || (int) d->height != oldh) {
			resized = 2;
			ui_resize();
			resized = 0;
		    }
		}
		break;
	    case KeyRelease:
		{
		    switch (XLookupKeysym(&ev.xkey, 0)) {
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
		    switch (ksym = XLookupKeysym(&ev.xkey, 0)) {
		    case XK_Left:
			iflag |= 1;
			ui_key(UIKEY_LEFT);
			break;
		    case XK_Right:
			iflag |= 2;
			ui_key(UIKEY_RIGHT);
			break;
		    case XK_Up:
			iflag |= 4;
			ui_key(UIKEY_UP);
			break;
		    case XK_Down:
			iflag |= 8;
			ui_key(UIKEY_DOWN);
			break;
#ifdef XK_Page_Up
		    case XK_Page_Up:
			iflag |= 4;
			ui_key(UIKEY_PGUP);
			break;
		    case XK_Page_Down:
			iflag |= 8;
			ui_key(UIKEY_PGDOWN);
			break;
#endif
		    case XK_Escape:
			ui_key(UIKEY_ESC);
		    case XK_BackSpace:
			ui_key(UIKEY_BACKSPACE);
			break;	/* This statement was missing.
				   I'm not sure if this is needed
				   because new X drivers handle
				   UIKEY_BACKSPACE better or
				   double backspaces were problems
				   in earlier versions of XaoS, too.
				   -- Zoltan, 2004-10-30 */
		    default:
			{
			    CONST char *name;
			    char buff[256];
			    if (ksym == XK_F1)
				name = "h";
			    else {
				name = buff;
				buff[XLookupString
				     (&ev.xkey, buff, 256, &ksym, NULL)] =
				    0;
			    }
			    if (strlen(name) == 1) {
				if (ui_key(*name) == 2) {
				    return;
				}
			    }
			}
		    }
		}
		break;
	    default:
#ifdef MITSHM
		if (ev.xany.type == Completion) {
		    busy--;
		    if (display)
			x11_display();
		}
#endif
		break;
	    }
	}
	while (busy >= 2 ||	/*XEventsQueued (d->display, QueuedAlready) */
	       XPending(d->display));
    }
    *mx = mousex;
    *my = mousey;
    *mb = mousebuttons;
    *k = iflag;
}

/*static int defined; */
static void x11_cursor(int mode)
{
    /*if(defined)
       XUndefineCursor(d->display,d->window),defined=0; */
    switch (mode) {
    case NORMALMOUSE:
	XDefineCursor(d->display, d->window, normal);
	/*defined=1; */
	break;
    case WAITMOUSE:
	XDefineCursor(d->display, d->window, xwait);
	/*defined=1; */
	break;
    case REPLAYMOUSE:
	XDefineCursor(d->display, d->window, replay);
	/*defined=1; */
	break;
    }
    XFlush(d->display);
}

#if 0
static Atom atom;
void x11_copy()
{
    if (slection)
	free(selection), selection = NULL;
    selection = ui_getpos();
    atom = XInternAtom(d->display, "image/x-xaos.position", False);
    printf("%i\n", atom);
    XSetSelectionOwner(d->display, atom, d->window, CurrentTime);
}
#endif
static int x11_init(void)
{
    if (xparams.windowid != -1)
	xparams.rootwindow = xparams.fullscreen = 0;
    if (xparams.fullscreen || xparams.rootwindow)
	sharedcolormap = 1;	/*private colormap is broken in fullscreen
				   mode (at least at my X) */
    xparams.privatecolormap = !sharedcolormap;
    if (xparams.display == NULL) {	/*solaris stuff */
	xparams.display = getenv("DISPLAY");
    }
    if (size != NULL) {
	int x, y;
	sscanf(size, "%ix%i", &x, &y);
	if (x < 0)
	    x = XSIZE;
	if (y < 0)
	    y = YSIZE;
	d = xalloc_display("XaoS", x, y, &xparams);
    } else
	d = xalloc_display("XaoS", XSIZE, YSIZE, &xparams);
    if (d == NULL)
	return 0;
    /*normal=XCreateFontCursor(d->display,XC_arrow); */
    normal = XCreateFontCursor(d->display, XC_left_ptr);
    xwait = XCreateFontCursor(d->display, XC_watch);
    replay = XCreateFontCursor(d->display, XC_dot);
    if (d->truecolor || d->privatecolormap)
	x11_driver.flags &= ~RANDOM_PALETTE_SIZE;
    if (!alloc_image(d)) {
	xfree_display(d);
	return (0);
    }
    allocated = 1;
    switch (d->visual->class) {
    case StaticGray:
	if (d->depth == 1) {
	    if (BitmapBitOrder(d->display) == LSBFirst)
		if (WhitePixel(d->display, d->screen))
		    x11_driver.imagetype = UI_LBITMAP;
		else
		    x11_driver.imagetype = UI_LIBITMAP;
	    else if (WhitePixel(d->display, d->screen))
		x11_driver.imagetype = UI_MBITMAP;
	    else
		x11_driver.imagetype = UI_MIBITMAP;
	} else {
	    /*Warning! this is untested. I believe it works */
	    /*x11_driver.set_color = x11_set_color; */
	    x11_driver.palettestart = 0;
	    x11_driver.paletteend = 256;
	    x11_driver.maxentries = 256;
	    x11_driver.imagetype = UI_GRAYSCALE;
	}
	break;
    case StaticColor:
      smallcolor:
	{
	    int end = 256;
	    int start = 0;
	    int entries = d->visual->map_entries;
	    if (d->visual->class == TrueColor) {
		entries = (int) (d->image[0]->red_mask |
				 d->image[0]->green_mask |
				 d->image[0]->blue_mask);
	    }
	    x11_driver.imagetype = UI_FIXEDCOLOR;
	    if (end > entries)
		end = entries;
	    if (end < 64)
		start = 0;
	    x11_driver.set_range = x11_setpaletterange;
	    x11_driver.palettestart = start;
	    x11_driver.paletteend = end;
	    x11_driver.maxentries = end - start;
	}
	break;
    case PseudoColor:
    case GrayScale:
	if (d->privatecolormap) {
	    int end = 256;
	    int start = 16;
	    if (end > d->visual->map_entries)
		end = d->visual->map_entries;
	    if (end < 64)
		start = 0;
	    x11_driver.set_range = x11_setpaletterange;
	    x11_driver.palettestart = start;
	    x11_driver.paletteend = end;
	    x11_driver.maxentries = end - start;
	} else {
	    int end = 256;
	    if (end > d->visual->map_entries)
		end = d->visual->map_entries;
	    x11_driver.set_color = x11_set_color, x11_driver.flags |=
		RANDOM_PALETTE_SIZE;
	    x11_driver.palettestart = 0;
	    x11_driver.paletteend = end;
	    x11_driver.maxentries = end;
	}
	break;
    case TrueColor:
	x11_driver.rmask = d->image[0]->red_mask;
	x11_driver.gmask = d->image[0]->green_mask;
	x11_driver.bmask = d->image[0]->blue_mask;
	{
	    unsigned char c[4];
	    int order = MSBFirst;
	    *(unsigned short *) c = 0xff;
	    if (c[0] == (unsigned char) 0xff)
		order = LSBFirst;
	    if (order != d->image[0]->byte_order) {
		int shift = 32 - d->image[0]->bits_per_pixel;
#define SWAPE(c)  (((c&0xffU)<<24)|((c&0xff00U)<<8)|((c&0xff0000U)>>8)|((c&0xff000000U)>>24))
		x11_driver.rmask = SWAPE(x11_driver.rmask) >> shift;
		x11_driver.gmask = SWAPE(x11_driver.gmask) >> shift;
		x11_driver.bmask = SWAPE(x11_driver.bmask) >> shift;
	    }
	}
	switch (d->image[0]->bits_per_pixel) {
	case 8:
	    goto smallcolor;
	case 16:
	    x11_driver.imagetype = UI_TRUECOLOR16;
	    break;
	case 24:
	    x11_driver.imagetype = UI_TRUECOLOR24;
	    break;
	case 32:
	    x11_driver.imagetype = UI_TRUECOLOR;
	    break;
	default:
	    printf("Fatal error:unsupported bits per pixel!\n");
	}
    }
    x11_driver.maxwidth = XDisplayWidth(d->display, d->screen);
    x11_driver.maxheight = XDisplayHeight(d->display, d->screen);
    x11_driver.width =
	((double) ((unsigned int) XDisplayWidthMM(d->display, d->screen)))
	/ x11_driver.maxwidth / 10.0;
    x11_driver.height =
	((double) ((unsigned int) XDisplayHeightMM(d->display, d->screen)))
	/ x11_driver.maxheight / 10.0;
    x11_driver.textheight = xsetfont(d, "fixed");
    x11_driver.textwidth =
	d->font_struct->max_bounds.rbearing -
	d->font_struct->min_bounds.lbearing;
#ifdef MITSHM
    Completion = XShmGetEventBase(d->display) + ShmCompletion;
#endif
    if (d->privatecolormap) {
	x11_driver.flags |= PALETTE_ROTATION | ROTATE_INSIDE_CALCULATION;
    }
    return (1);
}

static void x11_uninitialise(void)
{
#if 0
    if (selection)
	free(selection), selection = NULL;
#endif
    xfree_colors(d);
    xfree_display(d);
}

static void x11_getmouse(int *x, int *y, int *b)
{
    int rootx, rooty;
    Window rootreturn, childreturn;
    XQueryPointer(d->display, d->window,
		  &rootreturn, &childreturn,
		  &rootx, &rooty, x, y, (unsigned int *) b);
}

static CONST struct params params[] = {
    {"", P_HELP, NULL, "X11 driver options:"},
    {"-display", P_STRING, &xparams.display, "Select display"},
    {"-size", P_STRING, &size, "Select size of window (WIDTHxHEIGHT)."},
    {"-sync", P_SWITCH, &Xsync,
     "Generate sync signals before looking for events. This\n\t\t\thelps on old and buggy HP-UX X servers."},
    {"-shared", P_SWITCH, &sharedcolormap,
     "Use shared colormap on pseudocolor display."},
    {"-usedefault", P_SWITCH, &xparams.usedefault,
     "Use default visual if autodetection causes troubles."},
    {"-nomitshm", P_SWITCH, &xparams.nomitshm,
     "Disable MITSHM extension."},
    {"-fullscreen", P_SWITCH, &xparams.fullscreen,
     "Enable fullscreen mode."},
    {"-windowid", P_NUMBER, &xparams.windowid, "Use selected window."},
    {"-window-id", P_NUMBER, &xparams.windowid, "Use selected window."},
    {"-root", P_SWITCH, &xparams.rootwindow, "Use root window."},
    {NULL, 0, NULL, NULL}
};

struct ui_driver x11_driver = {
    "X11",
    x11_init,
    x11_getsize,
    x11_processevents,
    x11_getmouse,
    x11_uninitialise,
    NULL,
    NULL,
    x11_print,
    x11_display,
    x11_alloc_buffers,
    x11_free_buffers,
    x11_flip_buffers,
    x11_cursor,
    x11_flush,
    8,
    8,
    params,
    RESOLUTION | PIXELSIZE | NOFLUSHDISPLAY /*| UPDATE_AFTER_RESIZE */ ,
    0.0, 0.0,
    0, 0,
    UI_C256,
    16, 254, 254 - 16
};

#endif
