/*
 *     XaoS, a fast portable realtime fractal zoomer
 *                  Copyright (C) 1996 by
 *
 *      Jan Hubicka          (hubicka@paru.cas.cz)
 *      Thomas Marsh         (tmarsh@austin.ibm.com)
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
#include <config.h>
#undef _EFENCE_
#ifdef _plan9_
#include <u.h>
#include <libc.h>
#include <ctype.h>
#else
#include <limits.h>
#include <aconfig.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#ifdef __EMX__
#include <float.h>
#endif
#ifdef __EMX__
#include <sys/types.h>
#endif
#ifndef _MAC
#include <sys/stat.h>
#endif
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>
#endif
#include <fconfig.h>
#ifndef _plan9_
#include <assert.h>
#endif
#include <filter.h>
#include <ui_helper.h>
#include <ui.h>
#include <param.h>
#include <version.h>
#include <timers.h>
#include <plane.h>
#include <xthread.h>
#include <xerror.h>
#include <xmenu.h>
#include <grlib.h>
#include <archaccel.h>
#include "uiint.h"
#ifdef HAVE_GETTEXT
#include <libintl.h>
#include <locale.h>
#else
#define gettext(STRING) STRING
#endif

#ifdef SFFE_USING
#include "sffe.h"
#endif

#ifdef DESTICKY
int euid, egid;
#endif
#ifdef DEBUG
#ifdef __linux__
#include <malloc.h>
#define MEMCHECK
#endif
#endif
#define textheight1 (driver->textheight)
#define textwidth1 (driver->textwidth)
#define ui_flush() (driver->flush?driver->flush(),1:0)
#ifdef MEMCHECK
#define STATUSLINES 13
#else
#define STATUSLINES 11
#endif
static void ui_mouse(int mousex, int mousey, int mousebuttons,
                     int iterchange);
#ifndef exit_xaos
#define exit_xaos(i) exit(i)
#endif

xio_pathdata configfile;
static void ui_unregistermenus(void);
static void ui_mkimages(int, int);
static void ui_mainloop(int loop);

int prog_argc;
int err;
char **prog_argv;
/*UI state */
uih_context *uih;
const struct ui_driver *driver;
char statustext[256];
int ui_nogui;
static struct image *image;
static int statusstart;
static struct uih_window *statuswindow = NULL;
static int ministatusstart;
static struct uih_window *ministatuswindow = NULL;
static int mouse;
/* Used by ui_mouse */
static int dirty = 0;
static int lastiter;
static int maxiter;
static int lastbuttons, lastx, lasty;
static int callresize = 0;
static tl_timer *maintimer;
static tl_timer *arrowtimer;
static tl_timer *loopt;
static int todriver = 0;

/* Command line variables */
static char *defpipe;
static char *defdriver = NULL;
static int deflist;
static int printconfig;
static int printspeed;
static int delaytime = 0;
static int defthreads = 0;
static int maxframerate = 80;
static float defscreenwidth = 0.0, defscreenheight = 0.0, defpixelwidth =
    0.0, defpixelheight = 0.0;

#ifdef SFFE_USING
char *sffeform = NULL;
char *sffeinit = NULL;
#endif

const struct params global_params[] = {
    {"-delay", P_NUMBER, &delaytime,
     "Delay screen updates (milliseconds)"},
    {"-driver", P_STRING, &defdriver, "Select driver"},
    {"-list", P_SWITCH, &deflist, "List available drivers. Then exit"},
    {"-config", P_SWITCH, &printconfig, "Print configuration. Then exit"},
    {"-speedtest", P_SWITCH, &printspeed,
     "Test speed of calculation loop. Then exit"},
#ifndef nthreads
    {"-threads", P_NUMBER, &defthreads,
     "Set number of threads (CPUs) to use"},
#else
    {"-threads", P_NUMBER, &defthreads,
     "Multiple CPUs unsupported - please recompile XaoS with threads enabled"},
#endif
#ifdef COMPILE_PIPE
    {"-pipe", P_STRING, &defpipe,
     "Accept commands from pipe (use \"-\" for stdin)"},
#else
    {"-pipe", P_STRING, &defpipe,
     "Pipe input unavailable (recompile XaoS)"},
#endif
    {"-maxframerate", P_NUMBER, &maxframerate,
     "Maximal framerate (0 for unlimited - default)"},
    {"", P_HELP, NULL,
     "Screen size options: \n\n  Knowledge of exact screen size makes random dot stereogram look better. \n  Also is used for choosing correct view area"},
    {"-screenwidth", P_FLOAT, &defscreenwidth,
     "exact size of screen in centimeters"},
    {"-screenheight", P_FLOAT, &defscreenheight,
     "exact size of screen in centimeters"},
    {"", P_HELP, NULL,
     "  Use this option in case you use some kind of virtual screen\n  or something similiar that confuses previous options"},
    {"-pixelwidth", P_FLOAT, &defpixelwidth,
     "exact size of one pixel in centimeters"},
    {"-pixelheight", P_FLOAT, &defpixelheight,
     "exact size of one pixel in centimeters"},
#ifdef SFFE_USING
    {"-formula", P_STRING, &sffeform,
     "user formula"},
    {"-forminit", P_STRING, &sffeinit,
     "z0 for user formula"},
#endif
    {NULL, 0, NULL, NULL}
};

static int resizeregistered = 0;
static void ui_updatemenus(uih_context * c, const char *name)
{
    const struct menuitem *item;
    if (ui_nogui) {
        if (name == NULL) {
            printf("Root \"%s\"", uih->menuroot);
        }
        item = menu_findcommand(name);
        if (item == NULL) {
            /*x_fatalerror ("Internall error:unknown command %s", name); */
            return;
        }
        if (item->flags & MENUFLAG_CHECKBOX) {
            if (menu_enabled(item, c))
                printf("checkbox \"%s\" on\n", name);
            else
                printf("checkbox \"%s\" off\n", name);
        }
        if (item->flags & MENUFLAG_RADIO) {
            if (menu_enabled(item, c))
                printf("radio \"%s\"\n", name);
        }
    }
    if (driver != NULL && driver->gui_driver) {
        if (name == NULL) {
            if (driver->gui_driver->setrootmenu)
                driver->gui_driver->setrootmenu(c, uih->menuroot);
            return;
        }
        item = menu_findcommand(name);
        if (item == NULL) {
            /*fprintf (stderr, "Internall error:unknown command %s\n", name); */
            return;
        }
        if (item->flags & (MENUFLAG_CHECKBOX | MENUFLAG_RADIO)) {
            if (driver->gui_driver->enabledisable)
                driver->gui_driver->enabledisable(uih, name);
        }
    }
}

static void mousetype(int m)
{
#ifdef _plan9_
#define filevisible 0
#endif
    if (ui_nmenus || helpvisible || filevisible || dialogvisible
        || yesnodialogvisible)
        m = NORMALMOUSE;
    if (mouse != m) {
        mouse = m;
        if (driver->mousetype != NULL)
            driver->mousetype(m);
    }
}

static void ui_display(void)
{
    if (nthreads == 1)
        uih_drawwindows(uih);
    driver->display();
    uih_cycling_continue(uih);
    if (!(driver->flags & NOFLUSHDISPLAY))
        ui_flush();
}

