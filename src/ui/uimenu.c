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
#ifdef _plan9_
#include <u.h>
#include <libc.h>
#include <ctype.h>
#else
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#endif
#include <config.h>
#include <fconfig.h>
#ifndef _plan9_
#include <assert.h>
#endif
#include <filter.h>
#include <ui_helper.h>
#include <ui.h>
#include <timers.h>
#include <xmenu.h>
#include <grlib.h>
#include "uiint.h"


#define NMENUS 20
struct ui_menuitems {
    int x, y, width, height;
    const menuitem *item;
    int separator;
};
static struct ui_menu {
    int x, y, width, height;
    const char *name;
    const char *fullname;
    int namewidth;
    int selected;
    int n;
    int flags;
    struct ui_menuitems *items;
    struct uih_window *window;
    tl_timer *timer;
} *ui_menus[NMENUS];
int ui_nmenus;

#define MENU_HORIZONTAL 1
#define MENU_PRESSED 2
#define MENU_AUTOHIDE 4

#define SUBMENUWIDTH xtextwidth(uih->image, uih->font, ">")
#define MENUPAUSE xtextwidth(uih->image, uih->font, "X")
#define MENUWIDTH(a) (xtextwidth(uih->image, uih->font, "w")+MENUPAUSE+xtextwidth(uih->image, uih->font, a)+SUBMENUWIDTH)
#define HMENUWIDTH(a) (xtextwidth(uih->image, uih->font, a)+xtextwidth(uih->image, uih->font, "  "))

#define SEPARATORSIZE 6
static struct ui_menuitems *ui_getmenuitems(const char *name, int *width1,
					    int *height1, int *n1,
					    int horizontal)
{
    const menuitem *item;
    int nseparators = 0;
    int i;
    int width = 0;
    int n;
    struct ui_menuitems *items;
    for (n = 0; (item = menu_item(name, n)) != NULL; n++)
	if (item->type == MENU_SEPARATOR)
	    nseparators++;
    n -= nseparators;
    *n1 = n;
    *height1 = n * xtextheight(uih->image, uih->font) + nseparators * SEPARATORSIZE;
    items =
	(struct ui_menuitems *) malloc(n * sizeof(struct ui_menuitems));
    nseparators = 0;
    for (i = 0; i < n; i++) {
	int w;
	int sbehind = -1;
	nseparators--;
	do {
	    nseparators++;
	    sbehind++;
	    item = menu_item(name, i + nseparators);
	}
	while (item->type == MENU_SEPARATOR);
	items[i].item = item;
	items[i].separator = sbehind;
	if (horizontal) {
	    w = HMENUWIDTH(items[i].item->name);
	    if (items[i].item->key) {
		char c[10];
		sprintf(c, "(%s)", items[i].item->key);
		w += xtextwidth(uih->image, uih->font, c);
	    }
	} else {
	    w = MENUWIDTH(items[i].item->name);
	    if (items[i].item->key) {
		char c[10];
		sprintf(c, " %s ", items[i].item->key);
		w += xtextwidth(uih->image, uih->font, c);
	    }
	}
	items[i].width = w;
	items[i].height = xtextheight(uih->image, uih->font) + 1;
	if (w > width)
	    width = w;
    }
    *width1 = width;
    return (items);
}

static void
ui_menusize(uih_context * c, int *x, int *y, int *w, int *h, void *data)
{
    struct ui_menu *m = (struct ui_menu *) data;
    *x = m->x;
    *y = m->y;
    *w = m->width;
    *h = m->height;
}

