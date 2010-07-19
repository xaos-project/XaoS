#include <config.h>
#ifdef _plan9_
#include <u.h>
#include <libc.h>
#else
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include <string.h>
#endif

#include <fconfig.h>
#include <filter.h>
#include <fractal.h>
#include <ui_helper.h>
#include <ui.h>
#include <xshl.h>
#include <xmenu.h>
#include <grlib.h>
#include <misc-f.h>
#include "uiint.h"
#define HISTORYSIZE 10
static int historypos;
static struct xshl_line *lines;
static int helpwidth, helpheight, helpx, helpy;
static struct uih_window *helpw;
static int textheight;
static char *ui_helppage[HISTORYSIZE];
static int ui_helppos[HISTORYSIZE];
int helpvisible;

static struct xshl_item *presseditem;
static struct xshl_item *selecteditem;
static int pressedline;
static int selectedline;
#define SCROOLSIZE 10
#define XPOS(xp) (2 * BORDERWIDTH + (xp))
#define YPOS(yp) (2*BORDERWIDTH+ (yp)-ui_helppos[historypos]+10)
#define SCROOLSTART (helpx+helpwidth-WBORDERS)
#define WBORDERS (4*BORDERWIDTH+SCROOLSIZE)
#define HBORDERS (xtextheight(uih->image, uih->font)+8*BORDERHEIGHT)
#define WIDTH  (uih->image->width-WBORDERS)
#define HEIGHT (uih->image->height-HBORDERS)

static int pressedbutton = -1;
static int selectedbutton = -1;

#define NBUTTONS 3
const char *const names[] = { "OK", "Back", "Main" };

#define BUTTONSTART(i) (helpx+BORDERWIDTH+(helpwidth-2*BORDERWIDTH)/NBUTTONS*(i))
static void ui_build_help(char *name);
static void ui_backhelp(void)
{
    historypos--;
    if (historypos < 0)
	historypos = HISTORYSIZE - 1;
    ui_build_help(ui_helppage[historypos]);
}

static void do_button(int i)
{
    switch (i) {
    case 0:
	ui_close_help();
	break;
    case 1:
	ui_backhelp();
	break;
    case 2:
	ui_help("main");
	break;
    }
}

void ui_close_help(void)
{
    if (helpw != NULL) {
	xshl_free(lines);
	helpvisible = 0;
	uih_removew(uih, helpw);
	uih->display = 1;
	helpw = NULL;
    }
}

static int getwidth(void *data, int flags, const char *text)
{
    if (uih->image->flags & AAIMAGE)
        return (xtextwidth(uih->image, uih->font, text));
    return (xtextwidth(uih->image, uih->font, text) - 1);
}

static void
helpsize(struct uih_context *c, int *x, int *y, int *width, int *height,
	 void
	 *data)
{
    *x = helpx;
    *y = helpy;
    *width = helpwidth;
    *height = helpheight;
}