float ui_get_windowwidth(int width)
{
    if (defscreenwidth > 0.0 && driver->flags & RESOLUTION)
        return (defscreenwidth * width / driver->maxwidth);
    if (defscreenwidth > 0.0)
        return (defscreenwidth);
    if (defpixelwidth > 0.0)
        return (defpixelwidth * width);
    return (0);
}

static float get_windowwidth(int width)
{
    float w = ui_get_windowwidth(width);
    if (w)
        return w;
    if (driver->flags & PIXELSIZE)
        return (driver->width * width);
    if (driver->flags & SCREENSIZE)
        return (driver->width);
    if (driver->flags & RESOLUTION)
        return (29.0 / driver->maxwidth * width);
    return (29.0);
}

float ui_get_windowheight(int height)
{
    if (defscreenheight > 0.0 && driver->flags & RESOLUTION)
        return (defscreenheight * height / driver->maxheight);
    if (defscreenheight > 0.0)
        return (defscreenheight);
    if (defpixelheight > 0.0)
        return (defpixelheight * height);
    return 0;
}

static float get_windowheight(int height)
{
    float h = ui_get_windowheight(height);
    if (h)
        return h;
    if (driver->flags & PIXELSIZE)
        return (driver->height * height);
    if (driver->flags & SCREENSIZE)
        return (driver->height);
    if (driver->flags & RESOLUTION)
        return (21.0 / driver->maxheight * height);
    return (21.5);
}

extern int dynsize;
static void ui_outofmem(void)
{
    x_error(gettext("XaoS is out of memory."));
}

#define CHECKPROCESSEVENTS(b,k) assert(!((k)&~15)&&!((b)&~(BUTTON1|BUTTON2|BUTTON3)))
static int
ui_passfunc(struct uih_context *c, int display, const char *text,
            float percent)
{
    char str[80];
    int x = 0, y = 0, b = 0, k = 0;
    driver->processevents(0, &x, &y, &b, &k);
    ui_mouse(x, y, b, k);
    CHECKPROCESSEVENTS(b, k);
    if (!uih->play) {
        if (uih->display)
            ui_display(), display = 1;
        if (!c->interruptiblemode && !uih->play) {
            if (display) {
                if (percent)
                    sprintf(str, "%s %3.2f%%        ", text,
                            (double) percent);
                else
                    sprintf(str, "%s          ", text);
                driver->print(0, uih->image->height - textheight1, str);
                ui_flush();
            }
        } else {
            if (!(driver->flags & NOFLUSHDISPLAY))
                ui_flush();
        }
    }
    return (0);
}

static void ui_updatestatus(void)
{
    double times =
        (uih->fcontext->currentformula->v.rr) / (uih->fcontext->s.rr);
    double timesnop = log(times) / log(10.0);
    double speed;
    uih_drawwindows(uih);
    driver->display();
    uih_cycling_continue(uih);
    speed = uih_displayed(uih);
    sprintf(statustext,
            gettext
            ("%s %.2f times (%.1fE) %2.2f frames/sec %c %i %i %i %i            "),
            times < 1 ? gettext("unzoomed") : gettext("zoomed"),
            times < 1 ? 1.0 / times : times, timesnop, speed,
            uih->autopilot ? 'A' : ' ', uih->fcontext->coloringmode + 1,
            uih->fcontext->incoloringmode + 1, uih->fcontext->plane + 1,
            uih->fcontext->maxiter);

    if (!(driver->flags & NOFLUSHDISPLAY))
        ui_flush();
    STAT(printf(gettext("framerate:%f\n"), speed));
    driver->print(0, 0, "");
}

void ui_updatestarts(void)
{
    int y = 0;
    y += ui_menuwidth();
    ministatusstart = y;
    if (ministatuswindow != NULL)
        y += xtextheight(uih->image, uih->font);
    statusstart = y;
    if (statuswindow != NULL)
        y += xtextheight(uih->image, uih->font) * STATUSLINES;
    uih->messg.messagestart = y;
}

void ui_menuactivate(const menuitem * item, dialogparam * d)
{
    if (item == NULL)
        return;
    ui_closemenus();
    if (item->type == MENU_SUBMENU) {
        ui_menu(item->shortname);
        return;
    } else {
        if (menu_havedialog(item, uih) && d == NULL) {
            ui_builddialog(item);
            return;
        }
        if (uih->incalculation && !(item->flags & MENUFLAG_INCALC)) {
            menu_addqueue(item, d);
            if (item->flags & MENUFLAG_INTERRUPT)
                uih_interrupt(uih);
            return;
        }
        if (item->flags & MENUFLAG_CHECKBOX) {
            char s[256];
            ui_updatestatus();
            if (!menu_enabled(item, uih))
                sprintf(s, gettext("Enabling: %s. "), item->name);
            else
                sprintf(s, gettext("Disabling: %s. "), item->name);
            uih_message(uih, s);
            ui_flush();
        } else
            uih_message(uih, item->name);
        uih_saveundo(uih);
        menu_activate(item, uih, d);
        if (d != NULL)
            menu_destroydialog(item, d, uih);
    }
}

xio_path ui_getfile(const char *basename, const char *extension)
{
    return (xio_getfilename(basename, extension));
}

static void
ui_statuspos(uih_context * uih, int *x, int *y, int *w, int *h, void *data)
{
    *x = 0;
    *y = statusstart;
    *w = uih->image->width;
    *h = xtextheight(uih->image, uih->font) * STATUSLINES;
}

