#include <windows.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <config.h>
/*#include <filter.h>
   #include <ui_helper.h> */
#include <xerror.h>
#include <xldio.h>
#include <ui.h>
#include <xmenu.h>
#include "ui_win32.h"

#ifdef HAVE_GETTEXT
#include <libintl.h>
#else
#define gettext(STRING) STRING
#endif

TCHAR text[100];

#define QUESTIONSTART 100
#define ITEMSTART 10
#define PERITEM 5

#define OK 1
#define CANCEL 2
#define HELP 3

static struct dialogrecord {
    CONST menuitem *item;
    CONST menudialog *dialog;
    int nitems;
    HWND windialog;
    struct uih_context *c;
    struct dialogrecord *next, *prev;
} *firstdialog = NULL;
static CONST char *win32_getextension(CONST char *ch)
{
    int i = 0;
    while (ch[i]) {
	if (ch[i] == '*')
	    return (ch + i + 1);
	i++;
    }
    return ch + i;
}

static char *win32_dofiledialog(struct uih_context *uih,
				CONST menuitem * item,
				CONST menudialog * dialog)
{
    OPENFILENAME ofn;
    char szDirName[256];
    char szFile[256], szFileTitle[256];
    UINT i, p;
    char szFilter[256];

    helptopic = item->shortname;
    szDirName[0] = 0;
    /*GetSystemDirectory(szDirName, sizeof(szDirName)); */
    szFile[0] = 0;
    if (dialog[0].type == DIALOG_OFILE) {
	strcpy(szFile, dialog[0].defstr);
	for (i = 0; dialog[0].defstr[i] && dialog[0].defstr[i] != '*';
	     i++);
	szFile[i] = 0;
	strcpy(szFile,
	       ui_getfile(szFile, win32_getextension(dialog[0].defstr)));
    }

    for (i = 0; dialog[0].defstr[i] && dialog[0].defstr[i] != '*'; i++)
	if (!dialog[0].defstr[i])
	    i = 0;
    strcpy(szFilter, dialog[0].defstr + i);
    p = strlen(szFilter);
    strcpy(szFilter + p + 1, dialog[0].defstr + i);
    p += strlen(szFilter + p + 1) + 1;
    strcpy(szFilter + p + 1, "All files");
    p += strlen(szFilter + p + 1) + 1;
    strcpy(szFilter + p + 1, "*.*");
    p += strlen(szFilter + p + 1) + 1;
    szFilter[p + 1] = 0;

    memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle);
    ofn.lpstrInitialDir = szDirName;
    ofn.Flags =
	OFN_SHOWHELP | OFN_PATHMUSTEXIST | (dialog[0].type ==
					    DIALOG_IFILE ?
					    OFN_FILEMUSTEXIST :
					    OFN_OVERWRITEPROMPT);
    if (dialog[0].type == DIALOG_IFILE)
	i = GetOpenFileName(&ofn);
    else
	i = GetSaveFileName(&ofn);
    helptopic = "main";
    if (i) {
	return (strdup(ofn.lpstrFile));
    }
    return NULL;
}


static void
win32_filedialog(struct uih_context *uih, CONST menuitem * item,
		 CONST menudialog * dialog)
{
    char *name = win32_dofiledialog(uih, item, dialog);
    if (name) {
	dialogparam *param = malloc(sizeof(dialogparam));
	param->dstring = name;
	ui_menuactivate(item, param);
    }
}

static void win32_freedialog(struct dialogrecord *r)
{
    if (r->next)
	r->next->prev = r->prev;
    if (r->prev)
	r->prev->next = r->next;
    else
	firstdialog = r->next;
    free(r);
}

