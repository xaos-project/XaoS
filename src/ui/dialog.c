/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright (C) 1996 by
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
#undef _EFENCE_
#include <config.h>
#ifdef _plan9_
#include <u.h>
#include <libc.h>
#include <ctype.h>
#else
#include <aconfig.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#ifndef _MAC
#include <sys/stat.h>
#endif
#endif
#include <fconfig.h>
#ifndef _plan9_
#include <assert.h>
#endif
#include <filter.h>
#include <fractal.h>
#include <ui_helper.h>
#include <ui.h>
#include <xmenu.h>
#include <grlib.h>
#include "uiint.h"
#include <xldio.h>
#include <misc-f.h>

#ifdef HAVE_GETTEXT
#include <libintl.h>
#else
#define gettext(STRING) STRING
#endif

struct dialogitem;
struct dialogtype {
    void (*build) (struct dialogitem * item, const menudialog * entry);
    int (*key) (struct dialogitem * item, int key);
    void (*mouse) (struct dialogitem * item, int x, int y, int buttons,
		   int flags);
    void (*destroy) (struct dialogitem * item, dialogparam * param);
    void (*draw) (struct dialogitem * item);
    void (*unselect) (struct dialogitem * item);
};
struct dialogitem {
    int y;
    int width, width1, height;
    const menudialog *dialog;
    void *data;
    const struct dialogtype *type;
};
static struct opendialog {
    int x, y, width, height;
    int half;
    int nitems;
    const menudialog *dialog;
    int mousereleased;
    int mousegrab;
    const menuitem *item;
    int current;
    struct dialogitem *items;
    struct uih_window *window;
} dialog;

static dialogparam *qparam;
static const menuitem *qitem;

int dialogvisible;

int yesnodialogvisible;
static struct yesnodialog {
    int width;
    int questionwidth;
    int mousereleased;
    char *question;
    void (*handler) (int yes);
    int selected;
    int pressed;
    struct uih_window *window;
} yesnodialog;

// These 3 definitions are no longer used:
static const char *const oktext = "OK";
static const char *const canceltext = "Cancel";
static const char *const helptext = "Help";

static int okwidth;
static int cancelwidth;
#define SELECTED(item) ((item-dialog.items)==dialog.current)
static void NEXT(void)
{
    dialog.items[dialog.current].type->unselect(dialog.items +
						dialog.current);
    dialog.current = (dialog.current + 1) % dialog.nitems;
    uih->display = 1;
}

static void PREV(void)
{
    dialog.items[dialog.current].type->unselect(dialog.items +
						dialog.current);
    dialog.current =
	dialog.current ? (dialog.current - 1) : dialog.nitems - 1;
    uih->display = 1;
}

struct okdata {
    int pressed;
    int selected;
};

const char *const yestext = "Yes";
const char *const notext = "No";
#define YESNOX ((uih->image->width-yesnodialog.width)/2)
#define YESNOHEIGHT (2*BUTTONHEIGHT+2*BORDERHEIGHT)
#define YESNOY ((uih->image->height-YESNOHEIGHT)/2)
void
ui_drawbutton(const char *text, int pressed, int selected, int x1, int x2,
	      int y)
{
    int width = xtextwidth(uih->image, uih->font, text);
    /*printf("%s %i %i\n",text,pressed,selected); */
    if (uih->palette->type & BITMAPS) {
	uih_drawborder(uih, x1, y, x2 - x1, BUTTONHEIGHT,
		       (pressed != 0
			|| selected != 0) * BORDER_PRESSED | BORDER_LIGHT);
	xprint(uih->image, uih->font, (x1 + x2 - width) / 2 + pressed,
	       y + BORDERHEIGHT + pressed, text, selected
	       || pressed ? BGCOLOR(uih) : FGCOLOR(uih), BGCOLOR(uih),
	       TEXT_PRESSED);
    } else {
	uih_drawborder(uih, x1, y, x2 - x1, BUTTONHEIGHT,
		       (pressed != 0) * BORDER_PRESSED | BORDER_LIGHT);
	xprint(uih->image, uih->font, (x1 + x2 - width) / 2 + pressed,
	       y + BORDERHEIGHT + pressed, text, 
	       selected ? SELCOLOR(uih) : FGCOLOR(uih), BGCOLOR(uih),
	       /*TEXT_PRESSED */ 0);
    }
}

static void
ui_yesnopos(struct uih_context *c, int *x, int *y, int *w, int *h,
	    void *data)
{
#ifdef _plan9_
#define filevisible 0
#endif
    if (filevisible || helpvisible) {
	*x = *y = *w = *h = 0;
	return;
    }
    *w = yesnodialog.width;
    *h = YESNOHEIGHT;
    *x = YESNOX;
    *y = YESNOY;
}

static void ui_drawyesno(struct uih_context *c, void *data)
{
    xprint(uih->image, uih->font,
	   YESNOX + (yesnodialog.width - yesnodialog.questionwidth) / 2,
	   YESNOY + BORDERHEIGHT, yesnodialog.question, 
	   FGCOLOR(uih), BGCOLOR(uih), 0);
    ui_drawbutton(yestext, yesnodialog.pressed == 0,
		  yesnodialog.selected == 0, YESNOX + BORDERWIDTH + 1,
		  YESNOX + (yesnodialog.width) / 2 - 1,
		  YESNOY + BUTTONHEIGHT + BORDERHEIGHT);
    ui_drawbutton(notext, yesnodialog.pressed == 1,
		  yesnodialog.selected == 1,
		  YESNOX + (yesnodialog.width) / 2 + 1,
		  YESNOX + yesnodialog.width - BORDERWIDTH - 1,
		  YESNOY + BUTTONHEIGHT + BORDERHEIGHT);
}

static void ui_closeyesno(int success)
{
    if (!yesnodialogvisible)
	return;
    free(yesnodialog.question);
    yesnodialog.handler(success);
    yesnodialogvisible = 0;
    uih_removew(uih, yesnodialog.window);
    uih->display = 1;
}

