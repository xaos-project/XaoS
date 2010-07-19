#ifndef _plan9_
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#endif
#endif

#include <config.h>
#include <filter.h>
#include <fractal.h>
#include <ui_helper.h>
#include <xmenu.h>
#include <grlib.h>
#include <ui.h>
#include <xerror.h>
#include <misc-f.h>
#include "uiint.h"

#ifdef HAVE_GETTEXT
#include <libintl.h>
#else
#define gettext(STRING) STRING
#endif

static char **dirs;
static char **sdirs;
static int ndirs;
static char **names;
static char **snames;
static int nnames;

struct ui_textdata *dir, *filename;
static char lastdir[256];
static char *currdir;

static struct uih_window *filew;
static const char *mask;
static void (*callback) (const char *name, int succ);
int filevisible;
static int filex, filey, filewidth, fileheight;

#define ADIR 0
#define AFILELIST 1
#define ADIRLIST 2
#define AFILE 3
#define AOK 4

#define DIRSTART (filey+BORDERHEIGHT)
#define LISTSTART (DIRSTART+BUTTONHEIGHT+BORDERHEIGHT)
#define LISTEND (FILESTART-BORDERHEIGHT)
#define FILESTART (OKSTART-BORDERHEIGHT-BUTTONHEIGHT)
#define OKSTART (filey+fileheight-BORDERHEIGHT-BUTTONHEIGHT)
#define SCROLLWIDTH 10

#define LISTWIDTH ((filewidth-10*BORDERWIDTH-2*SCROLLWIDTH)/2)

#define NVISIBLE ((LISTEND-LISTSTART-2*BORDERHEIGHT)/xtextheight(uih->image, uih->font))

static int selectedname;
static int selecteddir;
static int namestart;
static int dirstart;

static int pressedbutton;
static int activebutton;
static int active;

static void ui_freenames(void)
{
    int i;
    selectedname = 0;
    selecteddir = 0;
    if (nnames) {
	for (i = 0; i < nnames; i++)
	    free(names[i]), free(snames[i]);
	free(names);
	nnames = 0;
    }
    if (ndirs) {
	for (i = 0; i < ndirs; i++)
	    free(dirs[i]), free(sdirs[i]);
	free(dirs);
	ndirs = 0;
    }
    if (snames)
	free(snames), snames = NULL;
    if (sdirs)
	free(sdirs), sdirs = NULL;
}

static int compar(const void *a, const void *b)
{
    return (strcmp(*(const char **) a, *(const char **) b));
}

static char **ui_mksnames(int nnames, char **names, int width)
{
    char **snames = NULL;
    int i;
    if (nnames) {
	qsort(names, nnames, sizeof(*names),	/*(int (*)(const void *, const
						   void *))strcmp */ compar);
	snames = (char **) malloc(sizeof(*snames) * nnames);
	for (i = 0; i < nnames; i++) {
	    if (xtextwidth(uih->image, uih->font, names[i]) <= width)
		snames[i] = mystrdup(names[i]);
	    else {
		int y;
		int swidth = 0;
		int len = (int) strlen(names[i]);
		snames[i] = (char *) malloc(strlen(names[i]) + 2);
		for (y = len - 4; y < len; y++)
                    swidth += xtextcharw(uih->image, uih->font, names[i][y]);
                swidth += xtextcharw(uih->image, uih->font, '|');
		y = 0;
		while (swidth < width) {
		    snames[i][y] = names[i][y];
                    swidth += xtextcharw(uih->image, uih->font, names[i][y]);
		    y++;
		}
		snames[i][y - 1] = '|';
		snames[i][y] = 0;
		strcat(snames[i], names[i] + len - 4);
	    }
	}
    }
    return (snames);
}

static void ui_buildnames(int width)
{
    ui_freenames();
    xio_getfiles(currdir, &names, &dirs, &nnames, &ndirs);
    if (snames)
	free(snames), snames = NULL;
    if (sdirs)
	free(sdirs), sdirs = NULL;
    snames = ui_mksnames(nnames, names, width);
    sdirs = ui_mksnames(ndirs, dirs, width);
}

