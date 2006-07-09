/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright © 1996,1997 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *	
 *	Mac OS X Driver by J.B. Langston (jb-langston at austin dot rr dot com)
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
#include "config.h"
#include "ui.h"
#include "xmenu.h"

#include <Carbon/Carbon.h>

struct menurecord
{
	MenuRef menu;
	
	int type;
	int id;
	struct uih_context *c;
	CONST menuitem *item;
	struct menurecord *nextrecord;
	struct menurecord *prevrecord;
	struct menurecord *submenu;
	struct menurecord *nextsubmenu;
};

struct menurecord *firstrecord;
struct menurecord rootmenu;

static void osx_appendmenu (struct uih_context *c, struct menurecord *menu,
							int is_root);


static struct menurecord *
osx_createmenu (struct uih_context *c,
				CONST char *name, CONST char *title, int type, int is_root)
{
	CFStringRef cfStr;
	struct menurecord *m =
		(struct menurecord *) malloc (sizeof (struct menurecord));
	static int id = 1;
	m->id = id++;
	m->type = type;
	
    CreateNewMenu (m->id, 0, &m->menu);
	cfStr = CFStringCreateWithCString(kCFAllocatorDefault, title, kCFStringEncodingUTF8);
	SetMenuTitleWithCFString(m->menu, cfStr);
	CFRelease(cfStr);
	
	m->c = c;
	m->item = menu_findcommand (name);
	m->nextrecord = firstrecord;
	m->prevrecord = NULL;
	firstrecord = m;
	m->nextsubmenu = NULL;
	m->submenu = NULL;
	osx_appendmenu (c, m, false);
	
	if (is_root)
		InsertMenu (m->menu, 0);
	else if (m->type == MENU_SUBMENU)
		InsertMenu (m->menu, kInsertHierarchicalMenu);
	
	return m;
}

static void
osx_appendmenu (struct uih_context *c, struct menurecord *menu, int is_root)
{
	int i;
	CONST menuitem *item;
	char title[256];
	MenuItemAttributes flags;	
	MenuItemIndex index;
	CFStringRef cfStr;
	
	for (i = 0; (item = menu_item (menu->item->shortname, i)) != NULL; i++)
    {
		struct menurecord *submenu = NULL;
		/*
		 // Mac applications provide their own quit function in the application menu,
		 // so the quit menu item should not be included in the file menu
		 if (item->shortname && strcmp(item->shortname, "") == 0)
		 continue;
		 */
		
		if (item->type == MENU_SUBMENU)
		{
			submenu = osx_createmenu (c, item->shortname, item->name, MENU_SUBMENU,	is_root);
			
			submenu->nextsubmenu = menu->submenu;
			menu->submenu = submenu;
		}		
		
		if (!is_root)
		{
			// On the Mac it is customary to put ... after menu items that bring up a dialog
			strcpy(title, item->name);
			if (item->type == MENU_CUSTOMDIALOG || item->type == MENU_DIALOG)
				strcat (title, "...");
			
			// Set shortcut key
			if (item->key)
			{
				strcat (title, " (");
				strcat (title, item->key);
				strcat (title, ")");
			}
			
			// Append menu item
			cfStr = CFStringCreateWithCString(kCFAllocatorDefault, title, kCFStringEncodingUTF8);
			flags = (item->type == MENU_SEPARATOR) ? kMenuItemAttrSeparator : 0;
			AppendMenuItemTextWithCFString(menu->menu, cfStr, flags, 0, &index);
			CFRelease(cfStr);			
			
			// Set mac-like shortcut keys
			if (item->shortname && strcmp(item->shortname, "loadpos") == 0)
				SetMenuItemCommandKey(menu->menu, index, false, 'O');
			else if (item->shortname && strcmp(item->shortname, "savepos") == 0)
				SetMenuItemCommandKey(menu->menu, index, false, 'S');
			else if (item->shortname && strcmp(item->shortname, "undo") == 0)
				SetMenuItemCommandKey(menu->menu, index, false, 'Z');
			else if (item->shortname && strcmp(item->shortname, "redo") == 0)
			{
				SetMenuItemCommandKey(menu->menu, index, false, 'Z');
				SetMenuItemModifiers(menu->menu, index, kMenuShiftModifier);
			}
			
			// Check active radio or checkbox menu items
			if (item->flags & (MENUFLAG_RADIO | MENUFLAG_CHECKBOX) && menu_enabled (item, c))
				CheckMenuItem(menu->menu, index, true);
			
			// Attach submenu
			if (item->type == MENU_SUBMENU)
				SetMenuItemHierarchicalID (menu->menu, index, submenu->id);
		}
    }
	
}

void
osx_menu_selected (int menu_id, int item_no)
{
	struct menurecord *menu = firstrecord;
	
	while (menu)
    {
		if (menu_id == menu->id)
		{
			ui_menuactivate (menu_item (menu->item->shortname, item_no - 1),
							 NULL);
			return;
		}
		menu = menu->nextrecord;
    }
}

static void
osx_freestructures (struct menurecord *menu)
{
	struct menurecord *sub, *nextsub;
	
	sub = menu->submenu;
	while (sub != NULL)
    {
		nextsub = sub->nextsubmenu;
		osx_freestructures (sub);
		sub = nextsub;
    }
	if (menu->nextrecord)
		menu->nextrecord->prevrecord = menu->prevrecord;
	if (menu->prevrecord)
		menu->prevrecord->nextrecord = menu->nextrecord;
	else
		firstrecord = menu->nextrecord;
	
	DeleteMenu (menu->id);
	DisposeMenu (menu->menu);
	
	free (menu);
}

void
osx_dorootmenu (struct uih_context *uih, CONST char *name)
{
	struct menurecord *sub, *nextsub;
	
	sub = rootmenu.submenu;
	rootmenu.c = uih;
	while (sub != NULL)
    {
		nextsub = sub->nextsubmenu;
		osx_freestructures (sub);
		sub = nextsub;
    }
	
	rootmenu.item = menu_findcommand (name);
	rootmenu.submenu = NULL;
	
	osx_appendmenu (uih, &rootmenu, true);
	
	DrawMenuBar ();
}

/* Check all created menus to see if there is the changed item and do changes
as neccesary.

This implementation is rather ugly and slow. Try if it is fast enought
and change it otherwise
*/

void
osx_enabledisable (struct uih_context *uih, CONST char *name)
{
	CONST struct menuitem *chgitem = menu_findcommand (name);
	CONST struct menuitem *item;
	CONST struct menuitem *radio;
	
	struct menurecord *menu = firstrecord;
	int i;
	
	int checked = menu_enabled (chgitem, uih);
	while (menu)
    {
		if (menu->item != NULL)
			for (i = 0; (item = menu_item (menu->item->shortname, i)) != NULL;
				 i++)
			{
				if (item == chgitem)
				{
					if ((chgitem->flags & MENUFLAG_RADIO) && checked)
					{
						int y;
						for (y = 0;
							 (radio =
							  menu_item (menu->item->shortname, y)) != NULL; y++)
							if (radio->flags & MENUFLAG_RADIO)
								CheckMenuItem (menu->menu, y + 1, false);
						
					}
					
					CheckMenuItem (menu->menu, i + 1, checked);
					
				}
			}
				menu = menu->nextrecord;
    }
}
