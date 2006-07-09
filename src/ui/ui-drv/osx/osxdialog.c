#include <aconfig.h>
#ifdef OSX_DRIVER

#include <Carbon/Carbon.h>

#include "ui.h"


static void
osx_filedialog (struct uih_context *uih, CONST menuitem * item,
				  CONST menudialog * dialog)
{
	char *name = "";//osx_dofiledialog (uih, item, dialog);
	if (name)
    {
		dialogparam *param = malloc (sizeof (dialogparam));
		param->dstring = name;
		ui_menuactivate (item, param);
    }
}


void
osx_dialog (struct uih_context *uih, CONST char *name)
{
	CONST menuitem *item = menu_findcommand (name);
	CONST menudialog *dialog;
	int nitems;
	char s[256];
	
	if (!item)
		return;
	dialog = menu_getdialog (uih, item);
	if (!dialog)
		return;
	for (nitems = 0; dialog[nitems].question; nitems++);
	if (nitems == 1
		&& (dialog[0].type == DIALOG_IFILE || dialog[0].type == DIALOG_OFILE))
		osx_filedialog (uih, item, dialog);
	else
    {
		/*
		struct dialogrecord *r = calloc (sizeof (*r), 1);
		r->next = firstdialog;
		firstdialog = r;
		r->prev = NULL;
		r->item = item;
		r->nitems = nitems;
		r->dialog = dialog;
		r->c = uih;
		sprintf (s, "%sBox", item->shortname);
		if (DialogBox (hInstance, s, hWnd, DialogHandler) == -1)
		{
			r->windialog=CreateDialog (hInstance, s, hWnd, DialogHandler);
			if(r->windialog==NULL) {
			x_message ("Failed to create dialog %s", item->shortname);
			osx_freedialog (r);
			}
		osx_freedialog (r);
		x_message("Dialog (%s %i %s) not implemented", name, nitems, dialog[0].question); 
		}
		*/
	}
}
#endif