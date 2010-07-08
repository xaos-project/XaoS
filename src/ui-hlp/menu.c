#ifndef _plan9_
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#else
#include <u.h>
#include <libc.h>
#endif

#include <aconfig.h>
#include <filter.h>
#include <fconfig.h>
#include <formulas.h>
#include <ui_helper.h>
#include <plane.h>
#include <xmenu.h>
#include "play.h"
#ifdef HAVE_GETTEXT
#include <libintl.h>
#else
#define gettext(STRING) STRING
#endif

#define LANG(name,name2) MENUSTRING("lang", NULL, name,name2,0, (void (*) (struct uih_context *c, char *))uih_loadcatalog, name2)
#define TUTOR(name1,name2,name3) MENUSTRING(name1, NULL, name2,name3,MENUFLAG_INTERRUPT|UI,uih_playtutorial,name3)
#define LANG_I(name,name2) MENUSTRING_I("lang", NULL, name,name2,0, (void (*) (struct uih_context *c, char *))uih_loadcatalog, name2)
#define TUTOR_I(name1,name2,name3) MENUSTRING_I(name1, NULL, name2,name3,MENUFLAG_INTERRUPT|UI,uih_playtutorial,name3)

const static char *const morphstypes[] = {
    "view",
    "julia",
    "angle",
    "line"
};

static const char *const imgtypes[] = {
    "Truecolor",
    "256 colors",
    NULL
};

static const char *const yesno[] = {
    "No",
    "Yes",
    NULL
};

const static char *const lineposs[] = {
    "screen",
    "scaled",
    "fractal",
    NULL
};

const char *const uih_colornames[] = {
    "white",
    "black",
    "red",
    NULL
};

/* Registering internationalized dialogs.
 *
 * The method we are internationalizing dialogs is similar to
 * menu i18n. The original version of XaoS (without i18n)
 * contained lots of static variables. Each row of a variable
 * (which is a structure) contains a widget of a dialog.
 * The last row contains NULL and 0 values to show
 * that the dialog does not contain any other widgets.
 *
 * Here we are using a huge static variable which contains
 * all widget from all dialogs. We copy each dialog after
 * each other into this huge array. The original static
 * variables will now be pointers pointing to the first
 * row of the widget data from the appropriate line
 * of the huge array.
 *
 * Note that in the first version there are only 2
 * internationalized text, the rest will be "converted"
 * continously (as I have enough time :-).
 *
 * Zoltan Kovacs <kovzol@math.u-szeged.hu>, 2003-01-05
 */

#define MAX_MENUDIALOGS_I18N 100
#define Register(variable) variable = & menudialogs_i18n[no_menudialogs_i18n]
static menudialog menudialogs_i18n[MAX_MENUDIALOGS_I18N];
//static int no_menudialogs_i18n;

static menudialog *uih_perturbationdialog, *uih_juliadialog,
    *uih_smoothmorphdialog, *uih_renderdialog, *uih_viewdialog,
    *uih_linedialog, *uih_colordialog, *uih_rotationdialog,
    *uih_lettersdialog, *uih_iterdialog, *dtextparam, *dcommand,
    *loaddialog, *playdialog, *saveimgdialog, *saveposdialog,
    *uih_formuladialog, *uih_plviewdialog, *uih_coorddialog,
    *uih_angledialog, *uih_autorotatedialog, *uih_fastrotatedialog,
    *uih_filterdialog, *uih_shiftdialog, *uih_speeddialog, *printdialog,
    *uih_bailoutdialog, *saveanimdialog, *uih_juliamodedialog,
    *uih_textposdialog, *uih_fastmodedialog, *uih_timedialog,
    *uih_numdialog, *uih_fpdialog, *palettedialog, *uih_cyclingdialog
#ifdef SFFE_USING
, *uih_sffedialog, *uih_sffeinitdialog
#endif
;

extern char *xtextposnames[];
extern char *ytextposnames[];


void uih_registermenudialogs_i18n(void)
{
    int no_menudialogs_i18n = 0;

/*
 * The original code was:

static menudialog uih_perturbationdialog[] = {
  DIALOGCOORD ("Perturbation:", 0, 0),
  {NULL}

};

 * Now first the static variable have to be registered (1),
 * the widget must be inserted into the huge array (2),
 * and the last row shows that no more widget comes (3).
 */

    Register(uih_perturbationdialog);	// (1)
    DIALOGCOORD_I(gettext("Perturbation:"), 0, 0);	// (2)
    NULL_I();			// (3)

    Register(uih_juliadialog);
    DIALOGCOORD_I(gettext("Julia-seed:"), 0, 0);
    NULL_I();

    Register(uih_smoothmorphdialog);
    DIALOGCHOICE_I(gettext("Morphing type:"), morphstypes, 0);
    DIALOGINT_I(gettext("Startuptime:"), 0);
    DIALOGINT_I(gettext("Stoptime:"), 0);
    NULL_I();

    Register(uih_renderdialog);
    DIALOGIFILE_I(gettext("File to render:"), "fract*.xaf");
    DIALOGOFILE_I(gettext("Basename:"), "anim");
    DIALOGINT_I(gettext("Width:"), 640);
    DIALOGINT_I(gettext("Height:"), 480);
    DIALOGFLOAT_I(gettext("Real width (cm):"), 29.0);
    DIALOGFLOAT_I(gettext("Real height (cm):"), 21.0);
    DIALOGFLOAT_I(gettext("Framerate:"), 30);
    DIALOGCHOICE_I(gettext("Image type:"), imgtypes, 0);
    DIALOGCHOICE_I(gettext("Antialiasing:"), yesno, 0);
    DIALOGCHOICE_I(gettext("Always recalculate:"), yesno, 0);
    DIALOGCHOICE_I(gettext("Calculate MPEG motion vectors:"), yesno, 0);
    DIALOGINT_I(gettext("Recommended I frame distance:"), 27);
    NULL_I();

    Register(uih_viewdialog);
    DIALOGCOORD_I(gettext("Center:"), 0, 0);
    DIALOGFLOAT_I(gettext("Radius:"), 1);
    DIALOGFLOAT_I(gettext("Angle:"), 0);
    NULL_I();

    Register(uih_linedialog);
    DIALOGCHOICE_I(gettext("Mode:"), lineposs, 0);
    DIALOGCOORD_I(gettext("Start:"), 0, 0);
    DIALOGCOORD_I(gettext("End:"), 0, 0);
    NULL_I();

    Register(uih_colordialog);
    DIALOGCHOICE_I(gettext("Color:"), uih_colornames, 0);
    NULL_I();

    Register(uih_rotationdialog);
    DIALOGFLOAT_I(gettext("Rotations per second:"), 0);
    NULL_I();

    Register(uih_lettersdialog);
    DIALOGINT_I(gettext("Letters per second:"), 0);
    NULL_I();

    Register(uih_iterdialog);
    DIALOGINT_I(gettext("Iterations:"), 0);
    NULL_I();

    Register(dtextparam);
    DIALOGSTR_I(gettext("Text:"), "");
    NULL_I();

    Register(dcommand);
    DIALOGSTR_I(gettext("Your command:"), "");
    NULL_I();

    Register(loaddialog);
    DIALOGIFILE_I(gettext("Filename:"), "fract*.xpf");
    NULL_I();

    Register(playdialog);
    DIALOGIFILE_I(gettext("Filename:"), "anim*.xaf");
    NULL_I();

    Register(saveimgdialog);
    DIALOGOFILE_I(gettext("Filename:"), "fract*.png");
    NULL_I();

    Register(saveposdialog);
    DIALOGOFILE_I(gettext("Filename:"), "fract*.xpf");
    NULL_I();

    Register(uih_formuladialog);
    DIALOGKEYSTR_I(gettext("Formula:"), "mandel");
    NULL_I();

    Register(uih_plviewdialog);
    DIALOGFLOAT_I(gettext("X center:"), 0);
    DIALOGFLOAT_I(gettext("Y center:"), 0);
    DIALOGFLOAT_I(gettext("X Radius:"), 1);
    DIALOGFLOAT_I(gettext("Y Radius:"), 1);
    NULL_I();

    Register(uih_coorddialog);
    DIALOGCOORD_I(gettext("Coordinates:"), 0, 0);
    NULL_I();

    Register(uih_angledialog);
    DIALOGFLOAT_I(gettext("Angle:"), 1);
    NULL_I();

    Register(uih_autorotatedialog);
    DIALOGONOFF_I(gettext("continuous rotation"), 0);
    NULL_I();

    Register(uih_fastrotatedialog);
    DIALOGONOFF_I(gettext("Fast rotation"), 0);
    NULL_I();

    Register(uih_filterdialog);
    DIALOGKEYSTR_I(gettext("filter"), "");
    DIALOGONOFF_I(gettext("enable"), 0);
    NULL_I();

    Register(uih_shiftdialog);
    DIALOGINT_I(gettext("Amount:"), 0);
    NULL_I();

    Register(uih_speeddialog);
    DIALOGFLOAT_I(gettext("Zooming speed:"), 0);
    NULL_I();

    Register(printdialog);
    DIALOGSTR_I(gettext("Name:"), "");
    NULL_I();

    Register(uih_bailoutdialog);
    DIALOGFLOAT_I(gettext("Bailout:"), 0);
    NULL_I();

    Register(saveanimdialog);
    DIALOGOFILE_I(gettext("Filename:"), "anim*.xaf");
    NULL_I();

    Register(uih_juliamodedialog);
    DIALOGONOFF_I(gettext("Julia mode:"), 0);
    NULL_I();

    Register(uih_textposdialog);
    DIALOGCHOICE_I(gettext("Horizontal position:"), xtextposnames, 0);
    DIALOGCHOICE_I(gettext("Vertical position:"), ytextposnames, 0);
    NULL_I();

    Register(uih_fastmodedialog);
    DIALOGCHOICE_I(gettext("Dynamic resolution:"), save_fastmode, 0);
    NULL_I();

    Register(uih_timedialog);
    DIALOGINT_I(gettext("Time:"), 0);
    NULL_I();

    Register(uih_numdialog);
    DIALOGINT_I(gettext("Number:"), 0);
    NULL_I();

    Register(uih_fpdialog);
    DIALOGFLOAT_I(gettext("Number:"), 0);
    NULL_I();

    Register(palettedialog);
    DIALOGINT_I(gettext("Algorithm number:"), 0);
    DIALOGINT_I(gettext("Seed:"), 0);
    DIALOGINT_I(gettext("Shift:"), 0);
    NULL_I();

    Register(uih_cyclingdialog);
    DIALOGINT_I(gettext("Frames per second:"), 0);
    NULL_I();

#ifdef SFFE_USING
    Register(uih_sffedialog);
    DIALOGSTR_I(gettext("Formula:"), "z^2+c");
    NULL_I();

    Register(uih_sffeinitdialog);
    DIALOGSTR_I(gettext("Initialization:"), "");
    NULL_I();
#endif

#ifdef DEBUG
    printf("Filled %d widgets out of %d.\n",
	   no_menudialogs_i18n, MAX_MENUDIALOGS_I18N);
#endif
}