static int win32_dodialog(struct dialogrecord *r, HWND hDLG)
{
    dialogparam *p = calloc(sizeof(*p), r->nitems);
    int i;
    char s[256];
    for (i = 0; i < r->nitems; i++) {
	switch (r->dialog[i].type) {
	case DIALOG_IFILE:
	case DIALOG_OFILE:
	case DIALOG_STRING:
	case DIALOG_KEYSTRING:
	    GetDlgItemText(hDLG, i * PERITEM + ITEMSTART, s, sizeof(s));
	    p[i].dstring = strdup(s);
	    break;
	case DIALOG_INT:
	    GetDlgItemText(hDLG, i * PERITEM + ITEMSTART, s, sizeof(s));
	    p[i].dint = r->dialog[i].defint;
	    sscanf(s, "%i", &p[i].dint);
	    break;
	case DIALOG_FLOAT:
	    GetDlgItemText(hDLG, i * PERITEM + ITEMSTART, s, sizeof(s));
	    p[i].number = r->dialog[i].deffloat;
	    p[i].number = ui_getfloat(s);
	    break;
	case DIALOG_COORD:
	    GetDlgItemText(hDLG, i * PERITEM + ITEMSTART, s, sizeof(s));
	    p[i].dcoord[0] = r->dialog[i].deffloat;
	    p[i].dcoord[0] = ui_getfloat(s);
	    GetDlgItemText(hDLG, i * PERITEM + ITEMSTART + 1, s,
			   sizeof(s));
	    p[i].dcoord[1] = r->dialog[i].deffloat2;
	    p[i].dcoord[1] = ui_getfloat(s);
	    break;
	case DIALOG_CHOICE:
	    /*x_message("Choice is not implemented yet"); */
	    {
		int y;
		y = LOWORD(SendDlgItemMessage
			   (hDLG, i * PERITEM + ITEMSTART, CB_GETCURSEL, 0,
			    0L));
		p[i].dint =
		    LOWORD(SendDlgItemMessage
			   (hDLG, i * PERITEM + ITEMSTART, CB_GETITEMDATA,
			    (WPARAM) y, 0L));
	    }
	}
    }
    ui_menuactivate(r->item, p);
    return 1;
}

static BOOL APIENTRY
DialogHandler(HWND hDLG, UINT message, UINT wParam, LONG lParam)
{
    struct dialogrecord *rec = firstdialog;
    int i;
/*   while(rec->windialog!=hDLG) rec=rec->next; */
    if (!rec->windialog)
	rec->windialog = hDLG;
    switch (message) {
    case WM_INITDIALOG:
	/*x_message("Creating dialog"); */
	ShowWindow(hDLG, SW_HIDE);
	/*CenterWindow (hDLG, GetWindow (hDLG, GW_OWNER)); */
	if (GetWindowText(hDLG, text, GetWindowTextLength(hDLG) + 1) > 0);
	SetWindowText(hDLG, gettext(text));
	SetDlgItemText(hDLG, OK, gettext("OK"));
	SetDlgItemText(hDLG, CANCEL, gettext("Cancel"));
	SetDlgItemText(hDLG, HELP, gettext("Help"));
	for (i = 0; rec->dialog[i].question; i++) {
	    if (GetDlgItemText
		(hDLG, i * PERITEM + QUESTIONSTART, text, 100) > 0)
		SetDlgItemText(hDLG, i * PERITEM + QUESTIONSTART,
			       gettext(text));
	    switch (rec->dialog[i].type) {
		char s[256];
	    case DIALOG_STRING:
	    case DIALOG_IFILE:
	    case DIALOG_OFILE:
		SetDlgItemText(hDLG, i * PERITEM + ITEMSTART,
			       rec->dialog[i].defstr);
		break;
	    case DIALOG_INT:
		sprintf(s, "%i", rec->dialog[i].defint);
		SetDlgItemText(hDLG, i * PERITEM + ITEMSTART, s);
		break;
	    case DIALOG_COORD:
		sprintf(s, "%g", (double) rec->dialog[i].deffloat2);
		SetDlgItemText(hDLG, i * PERITEM + ITEMSTART + 1, s);
		/*Fall trought */
	    case DIALOG_FLOAT:
		sprintf(s, "%g", (double) rec->dialog[i].deffloat);
		SetDlgItemText(hDLG, i * PERITEM + ITEMSTART, s);
		break;
	    case DIALOG_CHOICE:
		{
		    CONST char **strings =
			(CONST char **) rec->dialog[i].defstr;
		    int y;
		    int pos;
		    for (y = 0; strings[y]; y++) {
			pos =
			    LOWORD(SendDlgItemMessage
				   (hDLG, i * PERITEM + ITEMSTART,
				    CB_ADDSTRING, (WPARAM) 0,
				    (LPARAM) (LPSTR) strings[y]));
			/*x_message("%s %i",strings[y],pos); */
			SendMessage(GetDlgItem
				    (hDLG, i * PERITEM + ITEMSTART),
				    CB_SETITEMDATA, (WPARAM) pos, y);
			if (y == rec->dialog[i].defint) {
			    pos =
				SendMessage(GetDlgItem
					    (hDLG,
					     i * PERITEM + ITEMSTART),
					    CB_SETCURSEL, (WPARAM) pos,
					    0L);
			    /*x_message("Default %i",pos); */
			}
		    }
		    pos =
			LOWORD(SendDlgItemMessage
			       (hDLG, i * PERITEM + ITEMSTART, CB_GETCOUNT,
				(WPARAM) 0, 0));
		    /*x_message("Count %i",pos); */
		}
		break;
	    }
	}
	CenterWindow(hDLG, GetWindow(hDLG, GW_OWNER));
	ShowWindow(hDLG, SW_SHOW);
	return (TRUE);
    case WM_SYSCOMMAND:
	if (wParam == SC_CLOSE) {
	    EndDialog(hDLG, 0);
	    return (TRUE);
	}
	break;
    case WM_COMMAND:
	if (wParam == OK) {
	    if (win32_dodialog(rec, hDLG)) {
		EndDialog(hDLG, 0);
		return (TRUE);
	    }
	}
	if (wParam == CANCEL) {
	    EndDialog(hDLG, 0);
	    return (TRUE);
	}
	if (wParam == HELP) {
	    win32_help(rec->c, rec->item->shortname);
	    return (TRUE);
	}
	{
	    int i = (wParam - ITEMSTART) / PERITEM;
	    int pos = (wParam - ITEMSTART) % PERITEM;
	    if (i >= 0 && i < rec->nitems) {
		if (pos == 1
		    && (rec->dialog[i].type == DIALOG_IFILE
			|| rec->dialog[i].type == DIALOG_OFILE)) {
		    /*x_message("File dialog\n"); */
		    char *file =
			win32_dofiledialog(rec->c, rec->item,
					   rec->dialog + i);
		    if (file) {
			SetDlgItemText(hDLG, wParam - 1, file);
			free(file);
		    }
		}
	    }
	}
	break;
    }
    return FALSE;
}

