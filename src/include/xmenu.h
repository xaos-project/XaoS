#ifndef XMENU_H
#define XMENU_H
#include <xio.h>
#include <fconfig.h>
#ifdef __cplusplus
extern "C"
{
#endif

  struct uih_context;
  typedef union
  {
    char *dstring;
    int dint;
    number_t number;
    number_t dcoord[2];
    xio_path dpath;
    void *dummy;
  }
  dialogparam;
  typedef struct dialog
  {
    CONST char *question;
    int type;
    int defint;
    CONST char *defstr;
    number_t deffloat;
    number_t deffloat2;
  }
  menudialog;

  typedef char *(*tokenfunc) (struct uih_context * c);
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

#define DIALOGIFILE_I(_question,_filename) \
 menudialogs_i18n[no_menudialogs_i18n].question=_question; \
 menudialogs_i18n[no_menudialogs_i18n].type=DIALOG_IFILE; \
 menudialogs_i18n[no_menudialogs_i18n].defint=0; \
 menudialogs_i18n[no_menudialogs_i18n].defstr=_filename; \
 menudialogs_i18n[no_menudialogs_i18n].deffloat=0; \
 menudialogs_i18n[no_menudialogs_i18n].deffloat2=0; \
 ++no_menudialogs_i18n;
#define DIALOGOFILE_I(_question,_filename) \
 menudialogs_i18n[no_menudialogs_i18n].question=_question; \
 menudialogs_i18n[no_menudialogs_i18n].type=DIALOG_OFILE; \
 menudialogs_i18n[no_menudialogs_i18n].defint=0; \
 menudialogs_i18n[no_menudialogs_i18n].defstr=_filename; \
 ++no_menudialogs_i18n;
#define DIALOGKEYSTR_I(_question,_default) \
 menudialogs_i18n[no_menudialogs_i18n].question=_question; \
 menudialogs_i18n[no_menudialogs_i18n].type=DIALOG_KEYSTRING; \
 menudialogs_i18n[no_menudialogs_i18n].defint=0; \
 menudialogs_i18n[no_menudialogs_i18n].defstr=_default; \
 ++no_menudialogs_i18n;
#define DIALOGSTR_I(_question,_default) \
 menudialogs_i18n[no_menudialogs_i18n].question=_question; \
 menudialogs_i18n[no_menudialogs_i18n].type=DIALOG_STRING; \
 menudialogs_i18n[no_menudialogs_i18n].defint=0; \
 menudialogs_i18n[no_menudialogs_i18n].defstr=_default; \
 ++no_menudialogs_i18n;
#define DIALOGINT_I(_question,_default) \
 menudialogs_i18n[no_menudialogs_i18n].question=_question; \
 menudialogs_i18n[no_menudialogs_i18n].type=DIALOG_INT; \
 menudialogs_i18n[no_menudialogs_i18n].defint=_default; \
 ++no_menudialogs_i18n;
#define DIALOGONOFF_I(_question,_default) \
 menudialogs_i18n[no_menudialogs_i18n].question=_question; \
 menudialogs_i18n[no_menudialogs_i18n].type=DIALOG_ONOFF; \
 menudialogs_i18n[no_menudialogs_i18n].defint=_default; \
 ++no_menudialogs_i18n;
#define DIALOGFLOAT_I(_question,_default) \
 menudialogs_i18n[no_menudialogs_i18n].question=_question; \
 menudialogs_i18n[no_menudialogs_i18n].type=DIALOG_FLOAT; \
 menudialogs_i18n[no_menudialogs_i18n].defint=0; \
 menudialogs_i18n[no_menudialogs_i18n].defstr=NULL; \
 menudialogs_i18n[no_menudialogs_i18n].deffloat=_default; \
 ++no_menudialogs_i18n;
#define DIALOGCHOICE_I(_question,_table,_default) \
 menudialogs_i18n[no_menudialogs_i18n].question=_question; \
 menudialogs_i18n[no_menudialogs_i18n].type=DIALOG_CHOICE; \
 menudialogs_i18n[no_menudialogs_i18n].defint=_default; \
 menudialogs_i18n[no_menudialogs_i18n].defstr=(CONST char *)_table; \
 ++no_menudialogs_i18n;
#define DIALOGCOORD_I(_question,_default1,_default2) \
 menudialogs_i18n[no_menudialogs_i18n].question=_question; \
 menudialogs_i18n[no_menudialogs_i18n].type=DIALOG_COORD; \
 menudialogs_i18n[no_menudialogs_i18n].defint=0; \
 menudialogs_i18n[no_menudialogs_i18n].defstr=NULL; \
 menudialogs_i18n[no_menudialogs_i18n].deffloat=_default1; \
 menudialogs_i18n[no_menudialogs_i18n].deffloat2=_default2; \
 ++no_menudialogs_i18n;

#define NULL_I() \
 menudialogs_i18n[no_menudialogs_i18n].question=NULL; \
 menudialogs_i18n[no_menudialogs_i18n].type=0; \
 menudialogs_i18n[no_menudialogs_i18n].defint=0; \
 menudialogs_i18n[no_menudialogs_i18n].defstr=NULL; \
 menudialogs_i18n[no_menudialogs_i18n].deffloat=0; \
 menudialogs_i18n[no_menudialogs_i18n].deffloat2=0; \
 ++no_menudialogs_i18n;

  typedef struct menuitem
  {
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
    CONST menudialog *(*dialog) (struct uih_context *);
  }
  menuitem;

#define MENU_NOPARAM 1
#define MENU_SUBMENU 2
#define MENU_INT     3
#define MENU_STRING  4
#define MENU_DIALOG  6
#define MENU_CUSTOMDIALOG  7
#define MENU_SEPARATOR 8

/* Definitions for static menuitems. These items cannot be internationalized.
   All of these definitions will become obsolete soon: */

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


/* Definitions for internationalized menus. All of them must be defined
   dynamically because gettext() cannot be used within a static
   variable. Usage (example): 

   SUBMENU_I("file", "q", "Quit", "quitmenu")
	 
   See ui/ui.c, ui_registermenus_i18n() for further details. */

#define MENUNOP_I(_menuname,_key,_name,_shortname,_flags,_function)\
  menuitems_i18n[no_menuitems_i18n].menuname = _menuname; \
  menuitems_i18n[no_menuitems_i18n].shortname = _shortname; \
  menuitems_i18n[no_menuitems_i18n].key = _key; \
  menuitems_i18n[no_menuitems_i18n].type = MENU_NOPARAM; \
  menuitems_i18n[no_menuitems_i18n].flags = _flags; \
  menuitems_i18n[no_menuitems_i18n].iparam = 0; \
  menuitems_i18n[no_menuitems_i18n].name = _name; \
  menuitems_i18n[no_menuitems_i18n].pparam = NULL; \
  menuitems_i18n[no_menuitems_i18n].function = (void (*)(void))_function;  \
  ++no_menuitems_i18n;

#define MENUNOPCB_I(_menuname,_key,_name,_shortname,_flags,_function,_checkbutton) \
  menuitems_i18n[no_menuitems_i18n].menuname = _menuname; \
  menuitems_i18n[no_menuitems_i18n].shortname = _shortname; \
  menuitems_i18n[no_menuitems_i18n].key = _key; \
  menuitems_i18n[no_menuitems_i18n].type = MENU_NOPARAM; \
  menuitems_i18n[no_menuitems_i18n].flags = (_flags)|MENUFLAG_CHECKBOX; \
  menuitems_i18n[no_menuitems_i18n].iparam = 0; \
  menuitems_i18n[no_menuitems_i18n].name = _name; \
  menuitems_i18n[no_menuitems_i18n].pparam = NULL; \
  menuitems_i18n[no_menuitems_i18n].function = (void (*)(void))_function; \
  menuitems_i18n[no_menuitems_i18n].control = (int (*)(void))_checkbutton;  \
  ++no_menuitems_i18n;

#define MENUINT_I(_menuname,_key,_name,_shortname,_flags,_function,_param) \
  menuitems_i18n[no_menuitems_i18n].menuname = _menuname; \
  menuitems_i18n[no_menuitems_i18n].shortname = _shortname; \
  menuitems_i18n[no_menuitems_i18n].key = _key; \
  menuitems_i18n[no_menuitems_i18n].type = MENU_INT; \
  menuitems_i18n[no_menuitems_i18n].flags = _flags; \
  menuitems_i18n[no_menuitems_i18n].iparam = _param; \
  menuitems_i18n[no_menuitems_i18n].name = _name; \
  menuitems_i18n[no_menuitems_i18n].function = (void (*)(void))_function;  \
  ++no_menuitems_i18n;

#define MENUINTRB_I(_menuname,_key,_name,_shortname,_flags,_function,_param,_checkbutton) \
  menuitems_i18n[no_menuitems_i18n].menuname = _menuname; \
  menuitems_i18n[no_menuitems_i18n].shortname = _shortname; \
  menuitems_i18n[no_menuitems_i18n].key = _key; \
  menuitems_i18n[no_menuitems_i18n].type = MENU_INT; \
  menuitems_i18n[no_menuitems_i18n].flags = (_flags)|MENUFLAG_RADIO; \
  menuitems_i18n[no_menuitems_i18n].iparam = _param; \
  menuitems_i18n[no_menuitems_i18n].pparam = NULL; \
  menuitems_i18n[no_menuitems_i18n].name = _name; \
  menuitems_i18n[no_menuitems_i18n].function = (void (*)(void))_function;  \
  menuitems_i18n[no_menuitems_i18n].control = (int (*)(void))_checkbutton; \
  ++no_menuitems_i18n;

#define SUBMENU_I(_menuname,_key,_name,_param) \
  menuitems_i18n[no_menuitems_i18n].menuname = _menuname; \
  menuitems_i18n[no_menuitems_i18n].shortname = _param; \
  menuitems_i18n[no_menuitems_i18n].key = _key; \
  menuitems_i18n[no_menuitems_i18n].type = MENU_SUBMENU; \
  menuitems_i18n[no_menuitems_i18n].flags = 0; \
  menuitems_i18n[no_menuitems_i18n].iparam = 0; \
  menuitems_i18n[no_menuitems_i18n].name = _name; \
  menuitems_i18n[no_menuitems_i18n].pparam = NULL; \
  menuitems_i18n[no_menuitems_i18n].function = NULL;  \
  ++no_menuitems_i18n;

#define MENUDIALOG_I(_menuname,_key,_name,_shortname,_flags,_function,_param) \
  menuitems_i18n[no_menuitems_i18n].menuname = _menuname; \
  menuitems_i18n[no_menuitems_i18n].shortname = _shortname; \
  menuitems_i18n[no_menuitems_i18n].key = _key; \
  menuitems_i18n[no_menuitems_i18n].type = MENU_DIALOG; \
  menuitems_i18n[no_menuitems_i18n].flags = _flags; \
  menuitems_i18n[no_menuitems_i18n].iparam = 0; \
  menuitems_i18n[no_menuitems_i18n].name = _name; \
  menuitems_i18n[no_menuitems_i18n].pparam = _param; \
  menuitems_i18n[no_menuitems_i18n].function = (void (*)(void))_function;  \
  ++no_menuitems_i18n;

#define MENUDIALOGCB_I(_menuname,_key,_name,_shortname,_flags,_function,_param,_check) \
  menuitems_i18n[no_menuitems_i18n].menuname = _menuname; \
  menuitems_i18n[no_menuitems_i18n].shortname = _shortname; \
  menuitems_i18n[no_menuitems_i18n].key = _key; \
  menuitems_i18n[no_menuitems_i18n].type = MENU_DIALOG; \
  menuitems_i18n[no_menuitems_i18n].flags = (_flags)|MENUFLAG_CHECKBOX; \
  menuitems_i18n[no_menuitems_i18n].iparam = 0; \
  menuitems_i18n[no_menuitems_i18n].name = _name; \
  menuitems_i18n[no_menuitems_i18n].pparam = _param; \
  menuitems_i18n[no_menuitems_i18n].function = (void (*)(void))_function;  \
  menuitems_i18n[no_menuitems_i18n].control = (int (*)(void))_check; \
  ++no_menuitems_i18n;

#define MENUCDIALOG_I(_menuname,_key,_name,_shortname,_flags,_function,_param) \
  menuitems_i18n[no_menuitems_i18n].menuname = _menuname; \
  menuitems_i18n[no_menuitems_i18n].shortname = _shortname; \
  menuitems_i18n[no_menuitems_i18n].key = _key; \
  menuitems_i18n[no_menuitems_i18n].type = MENU_CUSTOMDIALOG; \
  menuitems_i18n[no_menuitems_i18n].flags = _flags; \
  menuitems_i18n[no_menuitems_i18n].iparam = 0; \
  menuitems_i18n[no_menuitems_i18n].name = _name; \
  menuitems_i18n[no_menuitems_i18n].pparam = NULL; \
  menuitems_i18n[no_menuitems_i18n].control = NULL; \
  menuitems_i18n[no_menuitems_i18n].function = (void (*)(void))_function; \
  menuitems_i18n[no_menuitems_i18n].dialog = (CONST menudialog *(*)(struct uih_context *))_param; \
  ++no_menuitems_i18n;

#define MENUCDIALOGCB_I(_menuname,_key,_name,_shortname,_flags,_function,_param,_check)\
  menuitems_i18n[no_menuitems_i18n].menuname = _menuname; \
  menuitems_i18n[no_menuitems_i18n].shortname = _shortname; \
  menuitems_i18n[no_menuitems_i18n].key = _key; \
  menuitems_i18n[no_menuitems_i18n].type = MENU_CUSTOMDIALOG; \
  menuitems_i18n[no_menuitems_i18n].flags = (_flags)|MENUFLAG_CHECKBOX; \
  menuitems_i18n[no_menuitems_i18n].iparam = 0; \
  menuitems_i18n[no_menuitems_i18n].name = _name; \
  menuitems_i18n[no_menuitems_i18n].pparam = _param; \
  menuitems_i18n[no_menuitems_i18n].function = (void (*)(void))_function;  \
  menuitems_i18n[no_menuitems_i18n].control = (int (*)(void))_check; \
  menuitems_i18n[no_menuitems_i18n].dialog = (CONST menudialog *(*)(struct uih_context *))_param; \
  ++no_menuitems_i18n;

#define MENUSEPARATOR_I(_menuname) \
  menuitems_i18n[no_menuitems_i18n].menuname = _menuname; \
  menuitems_i18n[no_menuitems_i18n].shortname = NULL; \
  menuitems_i18n[no_menuitems_i18n].key = 0; \
  menuitems_i18n[no_menuitems_i18n].type = MENU_SEPARATOR; \
  menuitems_i18n[no_menuitems_i18n].flags = 0; \
  menuitems_i18n[no_menuitems_i18n].iparam = 0; \
  menuitems_i18n[no_menuitems_i18n].name = ""; \
  menuitems_i18n[no_menuitems_i18n].pparam = NULL; \
  menuitems_i18n[no_menuitems_i18n].function = NULL; \
  ++no_menuitems_i18n;

#define SUBMENUNOOPT_I(_menuname,_key,_name,_param) \
  menuitems_i18n[no_menuitems_i18n].menuname = _menuname; \
  menuitems_i18n[no_menuitems_i18n].shortname = _param; \
  menuitems_i18n[no_menuitems_i18n].key = _key; \
  menuitems_i18n[no_menuitems_i18n].type = MENU_SUBMENU; \
  menuitems_i18n[no_menuitems_i18n].flags = MENUFLAG_NOOPTION; \
  menuitems_i18n[no_menuitems_i18n].iparam = 0; \
  menuitems_i18n[no_menuitems_i18n].name = _name; \
  menuitems_i18n[no_menuitems_i18n].pparam = NULL; \
  menuitems_i18n[no_menuitems_i18n].function = NULL;  \
  ++no_menuitems_i18n;

#define MENUSTRING_I(_menuname,_key,_name,_shortname,_flags,_function,_param) \
  menuitems_i18n[no_menuitems_i18n].menuname = _menuname; \
  menuitems_i18n[no_menuitems_i18n].shortname = _shortname; \
  menuitems_i18n[no_menuitems_i18n].key = _key; \
  menuitems_i18n[no_menuitems_i18n].type = MENU_STRING; \
  menuitems_i18n[no_menuitems_i18n].flags = _flags; \
  menuitems_i18n[no_menuitems_i18n].iparam = 0; \
  menuitems_i18n[no_menuitems_i18n].name = _name; \
  menuitems_i18n[no_menuitems_i18n].pparam = _param; \
  menuitems_i18n[no_menuitems_i18n].function = (void (*)(void))_function;  \
  ++no_menuitems_i18n;

/* End of i18n definitions. */


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
#define menu_getdialog(context, m) \
 ((m)->type==MENU_DIALOG?(CONST menudialog *)(m)->pparam:(m)->dialog(context))

  void menu_add (CONST menuitem * item, int n);
  void menu_insert (CONST menuitem * item, CONST char *before, int n);
  CONST menuitem *menu_findkey (CONST char *key, CONST char *root);
  CONST menuitem *menu_findcommand (CONST char *name);
  CONST char *menu_fullname (CONST char *menu);
  CONST menuitem *menu_item (CONST char *menu, int n);
  void menu_delete (CONST menuitem * items, int n);
  int menu_enabled (CONST menuitem * item, struct uih_context *c);
  void menu_activate (CONST menuitem * item, struct uih_context *c,
		      dialogparam * d);
  CONST menuitem *menu_genernumbered (int n, CONST char *menuname,
				      CONST char *CONST * CONST names,
				      CONST char *keys, int type, int flags,
				      void (*fint) (struct uih_context *
						    context, int),
				      int (*cint) (struct uih_context *
						   context, int),
				      CONST char *prefix);
  void menu_delnumbered (int n, CONST char *name);
  void menu_addqueue (CONST menuitem * item, dialogparam * d);
  CONST menuitem *menu_delqueue (dialogparam ** d);
  void menu_destroydialog (CONST menuitem * item, dialogparam * d,
			   struct uih_context *uih);
  int menu_havedialog (CONST menuitem * item, struct uih_context *c);
  int menu_available (CONST menuitem * item, CONST char *root);
  CONST char *menu_processcommand (struct uih_context *uih, tokenfunc f,
				   int scheme, int mask, CONST char *root);
  void menu_printhelp (void);

  number_t menu_getfloat (CONST char *s, CONST char **error);
  CONST char *menu_fillparam (struct uih_context *uih, tokenfunc f,
			      CONST menudialog * d, dialogparam * p);
  int menu_processargs (int n, int argc, char **argv);
  void uih_xshlprintmenu (struct uih_context *c, CONST char *name);
  void uih_xshlprintmenus (struct uih_context *c);
  void menu_forall (struct uih_context *c,
		    void (*callback) (struct uih_context * c,
				      CONST menuitem * item));

#ifdef __cplusplus
}
#endif
#endif
