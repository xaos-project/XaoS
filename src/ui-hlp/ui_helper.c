#include <config.h>
#ifndef _plan9_
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#ifdef __EMX__
#include <float.h>
/*
   #ifndef M_PI
   #define M_PI 3.14159265358979323846
   #endif
 */
#endif
#else
#include <u.h>
#include <libc.h>
#include <ctype.h>
#endif
#include <fconfig.h>
#include <filter.h>
#include <ui_helper.h>
#include <plane.h>
#include "../include/timers.h"
#include <zoom.h>
#include <xmenu.h>
#include <xerror.h>
#include "autopilot.h"
#include "grlib.h"
#include "play.h"
#include <archaccel.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#ifdef SFFE_USING
#include "sffe.h"
#endif

#ifdef HAVE_GETTEXT
#include <libintl.h>
#else
#define gettext(STRING) STRING
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define WAITTIME  50000
#define WAITTIME1 200000
#define WAITTIME2 2000000
#define uih_palettechg(uih) if(!uih->recalculatemode&&uih->queue->palettechg!=NULL) uih->recalculatemode=UIH_PALETTEDRAW, uih->display=1
#include <misc-f.h>

static struct filter *uih_getinstance(const struct filteraction *a);
static void uih_destroyinstance(struct filter *f);
static int uih_require(struct filter *f, struct requirements *r);
static int uih_initialize(struct filter *f, struct initdata *i);
static const rgb_t uicolors[] = {
    {0, 0, 0},
    {255, 255, 255},
    {255, 65, 0},
    {64, 64, 64},
    {128, 128, 128},
    {128 + 64, 128 + 64, 128 + 64}
};

static const rgb_t uibwcolors[] = {
    {0, 0, 0},
    {255, 255, 255},
    {255, 255, 255},
    {255, 255, 255},
    {255, 255, 255},
    {255, 255, 255}
};

static const struct filteraction uih_filter = {
    "XaoS's user interface layer",
    "ui",
    0,
    uih_getinstance,
    uih_destroyinstance,
    NULL,
    uih_require,
    uih_initialize,
    convertupgeneric,
    convertdowngeneric,
    NULL
};

static uih_context *uih;
static int waitcount, waitcount1, waitcount2;

extern const struct filteraction interlace_filter, stereogram_filter,
    subwindow_filter, smalliter_filter, julia_filter, blur_filter,
    edge_filter, edge2_filter, rotate_filter, starfield_filter,
    truecolor_filter, fixedcolor_filter, bitmap_filter, emboss_filter,
    palette_filter, antialias_filter, threed_filter;

const struct filteraction *const uih_filters[MAXFILTERS] = {
    &edge_filter,
    &edge2_filter,
    &threed_filter,
    &starfield_filter,
    &stereogram_filter,
    &interlace_filter,
    &blur_filter,
    &emboss_filter,
    &palette_filter,
    &antialias_filter,
    &truecolor_filter,
    NULL
};

const int uih_nfilters = 11;

static void uih_invalidatepos(uih_context * uih)
{
    uih->xcenterm = INT_MAX;
    uih->xcenterm = INT_MAX;
}

static void uih_finishpalette(struct uih_context *uih)
{
    if (uih->image->palette->flags & UNFINISHED) {
	if (uih->image->palette->allocfinished != NULL)
	    uih->image->palette->allocfinished(uih->image->palette);
	uih->image->palette->flags &= ~UNFINISHED;
    }
}

static void
uih_getcoord(uih_context * uih, int x, int y, number_t * xr, number_t * yr)
{
    uih->uifilter->action->convertdown(uih->uifilter, &x, &y);
    *xr = (((number_t)
	    (uih->fcontext->rs.nc +
	     (x) * ((uih->fcontext->rs.mc - uih->fcontext->rs.nc) /
		    (number_t) uih->zengine->image->width))));
    *yr = (((number_t)
	    (uih->fcontext->rs.ni +
	     (y) * ((uih->fcontext->rs.mi - uih->fcontext->rs.ni) /
		    (number_t) uih->zengine->image->height))));
    rotateback(*(uih->fcontext), *xr, *yr);
}

int uih_enablefilter(uih_context * c, int n)
{
    if (c->filter[n] == NULL) {
	struct filter *f, *f1;
	int i, wascycling = 0;
	if (c->cycling)
	    uih_cycling_off(c), wascycling = 1;
	f = uih_filters[n]->getinstance(uih_filters[n]);
	f1 = c->uifilter;
	if (c->fixedcolor != NULL)
	    f1 = c->fixedcolor;
	for (i = MAXFILTERS - 1; i > n; i--) {
	    if (c->filter[i])
		f1 = c->filter[i];
	}
	uih_newimage(c);
	insertfilter(f, f1);
	if (!initqueue(c->queue)) {
	    c->ddatalost = 1;
	    removefilter(f);
	    f->action->destroyinstance(f);
	    if (!initqueue(c->queue)) {
		x_fatalerror
		    ("Fatal error. Can not continue - initialization of queue can not be performed eigher with or without filter");
	    }
	    if (wascycling)
		uih_cycling_on(c);
	    if (!strcmp("palette", uih_filters[n]->shortname)) {
		uih_updatemenus(c, "palettef");
	    } else {
		uih_updatemenus(c, uih_filters[n]->shortname);
	    }
	    return 0;
	} else
	    c->filter[n] = f;
	if (wascycling)
	    uih_cycling_on(c);
	if (!strcmp("palette", uih_filters[n]->shortname)) {
	    uih_updatemenus(c, "palettef");
	} else {
	    uih_updatemenus(c, uih_filters[n]->shortname);
	}
	return 1;
    }
    return 0;
}

void uih_disablefilter(uih_context * c, int n)
{
    if (n == c->aliasnum)
	return;
    if (c->filter[n] != NULL) {
	int wascycling = 0;
	struct filter *f = c->filter[n];
	if (c->cycling)
	    uih_cycling_off(c), wascycling = 1;
	uih_newimage(c);
	removefilter(f);
	if (!initqueue(c->queue)) {
	    struct filter *f1;
	    int i;
	    c->ddatalost = 1;
	    f1 = c->uifilter;
	    if (c->fixedcolor != NULL)
		f1 = c->fixedcolor;
	    for (i = MAXFILTERS - 1; i > n; i--) {
		if (c->filter[i])
		    f1 = c->filter[i];
	    }
	    insertfilter(f, f1);
	    if (!initqueue(c->queue)) {
		x_fatalerror("Fatal error. Can not continue");
	    }
	} else
	    f->action->destroyinstance(f), c->filter[n] = NULL;
	if (wascycling)
	    uih_cycling_on(c);
	if (!strcmp("palette", uih_filters[n]->shortname)) {
	    uih_updatemenus(c, "palettef");
	} else {
	    uih_updatemenus(c, uih_filters[n]->shortname);
	}
    }
}

int uih_fastrotateenable(uih_context * c)
{
    int wascycling = 0;
    if (c->juliamode)
	uih_disablejulia(c);
    if (!c->fastrotate && !c->juliamode) {
	if (c->cycling)
	    uih_cycling_off(c), wascycling = 1;
	c->rotatef = rotate_filter.getinstance(&rotate_filter);
	if (c->rotatef == NULL)
	    goto end;
	uih_newimage(c);
	addfilter(c->rotatef, c->zengine);
	if (!initqueue(c->queue))
	    goto end2;
	if (wascycling)
	    uih_cycling_on(c);
	c->fastrotate = 1;
	return 1;
    }
    return 0;
  end2:
    removefilter(c->rotatef);
    initqueue(c->queue);
    c->rotatef->action->destroyinstance(c->rotatef);
  end:
    if (wascycling)
	uih_cycling_on(c);
    return 0;
}

void uih_fastrotatedisable(uih_context * c)
{
    if (c->fastrotate) {
	int wascycling = 0;
	uih_rotatemode(c, ROTATE_NONE);
	if (c->cycling)
	    uih_cycling_off(c), wascycling = 1;
	c->fastrotate = 0;
	removefilter(c->rotatef);
	initqueue(c->queue);
	c->rotatef->action->destroyinstance(c->rotatef);
	uih_newimage(c);
	if (wascycling)
	    uih_cycling_on(c);
    }
}

void uih_rotate(struct uih_context *c, int n)
{
    if (!n)
	uih_fastrotate(c, 0);
    else {
	uih_fastrotate(c, 1);
	uih_rotatemode(c, n);
    }
}

static void uih_fixedcolordisable(uih_context * c)
{
#ifdef SCONVERTORS
    if (c->fixedcolor != NULL) {
	int wascycling = 0;
	if (c->cycling)
	    uih_cycling_off(c), wascycling = 1;
	initqueue(c->queue);
	removefilter(c->fixedcolor);
	initqueue(c->queue);
	c->fixedcolor->action->destroyinstance(c->fixedcolor);
	c->fixedcolor = NULL;
	uih_newimage(c);
	if (wascycling)
	    uih_cycling_on(c);
    }
#endif
}