#undef Register

/*
 * End of registering internationalized dialogs.
 */

#ifdef SFFE_USING
void uih_sffein(uih_context * c, const char *text);
void uih_sffeinitin(uih_context * c, const char *text);
#endif

static void uih_smoothmorph(struct uih_context *c, dialogparam * p)
{
    if (!c->playc)
	return;
    switch (p[0].dint) {
    case 0:
	c->playc->morphtimes[0] = p[1].dint;
	c->playc->morphtimes[1] = p[2].dint;
	break;
    case 1:
	c->playc->morphjuliatimes[0] = p[1].dint;
	c->playc->morphjuliatimes[1] = p[2].dint;
	break;
    case 2:
	c->playc->morphangletimes[0] = p[1].dint;
	c->playc->morphangletimes[1] = p[2].dint;
	break;
    case 3:
	c->playc->morphlinetimes[0] = p[1].dint;
	c->playc->morphlinetimes[1] = p[2].dint;
	break;
    }
}

static void uih_render(struct uih_context *c, dialogparam * d)
{
#ifdef MACOSX
    /* 
     * On Mac OS X, we must ensure basename specifies an absolute path in order
     * to prevent png files from being rendered to the root directory.
     */
    if (d[1].dstring[0] != '/') {
	uih_error(c,
		  gettext
		  ("renderanim: Must specify a valid absolute path for basename"));
	return;
    }
#endif
    if (d[2].dint <= 0 || d[2].dint > 4096) {
	uih_error(c,
		  gettext
		  ("renderanim: Width parameter must be positive integer in the range 0..4096"));
	return;
    }
    if (d[3].dint <= 0 || d[3].dint > 4096) {
	uih_error(c,
		  gettext
		  ("renderanim: Height parameter must be positive integer in the range 0..4096"));
	return;
    }
    if (d[4].number <= 0 || d[5].number <= 0) {
	uih_error(c,
		  gettext
		  ("renderanim: Invalid real width and height dimensions"));
	return;
    }
    if (d[6].number <= 0 || d[6].number >= 1000000) {
	uih_error(c, gettext("renderanim: invalid framerate"));
	return;
    }
    if (d[7].dint && d[8].dint) {
	uih_error(c,
		  gettext
		  ("renderanim: antialiasing not supported in 256 color mode"));
	return;
    }
    if (d[11].dint <= 0 || d[11].dint >= 1000000) {
	uih_error(c, gettext("renderanim: incorrect I frame distance"));
	return;
    }
    uih_renderanimation(c, d[1].dstring, (xio_path) d[0].dstring,
			d[2].dint, d[3].dint, d[4].number / d[2].dint,
			d[5].number / d[3].dint,
			(int) (1000000 / d[6].number),
#ifdef STRUECOLOR24
			d[7].dint ? C256 : TRUECOLOR24,
#else
			d[7].dint ? C256 : TRUECOLOR,
#endif
			d[8].dint, d[9].dint, c->letterspersec, NULL,
			d[10].dint, d[11].dint);
}

static menudialog *uih_getcolordialog(struct uih_context *c)
{
    if (c != NULL) {
	uih_colordialog[0].defint = c->color;
    }
    return (uih_colordialog);
}

static void uih_setcolor(struct uih_context *c, int color)
{
    c->color = color;
}


static menudialog *uih_getperturbationdialog(struct uih_context *c)
{
    if (c != NULL) {
	uih_perturbationdialog[0].deffloat = c->fcontext->bre;
	uih_perturbationdialog[0].deffloat2 = c->fcontext->bim;
    }
    return (uih_perturbationdialog);
}

static menudialog *uih_getjuliadialog(struct uih_context *c)
{
    if (c != NULL) {
	uih_juliadialog[0].deffloat = c->fcontext->pre;
	uih_juliadialog[0].deffloat2 = c->fcontext->pim;
    }
    return (uih_juliadialog);
}

static void uih_plview(struct uih_context *c, dialogparam * d)
{
    if (d[2].number <= 0 || d[3].number <= 0) {
	uih_error(c, gettext("animateview: Invalid viewpoint"));
	return;
    }
    c->fcontext->s.cr = d[0].number;
    c->fcontext->s.ci = d[1].number;
    c->fcontext->s.rr = d[2].number;
    c->fcontext->s.ri = d[3].number;
    uih_newimage(c);
}

static void uih_plview2(struct uih_context *c, dialogparam * d)
{
    if (d[2].number <= 0 || d[3].number <= 0) {
	uih_error(c, gettext("animateview: Invalid viewpoint"));
	return;
    }
    c->fcontext->s.cr = d[0].number;
    c->fcontext->s.ci = d[1].number;
    c->fcontext->s.rr = d[2].number;
    c->fcontext->s.ri = d[3].number;
    uih_animate_image(c);
}

static void uih_dview(struct uih_context *c, dialogparam * d)
{
    if (d[1].number <= 0) {
	uih_error(c, gettext("Invalid viewpoint"));
	return;
    }
    c->fcontext->s.cr = d[0].dcoord[0];
    c->fcontext->s.ci = d[0].dcoord[1];
    c->fcontext->s.rr = d[1].number;
    c->fcontext->s.ri = d[1].number;
    uih_angle(c, d[2].number);
    uih_newimage(c);
}

