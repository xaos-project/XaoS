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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <config.h>
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include "xlib.h"
#ifdef AMIGA
#define XFlush(x) while(0)
#endif

#define chkalloc(n) if (!n) fprintf(stderr, "out of memory\n"), exit(-1)

extern int prog_argc;
extern char **prog_argv;
Atom wmDeleteWindow;

int xupdate_size(xdisplay * d)
{
    int tmp;
    Window wtmp;
    unsigned int width = d->width, height = d->height;
    XSync(d->display, False);
    XGetGeometry(d->display, d->window, &wtmp, &tmp, &tmp, &d->width,
		 &d->height, (unsigned int *) &tmp, (unsigned int *) &tmp);
    if (d->width != width || d->height != height)
	return 1;
    return 0;
}

void xflip_buffers(xdisplay * d)
{
    d->back = d->vbuffs[d->current];
    d->current ^= 1;
    d->vbuff = d->vbuffs[d->current];
}


void draw_screen(xdisplay * d)
{
#ifdef MITSHM
    if (d->SharedMemFlag) {
	XShmPutImage(d->display, d->window, d->gc, d->image[d->current], 0,
		     0, 0, 0, d->bwidth, d->bheight, True);
    } else
#endif
	XPutImage(d->display, d->window, d->gc, d->image[d->current], 0, 0,
		  0, 0, d->bwidth, d->bheight);
    /*XFlush(d->display); *//*gives small rest to X but degrades perofrmance
       too much */
    d->screen_changed = 0;
}

#ifdef MITSHM
int alloc_shm_image(xdisplay * new)
{
    register char *ptr;
    int temp, size = 0, i;
    ptr = DisplayString(new->display);
    if (!ptr || (*ptr == ':') || !strncmp(ptr, "localhost:", 10) ||
	!strncmp(ptr, "unix:", 5) || !strncmp(ptr, "local:", 6)) {
	new->SharedMemOption =
	    XQueryExtension(new->display, "MIT-SHM", &temp, &temp, &temp);
    } else {
	new->SharedMemOption = False;
	return 0;
    }
    new->SharedMemFlag = False;

    if (new->SharedMemFlag) {
	XShmDetach(new->display, &new->xshminfo[0]);
	XShmDetach(new->display, &new->xshminfo[1]);
	new->image[0]->data = (char *) NULL;
	new->image[1]->data = (char *) NULL;
	shmdt(new->xshminfo[0].shmaddr);
	shmdt(new->xshminfo[1].shmaddr);
    }
    for (i = 0; i < 2; i++) {
	if (new->SharedMemOption) {
	    new->SharedMemFlag = False;
	    new->image[i] =
		XShmCreateImage(new->display, new->visual, new->depth,
				new->depth == 1 ? XYBitmap : ZPixmap, NULL,
				&new->xshminfo[i], new->width,
				new->height);
	    if (new->image[i]) {
		temp =
		    new->image[i]->bytes_per_line *
		    (new->image[i]->height + 150);
		new->linewidth = new->image[i]->bytes_per_line;
		if (temp > size)
		    size = temp;
		new->xshminfo[i].shmid =
		    shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
		if (new->xshminfo[i].shmid != -1) {
		    errno = 0;
		    new->xshminfo[i].shmaddr =
			(char *) shmat(new->xshminfo[i].shmid, 0, 0);
		    if (!errno) {
			new->image[i]->data = new->xshminfo[i].shmaddr;
			new->data[i] = new->vbuffs[i] =
			    (char *) new->image[i]->data;
			new->xshminfo[i].readOnly = True;

			new->SharedMemFlag =
			    XShmAttach(new->display, &new->xshminfo[i]);
			XSync(new->display, False);
			if (!new->SharedMemFlag) {
			    XDestroyImage(new->image[i]);
			    new->image[i] = (XImage *) NULL;
			    new->SharedMemFlag = 0;
			    return 0;
			}
		    }
		    /* Always Destroy Shared Memory Ident */
		    shmctl(new->xshminfo[i].shmid, IPC_RMID, 0);
		}
		if (!new->SharedMemFlag) {
		    XDestroyImage(new->image[i]);
		    new->image[i] = (XImage *) NULL;
		    new->SharedMemFlag = 0;
		    return 0;
		}
	    } else {
		new->SharedMemFlag = 0;
		return 0;
	    }
	} else {
	    new->SharedMemFlag = 0;
	    return 0;
	}
    }
    new->current = 0;
    xflip_buffers(new);
    return 1;
}