static void ui_drawstatus(uih_context * uih, void *data)
{
    char str[6000];
    int h = xtextheight(uih->image, uih->font);
    sprintf(str, gettext("Fractal name:%s"),
            uih->fcontext->currentformula->name[!uih->fcontext->
                                                mandelbrot]);
    xprint(uih->image, uih->font, 0, statusstart, str,
           FGCOLOR(uih), BGCOLOR(uih), 0);
    sprintf(str, gettext("Fractal type:%s"),
            uih->
            fcontext->mandelbrot ? gettext("Mandelbrot") :
            gettext("Julia"));
#ifdef SFFE_USING
    if (uih->fcontext->currentformula->flags & SFFE_FRACTAL) {
        sprintf(str, gettext("Formula:%s"), uih->parser->expression);
    };
#endif
    xprint(uih->image, uih->font, 0, statusstart + h, str,
           FGCOLOR(uih), BGCOLOR(uih), 0);
    sprintf(str, gettext("View:[%1.12f,%1.12f]"),
            (double) uih->fcontext->s.cr, (double) uih->fcontext->s.ci);
    xprint(uih->image, uih->font, 0, statusstart + 2 * h, str,
            FGCOLOR(uih), BGCOLOR(uih), 0);
    sprintf(str, gettext("size:[%1.12f,%1.12f]"),
            (double) uih->fcontext->s.rr, (double) uih->fcontext->s.ri);
    xprint(uih->image, uih->font, 0, statusstart + 3 * h, str,
            FGCOLOR(uih), BGCOLOR(uih), 0);
    sprintf(str, gettext("Rotation:%4.2f   Screen size:%i:%i"),
            (double) uih->fcontext->angle, uih->image->width,
            uih->image->height);
    xprint(uih->image, uih->font, 0, statusstart + 4 * h, str,
            FGCOLOR(uih), BGCOLOR(uih), 0);
    sprintf(str, gettext("Iterations:%-4i Palette size:%i"),
            uih->fcontext->maxiter, uih->image->palette->size);
    xprint(uih->image, uih->font, 0, statusstart + 5 * h, str,
            FGCOLOR(uih), BGCOLOR(uih), 0);
    sprintf(str, "Bailout:%4.2f", (double) uih->fcontext->bailout);
    xprint(uih->image, uih->font, 0, statusstart + 6 * h, str,
            FGCOLOR(uih), BGCOLOR(uih), 0);
    sprintf(str, gettext("Autopilot:%-4s  Plane:%s"),
            uih->autopilot ? gettext("On") : gettext("Off"),
            planename[uih->fcontext->plane]);
    xprint(uih->image, uih->font, 0, statusstart + 7 * h, str,
            FGCOLOR(uih), BGCOLOR(uih), 0);
    sprintf(str, gettext("incoloring:%s    outcoloring:%s"),
            incolorname[uih->fcontext->incoloringmode],
            outcolorname[uih->fcontext->coloringmode]);
    xprint(uih->image, uih->font, 0, statusstart + 8 * h, str,
            FGCOLOR(uih), BGCOLOR(uih), 0);
    sprintf(str, gettext("zoomspeed:%f"), (float) uih->maxstep * 1000);
    xprint(uih->image, uih->font, 0, statusstart + 9 * h, str,
            FGCOLOR(uih), BGCOLOR(uih), 0);
    if (uih->fcontext->mandelbrot)
        strcpy(str, gettext("Parameter:none"));
    else
        sprintf(str, gettext("Parameter:[%f,%f]"),
                (float) uih->fcontext->pre, (float) uih->fcontext->pim);
    xprint(uih->image, uih->font, 0, statusstart + 10 * h, str,
            FGCOLOR(uih), BGCOLOR(uih), 0);
#ifdef MEMCHECK
    {
        struct mallinfo i = mallinfo();
        sprintf(str, "Allocated arena:%i Wasted:%i %i", i.arena, i.ordblks,
                i.fordblks);
        xprint(uih->image, uih->font, 0, statusstart + 11 * h, str,
               FGCOLOR(uih), BGCOLOR(uih), 0);
        sprintf(str, "Mmaped blocks%i Mmaped area:%i keep:%i", i.hblks,
                i.hblkhd, i.keepcost);
        xprint(uih->image, uih->font, 0, statusstart + 12 * h, str,
               FGCOLOR(uih), BGCOLOR(uih), 0);
    }
#endif
    ui_flush();
}

static void ui_status(uih_context * uih)
{
    if (statuswindow == NULL) {
        statuswindow =
            uih_registerw(uih, ui_statuspos, ui_drawstatus, NULL, 0);
    } else {
        uih_removew(uih, statuswindow);
        statuswindow = NULL;
    }
    ui_updatemenus(uih, "status");
    ui_updatemenus(uih, "animministatus");
    ui_updatestarts();
}

static int ui_statusenabled(uih_context * uih)
{
    return (statuswindow != NULL);
}

static void
ui_ministatuspos(uih_context * uih, int *x, int *y, int *w, int *h,
                 void *data)
{
    *x = 0;
    *y = ministatusstart;
    *w = uih->image->width;
    *h = xtextheight(uih->image, uih->font);
}

static void ui_drawministatus(uih_context * uih, void *data)
{
    xprint(uih->image, uih->font, 0, ministatusstart, statustext,
            FGCOLOR(uih), BGCOLOR(uih), 0);
}

static void ui_noguisw(uih_context * uih)
{
    ui_nogui ^= 1;
    ui_updatemenus(uih, "nogui");
}

static int ui_noguienabled(uih_context * uih)
{
    return (ui_nogui);
}

static void ui_ministatus(uih_context * uih)
{
    if (ministatuswindow == NULL) {
        ministatuswindow =
            uih_registerw(uih, ui_ministatuspos, ui_drawministatus, NULL,
                          0);
    } else {
        uih_removew(uih, ministatuswindow);
        ministatuswindow = NULL;
    }
    ui_updatestarts();
    ui_updatemenus(uih, "ministatus");
    ui_updatemenus(uih, "animministatus");
}

static int ui_ministatusenabled(uih_context * uih)
{
    return (ministatuswindow != NULL);
}

static void ui_message(struct uih_context *u)
{
    char s[80];
    if (uih->play)
        return;
    mousetype(WAITMOUSE);
    sprintf(s, gettext("Please wait while calculating %s"),
            uih->fcontext->currentformula->name[!uih->fcontext->
                                                mandelbrot]);
    driver->print(0, 0, s);
}

#define ROTATESPEEDUP 30
static int
procescounter(int *counter, const char *text, int speed, int keys,
              int lastkeys, int down, int up, int tenskip, int min,
              int max)
{
    static int pid = -1;
    int changed = 0;
    char str[80];
    if (tl_lookup_timer(arrowtimer) > 1000000)
        tl_reset_timer(arrowtimer);
    if ((keys & up) && !(lastkeys & up)) {
        (*counter)++;
        tenskip = 0;
        changed = 1;
        tl_reset_timer(arrowtimer);
    }
    if ((keys & down) && !(lastkeys & down)) {
        (*counter)--;
        tenskip = 0;
        changed = 1;
        tl_reset_timer(arrowtimer);
    }
    while (tl_lookup_timer(arrowtimer) > speed * FRAMETIME) {
        tl_slowdown_timer(arrowtimer, speed * FRAMETIME);
        if (keys & up) {
            if (tenskip && !(*counter % 10))
                (*counter) += 10;
            else
                (*counter)++;
            changed = 1;
        }
        if (keys & down) {
            if (tenskip && !(*counter % 10))
                (*counter) -= 10;
            else
                (*counter)--;
            changed = 1;
        }
    }
    if (changed) {
        if (*counter > max)
            *counter = max;
        if (*counter < min)
            *counter = min;
        sprintf(str, text, *counter);
        uih_rmmessage(uih, pid);
        pid = uih_message(uih, str);
        ui_flush();
    }
    return changed;
}