static menudialog *uih_getviewdialog(struct uih_context *c)
{
    number_t xs, ys;
    if (c != NULL) {
	xs = c->fcontext->s.rr;
	ys = c->fcontext->s.ri * c->fcontext->windowwidth /
	    c->fcontext->windowheight;
	uih_viewdialog[0].deffloat = c->fcontext->s.cr;
	uih_viewdialog[0].deffloat2 = c->fcontext->s.ci;
	uih_viewdialog[2].deffloat = c->fcontext->angle;
	if (xs > ys)
	    uih_viewdialog[1].deffloat = c->fcontext->s.rr;
	else
	    uih_viewdialog[1].deffloat = c->fcontext->s.ri;
    }
    return (uih_viewdialog);
}

static void uih_printdialog(struct uih_context *c, const char *name)
{
    const menuitem *item = menu_findcommand(name);
    int y;
    const menudialog *di;
    if (item == NULL) {
	fprintf(stderr, "print_dialog:unknown function %s\n", name);
	return;
    }
    if (item->type != MENU_DIALOG && item->type != MENU_CUSTOMDIALOG) {
	fprintf(stderr, "print_dialog:%s don't have any dialog. Sorry.\n",
		name);
	return;
    }
    di = menu_getdialog(c, item);
    if (item->type == MENU_CUSTOMDIALOG)
	printf("customdialog \"%s\"\n", item->shortname);
    else
	printf("dialog \"%s\"\n", item->shortname);
    for (y = 0; di[y].question != NULL; y++) {
	printf("dialogentry \"%s\" ", di[y].question);
	switch (di[y].type) {
	case DIALOG_INT:
	    printf("integer %i", di->defint);
	    break;
	case DIALOG_FLOAT:
	    printf("float %f", (double) di->deffloat);
	    break;
	case DIALOG_STRING:
	    printf("string \"%s\"", di->defstr);
	case DIALOG_KEYSTRING:
	    printf("keyword \"%s\"", di->defstr);
	    break;
	case DIALOG_IFILE:
	    printf("inputfile \"%s\"", di->defstr);
	    break;
	case DIALOG_OFILE:
	    printf("outputfile \"%s\"", di->defstr);
	    break;
	case DIALOG_ONOFF:
	    printf("onoff %s", di->defint ? "#t" : "#f");
	    break;
	case DIALOG_COORD:
	    printf("complex %f %f", (double) di->deffloat,
		   (double) di->deffloat2);
	    break;
	case DIALOG_CHOICE:
	    printf("choice {");
	    {
		int y;
		const char **str = (const char **) di->defstr;
		for (y = 0; str[y] != NULL; y++)
		    printf("%s ", str[y]);
		printf("}");
		printf("%s ", str[di->defint]);
	    }
	    break;
	}
	printf("\n");
    }
    printf("enddialog\n");
}

static void
uih_printmenu(struct uih_context *c, const char *name, int recursive)
{
    const char *fullname;
    int i = 0;
    const menuitem *item;
    if ((fullname = menu_fullname(name)) == NULL) {
	printf("Menu not found\n");
	return;
    }
    printf("\n\nmenu \"%s\" \"%s\"\n", fullname, name);
    for (i = 0; (item = menu_item(name, i)) != NULL; i++) {
	if (item->type == MENU_SUBMENU) {
	    printf("submenu \"%s\" \"%s\"\n", item->name, item->shortname);
	    continue;
	}
	if (item->type == MENU_SUBMENU) {
	    printf("separator");
	    continue;
	}
	printf("menuentry \"%s\" \"%s\" ", item->name, item->shortname);
	if (item->flags & MENUFLAG_RADIO)
	    printf("radio %s", menu_enabled(item, c) ? "on" : "off");
	else if (item->flags & MENUFLAG_CHECKBOX)
	    printf("checkbox %s", menu_enabled(item, c) ? "on" : "off");
	else
	    printf("normal");
	if (item->flags & MENUFLAG_DIALOGATDISABLE)
	    printf(" dialogatdisable");
	if (item->type == MENU_DIALOG || item->type == MENU_CUSTOMDIALOG)
	    printf(" dialog");
	printf("\n");
    }
    printf("endmenu\n");
    if (recursive)
	for (i = 0; (item = menu_item(name, i)) != NULL; i++) {
	    if (item->type == MENU_SUBMENU) {
		uih_printmenu(c, item->shortname, 1);
	    }
	}
}

static void uih_printmenuwr(struct uih_context *c, const char *name)
{
    uih_printmenu(c, name, 0);
}

static void uih_printallmenus(struct uih_context *c)
{
    uih_printmenu(c, "root", 1);
    uih_printmenu(c, "animroot", 1);
    printf("endmenu\n");
}

static menudialog *uih_getlettersdialog(struct uih_context *c)
{
    if (c != NULL)
	uih_lettersdialog[0].defint = c->letterspersec;
    return (uih_lettersdialog);
}

static menudialog *uih_getiterdialog(struct uih_context *c)
{
    if (c != NULL)
	uih_iterdialog[0].defint = c->fcontext->maxiter;
    return (uih_iterdialog);
}

static menudialog *uih_getbailoutdialog(struct uih_context *c)
{
    if (c != NULL)
	uih_bailoutdialog[0].deffloat = c->fcontext->bailout;
    return (uih_bailoutdialog);
}

static int uih_saveanimenabled(struct uih_context *c)
{
    if (c == NULL)
	return 0;
    return (c->save);
}



static menudialog *uih_getrotationdialog(struct uih_context *c)
{
    if (c != NULL)
	uih_rotationdialog[0].deffloat = c->rotationspeed;
    return (uih_rotationdialog);
}

static menudialog *uih_getpalettedialog(struct uih_context *uih)
{
    if (uih != NULL) {
	palettedialog[0].defint = uih->palettetype;
	palettedialog[1].defint = uih->paletteseed;
	palettedialog[2].defint =
	    uih->paletteshift + uih->manualpaletteshift;
    }
    return (palettedialog);
}

static menudialog *uih_getcyclingdialog(struct uih_context *uih)
{
    if (uih != NULL)
	uih_cyclingdialog[0].defint = uih->cyclingspeed * uih->direction;
    return (uih_cyclingdialog);
}

static menudialog *uih_getspeeddialog(struct uih_context *uih)
{
    if (uih != NULL)
	uih_speeddialog[0].deffloat = uih->speedup / STEP;
    return (uih_speeddialog);
}

static void uih_setspeed(uih_context * c, number_t p)
{
    if (p >= 100)
	p = 1.0;
    if (p < 0)
	p = 0;
    c->speedup = STEP * p;
    c->maxstep = MAXSTEP * p;

}

static void uih_palette(struct uih_context *uih, dialogparam * p)
{
    int n1 = p[0].dint;
    int n2 = p[1].dint;
    int shift = p[2].dint;
    if (!n1) {
	uih_playdefpalette(uih, shift);
	return;
    }
    if (n1 < 1 || n1 > PALGORITHMS) {
	uih_error(uih, gettext("Unknown palette type"));
    }
    if (uih->zengine->fractalc->palette == NULL)
	return;
    if (mkpalette(uih->zengine->fractalc->palette, n2, n1 - 1) != 0) {
	uih_newimage(uih);
    }
    uih->manualpaletteshift = 0;
    uih->palettetype = n1;
    uih->palettechanged = 1;
    uih->paletteseed = n2;
    if (shiftpalette(uih->zengine->fractalc->palette, shift)) {
	uih_newimage(uih);
    }
    uih->paletteshift = shift;
}

static int uih_rotateselected(struct uih_context *c, int n)
{
    if (c == NULL)
	return 0;
    if (!c->fastrotate)
	return !n;
    return (c->rotatemode == n);
}

static int uih_guessingselected(struct uih_context *c, int n)
{
    if (c == NULL)
	return 0;
    return (c->fcontext->range == n);
}

static int uih_fastmode(struct uih_context *c, int n)
{
    if (c == NULL)
	return 0;
    return (c->fastmode == n);
}

static int uih_periodicityselected(struct uih_context *c)
{
    if (c == NULL)
	return 0;
    return (c->fcontext->periodicity);
}

static void uih_periodicitysw(struct uih_context *c)
{
    uih_setperiodicity(c, c->fcontext->periodicity ^ 1);
}

static int uih_cyclingselected(struct uih_context *c)
{
    if (c == NULL)
	return 0;
    return (c->cycling && c->cyclingdirection == 1);
}