void free_shm_image(xdisplay * d)
{
    if (d->SharedMemFlag) {
	XDestroyImage(d->image[0]);
	XDestroyImage(d->image[1]);
	XShmDetach(d->display, &d->xshminfo[0]);
	XShmDetach(d->display, &d->xshminfo[1]);
	shmdt(d->xshminfo[0].shmaddr);
	shmdt(d->xshminfo[1].shmaddr);
    }
}

#endif

int alloc_image(xdisplay * d)
{
    int i;
    d->bwidth = d->width;
    d->bheight = d->height;
#ifdef MITSHM
    if (!d->params->nomitshm && d->depth != 1 && alloc_shm_image(d)) {
	return 1;
    }
#endif
    for (i = 0; i < 2; i++) {

	d->image[i] =
	    XCreateImage(d->display, d->visual, d->depth,
			 d->depth == 1 ? XYBitmap : ZPixmap, 0, NULL,
			 d->width, d->height, 32, 0);
	if (d->image[i] == NULL) {
	    printf("Out of memory for image..exiting\n");
	    exit(-1);
	}
	d->image[i]->data =
	    malloc(d->image[i]->bytes_per_line * d->height);
	if (d->image[i]->data == NULL) {
	    printf("Out of memory for image buffers..exiting\n");
	    exit(-1);
	}
	{
	    unsigned char c[4];
	    int byteexact = 0;
	    *(unsigned short *) c = 0xff;
	    if ((!(d->image[i]->red_mask & ~0xffU)
		 || !(d->image[i]->red_mask & ~0xff00U)
		 || !(d->image[i]->red_mask & ~0xff0000U)
		 || !(d->image[i]->red_mask & ~0xff000000U))
		&& (!(d->image[i]->green_mask & ~0xffU)
		    || !(d->image[i]->green_mask & ~0xff00U)
		    || !(d->image[i]->green_mask & ~0xff0000U)
		    || !(d->image[i]->green_mask & ~0xff000000U))
		&& (!(d->image[i]->blue_mask & ~0xffU)
		    || !(d->image[i]->blue_mask & ~0xff00U)
		    || !(d->image[i]->blue_mask & ~0xff0000U)
		    || !(d->image[i]->blue_mask & ~0xff000000U)))
		byteexact = 1;
	    if (!byteexact) {
		/*Make endianity correct */
		if (c[0] == (unsigned char) 0xff) {
		    if (d->image[i]->byte_order != LSBFirst) {
			d->image[i]->byte_order = LSBFirst;
			/*XInitImage(d->image[i]); */
		    }
		} else {
		    if (d->image[i]->byte_order != MSBFirst) {
			d->image[i]->byte_order = MSBFirst;
			/*XInitImage(d->image[i]); */
		    }
		}
	    }
	}
	d->data[i] = d->vbuffs[i] = (char *) d->image[i]->data;
	d->linewidth = d->image[i]->bytes_per_line;
    }
    xflip_buffers(d);
    return 1;
}

void free_image(xdisplay * d)
{
#ifdef MITSHM
    if (d->SharedMemFlag) {
	free_shm_image(d);
	return;
    }
#endif
    XDestroyImage(d->image[0]);
    XDestroyImage(d->image[1]);
}

#define MAX(x,y) ((x)>(y)?(x):(y))