static void ui_drawmenu(uih_context * c, void *data)
{
    struct ui_menu *m = (struct ui_menu *) data;
    int i;
    int width1 = xtextwidth(c->image, c->font, "w");
    char s[2];
    s[1] = 0;
    if (!(m->flags & MENU_HORIZONTAL))
	xprint(c->image, c->font, m->x + (m->width - m->namewidth) / 2,
	       m->y + BORDERWIDTH, m->fullname, SELCOLOR(c),
	       BGCOLOR(c), 0);
    for (i = 0; i < m->n; i++) {
	int color = (i == m->selected ? SELCOLOR(c) : FGCOLOR(c));
	int pressed = 0;
	if ((uih->palette->type & BITMAPS) && i == m->selected) {
	    pressed = TEXT_PRESSED;
	    color = BGCOLOR(c);
	    xrectangle(uih->image, m->items[i].x, m->items[i].y,
		       m->items[i].width, m->items[i].height, FGCOLOR(c));
	}
	if (!(m->flags & MENU_HORIZONTAL)) {
	    if (m->items[i].separator) {
		xhline(c->image, m->x + 5,
		       m->items[i].y - 2 - SEPARATORSIZE / 2,
		       m->width - 10, BGCOLOR(c));
		xhline(c->image, m->x + 5,
		       m->items[i].y - 1 - SEPARATORSIZE / 2,
		       m->width - 10, LIGHTGRAYCOLOR(c));
	    }
	    if (i < 10)
		s[0] = '0' + (i == 9 ? 0 : i + 1);
	    else
		s[0] = 'A' + (i - 10);
	    xprint(c->image, c->font, m->items[i].x, m->items[i].y, s,
		   color, BGCOLOR(c), pressed);
	    if (menu_enabled(m->items[i].item, uih)) {
		xprint(c->image, c->font, m->items[i].x + width1,
		       m->items[i].y, "X", color, BGCOLOR(c),
		       pressed);
	    }
	    xprint(c->image, c->font, m->items[i].x + width1 + MENUPAUSE,
		   m->items[i].y, m->items[i].item->name,
		   color, BGCOLOR(c), pressed);
	    if (m->items[i].item->key) {
		char ch[20];
		sprintf(ch, " %s ", m->items[i].item->key);
		xprint(c->image, c->font,
		       m->items[i].x + m->items[i].width - SUBMENUWIDTH -
		       xtextwidth(uih->image, uih->font, ch), m->items[i].y, ch,
		       LIGHTGRAYCOLOR(c), BGCOLOR(c),
		       pressed);
	    }
	    if (m->items[i].item->type == MENU_SUBMENU)
		xprint(c->image, c->font,
		       m->items[i].x + m->items[i].width - SUBMENUWIDTH,
		       m->items[i].y, ">", color, BGCOLOR(c),
		       pressed);
	} else {
	    xprint(c->image, c->font, m->items[i].x, m->items[i].y,
		   m->items[i].item->name, color, BGCOLOR(c),
		   pressed);
	    if (m->items[i].item->key) {
		char ch[20];
		sprintf(ch, "%s", m->items[i].item->key);
		xprint(c->image, c->font,
		       m->items[i].x + xtextwidth(uih->image, uih->font,
						  m->items[i].item->name) +
		       2, m->items[i].y, ch,
		       LIGHTGRAYCOLOR(c), BGCOLOR(c), pressed);
	    }
	}
    }
}

static struct ui_menu *ui_buildmenu(const char *name, int x, int y,
				    int flags)
{
    int shift = 0;
    int width, height;
    int textheight = xtextheight(uih->image, uih->font);
    struct ui_menu *menu;
    int i;
    menu = (struct ui_menu *) malloc(sizeof(*menu));
    menu->timer = tl_create_timer();
    tl_reset_timer(menu->timer);
    menu->flags = flags;
    menu->items =
	ui_getmenuitems(name, &width, &height, &menu->n,
			flags & MENU_HORIZONTAL);
    menu->selected = -1;
    menu->fullname = menu_fullname(name);
    menu->name = name;
    menu->namewidth = xtextwidth(uih->image, uih->font, menu->fullname);
    if (!(menu->flags & MENU_HORIZONTAL)) {
	if (menu->namewidth > width)
	    width = menu->namewidth;
	width += 2 * BORDERWIDTH;
	height += 2 * BORDERHEIGHT + xtextheight(uih->image, uih->font);
	if (x + width > uih->image->width)
	    x = uih->image->width - width;
	if (y + height > uih->image->height)
	    y = uih->image->height - height;
	if (x < 0)
	    x = 0;
	if (y < 0)
	    y = 0;
	shift = 0;
	for (i = 0; i < menu->n; i++) {
	    shift += menu->items[i].separator * SEPARATORSIZE;
	    menu->items[i].x = x + BORDERWIDTH;
	    menu->items[i].y =
		y + BORDERWIDTH + textheight * (i + 1) + shift;
	    menu->items[i].width = width - 2 * BORDERWIDTH;
	    menu->items[i].height = textheight;
	}
    } else {
	int line = 0;
	int xpos = BORDERWIDTH;
	x = 0, width = uih->image->width;
	for (i = 0; i < menu->n; i++) {
	    if (xpos + 2 * BORDERWIDTH + menu->items[i].width >
		uih->image->width)
		xpos = BORDERWIDTH, line++;
	    menu->items[i].x = xpos;
	    menu->items[i].y = y + BORDERWIDTH + line * textheight;
	    xpos += menu->items[i].width;
	    menu->items[i].height = textheight;
	}
	height = (line + 1) * textheight + 2 * BORDERWIDTH;
    }
    menu->selected = -1;
    menu->window =
	uih_registerw(uih, ui_menusize, ui_drawmenu, menu, DRAWBORDER);
    uih->display = 1;
    menu->x = x;
    menu->y = y;
    menu->width = width;
    menu->height = height;
    return (menu);
}

static void ui_closemenu(struct ui_menu *menu)
{
    free(menu->items);
    tl_free_timer(menu->timer);
    uih_removew(uih, menu->window);
    uih->display = 1;
    free(menu);
}