static int uih_rcyclingselected(struct uih_context *c)
{
    if (c == NULL)
	return 0;
    return (c->cycling && c->cyclingdirection == -1);
}

static void uih_cyclingsw(struct uih_context *c)
{
    // Andrew Stone: this fixes what I consider a bug - switching from Y to y should keep cycling:  
    if (c->cycling && c->cyclingdirection == -1)
	uih_cycling_off(c);
    c->cyclingdirection = 1;
    if (c->cycling)
	uih_cycling_off(c);
    else if (!uih_cycling_on(c))
	uih_error(c, gettext("Initialization of color cycling failed.")),
	    uih_message(c,
			gettext("Try to enable palette emulation filter"));
}

static void uih_rcyclingsw(struct uih_context *c)
{
    // Andrew Stone: this fixes what I consider a bug - switching from y to Y should keep cycling:         
    if (c->cycling && c->cyclingdirection == 1)
	uih_cycling_off(c);
    c->cyclingdirection = -1;
    if (c->cycling)
	uih_cycling_off(c);
    else if (!uih_cycling_on(c))
	uih_error(c, gettext("Initialization of color cycling failed.")),
	    uih_message(c,
			gettext("Try to enable palette emulation filter"));
}

static void uih_juliasw(struct uih_context *c)
{
    if (!c->juliamode)
	uih_enablejulia(c);
    else
	uih_disablejulia(c);
}

static int uih_juliaselected(struct uih_context *c)
{
    if (c == NULL)
	return 0;
    return (c->juliamode);
}

static int uih_mandelbrotselected(struct uih_context *c)
{
    if (c == NULL)
	return 0;
    return (c->fcontext->mandelbrot);
}

static void uih_mandelbrotsw(struct uih_context *c, number_t x, number_t y)
{
    c->fcontext->mandelbrot ^= 1;
    if (c->fcontext->mandelbrot == 0 && !c->juliamode) {
	c->fcontext->pre = x;
	c->fcontext->pim = y;
    } else
	uih_disablejulia(c);
    c->fcontext->version++;
    uih_newimage(c);
    uih_updatemenus(c, "uimandelbrot");
}

static int uih_autopilotselected(struct uih_context *c)
{
    if (c == NULL)
	return 0;
    return (c->autopilot);
}

static int uih_fixedstepselected(struct uih_context *c)
{
    if (c == NULL)
	return 0;
    return (c->fixedstep);
}

static void uih_persw(struct uih_context *c, number_t x, number_t y)
{
    if (c->fcontext->bre || c->fcontext->bim)
	uih_setperbutation(c, 0.0, 0.0);
    else
	uih_setperbutation(c, x, y);
    // printf(""); // fixme: some newer versions of gcc crashes without this
}

static int uih_perselected(struct uih_context *c)
{
    if (c == NULL)
	return 0;
    return (c->fcontext->bre || c->fcontext->bim);
}

static void uih_autopilotsw(struct uih_context *c)
{
    if (c->autopilot)
	uih_autopilot_off(c);
    else
	uih_autopilot_on(c);
}

static void uih_fixedstepsw(struct uih_context *c)
{
    c->fixedstep ^= 1;
}

static void uih_setxtextpos(uih_context * c, int p)
{
    uih_settextpos(c, p, c->ytextpos);
}

static int uih_xtextselected(uih_context * c, int p)
{
    if (c == NULL)
	return 0;
    return (c->xtextpos == p);
}

static void uih_setytextpos(uih_context * c, int p)
{
    uih_settextpos(c, c->xtextpos, p);
}

static int uih_ytextselected(uih_context * c, int p)
{
    if (c == NULL)
	return 0;
    return (c->ytextpos == p);
}

static void uih_menumkpalette(uih_context * c)
{
    char s[256];
    uih_mkpalette(c);
    sprintf(s, gettext("Algorithm:%i seed:%i size:%i"), c->palettetype,
	    c->paletteseed, c->zengine->fractalc->palette->size);
    uih_message(c, s);
}

static void uih_shiftpalette(uih_context * c, int shift)
{
    if (shiftpalette(c->zengine->fractalc->palette, shift)) {
	uih_newimage(c);
    }
    c->manualpaletteshift += shift;
}

static void uih_fshift(uih_context * c)
{
    uih_shiftpalette(c, 1);
}

static void uih_bshift(uih_context * c)
{
    uih_shiftpalette(c, -1);
}

static const menuitem *menuitems;	/*XaoS menu specifications */
/* This structure is now empty. All static definitions have been moved
   to uih_registermenus_i18n() which fills up its own static array. */


/* Registering internationalized menus. See also include/xmenu.h
   for details. Note that MAX_MENUITEMS_I18N should be increased
   if more items will be added in future.
   
   On 2006-07-12 J.B. Langston wrote:
   
   The menu.c.diff file changes the MAX_MENUITEMS_I18N macro from
   200 to 250.  As of 3.2.1, the number of allocated menu items had 
   been exceeded (there are 201 menu items now). This was causing
   the memory within the application to be clobbered, resulting 
   in subtle bugs on the PowerPC platform (both X11 and OSX drivers).
   For example, rendering animations generated a segmentation fault.
   It's quite possible it could have been causing other subtle bugs
   elsewhere in the program. */

#define MAX_MENUITEMS_I18N 250
static menuitem menuitems_i18n[MAX_MENUITEMS_I18N];
int uih_no_menuitems_i18n;