#define INPUTSIZE 20
#define XBORDER 0
#define YBORDER 5
#define XSEP 4

#define CHARWIDTH 4
#define MINWIDTH ((7*3)*CHARWIDTH)
#define LINEHEIGHT 14
#define TEXTHEIGHT 11

static FILE *file;
static void
win32_outputdialog(struct uih_context *uih, CONST struct menuitem *item)
{
    CONST menudialog *dialog;
    int leftsize = 0;
    int rightsize = 0;
    int width, height;
    int i;
    rightsize = INPUTSIZE;
    if (item->type != MENU_DIALOG && item->type != MENU_CUSTOMDIALOG)
	return;
    if (item->flags & MENUFLAG_NOMENU)
	return;
    dialog = menu_getdialog(uih, item);
    for (i = 0; dialog[i].question; i++) {
	if (leftsize < (int) strlen(dialog[i].question))
	    leftsize = strlen(dialog[i].question);
    }
    if (i == 1
	&& (dialog[0].type == DIALOG_IFILE
	    || dialog[0].type == DIALOG_OFILE))
	return;
    leftsize = XBORDER + leftsize * CHARWIDTH + XSEP;
    rightsize = XBORDER + rightsize * CHARWIDTH;
    width = leftsize + rightsize;
    if (width < MINWIDTH)
	width = MINWIDTH;
    height = 2 * YBORDER + (i + 1) * LINEHEIGHT;
    fprintf(file, "%sBox  DIALOG %i, %i, %i, %i\n", item->shortname, 52,
	    57, width, height);
    fprintf(file, "STYLE DS_MODALFRAME | WS_CAPTION | WS_SYSMENU\n");
    fprintf(file, "CAPTION \"%s\"\n", item->name);
    fprintf(file, "FONT 8, \"MS Shell Dlg\"\n");
    fprintf(file, "BEGIN\n");
    for (i = 0; dialog[i].question; i++) {
	fprintf(file, "  RTEXT \"%s\", %i, %i, %i, %i, %i, WS_GROUP\n",
		dialog[i].question,
		i * PERITEM + QUESTIONSTART,
		0, YBORDER + i * LINEHEIGHT, leftsize - XSEP, TEXTHEIGHT);
	switch (dialog[i].type) {
	case DIALOG_INT:
	case DIALOG_FLOAT:
	case DIALOG_STRING:
	case DIALOG_KEYSTRING:
	    fprintf(file,
		    "  EDITTEXT %i, %i, %i, %i, %i, ES_AUTOHSCROLL | WS_TABSTOP\n",
		    i * PERITEM + ITEMSTART, leftsize,
		    i * LINEHEIGHT + YBORDER, rightsize - XSEP,
		    TEXTHEIGHT);
	    break;
	case DIALOG_COORD:
	    fprintf(file,
		    "  EDITTEXT %i, %i, %i, %i, %i, ES_AUTOHSCROLL | WS_TABSTOP\n",
		    i * PERITEM + ITEMSTART, leftsize,
		    i * LINEHEIGHT + YBORDER,
		    (rightsize - XSEP - 4 * CHARWIDTH) / 2, TEXTHEIGHT);
	    fprintf(file,
		    "  EDITTEXT %i, %i, %i, %i, %i, ES_AUTOHSCROLL | WS_TABSTOP\n",
		    i * PERITEM + ITEMSTART + 1,
		    leftsize + (rightsize - XSEP + CHARWIDTH) / 2,
		    i * LINEHEIGHT + YBORDER,
		    (rightsize - XSEP - 4 * CHARWIDTH) / 2, TEXTHEIGHT);
	    fprintf(file, "  RTEXT \"+\", -1, %i, %i, %i, %i\n",
		    leftsize + (rightsize - XSEP - 2 * CHARWIDTH) / 2,
		    YBORDER + i * LINEHEIGHT, CHARWIDTH, TEXTHEIGHT);
	    fprintf(file, "  RTEXT \"i\", -1, %i, %i, %i, %i\n",
		    leftsize + rightsize - XSEP - CHARWIDTH,
		    YBORDER + i * LINEHEIGHT, CHARWIDTH, TEXTHEIGHT);
	    break;
	case DIALOG_CHOICE:
	    fprintf(file,
		    "  COMBOBOX %i, %i, %i, %i, %i, CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP\n",
		    i * PERITEM + ITEMSTART, leftsize,
		    i * LINEHEIGHT + YBORDER, rightsize - XSEP,
		    TEXTHEIGHT * 10);
	    break;
	case DIALOG_IFILE:
	case DIALOG_OFILE:
#define BROWSEWIDTH ((strlen("Browse")+1)*CHARWIDTH)
	    fprintf(file,
		    "  EDITTEXT %i, %i, %i, %i, %i, ES_AUTOHSCROLL | WS_TABSTOP\n",
		    i * PERITEM + ITEMSTART, leftsize,
		    i * LINEHEIGHT + YBORDER,
		    rightsize - 2 * XSEP - BROWSEWIDTH, TEXTHEIGHT);
	    fprintf(file,
		    "  PUSHBUTTON \"Browse\", %i, %i, %i, %i, %i, WS_GROUP\n",
		    i * PERITEM + ITEMSTART + 1,
		    leftsize + rightsize - XSEP - BROWSEWIDTH,
		    i * LINEHEIGHT + YBORDER, BROWSEWIDTH, TEXTHEIGHT);
	    break;
	}
    }
    fprintf(file,
	    "  DEFPUSHBUTTON \"&OK\", %i, %i, %i, %i, %i, WS_GROUP\n", OK,
	    XSEP / 2, i * LINEHEIGHT + YBORDER, width / 3 - XSEP, 14);
    fprintf(file, "  PUSHBUTTON \"&Cancel\", %i, %i, %i, %i, %i\n", CANCEL,
	    width / 3 + XSEP / 2, i * LINEHEIGHT + YBORDER,
	    width / 3 - XSEP, 14);
    fprintf(file, "  PUSHBUTTON \"&Help\", %i, %i, %i, %i, %i\n", HELP,
	    2 * width / 3 + XSEP / 2, i * LINEHEIGHT + YBORDER,
	    width / 3 - XSEP, 14);
    fprintf(file, "END\n");
}