static void ui_openmenu(const char *name, int x, int y, int flags)
{
    if (ui_nogui) {
	printf("menu \"%s\"\n", name);
	return;
    }
    if (driver->gui_driver && driver->gui_driver->menu) {
	driver->gui_driver->menu(uih, name);
	return;
    }
    if (ui_nmenus > NMENUS)
	return;
    ui_menus[ui_nmenus] = ui_buildmenu(name, x, y, flags);
    ui_nmenus++;
    ui_updatestarts();
}

static void ui_closetopmenu(void)
{
    if (!ui_nmenus)
	return;
    ui_nmenus--;
    ui_closemenu(ui_menus[ui_nmenus]);
    ui_updatestarts();
}

void ui_closemenus(void)
{
    while (ui_nmenus)
	ui_closetopmenu();
}

void ui_menu(const char *m)
{
    int mousex, mousey, buttons;
    driver->getmouse(&mousex, &mousey, &buttons);
    ui_openmenu(m, mousex, mousey, 0);
}

static void ui_menupress(int number)
{
    const menuitem *item;
    if (number >= ui_menus[ui_nmenus - 1]->n)
	return;
    ui_menus[ui_nmenus - 1]->selected = number;
    item = ui_menus[ui_nmenus - 1]->items[number].item;
    if (item != NULL) {
	uih->display = 1;
	if (item->type == MENU_SUBMENU) {
	    int flags = 0;
	    int mousex, mousey, buttons;
	    driver->getmouse(&mousex, &mousey, &buttons);
	    if (buttons & BUTTON1)
		flags |= MENU_PRESSED;
	    if ((ui_menus[ui_nmenus - 1]->flags & MENU_HORIZONTAL))
		ui_openmenu(item->shortname,
			    ui_menus[ui_nmenus - 1]->items[number].x,
			    ui_menus[ui_nmenus - 1]->items[number].y +
			    ui_menus[ui_nmenus - 1]->items[number].height,
			    flags);
	    else
		ui_openmenu(item->shortname,
			    ui_menus[ui_nmenus - 1]->items[number].x +
			    ui_menus[ui_nmenus - 1]->items[number].width,
			    ui_menus[ui_nmenus - 1]->items[number].y,
			    flags);
	} else
	    ui_menuactivate(item, NULL);
    }
}

int ui_menumouse(int x, int y, int mousebuttons, int flags)
{
    if (ui_nmenus) {
	struct ui_menu *m = ui_menus[ui_nmenus - 1];
	int place = -1;
	int inmenu = 0;
	if (x >= m->x && y >= m->y && x <= m->x + m->width
	    && y <= m->y + m->height) {
	    int i;
	    for (i = 0; i < m->n; i++) {
		if (x >= m->items[i].x && y >= m->items[i].y
		    && x <= m->items[i].x + m->items[i].width
		    && y <= m->items[i].y + m->items[i].height) {
		    place = i;
		    break;
		}
	    }
	    inmenu = 1;
	} else {
	    if (ui_nmenus > 1) {
		struct ui_menu *m2 = ui_menus[ui_nmenus - 2];
		int i;
		i = m2->selected;
		if (x >= m2->items[i].x && y >= m2->items[i].y
		    && x <= m2->items[i].x + m2->items[i].width
		    && y <= m2->items[i].y + m2->items[i].height)
		    inmenu = 1;
	    }
	}
	if ((m->flags & MENU_AUTOHIDE) && !inmenu) {
	    ui_closetopmenu();
	    return (ui_menumouse(x, y, mousebuttons, flags));
	}
	if (flags & MOUSE_MOVE && m->selected != place)
	    m->selected = place, uih->display = 1;
	if (m->flags & MENU_PRESSED) {
	    if (inmenu && place >= 0 && (m->flags & MENU_HORIZONTAL)
		&& (flags & MOUSE_DRAG)
		&& m->items[place].item->type == MENU_SUBMENU) {
		ui_menupress(place);
		return 1;
	    } else if (inmenu && place >= 0 && (flags & MOUSE_DRAG)
		       && m->items[place].item->type == MENU_SUBMENU
		       && x >
		       m->items[place].x + m->items[place].width -
		       2 * SUBMENUWIDTH) {
		ui_menupress(place);
		return 1;
	    }
	    if (flags & MOUSE_RELEASE || !(flags & MOUSE_DRAG)) {
		if (tl_lookup_timer(m->timer) < 300000) {
		    m->flags &= ~MENU_PRESSED;
		    return 1;
		}
		if (!inmenu || place < 0) {
		    ui_closetopmenu();
		    return (ui_menumouse(x, y, mousebuttons, flags));
		}
		ui_menupress(place);
		return 1;
	    }
	    if (!inmenu) {	/*Trace all menus back and look, if user selected some */
		int nmenu;
		for (nmenu = ui_nmenus - 2; nmenu > -1; nmenu--) {
		    struct ui_menu *m2 = ui_menus[nmenu];
		    if (x > m2->x && y > m2->y && x < m2->x + m2->width
			&& y < m2->y + m2->height) {
			ui_closetopmenu();
			m2->flags |= MENU_PRESSED;
			return (ui_menumouse(x, y, mousebuttons, flags));
		    }
		}
	    }
	} else if (flags & MOUSE_PRESS) {
	    if (!inmenu || place < 0) {
		ui_closetopmenu();
		return (ui_menumouse(x, y, mousebuttons, flags));
	    }
	    ui_menupress(place);
	}
	return (1);
    } else {
	if (!ui_nogui &&
	    (!driver->gui_driver || !driver->gui_driver->setrootmenu) &&
	    (flags & MOUSE_MOVE) && y < xtextheight(uih->image, uih->font) + 1
	    && !(mousebuttons))
	    ui_openmenu(uih->menuroot, 0, 0,
			MENU_HORIZONTAL | MENU_AUTOHIDE);
    }
    return (0);
}