void uih_registermenus_i18n(void)
{
    // Special version (currently it's OK):
    int no_menuitems_i18n = 0;
    SUBMENU_I("", NULL, gettext("Root menu"), "root");
    SUBMENU_I("", NULL, gettext("Animation root menu"), "animroot");
    SUBMENU_I("", NULL, gettext("Replay only commands"), "plc");
    SUBMENU_I("", NULL, gettext("Command line options only"), "comm");
#define MP (MENUFLAG_NOMENU|MENUFLAG_NOPLAY|MENUFLAG_ATSTARTUP)
    MENUNOP_I("comm", NULL,
	      gettext("print menus specifications of all menus"),
	      "print_menus", MP, uih_printallmenus);
    MENUDIALOG_I("comm",
		 NULL,
		 gettext
		 ("print menu specification"),
		 "print_menu", MP, uih_printmenuwr, printdialog);
    MENUDIALOG_I("comm", NULL,
		 gettext("print menu specification in xshl format"),
		 "xshl_print_menu", MP, uih_xshlprintmenu, printdialog);
    MENUNOP_I("comm", NULL,
	      gettext
	      ("print all menu specifications in xshl format"),
	      "xshl_print_menus", MP, uih_xshlprintmenus);
    MENUDIALOG_I("comm", NULL, gettext("print dialog specification"),
		 "print_dialog", MP, uih_printdialog, printdialog);
#undef MP
#define MP (MENUFLAG_NOMENU|MENUFLAG_NOOPTION)
    /* Commands suitable only for animation replay */
    SUBMENU_I("plc", NULL, gettext("Line drawing functions"), "linemenu");
    MENUDIALOG_I("linemenu", NULL, gettext("Line"), "line", MP, uih_line,
		 uih_linedialog);
    MENUDIALOG_I("linemenu", NULL,
		 gettext("Morph line"),
		 "morphline", MP, uih_morphline, uih_linedialog);
    MENUDIALOG_I("linemenu", NULL, gettext("Morph last line"),
		 "morphlastline", MP, uih_morphlastline, uih_linedialog);
    MENUDIALOG_I("linemenu", NULL,
		 gettext("Set line key"),
		 "linekey", MP, uih_setkey, uih_numdialog);
    MENUNOP_I("linemenu", NULL, gettext("Clear line"), "clearline", MP,
	      uih_clear_line);
    MENUNOP_I("linemenu", NULL,
	      gettext("Clear all lines"), "clearlines", MP,
	      uih_clear_lines);
    SUBMENU_I("plc", NULL, gettext("Animation functions"), "animf");
    MENUDIALOG_I("animf", NULL, gettext("View"), "animateview", MP,
		 uih_plview2, uih_plviewdialog);
    MENUDIALOG_I("animf", NULL,
		 gettext
		 ("Morph view"),
		 "morphview", MP, uih_playmorph, uih_plviewdialog);
    MENUDIALOG_I("animf", NULL, gettext("Morph julia"), "morphjulia", MP,
		 uih_playmorphjulia, uih_coorddialog);
    MENUDIALOG_I("animf",
		 NULL,
		 gettext
		 ("Move view"), "moveview", MP, uih_playmove,
		 uih_coorddialog);
    MENUDIALOG_I("animf", NULL, gettext("Morph angle"), "morphangle", MP,
		 uih_playmorphangle, uih_angledialog);
    MENUDIALOG_I("animf", NULL, gettext("Zoom center"), "zoomcenter", MP,
		 uih_zoomcenter, uih_coorddialog);
    MENUNOP_I("animf", NULL, gettext("Zoom"), "zoom", MP, uih_playzoom);
    MENUNOP_I("animf", NULL, gettext("Un-zoom"),
	      "unzoom", MP, uih_playunzoom);
    MENUNOP_I("animf",
	      NULL, gettext("Stop zooming"), "stop", MP, uih_playstop);
    MENUDIALOG_I("animf", NULL, gettext("Smooth morphing parameters"),
		 "smoothmorph", MP, uih_smoothmorph,
		 uih_smoothmorphdialog);
    SUBMENU_I("plc", NULL, gettext("Timing functions"), "time");
    MENUDIALOG_I("time", NULL, gettext("Usleep"), "usleep", MP,
		 uih_playusleep, uih_timedialog);
    MENUNOP_I("time", NULL,
	      gettext("Wait for text"), "textsleep", MP,
	      uih_playtextsleep);
    MENUNOP_I("time", NULL, gettext("Wait for complete image"), "wait", MP,
	      uih_playwait);
    MENUDIALOG_I("plc", NULL, gettext("Include file"), "load", MP,
		 uih_playload, loaddialog);
    MENUDIALOG_I("palette", NULL, gettext("Default palette"),
		 "defaultpalette", MP, uih_playdefpalette, uih_numdialog);
    MENUDIALOG_I("fractal", NULL, gettext("Formula"), "formula", MP,
		 uih_play_formula, uih_formuladialog);
    MENUDIALOG_I("ui", NULL, gettext("Maximal zooming step"), "maxstep",
		 MP, uih_setmaxstep, uih_fpdialog);
    MENUDIALOG_I("ui", NULL, gettext("Zooming speedup"), "speedup", MP,
		 uih_setspeedup, uih_fpdialog);
    MENUDIALOG_I("mfilter", NULL, gettext("Filter"), "filter", MP,
		 uih_playfilter, uih_filterdialog);
#undef MP
#define UI (MENUFLAG_NOPLAY|MENUFLAG_NOOPTION)
    MENUCDIALOG_I("ui", NULL, gettext("Letters per second"),
		  "letterspersec", MENUFLAG_NOMENU, uih_letterspersec,
		  uih_getlettersdialog);
    MENUCDIALOG_I("uia", NULL,
		  gettext
		  ("Letters per second"),
		  "letters", UI, uih_letterspersec, uih_getlettersdialog);
    MENUNOP_I("uia", "z", gettext("Interrupt"), "animinterrupt",
	      MENUFLAG_INTERRUPT | MENUFLAG_INCALC, uih_interrupt);
    SUBMENU_I("root", "s", gettext("File"), "file");
    SUBMENU_I("root", NULL, gettext("Edit"), "edit");
    SUBMENU_I("root", NULL, gettext("Fractal"), "fractal");
    SUBMENU_I("root", NULL, gettext("Calculation"), "calc");
    SUBMENU_I("root", "e", gettext("Filters"), "mfilter");
    SUBMENU_I("root", NULL, gettext("UI"), "ui");
    SUBMENU_I("root", NULL, gettext("Misc"), "misc");
#ifdef MACOSX
    SUBMENU_I("root", NULL, gettext("Window"), "window");
#endif
    SUBMENU_I("root", NULL, gettext("Help"), "helpmenu");
    SUBMENU_I("helpmenu", NULL, gettext("Tutorials"),
	      "tutor") SUBMENUNOOPT_I("animroot", "f", gettext("File"),
				      "file");
    // You cannot have menu items directly on the root menu in some OS
    // So we put the "Stop Replay" item in the UI menu instead
    MENUSEPARATOR_I("uia");
    MENUNOP_I("uia", "s", gettext("Stop replay"), "stopreplay",
	      UI | MENUFLAG_INTERRUPT, uih_replaydisable);
    SUBMENUNOOPT_I("animroot", NULL, gettext("UI"), "uia");
    SUBMENUNOOPT_I("animroot", NULL, gettext("Help"), "helpmenu");
    MENUDIALOG_I("misc", "!", gettext("Command"), "command", UI,
		 uih_command, dcommand);
    MENUDIALOG_I("misc", NULL, gettext("Play string"), "playstr",
		 MENUFLAG_NOMENU, uih_playstr, dcommand);
    MENUDIALOG_I("misc", NULL, gettext("Render animation"), "renderanim",
		 UI, uih_render, uih_renderdialog);
    MENUSEPARATOR_I("misc");
    MENUNOP_I("misc", NULL, gettext("Clear screen"), "clearscreen",
	      MENUFLAG_NOOPTION, uih_clearscreen);
    MENUNOP_I("misc", NULL, gettext("Display fractal"), "display",
	      MENUFLAG_NOOPTION, uih_display);
    MENUSEPARATOR_I("misc");
    MENUDIALOG_I("misc", NULL, gettext("Display text"), "text", 0, uih_text, dtextparam);	/*FIXME: Should allow multiline */

    MENUCDIALOG_I("misc", NULL, gettext("Color"), "color", 0, uih_setcolor,
		  uih_getcolordialog);
    SUBMENU_I("misc", NULL, gettext("Horizontal text position"),
	      "xtextpos");
    SUBMENU_I("misc", NULL, gettext("Vertical text position"), "ytextpos");
    MENUDIALOG_I("misc", NULL,
		 gettext("Text position"),
		 "textposition",
		 MENUFLAG_NOMENU | MENUFLAG_INCALC,
		 uih_playtextpos, uih_textposdialog);
    MENUDIALOG_I("misc", NULL, gettext("Message"), "message",
		 MENUFLAG_NOMENU, uih_playmessage, dtextparam);
    /* The following 6 menu options should not be translated. The example
       files heavily use these constants and lots of examples will not work
       anymore... :-(  Anyway, this should be fixed somehow. */

    MENUINTRB_I("ytextpos", NULL, "Up", "ytextup", UI, uih_setytextpos,
		UIH_TEXTTOP, uih_ytextselected);
    MENUINTRB_I("ytextpos",
		NULL, "Middle",
		"ytextmiddle",
		UI, uih_setytextpos, UIH_TEXTMIDDLE, uih_ytextselected);
    MENUINTRB_I("ytextpos", NULL, "Bottom", "ytextbottom", UI,
		uih_setytextpos, UIH_TEXTBOTTOM, uih_ytextselected);
    MENUINTRB_I("xtextpos", NULL, "Left",
		"xtextleft", UI,
		uih_setxtextpos, UIH_TEXTLEFT, uih_xtextselected);
    MENUINTRB_I("xtextpos", NULL, "Center", "xtextcenter", UI,
		uih_setxtextpos, UIH_TEXTCENTER, uih_xtextselected);
    MENUINTRB_I("xtextpos", NULL, "Right",
		"xtexteight", UI,
		uih_setxtextpos, UIH_TEXTRIGHT, uih_xtextselected);
    MENUDIALOG_I("file", NULL, gettext("Load"), "loadpos",
		 MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY, uih_loadfile,
		 loaddialog);
    MENUDIALOG_I("file", NULL, gettext("Save"),
		 "savepos", 0, uih_saveposfile, saveposdialog);
    MENUSEPARATOR_I("file") MENUDIALOGCB_I("file", NULL, gettext("Record"),
					   "record", 0, uih_saveanimfile,
					   saveanimdialog,
					   uih_saveanimenabled);
    MENUDIALOG_I("file", NULL, gettext("Replay"), "play",
		 MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY, uih_playfile,
		 playdialog);
    MENUSEPARATOR_I("file");
    MENUDIALOG_I("file",
		 NULL,
		 gettext
		 ("Save image"), "saveimg", 0, uih_savepngfile,
		 saveimgdialog);
    MENUNOP_I("file", NULL, gettext("Load random example"), "loadexample",
	      MENUFLAG_INTERRUPT, uih_loadexample);
    MENUNOP_I("file", NULL, gettext("Save configuration"), "savecfg", 0,
	      uih_savecfg);
#ifndef MACOSX
    MENUSEPARATOR_I("file");
#endif
    MENUNOP_I("edit", "u", gettext("Undo"), "undo",
	      MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY |
	      MENUFLAG_NOOPTION, uih_undo);
    MENUNOP_I("edit", NULL,
	      gettext("Redo"),
	      "redo",
	      MENUFLAG_INTERRUPT
	      | MENUFLAG_NOPLAY | MENUFLAG_NOOPTION, uih_redo);
    SUBMENU_I("fractal", NULL, gettext("Formulae"), "mformula");
    SUBMENU_I("fractal", NULL, gettext("More formulae"), "oformula");

#ifdef SFFE_USING
    /*FIXME: Should allow multiline */
    MENUSEPARATOR_I("fractal");
    MENUDIALOG_I("fractal", NULL, gettext("User formula"), "usrform", 0,
		 uih_sffein, uih_sffedialog);
    MENUDIALOG_I("fractal", NULL, gettext("User initialization"),
		 "usrformInit", 0, uih_sffeinitin, uih_sffeinitdialog);
#endif

    MENUSEPARATOR_I("fractal");
    SUBMENU_I("fractal", "f", gettext("Incoloring mode"), "mincoloring");
    SUBMENU_I("fractal", "c", gettext("Outcoloring mode"), "moutcoloring");
    SUBMENU_I("fractal", "i", gettext("Plane"), "mplane");
    SUBMENU_I("fractal", NULL, gettext("Palette"), "palettemenu");
    MENUSEPARATOR_I("fractal");
    MENUCDIALOGCB_I("fractal", "m",
		    gettext("Mandelbrot mode"),
		    "uimandelbrot",
		    MENUFLAG_DIALOGATDISABLE |
		    MENUFLAG_INTERRUPT | UI,
		    uih_mandelbrotsw,
		    uih_getjuliadialog, uih_mandelbrotselected);
    MENUCDIALOGCB_I("fractal", "b", gettext("Perturbation"),
		    "uiperturbation", MENUFLAG_INTERRUPT | UI, uih_persw,
		    uih_getperturbationdialog, uih_perselected);
    MENUCDIALOG_I("fractal", NULL,
		  gettext("Perturbation"),
		  "perturbation",
		  MENUFLAG_NOMENU |
		  MENUFLAG_INTERRUPT,
		  uih_setperbutation, uih_getperturbationdialog);
    MENUSEPARATOR_I("fractal");
    MENUCDIALOG_I("fractal", NULL,
		  gettext("View"), "uiview",
		  MENUFLAG_INTERRUPT | UI, uih_dview, uih_getviewdialog);
    MENUSEPARATOR_I("fractal");
    MENUNOP_I("fractal", NULL,
	      gettext("Reset to defaults"), "initstate", 0, uih_initstate);
    MENUDIALOG_I("fractal", NULL, gettext("Julia mode"), "julia",
		 MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_playjulia,
		 uih_juliamodedialog);
    MENUDIALOG_I("fractal", NULL,
		 gettext("View"), "view",
		 MENUFLAG_NOMENU |
		 MENUFLAG_INTERRUPT, uih_plview, uih_plviewdialog);
    MENUDIALOG_I("fractal", NULL, gettext("Set angle"), "angle",
		 MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_angle,
		 uih_angledialog);
    MENUDIALOG_I("fractal", NULL,
		 gettext("Set plane"),
		 "plane",
		 MENUFLAG_NOMENU |
		 MENUFLAG_INTERRUPT, uih_setplane, uih_numdialog);
    MENUDIALOG_I("fractal", NULL, gettext("Inside coloring mode"),
		 "incoloring", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT,
		 uih_setincoloringmode, uih_numdialog);
    MENUDIALOG_I("fractal", NULL,
		 gettext
		 ("Outside coloring mode"),
		 "outcoloring",
		 MENUFLAG_NOMENU |
		 MENUFLAG_INTERRUPT, uih_setoutcoloringmode,
		 uih_numdialog);
    MENUDIALOG_I("fractal", NULL,
		 gettext("Inside truecolor coloring mode"), "intcoloring",
		 MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_setintcolor,
		 uih_numdialog);
    MENUDIALOG_I("fractal", NULL,
		 gettext("Outside truecolor coloring mode"),
		 "outtcoloring", MENUFLAG_NOMENU | MENUFLAG_INTERRUPT,
		 uih_setouttcolor, uih_numdialog);
    MENUDIALOG_I("fractal", NULL, gettext("Julia seed"), "juliaseed",
		 MENUFLAG_NOMENU | MENUFLAG_INTERRUPT, uih_setjuliaseed,
		 uih_coorddialog);
    MENUNOP_I("palettemenu", "d", gettext("Default palette"), "defpalette",
	      0, uih_mkdefaultpalette);
    MENUNOP_I("palettemenu", "p", gettext("Random palette"),
	      "randompalette", 0, uih_menumkpalette);
    MENUCDIALOG_I("palettemenu", NULL, gettext("Custom palette"),
		  "palette", 0, uih_palette, uih_getpalettedialog);
    MENUSEPARATOR_I("palettemenu");
    MENUNOPCB_I("palettemenu", "y",
		gettext("Color cycling"),
		"cycling", 0, uih_cyclingsw, uih_cyclingselected);
    MENUNOPCB_I("palettemenu", "Y", gettext("Reversed color cycling"),
		"rcycling", MENUFLAG_NOOPTION | MENUFLAG_NOPLAY,
		uih_rcyclingsw, uih_rcyclingselected);
    MENUCDIALOG_I("palettemenu", NULL,
		  gettext
		  ("Color cycling speed"),
		  "cyclingspeed", 0, uih_setcycling, uih_getcyclingdialog);
    MENUSEPARATOR_I("palettemenu");
    MENUDIALOG_I("palettemenu", NULL,
		 gettext("Shift palette"),
		 "shiftpalette", 0, uih_shiftpalette, uih_shiftdialog);
    MENUNOP_I("palettemenu", "+", gettext("Shift one forward"), "fshift",
	      MENUFLAG_NOOPTION | MENUFLAG_NOPLAY, uih_fshift);
    MENUNOP_I("palettemenu", "-",
	      gettext("Shift one backward"),
	      "bshift", MENUFLAG_NOOPTION | MENUFLAG_NOPLAY, uih_bshift);
    SUBMENU_I("calc", NULL, gettext("Solid guessing"), "mguess");
    SUBMENU_I("calc", NULL, gettext("Dynamic resolution"), "dynamic");
    MENUNOPCB_I("calc", "k",
		gettext("Periodicity checking"),
		"periodicity", 0, uih_periodicitysw,
		uih_periodicityselected);
    MENUCDIALOG_I("calc", NULL, gettext("Iterations"), "maxiter",
		  MENUFLAG_INTERRUPT, uih_setmaxiter, uih_getiterdialog);
    MENUCDIALOG_I("calc", NULL, gettext("Bailout"), "bailout",
		  MENUFLAG_INTERRUPT, uih_setbailout,
		  uih_getbailoutdialog);
    MENUSEPARATOR_I("calc");
    MENUNOPCB_I("calc", "j", gettext("Fast julia mode"), "fastjulia", 0,
		uih_juliasw, uih_juliaselected);
    SUBMENU_I("calc", "o", gettext("Rotation"), "rotate");
    MENUDIALOG_I("calc", NULL, gettext("Solid guessing range"), "range",
		 MENUFLAG_NOMENU, uih_setguessing, uih_numdialog);
    MENUINTRB_I("rotate", NULL,
		gettext("Disable rotation"),
		"norotate", UI, uih_rotate, 0, uih_rotateselected);
    MENUSEPARATOR_I("rotate");
    MENUINTRB_I("rotate", NULL,
		gettext("Continuous rotation"),
		"controtate", UI, uih_rotate,
		ROTATE_CONTINUOUS, uih_rotateselected);
    MENUINTRB_I("rotate", NULL, gettext("Rotate by mouse"), "mouserotate",
		UI, uih_rotate, ROTATE_MOUSE, uih_rotateselected);
    MENUCDIALOG_I("rotate", NULL,
		  gettext
		  ("Rotation speed"),
		  "rotationspeed", 0,
		  uih_rotationspeed, uih_getrotationdialog);
    MENUDIALOG_I("rotate", NULL, gettext("Automatic rotation"),
		 "autorotate", MENUFLAG_NOMENU, uih_playautorotate,
		 uih_autorotatedialog);
    MENUDIALOG_I("rotate", NULL,
		 gettext
		 ("Fast rotation mode"),
		 "fastrotate",
		 MENUFLAG_NOMENU,
		 (funcptr) uih_fastrotate, uih_fastrotatedialog);
    MENUINTRB_I("dynamic", NULL, gettext("Disable dynamic resolution"),
		"nodynamic", UI, uih_setfastmode, 1, uih_fastmode);
    MENUSEPARATOR_I("dynamic");
    MENUINTRB_I("dynamic", NULL, gettext("Use only during animation"),
		"dynamicanimation", UI, uih_setfastmode, 2, uih_fastmode);
    MENUINTRB_I("dynamic", NULL,
		gettext
		("Use also for new images"),
		"dynamicnew", UI, uih_setfastmode, 3, uih_fastmode);
    MENUDIALOG_I("dynamic", NULL, gettext("Dynamic resolution mode"),
		 "fastmode", MENUFLAG_NOMENU, uih_setfastmode,
		 uih_fastmodedialog);
    MENUNOPCB_I("ui", "a",
		gettext("Autopilot"),
		"autopilot", 0, uih_autopilotsw, uih_autopilotselected);
    MENUSEPARATOR_I("ui");
    MENUNOPCB_I("ui", "v",
		gettext("VJ mode"),
		"inhibittextoutput", 0, uih_inhibittextsw,
		uih_inhibittextselected);
    MENUSEPARATOR_I("ui");
    MENUNOP_I("ui", "r", gettext("Recalculate"),
	      "recalculate", 0, uih_recalculate);
    MENUNOP_I("ui", "z",
	      gettext
	      ("Interrupt"),
	      "interrupt",
	      MENUFLAG_INTERRUPT | MENUFLAG_INCALC, uih_interrupt);
    MENUSEPARATOR_I("ui");
    MENUCDIALOG_I("ui", NULL,
		  gettext("Zooming speed"), "speed",
		  0, uih_setspeed, uih_getspeeddialog);
    MENUNOPCB_I("ui", NULL, gettext("Fixed step"), "fixedstep", 0,
		uih_fixedstepsw, uih_fixedstepselected);
    MENUINTRB_I("mguess", NULL,
		gettext
		("Disable solid guessing"),
		"noguess", UI, uih_setguessing, 1, uih_guessingselected);
    MENUSEPARATOR_I("mguess");
    MENUINTRB_I("mguess", NULL,
		gettext("Guess 2x2 rectangles"),
		"guess2", UI, uih_setguessing, 2, uih_guessingselected);
    MENUINTRB_I("mguess", NULL, gettext("Guess 3x3 rectangles"), "guess3",
		UI, uih_setguessing, 3, uih_guessingselected);
    MENUINTRB_I("mguess", NULL,
		gettext
		("Guess 4x4 rectangles"),
		"guess4", UI, uih_setguessing, 4, uih_guessingselected);
    MENUINTRB_I("mguess", NULL, gettext("Guess 5x5 rectangles"), "guess5",
		UI, uih_setguessing, 5, uih_guessingselected);
    MENUINTRB_I("mguess", NULL,
		gettext
		("Guess 6x6 rectangles"),
		"guess6", UI, uih_setguessing, 6, uih_guessingselected);
    MENUINTRB_I("mguess", NULL, gettext("Guess 7x7 rectangles"), "guess7",
		UI, uih_setguessing, 7, uih_guessingselected);
    MENUINTRB_I("mguess", NULL,
		gettext
		("Guess 8x8 rectangles"),
		"guess8", UI, uih_setguessing, 8, uih_guessingselected);
    MENUINTRB_I("mguess", NULL, gettext("Guess unlimited rectangles"),
		"guessall", UI, uih_setguessing, 2048,
		uih_guessingselected);
    /* Language selection is not sensible anymore if i18n is used: */
#ifndef HAVE_GETTEXT
    SUBMENU_I("tutor", NULL, gettext("Language"), "lang");
    MENUSEPARATOR_I("tutor");
#endif
    SUBMENU_I("tutor", NULL, gettext("An introduction to fractals"),
	      "intro");
    SUBMENU_I("tutor", NULL, gettext("XaoS features overview"),
	      "features");
    SUBMENU_I("tutor", NULL, gettext("Math behind fractals"), "fmath");
    SUBMENU_I("tutor", NULL, gettext("Other fractal types in XaoS"),
	      "otherf");
    SUBMENU_I("tutor", NULL, gettext("What's new?"), "new");
    /* Language selection is not sensible anymore if i18n is used: */
#ifndef HAVE_GETTEXT
    LANG_I("Cesky", "cesky");
    LANG_I("Deutsch", "deutsch");
    LANG_I("English", "english");
    LANG_I("Espanhol", "espanhol");
    LANG_I("Francais", "francais");
    LANG_I("Magyar", "magyar");
#endif
    TUTOR_I("intro", gettext("Whole story"), "fractal.xaf");
    MENUSEPARATOR_I("intro");
    TUTOR_I("intro", gettext("Introduction"), "intro.xaf");
    TUTOR_I("intro", gettext("Mandelbrot set"), "mset.xaf");
    TUTOR_I("intro", gettext("Julia set"), "julia.xaf");
    TUTOR_I("intro", gettext("Higher power Mandelbrots"), "power.xaf");
    TUTOR_I("intro", gettext("Newton's method"), "newton.xaf");
    TUTOR_I("intro", gettext("Barnsley's formula"), "barnsley.xaf");
    TUTOR_I("intro", gettext("Phoenix"), "phoenix.xaf");
    TUTOR_I("intro", gettext("Octo"), "octo.xaf");
    TUTOR_I("intro", gettext("Magnet"), "magnet.xaf");
    TUTOR_I("features", gettext("All features"), "features.xaf");
    MENUSEPARATOR_I("features");
    TUTOR_I("features", gettext("Outcoloring modes"), "outcolor.xaf");
    TUTOR_I("features", gettext("Incoloring modes"), "incolor.xaf");
    TUTOR_I("features", gettext("True-color coloring modes"),
	    "truecol.xaf");
    TUTOR_I("features", gettext("Filters"), "filter.xaf");
    TUTOR_I("features", gettext("Planes"), "plane.xaf");
    TUTOR_I("features", gettext("Animations and position files"),
	    "anim.xaf");
    TUTOR_I("features", gettext("Perturbation"), "pert.xaf");
    TUTOR_I("features", gettext("Random palettes"), "palette.xaf");
    TUTOR_I("features", gettext("Other noteworthy features"), "other.xaf");
    TUTOR_I("fmath", gettext("Whole story"), "fmath.xaf");
    MENUSEPARATOR_I("fmath");
    TUTOR_I("fmath", gettext("The definition and fractal dimension"),
	    "dimension.xaf");
    TUTOR_I("fmath", gettext("Escape time fractals"), "escape.xaf");
    TUTOR_I("otherf", gettext("Other fractal types in XaoS"),
	    "otherfr.xaf");
    MENUSEPARATOR_I("otherf");
    TUTOR_I("otherf", gettext("Triceratops and Catseye fractals"),
	    "trice.xaf");
    TUTOR_I("otherf", gettext("Mandelbar, Lambda, Manowar and Spider"),
	    "fourfr.xaf");
    TUTOR_I("otherf",
	    gettext("Sierpinski Gasket, S.Carpet, Koch Snowflake"),
	    "classic.xaf");
    TUTOR_I("new", gettext("What's new in 3.0?"), "new30.xaf");
#ifdef DEBUG
    printf("Filled %d menu items out of %d.\n", no_menuitems_i18n,
	   MAX_MENUITEMS_I18N);
#endif
    menu_add(menuitems_i18n, no_menuitems_i18n);
    uih_no_menuitems_i18n = no_menuitems_i18n;
}