void win32_genresources(struct uih_context *uih)
{
    file = fopen("xaos.dlg", "w");
    menu_forall(uih, win32_outputdialog);
    fclose(file);

}

void win32_dialog(struct uih_context *uih, CONST char *name)
{
    CONST menuitem *item = menu_findcommand(name);
    CONST menudialog *dialog;
    int nitems;
    char s[256];

    if (!item)
	return;
    dialog = menu_getdialog(uih, item);
    if (!dialog)
	return;
    for (nitems = 0; dialog[nitems].question; nitems++);
    if (nitems == 1
	&& (dialog[0].type == DIALOG_IFILE
	    || dialog[0].type == DIALOG_OFILE))
	win32_filedialog(uih, item, dialog);
    else {
	struct dialogrecord *r = calloc(sizeof(*r), 1);
	r->next = firstdialog;
	firstdialog = r;
	r->prev = NULL;
	r->item = item;
	r->nitems = nitems;
	r->dialog = dialog;
	r->c = uih;
	sprintf(s, "%sBox", item->shortname);
	if (DialogBox(hInstance, s, hWnd, DialogHandler) == -1) {
	    /*r->windialog=CreateDialog (hInstance, s, hWnd, DialogHandler);
	       if(r->windialog==NULL) { */
	    x_message("Failed to create dialog %s", item->shortname);
	    win32_freedialog(r);
	}
	win32_freedialog(r);
	/*x_message("Dialog (%s %i %s) not implemented", name, nitems, dialog[0].question); */
    }
}