static void
ui_mouse(int mousex, int mousey, int mousebuttons, int iterchange)
{
    int flags;
    char str[80];
    static int spid;
    flags = 0;
    if (mousex != lastx || mousey != lasty)
        flags |= MOUSE_MOVE;
    if ((mousebuttons & BUTTON1) && !(lastbuttons & BUTTON1))
        flags |= MOUSE_PRESS;
    if (!(mousebuttons & BUTTON1) && (lastbuttons & BUTTON1))
        flags |= MOUSE_RELEASE;
    if (mousebuttons & BUTTON1)
        flags |= MOUSE_DRAG;
    lastx = mousex;
    lasty = mousey;
    lastbuttons = mousebuttons;
    tl_update_time();
    CHECKPROCESSEVENTS(mousebuttons, iterchange);
    if (ui_helpmouse(mousex, mousey, mousebuttons, flags)) {
        uih_update(uih, mousex, mousey, 0);
        return;
    }
#ifndef _plan9_
    if (ui_mousefilesel(mousex, mousey, mousebuttons, flags)) {
        uih_update(uih, mousex, mousey, 0);
        return;
    }
#endif
    if (ui_dialogmouse(mousex, mousey, mousebuttons, flags)) {
        uih_update(uih, mousex, mousey, 0);
        return;
    }
    if (ui_menumouse(mousex, mousey, mousebuttons, flags)) {
        uih_update(uih, mousex, mousey, 0);
        return;
    }
    uih_update(uih, mousex, mousey, mousebuttons);
    if (uih->play) {
        procescounter(&uih->letterspersec,
                      gettext("Letters per second %i  "), 2, iterchange,
                      lastiter, 1, 2, 0, 1, INT_MAX);
        return;
    }
    if (!uih->cycling) {
        if (uih->rotatemode == ROTATE_CONTINUOUS) {
            static int rpid;
            if (iterchange == 2) {
                uih->rotationspeed +=
                    ROTATESPEEDUP * tl_lookup_timer(maintimer) / 1000000.0;
                uih_rmmessage(uih, rpid);
                sprintf(str,
                        gettext
                        ("Rotation speed:%2.2f degrees per second "),
                        (float) uih->rotationspeed);
                rpid = uih_message(uih, str);
                ui_flush();
            }
            if (iterchange == 1) {
                uih->rotationspeed -=
                    ROTATESPEEDUP * tl_lookup_timer(maintimer) / 1000000.0;
                uih_rmmessage(uih, rpid);
                sprintf(str,
                        gettext
                        ("Rotation speed:%2.2f degrees per second "),
                        (float) uih->rotationspeed);
                rpid = uih_message(uih, str);
                ui_flush();
            }
            tl_reset_timer(maintimer);
        } else {
            if (!dirty)
                maxiter = uih->fcontext->maxiter;
            if (procescounter
                (&maxiter, gettext("Iterations: %i   "), 1, iterchange,
                 lastiter, 1, 2, 1, 1, INT_MAX) || (iterchange & 3)) {
                dirty = 1;
                lastiter = iterchange;
                return;
            }
        }
    }
    if (dirty) {
        if (uih->incalculation)
            uih_interrupt(uih);
        else
            uih_setmaxiter(uih, maxiter), dirty = 0;
    }
    if (uih->cycling) {
        if (procescounter
            (&uih->cyclingspeed, gettext("Cycling speed: %i   "), 1,
             iterchange, lastiter, 1, 2, 0, -1000000, INT_MAX)) {
            uih_setcycling(uih, uih->cyclingspeed);
        }
    }
    if (iterchange & 4
        && (tl_lookup_timer(maintimer) > FRAMETIME || mousebuttons)) {
        double mul1 = tl_lookup_timer(maintimer) / FRAMETIME;
        double su = 1 + (SPEEDUP - 1) * mul1;
        if (su > 2 * SPEEDUP)
            su = SPEEDUP;
        tl_reset_timer(maintimer);
        uih->speedup *= su, uih->maxstep *= su;
        sprintf(str, gettext("speed:%2.2f "),
                (double) uih->speedup * (1.0 / STEP));
        uih_rmmessage(uih, spid);
        spid = uih_message(uih, str);
        ui_flush();
    }
    if (iterchange & 8
        && (tl_lookup_timer(maintimer) > FRAMETIME || mousebuttons)) {
        double mul1 = tl_lookup_timer(maintimer) / FRAMETIME;
        double su = 1 + (SPEEDUP - 1) * mul1;
        if (su > 2 * SPEEDUP)
            su = SPEEDUP;
        tl_reset_timer(maintimer);
        uih->speedup /= su, uih->maxstep /= su;
        sprintf(str, gettext("speed:%2.2f "),
                (double) uih->speedup * (1 / STEP));
        uih_rmmessage(uih, spid);
        spid = uih_message(uih, str);
        ui_flush();
    }
    lastiter = iterchange;
    return;
}

void ui_call_resize(void)
{
    callresize = 1;
    uih_interrupt(uih);
}

static int
ui_alloccolor(struct palette *pal, int init, int r, int g, int b)
{
    int i;
    i = driver->set_color(r, g, b, init);
    if (i == -1)
        return (-1);
    if (init)
        pal->size = 0;
    pal->pixels[pal->size] = i;
    pal->rgb[i][0] = r;
    pal->rgb[i][1] = g;
    pal->rgb[i][2] = b;
    pal->size++;
    if (driver->flags & UPDATE_AFTER_PALETTE) {
        uih->display = 1;
    }
    return (i);
}

static void
ui_setpalette(struct palette *pal, int start, int end, rgb_t * rgb1)
{
    driver->set_range((ui_palette) rgb1, start, end);
}

static void ui_flip(struct image *image)
{
    flipgeneric(image);
    driver->flip_buffers();
}

static int ui_driverselected(uih_context * c, int d)
{
    return (driver == drivers[d]);
}

static void ui_setdriver(uih_context * c, int d)
{
    todriver = d + 1;
}

static void processbuffer(void)
{
    const menuitem *item;
    dialogparam *d;
    if (uih->incalculation)
        return;
    while ((item = menu_delqueue(&d)) != NULL) {
        ui_menuactivate(item, d);
    }
}

static void ui_doquit(int i) NORETURN;
static void ui_doquit(int i)
{
    uih_cycling_off(uih);
    uih_freecatalog(uih);
    uih_freecontext(uih);
    tl_free_timer(maintimer);
    tl_free_timer(arrowtimer);
    tl_free_timer(loopt);
    driver->free_buffers(NULL, NULL);
    driver->uninit();
    destroypalette(image->palette);
    destroy_image(image);
    xth_uninit();
    xio_uninit();
    ui_unregistermenus();
    uih_unregistermenus();
    exit_xaos(i);
}

void ui_quit(void)
{
#ifndef _MAC
    printf(gettext("Thank you for using XaoS\n"));
#endif
    ui_doquit(0);
}

static void ui_quitwr(uih_context * c, int quit)
{
    if (c == NULL) {
        ui_unregistermenus();
        uih_unregistermenus();
        xio_uninit();
        exit_xaos(0);
    }
    if (quit)
        ui_quit();
}

int ui_key(int key)
{
    int sym;
    char mkey[2];
    const menuitem *item;
#ifdef _plan9_
#define ui_keyfilesel(k) 0
#endif
    if (!ui_helpkeys(key) && !ui_keyfilesel(key) && !ui_dialogkeys(key)
        && !ui_menukey(key))
        switch (sym = tolower(key)) {
        case ' ':
            ui_closemenus();
            uih->display = 1;
            if (uih->play) {
                if (uih->incalculation)
                    ui_updatestatus();
                else {
                    uih_skipframe(uih);
                    driver->print(0, 0,
                                  gettext("Skipping, please wait..."));
                }
            }
            break;
        default:
            {
                int number;
                if (sym >= '0' && sym <= '9') {
                    number = sym - '1';
                    if (number < 0)
                        number = 9;
                    if (number == -1)
                        break;
                }
            }
            mkey[0] = key;
            mkey[1] = 0;
            item = menu_findkey(mkey, uih->menuroot);
            if (item == NULL) {
                mkey[0] = sym;
                item = menu_findkey(mkey, uih->menuroot);
            }
            if (item != NULL) {
                dialogparam *p = NULL;
                if (menu_havedialog(item, uih)) {
                    const menudialog *d = menu_getdialog(uih, item);
                    int mousex, mousey, buttons;
                    driver->getmouse(&mousex, &mousey, &buttons);
                    if (d[0].question != NULL && d[1].question == NULL
                        && d[0].type == DIALOG_COORD) {
                        p = (dialogparam *) malloc(sizeof(dialogparam));
                        uih_screentofractalcoord(uih, mousex, mousey,
                                                 p->dcoord, p->dcoord + 1);
                    }
                }
                ui_menuactivate(item, p);
            }
            break;
        }
    processbuffer();
    return 0;
}