static const menuitem menuitems2[] = {
    SUBMENU("mincoloring", NULL, "True-color incoloring mode",
	    "tincoloring"),
    SUBMENU("moutcoloring", NULL, "True-color outcoloring mode",
	    "toutcoloring")
};

static int uih_selectedformula(struct uih_context *c, int n)
{
    if (c == NULL)
	return 0;
    return (c->fcontext->currentformula == formulas + n);
}

static int uih_selectedincoloring(struct uih_context *c, int n)
{
    if (c == NULL)
	return 0;
    return (c->fcontext->incoloringmode == n);
}

static void uih_setintruecolor(struct uih_context *c, int n)
{
    uih_setincoloringmode(c, 10);
    uih_setintcolor(c, n);
}

static int uih_selectedintcoloring(struct uih_context *c, int n)
{
    if (c == NULL)
	return 0;
    return (c->fcontext->intcolor == n);
}

static int uih_selectedoutcoloring(struct uih_context *c, int n)
{
    if (c == NULL)
	return 0;
    return (c->fcontext->coloringmode == n);
}

static int uih_selectedplane(struct uih_context *c, int n)
{
    if (c == NULL)
	return 0;
    return (c->fcontext->plane == n);
}

static void uih_setouttruecolor(struct uih_context *c, int n)
{
    uih_setoutcoloringmode(c, 10);
    uih_setouttcolor(c, n);
}