static int uih_fixedcolorenable(uih_context * c)
{
#ifdef SCONVERTORS
    const struct filteraction *fa = NULL;
    int wascycling = 0;
    preallocpalette(c->palette);
    switch (c->image->palette->type) {
#ifdef SFIXEDCOLOR
    case FIXEDCOLOR:
	fa = &fixedcolor_filter;
	break;
#endif
#ifdef SBITMAPS
    case LBITMAP:
    case MBITMAP:
    case LIBITMAP:
    case MIBITMAP:
	fa = &bitmap_filter;
	break;
#endif
    default:
	x_fatalerror("Unsupported image type. Recompile XaoS");
    }
    if (c->fixedcolor != NULL && c->fixedcolor->action != fa)
	uih_fixedcolordisable(c);
    if (c->fixedcolor == NULL) {
	if (c->cycling)
	    uih_cycling_off(c), wascycling = 1;
	c->fixedcolor = fa->getinstance(fa);
	if (c->fixedcolor == NULL)
	    goto end;
	uih_newimage(c);
	addfilter(c->fixedcolor, c->uifilter->previous);
	if (!initqueue(c->queue))
	    goto end2;
	if (wascycling)
	    uih_cycling_on(c);
	return 1;
    }
    return 0;
  end2:
    removefilter(c->fixedcolor);
    c->fixedcolor->action->destroyinstance(c->fixedcolor);
    c->fixedcolor = NULL;
    initqueue(c->queue);
  end:
    if (wascycling)
	uih_cycling_on(c);
    return 0;
#else
    x_fatalerror("Fixed color not supported, please recompile XaoS");
    return 0;
#endif
}

int uih_fastrotate(uih_context * c, int mode)
{
    if (mode)
	return (uih_fastrotateenable(c));
    uih_fastrotatedisable(c);
    return 1;
}

void uih_angle(uih_context * c, number_t angle)
{
    if (angle != c->fcontext->angle) {
	if (!c->fastrotate) {
	    c->fcontext->version++;
	    uih_newimage(c);
	}
	c->fcontext->angle = angle;
	uih_animate_image(c);
    }
}

void uih_rotatemode(uih_context * c, int mode)
{
    const char *names[] = {
	"norotate",
	"mouserotate",
	"controtate"
    };
    if (c->fastrotate) {
	if (c->rotatemode != mode) {
	    c->rotatemode = mode;
	    if (mode == ROTATE_CONTINUOUS)
		tl_reset_timer(c->doittimer);
	    uih_updatemenus(c, names[mode]);
	}
    }
}

int uih_enablejulia(uih_context * c)
{
    int wascycling = 0;
    if (!c->juliamode && c->fcontext->mandelbrot
	/*&&c->fcontext->currentformula->calculate_julia != NULL */ ) {
	struct filter *addf = c->zengine;
	uih_newimage(c);
	if (c->fastrotate)
	    uih_fastrotatedisable(c);
	if (c->cycling)
	    uih_cycling_off(c), wascycling = 1;
	if (c->fcontext->currentformula->calculate_julia == NULL
	    || c->fcontext->slowmode)
	    c->julia = zoom_filter.getinstance(&zoom_filter);
	else
	    c->julia = julia_filter.getinstance(&julia_filter);
	if (c->julia == NULL)
	    goto end;

	c->subwindow = subwindow_filter.getinstance(&subwindow_filter);
	if (c->subwindow == NULL)
	    goto end2;
	if (c->fcontext->currentformula->calculate_julia != NULL
	    && !c->fcontext->slowmode) {
	    c->smalliter = smalliter_filter.getinstance(&smalliter_filter);
	    if (c->smalliter == NULL)
		goto end3;
	} else
	    c->smalliter = NULL;
	addfilter(c->subwindow, addf);
	if (c->fcontext->currentformula->calculate_julia != NULL
	    && !c->fcontext->slowmode) {
	    addfilter(c->smalliter, addf);
	}
	addfilter(c->julia, addf);
	subwindow_setsecond(c->subwindow, addf);
	if (!initqueue(c->queue))
	    goto end4;
	if (c->fcontext->currentformula->calculate_julia == NULL
	    || c->fcontext->slowmode)
	    c->juliamode = 2;
	else
	    c->juliamode = 1;
	uih_updatemenus(c, "fastjulia");
	return 1;
    }
    uih_updatemenus(c, "fastjulia");
    return 0;
  end4:;
    removefilter(c->subwindow);
    removefilter(c->julia);
    if (c->smalliter != NULL)
	removefilter(c->smalliter);
    initqueue(c->queue);
  end3:;
    c->smalliter->action->destroyinstance(c->smalliter);
  end2:;
    c->subwindow->action->destroyinstance(c->subwindow);
  end:;
    c->julia->action->destroyinstance(c->julia);
    if (wascycling)
	uih_cycling_on(c);
    uih_updatemenus(c, "fastjulia");
    return 0;
}

void uih_disablejulia(uih_context * c)
{
    int wascycling = 0;
    if (c->juliamode) {
	uih_newimage(c);
	c->fcontext->version++;
	if (c->cycling)
	    uih_cycling_off(c), wascycling = 1;
	removefilter(c->subwindow);
	removefilter(c->julia);
	if (c->smalliter != NULL)
	    removefilter(c->smalliter);
	initqueue(c->queue);
	if (c->smalliter != NULL)
	    c->smalliter->action->destroyinstance(c->smalliter);
	c->subwindow->action->destroyinstance(c->subwindow);
	c->julia->action->destroyinstance(c->julia);
	if (wascycling)
	    uih_cycling_on(c);
	c->juliamode = 0;
	uih_updatemenus(c, "fastjulia");
    }
}

int uih_setjuliamode(uih_context * c, int mode)
{
    if (mode)
	return uih_enablejulia(c);
    uih_disablejulia(c);
    return 1;
}

void uih_rotationspeed(uih_context * c, number_t speed)
{
    c->rotationspeed = speed;
}

static void uih_cyclinghandler(void *userdata, int n)
{
    struct uih_context *uih = (struct uih_context *) userdata;
    int direct;
    if (uih->zengine->fractalc->palette != NULL &&
	uih->zengine->fractalc->palette->cyclecolors == NULL)
	return;
    direct = uih->direction * uih->cyclingdirection * n;
    if (direct > 0)
	direct %= uih->zengine->fractalc->palette->size - 1;
    else
	direct =
	    -((-direct) % (uih->zengine->fractalc->palette->size - 1));
    if (direct) {
	uih->paletteshift += direct;
	while (uih->paletteshift < 0)
	    uih->paletteshift += uih->zengine->fractalc->palette->size - 1;
	uih->paletteshift =
	    uih->paletteshift % (uih->zengine->fractalc->palette->size -
				 1);
	uih->zengine->fractalc->palette->cyclecolors(uih->
						     zengine->fractalc->
						     palette, direct);
	if (uih->flags & UPDATE_AFTER_PALETTE
	    && (!uih->play || !uih->nonfractalscreen))
	    uih->display = 1;
	uih_palettechg(uih);
    }
}

void uih_cycling_off(struct uih_context *c)
{
    if (c->cycling) {
	tl_free_timer(c->cyclingtimer);
	c->cycling = 0;
	uih_updatemenus(c, "cycling");
	uih_updatemenus(c, "rcycling");
    }
}

void uih_display(struct uih_context *c)
{
    c->display = 1;
    c->nonfractalscreen = 0;
    if (c->clearscreen)
	c->clearscreen = 0;
    c->displaytext = 0;
    c->nletters = 0;
    c->display = 1;
    if (c->text[0] != NULL)
	free(c->text[0]), c->text[0] = NULL;
    if (c->text[1] != NULL)
	free(c->text[1]), c->text[1] = NULL;
    if (c->text[2] != NULL)
	free(c->text[2]), c->text[2] = NULL;
    if (c->play)
	uih_clear_lines(c);
}

void uih_cycling_stop(struct uih_context *c)
{
    if (c->cycling && !c->stopped) {
	tl_remove_timer(c->cyclingtimer);
	c->stopped = 1;
    }
}

void uih_cycling_continue(struct uih_context *c)
{
    if (c->cycling && c->stopped) {
	c->stopped = 0;
	tl_add_timer(syncgroup, c->cyclingtimer);
    }
}

void uih_loadfile(struct uih_context *c, xio_constpath d)
{
    xio_file f;
    f = xio_ropen(d);
    if (f == NULL) {
#ifdef _plan9_
	uih_error(c, (char *) errstr);
#else
	uih_error(c, strerror(errno));
#endif
	return;
    }
    uih_load(c, f, d);
    return;
}

void uih_loadstr(struct uih_context *c, const char *data)
{
    xio_file f;
    f = xio_strropen(data);
    uih_load(c, f, "");
    return;
}

void uih_playstr(struct uih_context *c, const char *data)
{
    xio_file f;
    f = xio_strropen(mystrdup(data));
    uih_replayenable(c, f, "", 1);
    return;
}