#ifdef _EFENCE_
int EF_ALIGNMENT = 1;
int EF_PROTECT_BELOW = 0;
int EF_PROTECT_FREE = 1;
#endif
static void ui_helpwr(struct uih_context *c)
{
    ui_help("main");
}

char *ui_getpos(void)
{
    return (uih_savepostostr(uih));
}

void ui_loadstr(const char *n)
{
    uih_loadstr(uih, n);
}

static menuitem *menuitems;
/* This structure is now empty. All static definitions have been moved
   to ui_registermenus_i18n() which fills up its own static array. */

/* Registering internationalized menus. See also include/xmenu.h
   for details. Note that MAX_MENUITEMS_I18N may be increased
   if more items will be added in future. */

/* Details of implementation:
 *
 * There are static menuitems_i18n[] arrays for several *.c files.
 * In these files this array is common for all functions.
 * Here, e.g. add_resizeitems() and ui_registermenus_i18n()
 * use the same array and ui_no_menuitems_i18n is the counter
 * that counts the number of items. The local variables
 * count the local items.
 */

#define MAX_MENUITEMS_I18N 20
/* These variables must be global: */
static menuitem menuitems_i18n[MAX_MENUITEMS_I18N];
int ui_no_menuitems_i18n = 0, ui_no_resizeitems;
static menuitem *resizeitems;

#define UI (MENUFLAG_NOPLAY|MENUFLAG_NOOPTION)
static void add_resizeitems()
{
    // General version, it's needed now:
    int no_menuitems_i18n = ui_no_menuitems_i18n;	/* This variable must be local. */
    MENUNOP_I("ui", "=", gettext("Resize"), "resize",
              UI | MENUFLAG_INTERRUPT, ui_call_resize);
    MENUNOP_I("uia", "=", gettext("Resize"), "animresize",
              UI | MENUFLAG_INTERRUPT, ui_call_resize);
    no_menuitems_i18n -= ui_no_menuitems_i18n;
    resizeitems = &menuitems_i18n[ui_no_menuitems_i18n];
    menu_add(resizeitems, no_menuitems_i18n);
    ui_no_resizeitems = no_menuitems_i18n;
    ui_no_menuitems_i18n += no_menuitems_i18n;
}

static void ui_registermenus_i18n(void)
{
    int no_menuitems_i18n = ui_no_menuitems_i18n;	/* This variable must be local. */
#ifndef MACOSX
    SUBMENU_I("file", "q", gettext("Quit"), "quitmenu");
    MENUINT_I("quitmenu", NULL, gettext("Exit now"), "quit",
              MENUFLAG_INTERRUPT | MENUFLAG_ATSTARTUP, ui_quitwr, 1);
    MENUINT_I("quitmenu", NULL, gettext("Not yet"), "noquit", UI,
              ui_quitwr, 0);
#endif
    MENUNOP_I("helpmenu", "h", gettext("Help"), "help", MENUFLAG_INCALC,
              ui_helpwr);
    MENUNOPCB_I("ui", NULL, gettext("Disable XaoS's builtin GUI"), "nogui",
                MENUFLAG_INCALC | MENUFLAG_ATSTARTUP | MENUFLAG_NOMENU,
                ui_noguisw, ui_noguienabled);
    MENUSEPARATOR_I("ui");
    MENUNOPCB_I("ui", "/", gettext("Status"), "status", MENUFLAG_INCALC, ui_status, ui_statusenabled);	/*FIXME: add also ? as key */

    MENUNOPCB_I("ui", "l", gettext("Ministatus"), "ministatus",
                MENUFLAG_INCALC, ui_ministatus, ui_ministatusenabled);
    MENUSEPARATOR_I("ui");
    MENUSEPARATOR_I("uia");
    MENUNOPCB_I("uia", "/", gettext("Status"), "animstatus", UI | MENUFLAG_INCALC, ui_status, ui_statusenabled);	/*FIXME: add also ? as key */

    MENUNOPCB_I("uia", "l", gettext("Ministatus"), "animministatus",
                UI | MENUFLAG_INCALC, ui_ministatus, ui_ministatusenabled);
    MENUSEPARATOR_I("uia");
    SUBMENU_I("ui", NULL, gettext("Driver"), "drivers");
    SUBMENU_I("uia", NULL, gettext("Driver"), "drivers");
    no_menuitems_i18n -= ui_no_menuitems_i18n;
    menu_add(&(menuitems_i18n[ui_no_menuitems_i18n]), no_menuitems_i18n);
    ui_no_menuitems_i18n += no_menuitems_i18n;
}

/* Registering driver items: */
static menuitem *driveritems;
static void ui_registermenus(void)
{
    int i;
    menuitem *item;
    menu_add(menuitems, NITEMS(menuitems));
    driveritems = item = (menuitem *) malloc(sizeof(menuitem) * ndrivers);
    for (i = 0; i < ndrivers; i++) {
        item[i].menuname = "drivers";
        item[i].shortname = drivers[i]->name;
        item[i].key = NULL;
        item[i].type = MENU_INT;
        item[i].flags = MENUFLAG_RADIO | UI;
        item[i].iparam = i;
        item[i].name = drivers[i]->name;
        item[i].function = (void (*)(void)) ui_setdriver;
        item[i].control = (int (*)(void)) ui_driverselected;
    }
    menu_add(item, ndrivers);
}

static void ui_unregistermenus(void)
{
    menu_delete(menuitems, NITEMS(menuitems));
    menu_delete(driveritems, ndrivers);
    menu_delete(menuitems_i18n, ui_no_menuitems_i18n);
    free(driveritems);
}

int number_six = 6;

#ifdef SFFE_USING
        /* parser variables vars */
cmplx Z, C, pZ;
#endif

#define MAX_WELCOME 50

