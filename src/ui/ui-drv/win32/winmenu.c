#include <windows.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <config.h>
/*#include <filter.h>
   #include <ui_helper.h> */
#include <xerror.h>
#include <ui.h>
#include <xmenu.h>
#include "ui_win32.h"
#define IDMUL 64
#define MAIN 0
#define POPUP 1
#define WSUBMENU 2
struct menurecord {
    int type;
    int id;
    struct uih_context *c;
    CONST menuitem *item;
    struct menurecord *nextrecord;
    struct menurecord *prevrecord;
    HMENU menu;
    struct menurecord *submenu;
    struct menurecord *nextsubmenu;
} *firstrecord;
struct menurecord rootmenu;

static void win32_appendmenu(struct uih_context *c,
			     struct menurecord *menu);
HMENU win32_createrootmenu()
{
    /*printf("Createrootmenu\n"); */
    rootmenu.id = 0;
    rootmenu.menu = CreateMenu();
    rootmenu.nextrecord = firstrecord;
    rootmenu.prevrecord = NULL;
    firstrecord = &rootmenu;
    rootmenu.nextsubmenu = NULL;
    rootmenu.submenu = NULL;
    /*printf("Createrootmenu OK\n"); */
    return rootmenu.menu;
};

static struct menurecord *win32_createmenu(struct uih_context *c,
					   CONST char *name, int type)
{
    struct menurecord *m = calloc(sizeof(*m), 1);
    static int id = 1;
    /*printf("Createmenu %s\n",name); */
    m->id = id++;
    m->type = type;
    if (type != POPUP)
	m->menu = CreateMenu();
    else
	m->menu = CreatePopupMenu();
    m->c = c;
    m->item = menu_findcommand(name);
    m->nextrecord = firstrecord;
    m->prevrecord = NULL;
    firstrecord = m;
    m->nextsubmenu = NULL;
    m->submenu = NULL;
    win32_appendmenu(c, m);
    /*printf("Createmenu OK\n"); */
    return m;
};