void uih_recalculate(struct uih_context *c)
{
    c->fcontext->version++;
    uih_newimage(c);
}

void uih_playfile(struct uih_context *c, xio_constpath d)
{
    xio_file f;
    if (c->play)
	uih_replaydisable(c);
    f = xio_ropen(d);
    if (f == NULL) {
#ifdef _plan9_
	uih_error(c, (char *) errstr);
#else
	uih_error(c, strerror(errno));
#endif
	return;
    }
    uih_replayenable(c, f, d, 1);
    return;
}

void uih_playtutorial(struct uih_context *c, const char *name)
{
    xio_pathdata tmp;
    xio_file f = XIO_FAILED;

    f = xio_gettutorial(name, tmp);
    if (f == NULL) {
	uih_error(c, gettext("Tutorial files not found. Reinstall XaoS"));
	return;
    }
    uih_replayenable(c, f, tmp, 1);
    if (c->passfunc != NULL) {
	c->passfunc(c, 1, gettext("Preparing first image"), 0);
    }
}

void uih_loadexample(struct uih_context *c)
{
    xio_pathdata name;
    xio_file f = xio_getrandomexample(name);
    c->errstring = NULL;
    if (f == NULL) {
	uih_error(c, gettext("Could not open examples"));
	return;
    }
    uih_load(c, f, name);
    if (c->errstring == NULL) {
	char s[256];
#ifdef HAVE_LIBGEN_H
	sprintf(s, gettext("File %s loaded."), basename(name));
#else
        sprintf(s, gettext("File %s loaded."), name);
#endif
	uih_message(c, s);
    }
}

void uih_savepngfile(struct uih_context *c, xio_constpath d)
{
    const char *s;
    if (c->passfunc != NULL) {
	c->passfunc(c, 1, gettext("Saving image..."), 0);
    }
    if (c->recalculatemode) {
	uih_newimage(c);
	uih_clearwindows(c);
	uih_do_fractal(c);
    }
    if (c->interrupt) {
	uih_message(c, gettext("Save interrupted"));
	return;
    }
    c->errstring = NULL;
    s = uih_save(c, d);
    if (s != NULL)
	uih_error(c, s);
    if (c->errstring == NULL) {
	char s[256];
	sprintf(s, gettext("File %s saved."), d);
	uih_message(c, s);
    }
}

void uih_saveposfile(struct uih_context *c, xio_constpath d)
{
    xio_file f;
    c->errstring = NULL;
    f = xio_wopen(d);
    if (f == XIO_FAILED) {
	uih_error(c, gettext("Can not open file"));
	return;
    }
    uih_save_possition(c, f, UIH_SAVEPOS);
    if (c->errstring == NULL) {
	char s[256];
	sprintf(s, gettext("File %s saved."), d);
	uih_message(c, s);
    }
}

char *uih_savepostostr(struct uih_context *c)
{
    xio_file f;
    c->errstring = NULL;
    f = xio_strwopen();
    uih_save_possition(c, f, UIH_SAVEPOS);
    return (xio_getstring(f));
}

void uih_saveundo(struct uih_context *c)
{
    xio_file f;
    if (c->play)
	return;
    c->errstring = NULL;
    if (c->undo.undos[c->undo.last])
	free(c->undo.undos[c->undo.last]);
    f = xio_strwopen();
    uih_save_possition(c, f, UIH_SAVEPOS);
    c->undo.undos[c->undo.last] = xio_getstring(f);
    c->undo.last = (c->undo.last + 1) % UNDOLEVEL;
}

void uih_undo(struct uih_context *c)
{
    xio_file f;
    int pos = c->undo.last - 2;
    if (pos < 0)
	pos = UNDOLEVEL + pos;
    if (c->undo.undos[pos]) {
	f = xio_strropen(c->undo.undos[pos]);
	c->undo.undos[pos] = NULL;
	c->undo.last = pos;
	uih_load(c, f, "");
    }
}

void uih_redo(struct uih_context *c)
{
    xio_file f;
    int pos = c->undo.last;
    if (c->undo.undos[pos]) {
	f = xio_strropen(c->undo.undos[pos]);
	c->undo.undos[pos] = NULL;
	c->undo.last = pos;
	uih_load(c, f, "");
    }
}

extern xio_pathdata configfile;
void uih_savecfg(struct uih_context *c)
{
    xio_file f;
    c->errstring = NULL;
    f = xio_wopen(configfile);
    if (f == XIO_FAILED) {
	uih_message(c, (char *) xio_errorstring());
	return;
    }
    uih_save_possition(c, f, UIH_SAVEALL);
    if (c->errstring == NULL) {
	char s[256];
	sprintf(s, gettext("File %s saved."), configfile);
	uih_message(c, s);
    }
}

void uih_saveanimfile(struct uih_context *c, xio_constpath d)
{
    xio_file f;
    c->errstring = NULL;
    if (c->save) {
	uih_save_disable(c);
	uih_updatemenus(c, "record");
	return;
    }
    f = xio_wopen(d);
    if (f == XIO_FAILED) {
	uih_message(c, (char *) xio_errorstring());
	return;
    }
    uih_save_enable(c, f, UIH_SAVEANIMATION);
    if (c->errstring == NULL) {
	char s[256];
	sprintf(s, gettext("Recording to file %s enabled."), d);
	uih_message(c, s);
    }
    uih_updatemenus(c, "record");
}







const char *uih_save(struct uih_context *c, xio_constpath filename)
{
    const char *r;
    uih_cycling_stop(c);
    uih_stoptimers(c);
    uih_clearwindows(c);
    r = writepng(filename, c->queue->saveimage);
    uih_cycling_continue(c);
    uih_resumetimers(c);
    return (r);
}

void uih_setcycling(struct uih_context *c, int speed)
{
    c->cyclingspeed = speed;
    if (c->cyclingspeed < 0)
	c->direction = -1;
    else
	c->direction = 1;
    if (c->cycling) {
	if (c->cyclingspeed)
	    tl_set_interval(c->cyclingtimer,
			    1000000 / c->cyclingspeed * c->direction);
	else
	    tl_set_interval(c->cyclingtimer, 1000000 * 100);
    }
}

int uih_cycling_on(struct uih_context *c)
{
    if (c->zengine->fractalc->palette != NULL &&
	c->zengine->fractalc->palette->cyclecolors != NULL) {
	c->cycling = 1;
	tl_update_time();
	c->cyclingtimer = tl_create_timer();
	uih_emulatetimers(c);
	uih_setcycling(c, c->cyclingspeed);
	tl_set_multihandler(c->cyclingtimer, uih_cyclinghandler, c);
	tl_add_timer(syncgroup, c->cyclingtimer);
    } else {
	uih_updatemenus(c, "cycling");
	uih_updatemenus(c, "rcycling");
	return 0;
    }
    uih_updatemenus(c, "cycling");
    uih_updatemenus(c, "rcycling");
    return 1;
}

int uih_cycling(struct uih_context *uih, int mode)
{
    if (mode)
	return (uih_cycling_on(uih));
    uih_cycling_off(uih);
    return 1;
}



static void uih_waitfunc(struct filter *f)
{
    int l;
    tl_process_group(syncgroup, NULL);
    l = tl_lookup_timer(uih->calculatetimer);
    if (uih->interrupt)
	f->interrupt = 1, uih->endtime = l;
    if (uih->interruptiblemode) {
	if (f->incalculation && !uih->starttime)
	    uih->starttime = l;
	else if (uih->starttime && !f->incalculation && !uih->endtime)
	    uih->endtime = l;
	if ((uih->maxtime && l > uih->maxtime && f->readyforinterrupt)
	    || uih->interrupt)
	    f->interrupt = 1, uih->endtime = l;
    }
    if ((l) > (waitcount + 1) * WAITTIME) {
	int display = 0;
	if (!uih->interruptiblemode && l > (waitcount1 + 1) * WAITTIME1) {
	    display = 1;
	    waitcount1 = l / WAITTIME1;
	}
	if (f->image == uih->image && !uih->interruptiblemode
	    && l > (waitcount2 + 1) * WAITTIME2) {
	    if (!uih->play)
		uih->display = 1, uih_finishpalette(uih), display = 1;
	    waitcount2 = l / WAITTIME2;
	}
	if (uih->passfunc != NULL) {
	    f->interrupt |=
		uih->passfunc(uih, display, f->pass,
			      (float) (f->max ? f->pos * 100.0 /
				       f->max : 100.0));
	    uih->display = 0;
	}
	waitcount = l / WAITTIME;
    }
    uih_clearwindows(uih);
}