void ui_closefilesel(int success)
{
    char *text =
	(char *) malloc((int) strlen(filename->text) +
			(int) strlen(currdir) + 3);
    filevisible = 0;
    uih_removew(uih, filew);
    ui_freenames();
    sprintf(text, "%s" XIO_PATHSEPSTR "%s", currdir, filename->text);
    ui_closetext(dir);
    ui_closetext(filename);
    strcpy(lastdir, currdir);
    free(currdir);
    callback(text, success);
    free(text);
    uih->display = 1;
    ui_freenames();
}

static void
filepos(uih_context * c, int *x, int *y, int *w, int *h, void *data)
{
    *x = filex;
    *y = filey;
    *w = filewidth;
    *h = fileheight;
}

static void drawfile(uih_context * c, void *data)
{
    int i;
    int ypos;
    int h = xtextheight(uih->image, uih->font);
    uih_drawborder(uih, filex + BORDERWIDTH, DIRSTART,
		   filewidth - 2 * BORDERWIDTH, BUTTONHEIGHT,
		   BORDER_PRESSED | BORDER_LIGHT);
    uih_drawborder(uih, filex + BORDERWIDTH, FILESTART,
		   filewidth - 2 * BORDERWIDTH, BUTTONHEIGHT,
		   BORDER_PRESSED | BORDER_LIGHT);

    ui_drawbutton("OK", (pressedbutton == 0), active == AOK
		  && activebutton == 0, filex + BORDERWIDTH,
		  filex + filewidth / 2 - BORDERWIDTH, OKSTART);
    ui_drawbutton(gettext("Cancel"), (pressedbutton == 1), active == AOK
		  && activebutton == 1,
		  filex + filewidth / 2 + BORDERWIDTH,
		  filex + filewidth - BORDERWIDTH, OKSTART);

    uih_drawborder(uih, filex + BORDERWIDTH, LISTSTART,
		   LISTWIDTH + 3 * BORDERWIDTH + SCROLLWIDTH,
		   LISTEND - LISTSTART, BORDER_PRESSED);
    uih_drawborder(uih, filex + filewidth / 2 + BORDERWIDTH, LISTSTART,
		   LISTWIDTH + 3 * BORDERWIDTH + SCROLLWIDTH,
		   LISTEND - LISTSTART, BORDER_PRESSED);
    ypos = LISTSTART + BORDERHEIGHT;
    for (i = 0; ypos + h < LISTEND && i + namestart < nnames; i++) {
	if (i + namestart == selectedname) {
	    xrectangle(uih->image, filex + 2 * BORDERWIDTH, ypos,
		       LISTWIDTH, h,
		       (uih->palette->
			type & BITMAPS) ? BGCOLOR(uih) :
		       LIGHTGRAYCOLOR(uih));
	}
	if (uih->palette->type & BITMAPS)
	    xprint(uih->image, uih->font, filex + 2 * BORDERWIDTH, ypos,
		   snames[i + namestart], 
		   i + namestart ==
		   selectedname ? FGCOLOR(uih) : BGCOLOR(uih),
		   BGCOLOR(uih), TEXT_PRESSED);
	else
	    xprint(uih->image, uih->font, filex + 2 * BORDERWIDTH, ypos,
		   snames[i + namestart], 
		   (i + namestart) == selectedname
		   && active == AFILELIST ? SELCOLOR(uih) : FGCOLOR(uih),
		   BGCOLOR(uih), 0);
	ypos += h;
    }
    if (nnames) {
	int xstart = (namestart) * (LISTEND - LISTSTART) / nnames;
	int xend = (namestart + NVISIBLE) * (LISTEND - LISTSTART) / nnames;
	if (xstart > (LISTEND - LISTSTART - 2 * BORDERHEIGHT))
	    xstart = LISTEND - LISTSTART - 2 * BORDERHEIGHT;
	if (xend > (LISTEND - LISTSTART - 2 * BORDERHEIGHT))
	    xend = LISTEND - LISTSTART - 2 * BORDERHEIGHT;
	uih_drawborder(uih, filex + LISTWIDTH + 3 * BORDERWIDTH, LISTSTART + xstart + BORDERHEIGHT, SCROLLWIDTH, xend - xstart,	/*1|BORDER_LIGHT */
		       0);
    }

    ypos = LISTSTART + BORDERHEIGHT;
    for (i = 0; ypos + h < LISTEND && i + dirstart < ndirs; i++) {
	if (i + dirstart == selecteddir) {
	    xrectangle(uih->image, filex + filewidth / 2 + 2 * BORDERWIDTH,
		       ypos, LISTWIDTH, h,
		       (uih->palette->type & BITMAPS) ? BGCOLOR(uih) :
		       LIGHTGRAYCOLOR(uih));
	}
	if (uih->palette->type & BITMAPS)
	    xprint(uih->image, uih->font,
		   filex + filewidth / 2 + 2 * BORDERWIDTH, ypos,
		   sdirs[i + dirstart], 
		   i + dirstart ==
		   selecteddir ? FGCOLOR(uih) : BGCOLOR(uih), BGCOLOR(uih),
		   TEXT_PRESSED);
	else
	    xprint(uih->image, uih->font,
		   filex + filewidth / 2 + 2 * BORDERWIDTH, ypos,
		   sdirs[i + dirstart], 
		   (i + dirstart) == selecteddir
		   && active == ADIRLIST ? SELCOLOR(uih) : FGCOLOR(uih),
		   BGCOLOR(uih), 0);
	ypos += h;
    }
    if (ndirs) {
	int xstart = (dirstart) * (LISTEND - LISTSTART) / ndirs;
	int xend = (dirstart + NVISIBLE) * (LISTEND - LISTSTART) / ndirs;
	if (xstart > (LISTEND - LISTSTART - 2 * BORDERHEIGHT))
	    xstart = LISTEND - LISTSTART - 2 * BORDERHEIGHT;
	if (xend > (LISTEND - LISTSTART - 2 * BORDERHEIGHT))
	    xend = LISTEND - LISTSTART - 2 * BORDERHEIGHT;
	uih_drawborder(uih, filex + filewidth / 2 + LISTWIDTH + 3 * BORDERWIDTH, LISTSTART + xstart + BORDERHEIGHT, SCROLLWIDTH, xend - xstart,	/*1|BORDER_LIGHT */
		       0);
    }
    ui_drawtext(filename, active == AFILE);
    ui_drawtext(dir, active == ADIR);
}

