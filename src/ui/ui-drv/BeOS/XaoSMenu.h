#ifndef XAOSMENU_H
#define XAOSMENU_H
#include <Menu.h>
#include <MenuBar.h>
#include <Handler.h>
#include <PopUpMenu.h>
#include "xmenu.h"

#define BSUBMENU   0
#define BROOTMENU  1
#define BPOPUPMENU 2
struct uih_context;
class XaoSMenu;
class XaoSMenu
{
public:
	//typedef Window inherited;
	XaoSMenu(port_id port, CONST char *name, int width, int height);
	XaoSMenu(port_id port, CONST char *name, BPoint where, BHandler *t);
	XaoSMenu(port_id port, CONST char *name, BHandler *t);
	virtual ~XaoSMenu(void);

	void setMenu(struct uih_context *c, CONST char *name);
	void cleanMenu(void);

	const int type;
	BMenu *menu;
	BMenuBar *menuBox;
	BPopUpMenu *popup;

	//virtual void MessageReceived(BMessage *pMessage);

	void AddToList(XaoSMenu *menu);
	XaoSMenu *findMenu(BMenu *menu);
	XaoSMenu *next, *previous, *root;
	void EnableDisable(CONST menuitem *i);
	void DeleteSubmenus(XaoSMenu *root);
	void Lock();
	void Unlock();
private:
	//void SendEvent(long eventCode, const XaoSEvent &event) const;
	const port_id mEventPort;
	struct uih_context *context;
	char name[20];
	void removeItems(void);
	BHandler *target;
	BPoint where;
	static XaoSMenu *pmenu;
};
#endif