static int uih_selectedouttcoloring(struct uih_context *c, int n)
{
    if (c == NULL)
	return 0;
    return (c->fcontext->outtcolor == n);
}

static int uih_filterenabled(struct uih_context *c, int n)
{
    if (c == NULL)
	return 0;
    return (c->filter[n] != NULL);
}

static void uih_filtersw(struct uih_context *c, int n)
{
    if (c->filter[n] != NULL)
	uih_disablefilter(c, n);
    else
	uih_enablefilter(c, n);
}

static menuitem *formulaitems;
static menuitem *filteritems;
static char (*keys)[2];
void uih_registermenus(void)
{
    menuitem *item;
    int i;
    menu_add(menuitems, NITEMS(menuitems));
    formulaitems = item =
	(menuitem *) malloc(sizeof(menuitem) * nformulas);

    /* This code automatically generates code for fractal, incoloring and other
     * menus*/
    keys = malloc(sizeof(*keys) * nformulas);
    for (i = 0; i < nformulas; i++) {
	if (i < nmformulas) {
	    item[i].menuname = "mformula";
	} else {
	    item[i].menuname = "oformula";
	}
	item[i].key = keys[i];
	if (i < 9)
	    keys[i][0] = '1' + i;
	else if (i == 9)
	    keys[i][0] = '0';
#ifndef _MAC
	else
	    keys[i][0] = '7' + i;
#endif
	keys[i][1] = 0;
	item[i].type = MENU_INT;
	item[i].flags =
	    MENUFLAG_RADIO | MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY;
	item[i].iparam = i;
	item[i].name = formulas[i].name[!formulas[i].mandelbrot];
	item[i].shortname = formulas[i].shortname;
	item[i].function = (void (*)(void)) uih_setformula;
	item[i].control = (int (*)(void)) uih_selectedformula;
    }
    menu_add(item, nformulas);

    menu_genernumbered(INCOLORING - 1, "mincoloring", incolorname, NULL,
		       MENU_INT, UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT,
		       uih_setincoloringmode, uih_selectedincoloring,
		       "in");

    menu_genernumbered(TCOLOR - 1, "tincoloring", tcolorname, NULL,
		       MENU_INT, UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT,
		       uih_setintruecolor, uih_selectedintcoloring, "int");

    menu_genernumbered(OUTCOLORING - 1, "moutcoloring", outcolorname, NULL,
		       MENU_INT, UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT,
		       uih_setoutcoloringmode, uih_selectedoutcoloring,
		       "out");

    menu_genernumbered(TCOLOR - 1, "toutcoloring", tcolorname, NULL,
		       MENU_INT, UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT,
		       uih_setouttruecolor, uih_selectedouttcoloring,
		       "outt");

    {
	int i;
	for (i = 0; planename[i] != NULL; i++);
	menu_genernumbered(i, "mplane", planename, NULL, MENU_INT,
			   UI | MENUFLAG_RADIO | MENUFLAG_INTERRUPT,
			   uih_setplane, uih_selectedplane, "plane");
    }
    filteritems = item =
	(menuitem *) malloc(sizeof(menuitem) * uih_nfilters);

    /* This code automatically generates code for fractal, incoloring and other
     * menus*/
    for (i = 0; i < uih_nfilters; i++) {
	item[i].menuname = "mfilter";
	item[i].key = NULL;
	item[i].type = MENU_INT;
	item[i].flags =
	    MENUFLAG_CHECKBOX | MENUFLAG_INTERRUPT | MENUFLAG_NOPLAY;
	item[i].iparam = i;
	item[i].name = uih_filters[i]->name;
	item[i].shortname = uih_filters[i]->shortname;
	if (!strcmp(item[i].shortname, "palette"))
	    item[i].shortname = "palettef";
	/*this is one name collision because of ugly historical reasons */
	item[i].function = (void (*)(void)) uih_filtersw;
	item[i].control = (int (*)(void)) uih_filterenabled;
    }
    menu_add(item, uih_nfilters);

    menu_add(menuitems2, NITEMS(menuitems2));
}