void ui_init(int argc, char **argv)
{
    int i;
    int width, height;
    char welcome[MAX_WELCOME], language[11];
#ifdef HAVE_GETTEXT
    char *locale;
#endif
#ifdef DESTICKY
    euid = geteuid();
    egid = getegid();
#endif
#ifdef DESTICKY
    seteuid(getuid());		/* Don't need supervisor rights anymore. */
    setegid(getgid());
#endif

    strcpy(language, "english");
#ifdef HAVE_GETTEXT
    /* Setting all locales for XaoS: */
    locale = setlocale(LC_ALL, "");
    if (locale == NULL) {
        printf
            ("An error occured in your setlocale/gettext installation.\n");
        printf("I18n menus will not be available.\n");
    }
#ifdef _WIN32
    // x_message("%s",locale);
    if (locale != NULL) {
        if (strncmp(locale, "Hungarian", 9) == 0)
            strcpy(language, "magyar");
        if (strncmp(locale, "Czech", 5) == 0)
            strcpy(language, "cesky");
        if (strncmp(locale, "German", 6) == 0)
            strcpy(language, "deutsch");
        if (strncmp(locale, "Spanish", 7) == 0)
            strcpy(language, "espanhol");
        if (strncmp(locale, "French", 6) == 0)
            strcpy(language, "francais");
        if (strncmp(locale, "Romanian", 8) == 0)
            strcpy(language, "romanian");
        if (strncmp(locale, "Italian", 7) == 0)
            strcpy(language, "italiano");
        if (strncmp(locale, "Portuguese", 10) == 0)
            strcpy(language, "portuguese");
    }
    // x_message("%s",language);
#else
    if ((locale == NULL) || (strcmp(locale, "C") == 0))
        locale = getenv("LANG");
    else
        locale = setlocale(LC_MESSAGES, "");


#ifdef DEBUG
    printf("Trying to use locale settings for %s.\n", locale);
#endif

    if (locale != NULL) {
        if (strlen(locale) > 2)
            locale[2] = '\0';
        if (strcmp(locale, "hu") == 0)
            strcpy(language, "magyar");
        if (strcmp(locale, "cs") == 0)
            strcpy(language, "cesky");
        if (strcmp(locale, "de") == 0)
            strcpy(language, "deutsch");
        if (strcmp(locale, "es") == 0)
            strcpy(language, "espanhol");
        if (strcmp(locale, "fr") == 0)
            strcpy(language, "francais");
        if (strcmp(locale, "ro") == 0)
            strcpy(language, "romanian");
        if (strcmp(locale, "it") == 0)
            strcpy(language, "italiano");
        if (strcmp(locale, "pt") == 0)
            strcpy(language, "portuguese");
    }
#endif
#ifdef DEBUG
    printf("Using catalog file for %s language.\n", language);
#endif
    /* Without this some locales (e.g. the Hungarian) replaces "." to ","
       in numerical format and this will cause an automatic truncation
       at each parameter at certain places, e.g. drawing a new fractal. */
    setlocale(LC_NUMERIC, "C");
#ifdef DEBUG
    printf("Text domain will be bound to directory %s.\n",
#endif
           bindtextdomain("xaos",
#ifdef DOG_DRIVER
                          "..\\locale")
#ifdef DEBUG
        )
#endif
#else
#ifdef _WIN32
                          "..\\locale")
#ifdef DEBUG
        )
#endif
#else
#ifdef USE_LOCALEPATH
                          localepath)
#else
                          "/usr/share/locale")
#endif
#ifdef DEBUG
        )
#endif
#endif
#endif
        ;
#ifndef _WIN32
    bind_textdomain_codeset("xaos", "UTF-8");
#endif
    textdomain("xaos");
    /* Done setting locales. */
#endif
    xio_init(argv[0]);
    params_register(global_params);
    params_register(ui_fractal_params);
    uih_registermenudialogs_i18n();	/* Internationalized dialogs. */
    /* Dialogs must be generated before menus because menu items
       link to dialog pointers. */
    uih_registermenus_i18n();	/* Internationalized menus. */
    uih_registermenus();
    ui_registermenus();
    ui_registermenus_i18n();	/* Internationalized menus. */
    for (i = 0; i < ndrivers; i++)
        params_register(drivers[i]->params);
#ifdef __alpha__
#ifdef __linux__
    extern void ieee_set_fp_control(unsigned long);
    ieee_set_fp_control(1UL);
#endif
#endif
    prog_argc = argc;
    prog_argv = argv;
    if (!params_parser(argc, argv)) {
        ui_unregistermenus();
        uih_unregistermenus();
        xio_uninit();
        exit_xaos(-1);
    }
#ifdef MEM_DEBUG
    D_NORMAL;
#endif
#ifdef DEBUG
    printf("Initializing driver\n");
#endif
#ifndef __BEOS__
#ifndef _plan9_
    signal(SIGFPE, SIG_IGN);
#endif
#endif
    if (printconfig) {
#define tostring(s) #s
        x_message("XaoS configuration\n"
                  "Version:   %s\n"
                  "Type size: %i\n" "integer size: %i\n" "configfile: %s\n"
#ifndef _plan9_
#ifdef HAVE_ALLOCA
                  "using alloca\n"
#endif
#ifdef HAVE_LONG_DOUBLE
                  "using long double\n"
#endif
#ifdef const
                  "const disabled\n"
#endif
#ifdef inline
                  "inline disabled\n"
#endif
#ifdef HAVE_GETTIMEOFDAY
                  "using gettimeofday\n"
#endif
#ifdef HAVE_FTIME
                  "using ftime\n"
#endif
#ifdef MITSHM
                  "using mitshm\n"
#endif
#ifdef HAVE_MOUSEMASK
                  "using ncurses mouse\n"
#endif
#ifdef DEBUG
                  "debug enabled\n"
#endif
#ifdef NDEBUG
                  "assertions disabled\n"
#endif
#ifdef STATISTICS
                  "statistics enabled\n"
#endif
#ifdef SFFE_USING
                  "user formula evaluation\n"
#endif
#endif
                  , XaoS_VERSION, (int) sizeof(FPOINT_TYPE),
                  (int) sizeof(int), CONFIGFILE);
    }
    if (deflist || printconfig) {
        char s[256];
        strcpy(s, "Available drivers: ");
        for (i = 0; i < ndrivers; i++) {
            strcat(s, drivers[i]->name);
            if (i < ndrivers - 1)
                strcat(s, ", ");
        }
        x_message(s);
        ui_unregistermenus();
        uih_unregistermenus();
        xio_uninit();
        exit_xaos(0);
    }
#ifndef _plan9_
    xth_init(defthreads);
#endif
    {
        int i = ui_dorender_params();
        if (i) {
            ui_unregistermenus();
            uih_unregistermenus();
            xio_uninit();
            exit_xaos(i - 1);
        }
    }
    if (defdriver != NULL) {
        for (i = 0; i < ndrivers; i++) {
            int y;
            for (y = 0;
                 tolower(drivers[i]->name[y]) == tolower(defdriver[y])
                 && drivers[i]->name[y] != 0; y++);
            if (drivers[i]->name[y] == 0) {
                driver = drivers[i];
                if (driver->init())
                    break;
                else {
                    x_fatalerror("Can not initialize %s driver",
                                 defdriver);
                }
            }
        }
        if (i == ndrivers) {
            x_fatalerror("Unknown driver %s", defdriver);
        }
    } else {
        for (i = 0; i < ndrivers; i++) {
            driver = drivers[i];
            if (driver->init())
                break;
        }
        if (i == ndrivers) {
            x_fatalerror("Can not initialize driver");
        }
    }
#ifdef DEBUG
    printf("Getting size\n");
#endif
    driver->getsize(&width, &height);
#ifdef _plan9_
    xth_init(defthreads);	/*plan0 requires to initialize tasks after graphics */
#endif
    mousetype(WAITMOUSE);
    driver->print(0, 0, "Initializing. Please wait");
    driver->print(0, textheight1, "Creating framebuffer");
    ui_flush();
    ui_mkimages(width, height);

    driver->print(0, textheight1 * 2, "Initializing fractal engine");
    ui_flush();

    /* gloabuih initialization moved into uih_mkcontext function : malczak */
    uih =
        uih_mkcontext(driver->flags, image, ui_passfunc, ui_message,
                      ui_updatemenus);

    if (driver->gui_driver && driver->gui_driver->setrootmenu)
        driver->gui_driver->setrootmenu(uih, uih->menuroot);
    ui_flush();