static void drawhelp(struct uih_context *c, void *data)
{
    int i = 0;
    int y;
    int percentx, percenty;
    struct xshl_item *curritem;
    uih_drawborder(uih, helpx + BORDERWIDTH, helpy + BORDERHEIGHT,
		   helpwidth - WBORDERS + SCROOLSIZE,
		   helpheight - HBORDERS, BORDER_PRESSED);

    if (ui_helppos[historypos] > textheight - helpheight / 2 + HBORDERS)
	ui_helppos[historypos] = textheight - helpheight / 2 + HBORDERS;
    if (ui_helppos[historypos] < 0)
	ui_helppos[historypos] = 0;

    /*draw scroolbar */
    percentx =
	(helpheight - HBORDERS) * ui_helppos[historypos] / textheight;
    percenty =
	(helpheight - HBORDERS) * (ui_helppos[historypos] + helpheight -
				   HBORDERS) / textheight;
    if (percentx < BORDERHEIGHT)
	percentx = BORDERHEIGHT;
    if (percenty < BORDERHEIGHT)
	percenty = BORDERHEIGHT;
    if (percentx > helpheight - HBORDERS - BORDERHEIGHT)
	percentx = helpheight - HBORDERS - BORDERHEIGHT;
    if (percenty > helpheight - HBORDERS - BORDERHEIGHT)
	percenty = helpheight - HBORDERS - BORDERHEIGHT;
    uih_drawborder(uih, SCROOLSTART, helpy + BORDERHEIGHT + percentx,
		   SCROOLSIZE - 1, percenty - percentx, 0);
    for (i = 0; i < NBUTTONS; i++) {
	ui_drawbutton(names[i], pressedbutton == i, selectedbutton == i,
		      BUTTONSTART(i) + BORDERWIDTH,
		      BUTTONSTART(i + 1) - BORDERWIDTH,
		      helpy + helpheight - BUTTONHEIGHT - BORDERHEIGHT);
    }


    i = 0;
    while (YPOS(lines[i].y) < 2 * BORDERWIDTH) {
	i++;
	if (lines[i].y < 0)
	    return;
    }
    while ((y =
	    YPOS(lines[i].y)) <
           helpheight - HBORDERS - xtextheight(uih->image, uih->font)
	   && lines[i].y >= 0) {
	curritem = lines[i].first;
	while (curritem != NULL) {
	    unsigned int bgcolor = BGCOLOR(uih);
	    unsigned int fgcolor = /*FGCOLOR (uih) */ LIGHTGRAYCOLOR2(uih);
	    int flags = 0;
	    int x = XPOS(curritem->x);

	    if (curritem->c.flags & (XSHL_BIG | XSHL_RED))
		fgcolor = SELCOLOR(uih);
	    else if (curritem->c.flags & (XSHL_EMPH | XSHL_MONOSPACE))
		fgcolor =
		    uih->image->
		    flags & AAIMAGE ? SELCOLOR(uih) : FGCOLOR(uih);
	    else if (curritem->c.flags & (XSHL_BLACK))
		bgcolor = fgcolor = BGCOLOR(uih), flags = TEXT_PRESSED;
	    else
		bgcolor = fgcolor, flags |= TEXT_PRESSED;
	    if (uih->palette->type & BITMAPS) {
		flags = TEXT_PRESSED;
		fgcolor = BGCOLOR(uih);
	    }
	    if (curritem->c.linktext != NULL) {
		if (uih->palette->type & BITMAPS) {
		    if (curritem == presseditem
			|| curritem == selecteditem)
			fgcolor =
			    FGCOLOR(uih), xrectangle(uih->image, x + helpx,
						     y + helpy,
						     curritem->width,
                                                     xtextheight(uih->image,
                                                                 uih->font),
						     BGCOLOR(uih));
		    else
			xhline(uih->image, x + helpx,
                               y + helpy + xtextheight(uih->image, uih->font) - 1,
			       curritem->width, BGCOLOR(uih));
		} else {
		    if (uih->image->flags & AAIMAGE)
			fgcolor = curritem == presseditem
			    || curritem ==
			    selecteditem ? SELCOLOR(uih) : BGCOLOR(uih);
		    else {
			int i;
			i = strlen(curritem->c.linktext);
			if (i > 3
			    && !strcmp(".xaf",
				       curritem->c.linktext + i - 4))
			    xhline(uih->image, x + helpx,
                                   y + helpy + xtextheight(uih->image, uih->font) - 1,
				   curritem->width, curritem == presseditem
				   || curritem ==
				   selecteditem ? SELCOLOR(uih) :
				   SELCOLOR(uih));
			else
			    xhline(uih->image, x + helpx,
                                   y + helpy + xtextheight(uih->image, uih->font) - 1,
				   curritem->width, curritem == presseditem
				   || curritem ==
				   selecteditem ? SELCOLOR(uih) :
				   LIGHTGRAYCOLOR2(uih));
			xhline(uih->image, x + helpx + 1,
                               y + helpy + xtextheight(uih->image, uih->font) - 0,
			       curritem->width, BGCOLOR(uih));
		    }
		    flags = 0;
		    bgcolor = BGCOLOR(uih);
		    if (fgcolor == LIGHTGRAYCOLOR2(uih))
			fgcolor = FGCOLOR(uih);
		    if (curritem == presseditem
			|| curritem == selecteditem)
			fgcolor = SELCOLOR(uih);
		}
	    }
	    xprint(uih->image, uih->font, x + helpx, y + helpy,
		   curritem->text, fgcolor, bgcolor, flags);
	    curritem = curritem->next;
	}
	i++;
    }
}