void uih_do_fractal(uih_context * c)
{
    int flags;
    int time;

    c->interrupt = 0;
    c->display = 0;
    uih = c;
    if (c->juliamode && !c->fcontext->mandelbrot) {
	uih_disablejulia(c);
    }
    if ((c->juliamode == 1
	 && (c->fcontext->currentformula->calculate_julia == NULL
	     || c->fcontext->slowmode)) || (c->juliamode == 2
					    && c->
					    fcontext->currentformula->
					    calculate_julia != NULL
					    && !c->fcontext->slowmode)) {
	uih_disablejulia(c);
	uih_enablejulia(c);
	c->fcontext->version++;
    }

    tl_update_time();
    if (c->recalculatemode < c->fastmode && c->emulator == NULL
	&& !c->fixedstep)
	c->interruptiblemode = 1;
    else
	c->interruptiblemode = 0;
    if (!c->interruptiblemode && c->recalculatemode > UIH_ANIMATION) {
	if (c->longwait != NULL)
	    c->longwait(c);
	uih_stoptimers(c);
    }

    tl_update_time();
    tl_reset_timer(c->calculatetimer);
    c->starttime = 0;
    c->endtime = 0;
    waitcount = tl_lookup_timer(c->calculatetimer) / WAITTIME + 2;
    waitcount1 = tl_lookup_timer(c->calculatetimer) / WAITTIME1 + 1;
    waitcount2 = tl_lookup_timer(c->calculatetimer) / WAITTIME2 + 1;
#ifdef _UNDEFINED_
    waitcount = 2;
    waitcount1 = 1;
    waitcount2 = 1;
#endif
    c->incalculation = 1;

    if (!(c->flags & ROTATE_INSIDE_CALCULATION))
	uih_cycling_stop(c);

    time = tl_lookup_timer(c->doittimer);
    if (c->rotatemode == ROTATE_CONTINUOUS) {
	c->fcontext->angle += c->rotationspeed * time / 1000000.0;
    }

    tl_reset_timer(c->doittimer);
    c->indofractal = 1;
    if (c->recalculatemode < UIH_PALETTEDRAW) {
	if (c->queue->palettechg != NULL)
	    flags =
		c->queue->palettechg->action->doit(c->queue->palettechg,
						   PALETTEONLY, 0);
	else
	    flags = CHANGED;
    } else
	flags =
	    c->uifilter->previous->action->doit(c->uifilter->previous,
						c->interruptiblemode ?
						INTERRUPTIBLE : 0, time);
    c->indofractal = 0;

    if (!(c->flags & ROTATE_INSIDE_CALCULATION))
	uih_cycling_continue(c);

    c->dirty = 0;
    if (c->inanimation)
	c->inanimation--;
    c->ddatalost = 0;
    c->recalculatemode = 0;

    if (flags & ANIMATION)
	c->fastanimation = 1;
    else
	c->fastanimation = 0;
    if (c->emulator)
	c->inanimation = 1;
    if (flags & (ANIMATION | UNCOMPLETTE)
	|| (c->rotatemode == ROTATE_CONTINUOUS)) {
	tl_resume_timer(c->doittimer);
	c->uncomplette = 1;
	c->inanimation = 2;
	if (flags & UNCOMPLETTE)
	    c->recalculatemode = UIH_ANIMATION;
	else
	    c->recalculatemode = UIH_FILTERANIMATION;
	c->display = 1;
    } else {
	tl_stop_timer(c->doittimer);
	c->uncomplette = 0;
    }
    if ((flags & CHANGED) && (!c->play || !c->nonfractalscreen)) {
	c->display = 1;
	if (flags & INEXACT)
	    c->dirty = 1;
    } else
	c->incalculation = 0;
    uih_callcomplette(c);
    if (c->autopilot)
	c->inanimation = 1;
}

void uih_prepare_image(uih_context * c)
{
    if (c->play)
	uih_update_lines(c);
    if (c->display) {
	uih_clearwindows(c);
	xprepareimage(c->image);
	if (uih_needrecalculate(c))
	    uih_do_fractal(c);
    }
}

void uih_callcomplette(uih_context * c)
{
    if (!c->uncomplette && !c->display && !c->recalculatemode
	&& !c->inanimation && c->complettehandler != NULL) {
	c->complettehandler(c->handlerdata);
    }
}

void uih_setcomplettehandler(uih_context * c, void (h) (void *), void *d)
{
    c->complettehandler = h;
    c->handlerdata = d;
}

void uih_letterspersec(uih_context * c, int n)
{
    if (n < 1)
	n = 1;
    c->letterspersec = n;
}

double uih_displayed(uih_context * c)
{
    int drawingtime;
    uih_finishpalette(c);
    if (c->indofractal)
	return 0;		/*image is currently calculating */
    if (c->recalculatemode)
	c->display = 1;
    else
	c->display = 0;
    tl_update_time();
    uih_resumetimers(c);
    c->nonfractalscreen = 0;
    c->nletters = 0;
    if (c->incalculation) {
	c->incalculation = 0;
	drawingtime = tl_lookup_timer(c->calculatetimer);
	if (c->emulator)
	    drawingtime = 0;
	if (c->lasttime == -1
	    || (drawingtime && c->lasttime
		&& (drawingtime / c->lasttime < 0.2
		    || drawingtime / c->lasttime > 4)))
	    c->lasttime = drawingtime;
	c->lasttime = (c->lasttime * 30 + drawingtime) / 31;
	c->lastspeed = c->lasttime ? 1000000.0 / c->lasttime : 100.0;
	if (c->interruptiblemode) {
	    int i;
	    int time1, time;
	    time1 = drawingtime;
	    time1 -= c->endtime;
	    time = (drawingtime - c->endtime) + c->starttime;
	    if (c->times[0][0] == -1) {
		for (i = 0; i < AVRGSIZE; i++)
		    c->times[0][i] = time, c->times[1][i] = time1;
		c->count[0] = time * AVRGSIZE, c->count[1] =
		    time1 * AVRGSIZE;
	    }
	    c->timespos = (c->timespos + 1) % AVRGSIZE;
	    c->count[0] += time - c->times[0][c->timespos];
	    c->count[1] += time1 - c->times[1][c->timespos];
	    c->times[0][c->timespos] = time;
	    c->times[1][c->timespos] = time1;
	    c->maxtime = (c->count[0] * 5) / AVRGSIZE;
	    if (c->step || c->pressed
		|| (c->play
		    && (c->playc->morph || c->playc->morphangle
			|| c->playc->morphjulia
			|| c->playc->lines.morphing))
		|| (c->rotatemode == ROTATE_CONTINUOUS)
		|| c->fastanimation) {
		if (c->maxtime > 1000000 / 25)
		    c->maxtime = c->count[0] * 3 / AVRGSIZE;
		if (c->maxtime > 1000000 / 15)
		    c->maxtime = 1000000 / 15;
	    } else {
		c->maxtime = 1000000 / 3;
	    }
	    if (c->maxtime < 1000000 / 30)
		c->maxtime = 1000000 / 30;
	    c->maxtime -= c->count[1] / AVRGSIZE;
	    if (c->maxtime < c->starttime + 10000)
		c->maxtime = c->starttime + 10000;
	}
    }
    uih_callcomplette(c);
    return (c->lastspeed);
}

void uih_text(uih_context * c, const char *text)
{
    int i, l;
    c->display = 1;
    if (c->text[c->ytextpos])
	free(c->text[c->ytextpos]);
    c->textpos[c->ytextpos] = c->xtextpos;
    c->textcolor[c->ytextpos] = c->color;
    c->displaytext |= 1 << c->ytextpos;
    c->text[c->ytextpos] = mystrdup(text);
    l = (int) strlen(text);
    c->todisplayletters = 0;
    for (i = 0; i < l; i++) {
	if (isalnum(text[i]))
	    c->todisplayletters++;
	if (text[i] == '-')
	    c->todisplayletters += 3;
	if (text[i] == '.')
	    c->todisplayletters += 2;
    }
    c->step = 0;
}

void uih_clearscreen(uih_context * c)
{
    c->clearscreen = 1;
    if (c->save)
	c->savec->clearscreen = 1;
    if (c->displaytext)
	c->displaytext = 0;
    if (c->text[0] != NULL)
	free(c->text[0]), c->text[0] = NULL;
    if (c->text[1] != NULL)
	free(c->text[1]), c->text[1] = NULL;
    if (c->text[2] != NULL)
	free(c->text[2]), c->text[2] = NULL;
    c->nletters = 0;
    c->display = 1;
    if (c->play)
	uih_clear_lines(c);
}

void uih_settextpos(uih_context * c, int x, int y)
{
    const char *names1[] = {
	"ytextposup",
	"ytextposmiddle",
	"ytextposbottom",
    };
    const char *names2[] = {
	"xtextposleft",
	"xtextposcenter",
	"xtextposright",
    };
    c->xtextpos = x;
    c->ytextpos = y;
    uih_updatemenus(c, names1[y]);
    uih_updatemenus(c, names2[x]);
}


/*timming routines */

void uih_tbreak(uih_context * c)
{
    c->tbreak = 1;
}