void ui_buildyesno(const char *question, void (*handler) (int yes))
{
    if (yesnodialogvisible)
	ui_closeyesno(0);
    yesnodialogvisible = 1;
    yesnodialog.questionwidth = xtextwidth(uih->image, uih->font, question);
    yesnodialog.question = mystrdup(question);
    yesnodialog.mousereleased = 0;
    yesnodialog.width =
	xtextwidth(uih->image, uih->font, yestext) + xtextwidth(uih->image, uih->font,
						    notext) +
	8 * BORDERWIDTH + 2;
    if (yesnodialog.width < yesnodialog.questionwidth)
	yesnodialog.width = yesnodialog.questionwidth;
    yesnodialog.width += 2 * BORDERWIDTH;
    yesnodialog.handler = handler;
    yesnodialog.selected = 0;
    yesnodialog.pressed = -1;
    yesnodialog.window =
	uih_registerw(uih, ui_yesnopos, ui_drawyesno, NULL, DRAWBORDER);
    uih->display = 1;
}

static int ui_keyyesno(int key)
{
    if (!yesnodialogvisible)
	return 0;
    switch (key) {
    case UIKEY_LEFT:
	yesnodialog.selected ^= 1;
	uih->display = 1;
	return 1;
    case UIKEY_UP:
	return 1;
    case UIKEY_RIGHT:
    case UIKEY_TAB:
	yesnodialog.selected ^= 1;
	uih->display = 1;
	return 1;
    case UIKEY_DOWN:
	return 1;
    case 13:
    case '\n':
	ui_closeyesno(!yesnodialog.selected);
	return 1;
    case UIKEY_ESC:
	ui_closeyesno(0);
	return 1;
    }
    return 1;
}

static int ui_mouseyesno(int x, int y, int buttons, int flags)
{
    int mouseat = 0;
    if (!yesnodialogvisible)
	return 0;
    if (!yesnodialog.mousereleased && (flags & MOUSE_RELEASE)) {
	yesnodialog.mousereleased = 1;
	return 1;
    }
    if (!yesnodialog.mousereleased && (flags & MOUSE_DRAG)) {
	return 1;
    }
    yesnodialog.mousereleased = 1;
    if (x < YESNOX || y < YESNOY || x > YESNOX + yesnodialog.width
	|| y > YESNOY + YESNOHEIGHT) {
	if (flags & MOUSE_PRESS) {
	    ui_closeyesno(0);
	} else {
	    if (yesnodialog.pressed != -1)
		uih->display = 1;
	    yesnodialog.pressed = -1;
	}
	return 1;
    }
    if (x > YESNOX + yesnodialog.width / 2)
	mouseat = 1;
    if (flags & MOUSE_DRAG) {
	if (yesnodialog.pressed != mouseat)
	    uih->display = 1;
	if (yesnodialog.selected != mouseat)
	    uih->display = 1;
	yesnodialog.selected = mouseat;
	yesnodialog.pressed = mouseat;
    } else {
	if ((flags & MOUSE_MOVE) && yesnodialog.selected != mouseat)
	    uih->display = 1, yesnodialog.selected = mouseat;
	if (yesnodialog.pressed != -1)
	    uih->display = 1;
	yesnodialog.pressed = -1;
    }
    if (flags & MOUSE_RELEASE) {
	ui_closeyesno(!mouseat);
    }
    return 1;
}

static void ui_buildok(struct dialogitem *item, const menudialog * entry)
{
    struct okdata *ok;
    item->height = BUTTONHEIGHT;
    okwidth = xtextwidth(uih->image, uih->font, gettext("OK"));
    cancelwidth = xtextwidth(uih->image, uih->font, gettext("Cancel"));
    item->width = okwidth + 2 * BORDERWIDTH + 2;
    item->width1 = cancelwidth + 2 * BORDERWIDTH + 2;
    if (item->width < item->width1)
	item->width = item->width1;
    if (item->width > item->width1)
	item->width1 = item->width;
    item->data = ok = (struct okdata *) malloc(sizeof(struct okdata));
    ok->pressed = -1;
    ok->selected = 0;
}

static void ui_destroyok(struct dialogitem *item, dialogparam * param)
{
    free(item->data);
}

static int ui_keyok(struct dialogitem *item, int key)
{
    struct okdata *ok = (struct okdata *) item->data;
    switch (key) {
    case UIKEY_LEFT:
	if (ok->selected >= 1) {
	    ok->selected--;
	    uih->display = 1;
	    return 1;
	}
    case UIKEY_UP:
	PREV();
	return 1;
    case UIKEY_RIGHT:
    case UIKEY_TAB:
	if (ok->selected < 2) {
	    ok->selected++;
	    uih->display = 1;
	    return 1;
	}
    case UIKEY_DOWN:
	NEXT();
	return 1;
    case 13:
    case '\n':
	if (ok->selected <= 1)
	    ui_closedialog(!ok->selected);
	else
	    ui_help(dialog.item->shortname);
	return 1;
    }
    return 0;
}

static void
ui_mouseok(struct dialogitem *item, int x, int y, int buttons, int flags)
{
    struct okdata *ok = (struct okdata *) item->data;
    int mouseat = 0;
    if (x > dialog.x + dialog.width / 3)
	mouseat = 1;
    if (x > dialog.x + 2 * dialog.width / 3)
	mouseat = 2;
    if (flags & MOUSE_DRAG) {
	if (ok->pressed != mouseat)
	    uih->display = 1;
	if (ok->selected != mouseat)
	    uih->display = 1;
	ok->selected = mouseat;
	ok->pressed = mouseat;
    } else {
	if ((flags & MOUSE_MOVE) && ok->selected != mouseat)
	    uih->display = 1, ok->selected = mouseat;
	if (ok->pressed != -1)
	    uih->display = 1;
	ok->pressed = -1;
    }
    if (flags & MOUSE_RELEASE) {
	if (mouseat < 2)
	    ui_closedialog(!mouseat);
	else
	    ui_help(dialog.item->shortname);
    }
}