int ui_helpkeys(int key)
{
    int i;
    if (helpw == NULL)
	return 0;
    switch (key) {
    case 'h':
	ui_close_help();
	ui_menu("tutor");
	return 1;
    case 'm':
	ui_help("main");
	return 1;
    case UIKEY_DOWN:
    case 'j':
	ui_helppos[historypos] += 8, uih->display = 1;
	presseditem = NULL;
	selecteditem = NULL;
	break;
    case UIKEY_UP:
    case 'k':
	ui_helppos[historypos] -= 8, uih->display = 1;
	presseditem = NULL;
	selecteditem = NULL;
	break;
    case UIKEY_PGDOWN:
    case '+':
    case 'f':
    case ' ':
	ui_helppos[historypos] +=
	    helpheight - HBORDERS - 32, uih->display = 1;
	presseditem = NULL;
	selecteditem = NULL;
	break;
    case UIKEY_PGUP:
    case '-':
	ui_helppos[historypos] -=
	    helpheight - HBORDERS - 32, uih->display = 1;
	presseditem = NULL;
	selecteditem = NULL;
	break;
    case 'b':
    case 1:
	ui_backhelp();
	break;
    case UIKEY_ESC:
    case 'q':
    case 'c':
    case 'o':
	ui_close_help();
	break;
    case UIKEY_TAB:
    case UIKEY_RIGHT:
	uih->display = 1;
	if (selectedbutton < 0 || selectedbutton == NBUTTONS - 1) {
	    if (selecteditem == NULL) {
		i = 0;
		while (YPOS(lines[i].y) < 2 * BORDERWIDTH) {
		    i++;
		    if (lines[i].y < 0)
			break;
		}
		selecteditem = lines[i].first;
	    } else
		i = selectedline, selecteditem = selecteditem->next;
	    for (;
		 lines[i].y >= 0
		 && YPOS(lines[i].y) < helpheight - HBORDERS; i++) {
		selectedline = i;
		while (selecteditem != NULL) {
		    if (selecteditem->c.linktext != NULL) {
			selectedbutton = -1;
			return 0;
		    }
		    selecteditem = selecteditem->next;
		}
		selecteditem = lines[i + 1].first;
	    }
	    selecteditem = NULL;
	    selectedbutton = 0;
	} else
	    selectedbutton++;
	break;
    case 13:
    case '\n':
	if (selecteditem != NULL) {
	    ui_help(selecteditem->c.linktext);
	    return 1;
	}
	if (selectedbutton >= 0)
	    do_button(selectedbutton);
    }
    return 1;
}