static void setname(int name)
{
    ui_closetext(filename);
    filename =
	ui_opentext(filex + 2 * BORDERWIDTH, FILESTART + BORDERHEIGHT,
		    filewidth - 4 * BORDERWIDTH, names[name]);
}

#ifdef _WIN32
#define DRIVES
#endif
#ifdef DJGPP
#define DRIVES
#endif
static void setdir(int name)
{
    char *dirstring = dirs[name];
    char *s = NULL;
    if (dirstring[0] == '.' && !dirstring[1]) {
	/*do nothing */
	s = mystrdup(currdir);
    } else if (dirstring[0] && dirstring[1] && dirstring[0] == '.'
	       && dirstring[1] == '.' && !dirstring[2]) {
	int i = (int) strlen(currdir);
	s = (char *) malloc((int) strlen(dirstring) +
			    (int) strlen(currdir) + 2);
	strcpy(s, currdir);
	for (;
	     i >= 0 && s[i] != '/' && s[i] != '\\' && s[i] != XIO_PATHSEP;
	     i--);
	if (i < 0)
	    free(s), s = NULL;
	else
	    s[i] = 0;
    }
    if (s == NULL) {
	int i = (int) strlen(currdir);
	s = (char *) malloc((int) strlen(dirstring) +
			    (int) strlen(currdir) + 2);
	strcpy(s, currdir);
	if (currdir[i - 1] != '/' && currdir[i - 1] != '\\'
	    && currdir[i - 1] != XIO_PATHSEP)
	    strcat(s, XIO_PATHSEPSTR);
	strcat(s, dirstring);
	if (!s[0])
	    s[0] = XIO_PATHSEP, s[1] = 0;
    }
    free(currdir);
#ifdef DRIVES
    if (strlen(s) == 2
	&& ((s[0] >= 'a' && s[0] <= 'z') || (s[0] >= 'A' && s[0] <= 'Z'))
	&& s[1] == ':')
	s[2] = XIO_PATHSEP, s[3] = 0;
#endif
    currdir = s;
    ui_closetext(dir);
    dir =
	ui_opentext(filex + 2 * BORDERWIDTH, DIRSTART + BORDERHEIGHT,
		    filewidth - 4 * BORDERWIDTH, currdir);
    ui_freenames();
    ui_buildnames(LISTWIDTH);
    dirstart = 0;
    selecteddir = 0;
    namestart = 0;
    selectedname = 0;
    uih->display = 1;
}

