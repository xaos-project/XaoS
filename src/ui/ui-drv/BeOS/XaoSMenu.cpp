/*
 *     XaoS, a fast portable realtime fractal zoomer 
 *                  Copyright © 1996-1999 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
 *
 *	be_error.cpp	BeOS user interface error output
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
#include <string.h>
#include <Rect.h>
#include <MenuItem.h>
#include <Application.h>
#include <ctype.h>
#include "XaoSMenu.h"
#include "xmenu.h"

XaoSMenu::XaoSMenu(port_id port, CONST char *name,int width,int height)
: 
       type(BROOTMENU),
       mEventPort(port)
{
       BRect rect(0,0,width,height);
       menu=menuBox=new BMenuBar(rect,name);
	root=NULL;
	next=NULL;
	popup=NULL;
	previous=NULL;
	target=NULL;
}
XaoSMenu *XaoSMenu::pmenu=NULL;
XaoSMenu::XaoSMenu(port_id port, CONST char *name, BPoint w, BHandler *t)
: 
       type(BPOPUPMENU),
       mEventPort(port),
       target(t),
       where(w)
{
       if(pmenu) delete pmenu, pmenu=NULL;
       menu=popup=new BPopUpMenu(name,this);
       pmenu=this;
	root=NULL;
	previous=NULL;
       next=NULL;
}
XaoSMenu::XaoSMenu(port_id port, CONST char *name, BHandler *t)
: 
       type(BSUBMENU),
       mEventPort(port),
       target(t)
{
        menu=new BMenu(name);
	next=NULL;
	root=NULL;
	popup=NULL;
	previous=NULL;
}
void
XaoSMenu::DeleteSubmenus(XaoSMenu *m)
{
	if (next) next->DeleteSubmenus(m);
	if (this->root==m) 
	{
		if(next) next->DeleteSubmenus(this);
		this->cleanMenu();
		delete this;
	}
}
XaoSMenu::~XaoSMenu()
{
	DeleteSubmenus(this);
	if (menu) delete menu;
	if (next) next->previous=previous;
	if (previous) previous->next=next;
}
void
XaoSMenu::removeItems(void)
{
	BMenuItem *i;
	int c=menu->CountItems(),n;
	for(n=c-1;n>=0;n--)
	{
		i=menu->ItemAt(0);
		if(i->Submenu()) {
			XaoSMenu *m=next->findMenu(i->Submenu());
			m->cleanMenu();
			delete m;
		}
		menu->RemoveItem(i);
	}
}
void
XaoSMenu::cleanMenu()
{
        if(type == BROOTMENU && pmenu && !popup) pmenu=NULL, delete pmenu;
	menu=NULL;
}
XaoSMenu *
XaoSMenu::findMenu(BMenu *m)
{
	if(menu == m) return this;
	if(!next) return NULL;
	return (next->findMenu(m));
	//Unlock();
}
void
XaoSMenu::setMenu(struct uih_context *c, CONST char *n)
{
	int i;
	CONST menuitem *item;
	BMenuItem *beitem;
	removeItems();
	strcpy(name,n);
	context=c;
	menu->SetRadioMode(false);
	for (i=0; (item = menu_item(name, i)) != NULL; i++)
	{
                if(item->type==MENU_SEPARATOR)
                {
			menu->AddSeparatorItem();
                } else if(item->type==MENU_SUBMENU)
		{
			BMessage *m1=new BMessage('XaCm');
			m1->AddPointer("Cmd",item);
			XaoSMenu *m = new XaoSMenu(mEventPort, item->name, target);
			AddToList(m);
			m->root=this;
			m->setMenu(c,item->shortname);
			beitem= new BMenuItem(m->menu,m1);
			if (item->key && type != BROOTMENU) beitem->SetShortcut(toupper(item->key[0]),0);
			menu->AddItem(beitem);
		} else {
			BMessage *m=new BMessage('XaCm');
			m->AddPointer("Cmd",item);
			menu->AddItem(beitem = new BMenuItem(item->name, m));
			if (item->key && type != BROOTMENU) beitem->SetShortcut(toupper(item->key[0]),0);
			beitem->SetMarked (menu_enabled(item,c));
                }
	}
	if (target!=NULL) menu->SetTargetForItems(target);
        if (popup) popup->Go(where, true, false, true);
}
void XaoSMenu::EnableDisable(CONST menuitem *i)
{
	int c=menu->CountItems(),n;
	BMenuItem *bi;
	for(n=0;n<c;n++)
	{
		bi=menu->ItemAt(n);
		if(!bi->Submenu() && bi->Message()) {
		   CONST menuitem *ptr;	
		   bi->Message()->FindPointer("Cmd",(void **)&ptr);
		   if (ptr==i) {
			bi->SetMarked(menu_enabled(i,context));
			if (i->flags & MENUFLAG_RADIO) {
			  int d;
			  for(d=0;d<c;d++)
			  {
				bi=menu->ItemAt(d);
				if(d!=n&&!bi->Submenu() && bi->Message())
				{
		   			bi->Message()->FindPointer("Cmd",(void **)&ptr);
					if (ptr && (ptr->flags & MENUFLAG_RADIO)) bi->SetMarked(menu_enabled(ptr,context));
				}
			  }
			}
		   }
		}
		
	}
	if(next) next->EnableDisable(i);
}
void XaoSMenu::AddToList(XaoSMenu *m)
{
	XaoSMenu *lm=this;
	while(lm->next) lm=lm->next;
	m->previous=lm;
	if(lm->next) next->previous=lm;
	lm->next=m;
}