static void ui_drawok(struct dialogitem *item)
{
    struct okdata *ok = (struct okdata *) item->data;
    ui_drawbutton(gettext("OK"), ok->pressed == 0, SELECTED(item)
		  && ok->selected == 0, dialog.x + BORDERWIDTH + 1,
		  dialog.x + (dialog.width) / 3 - 1, item->y);
    ui_drawbutton(gettext("Cancel"), ok->pressed == 1, SELECTED(item)
		  && ok->selected == 1, dialog.x + (dialog.width) / 3 + 1,
		  dialog.x + 2 * dialog.width / 3 - BORDERWIDTH, item->y);
    ui_drawbutton(gettext("Help"), ok->pressed == 2, SELECTED(item)
		  && ok->selected == 2, dialog.x + 2 * (dialog.width) / 3,
		  dialog.x + dialog.width - BORDERWIDTH - 1, item->y);
}

static void ui_unselectok(struct dialogitem *item)
{
    struct okdata *ok = (struct okdata *) item->data;
    ok->pressed = -1;
    ok->selected = 0;
    uih->display = 1;
}

const static struct dialogtype okdialog = {
    ui_buildok,
    ui_keyok,
    ui_mouseok,
    ui_destroyok,
    ui_drawok,
    ui_unselectok
};

void ui_updatetext(struct ui_textdata *d)
{
    int again = 1;
    int i;
    int wi;
    int len = (int) strlen(d->text);
    if (d->start >= len)
	d->start = 0;
    if (d->cursor > len)
	d->cursor = len;
    if (d->cursor < d->start)
	d->start = d->cursor;
    do {
	wi = 0;
	for (i = 0; d->text[d->start + i]; i++) {
	    if (d->start + i == d->cursor)
		d->cursorpos = wi;
	    wi += xtextcharw(uih->image, uih->font, d->text[d->start + i]);
	    if (wi >= d->width) {
		break;
	    }
	}
	if (d->start + i == d->cursor && wi < d->width)
	    d->cursorpos = wi;
	if (d->start + i < d->cursor)
	    d->start++;
	else
	    again = 0;
    }
    while (again);
    d->ndisplayed = i;
    while (again);
}

struct ui_textdata *ui_opentext(int x, int y, int width, const char *def)
{
    struct ui_textdata *d = (struct ui_textdata *) malloc(sizeof(*d));
    char *text;
    int size = 100;
    if ((int) strlen(def) > size)
	size = (int) strlen(def) * 2;
    d->x = x;
    d->y = y;
    d->width = width;
    text = (char *) malloc(size);
    strcpy(text, def);
    d->text = text;
    d->cursor = 0;
    d->cursorpos = 0;
    d->start = 0;
    d->ndisplayed = 0;
    d->clear = 1;
    d->size = size;
    ui_updatetext(d);
    return (d);
}

void ui_drawtext(struct ui_textdata *d, int active)
{
    char *c = (char *) malloc(d->ndisplayed + 2);
    strncpy(c, d->text + d->start, d->ndisplayed);
    c[d->ndisplayed] = 0;
    xprint(uih->image, uih->font, d->x, d->y, c, 
	   (uih->palette->type & BITMAPS) ? BGCOLOR(uih) : ((active
							     && d->clear) ?
							    SELCOLOR(uih) :
							    FGCOLOR(uih)),
	   BGCOLOR(uih),
	   (uih->palette->type & BITMAPS) ? TEXT_PRESSED : 0);
    if (active) {
	xdrawcursor(uih->image, d->x + d->cursorpos, d->y,
		    (uih->palette->
		     type & BITMAPS) ? BGCOLOR(uih) : SELCOLOR(uih),
		    xtextheight(uih->image, uih->font));
    }
    free(c);
}

void ui_textmouse(struct ui_textdata *d, int x, int y)
{
    if (y > d->y && y < d->y + xtextheight(uih->image, uih->font) && x > d->x) {
	int w = 0;
	int i;
	int xp = d->x;
	for (i = 0; i < d->ndisplayed + 1 && xp - w / 2 < x; i++) {
	    w = xtextcharw(uih->image, uih->font, d->text[i + d->start]);
	    xp += w;
	}
	d->cursor = i + d->start - 1;
	if (d->cursor < 0)
	    d->cursor = 0;
	d->clear = 0;
	ui_updatetext(d);
	uih->display = 1;
    }
}

void ui_closetext(struct ui_textdata *d)
{
    free(d->text);
    free(d);
}

int ui_textkey(struct ui_textdata *d, int key)
{
    switch (key) {
    case UIKEY_LEFT:
	if (d->clear)
	    d->clear = 0;
	if (d->cursor)
	    d->cursor--;
	else
	    return 0;
	ui_updatetext(d);
	uih->display = 1;
	return 1;
    case UIKEY_RIGHT:
	if (d->clear)
	    d->clear = 0;
	if (d->cursor < (int) strlen(d->text))
	    d->cursor++;
	else
	    return 0;
	ui_updatetext(d);
	uih->display = 1;
	return 1;
    case UIKEY_HOME:
	if (d->clear)
	    d->clear = 0;
	d->cursor = 0;
	ui_updatetext(d);
	uih->display = 1;
	return 1;
    case UIKEY_END:
	if (d->clear)
	    d->clear = 0;
	d->cursor = (int) strlen(d->text);
	ui_updatetext(d);
	uih->display = 1;
	return 1;
    case UIKEY_BACKSPACE:
	if (d->clear)
	    d->text[0] = 0, d->clear = 0;
	else if (d->cursor) {
	    int len, i;
	    len = (int) strlen(d->text);
	    for (i = d->cursor; i <= len; i++) {
		d->text[i - 1] = d->text[i];
	    }
	    d->cursor--;
	}
	ui_updatetext(d);
	uih->display = 1;
	return 1;
    }
    if (isprint(key)) {
	int i;
	int len;
	if (d->clear)
	    d->text[0] = 0, d->clear = 0;
	if ((len = (int) strlen(d->text)) > d->size - 2) {
	    d->text = (char *) realloc(d->text, d->size * 2);
	}
	for (i = len; i >= d->cursor; i--) {
	    d->text[i + 1] = d->text[i];
	}
	d->text[d->cursor] = key;
	d->cursor++;
	ui_updatetext(d);
	uih->display = 1;
	return 1;
    }
    return 0;
}