static void setexactdir(const char *dirstring)
{
    free(currdir);
    currdir = mystrdup(dirstring);
    ui_closetext(dir);
    dir =
	ui_opentext(filex + 2 * BORDERWIDTH, DIRSTART + BORDERHEIGHT,
		    filewidth - 4 * BORDERWIDTH, currdir);
    ui_freenames();
    ui_buildnames(LISTWIDTH);
    dirstart = 0;
    selecteddir = 0;
    namestart = 0;
    selectedname = 0;
    uih->display = 1;
}

int ui_keyfilesel(int k)
{
    if (!filevisible)
	return 0;
    if (k == UIKEY_ESC)
	ui_closefilesel(0);
    else
	switch (active) {
	default:
	    active = AFILE;
	    uih->display = 1;
	    break;
	case ADIR:
	    if (ui_textkey(dir, k))
		break;
	    if (k == '\n' || k == 13) {
		active = AFILE;
		setexactdir(dir->text);
		uih->display = 1;
	    }
	    if (k == UIKEY_UP) {
		active = AOK;
		uih->display = 1;
		activebutton = 1;
	    }
	    if (k == '\t' || k == UIKEY_DOWN) {
		active++;
		uih->display = 1;
	    }
	    break;
	case AFILELIST:
	    switch (k) {
	    case '\t':
	    case UIKEY_RIGHT:
		uih->display = 1;
		active = ADIRLIST;
		break;
	    case UIKEY_LEFT:
		uih->display = 1;
		active = ADIR;
		break;
	    case UIKEY_UP:
		if (selectedname) {
		    uih->display = 1;
		    selectedname--;
		    if (selectedname < namestart)
			namestart = selectedname;
		}
		break;
	    case UIKEY_DOWN:
		if (selectedname < nnames - 1) {
		    uih->display = 1;
		    selectedname++;
		    if (selectedname >= namestart + NVISIBLE)
			namestart = selectedname - NVISIBLE + 1;
		}
		break;
	    case UIKEY_HOME:
		if (selectedname) {
		    uih->display = 1;
		    selectedname = namestart = 0;
		}
		break;
	    case UIKEY_END:
		if (selectedname < nnames - 1) {
		    uih->display = 1;
		    selectedname = nnames - 1;;
		    if (selectedname >= namestart + NVISIBLE)
			namestart = selectedname - NVISIBLE + 1;
		}
		break;
	    case '\n':
	    case 13:
		setname(selectedname);
		uih->display = 1;
		active = AFILE;
		break;
	    }
	    break;
	case ADIRLIST:
	    switch (k) {
	    case '\t':
	    case UIKEY_RIGHT:
		uih->display = 1;
		active = AFILE;
		break;
	    case UIKEY_LEFT:
		uih->display = 1;
		active = AFILELIST;
		break;
	    case UIKEY_UP:
		if (selecteddir) {
		    uih->display = 1;
		    selecteddir--;
		    if (selecteddir < dirstart)
			dirstart = selecteddir;
		}
		break;
	    case UIKEY_DOWN:
		if (selecteddir < ndirs - 1) {
		    uih->display = 1;
		    selecteddir++;
		    if (selecteddir >= dirstart + NVISIBLE)
			dirstart = selecteddir - NVISIBLE + 1;
		}
		break;
	    case UIKEY_HOME:
		if (selecteddir) {
		    uih->display = 1;
		    selecteddir = dirstart = 0;
		}
		break;
	    case UIKEY_END:
		if (selecteddir < ndirs - 1) {
		    uih->display = 1;
		    selecteddir = ndirs - 1;;
		    if (selecteddir >= dirstart + NVISIBLE)
			dirstart = selecteddir - NVISIBLE + 1;
		}
		break;
	    case '\n':
	    case 13:
		setdir(selecteddir);
		uih->display = 1;
		break;
	    }
	    break;
	case AFILE:
	    if (ui_textkey(filename, k))
		break;
	    if (k == '\t' || k == UIKEY_DOWN) {
		active++;
		uih->display = 1;
		activebutton = 0;
	    }
	    if (k == UIKEY_UP) {
		active--;
		uih->display = 1;
	    }

	    if (k == '\n' || k == 13) {
		ui_closefilesel(1);
	    }
	    break;
	case AOK:
	    if (k == '\n' || k == 13)
		ui_closefilesel(!activebutton);
	    if (k == '\t' || k == UIKEY_RIGHT || k == UIKEY_DOWN) {
		uih->display = 1;
		activebutton++;
		if (activebutton > 2) {
		    activebutton = 0;
		    active = ADIR;
		}
	    }
	    if (k == UIKEY_LEFT || k == UIKEY_UP) {
		uih->display = 1;
		activebutton--;
		if (activebutton < 0) {
		    activebutton = 0;
		    active = AFILE;
		}
	    }
	}
    return 1;
}