void uih_emulatetimers(uih_context * c)
{
    if (c->emulator == NULL)
	return;
    tl_emulate_timer(c->maintimer, c->emulator);
    tl_emulate_timer(c->doittimer, c->emulator);
    if (c->autopilot)
	tl_emulate_timer(c->autopilottimer, c->emulator);
    if (c->cycling) {
	tl_emulate_timer(c->cyclingtimer, c->emulator);
    }
    if (c->play) {
	tl_emulate_timer(c->playc->timer, c->emulator);
    }
    if (c->save) {
	tl_emulate_timer(c->savec->timer, c->emulator);
	tl_emulate_timer(c->savec->synctimer, c->emulator);
    }
}

static void uih_unemulatetimers(uih_context * c)
{
    tl_unemulate_timer(c->maintimer);
    tl_unemulate_timer(c->doittimer);
    if (c->autopilot)
	tl_unemulate_timer(c->autopilottimer);
    if (c->cycling)
	tl_unemulate_timer(c->cyclingtimer);
    if (c->play) {
	tl_unemulate_timer(c->playc->timer);
    }
    if (c->save) {
	tl_unemulate_timer(c->savec->timer);
	tl_unemulate_timer(c->savec->synctimer);
    }
}

void uih_constantframetime(uih_context * c, int time)
{
    if (c->emulator == NULL)
	c->emulator = tl_create_emulator();
    c->emulatedframetime = time;
    uih_emulatetimers(c);
}

void uih_noconstantframetime(uih_context * c)
{
    if (c->emulator == NULL)
	return;
    uih_unemulatetimers(c);
    tl_free_emulator(c->emulator);
    c->emulator = NULL;
}

void uih_stoptimers(uih_context * c)
{
    if (!c->stoppedtimers) {
	c->stoppedtimers = 1;
	c->display = 1;
	tl_stop_timer(c->maintimer);
	tl_stop_timer(c->doittimer);
	if (c->autopilot)
	    tl_stop_timer(c->autopilottimer);
	if (c->play) {
	    tl_stop_timer(c->playc->timer);
	    if (c->cycling)
		tl_stop_timer(c->cyclingtimer);
	}
	if (c->save) {
	    tl_stop_timer(c->savec->timer);
	    tl_stop_timer(c->savec->synctimer);
	    if (c->cycling)
		tl_stop_timer(c->cyclingtimer);
	}
    }
}

void uih_slowdowntimers(uih_context * c, int time)
{
    tl_slowdown_timer(c->maintimer, time);
    if (c->autopilot)
	tl_slowdown_timer(c->autopilottimer, time);
    if (c->play) {
	tl_slowdown_timer(c->playc->timer, time);
	if (c->cycling)
	    tl_slowdown_timer(c->cyclingtimer, time);
    }
    if (c->save) {
	tl_slowdown_timer(c->savec->timer, time);
	tl_slowdown_timer(c->savec->synctimer, time);
	if (c->cycling)
	    tl_slowdown_timer(c->cyclingtimer, time);
    }
}

void uih_resumetimers(uih_context * c)
{
    if (c->stoppedtimers) {
	c->stoppedtimers = 0;
	tl_resume_timer(c->maintimer);
	if (c->cycling)
	    tl_resume_timer(c->cyclingtimer);
	if (c->autopilot)
	    tl_resume_timer(c->autopilottimer);
	if (c->play) {
	    tl_resume_timer(c->playc->timer);
	}
	if (c->save) {
	    tl_resume_timer(c->savec->timer);
	    tl_resume_timer(c->savec->synctimer);
	}
    }
}

/*autopilot implementation */

static void uih_changed(void)
{
    uih_newimage(uih);
}

static void uih_autopilothandler(void *uih1, int n)
{
    uih = (uih_context *) uih1;
    do_autopilot(uih, &uih->autopilotx, &uih->autopiloty,
		 &uih->autopilotbuttons, uih_changed, n);
}

static INLINE void uih_zoom(uih_context * uih)
{
    uih->step += uih->speedup * 2 * uih->mul;
    if (uih->step > uih->maxstep)
	uih->step = uih->maxstep;
    else if (uih->step < -uih->maxstep)
	uih->step = -uih->maxstep;
}

static INLINE void uih_unzoom(uih_context * uih)
{
    uih->step -= uih->speedup * 2 * uih->mul;
    if (uih->step > uih->maxstep)
	uih->step = uih->maxstep;
    else if (uih->step < -uih->maxstep)
	uih->step = -uih->maxstep;
}

static INLINE void uih_slowdown(uih_context * uih)
{
    if (uih->step > 0) {
	if (uih->step < uih->speedup * uih->mul)
	    uih->step = 0;
	else
	    uih->step -= uih->speedup * uih->mul;
    } else if (uih->step < 0) {
	if (uih->step > -uih->speedup * uih->mul)
	    uih->step = 0;
	else
	    uih->step += uih->speedup * uih->mul;
    }
}

static INLINE void uih_zoomupdate(uih_context * uih)
{
    number_t x;
    number_t y;
    number_t mmul = pow((double) (1 - uih->step), (double) uih->mul);
    number_t mc = uih->fcontext->s.cr - uih->fcontext->s.rr / 2;
    number_t nc = uih->fcontext->s.cr + uih->fcontext->s.rr / 2;
    number_t mi = uih->fcontext->s.ci - uih->fcontext->s.ri / 2;
    number_t ni = uih->fcontext->s.ci + uih->fcontext->s.ri / 2;
    x = uih->xcenter, y = uih->ycenter;
    mc = x + (mc - x) * (mmul);
    nc = x + (nc - x) * (mmul);
    mi = y + (mi - y) * (mmul);
    ni = y + (ni - y) * (mmul);
    uih->fcontext->s.rr = nc - mc;
    uih->fcontext->s.ri = ni - mi;
    uih->fcontext->s.cr = (nc + mc) / 2;
    uih->fcontext->s.ci = (ni + mi) / 2;
    uih_animate_image(uih);
}


/*main uih loop */

