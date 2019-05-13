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
#ifndef UI_H
#define UI_H
/*
#include "xio.h"
#include "zoom.h"
*/
#include "config.h"
#include "param.h"
#include "xmenu.h"
#ifdef __cplusplus
extern "C"
{
#endif


#undef BUTTON1
#undef BUTTON2
#undef BUTTON3
#define BUTTON1 256
#define BUTTON2 512
#define BUTTON3 1024
#define UI_C256 1
#define UI_GRAYSCALE 2
#define UI_TRUECOLOR16 4
#define UI_TRUECOLOR24 8
#define UI_TRUECOLOR 16
#define UI_PROTECTIMAGES 32
#define UI_LBITMAP 256
#define UI_MBITMAP 512
#define UI_LIBITMAP 1024
#define UI_MIBITMAP 2048
#define UI_FIXEDCOLOR 4096


#define NORMALMOUSE 0
#define WAITMOUSE 1
#define REPLAYMOUSE 2
#define VJMOUSE 3

#define UIKEY_UP 257
#define UIKEY_DOWN 258
#define UIKEY_LEFT 259
#define UIKEY_RIGHT 260
#define UIKEY_ESC 261
#define UIKEY_BACKSPACE 8
#define UIKEY_TAB '\t'
#define UIKEY_HOME 262
#define UIKEY_END 263
#define UIKEY_PGUP 264
#define UIKEY_PGDOWN 265

    typedef unsigned char ui_rgb[4];
    typedef ui_rgb *ui_palette;
    struct uih_context;
    struct ui_driver
    {
        const char *name;
        int textwidth;
        int textheight;         /*width of text */
        /*int helpsize; */
        const struct params *params;
        int flags;
        float width, height;
        int maxwidth, maxheight;
        int imagetype;
        int palettestart, paletteend, maxentries;
        int rmask, gmask, bmask;
    };

    extern struct uih_context *globaluih;

#ifdef USE_LOCALEPATH
    extern char *localepath;
#endif

    number_t ui_getfloat (const char *text);
    void ui_resize (void);
    void ui_call_resize (void);
    void ui_quit (void) NORETURN;
    void ui_menuactivate (const menuitem * item, dialogparam * d);
    int ui_key (int);
    xio_path ui_getfile (const char *basename, const char *extension);
    char *ui_getpos (void);
    int MAIN_FUNCTION (int argc, char **argv);



#ifndef RANDOM_PALETTE_SIZE
#define RANDOM_PALETTE_SIZE 1   /*FIXME currently ignored */
#define FULLSCREEN 2
#define UPDATE_AFTER_PALETTE 4
#define UPDATE_AFTER_RESIZE 8
#define PALETTE_ROTATION 16
#define ASYNC_PALETTE 32
#define ROTATE_INSIDE_CALCULATION 64
#define PALETTE_REDISPLAYS 128
#define SCREENSIZE 256
#define PIXELSIZE 512
#define RESOLUTION 1024
#endif
#define NOFLUSHDISPLAY 2048
#define AALIB	(4096*2)
#define RESIZE_COMMAND (4096*4)

#ifdef __cplusplus
}
#endif
#endif                          /* UI_H */