int ui_mousefilesel(int x, int y, int buttons, int flags)
{
    static int grabbed = -1;
    if (!filevisible)
	return 0;
    if (grabbed >= 0 && (flags & MOUSE_DRAG)) {
	if (!grabbed) {
	    int pos;
	    pos =
		(y - LISTSTART) * nnames / (LISTEND - LISTSTART -
					    2 * BORDERHEIGHT);
	    if (pos >= nnames - NVISIBLE)
		pos = nnames - NVISIBLE;
	    if (pos < 0)
		pos = 0;
	    if (pos != namestart) {
		namestart = pos;
		uih->display = 1;
		if (selectedname < pos)
		    selectedname = pos;
		if (selectedname >= pos + NVISIBLE)
		    selectedname = pos + NVISIBLE - 1;
	    }
	} else {
	    int pos;
	    pos =
		(y - LISTSTART) * ndirs / (LISTEND - LISTSTART -
					   2 * BORDERHEIGHT);
	    if (pos >= ndirs - NVISIBLE)
		pos = ndirs - NVISIBLE;
	    if (pos < 0)
		pos = 0;
	    if (pos != dirstart) {
		dirstart = pos;
		uih->display = 1;
		if (selecteddir < pos)
		    selecteddir = pos;
		if (selecteddir >= pos + NVISIBLE)
		    selecteddir = pos + NVISIBLE - 1;
	    }
	}
    } else
	grabbed = -1;
    if (x < filex || y < filey || x > filex + filewidth
	|| y > filex + fileheight) {
	if (flags & MOUSE_PRESS)
	    ui_closefilesel(0);
	return 1;
    }
    if (y < LISTSTART) {
	if (pressedbutton != -1)
	    pressedbutton = -1, uih->display = 1;
	if ((flags & MOUSE_MOVE) && active != ADIR)
	    active = ADIR, uih->display = 1;
	if (flags & MOUSE_PRESS)
	    ui_textmouse(dir, x, y);
    } else if (y < LISTEND) {
	int mouseat = 0;
	if (pressedbutton != -1)
	    pressedbutton = -1, uih->display = 1;
	if (x > filex + filewidth / 2)
	    mouseat = 1, x -= filewidth / 2;
	x -= filex;
	if (flags & MOUSE_MOVE) {
	    if (!mouseat && active != AFILELIST)
		active = AFILELIST, uih->display = 1;
	    if (mouseat && active != ADIRLIST)
		active = ADIRLIST, uih->display = 1;
	}
	if (x > LISTWIDTH && (flags & MOUSE_PRESS))
	    grabbed = mouseat;
	else {
	    if (flags & MOUSE_PRESS) {
		int atitem =
		    (y - LISTSTART -
		     BORDERHEIGHT) / xtextheight(uih->image, uih->font);
		if (atitem < 0)
		    atitem = 0;
		if (!mouseat) {
		    atitem += namestart;
		    if (atitem < nnames) {
			if (atitem == selectedname
			    && !strcmp(names[selectedname],
				       filename->text)) {
			    ui_closefilesel(1);
			} else {
			    selectedname = atitem;
			    uih->display = 1;
			    setname(selectedname);
			}
		    }
		} else {
		    atitem += dirstart;
		    if (atitem < ndirs) {
			selecteddir = atitem;
			uih->display = 1;
			setdir(selecteddir);
		    }
		}
	    }
	}
    } else if (y < OKSTART) {
	if (pressedbutton != -1)
	    pressedbutton = -1, uih->display = 1;
	/*exit(1); */
	if ((flags & MOUSE_MOVE) && active != AFILE)
	    active = AFILE, uih->display = 1;
	if (flags & MOUSE_PRESS)
	    ui_textmouse(filename, x, y);
    } else {
	int mouseat = 0;
	if (x > filex + filewidth / 2)
	    mouseat = 1;
	if (flags & MOUSE_PRESS) {
	    if (active != AOK)
		active = AOK, uih->display = 1;
	    if (activebutton != mouseat || pressedbutton != mouseat)
		activebutton = pressedbutton = mouseat, uih->display = 1;
	}
	if ((flags & MOUSE_MOVE) && pressedbutton != mouseat)
	    uih->display = 1, pressedbutton = -1, active =
		AOK, activebutton = mouseat;
	if ((flags & MOUSE_RELEASE) && pressedbutton == mouseat)
	    ui_closefilesel(!mouseat);
    }
    return 1;
}

void
ui_buildfilesel(const char *f, const char *m,
		void (*c) (const char *, int))
{
    if (filevisible) {
	x_fatalerror("Internal error!");
    }
    pressedbutton = activebutton = active = -1;
    if (lastdir[0] == 0)
	getcwd(lastdir, 256);
    lastdir[255] = 0;
    currdir = mystrdup(lastdir);
    callback = c;
    active = AFILE;
    filex = 0;
    filey = 0;
    filewidth = uih->image->width;
    fileheight = uih->image->height;
    namestart = dirstart = 0;
    mask = m;
    dir =
	ui_opentext(filex + 2 * BORDERWIDTH, DIRSTART + BORDERHEIGHT,
		    filewidth - 4 * BORDERWIDTH, lastdir);
    filename =
	ui_opentext(filex + 2 * BORDERWIDTH, FILESTART + BORDERHEIGHT,
		    filewidth - 4 * BORDERWIDTH, f);
    filevisible = 1;
    ui_buildnames(LISTWIDTH);
    filew = uih_registerw(uih, filepos, drawfile, 0, DRAWBORDER);
}
#endif