int uih_update(uih_context * c, int mousex, int mousey, int mousebuttons)
{
    int inmovement = 0;
    int slowdown = 1;
    int time;
    uih = c;

    if (!mousebuttons && uih->lastbuttons)
	uih_saveundo(c);
    uih->lastbuttons = mousebuttons;
    if (c->incalculation)
	return 0;
    if (c->emulator != NULL)
	tl_elpased(c->emulator, c->emulatedframetime);

    if (mousebuttons == (BUTTON1 | BUTTON3))
	mousebuttons = BUTTON2;
    tl_process_group(syncgroup, NULL);	/*First we need to update timming */
    tl_update_time();
    time = tl_lookup_timer(c->maintimer);
    if (c->autopilot) {		/*now handle autopilot */
	tl_process_group(c->autopilotgroup, NULL);
    }
    if (!c->inanimation)
	time = 0;
    if (time > 2000000) {
	uih_slowdowntimers(uih, time - 2000000);
	time = 2000000;
    }
    if (c->inanimation)
	c->inanimation--;
    tl_reset_timer(c->maintimer);
    c->mul = (double) time / FRAMETIME;
    if (c->fixedstep)
	c->mul = 0.3;
    if (c->tbreak)
	c->mul = 1, c->tbreak--;
    if (c->mul == 0)
	c->mul = 0.00000001;
    if (c->play) {
	uih_playupdate(c);
	if (!c->play) {
	    c->inanimation = 2;
	    return 1;
	}
	if (c->playc->lines.morphing)	/*inmovement=1, c->display=1; */
	    uih_update_lines(c);
	if (c->step)
	    uih_zoomupdate(c), inmovement = 1;
	switch (c->zoomactive) {
	case 1:
	    uih_zoom(c), inmovement = 1;
	    break;
	case -1:
	    uih_unzoom(c), inmovement = 1;
	    break;
	default:
	    uih_slowdown(c);
	}
	if (c->playc->morph) {
	    int timer =
		tl_lookup_timer(c->playc->timer) - c->playc->starttime;
	    number_t mmul =	/*(tl_lookup_timer (c->playc->timer) - c->playc->starttime) / (number_t) (c->playc->frametime - c->playc->starttime); */
		MORPHVALUE(timer,
			   c->playc->frametime - c->playc->starttime,
			   c->playc->morphtimes[0],
			   c->playc->morphtimes[1]);
	    number_t srr, drr;
	    number_t mmul1;
	    if (c->playc->source.rr * c->fcontext->windowwidth >
		c->playc->source.ri * c->fcontext->windowheight)
		srr = c->playc->source.rr;
	    else
		srr = c->playc->source.ri;
	    if (c->playc->destination.rr * c->fcontext->windowwidth >
		c->playc->destination.ri * c->fcontext->windowheight)
		drr = c->playc->destination.rr;
	    else
		drr = c->playc->destination.ri;
	    if (srr == drr)
		mmul1 = mmul;
	    else
		mmul1 =
		    (exp(log(srr) + ((log(drr) - log(srr)) * mmul)) -
		     srr) / (drr - srr);
	    if (mmul1 > 1)
		mmul1 = 1;
	    if (mmul1 < 0)
		mmul1 = 0;
	    inmovement = 1;
	    c->fcontext->s.rr =
		c->playc->source.rr + (c->playc->destination.rr -
				       c->playc->source.rr) * mmul1;
	    c->fcontext->s.ri =
		c->playc->source.ri + (c->playc->destination.ri -
				       c->playc->source.ri) * mmul1;
	    c->fcontext->s.cr =
		c->playc->source.cr + (c->playc->destination.cr -
				       c->playc->source.cr) * mmul1;
	    c->fcontext->s.ci =
		c->playc->source.ci + (c->playc->destination.ci -
				       c->playc->source.ci) * mmul1;
	    uih_animate_image(c);
	}
	if (c->playc->morphjulia) {
	    int timer =
		tl_lookup_timer(c->playc->timer) - c->playc->starttime;
	    number_t mmul =	/*(tl_lookup_timer (c->playc->timer) - c->playc->starttime) / (number_t) (c->playc->frametime - c->playc->starttime); */
		MORPHVALUE(timer,
			   c->playc->frametime - c->playc->starttime,
			   c->playc->morphjuliatimes[0],
			   c->playc->morphjuliatimes[1]);
	    uih_setjuliaseed(uih,
			     c->playc->sr + (c->playc->dr -
					     c->playc->sr) * mmul,
			     c->fcontext->pim =
			     c->playc->si + (c->playc->di -
					     c->playc->si) * mmul);
	    inmovement = 1;
	}
	if (c->playc->morphangle) {
	    int timer =
		tl_lookup_timer(c->playc->timer) - c->playc->starttime;
	    number_t mmul =	/*(tl_lookup_timer (c->playc->timer) - c->playc->starttime) / (number_t) (c->playc->frametime - c->playc->starttime); */
		MORPHVALUE(timer,
			   c->playc->frametime - c->playc->starttime,
			   c->playc->morphangletimes[0],
			   c->playc->morphangletimes[1]);
	    uih_angle(uih,
		      c->playc->srcangle + (c->playc->destangle -
					    c->playc->srcangle) * mmul);
	    inmovement = 1;
	}
    } else {
	if (!c->juliamode) {
	    if (c->autopilot) {	/*now handle autopilot */
		mousex = c->autopilotx;
		mousey = c->autopiloty;
		mousebuttons = c->autopilotbuttons;
		inmovement = 1;
	    }
	    if (c->step) {
		number_t x;
		number_t y;
		if (mousex != c->xcenterm || mousey != c->ycenterm) {
		    c->xcenterm = mousex;
		    c->ycenterm = mousey;
		    uih_getcoord(uih, mousex, mousey, &x, &y);
		    c->xcenter = x;
		    c->ycenter = y;
		}
		uih_zoomupdate(c), inmovement = 1;
	    }
	    c->zoomactive = 0;
	    if (c->rotatemode != ROTATE_MOUSE)
		switch (mousebuttons) {	/*process buttons */
		case BUTTON1:
		    c->zoomactive = 1;
		    inmovement = 1;
		    break;
		case BUTTON3:
		    c->zoomactive = -1;
		    inmovement = 1;
		    break;
		}
	    uih_saveframe(c);
	    if (c->rotatemode != ROTATE_MOUSE) {
		c->rotatepressed = 0;
		switch (mousebuttons) {	/*process buttons */
		case BUTTON1:
		    uih_zoom(c), slowdown = 0;
		    break;
		case BUTTON3:
		    uih_unzoom(c), slowdown = 0;
		    break;
		case BUTTON2:
		    {
			number_t x, y;
			uih_getcoord(uih, mousex, mousey, &x, &y);
			if (c->pressed && (c->oldx != x || c->oldy != y)) {
			    c->fcontext->s.cr -= x - c->oldx;
			    c->fcontext->s.ci -= y - c->oldy;
			    uih_animate_image(c);
			    c->moved = 1;
			}
			c->pressed = 1;
			c->speed = 0;
			update_view(c->fcontext);
			uih_getcoord(uih, mousex, mousey, &c->oldx,
				     &c->oldy);
		    }
		    break;
		}
	    } else {
		if (mousebuttons & BUTTON1) {
		    number_t x, y;
		    number_t angle;

		    x = (mousex -
			 c->image->width / 2) * c->image->pixelwidth;
		    y = (mousey -
			 c->image->height / 2) * c->image->pixelheight;
		    angle = -atan2(x, y) * 180 / M_PI;
		    if (c->rotatepressed) {
			uih_angle(uih,
				  c->fcontext->angle + angle -
				  c->oldangle);
		    }
		    c->rotatepressed = 1;
		    c->oldangle = angle;
		} else
		    c->rotatepressed = 0;
	    }
	    if (!(mousebuttons & BUTTON2))
		c->pressed = 0;
	    if (slowdown)
		uih_slowdown(c);
	} else {
	    if (mousebuttons & BUTTON1) {
		number_t x, x1 = c->fcontext->pre;
		number_t y, y1 = c->fcontext->pim;
		c->zoomactive = 0;
		uih_getcoord(uih, mousex, mousey, &x, &y);
		c->fcontext->pre = x;
		c->fcontext->pim = y;
		uih_saveframe(c);
		c->pressed = 1;
		recalculate(c->fcontext->plane, &c->fcontext->pre,
			    &c->fcontext->pim);
		if (c->fcontext->pre != x1 || c->fcontext->pim != y1) {
		    uih_animate_image(c);
		}
	    } else
		c->pressed = 0;
	}
    }
    if (!inmovement)
	uih_tbreak(c);
    if (c->uncomplette)
	inmovement = 1;
    if (!c->recalculatemode && !c->display)
	uih_finishpalette(c);
    if (!inmovement)
	uih_callcomplette(c);
    if (c->inanimation < inmovement * 2)
	c->inanimation = inmovement * 2;
    return (inmovement * 2);
}

/*actions that can be used be user interface */

void uih_autopilot_on(uih_context * c)
{
    if (!c->autopilot) {
	clean_autopilot(c);
	uih_autopilothandler(c, 1);
	tl_update_time();
	uih_resumetimers(c);
	c->autopilottimer = tl_create_timer();
	c->autopilotgroup = tl_create_group();
	tl_set_multihandler(c->autopilottimer, uih_autopilothandler, c);
	tl_set_interval(c->autopilottimer, 1000000 / 25);
	tl_reset_timer(c->autopilottimer);
	tl_add_timer(c->autopilotgroup, c->autopilottimer);
	tl_update_time();
	c->autopilot = 1;
	uih_emulatetimers(c);
	uih_updatemenus(c, "autopilot");
    }
}

void uih_autopilot_off(uih_context * c)
{
    if (c->autopilot) {
	tl_remove_timer(c->autopilottimer);
	tl_free_timer(c->autopilottimer);
	tl_free_group(c->autopilotgroup);
	c->autopilot = 0;
	uih_updatemenus(c, "autopilot");
    }
}

void uih_mkdefaultpalette(uih_context * c)
{
    if (c->zengine->fractalc->palette == NULL)
	return;
    uih_cycling_stop(c);
    if (mkdefaultpalette(c->zengine->fractalc->palette) != 0) {
	uih_newimage(c);
    }
    uih_palettechg(c);
    c->paletteshift = 0;
    c->manualpaletteshift = 0;
    c->palettechanged = 1;
    c->palettetype = 0;
    uih_finishpalette(c);
    uih_cycling_continue(c);
}

void uih_mkpalette(uih_context * c)
{
    int seed;
    int alg = rand() % PALGORITHMS;
    if (c->zengine->fractalc->palette == NULL)
	return;
    uih_cycling_stop(c);
    if (mkpalette(c->zengine->fractalc->palette, seed = rand(), alg) != 0) {
	uih_newimage(c);
    }
    uih_palettechg(c);
    c->paletteshift = 0;
    c->manualpaletteshift = 0;
    c->paletteseed = seed;
    uih_finishpalette(c);
    c->palettechanged = 1;
    c->palettetype = alg + 1;
    uih_cycling_continue(c);
}

/*Basic inicialization routines */

static void uih_alloctables(uih_context * c)
{
    c->zengine = zoom_filter.getinstance(&zoom_filter);
    if (c->zengine == NULL)
	return;
    c->fcontext =
	make_fractalc(0, c->image->pixelwidth * c->image->width,
		      c->image->pixelheight * c->image->height);
    uih_updatemenus(c, "periodicity") uih_updatemenus(c, "in0")
	uih_updatemenus(c, "int0") uih_updatemenus(c,
						   "out0")
	uih_updatemenus(c, "outt0")
	uih_updatemenus(c, "plane0") uih_updatemenus(c, "guess3") uih = c;
    c->uifilter = uih_filter.getinstance(&uih_filter);
    c->queue = create_queue(c->uifilter);
    insertfilter(c->zengine, c->uifilter);
}

static int uih_initqueue(uih_context * c)
{
    return (initqueue(c->queue));
}

void uih_setmaxstep(uih_context * c, number_t p)
{
    c->maxstep = p;
}

void uih_setspeedup(uih_context * c, number_t p)
{
    c->speedup = p;
}