void uih_unregistermenus(void)
{
    menu_delete(menuitems, NITEMS(menuitems));
    menu_delete(menuitems_i18n, uih_no_menuitems_i18n);

    free(keys);
    menu_delete(formulaitems, nformulas);
    free(formulaitems);

    menu_delnumbered(INCOLORING - 1, "in");

    menu_delnumbered(TCOLOR - 1, "int");

    menu_delnumbered(OUTCOLORING - 1, "out");

    menu_delnumbered(TCOLOR - 1, "outt");
    {
	int i;
	for (i = 0; planename[i] != NULL; i++);
	menu_delnumbered(i, "plane");
    }

    menu_delete(filteritems, uih_nfilters);
    free(filteritems);

    menu_delete(menuitems2, NITEMS(menuitems2));
}

#ifdef SFFE_USING
void uih_sffein(uih_context * c, const char *text)
{
    char str[200];
    int err;
    if (strlen(text)) {
	c->parser->errormsg = (char *) str;
	if ((err = sffe_parse(&c->parser, (char *) text)) > 0) {
	    uih_message(c, str);
	    sffe_parse(&c->parser, "z^2+c");
	    uih_sffedialog->defstr = c->parser->expression;
	} else {
	    uih_sffedialog->defstr = c->parser->expression;
	    uih_message(c, c->parser->expression);
	    if (!(c->fcontext->currentformula->flags & SFFE_FRACTAL)) {
		uih_play_formula(c, "user");
	    } else
		uih_recalculate(c);
	};

	c->parser->errormsg = NULL;
    };
};

void uih_sffeinitin(uih_context * c, const char *text)
{
    extern cmplx pZ;
    extern cmplx C;
    char str[200];
    int err;
    uih_sffeinitdialog->defstr = "";
    if (strlen(text)) {
	if (!c->pinit) {
	    c->pinit = sffe_alloc();
	    sffe_regvar(&c->pinit, &pZ, 'p');
	    sffe_regvar(&c->pinit, &C, 'c');
	};

	c->pinit->errormsg = (char *) str;
	if ((err = sffe_parse(&c->pinit, (char *) text)) > 0) {
	    uih_message(c, str);
	    sffe_free(&c->pinit);
	    c->pinit = NULL;
	} else {
	    uih_sffeinitdialog->defstr = c->pinit->expression;	/*FIXME shouldnt this be done by str copy */
	    uih_message(c, c->pinit->expression);
	    if (!(c->fcontext->currentformula->flags & SFFE_FRACTAL)) {
		uih_play_formula(c, "user");
	    } else
		uih_recalculate(c);
	    c->pinit->errormsg = NULL;
	};

    } else if (c->pinit) {
	sffe_free(&c->pinit);
	c->pinit = NULL;
	uih_recalculate(c);
    };
};
#endif