xdisplay *xalloc_display(CONST char *s, int x, int y, xlibparam * params)
{
    xdisplay *new;
    Visual *defaultvisual;
    XVisualInfo vis;
    int found;
    int i;

    XClassHint classHint;
    XWMHints *hints;
    char **faked_argv;

    new = (xdisplay *) calloc(sizeof(xdisplay), 1);
    chkalloc(new);
    new->display = XOpenDisplay(params->display);
    if (!new->display) {
	free((void *) new);
	return NULL;
    }
    new->screen = DefaultScreen(new->display);

    new->attributes =
	(XSetWindowAttributes *) malloc(sizeof(XSetWindowAttributes));
    chkalloc(new->attributes);
    new->attributes->background_pixel =
	BlackPixel(new->display, new->screen);
    new->attributes->border_pixel = BlackPixel(new->display, new->screen);
    new->attributes->event_mask = ButtonPressMask | StructureNotifyMask |
	ButtonReleaseMask | PointerMotionMask | KeyPressMask |
	ExposureMask | KeyReleaseMask;


    new->attr_mask = CWBackPixel | CWEventMask;
    if (params->fullscreen || params->rootwindow) {
	new->attributes->override_redirect = True;
	new->attr_mask |= CWOverrideRedirect;
    } else
	new->attr_mask |= CWBorderPixel;
    new->class = InputOutput;
    new->xcolor.n = 0;
    new->parent_window = RootWindow(new->display, new->screen);
    defaultvisual = DefaultVisual(new->display, new->screen);
    new->params = params;

    found = 0;
    for (i = 31; i > 13 && !found; i--)
	if (XMatchVisualInfo
	    (new->display, new->screen, i, TrueColor, &vis)) {
	    found = 1;
	}
    if (defaultvisual->class != StaticGray
	&& defaultvisual->class != GrayScale) {
	for (i = 8; i && !found; i--)
	    if (XMatchVisualInfo
		(new->display, new->screen, i, PseudoColor, &vis)) {
		found = 1;
	    }
	for (i = 8; i && !found; i--)
	    if (XMatchVisualInfo
		(new->display, new->screen, i, StaticColor, &vis)) {
		found = 1;
	    }
	for (i = 8; i && !found; i--)
	    if (XMatchVisualInfo
		(new->display, new->screen, i, TrueColor, &vis)) {
		found = 1;
	    }
    }
    if (!found
	&& XMatchVisualInfo(new->display, new->screen, 8, StaticGray,
			    &vis)) {
	found = 1;
    }
    for (i = 8; i && !found; i--)
	if (XMatchVisualInfo
	    (new->display, new->screen, i, GrayScale, &vis)) {
	    found = 1;
	}
    if (!found
	&& XMatchVisualInfo(new->display, new->screen, 1, StaticGray,
			    &vis)) {
	found = 8;
    }
    if (!found || params->fullscreen || params->rootwindow) {
	new->visual = defaultvisual;
	new->depth = DefaultDepth(new->display, new->screen);
    } else {
	new->visual = vis.visual;
	new->depth = vis.depth;
    }

    switch (new->visual->class) {
    case StaticColor:
    case StaticGray:
      smallcolor:
	new->truecolor = 0;
	new->fixedcolormap = 1;
	break;
    case PseudoColor:
    case GrayScale:
	if (new->depth <= 8) {
	    new->truecolor = 0;
	    new->fixedcolormap = 0;
	} else {
	    goto visuals;
	}
	break;
    case TrueColor:
	new->truecolor = 1;
	new->fixedcolormap = 1;
	if (new->depth <= 8)
	    goto smallcolor;
	if (new->depth > 32) {
	    goto visuals;
	}
	break;
    default:
      visuals:
	printf
	    ("Unusuported visual. Please contact authors. Maybe it will be supported in next release:)\n");
	return (NULL);
    }
    new->privatecolormap = params->privatecolormap;
    new->attributes->colormap = new->defaultcolormap =
	DefaultColormap(new->display, new->screen);
    if (new->visual->visualid != defaultvisual->visualid) {
	new->privatecolormap = 1;
    }
    if ( /*!new->fixedcolormap && */ new->privatecolormap) {
	unsigned long pixels[256];
	int i;
	new->attributes->colormap =
	    XCreateColormap(new->display,
			    RootWindow(new->display, new->screen),
			    new->visual, AllocNone);
	if (new->visual->visualid == defaultvisual->visualid
	    && new->visual->class == PseudoColor) {
	    XAllocColorCells(new->display, new->attributes->colormap, 1, 0,
			     0, pixels, MAX(new->visual->map_entries,
					    256));
	    for (i = 0; i < 16; i++) {
		new->xcolor.c[i].pixel = pixels[i];
	    }
	    XQueryColors(new->display, new->defaultcolormap, new->xcolor.c,
			 16);
	    XStoreColors(new->display, new->attributes->colormap,
			 new->xcolor.c, 16);
	}
    }
    new->colormap = new->attributes->colormap;
    new->attr_mask |= CWColormap;

    new->window_name = (char *) s;
    new->height = y;
    new->width = x;
    new->border_width = 2;
    new->lastx = 0;
    new->lasty = 0;
    new->font_struct = (XFontStruct *) NULL;

    if (params->fullscreen || params->rootwindow) {
	Window wtmp;
	int tmp;
	/* Get size of the root window */
	XGetGeometry(new->display, RootWindow(new->display, new->screen), &wtmp, &tmp, &tmp, &new->width, &new->height, (unsigned int *) &tmp,	/* border width */
		     (unsigned int *) &tmp);	/* depth */
	new->border_width = 0;
    }

    if (params->windowid != -1) {
	Window wtmp;
	int tmp;

	new->parent_window = params->windowid;
	XGetGeometry(new->display, new->parent_window, &wtmp, &tmp, &tmp, &new->width, &new->height, (unsigned int *) &tmp,	/* border width */
		     (unsigned int *) &tmp);	/* depth */
	XSelectInput(new->display, new->parent_window, ResizeRedirectMask);
    }
    if (params->rootwindow)
	new->window = RootWindow(new->display, new->screen);
    else
	new->window = XCreateWindow(new->display, new->parent_window, 0, 0,
				    new->width, new->height,
				    new->border_width, new->depth,
				    new->class, new->visual,
				    new->attr_mask, new->attributes);

    classHint.res_name = (char *) "xaos";
    classHint.res_class = (char *) "XaoS";
    hints = XAllocWMHints();
    hints->initial_state = NormalState;
    hints->window_group = new->window;
    hints->flags = (WindowGroupHint | StateHint);

    {
	int fake = 0;

	if (prog_argc < 2)
	    fake = 1;

	if (fake == 0)
	    if (strcmp(prog_argv[prog_argc - 2], "-driver") &&
		strcmp(prog_argv[prog_argc - 1], "x11"))
		fake = 1;

	if (fake) {
	    int i;
	    faked_argv =
		(char **) malloc((prog_argc + 2) * sizeof(char *));
	    for (i = 0; i < prog_argc; i++)
		faked_argv[i] = prog_argv[i];
	    faked_argv[prog_argc] = (char *) "-driver";
	    faked_argv[prog_argc + 1] = (char *) "x11";

	    XSetWMProperties(new->display, new->window, NULL, NULL,
			     faked_argv, prog_argc + 2, NULL, hints,
			     &classHint);

	    free(faked_argv);
	} else
	    XSetWMProperties(new->display, new->window, NULL, NULL,
			     prog_argv, prog_argc, NULL, hints,
			     &classHint);
    }
    XSetIconName(new->display, new->window, "xaos");

    wmDeleteWindow = XInternAtom(new->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(new->display, new->window, &wmDeleteWindow, 1);

    new->gc = XCreateGC(new->display, new->window, 0L, &(new->xgcvalues));
    XSetBackground(new->display, new->gc,
		   BlackPixel(new->display, new->screen));
    XSetForeground(new->display, new->gc,
		   WhitePixel(new->display, new->screen));
    XStoreName(new->display, new->window, new->window_name);
    XMapWindow(new->display, new->window);
    if (params->fullscreen || params->rootwindow)
	XSetInputFocus(new->display, new->window, RevertToNone,
		       CurrentTime);

    return (new);
}

void xsetcolor(xdisplay * d, int col)
{
    switch (col) {
    case 0:
	XSetForeground(d->display, d->gc,
		       BlackPixel(d->display, d->screen));
	break;
    case 1:
	XSetForeground(d->display, d->gc,
		       WhitePixel(d->display, d->screen));
	break;
    default:
	if ((col - 2) > d->xcolor.n) {
	    fprintf(stderr, "color error\n");
	    exit(-1);
	}
	XSetForeground(d->display, d->gc, d->xcolor.c[col - 2].pixel);
	break;
    }
}

void xsetpaletterange(xdisplay * d, ui_palette c, int start, int end)
{
    int i;
    if (d->visual->class == StaticColor || d->visual->class == TrueColor) {
	for (i = start; i < end; i++)
	    d->xcolor.c[i].pixel = i;
	XQueryColors(d->display, d->colormap, d->xcolor.c + start,
		     end - start);
	for (i = start; i < end; i++) {
	    c[i - start][0] = d->xcolor.c[i].red / 256;
	    c[i - start][1] = d->xcolor.c[i].green / 256;
	    c[i - start][2] = d->xcolor.c[i].blue / 256;
	}

    } else {
	for (i = start; i < end; i++) {
	    d->xcolor.c[i].pixel = i;
	    d->xcolor.c[i].flags = DoRed | DoGreen | DoBlue;
	    d->xcolor.c[i].red = c[i - start][0] * 256;
	    d->xcolor.c[i].green = c[i - start][1] * 256;
	    d->xcolor.c[i].blue = c[i - start][2] * 256;
	}
	XStoreColors(d->display, d->colormap, d->xcolor.c + start,
		     end - start);
    }
}

int xalloc_color(xdisplay * d, int r, int g, int b, int readwrite)
{
    d->xcolor.n++;
    d->xcolor.c[d->xcolor.n - 1].flags = DoRed | DoGreen | DoBlue;
    d->xcolor.c[d->xcolor.n - 1].red = r;
    d->xcolor.c[d->xcolor.n - 1].green = g;
    d->xcolor.c[d->xcolor.n - 1].blue = b;
    d->xcolor.c[d->xcolor.n - 1].pixel = d->xcolor.n - 1;
    if ((readwrite && !d->fixedcolormap) || d->privatecolormap) {
	unsigned long cell;
	if (d->privatecolormap) {
	    cell = d->xcolor.c[d->xcolor.n - 1].pixel += 16;
	    if ((int) d->xcolor.c[d->xcolor.n - 1].pixel >=
		d->visual->map_entries) {
		d->xcolor.n--;
		return (-1);
	    }
	} else {
	    if (!XAllocColorCells
		(d->display, d->colormap, 0, 0, 0, &cell, 1)) {
		d->xcolor.n--;
		if (d->xcolor.n <= 32)
		    printf
			("Colormap is too full! close some colorfull applications or use -private\n");
		return (-1);
	    }
	    d->xcolor.c[d->xcolor.n - 1].pixel = cell;
	}
	XStoreColor(d->display, d->colormap,
		    &(d->xcolor.c[d->xcolor.n - 1]));
	return ((int) cell);
    }
    if (!XAllocColor
	(d->display, d->colormap, &(d->xcolor.c[d->xcolor.n - 1]))) {
	d->xcolor.n--;
	if (d->xcolor.n <= 32)
	    printf
		("Colormap is too full! close some colorfull aplications or use -private\n");
	return (-1);
    }
    d->pixels[d->xcolor.n - 1] = d->xcolor.c[d->xcolor.n - 1].pixel;
    return (d->depth !=
	    8 ? d->xcolor.n - 1 : (int) d->xcolor.c[d->xcolor.n -
						    1].pixel);
}

void xfree_colors(xdisplay * d)
{
    unsigned long pixels[256];
    int i;
    for (i = 0; i < d->xcolor.n; i++)
	pixels[i] = d->xcolor.c[i].pixel;
    if (!d->privatecolormap)
	XFreeColors(d->display, d->colormap, pixels, d->xcolor.n, 0);
    d->xcolor.n = 0;
}

void xfree_display(xdisplay * d)
{
    XSync(d->display, 0);
    if (d->font_struct != (XFontStruct *) NULL) {
	XFreeFont(d->display, d->font_struct);
    }
    XUnmapWindow(d->display, d->window);
    XDestroyWindow(d->display, d->window);
    XCloseDisplay(d->display);
    free((void *) d->attributes);
    free((void *) d);
}

void xclear_screen(xdisplay * d)
{
    XClearWindow(d->display, d->window);
    d->screen_changed = 1;
}

void xmoveto(xdisplay * d, int x, int y)
{
    d->lastx = x, d->lasty = y;
}

int xsetfont(xdisplay * d, CONST char *font_name)
{

    if (d->font_struct != (XFontStruct *) NULL) {
	XFreeFont(d->display, d->font_struct);
    }
    d->font_struct = XLoadQueryFont(d->display, font_name);
    XSetFont(d->display, d->gc, d->font_struct->fid);
    if (!d->font_struct) {
	fprintf(stderr, "could not load font: %s\n", font_name);
	exit(-1);
    }
    return (d->font_struct->max_bounds.ascent +
	    d->font_struct->max_bounds.descent);
}

void xouttext(xdisplay * d, CONST char *string)
{
    int sz;

    sz = (int) strlen(string);
    XDrawImageString(d->display, d->window, d->gc, d->lastx, d->lasty,
		     string, sz);
}

void xresize(xdisplay * d, XEvent * ev)
{
    XSync(d->display, False);
    d->width = ev->xconfigure.width;
    d->height = ev->xconfigure.height;
}

#endif