int ui_helpmouse(int x, int y, int buttons, int flags)
{
    static int grabbed = 0;
    int atpressed = 0;
    if (helpw == NULL)
	return 0;
    if (x < helpx || y < helpy || x > helpx + helpwidth
	|| y > helpy + helpheight) {
	if (flags & MOUSE_PRESS)
	    ui_close_help();
	pressedbutton = -1;
	if (pressedbutton != -1)
	    pressedbutton = -1, uih->display = 1;
	return 1;
    } else {
	if (y > helpy + helpheight - BUTTONHEIGHT - 2 * BORDERHEIGHT) {
	    int button;
	    int i;
	    for (i = 0; i <= NBUTTONS; i++)
		if (x < BUTTONSTART(i))
		    break;
	    button = i - 1;
	    if (flags & MOUSE_DRAG) {
		if (pressedbutton != selectedbutton
		    || pressedbutton != button)
		    pressedbutton = selectedbutton = button, uih->display =
			1;
	    } else {
		if (pressedbutton != -1)
		    pressedbutton = -1, uih->display = 1;
		if (flags & MOUSE_RELEASE)
		    do_button(button);
	    }
	    return 1;
	}
	if (pressedbutton != -1)
	    pressedbutton = -1, uih->display = 1;
	if (buttons
	    && ((x > SCROOLSTART && y < helpy + helpheight - HBORDERS)
		|| grabbed)) {
	    /*we are in scroolbar */
	    int pos = (y - helpy) * textheight / (helpheight - HBORDERS);
	    if (pos != ui_helppos[historypos])
		ui_helppos[historypos] = pos, uih->display = 1;
	    grabbed = 1;
	    return 1;
	} else
	    grabbed = 0;
        y -= helpy + xtextheight(uih->image, uih->font);
	x -= 2 * BORDERWIDTH + helpx;
	pressedbutton = -1;
	if (presseditem != NULL) {
	    if (YPOS(lines[pressedline].y) + 1 >= y
                && YPOS(lines[pressedline].y) <= y + xtextheight(uih->image, uih->font)
		&& presseditem->x <= x
		&& presseditem->x + presseditem->width >= x)
		atpressed = 1;
	}
	if (flags & MOUSE_PRESS || ((flags & MOUSE_DRAG) && !atpressed)) {
	    int i = 0;
	    for (i = 0;
		 lines[i].y >= 0 && (YPOS(lines[i].y) + 1 <= y
				     || YPOS(lines[i].y) >=
                                     y + xtextheight(uih->image, uih->font)); i++);
	    if (lines[i].y >= 0) {
		struct xshl_item *item = lines[i].first;
		while (item != NULL) {
		    if (item->c.linktext != NULL && item->x <= x
			&& item->x + item->width >= x)
			break;
		    item = item->next;
		}
		if (item != NULL) {
		    uih->display = 1;
		    presseditem = item;
		    pressedline = i;
		}
	    }
	} else if (flags & MOUSE_MOVE) {
	    if (!atpressed && presseditem != NULL)
		uih->display = 1, presseditem = 0;
	} else if ((flags & MOUSE_RELEASE)) {
	    if (atpressed) {
		ui_help(presseditem->c.linktext);
		return 1;
	    }
	    if (presseditem != NULL)
		presseditem = NULL, uih->display = 1;
	    /*odfajruj to! */
	}
    }
    return 1;
}

static void ui_build_help(char *name)
{
    int i;
    int width;
    if (ui_nogui) {
	printf("help \"%s\"\n", name);
	return;
    }
    if (driver->gui_driver && driver->gui_driver->help) {
	driver->gui_driver->help(uih, name);
	return;
    }
    if (helpw != NULL)
	ui_close_help();
    pressedbutton = -1;
    helpvisible = 1;
    width = 80 * xtextwidth(uih->image, uih->font, "w");
    if (width > WIDTH)
	width = WIDTH;
    lines =
	help_make(name ? name : "main", getwidth, width - 2,
                  xtextheight(uih->image, uih->font), xtextheight(uih->image, uih->font));
    if (lines == NULL) {
	lines =
            help_make("main", getwidth, width - 2, xtextheight(uih->image, uih->font),
                      xtextheight(uih->image, uih->font));
	if (lines == NULL) {
	    helpvisible = 0;
	    uih_message(uih, "Help file not found");
	    return;
	}
    }
    width += WBORDERS;
    uih->display = 1;
    presseditem = selecteditem = NULL;
    helpwidth = width;
    helpx = (uih->image->width - width) / 2;
    for (i = 0; lines[i].y >= 0; i++);
    textheight = lines[i - 1].y + 4 * xtextheight(uih->image, uih->font);
    if (textheight < HEIGHT)
	helpheight = textheight;
    else
	helpheight = HEIGHT;
    helpheight += HBORDERS;
    helpy = (uih->image->height - helpheight) / 2;
    helpw = uih_registerw(uih, helpsize, drawhelp, 0, DRAWBORDER);
}

void ui_help(const char *name)
{
    if (strlen(name) > 4 && !strcmp(name + strlen(name) - 4, ".xaf")) {
	uih_playtutorial(uih, name);
	if (helpw != NULL)
	    ui_close_help();
	/*FIXME!!!!!!! This needs to be queued! */
	return;
    }
    historypos++;
    historypos %= HISTORYSIZE;
    ui_helppage[historypos] = mystrdup(name);	/* NOTE we are not freeing this memory. I believe it is not problem. */
    ui_helppos[historypos] = 0;
    ui_build_help(ui_helppage[historypos]);
}
