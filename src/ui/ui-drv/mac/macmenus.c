/*
** mac_menu.c
**
** Written by Dominic Mazzoni
**
** This code implements native Macintosh menus for the Xaos Fractal Zoomer.
** The root menu becomes the main mac menu bar, with all of its menus appended
** after the apple menu.  Any menu titled "help" becomes part of the Mac help
** menu.
**
** This code was adapted from the MS Windows driver source code
**
** Hierarchical menus inside the Help menu do not work, and I don't know why.
** This may be a Mac OS bug.  In the meantime, I moved Tutorials to their own
** menu.
*/

#include "config.h"
#include "ui.h"
#include "xmenu.h"

#include <Menus.h>

#define IDMUL 64
#define MAIN 0
#define POPUP 1
#define WSUBMENU 2

struct menurecord
{
  MenuHandle menu;

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

int          mac_helpMenuID = -1;
MenuHandle   mac_helpMenu = 0;
int          mac_helpMenuItems = 0;

static void mac_appendmenu (struct uih_context *c, struct menurecord *menu, int is_root);



///////////////////////////



MenuHandle mac_createrootmenu ()
{
  rootmenu.id = 0;
  rootmenu.menu = NULL;
  rootmenu.nextrecord = firstrecord;
  rootmenu.prevrecord = NULL;
  firstrecord = &rootmenu;
  rootmenu.nextsubmenu = NULL;
  rootmenu.submenu = NULL;

  return rootmenu.menu;
}

static struct menurecord *mac_createmenu (struct uih_context *c,
                                          CONST char *name,
                                          CONST char *title,
                                          int type, int is_root)
{
  Str255 titlestr;
  struct menurecord *m = (struct menurecord *)calloc (sizeof (struct menurecord), 1);
  static int id = 1;
  m->id = id++;
  m->type = type;
  
  strcpy(titlestr, title);
  c2pstr(titlestr);
  
  if (strstr(name,"help")) {
    HMGetHelpMenuHandle(&mac_helpMenu);
    mac_helpMenuID = (**(mac_helpMenu)).menuID;
    mac_helpMenuItems = CountMenuItems(mac_helpMenu);
    
    m->menu = mac_helpMenu;
    m->id = mac_helpMenuID;
  }
	else
    m->menu = NewMenu(m->id, titlestr);

  m->c = c;
  m->item = menu_findcommand (name);
  m->nextrecord = firstrecord;
  m->prevrecord = NULL;
  firstrecord = m;
  m->nextsubmenu = NULL;
  m->submenu = NULL;
  mac_appendmenu (c, m, false);
  
  if (m->menu != mac_helpMenu) {
	  if (is_root)
	    MacInsertMenu(m->menu, 0);
	  else if (m->type == MENU_SUBMENU)
	    MacInsertMenu(m->menu, hierMenu);
	}

  return m;
}

static void mac_appendmenu (struct uih_context *c, struct menurecord *menu, int is_root)
{
  int i, x, y;
  CONST menuitem *item;
  char out[256];
  char used[256];
  memset (used, 0, 256);

  for (i = 0; (item = menu_item (menu->item->shortname, i)) != NULL; i++)
    {
      struct menurecord *submenu = NULL;

      x = 0;
      y = 0;
      while(item->name[y]) {
        if ((item->name[y]>='A' && item->name[y]<='Z') ||
            (item->name[y]>='a' && item->name[y]<='z') ||
            (item->name[y]>='0' && item->name[y]<='9') ||
            item->name[y] == ' ')
          out[x++] = item->name[y];
        if (item->name[y] == '^')
          out[x++] = '`';
        y++;
      }
      out[x] = 0;
      
      if (item->type == MENU_SUBMENU)
			{
			  submenu = mac_createmenu (c, item->shortname, item->name, MENU_SUBMENU, is_root);

			  submenu->nextsubmenu = menu->submenu;
			  menu->submenu = submenu;
			}
			
			// On the Mac it is customary to put ... after menu items
			// that bring up a dialog
			if (item->type == MENU_CUSTOMDIALOG)
			  strcat(out, "É");

      if (item->flags & (MENUFLAG_RADIO | MENUFLAG_CHECKBOX) &&
       	  menu_enabled (item, c))
      {
        // These characters put a checkmark next to the item
        strcat(out, "!\022");
	    }
	    
      if (item->key)
			{
			  char cmdstr[3];
			  char ch;
			  
			  ch = item->key[0];
			  if (ch>='a' && ch<='z')
			    ch -= ('a'-'A');
			  
			  if ((ch>='A' && ch<='Z') || (ch>='0' && ch<='9')) {			  
				  cmdstr[0] = '/';
				  cmdstr[1] = ch;
				  cmdstr[2] = 0;
				  
				  strcat (out, cmdstr);
				}
			}

	    if (item->type == MENU_SEPARATOR)
        strcpy(out,"(-");
			
	    c2pstr(out);

      if (!is_root) {
      
	      MacAppendMenu(menu->menu, (StringPtr)out);
	      
	      if (item->type == MENU_SUBMENU)
	      {
	        // Attach submenu
	        SetItemMark(menu->menu, CountMenuItems(menu->menu), submenu->id);
	      
	        // This tells that Mac OS that this item is hierarchical
	        SetItemCmd(menu->menu, CountMenuItems(menu->menu), 0x1B);
	      }
      }
    }

}



static void mac_freestructures (struct menurecord *menu)
{
  struct menurecord *sub, *nextsub;

  sub = menu->submenu;
  while (sub != NULL)
    {
      nextsub = sub->nextsubmenu;
      mac_freestructures (sub);
      sub = nextsub;
    }
  if (menu->nextrecord)
    menu->nextrecord->prevrecord = menu->prevrecord;
  if (menu->prevrecord)
    menu->prevrecord->nextrecord = menu->nextrecord;
  else
    firstrecord = menu->nextrecord;
  
  if (menu->menu == mac_helpMenu)
  {
    int c = CountMenuItems(mac_helpMenu);
    while(c > mac_helpMenuItems) {
      DeleteMenuItem(mac_helpMenu, c);
      c--;
    }
  }
  else
  {
	  MacDeleteMenu(menu->id);
	  DisposeMenu(menu->menu);
	}
  
  free (menu);
}

void mac_menu_selected (int menu_id, int item_no)
{
  struct menurecord *menu = firstrecord;

  if (menu_id == mac_helpMenuID)
    item_no -= mac_helpMenuItems;

  while (menu)
    {
      if (menu_id == menu->id)
			{
			  ui_menuactivate (menu_item (menu->item->shortname, item_no-1), NULL);
			  return;
			}
      menu = menu->nextrecord;
    }
}

void mac_dorootmenu (struct uih_context *uih, CONST char *name)
{
  struct menurecord *sub, *nextsub;

  sub = rootmenu.submenu;
  rootmenu.c = uih;
  while (sub != NULL)
  {
    nextsub = sub->nextsubmenu;
    mac_freestructures (sub);
    sub = nextsub;
  }

  rootmenu.item = menu_findcommand (name);
  rootmenu.submenu = NULL;
  
  mac_appendmenu (uih, &rootmenu, true);
  
  DrawMenuBar();
}

/* Check all created menus to see if there is the changed item and do changes
   as neccesary.

   This implementation is rather ugly and slow. Try if it is fast enought
   and change it otherwise
*/

void mac_enabledisable (struct uih_context *uih, CONST char *name)
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
			for (i = 0; (item = menu_item (menu->item->shortname, i)) != NULL; i++)
			  {
			    if (item == chgitem)
			      {
							if ((chgitem->flags & MENUFLAG_RADIO) && checked)
							  {
							    int y;
							    for (y = 0; (radio = menu_item (menu->item->shortname, y)) != NULL; y++)
							      if (radio->flags & MENUFLAG_RADIO)
								      					      CheckItem (menu->menu,
																                 y+1,
																                 false);

							  }

				      CheckItem (menu->menu,
				                 i+1,
				                 checked);
					                 
			      }
			  }
    menu = menu->nextrecord;
  }
}

void mac_menu (struct uih_context *c, CONST char *name)
{
  /*
  Point p;
  struct menurecord *m = firstrecord;
  int result;

  // Delete records about all popups, since they are closed now
  
  while (m)
    {
      if (m->type == POPUP)
			{
			  DisposeMenu (m->menu);
			  mac_freestructures (m);
			  break;
			}
      m = m->nextrecord;
    }

  m = mac_createmenu (c, name, name, POPUP, false);

  GetMouse (&p);
  LocalToGlobal(&p);
  
  result = PopUpMenuSelect(m->menu, p.v, p.h, 1);
  */
}

void mac_uninitializewindows ()
{
  while (firstrecord)
    {
      struct menurecord *r = firstrecord;
      while (r->type == WSUBMENU)
	      r = r->nextrecord;
      DestroyMenu (r->menu);
      
      mac_freestructures (r);
    }
}