static void
ui_buildstring(struct dialogitem *item, const menudialog * entry)
{
    item->height = BUTTONHEIGHT;
    item->width = xtextwidth(uih->image, uih->font, item->dialog->question);
    item->width1 = xtextcharw(uih->image, uih->font, 'w') * 20;
    item->data = ui_opentext(0, 0, 2043, item->dialog->defstr);
}

static void ui_destroystring(struct dialogitem *item, dialogparam * param)
{
    struct ui_textdata *text = (struct ui_textdata *) item->data;
    param->dstring = mystrdup(text->text);
    ui_closetext(text);
}

static void ui_drawquestion(struct dialogitem *item)
{
    if (uih->palette->type & BITMAPS) {
	if (SELECTED(item))
	    xrectangle(uih->image, dialog.x + BORDERWIDTH, item->y,
		       dialog.half - dialog.x - 2 * BORDERWIDTH,
		       BUTTONHEIGHT, FGCOLOR(uih));
	xprint(uih->image, uih->font, dialog.half - item->width,
	       item->y + BORDERHEIGHT, item->dialog->question,
	       SELECTED(item) ? BGCOLOR(uih) : FGCOLOR(uih),
	       BGCOLOR(uih), TEXT_PRESSED);
    } else {
	xprint(uih->image, uih->font, dialog.half - item->width,
	       item->y + BORDERHEIGHT, item->dialog->question,
	       SELECTED(item) ? SELCOLOR(uih) : FGCOLOR(uih), BGCOLOR(uih),
	       0);
    }
}

static void ui_drawstring(struct dialogitem *item)
{
    struct ui_textdata *text = (struct ui_textdata *) item->data;
    if (text->width == 2043)
	text->x = dialog.half + BORDERWIDTH, text->width =
	    dialog.width + dialog.x - dialog.half - 2 * BORDERWIDTH,
	    text->y = item->y + BORDERHEIGHT, ui_updatetext(text);
    uih_drawborder(uih, dialog.half, item->y,
		   dialog.width - dialog.half + dialog.x - BORDERWIDTH,
		   BUTTONHEIGHT, BORDER_PRESSED | BORDER_LIGHT);
    ui_drawtext(text, SELECTED(item));
    ui_drawquestion(item);
}

static int ui_keystring(struct dialogitem *item, int key)
{
    struct ui_textdata *text = (struct ui_textdata *) item->data;
    return (ui_textkey(text, key));
}

static void
ui_mousestring(struct dialogitem *item, int x, int y, int buttons,
	       int flags)
{
    struct ui_textdata *text = (struct ui_textdata *) item->data;
    if (flags & MOUSE_DRAG) {
	ui_textmouse(text, x, y);
    }
}

static void ui_unselectstring(struct dialogitem *item)
{
}

const static struct dialogtype stringdialog = {
    ui_buildstring,
    ui_keystring,
    ui_mousestring,
    ui_destroystring,
    ui_drawstring,
    ui_unselectstring
};

static void ui_buildint(struct dialogitem *item, const menudialog * entry)
{
    char s[50];
    item->height = BUTTONHEIGHT;
    item->width = xtextwidth(uih->image, uih->font, item->dialog->question);
    item->width1 = xtextcharw(uih->image, uih->font, 'w') * 5;
    sprintf(s, "%i", item->dialog->defint);
    item->data = ui_opentext(0, 0, 2043, s);
}

static void ui_destroyint(struct dialogitem *item, dialogparam * param)
{
    struct ui_textdata *text = (struct ui_textdata *) item->data;
    param->dint = (int) atol(text->text);
    ui_closetext(text);
}

const static struct dialogtype intdialog = {
    ui_buildint,
    ui_keystring,
    ui_mousestring,
    ui_destroyint,
    ui_drawstring,
    ui_unselectstring
};

static const char *ui_getextension(const char *ch)
{
    int i = 0;
    while (ch[i]) {
	if (ch[i] == '*')
	    return (ch + i + 1);
	i++;
    }
    return ch + i;
}