void uih_setmaxiter(uih_context * c, int maxiter)
{
    if (maxiter < 1)
	maxiter = 1;
    if (maxiter > 2000000)
	maxiter = 2000000;
    if (c->fcontext->maxiter != (unsigned int) maxiter) {
	c->fcontext->maxiter = maxiter;
	c->fcontext->version++;
	uih_newimage(c);
    }
}

void uih_setbailout(uih_context * c, number_t bailout)
{
    if (bailout < 0)
	bailout = 0;
    if (c->fcontext->bailout != (number_t) bailout) {
	c->fcontext->bailout = bailout;
	c->fcontext->version++;
	uih_newimage(c);
    }
}

void uih_setincoloringmode(uih_context * c, int mode)
{
    if (mode < 0)
	mode = rand() % INCOLORING;
    if (mode > INCOLORING)
	mode = INCOLORING;
    if (c->fcontext->incoloringmode != mode) {
	char str[10];
	c->fcontext->incoloringmode = mode;
	c->fcontext->version++;
	uih_newimage(c);
	sprintf(str, "in%i", mode);
	uih_updatemenus(c, str);
    }
}

void uih_setintcolor(uih_context * c, int mode)
{
    if (mode < 0)
	mode = rand() % TCOLOR;
    if (mode > TCOLOR)
	mode = TCOLOR;
    if (c->fcontext->intcolor != mode) {
	char str[10];
	c->fcontext->intcolor = mode;
	if (c->fcontext->incoloringmode == 10) {
	    c->fcontext->version++;
	    uih_newimage(c);
	}
	sprintf(str, "int%i", mode);
	uih_updatemenus(c, str);
    }
}

void uih_setouttcolor(uih_context * c, int mode)
{
    if (mode < 0)
	mode = rand() % TCOLOR;
    if (mode > TCOLOR)
	mode = TCOLOR;
    if (c->fcontext->outtcolor != mode) {
	char str[10];
	c->fcontext->outtcolor = mode;
	if (c->fcontext->coloringmode == 10) {
	    c->fcontext->version++;
	    uih_newimage(c);
	}
	sprintf(str, "outt%i", mode);
	uih_updatemenus(c, str);
    }
}

void uih_setperbutation(uih_context * c, number_t zre, number_t zim)
{
    if (c->fcontext->bre != zre || c->fcontext->bim != zim) {
	c->fcontext->bre = zre;
	c->fcontext->bim = zim;
	if (c->fcontext->mandelbrot) {
	    c->fcontext->version++;
	    uih_newimage(c);
	}
	uih_updatemenus(c, "uiperturbation");
    }
}

void uih_perbutation(uih_context * c, int mousex, int mousey)
{
    number_t r, i;
    uih_getcoord(c, mousex, mousey, &r, &i);
    uih_setperbutation(c, r, i);
}

void uih_setjuliaseed(uih_context * c, number_t zre, number_t zim)
{
    if (c->fcontext->pre != zre || c->fcontext->pim != zim) {
	c->fcontext->pre = zre;
	c->fcontext->pim = zim;
	if (c->juliamode) {
	    uih_animate_image(c);
	} else {
	    if (!c->fcontext->mandelbrot) {
		c->fcontext->version++;
		if (c->playc && c->playc->morphjulia)
		    uih_animate_image(c);
		else
		    uih_newimage(c);
	    }
	}
    }
}

void uih_setfastmode(uih_context * c, int mode)
{
    const char *names[] = {
	"nodynamic",
	"nodynamic",
	"dynamicanimation",
	"dynamicnew",
	"dynamicnew"
    };
    if (mode < 0)
	mode = 0;
    c->fastmode = mode;
    uih_updatemenus(c, names[mode]);
}

void uih_setoutcoloringmode(uih_context * c, int mode)
{
    if (mode < 0)
	mode = rand() % OUTCOLORING;
    if (mode > OUTCOLORING)
	mode = OUTCOLORING - 1;
    if (c->fcontext->coloringmode != mode) {
	char str[10];
	c->fcontext->coloringmode = mode;
	c->fcontext->version++;
	uih_newimage(c);
	sprintf(str, "out%i", mode);
	uih_updatemenus(c, str);
    }
}

void uih_setplane(uih_context * c, int mode)
{
    int i;

    if (mode < 0)
	mode = 0;
    for (i = 0; planename[i] != NULL; i++);
    if (mode >= i)
	mode = i - 1;
    if (mode < 0)
	mode = rand() % i;
    uih_invalidatepos(c);
    if (c->fcontext->plane != mode) {
	char str[10];
	c->fcontext->plane = mode;
	//if ( c->fcontext->plane == P_USER )
	//printf("USER NOT IMPLEMENTED");
	//uih_sffein( c, "z^3-c" )
	c->fcontext->version++;
	uih_newimage(c);
	sprintf(str, "plane%i", mode);
	uih_updatemenus(c, str);
    }
}

void
uih_screentofractalcoord(uih_context * c, int mousex, int mousey,
			 number_t * re, number_t * im)
{
    uih_getcoord(c, mousex, mousey, re, im);
    recalculate(c->fcontext->plane, re, im);
}

void uih_setmandelbrot(uih_context * c, int mode, int mousex, int mousey)
{
    if (mode < 0)
	mode = 0;
    if (mode > 1)
	mode = 1;
    if (c->fcontext->mandelbrot != mode) {
	c->fcontext->mandelbrot = mode;
	if (c->fcontext->mandelbrot == 0 && !c->juliamode) {
	    uih_getcoord(c, mousex, mousey, &c->fcontext->pre,
			 &c->fcontext->pim);
	    recalculate(c->fcontext->plane, &c->fcontext->pre,
			&c->fcontext->pim);
	} else
	    uih_disablejulia(c);
	c->fcontext->version++;
	uih_newimage(c);
	uih_updatemenus(c, "uimandelbrot");
    }
}

void uih_setguessing(uih_context * c, int range)
{
    char str[10];
    c->fcontext->range = range;
    if (range <= 1) {
	uih_updatemenus(c, "noguess");
    } else if (range > 8) {
	uih_updatemenus(c, "guessall");
    } else {
	sprintf(str, "guess%i", range);
	uih_updatemenus(c, str);
    }
}

void uih_setperiodicity(uih_context * c, int periodicity)
{
    c->fcontext->periodicity = periodicity;
    uih_updatemenus(c, "periodicity");
}

void uih_interrupt(uih_context * c)
{
    if (c->incalculation)
	c->interrupt = 1;
}

void uih_stopzooming(uih_context * c)
{
    c->speed = 0;
}

int uih_updateimage(uih_context * c, struct image *image)
{
    /*exit(); */
    c->image = image;
    c->palette = image->palette;
    c->queue->isinitialized = 0;
    c->ddatalost = 1;
    fractalc_resize_to(c->fcontext, c->image->pixelwidth * c->image->width,
		       c->image->pixelheight * c->image->height);
    c->display = 1;
    c->palette->ncells = sizeof(uicolors) / sizeof(rgb_t);
    c->palette->prergb = uicolors;
    if (c->palette->type & BITMAPS)
	c->palette->prergb = uibwcolors;
    c->inanimation = 2;
    uih_newimage(c);
    if (image->palette->type & (FIXEDCOLOR | BITMAPS))
	uih_fixedcolorenable(c);
    else
	uih_fixedcolordisable(c);
    return (uih_initqueue(c));
}

static void
uih_getcscreensizes(struct uih_context *uih, int *x, int *y, int *w,
		    int *h, void *data)
{
    *x = 0;
    *y = 0;
    if (uih->clearscreen)
	*w = uih->image->width, *h = uih->image->height;
    else
	*w = *h = 0;
}

static void uih_drawcscreen(struct uih_context *uih, void *data)
{
    if (uih->clearscreen)
	clear_image(uih->image);
}

#ifdef SFFE_USING
extern cmplx C, Z, pZ;
#endif
struct uih_context *globaluih;

