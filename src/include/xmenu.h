#ifndef XMENU_H
#define XMENU_H
#include <xio.h>
#include <fconfig.h>
#ifdef __cplusplus
extern "C" {
#endif

struct uih_context;
typedef union {
  char *dstring;
  int dint;
  number_t number;
  number_t dcoord[2];
  xio_path dpath;
  void *dummy;
} dialogparam;
typedef struct dialog {
  CONST char *question;
  int type;
  int defint;
  CONST char *defstr;
  number_t deffloat;
  number_t deffloat2;
} menudialog;

typedef char *(*tokenfunc)(struct uih_context *c);
#define DIALOG_INT 1
#define DIALOG_FLOAT 2
#define DIALOG_STRING 3
#define DIALOG_KEYSTRING 4
#define DIALOG_IFILE 5
#define DIALOG_OFILE 6
#define DIALOG_CHOICE 7
#define DIALOG_ONOFF 8
#define DIALOG_COORD 9

#define DIALOGIFILE(question,filename) {question, DIALOG_IFILE, 0,filename}
#define DIALOGOFILE(question,filename) {question, DIALOG_OFILE, 0,filename}
#define DIALOGSTR(question,default)  {question, DIALOG_STRING, 0, default}
#define DIALOGKEYSTR(question,default)  {question, DIALOG_KEYSTRING, 0, default}
#define DIALOGINT(question,default)  {question, DIALOG_INT, default}
#define DIALOGONOFF(question,default)  {question, DIALOG_ONOFF, default}
#define DIALOGFLOAT(question,default)  {question, DIALOG_FLOAT, 0, NULL, default}
#define DIALOGCHOICE(question,table,default)  {question, DIALOG_CHOICE, default,(CONST char *)table}
#define DIALOGCOORD(question,default1,default2)  {question, DIALOG_COORD,0, NULL, default1,default2}


typedef struct menuitem {
  CONST char *menuname;
  CONST char *key;
  CONST char *name;
  CONST char *shortname;
  int type; 
  int flags;
  void (*function) (void);
  int iparam;
  CONST void *pparam;
  int (*control) (void);
  CONST menudialog *(*dialog)(struct uih_context *);
} menuitem;

#define MENU_NOPARAM 1
#define MENU_SUBMENU 2
#define MENU_INT     3
#define MENU_STRING  4
#define MENU_DIALOG  6
#define MENU_CUSTOMDIALOG  7
#define MENU_SEPARATOR 8

#define MENUNOP(menuname,key,name,shortname,flags,function) {menuname, key,name,shortname, MENU_NOPARAM, flags, (void (*)(void))function}
#define MENUNOPCB(menuname,key,name,shortname,flags,function,checkbutton) {menuname, key, name,shortname, MENU_NOPARAM, (flags)|MENUFLAG_CHECKBOX, (void (*)(void))function,0,NULL,(int (*)(void))checkbutton}
#define MENUCOORDCB(menuname,key,name,shortname,flags,function,checkbutton) {menuname, key, name,shortname, MENU_COORD, (flags)|MENUFLAG_CHECKBOX, (void (*)(void))function,0,NULL,(int (*)(void))checkbutton}
#define MENUCOORD(menuname,key,name,shortname,flags,function) {menuname, key, name,shortname, MENU_COORD, flags, (void (*)(void))function}

#define MENUINT(menuname,key,name,shortname,flags,function,param) {menuname, key, name,shortname, MENU_INT, flags, (void (*)(void))function,param}
#define MENUINTRB(menuname,key,name,shortname,flags,function,param,checkbutton) {menuname, key, name,shortname, MENU_INT, (flags)|MENUFLAG_RADIO, (void (*)(void))function,param,NULL,(int (*)(void))checkbutton}
#define SUBMENU(menuname,key,name,param) {menuname, key, name,param, MENU_SUBMENU, 0, NULL,0,NULL}
#define MENUSEPARATOR(menuname) {menuname, 0, "", NULL, MENU_SEPARATOR, 0, NULL,0,NULL}
#define SUBMENUNOOPT(menuname,key,name,param) {menuname, key, name,param, MENU_SUBMENU, MENUFLAG_NOOPTION, NULL,0,NULL}
#define MENUDIALOG(menuname,key,name,shortname,flags,function,param) {menuname, key, name,shortname, MENU_DIALOG, flags, (void (*)(void))function,0,param}
#define MENUDIALOGCB(menuname,key,name,shortname,flags,function,param,check) {menuname, key, name,shortname, MENU_DIALOG, flags|MENUFLAG_CHECKBOX, (void (*)(void))function,0,param,(int (*)(void))check}
#define MENUCDIALOG(menuname,key,name,shortname,flags,function,param) {menuname, key, name,shortname, MENU_CUSTOMDIALOG, flags, (void (*)(void))function,0,NULL,NULL,(CONST menudialog *(*)(struct uih_context *))param}
#define MENUCDIALOGCB(menuname,key,name,shortname,flags,function,param,check) {menuname, key, name,shortname, MENU_CUSTOMDIALOG, flags|MENUFLAG_CHECKBOX,(void (*)(void))function,0,NULL,(int (*)(void))check,(CONST menudialog *(*)(struct uih_context *))param}
#define MENUSTRING(menuname,key,name,shortname,flags,function,param) {menuname, key, name,shortname, MENU_STRING, flags, (void (*)(void))function,0,param}


#define MENUFLAG_CHECKBOX 1
#define MENUFLAG_RADIO 2
#define MENUFLAG_INTERRUPT 4
#define MENUFLAG_INCALC 8
#define MENUFLAG_NOMENU 16
#define MENUFLAG_NOOPTION 32
#define MENUFLAG_NOPLAY 64
#define MENUFLAG_ATSTARTUP 128
#define MENUFLAG_DIALOGATDISABLE 256

#define NITEMS(n) (sizeof(n)/sizeof(menuitem))
#define menu_getdialog(context, m) ((m)->type==MENU_DIALOG?(CONST menudialog *)(m)->pparam:(m)->dialog(context))

void menu_add(CONST menuitem *item, int n);
void menu_insert(CONST menuitem *item, CONST char *before, int n);
CONST menuitem *menu_findkey(CONST char *key, CONST char *root);
CONST menuitem *menu_findcommand(CONST char *name);
CONST char *menu_fullname(CONST char *menu);
CONST menuitem *menu_item(CONST char *menu, int n);
void menu_delete(CONST menuitem *items, int n);
int menu_enabled(CONST menuitem *item, struct uih_context *c);
void menu_activate(CONST menuitem *item, struct uih_context *c, dialogparam *d);
CONST menuitem * menu_genernumbered (int n, CONST char *menuname, CONST char * CONST * CONST names, CONST char *keys, int type, int flags, void (*fint)(struct uih_context *context,int), int (*cint)(struct uih_context *context,int), CONST char *prefix);
void menu_delnumbered (int n, CONST char *name);
void menu_addqueue(CONST menuitem *item, dialogparam *d);
CONST menuitem *menu_delqueue (dialogparam ** d);
void menu_destroydialog(CONST menuitem *item, dialogparam *d, struct uih_context *uih);
int menu_havedialog(CONST menuitem *item, struct uih_context *c);
int menu_available(CONST menuitem *item, CONST char *root);
CONST char *menu_processcommand(struct uih_context *uih,tokenfunc f, int scheme, int mask, CONST char *root);
void menu_printhelp(void);

number_t menu_getfloat (CONST char *s, CONST char **error);
CONST char *menu_fillparam(struct uih_context *uih, tokenfunc f,CONST menudialog *d, dialogparam *p);
int menu_processargs(int n, int argc, char **argv);
void uih_xshlprintmenu (struct uih_context *c, CONST char *name);
void uih_xshlprintmenus (struct uih_context *c);
void menu_forall(struct uih_context *c, void (*callback)(struct uih_context *c, CONST menuitem *item));

#ifdef __cplusplus
}
#endif
#endif