#ifdef HOMEDIR
    if (getenv("HOME") != NULL) {
        char home[256], *env = getenv("HOME");
        int maxsize = 255 - (int) strlen(CONFIGFILE) - 1;	/*Avoid buffer owerflow */
        int i;
        for (i = 0; i < maxsize && env[i]; i++)
            home[i] = env[i];
        home[i] = 0;
        xio_addfname(configfile, home, CONFIGFILE);
    } else
#endif
        xio_addfname(configfile, XIO_EMPTYPATH, CONFIGFILE);
    ui_flush();
    srand(time(NULL));
    uih->fcontext->version++;
    maintimer = tl_create_timer();
    arrowtimer = tl_create_timer();
    loopt = tl_create_timer();
    driver->print(0, textheight1 * 3, "Loading message catalog");
    ui_flush();
    uih_loadcatalog(uih, language);
    driver->print(0, textheight1 * 4, "Initializing timming system");
    ui_flush();
    uih_newimage(uih);
    tl_update_time();
    /*tl_process_group (syncgroup, NULL); */
    tl_reset_timer(maintimer);
    tl_reset_timer(arrowtimer);
#ifdef COMPILE_PIPE
    if (defpipe != NULL) {
        driver->print(0, textheight1 * 5, "Initializing pipe");
        ui_flush();
        ui_pipe_init(defpipe);
    }
#else
    if (defpipe != NULL) {
        x_fatalerror("Pipe input not supported!");
    }
#endif
    /*uih_constantframetime(uih,1000000/20); */
    driver->print(0, textheight1 * 6, "Reading configuration file");
    {
        xio_file f = xio_ropen(configfile);	/*load the configuration file */
        if (f != XIO_FAILED) {
            uih_load(uih, f, configfile);
            if (uih->errstring) {
                x_error("Configuration file %s load failed", configfile);
                uih_printmessages(uih);
                x_error("Hint:try to remove it :)");
                ui_doquit(1);
            }
        }
    }
    driver->print(0, textheight1 * 7,
                  "Processing command line parameters");
    ui_flush();
    {
        const menuitem *item;
        dialogparam *d;
        while ((item = menu_delqueue(&d)) != NULL) {
            uih_saveundo(uih);
            menu_activate(item, uih, d);
        }
    }
#ifndef _plan9_
    sprintf(welcome, gettext("Welcome to XaoS version %s"), XaoS_VERSION);
    /*TYPE*/ uih_message(uih, welcome);
#endif
    uih_updatemenus(uih, driver->name);
    if (printspeed) {
        int c = 0;
        int x, y, b, k;
        int linesize = uih->image->bytesperpixel * uih->image->height;
        int size = linesize * uih->image->height;
        driver->print(0, textheight1 * 8, "Preparing for speedtest");
        ui_flush();
        uih->passfunc = NULL;
        tl_sleep(1000000);
        for (c = 0; c < 5; c++)
            driver->display(), ui_flush();
        driver->processevents(0, &x, &y, &b, &k);
        driver->print(0, textheight1 * 9, "Measuring dislay speed");
        ui_flush();
        tl_sleep(1000000);
        tl_update_time();
        tl_reset_timer(maintimer);
        c = 0;
        while (tl_lookup_timer(maintimer) < 5000000)
            driver->display(), ui_flush(), driver->processevents(0, &x, &y,
                                                                 &b, &k),
                tl_update_time(), c++;
        x_message("Driver speed: %g FPS (%.4f MBPS)", c / 5.0,
                  c * (double) size / 5.0 / 1024 / 1024);

        driver->print(0, textheight1 * 10, "Measuring memcpy speed");
        ui_flush();
        for (c = 0; c < 5; c++) {
            for (x = 0; x < uih->image->height; x++)
                memcpy(uih->image->currlines[y], uih->image->oldlines[y],
                       linesize);
        }
        tl_update_time();
        tl_reset_timer(maintimer);
        c = 0;
        while (tl_lookup_timer(maintimer) < 5000000) {
            for (x = 0; x < uih->image->height; x++)
                memcpy(uih->image->currlines[y], uih->image->oldlines[y],
                       linesize);
            tl_update_time(), c++;
        }
        x_message("Memcpy speed: %g FPS (%.4f MBPS)", c / 5.0,
                  c * (double) size / 5.0 / 1024 / 1024);

        driver->print(0, textheight1 * 10,
                      "Measuring missaligned memcpy speed");
        tl_update_time();
        tl_reset_timer(maintimer);
        c = 0;
        while (tl_lookup_timer(maintimer) < 5000000) {
            for (x = 0; x < uih->image->height; x++)
                memcpy(uih->image->currlines[y] + 1,
                       uih->image->oldlines[y] + 2, linesize - 2);
            tl_update_time(), c++;
        }
        x_message("Missaligned memcpy speed: %g FPS (%.4f MBPS)", c / 5.0,
                  c * (double) size / 5.0 / 1024 / 1024);

        driver->print(0, textheight1 * 10, "Measuring size6 memcpy speed");
        tl_update_time();
        tl_reset_timer(maintimer);
        c = 0;
        while (tl_lookup_timer(maintimer) < 5000000) {
            int x, y;
            for (y = 0; y < uih->image->height; y++)
                for (x = 0; x < linesize - 6; x += 6) {
                    memcpy(uih->image->currlines[y] + x,
                           uih->image->oldlines[y] + x, number_six);
                }
            tl_update_time(), c++;
        }
        x_message("Size 6 memcpy speed: %g FPS (%.4f MBPS)", c / 5.0,
                  c * (double) size / 5.0 / 1024 / 1024);

        driver->display();
        driver->print(0, textheight1 * 11, "Measuring calculation speed");
        ui_flush();
        speed_test(uih->fcontext, image);
        driver->print(0, textheight1 * 12,
                      "Measuring new image calculation loop");
        ui_flush();
        uih_prepare_image(uih);
        tl_update_time();
        tl_reset_timer(maintimer);
        for (c = 0; c < 5; c++)
            uih_newimage(uih), uih->fcontext->version++,
                uih_prepare_image(uih);
        driver->display();
        ui_flush();
        x_message("New image caluclation took %g seconds (%.2g fps)",
                  tl_lookup_timer(maintimer) / 5.0 / 1000000.0,
                  5000000.0 / tl_lookup_timer(maintimer));
        tl_update_time();
        for (c = 0; c < 5; c++)
            uih_animate_image(uih), uih_prepare_image(uih), c++;
        c = 0;
        tl_update_time();
        tl_reset_timer(maintimer);
        driver->print(0, textheight1 * 13,
                      "Measuring zooming algorithm loop");
        ui_flush();
        while (tl_lookup_timer(maintimer) < 5000000)
            uih_animate_image(uih), uih_prepare_image(uih),
                tl_update_time(), c++;
        x_message("Approximation loop speed: %g FPS", c / 5.0);
        ui_doquit(0);
    }
