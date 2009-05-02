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
#ifndef XAOS_X11_H
#define XAOS_X11_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include "config.h"
#include "ui.h"
#ifdef MITSHM
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif				/* MITSHM */

typedef struct {
    int n;
    XColor c[256];
} xcol_t;

typedef struct {
    int privatecolormap;
    int usedefault;
    int nomitshm;
    int fullscreen;
    char *display;
    int windowid;
    int rootwindow;
} xlibparam;

typedef struct {
    Colormap colormap;
    Colormap defaultcolormap;
    int fixedcolormap;
    int privatecolormap;
    xlibparam *params;
    Display *display;
    Window parent_window;
    Window window;
    unsigned int width, height;
    unsigned int bwidth, bheight;
    unsigned int border_width;
    unsigned long background;
    int depth;
    unsigned int class;
    Visual *visual;
    unsigned long valuemask;
    XSetWindowAttributes *attributes;
    unsigned long attr_mask;
    XSizeHints sizehints;
    int screen;
    char *window_name;
    int status;
    GC gc;
    XGCValues xgcvalues;
    xcol_t xcolor;
    Pixmap pixmap;
    XFontStruct *font_struct;
    int screen_changed;
    int lastx, lasty;
    int mouse_x, mouse_y;
    unsigned int mouse_buttons;
    int current;
    XImage *image[2];
#ifdef MITSHM
    XShmSegmentInfo xshminfo[2];
    int SharedMemOption;
    int SharedMemFlag;
#endif				/* MITSHM */
    unsigned long pixels[256];
    char *vbuffs[2];
    char *data[2];
    char *vbuff;
    char *back;
    int truecolor;
    int linewidth;
} xdisplay;

void xsetpaletterange(xdisplay * d, ui_palette c, int start, int end);
extern int alloc_shm_image(xdisplay * d);
extern void free_shm_image(xdisplay * d);
extern int alloc_image(xdisplay * d);
extern void free_image(xdisplay * d);
extern int xupdate_size(xdisplay * d);
extern void xflip_buffers(xdisplay * d);
extern xdisplay *xalloc_display(CONST char *n, int x, int y,
				xlibparam * p);
extern void xfree_display(xdisplay * d);
extern void xsetcolor(xdisplay * d, int col);
extern int xsetfont(xdisplay * d, CONST char *font_name);
extern int xalloc_color(xdisplay * d, int r, int g, int b, int readwrite);
extern void xfree_colors(xdisplay * d);
extern void xline(xdisplay * d, int x1, int y1, int x2, int y2);
extern void xmoveto(xdisplay * d, int x, int y);
extern void xlineto(xdisplay * d, int x, int y);
extern void xrect(xdisplay * d, int x1, int y1, int x2, int y2);
extern void xfillrect(xdisplay * d, int x1, int y1, int x2, int y2);
extern void xarc(xdisplay * d, int x, int y, unsigned int w,
		 unsigned int h, int a1, int a2);
extern void xfillarc(xdisplay * d, int x, int y, unsigned int w,
		     unsigned int h, int a1, int a2);
extern void xpoint(xdisplay * d, int x, int y);
extern void xflush(xdisplay * d);
extern void xclear_screen(xdisplay * d);
extern void xrotate_palette(xdisplay * d, int direction,
			    unsigned char c[3][256], int ncolors);
extern void draw_screen(xdisplay * d);
extern void xouttext(xdisplay * d, CONST char *string);
extern void xresize(xdisplay * d, XEvent * ev);
extern int xmouse_x(xdisplay * d);
extern int xmouse_y(xdisplay * d);
extern void xmouse_update(xdisplay * d);
extern unsigned int xmouse_buttons(xdisplay * d);

extern Atom wmDeleteWindow;

#endif				/* XAOS_X11_H */