int ui_menukey(int key)
{
    int k;
    if (!ui_nmenus) {
	if (key == '\n' || key == 13) {
	    ui_closemenus();
	    ui_openmenu(uih->menuroot, 0, 0, MENU_HORIZONTAL);
	    return 1;
	}
	return 0;
    } else {
	struct ui_menu *menu = ui_menus[ui_nmenus - 1];
	switch (key) {
	case '\n':
	case 13:
	    if (menu->selected >= 0)
		ui_menupress(menu->selected);
	    return 1;
	case 'h':
	    {
		const menuitem *item = menu->items[menu->selected].item;
		ui_closemenus();
		if (menu->selected >= 0) {
		    ui_help(item->shortname);
		} else
		    ui_help(menu->name);
		return 1;
	    }
	case UIKEY_LEFT:
	    if (menu->flags & MENU_HORIZONTAL) {
		if (menu->selected == -1)
		    menu->selected = 0;
		else
		    menu->selected--;
		if (menu->selected < 0)
		    menu->selected = menu->n - 1;
		uih->display = 1;
	    } else if (ui_nmenus == 2
		       && ui_menus[0]->flags & MENU_HORIZONTAL) {
		ui_closetopmenu();
		ui_menus[0]->selected--;
		if (ui_menus[0]->selected < 0)
		    ui_menus[0]->selected = ui_menus[0]->n - 1;
		ui_menupress(ui_menus[0]->selected);
	    } else
		ui_closetopmenu();
	    return 1;
	case UIKEY_RIGHT:
	    if (menu->flags & MENU_HORIZONTAL) {
		if (menu->selected == -1)
		    menu->selected = 0;
		else
		    menu->selected++;
		menu->selected %= menu->n;
		uih->display = 1;
	    } else if (menu->selected >= 0
		       && menu->items[menu->selected].item->type ==
		       MENU_SUBMENU) {
		ui_menupress(menu->selected);
	    } else if (ui_nmenus == 2
		       && ui_menus[0]->flags & MENU_HORIZONTAL) {
		ui_closetopmenu();
		ui_menus[0]->selected++;
		ui_menus[0]->selected %= ui_menus[0]->n;
		ui_menupress(ui_menus[0]->selected);
	    }
	    return 1;
	case UIKEY_DOWN:
	    if (menu->flags & MENU_HORIZONTAL) {
		if (menu->selected >= 0)
		    ui_menupress(ui_menus[0]->selected);
	    } else {
		if (menu->selected == -1)
		    menu->selected = 0;
		else
		    menu->selected++;
		menu->selected %= menu->n;
		uih->display = 1;
	    }
	    return 1;
	case UIKEY_ESC:
	    ui_closetopmenu();
	    return 1;
	case UIKEY_UP:
	    if (menu->flags & MENU_HORIZONTAL) {
		ui_closetopmenu();
	    } else {
		if (menu->selected == -1)
		    menu->selected = 0;
		else
		    menu->selected--;
		if (menu->selected < 0)
		    menu->selected = menu->n - 1;
		uih->display = 1;
	    }
	    return 1;
	}
	if (tolower(key) >= 'a'
	    && tolower(key) - 'a' < ui_menus[ui_nmenus - 1]->n - 10) {
	    ui_menupress(tolower(key) - 'a' + 10);
	    return 1;
	}
	if (key >= '0' && key <= '9') {
	    k = key - '1';
	    if (k == -1)
		k = 9;
	    ui_menupress(k);
	    return 1;
	}
	return 0;
    }
}

int ui_menuwidth(void)
{
    if (ui_nmenus && (ui_menus[0]->flags & MENU_HORIZONTAL))
	return (ui_menus[0]->height);
    return (0);
}