#ifdef SFFE_USING
    /*SFFE : malczak */
    if (uih->parser->expression == NULL) {
        if (sffeform)
            err = sffe_parse(&uih->parser, (char *) sffeform);
        else
            sffe_parse(&uih->parser, "z^2+c");
    }

    if (sffeinit) {
        uih->pinit = sffe_alloc();
        sffe_regvar(&uih->pinit, &pZ, 'p');
        sffe_regvar(&uih->pinit, &C, 'c');
        if (sffe_parse(&uih->pinit, (char *) sffeinit) > 0)
            sffe_free(&uih->pinit);
    };

    if (err > 0)
        sffe_parse(&uih->parser, "z^2+c");
     /*SFFE*/
#endif
        driver->print(0, textheight1 * 8, "Entering main loop");
    ui_flush();
}

#ifndef MAIN_FUNCTION
#define MAIN_FUNCTION main
#endif
int MAIN_FUNCTION(int argc, char **argv)
{
    ui_init(argc, argv);
    ui_mainloop(1);
    ui_quit();

    return 0;
}

static void ui_mkimages(int w, int h)
{
    struct palette *palette;
    int scanline;
    int width, height;
    union paletteinfo info;
    char *b1, *b2;
    void *data;
    width = w;
    height = h;
    if (resizeregistered && !(driver->flags & RESIZE_COMMAND)) {
        menu_delete(resizeitems, ui_no_resizeitems);
        resizeregistered = 0;
    } else {
        if (!resizeregistered && (driver->flags & RESIZE_COMMAND)) {
            add_resizeitems();
            resizeregistered = 1;
        }
    }
    if (!(scanline = driver->alloc_buffers(&b1, &b2, &data))) {
        driver->uninit();
        x_error(gettext("Can not allocate buffers"));
        ui_outofmem();
        exit_xaos(-1);
    }
    info.truec.rmask = driver->rmask;
    info.truec.gmask = driver->gmask;
    info.truec.bmask = driver->bmask;
    palette =
        createpalette(driver->palettestart, driver->paletteend,
                      driver->imagetype,
                      (driver->
                       flags & RANDOM_PALETTE_SIZE) ? UNKNOWNENTRIES : 0,
                      driver->maxentries,
                      driver->set_color != NULL ? ui_alloccolor : NULL,
                      driver->set_range != NULL ? ui_setpalette : NULL,
                      NULL, NULL, &info);
    if (!palette) {
        driver->uninit();
        x_error(gettext("Can not create palette"));
        ui_outofmem();
        exit_xaos(-1);
    }
    image =
        create_image_cont(width, height, scanline, 2, (unsigned char *) b1,
                          (unsigned char *) b2, palette, ui_flip,
                          (driver->flags & AALIB) ? AAIMAGE : 0,
                          get_windowwidth(width) / width,
                          get_windowheight(height) / height);
    if (!image) {
        driver->uninit();
        x_error(gettext("Can not create image"));
        ui_outofmem();
        exit_xaos(-1);
    }
    image->data = data;
    image->driver = driver->image_driver;
}

void ui_resize(void)
{
    int w, h;

    /* Prevent crash on startup for Mac OS X */
    if (!uih)
        return;

    if (uih->incalculation) {
        uih_interrupt(uih);
        return;
    }
    ui_closemenus();
    ui_closedialog(0);
    ui_close_help();
    uih_clearwindows(uih);
    uih_stoptimers(uih);
    uih_cycling_stop(uih);
    uih_savepalette(uih);
    driver->getsize(&w, &h);
    assert(w > 0 && w < 65000 && h > 0 && h < 65000);
    if (w != uih->image->width || h != uih->image->height
        || (driver->flags & UPDATE_AFTER_RESIZE)
        || uih->palette->type != driver->imagetype) {
        driver->free_buffers(NULL, NULL);
        destroy_image(uih->image);
        destroypalette(uih->palette);
        ui_mkimages(w, h);
        if (!uih_updateimage(uih, image)) {
            driver->uninit();
            x_error(gettext("Can not allocate tables"));
            ui_outofmem();
            exit_xaos(-1);
        }
        tl_process_group(syncgroup, NULL);
        tl_reset_timer(maintimer);
        tl_reset_timer(arrowtimer);
        uih_newimage(uih);
    }
    uih_newimage(uih);
    uih_restorepalette(uih);
    /*uih_mkdefaultpalette(uih); */
    uih->display = 1;;
    uih_cycling_continue(uih);
}

static void ui_driver(int d)
{
    const struct ui_driver *driver1;
    int width, height;
    ui_closemenus();
    ui_closedialog(0);
    ui_close_help();
    if (d < 0)
        d = 0;
    if (d >= ndrivers)
        d = ndrivers - 1;
    uih_stoptimers(uih);
    driver1 = driver;
    uih_clearwindows(uih);
    uih_cycling_off(uih);
    uih_savepalette(uih);
    driver->free_buffers(NULL, NULL);
    driver->uninit();
    driver = drivers[d];
    if (!driver->init()) {
        driver = driver1;
        uih_error(uih, gettext("Can not initialize driver"));
        if (!driver1->init()) {
            x_fatalerror(gettext
                         ("Can not return back to previous driver"));
        } else
            driver = driver1;
    }
    driver->getsize(&width, &height);
    destroy_image(uih->image);
    destroypalette(uih->palette);
    uih->flags = driver->flags;
    ui_mkimages(width, height);
    if (!uih_updateimage(uih, image)) {
        driver->uninit();
        x_error(gettext("Can not allocate tables"));
        ui_outofmem();
        exit_xaos(-1);
    }
    if (driver->gui_driver && driver->gui_driver->setrootmenu)
        driver->gui_driver->setrootmenu(uih, uih->menuroot);
    tl_process_group(syncgroup, NULL);
    tl_reset_timer(maintimer);
    tl_reset_timer(arrowtimer);
    uih->display = 1;
    uih_newimage(uih);
    uih_restorepalette(uih);
    ui_updatestatus();
    uih_updatemenus(uih, driver->name);
}

static void ui_mainloop(int loop)
{
    int inmovement = 1;
    int x, y, b, k;
    int time;
    driver->processevents((!inmovement && !uih->inanimation), &x, &y, &b,
                          &k);
    do {
        mousetype(uih->play ? REPLAYMOUSE : uih->
                  inhibittextoutput ? VJMOUSE : NORMALMOUSE);
        if (uih->display) {
            uih_prepare_image(uih);
            ui_updatestatus();
        }
        if ((time = tl_process_group(syncgroup, NULL)) != -1) {
            if (!inmovement && !uih->inanimation) {
                if (time > 1000000 / 50)
                    time = 1000000 / 50;
                if (time > delaytime) {
                    tl_sleep(time - delaytime);
                    tl_update_time();
                }
            }
            inmovement = 1;
        }
        if (delaytime || maxframerate) {
            tl_update_time();
            time = tl_lookup_timer(loopt);
            tl_reset_timer(loopt);
            time = 1000000 / maxframerate - time;
            if (time < delaytime)
                time = delaytime;
            if (time) {
                tl_sleep(time);
                tl_update_time();
            }
        }
        processbuffer();
        driver->processevents((!inmovement && !uih->inanimation), &x, &y,
                              &b, &k);

        inmovement = 0;
        ui_mouse(x, y, b, k);
        if (todriver)
            ui_driver(todriver - 1), todriver = 0;
        if (callresize)
            ui_resize(), callresize = 0;
    } while (loop);
}