static void
win32_appendmenu(struct uih_context *c, struct menurecord *menu)
{
    int i, y, hotkeyed;
    CONST menuitem *item;
    char out[256];
    char used[256];
    memset(used, 0, 256);
    /*printf("Appendmenu %s\n",menu->item->shortname); */
    for (i = 0; (item = menu_item(menu->item->shortname, i)) != NULL; i++) {
	struct menurecord *submenu = NULL;
	int flags = MF_ENABLED | MF_STRING;
	if (item->type == MENU_SEPARATOR)
	    flags |= MF_SEPARATOR;
	if (item->type == MENU_SUBMENU) {
	    submenu = win32_createmenu(c, item->shortname, WSUBMENU);
	    flags |= MF_POPUP;
	    submenu->nextsubmenu = menu->submenu;
	    menu->submenu = submenu;
	}
	if (item->flags & (MENUFLAG_RADIO | MENUFLAG_CHECKBOX) &&
	    menu_enabled(item, c))
	    flags |= MF_CHECKED;
	/*printf("   %s\n",item->name); */

	hotkeyed = 0;
	for (y = 0; item->name[y]; y++) {
	    if (!hotkeyed) {
		unsigned char c = tolower(item->name[y]);
		if (((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'))
		    && !used[c])
		    used[c] = 1, hotkeyed = 1, out[y] = '&';
	    }
	    out[y + hotkeyed] = item->name[y];
	}
	out[y + hotkeyed] = 0;
	if (item->type == MENU_CUSTOMDIALOG || item->type == MENU_DIALOG)
	    strcat(out, "...");
	if (menu->type != MAIN && item->key) {
	    strcat(out, "\t");
	    strcat(out, item->key);
	}
	AppendMenu(menu->menu, flags,
		   (submenu !=
		    NULL ? (UINT) submenu->menu : (UINT) (menu->id *
							  IDMUL + i)),
		   out);

    }
    /*printf("Appendmenu OK\n"); */
}

static void win32_freestructures(struct menurecord *menu)
{
    struct menurecord *sub, *nextsub;
    /*printf("Freestructures\n"); */
    sub = menu->submenu;
    while (sub != NULL) {
	nextsub = sub->nextsubmenu;
	win32_freestructures(sub);
	sub = nextsub;
    }
    if (menu->nextrecord)
	menu->nextrecord->prevrecord = menu->prevrecord;
    if (menu->prevrecord)
	menu->prevrecord->nextrecord = menu->nextrecord;
    else
	firstrecord = menu->nextrecord;
    free(menu);
}

void win32_pressed(int id)
{
    struct menurecord *menu = firstrecord;
    /*printf("Pressed\n"); */
    while (menu) {
	if (id / IDMUL == menu->id) {
	    ui_menuactivate(menu_item(menu->item->shortname, id % IDMUL),
			    NULL);
	    return;
	}
	menu = menu->nextrecord;
    }
    x_error("Unknown menu");
}

/* first destroy the old contents */
void win32_dorootmenu(struct uih_context *uih, CONST char *name)
{
    struct menurecord *sub, *nextsub;
    /*printf("dorootmenu %s\n",name); */
    /* Delete the old entries */
    while (DeleteMenu(rootmenu.menu, 0, MF_BYPOSITION));
    sub = rootmenu.submenu;
    rootmenu.c = uih;
    while (sub != NULL) {
	nextsub = sub->nextsubmenu;
	win32_freestructures(sub);
	sub = nextsub;
    }
    rootmenu.item = menu_findcommand(name);
    rootmenu.submenu = NULL;
    win32_appendmenu(uih, &rootmenu);
    SetMenu(hWnd, rootmenu.menu);
    /*printf("OK\n"); */
}

/* Check all created menus to see if there is the changed item and do changes
   as neccesary.

   This implementation is rather ugly and slow. Try if it is fast enought
   and change it otherwise */
void win32_enabledisable(struct uih_context *uih, CONST char *name)
{
    CONST struct menuitem *chgitem = menu_findcommand(name);
    CONST struct menuitem *item;
    struct menurecord *menu = firstrecord;
    int i;
    int checked = menu_enabled(chgitem, uih);
    while (menu) {
	if (menu->item != NULL)
	    for (i = 0;
		 (item = menu_item(menu->item->shortname, i)) != NULL;
		 i++) {
		if (item == chgitem) {
		    if (chgitem->flags & MENUFLAG_RADIO && checked) {
			int y;
			for (y = 0;
			     (item =
			      menu_item(menu->item->shortname, y)) != NULL;
			     y++)
			    if (item->flags & MENUFLAG_RADIO)
				CheckMenuItem(menu->menu,
					      menu->id * IDMUL + y,
					      MF_BYCOMMAND | MF_UNCHECKED);
		    }
		    CheckMenuItem(menu->menu, menu->id * IDMUL + i,
				  MF_BYCOMMAND | (checked ? MF_CHECKED :
						  MF_UNCHECKED));
		}
	    }
	menu = menu->nextrecord;
    }
}

void win32_menu(struct uih_context *c, CONST char *name)
{

    POINT p;
    struct menurecord *m = firstrecord;

    /*Delete records about all popups, since they are closed now */
    while (m) {
	if (m->type == POPUP) {
	    DestroyMenu(m->menu), win32_freestructures(m);
	    break;
	}
	m = m->nextrecord;
    }
    m = win32_createmenu(c, name, POPUP);
    GetCursorPos(&p);
    /*printf("menu %s %i %i\n",name,p.x,p.y); */
    TrackPopupMenu(m->menu, 0, p.x, p.y, 0, hWnd, 0);
}

void win32_uninitializewindows()
{
    while (firstrecord) {
	struct menurecord *r = firstrecord;
	while (r->type == WSUBMENU)
	    r = r->nextrecord;
	DestroyMenu(r->menu);
	win32_freestructures(r);
    }
}