number_t ui_getfloat(const char *c)
{
#ifdef HAVE_LONG_DOUBLE
    long double param;
#else
    double param;
#endif
#ifdef HAVE_LONG_DOUBLE
#ifndef USE_ATOLD
#ifdef USE_XLDIO
    param = x_strtold(c, NULL);
    if (0)
#else
    if (sscanf(c, "%LG", &param) == 0)
#endif
#else
    param = _atold(c);
    if (0)
#endif
    {
#else
    if (sscanf(c, "%lG", &param) == 0) {
#endif
	return 0;
    }
    return (param);
}

#define BROWSEWIDTH /*(2*BORDERWIDTH+xtextcharw(uih->image, uih->font,'B'))*/BUTTONHEIGHT
struct ui_filedata {
    struct ui_textdata *text;
    int active;
    int pressed;
};

static struct dialogitem *curritem;
static void filecallback(const char *name, int succ)
{
    struct ui_filedata *text = (struct ui_filedata *) curritem->data;
    if (succ) {
	ui_closetext(text->text);
	uih->display = 1;
	text->text = ui_opentext(0, 0, 2043, name);
	dialog.mousereleased = 0;
    }
    if (dialog.nitems == 2)
	ui_closedialog(succ);
}

static void ui_buildfile(struct dialogitem *item, const menudialog * entry)
{
    char str[256];
    struct ui_filedata *data =
	(struct ui_filedata *) malloc(sizeof(*data));
    int i = 0;
    item->height = BUTTONHEIGHT;
    item->width = xtextwidth(uih->image, uih->font, item->dialog->question);
    item->width1 = xtextcharw(uih->image, uih->font, 'w') * 20;
    while (item->dialog->defstr[i] != '*' && item->dialog->defstr[i] != 0)
	str[i] = item->dialog->defstr[i], i++;
    str[i] = 0;
    item->data = data;
    if (entry->type == DIALOG_OFILE)
	data->text =
	    ui_opentext(0, 0, 2043,
			ui_getfile(str,
				   ui_getextension(item->dialog->defstr)));
    else
	data->text = ui_opentext(0, 0, 2043, item->dialog->defstr);
    data->active = 0;
    data->pressed = 0;
#ifndef _plan9_
    if (dialog.nitems == 2) {
	curritem = item;
	ui_buildfilesel(data->text->text, "", filecallback);
    }
#endif
}

static void ui_destroyfile(struct dialogitem *item, dialogparam * param)
{
    struct ui_filedata *text = (struct ui_filedata *) item->data;
#ifndef _plan9_
    if (filevisible)
	ui_closefilesel(0);
#endif
    param->dpath = mystrdup(text->text->text);
    ui_closetext(text->text);
    free(text);
}

static void ui_drawfile(struct dialogitem *item)
{
    struct ui_filedata *data = (struct ui_filedata *) item->data;
    int wholesize =
	dialog.width + dialog.x - dialog.half - 2 * BORDERWIDTH;
    if (data->text->width == 2043) {
	data->text->x = dialog.half + BORDERWIDTH,
	    data->text->width = wholesize - BROWSEWIDTH - 2 * BORDERWIDTH,
	    data->text->y = item->y + BORDERHEIGHT;
	ui_updatetext(data->text);
    }
    uih_drawborder(uih, dialog.half, item->y, wholesize - BROWSEWIDTH,
		   BUTTONHEIGHT, BORDER_PRESSED | BORDER_LIGHT);
    ui_drawtext(data->text, SELECTED(item) && !data->active);
    xprint(uih->image, uih->font, dialog.half - item->width,
	   item->y + BORDERHEIGHT, item->dialog->question,
	   SELECTED(item) ? SELCOLOR(uih) : FGCOLOR(uih), BGCOLOR(uih), 0);
    ui_drawquestion(item);
    ui_drawbutton("B", data->pressed && SELECTED(item), SELECTED(item)
		  && data->active,
		  dialog.x + dialog.width - BROWSEWIDTH - BORDERWIDTH,
		  dialog.x + dialog.width - BORDERWIDTH, item->y);
}

static int ui_keyfile(struct dialogitem *item, int key)
{
    struct ui_filedata *text = (struct ui_filedata *) item->data;
    int i = 0;
    if (!text->active)
	i = ui_textkey(text->text, key);
    if (!i) {
	if (key == '\t' || key == UIKEY_RIGHT) {
	    text->active++;
	    if (text->active > 1) {
		text->active = 0;
		return 0;
	    }
	    uih->display = 1;
	    return 1;
	}
	if (key == UIKEY_LEFT) {
	    text->active--;
	    if (text->active < 0) {
		text->active = 1;
		return 0;
	    }
	    return 1;
	}
#ifndef _plan9_
	if ((key == 13 || key == '\n') && text->active) {
	    curritem = item;
	    ui_buildfilesel(text->text->text, "", filecallback);
	    return 1;
	}
#endif
    }
    return (i);
}

static void
ui_mousefile(struct dialogitem *item, int x, int y, int buttons, int flags)
{
    struct ui_filedata *text = (struct ui_filedata *) item->data;
    int i;
    if (flags & MOUSE_MOVE) {
	if (x < dialog.x + dialog.width - BORDERWIDTH - BROWSEWIDTH)
	    i = 0;
	else
	    i = 1;
	if (text->active != i)
	    text->active = i, uih->display = 1;
    }
#ifndef _plan9_
    if ((flags & MOUSE_RELEASE) && text->pressed) {
	text->pressed = 0;
	uih->display = 1;
	curritem = item;
	ui_buildfilesel(text->text->text, "", filecallback);
	return;
    }
#endif
    if (flags & MOUSE_DRAG) {
	if (x < dialog.x + dialog.width - BORDERWIDTH - BROWSEWIDTH) {
	    text->active = 0, ui_textmouse(text->text, x, y);
	    if (text->pressed)
		text->pressed = 0, uih->display = 1;
	} else if (!text->pressed) {
	    text->pressed = 1;
	    uih->display = 1;
	}
    }
}

static void ui_unselectfile(struct dialogitem *item)
{
    struct ui_filedata *text = (struct ui_filedata *) item->data;
    if (text->active)
	text->active = 0, uih->display = 1;
    if (text->pressed)
	text->pressed = 0, uih->display = 1;
}

const static struct dialogtype filedialog = {
    ui_buildfile,
    ui_keyfile,
    ui_mousefile,
    ui_destroyfile,
    ui_drawfile,
    ui_unselectfile
};

static void
ui_buildfloat(struct dialogitem *item, const menudialog * entry)
{
    char s[50];
    item->height = BUTTONHEIGHT;
    item->width = xtextwidth(uih->image, uih->font, item->dialog->question);
    item->width1 = xtextcharw(uih->image, uih->font, 'w') * 10;
    sprintf(s, "%g", (double) item->dialog->deffloat);
    item->data = ui_opentext(0, 0, 2043, s);
}

static void ui_destroyfloat(struct dialogitem *item, dialogparam * param)
{
    struct ui_textdata *text = (struct ui_textdata *) item->data;
    param->number = ui_getfloat(text->text);
    ui_closetext(text);
}

const static struct dialogtype floatdialog = {
    ui_buildfloat,
    ui_keystring,
    ui_mousestring,
    ui_destroyfloat,
    ui_drawstring,
    ui_unselectstring
};

struct ui_coorddata {
    struct ui_textdata *text[2];
    int active;
};
static void
ui_buildcoord(struct dialogitem *item, const menudialog * entry)
{
    char s[50];
    struct ui_coorddata *data =
	(struct ui_coorddata *) malloc(sizeof(*data));
    item->height = BUTTONHEIGHT;
    item->width = xtextwidth(uih->image, uih->font, item->dialog->question);
    item->width1 = xtextcharw(uih->image, uih->font, 'w') * 20;
    item->data = data;
    data->active = 0;
    sprintf(s, "%g", (double) item->dialog->deffloat);
    data->text[0] = ui_opentext(0, 0, 2043, s);
    sprintf(s, "%g", (double) item->dialog->deffloat2);
    data->text[1] = ui_opentext(0, 0, 2043, s);
}

static void ui_destroycoord(struct dialogitem *item, dialogparam * param)
{
    struct ui_coorddata *data = (struct ui_coorddata *) item->data;
    param->dcoord[0] = ui_getfloat(data->text[0]->text);
    param->dcoord[1] = ui_getfloat(data->text[1]->text);
    ui_closetext(data->text[0]);
    ui_closetext(data->text[1]);
    free(data);
}

#define SPACESIZE xtextwidth(uih->image, uih->font,"+")
#define ENDSIZE xtextwidth(uih->image, uih->font,"i")
static void ui_drawcoord(struct dialogitem *item)
{
    struct ui_coorddata *data = (struct ui_coorddata *) item->data;
    int wholesize =
	dialog.width + dialog.x - dialog.half - 2 * BORDERWIDTH - ENDSIZE;
    int half = (wholesize - SPACESIZE) / 2;
    if (data->text[0]->width == 2043) {
	data->text[0]->x = dialog.half + BORDERWIDTH,
	    data->text[0]->width = half - BORDERWIDTH,
	    data->text[0]->y = item->y + BORDERHEIGHT;
	data->text[1]->x = dialog.half + half + SPACESIZE + BORDERWIDTH,
	    data->text[1]->width = half - BORDERWIDTH,
	    data->text[1]->y = item->y + BORDERHEIGHT;
	ui_updatetext(data->text[0]);
	ui_updatetext(data->text[1]);
    }
    uih_drawborder(uih, dialog.half, item->y, half, BUTTONHEIGHT,
		   BORDER_PRESSED | BORDER_LIGHT);
    uih_drawborder(uih, dialog.half + half + SPACESIZE, item->y, half,
		   BUTTONHEIGHT, BORDER_PRESSED | BORDER_LIGHT);
    ui_drawtext(data->text[0], SELECTED(item) && !data->active);
    ui_drawtext(data->text[1], SELECTED(item) && data->active);
    xprint(uih->image, uih->font, dialog.half + half,
	   item->y + BORDERHEIGHT, "+", FGCOLOR(uih),
	   BGCOLOR(uih), 0);
    xprint(uih->image, uih->font,
	   dialog.x + dialog.width - BORDERWIDTH - ENDSIZE,
	   item->y + BORDERHEIGHT, "i", FGCOLOR(uih),
	   BGCOLOR(uih), 0);
    ui_drawquestion(item);
}

static int ui_keycoord(struct dialogitem *item, int key)
{
    struct ui_coorddata *text = (struct ui_coorddata *) item->data;
    int i = ui_textkey(text->text[text->active], key);
    if (!i) {
	if (key == '\t' || key == UIKEY_RIGHT) {
	    text->active++;
	    if (text->active > 1) {
		text->active = 0;
		return 0;
	    }
	    uih->display = 1;
	    if (key == UIKEY_RIGHT) {
		text->text[1]->cursor = 0;
		ui_updatetext(text->text[1]);
		uih->display = 1;
	    }
	    return 1;
	}
	if (key == UIKEY_LEFT) {
	    text->active--;
	    if (text->active < 0) {
		text->active = 1;
		return 0;
	    }
	    text->text[0]->cursor = (int) strlen(text->text[0]->text);
	    ui_updatetext(text->text[0]);
	    uih->display = 1;
	    return 1;
	}
    }
    return (i);
}

static void
ui_mousecoord(struct dialogitem *item, int x, int y, int buttons,
	      int flags)
{
    struct ui_coorddata *text = (struct ui_coorddata *) item->data;
    int i;
    if (flags & MOUSE_MOVE) {
	if (x < text->text[1]->x)
	    i = 0;
	else
	    i = 1;
	if (text->active != i)
	    text->active = i, uih->display = 1;
    }
    if (flags & MOUSE_DRAG) {
	if (x < text->text[1]->x)
	    text->active = 0, ui_textmouse(text->text[0], x, y);
	else
	    text->active = 1, ui_textmouse(text->text[1], x, y);
    }
}

static void ui_unselectcoord(struct dialogitem *item)
{
    struct ui_coorddata *text = (struct ui_coorddata *) item->data;
    text->active = 0;
}

const static struct dialogtype coorddialog = {
    ui_buildcoord,
    ui_keycoord,
    ui_mousecoord,
    ui_destroycoord,
    ui_drawcoord,
    ui_unselectcoord
};

struct ui_choicedata {
    const char **texts;
    int selected;
    int n;

    struct uih_window *menu;
    int x, y, width, height;
    int active;
};
static void
ui_choicemenupos(struct uih_context *uih, int *x, int *y, int *width,
		 int *height, void *data)
{
    struct ui_choicedata *choice = (struct ui_choicedata *) data;
    if (filevisible || helpvisible) {
	*x = *y = *width = *height = 0;
	return;
    }
    *x = choice->x;
    *y = choice->y;
    *width = choice->width;
    *height = choice->height;
}

static void ui_drawchoicemenu(uih_context * uih, void *data)
{
    struct ui_choicedata *choice = (struct ui_choicedata *) data;
    int i;
    for (i = 0; i < choice->n; i++) {
	xprint(uih->image, uih->font, choice->x + BORDERWIDTH,
	       choice->y + BORDERHEIGHT + i * xtextheight(uih->image, uih->font),
	       choice->texts[i],
	       i == choice->active ? SELCOLOR(uih) : FGCOLOR(uih),
	       BGCOLOR(uih), 0);
    }
}

static void
ui_buildchoicemenu(struct uih_context *uih, struct ui_choicedata *choice,
		   int x, int y, int width)
{
    if (choice->menu != NULL)
	return;
    choice->width = width + 2 * BORDERWIDTH;
    choice->x = x;
    choice->height = xtextheight(uih->image, uih->font) * choice->n + 2 * BORDERHEIGHT;
    choice->active = choice->selected;
    choice->y = y - choice->active * xtextheight(uih->image, uih->font);
    dialog.mousegrab = 1;
    if (choice->x + choice->width > uih->image->width)
	choice->x = uih->image->width - choice->width;
    if (choice->y + choice->height > uih->image->height)
	choice->y = uih->image->height - choice->height;
    if (choice->x < 0)
	choice->x = 0;
    if (choice->y < 0)
	choice->y = 0;
    choice->menu =
	uih_registerw(uih, ui_choicemenupos, ui_drawchoicemenu, choice,
		      DRAWBORDER);
    uih->display = 1;
}

static void
ui_closechoicemenu(struct uih_context *uih, struct ui_choicedata *choice)
{
    if (choice->menu == NULL)
	return;
    uih_removew(uih, choice->menu);
    choice->menu = NULL;
    uih->display = 1;
    dialog.mousegrab = 0;
    dialog.mousereleased = 0;
}

static void
ui_buildchoice(struct dialogitem *item, const menudialog * entry)
{
    int i;
    struct ui_choicedata *data =
	(struct ui_choicedata *) malloc(sizeof(*data));
    item->height = BUTTONHEIGHT;
    item->width = xtextwidth(uih->image, uih->font, item->dialog->question);
    item->width1 = 0;
    data->menu = NULL;

    data->texts = (const char **) entry->defstr;
    for (i = 0; data->texts[i] != NULL; i++) {
	int w = xtextwidth(uih->image, uih->font, data->texts[i]);
	if (w > item->width1)
	    item->width1 = w;
    }
    item->width1 += 2 * BORDERWIDTH;
    data->n = i;
    data->selected = entry->defint;
    item->data = data;
}

static void ui_destroychoice(struct dialogitem *item, dialogparam * param)
{
    struct ui_choicedata *data = (struct ui_choicedata *) item->data;
    param->dint = data->selected;
    ui_closechoicemenu(uih, data);
    free(data);
}

static void ui_drawchoice(struct dialogitem *item)
{
    struct ui_choicedata *data = (struct ui_choicedata *) item->data;
    uih_drawborder(uih, dialog.half, item->y, item->width1,
		   BUTTONHEIGHT | BORDER_LIGHT, 0);
    xprint(uih->image, uih->font, dialog.half + BORDERWIDTH,
	   item->y + BORDERHEIGHT, data->texts[data->selected],
	   SELECTED(item) ? SELCOLOR(uih) : FGCOLOR(uih),
	   BGCOLOR(uih), 0);
    ui_drawquestion(item);
}

static int ui_keychoice(struct dialogitem *item, int key)
{
    struct ui_choicedata *data = (struct ui_choicedata *) item->data;
    if (!data->menu) {
	switch (key) {
	case ' ':
	case '\n':
	case 13:
	case UIKEY_UP:
	case UIKEY_DOWN:
	case UIKEY_RIGHT:
	    ui_buildchoicemenu(uih, data, dialog.half, item->y,
			       item->width1);
	    return (1);
	}
    } else {
	switch (key) {
	case ' ':
	case 13:
	case '\n':
	    data->selected = data->active;
	    ui_closechoicemenu(uih, data);
	    return (1);
	case UIKEY_DOWN:
	    data->active++;
	    data->active %= data->n;
	    uih->display = 1;
	    return (1);
	case UIKEY_UP:
	    data->active--;
	    if (data->active < 0)
		data->active = data->n - 1;
	    uih->display = 1;
	    return (1);
	case UIKEY_ESC:
	    ui_closechoicemenu(uih, data);
	    return (1);
	}
    }
    return 0;
}

static void
ui_mousechoice(struct dialogitem *item, int x, int y, int buttons,
	       int flags)
{
    struct ui_choicedata *data = (struct ui_choicedata *) item->data;
    int in;
    if (data->menu != NULL) {
	in = 0;
	if (x > data->x && y > data->y && x < data->x + data->width
	    && y < data->y + data->height)
	    in = 1;
	if ((flags & MOUSE_PRESS) && !in) {
	    ui_closechoicemenu(uih, data);
	    return;
	}
	if ((flags & MOUSE_MOVE) && in) {
	    in = (y - data->y) / xtextheight(uih->image, uih->font);
	    if (in < 0)
		in = 0;
	    if (in >= data->n)
		in = data->n - 1;
	    if (data->active != in)
		data->active = in, uih->display = 1;
	}
	if (flags & MOUSE_RELEASE) {
	    data->selected = data->active;
	    ui_closechoicemenu(uih, data);
	    return;
	}
    } else {
	if (flags & MOUSE_PRESS && x > dialog.half)
	    ui_buildchoicemenu(uih, data, dialog.half, item->y,
			       item->width1);
    }
}

static void ui_unselectchoice(struct dialogitem *item)
{
    struct ui_choicedata *data = (struct ui_choicedata *) item->data;
    ui_closechoicemenu(uih, data);
}

const static struct dialogtype choicedialog = {
    ui_buildchoice,
    ui_keychoice,
    ui_mousechoice,
    ui_destroychoice,
    ui_drawchoice,
    ui_unselectchoice
};

static void
ui_dialogpos(struct uih_context *c, int *x, int *y, int *width,
	     int *height, void *data)
{
    *x = dialog.x;
    *y = dialog.y;
    if (filevisible || helpvisible) {
	*x = *y = *width = *height = 0;
	return;
    }
    *width = dialog.width;
    *height = dialog.height;
}

static void ui_dialogdraw(struct uih_context *c, void *data)
{
    int n;
    for (n = 0; n < dialog.nitems; n++)
	dialog.items[n].type->draw(dialog.items + n);
}

#define YSKIP 2
void ui_builddialog(const menuitem * item)
{
    int n = 2;
    int ypos;
    int width1 = 0;
    if (ui_nogui) {
	printf("dialog \"%s\"\n", item->shortname);
	return;
    }
    if (driver->gui_driver && driver->gui_driver->dialog) {
	driver->gui_driver->dialog(uih, item->shortname);
	return;
    }
    dialogvisible = 1;
    dialog.width = 0;
    dialog.height = 0;
    dialog.dialog = menu_getdialog(uih, item);
    dialog.item = item;
    for (n = 0; dialog.dialog[n].question != NULL; n++);
    n++;
    dialog.nitems = n;
    dialog.items =
	(struct dialogitem *) malloc(sizeof(struct dialogitem) *
				     dialog.nitems);
    dialog.mousereleased = 0;
    dialog.items[dialog.nitems - 1].type = &okdialog;
    for (n = 0; n < dialog.nitems; n++) {
	if (n < dialog.nitems - 1) {
	    switch (dialog.dialog[n].type) {
	    case DIALOG_STRING:
	    case DIALOG_KEYSTRING:
		dialog.items[n].type = &stringdialog;
		break;
	    case DIALOG_INT:
		dialog.items[n].type = &intdialog;
		break;
	    case DIALOG_IFILE:
	    case DIALOG_OFILE:
		dialog.items[n].type = &filedialog;
		break;
	    case DIALOG_FLOAT:
		dialog.items[n].type = &floatdialog;
		break;
	    case DIALOG_COORD:
		dialog.items[n].type = &coorddialog;
		break;
	    case DIALOG_CHOICE:
		dialog.items[n].type = &choicedialog;
		break;
	    default:
		printf("uidialog:unknown type!\n");
		exit(1);
	    }
	}
	dialog.items[n].dialog = dialog.dialog + n;
	dialog.items[n].type->build(dialog.items + n, dialog.dialog + n);
	dialog.height += dialog.items[n].height;
	if (width1 < dialog.items[n].width1)
	    width1 = dialog.items[n].width1;
	if (dialog.width < dialog.items[n].width)
	    dialog.width = dialog.items[n].width;
    }
    dialog.height += YSKIP * (n - 1);
    n = xtextwidth(uih->image, uih->font, gettext("OK")) + xtextwidth(uih->image, uih->font,
							  gettext
							  ("Cancel")) +
	xtextwidth(uih->image, uih->font, gettext("Help")) + 10;
    if (dialog.width < n)
	dialog.width = n;
    dialog.half = dialog.width + 2 * BORDERWIDTH;
    dialog.width += 2 * BORDERWIDTH + width1;
    dialog.height += 2 * BORDERHEIGHT;
    dialog.current = 0;
    dialog.x = (uih->image->width - dialog.width) / 2;
    dialog.half += dialog.x;
    dialog.y = (uih->image->height - dialog.height) / 2;
    ypos = dialog.y + BORDERHEIGHT;
    for (n = 0; n < dialog.nitems; n++) {
	dialog.items[n].y = ypos;
	ypos += dialog.items[n].height + YSKIP;
    }
    dialog.window =
	uih_registerw(uih, ui_dialogpos, ui_dialogdraw, NULL, DRAWBORDER);
    uih->display = 1;
}

static void ui_dialogquestion(int succesfull)
{
    if (succesfull)
	ui_menuactivate(qitem, qparam);
    else
	menu_destroydialog(qitem, qparam, uih);
}

void ui_closedialog(int succesfull)
{
    int n = 2;
    if (dialogvisible) {
	dialogparam *param =
	    (dialogparam *) malloc(sizeof(dialogparam) * (dialog.nitems));
	dialogvisible = 0;
	uih_removew(uih, dialog.window);
	uih->display = 1;
	for (n = 0; n < dialog.nitems; n++) {
	    dialog.items[n].type->destroy(dialog.items + n, param + n);
	}
	free(dialog.items);
	if (succesfull) {
	    for (n = 0; n < dialog.nitems - 1; n++)
		if (dialog.dialog[n].type == DIALOG_OFILE
		    && xio_exist(param[n].dpath)) {
		    qparam = param;
		    qitem = dialog.item;
		    ui_buildyesno("File exist. Overwrite?",
				  ui_dialogquestion);
		    return;
		}
	    ui_menuactivate(dialog.item, param);
	} else
	    menu_destroydialog(dialog.item, param, uih);
    }
}

int ui_dialogmouse(int x, int y, int mousebuttons, int flags)
{
    int i;
    if (ui_mouseyesno(x, y, mousebuttons, flags))
	return 1;
    if (!dialogvisible)
	return 0;
    if (!dialog.mousereleased && (flags & MOUSE_RELEASE)) {
	dialog.mousereleased = 1;
	return 1;
    }
    if (!dialog.mousereleased && (flags & MOUSE_DRAG)) {
	return 1;
    }
    dialog.mousereleased = 1;
    if (dialog.mousegrab) {
	dialog.items[dialog.current].type->mouse(dialog.items +
						 dialog.current, x, y,
						 mousebuttons, flags);
	return 1;
    }
    if (dialog.x > x || dialog.y > y || dialog.x + dialog.width < x
	|| dialog.y + dialog.height < y) {
	if (flags & MOUSE_PRESS) {
	    ui_closedialog(0);
	    return 1;
	}
	dialog.items[dialog.current].type->mouse(dialog.items +
						 dialog.current, x, y, 0,
						 0);
	return 1;
    }
    for (i = dialog.nitems - 1; i >= 0; i--)
	if (dialog.items[i].y < y)
	    break;
    if (i == -1) {
	dialog.items[dialog.current].type->mouse(dialog.items +
						 dialog.current, x, y, 0,
						 0);
	return 1;
    }
    if (((flags & MOUSE_PRESS) || (flags & MOUSE_MOVE))
	&& dialog.current != i) {
	dialog.items[dialog.current].type->unselect(dialog.items +
						    dialog.current);
	dialog.current = i;
	uih->display = 1;
    }
    dialog.items[i].type->mouse(dialog.items + i, x, y, mousebuttons,
				flags);
    return 1;
}

int ui_dialogkeys(int key)
{
    if (ui_keyyesno(key))
	return 1;
    if (!dialogvisible)
	return 0;
    if (key == UIKEY_ESC) {
	ui_closedialog(0);
	return 1;
    }
    if (!dialog.items[dialog.current].type->
	key(dialog.items + dialog.current, key)) {
	switch (key) {
	case 'h':
	    ui_help(dialog.item->shortname);
	    return 1;
	case UIKEY_TAB:
	case UIKEY_DOWN:
	case UIKEY_RIGHT:
	    NEXT();
	    break;
	case UIKEY_UP:
	case UIKEY_LEFT:
	    PREV();
	    break;
	case 13:
	case '\n':
	    ui_closedialog(1);
	}
    }
    return 1;
}