struct uih_context *uih_mkcontext(int flags, struct image *image,
				  int (*passfunc) (struct uih_context *,
						   int, const char *,
						   float),
				  void (*longwait) (struct uih_context *),
				  void (*upd) (struct uih_context *,
					       const char *))
{
    uih_context *uih;
    uih = (uih_context *) calloc(sizeof(*uih), 1);	/*setup parameters */
    uih->updatemenus = upd;
    uih->autopilot = 0;
    uih->flags = flags;
    uih->image = image;
    uih->playstring = NULL;
    uih->palette = image->palette;
    uih->menuroot = "root";
    uih->palette->ncells = sizeof(uicolors) / sizeof(rgb_t);
    uih->palette->prergb = uicolors;
    if (uih->palette->type & BITMAPS) {
	uih->palette->prergb = uibwcolors;
    }
    uih->speed = 0;
    uih->step = 0;
    uih->color = 0;
    uih->speedup = STEP;
    uih->maxstep = MAXSTEP;
    uih->lasttime = -1;
    uih->recalculatemode = UIH_NEW_IMAGE;
    uih->display = 1;
    uih->fastmode = 2;
    uih_updatemenus(uih, "dynamicanimation");
    uih->aliasnum = -1;
    uih->direction = 1;
    uih->cyclingdirection = 1;
    uih->cyclingspeed = ROTATIONSPEED;
    uih->ddatalost = 1;
    uih->xtextpos = 1;
    uih_updatemenus(uih, "xtextleft");
    uih->complettehandler = 0;
    uih->ytextpos = 1;
    uih_updatemenus(uih, "ytextup");
    uih->display = 0;
    uih->errstring = NULL;
    uih->rotatemode = 0;
    uih_updatemenus(uih, "norotate");
    uih->rotationspeed = 10;
    uih->longwait = longwait;
    uih->passfunc = passfunc;
    uih->nletters = 0;
    uih->letterspersec = 15;
    uih->maintimer = tl_create_timer();
    uih->calculatetimer = tl_create_timer();
    uih->doittimer = tl_create_timer();
#ifdef SFFE_USING
    uih->pinit = NULL;
    uih->parser = sffe_alloc();
    /* uih->cparser = sffe_alloc(); */
    sffe_regvar(&uih->parser, &pZ, 'p');
    sffe_regvar(&uih->parser, &Z, 'z');
    sffe_regvar(&uih->parser, &C, 'c');
    /* sffe_regvar( &uih->cparser, &C, */
#endif
    /* 25.I.2009, Bugfix #2507911, malczak
     * initilize globaluih here, not in 'ui.c'
     */
    globaluih = uih;

    tl_update_time();
    tl_reset_timer(uih->maintimer);
    tl_reset_timer(uih->calculatetimer);
    tl_stop_timer(uih->doittimer);
    tl_reset_timer(uih->doittimer);
    uih_alloctables(uih);
    uih_initqueue(uih);		/*FIXME return value should not be ignored */
    if (image->palette->type & (FIXEDCOLOR | BITMAPS))
	uih_fixedcolorenable(uih);
    uih_mkdefaultpalette(uih);
    uih_stoptimers(uih);
    clean_autopilot(uih);
    uih_newimage(uih);
    uih->cscreenwindow =
	uih_registerw(uih, uih_getcscreensizes, uih_drawcscreen, 0,
		      NONTRANSPARENTW);
    uih_initmessages(uih);
    uih_inittext(uih);
    uih_emulatetimers(uih);
    uih_setformula(uih, 0);
    uih_saveundo(uih);
    return (uih);
}

void uih_savepalette(uih_context * c)
{
    if (c->palette2 != NULL)
	destroypalette(c->palette2);
    if (c->zengine->fractalc->palette != NULL)
	c->palette2 = clonepalette(c->zengine->fractalc->palette);
}

void uih_restorepalette(uih_context * uih)
{
    if (uih->palette2 != NULL) {
	if (uih->zengine->fractalc->palette != NULL)
	    restorepalette(uih->zengine->fractalc->palette, uih->palette2);
	destroypalette(uih->palette2);
    }
    uih->palette2 = NULL;
    uih_finishpalette(uih);
}

void uih_loadpalette(uih_context * c, struct palette *palette)
{
    if (c->palette2)
	destroypalette(c->palette2);
    c->palette2 = clonepalette(palette);
    uih_restorepalette(c);
    uih_palettechg(c);
}

struct palette *uih_clonepalette(uih_context * c)
{
    if (c->zengine->fractalc->palette != NULL)
	return clonepalette(c->zengine->fractalc->palette);
    // I hope this is OK:
    return NULL;
}

void uih_setformula(uih_context * c, int num)
{
    set_formula(c->fcontext, num);
    uih_newimage(c);
    uih_updatemenus(c, "uimandelbrot");
    uih_updatemenus(c, "uiperturbation");
    uih_updatemenus(c, c->fcontext->currentformula->shortname);
}

void uih_initstate(struct uih_context *uih)
{
    int i;
    int ver = uih->fcontext->version;
    uih->step = 0;
    uih->speedup = STEP;
    uih->maxstep = MAXSTEP;
    uih_fastrotatedisable(uih);
    uih_disablejulia(uih);
    uih->color = 0;
    uih_cycling_off(uih);
    for (i = 0; i < uih_nfilters; i++)
	uih_disablefilter(uih, i);
    uih_setperbutation(uih, 0, 0);
    set_formula(uih->fcontext, 0);
    uih_setperiodicity(uih, 1);
    uih_setmaxiter(uih, 170);
    uih_setbailout(uih, 4);
    uih_setincoloringmode(uih, 0);
    uih_setoutcoloringmode(uih, 0);
    uih_setcycling(uih, 30);
    uih_display(uih);
    uih_setfastmode(uih, 2);
    uih_setintcolor(uih, 0);
    uih_setouttcolor(uih, 0);
    uih_setplane(uih, 0);
    uih_setguessing(uih, 3);
    uih_angle(uih, 0);
    uih_rotatemode(uih, 0);
    uih_rotationspeed(uih, 10);
    uih->xtextpos = 1;
    uih->ytextpos = 1;
    if (uih->playc) {
	uih->playc->morphtimes[0] = 0;
	uih->playc->morphtimes[1] = 0;
	uih->playc->morphjuliatimes[0] = 0;
	uih->playc->morphjuliatimes[1] = 0;
	uih->playc->morphangletimes[0] = 0;
	uih->playc->morphangletimes[1] = 0;
	uih->playc->morphlinetimes[0] = 0;
	uih->playc->morphlinetimes[1] = 0;
    }
    if (mkdefaultpalette(uih->zengine->fractalc->palette) != 0
	|| uih->recalculatemode || uih->fcontext->version != ver) {
	uih_newimage(uih);
    }
}


void uih_freecontext(uih_context * c)
{
    struct filter *f;
    int i;
#ifdef SFFE_USING
    /* sffe_free(&c->cparser); */
    sffe_free(&c->parser);
    if (c->pinit)
	sffe_free(&c->pinit);
#endif
    if (c->emulator != NULL)
	uih_noconstantframetime(c);
    for (i = 0; i < UNDOLEVEL; i++)
	if (c->undo.undos[i])
	    free(c->undo.undos[i]), c->undo.undos[i] = 0;
    while (c->queue->first) {
	f = c->queue->first;
	removefilter(c->queue->first);
	f->action->destroyinstance(f);
    }
    uih_destroymessages(c);
    uih_destroytext(c);
    uih_removew(c, c->cscreenwindow);
    free(c->queue);
    free_fractalc(c->fcontext);
    free(c);
}

static struct filter *uih_getinstance(const struct filteraction *a)
{
    struct filter *f = createfilter(a);
    f->data = uih;
    f->name = "XaoS's user interface layer";
    return (f);
}

static void uih_destroyinstance(struct filter *f)
{
    struct uih_context *c = (struct uih_context *) f->data;
    if (c->autopilot)
	uih_autopilot_off(c);
    if (c->cycling)
	uih_cycling_off(c);
    tl_free_timer(c->maintimer);
    tl_free_timer(c->calculatetimer);
    tl_free_timer(c->doittimer);
    if (c->save)
	uih_save_disable(c);
    if (c->play)
	uih_replaydisable(c);
    free(f);
}

static int wascycling;
static int uih_require(struct filter *f, struct requirements *r)
{
    struct uih_context *uih;
    uih = (struct uih_context *) f->data;
    f->req = *r;
    uih_clearwindows(uih);
    if (uih->cycling)
	uih_cycling_off(uih), wascycling = 1;
    if (!(r->supportedmask & uih->image->palette->type))
	return 0;
    /*FIXME something should be done here :) */
    return (1);
}

static int uih_initialize(struct filter *f, struct initdata *i)
{
    struct uih_context *uih;
    int returnval;
    uih = (struct uih_context *) f->data;
    f->queue->saveimage = uih->image;
    i->fractalc = uih->fcontext;
    uih_setfont(uih);
    tl_update_time();
    f->image = uih->image;
    f->wait_function = uih_waitfunc;
    uih->times[0][0] = -1;
    i->image = uih->image;
    f->fractalc = i->fractalc;
    f->image->palette->flags |= FINISHLATER;
    i->fractalc->palette = uih->image->palette;
    i->wait_function = uih_waitfunc;
    /*FIXME datalost should be handled in better way */
    if (uih->ddatalost)
	i->flags |= DATALOST;
    uih->tbreak = 2;
    uih_invalidatepos(uih);
    clean_autopilot(uih);
    returnval = f->previous->action->initialize(f->previous, i);
    if (wascycling)
	uih_cycling_on(uih), wascycling = 0;
    return returnval;
}

void uih_inhibittextsw(uih_context * c)
{
    c->inhibittextoutput ^= 1;
    uih_updatemenus(c, "inhibittextoutput");
}

int uih_inhibittextselected(uih_context * c)
{
    if (c == NULL)
	return 0;
    return c->inhibittextoutput;
}
