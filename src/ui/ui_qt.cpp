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
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>
#include <fconfig.h>
#include <assert.h>

#include <QtWidgets>
#include <QMessageBox>
#include <cstring>
#include <iostream>
#include <set>

#include "mainwindow.h"
#include "fractalwidget.h"

#include <filter.h>
#include <ui_helper.h>
#include <ui.h>
#include <param.h>
#include <timers.h>
#include <plane.h>
#include <xthread.h>
#include <xerror.h>
#include <xmenu.h>
#include <grlib.h>

#include "uiint.h"
#include "i18n.h"

#ifdef SFFE_USING
#include "sffe.h"
#endif

#ifdef DEBUG
#ifdef __linux__
#define MEMCHECK
#import <malloc.h>
#endif
#endif
#ifdef MEMCHECK
#define STATUSLINES 13
#else
#define STATUSLINES 11
#endif
static void ui_mouse(bool wait);
#ifndef exit_xaos
#define exit_xaos(i) exit(i)
#endif

xio_pathdata configfile;
static void ui_unregistermenus(void);
static struct image *ui_mkimages(int, int);

int err;
/*UI state */
uih_context *uih;
char statustext[256];
int ui_nogui;
static int statusstart;
static struct uih_window *statuswindow = NULL;
static int ministatusstart;
static struct uih_window *ministatuswindow = NULL;
static int callresize = 0;
static tl_timer *maintimer;
static tl_timer *arrowtimer;
static tl_timer *loopt;

/* Command line variables */
static char *defpipe;
static int printspeed;
static int delaytime = 0;
static int defthreads = 0;
static int maxframerate = 80;
float pixelwidth = 0.0, pixelheight = 0.0;

#ifdef SFFE_USING
char *sffeform = NULL;
char *sffeinit = NULL;
#endif

const struct params global_params[] = {
    {"-delay", P_NUMBER, &delaytime, "Delay screen updates (milliseconds)"},
    {"-speedtest", P_SWITCH, &printspeed,
     "Test speed of calculation loop. Then exit"},
#ifndef nthreads
    {"-threads", P_NUMBER, &defthreads, "Set number of threads (CPUs) to use"},
#else
    {"-threads", P_NUMBER, &defthreads,
     "Multiple CPUs unsupported - please recompile XaoS with threads enabled"},
#endif
#ifdef COMPILE_PIPE
    {"-pipe", P_STRING, &defpipe,
     "Accept commands from pipe (use \"-\" for stdin)"},
#else
    {"-pipe", P_STRING, &defpipe, "Pipe input unavailable (recompile XaoS)"},
#endif
    {"-maxframerate", P_NUMBER, &maxframerate,
     "Maximal framerate (0 for unlimited - default)"},
    {"", P_HELP, NULL,
     "Pixel size options: \n\n  Knowledge of exact pixel size makes random dot stereogram look better. \n  Also is used for choosing correct view area"},
    {"-pixelwidth", P_FLOAT, &pixelwidth,
     "exact size of one pixel in centimeters"},
    {"-pixelheight", P_FLOAT, &pixelheight,
     "exact size of one pixel in centimeters"},
#ifdef SFFE_USING
    {"-formula", P_STRING, &sffeform, "user formula"},
    {"-forminit", P_STRING, &sffeinit, "z0 for user formula"},
#endif
    {NULL, 0, NULL, NULL}};

static struct params params[] = {{NULL, 0, NULL, NULL}};

MainWindow *window;
FractalWidget *widget;

int main(int argc, char *argv[])
{
#ifdef _WIN32
    // On Windows, attach to parent console to allow command-line output
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    QCoreApplication::setApplicationName("XaoS");
    QCoreApplication::setApplicationVersion(XaoS_VERSION);
    QCoreApplication::setOrganizationName("XaoS Project");
    QCoreApplication::setOrganizationDomain("xaos.sourceforge.net");

    QApplication app(argc, argv);

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);
    QTranslator xaosTranslator;
    xaosTranslator.load("XaoS_" + QLocale::system().name(), ":/i18n");
    app.installTranslator(&xaosTranslator);

    QLocale::system().setNumberOptions(QLocale::DefaultNumberOptions);

    /* Without this some locales (e.g. the Hungarian) replaces "." to ","
       in numerical format and this will cause an automatic truncation
       at each parameter at certain places, e.g. drawing a new fractal. */
    setlocale(LC_NUMERIC, "C");

    ui_init(argc, argv);
    ui_mainloop(1);
}

extern "C" {

const char *qt_gettext(const char *text)
{
    static std::map<const char *, const char *> strings;
    const char *trans = strings[text];
    if (trans == NULL) {
        trans =
            strdup(QCoreApplication::translate("", text).toStdString().c_str());
        strings[text] = trans;
    }
    return trans;
}
}

static void ui_updatemenus(uih_context *c, const char *name)
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
    if (name == NULL) {
        window->buildMenu(c, uih->menuroot);
        return;
    }
    item = menu_findcommand(name);
    if (item == NULL) {
        /*fprintf (stderr, "Internall error:unknown command %s\n", name); */
        return;
    }
    if (item->flags & (MENUFLAG_CHECKBOX | MENUFLAG_RADIO)) {
        window->toggleMenu(uih, name);
    }
}

static void ui_display(void)
{
    if (nthreads == 1)
        uih_drawwindows(uih);
    widget->repaint();
    uih_cycling_continue(uih);
}

extern int dynsize;
static void ui_outofmem(void) { x_error(gettext("XaoS is out of memory.")); }

#define CHECKPROCESSEVENTS(b, k)                                               \
    assert(!((k) & ~15) && !((b) & ~(BUTTON1 | BUTTON2 | BUTTON3)))
static int ui_passfunc(struct uih_context *c, int display, const char *text,
                       float percent)
{
    char str[80];
    int x = 0, y = 0, b = 0, k = 0;
    ui_mouse(false);
    if (!uih->play) {
        if (uih->display)
            ui_display(), display = 1;
        if (!c->interruptiblemode && !uih->play) {
            if (display) {
                if (percent)
                    sprintf(str, "%s %3.2f%%        ", text, (double)percent);
                else
                    sprintf(str, "%s          ", text);
                window->showStatus(str);
            }
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
    widget->repaint();
    uih_cycling_continue(uih);
    speed = uih_displayed(uih);
    sprintf(
        statustext,
        gettext(
            "%s %.2f times (%.1fE) %2.2f frames/sec %c %i %i %i %u            "),
        times < 1 ? gettext("unzoomed") : gettext("zoomed"),
        times < 1 ? 1.0 / times : times, timesnop, speed,
        uih->autopilot ? 'A' : ' ', uih->fcontext->coloringmode + 1,
        uih->fcontext->incoloringmode + 1, uih->fcontext->plane + 1,
        uih->fcontext->maxiter);

    STAT(printf(gettext("framerate:%f\n"), speed));
    window->showStatus("");
}

void ui_updatestarts(void)
{
    int y = 0;
    ministatusstart = y;
    if (ministatuswindow != NULL)
        y += xtextheight(uih->image, uih->font);
    statusstart = y;
    if (statuswindow != NULL)
        y += xtextheight(uih->image, uih->font) * STATUSLINES;
    uih->messg.messagestart = y;
}

void ui_menuactivate(const menuitem *item, dialogparam *d)
{
    if (item == NULL)
        return;
    if (item->type == MENU_SUBMENU) {
        window->popupMenu(uih, item->shortname);
        return;
    } else {
        if (menu_havedialog(item, uih) && d == NULL) {
            window->showDialog(uih, item->shortname);
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

static void ui_statuspos(uih_context *uih, int *x, int *y, int *w, int *h,
                         void *data)
{
    *x = 0;
    *y = statusstart;
    *w = uih->image->width;
    *h = xtextheight(uih->image, uih->font) * STATUSLINES;
}

static void ui_drawstatus(uih_context *uih, void *data)
{
    char str[6000];
    int h = xtextheight(uih->image, uih->font);
    sprintf(str, gettext("Fractal name:%s"),
            uih->fcontext->currentformula->name[!uih->fcontext->mandelbrot]);
    xprint(uih->image, uih->font, 0, statusstart, str, FGCOLOR(uih),
           BGCOLOR(uih), 0);
    sprintf(str, gettext("Fractal type:%s"),
            uih->fcontext->mandelbrot ? gettext("Mandelbrot")
                                      : gettext("Julia"));
#ifdef SFFE_USING
    if (uih->fcontext->currentformula->flags & SFFE_FRACTAL) {
        sprintf(str, gettext("Formula:%s"), uih->parser->expression);
    };
#endif
    xprint(uih->image, uih->font, 0, statusstart + h, str, FGCOLOR(uih),
           BGCOLOR(uih), 0);
    sprintf(str, gettext("View:[%1.12f,%1.12f]"), (double)uih->fcontext->s.cr,
            (double)uih->fcontext->s.ci);
    xprint(uih->image, uih->font, 0, statusstart + 2 * h, str, FGCOLOR(uih),
           BGCOLOR(uih), 0);
    sprintf(str, gettext("size:[%1.12f,%1.12f]"), (double)uih->fcontext->s.rr,
            (double)uih->fcontext->s.ri);
    xprint(uih->image, uih->font, 0, statusstart + 3 * h, str, FGCOLOR(uih),
           BGCOLOR(uih), 0);
    sprintf(str, gettext("Rotation:%4.2f   Screen size:%i:%i"),
            (double)uih->fcontext->angle, uih->image->width,
            uih->image->height);
    xprint(uih->image, uih->font, 0, statusstart + 4 * h, str, FGCOLOR(uih),
           BGCOLOR(uih), 0);
    sprintf(str, gettext("Iterations:%-4u Palette size:%i"),
            uih->fcontext->maxiter, uih->image->palette->size);
    xprint(uih->image, uih->font, 0, statusstart + 5 * h, str, FGCOLOR(uih),
           BGCOLOR(uih), 0);
    sprintf(str, "Bailout:%4.2f", (double)uih->fcontext->bailout);
    xprint(uih->image, uih->font, 0, statusstart + 6 * h, str, FGCOLOR(uih),
           BGCOLOR(uih), 0);
    sprintf(str, gettext("Autopilot:%-4s  Plane:%s"),
            uih->autopilot ? gettext("On") : gettext("Off"),
            planename[uih->fcontext->plane]);
    xprint(uih->image, uih->font, 0, statusstart + 7 * h, str, FGCOLOR(uih),
           BGCOLOR(uih), 0);
    sprintf(str, gettext("incoloring:%s    outcoloring:%s"),
            incolorname[uih->fcontext->incoloringmode],
            outcolorname[uih->fcontext->coloringmode]);
    xprint(uih->image, uih->font, 0, statusstart + 8 * h, str, FGCOLOR(uih),
           BGCOLOR(uih), 0);
    sprintf(str, gettext("zoomspeed:%f"), (float)uih->maxstep * 1000);
    xprint(uih->image, uih->font, 0, statusstart + 9 * h, str, FGCOLOR(uih),
           BGCOLOR(uih), 0);
    if (uih->fcontext->mandelbrot)
        strcpy(str, gettext("Parameter:none"));
    else
        sprintf(str, gettext("Parameter:[%f,%f]"), (float)uih->fcontext->pre,
                (float)uih->fcontext->pim);
    xprint(uih->image, uih->font, 0, statusstart + 10 * h, str, FGCOLOR(uih),
           BGCOLOR(uih), 0);
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
}

static void ui_status(uih_context *uih)
{
    if (statuswindow == NULL) {
        statuswindow = uih_registerw(uih, ui_statuspos, ui_drawstatus, NULL, 0);
    } else {
        uih_removew(uih, statuswindow);
        statuswindow = NULL;
    }
    ui_updatemenus(uih, "status");
    ui_updatemenus(uih, "animministatus");
    ui_updatestarts();
}

static int ui_statusenabled(uih_context *uih) { return (statuswindow != NULL); }

static void ui_ministatuspos(uih_context *uih, int *x, int *y, int *w, int *h,
                             void *data)
{
    *x = 0;
    *y = ministatusstart;
    *w = uih->image->width;
    *h = xtextheight(uih->image, uih->font);
}

static void ui_drawministatus(uih_context *uih, void *data)
{
    xprint(uih->image, uih->font, 0, ministatusstart, statustext, FGCOLOR(uih),
           BGCOLOR(uih), 0);
}

static void ui_noguisw(uih_context *uih)
{
    ui_nogui ^= 1;
    ui_updatemenus(uih, "nogui");
}

static int ui_noguienabled(uih_context *uih) { return (ui_nogui); }

static void ui_ministatus(uih_context *uih)
{
    if (ministatuswindow == NULL) {
        ministatuswindow =
            uih_registerw(uih, ui_ministatuspos, ui_drawministatus, NULL, 0);
    } else {
        uih_removew(uih, ministatuswindow);
        ministatuswindow = NULL;
    }
    ui_updatestarts();
    ui_updatemenus(uih, "ministatus");
    ui_updatemenus(uih, "animministatus");
}

static int ui_ministatusenabled(uih_context *uih)
{
    return (ministatuswindow != NULL);
}

static void ui_message(struct uih_context *u)
{
    char s[100];
    if (uih->play)
        return;
    widget->setCursor(Qt::WaitCursor);
    sprintf(s, gettext("Please wait while calculating %s"),
            uih->fcontext->currentformula->name[!uih->fcontext->mandelbrot]);
    window->showStatus(s);
}

#define ROTATESPEEDUP 30
static int procescounter(int *counter, const char *text, int speed, int keys,
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
    }
    return changed;
}

static void ui_mouse(bool wait)
{
    int flags;
    char str[80];
    static int spid;
    QCoreApplication::processEvents(wait ? QEventLoop::WaitForMoreEvents
                                         : QEventLoop::AllEvents);
    static int dirty = 0;
    static int lastiter;
    static int maxiter;
    static int lastbuttons, lastx, lasty;

    int mousex = widget->mousePosition().x();
    int mousey = widget->mousePosition().y();
    int mousebuttons = window->mouseButtons();
    int iterchange = window->keyCombination();
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
    uih_update(uih, mousex, mousey, mousebuttons);
    if (uih->play) {
        procescounter(&uih->letterspersec, gettext("Letters per second %i  "),
                      2, iterchange, lastiter, 1, 2, 0, 1, INT_MAX);
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
                        gettext("Rotation speed:%2.2f degrees per second "),
                        (float)uih->rotationspeed);
                rpid = uih_message(uih, str);
            }
            if (iterchange == 1) {
                uih->rotationspeed -=
                    ROTATESPEEDUP * tl_lookup_timer(maintimer) / 1000000.0;
                uih_rmmessage(uih, rpid);
                sprintf(str,
                        gettext("Rotation speed:%2.2f degrees per second "),
                        (float)uih->rotationspeed);
                rpid = uih_message(uih, str);
            }
            tl_reset_timer(maintimer);
        } else {
            if (!dirty)
                maxiter = uih->fcontext->maxiter;
            if (procescounter(&maxiter, gettext("Iterations: %i   "), 1,
                              iterchange, lastiter, 1, 2, 1, 1, INT_MAX) ||
                (iterchange & 3)) {
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
        if (procescounter(&uih->cyclingspeed, gettext("Cycling speed: %i   "),
                          1, iterchange, lastiter, 1, 2, 0, -1000000,
                          INT_MAX)) {
            uih_setcycling(uih, uih->cyclingspeed);
        }
    }
    if (iterchange & 4 &&
        (tl_lookup_timer(maintimer) > FRAMETIME || mousebuttons)) {
        double mul1 = tl_lookup_timer(maintimer) / FRAMETIME;
        double su = 1 + (SPEEDUP - 1) * mul1;
        if (su > 2 * SPEEDUP)
            su = SPEEDUP;
        tl_reset_timer(maintimer);
        uih->speedup *= su, uih->maxstep *= su;
        sprintf(str, gettext("speed:%2.2f "),
                (double)uih->speedup * (1.0 / STEP));
        uih_rmmessage(uih, spid);
        spid = uih_message(uih, str);
    }
    if (iterchange & 8 &&
        (tl_lookup_timer(maintimer) > FRAMETIME || mousebuttons)) {
        double mul1 = tl_lookup_timer(maintimer) / FRAMETIME;
        double su = 1 + (SPEEDUP - 1) * mul1;
        if (su > 2 * SPEEDUP)
            su = SPEEDUP;
        tl_reset_timer(maintimer);
        uih->speedup /= su, uih->maxstep /= su;
        sprintf(str, gettext("speed:%2.2f "),
                (double)uih->speedup * (1 / STEP));
        uih_rmmessage(uih, spid);
        spid = uih_message(uih, str);
    }
    lastiter = iterchange;
    return;
}

void ui_call_resize(void)
{
    callresize = 1;
    uih_interrupt(uih);
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

static void ui_doquit(int i);
static void ui_doquit(int i)
{
    uih_cycling_off(uih);
    uih_freecatalog(uih);
    uih_freecontext(uih);
    tl_free_timer(maintimer);
    tl_free_timer(arrowtimer);
    tl_free_timer(loopt);
    delete window;
    destroypalette(uih->image->palette);
    destroy_image(uih->image);
    xth_uninit();
    xio_uninit();
    ui_unregistermenus();
    uih_unregistermenus();
    exit_xaos(i);
}

void ui_quit(void)
{
    printf(gettext("Thank you for using XaoS\n"));
    ui_doquit(0);
}

static void ui_quitwr(uih_context *c, int quit)
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
    switch (sym = tolower(key)) {
        case ' ':
            uih->display = 1;
            if (uih->play) {
                if (uih->incalculation)
                    ui_updatestatus();
                else {
                    uih_skipframe(uih);
                    window->showStatus(gettext("Skipping, please wait..."));
                }
            }
            break;
        default: {
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
                    mousex = widget->mousePosition().x();
                    mousey = widget->mousePosition().y();
                    buttons = window->mouseButtons();
                    if (d[0].question != NULL && d[1].question == NULL &&
                        d[0].type == DIALOG_COORD) {
                        p = (dialogparam *)malloc(sizeof(dialogparam));
                        uih_screentofractalcoord(uih, mousex, mousey, p->dcoord,
                                                 p->dcoord + 1);
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
    QDesktopServices::openUrl(QUrl(HELP_URL));
}

static void ui_about(struct uih_context *c)
{
    QMessageBox::about(
        NULL, qt_gettext("About"),
        "<a href=\"http://xaos.sf.net\">" +
            QCoreApplication::applicationName() + "</a> " +
            QCoreApplication::applicationVersion() + " (" +
            QSysInfo::kernelType() + " " +
            // QSysInfo::kernelVersion() + " "
            // QSysInfo::buildAbi() + " " +
            QSysInfo::buildCpuArchitecture() +
            ")"
            "<br>"
            "Fast interactive real-time fractal zoomer/morpher<br><br>"
            "Original Authors: Jan Hubička and Thomas Marsh<br>"
            "Copyright © 1996-2019 XaoS Contributors<br>"
            "<br>"
            "This program is free software; you can redistribute it and/or modify "
            "it under the terms of the GNU General Public License as published by "
            "the Free Software Foundation; either version 2 of the License, or "
            "(at your option) any later version.<br><br>"

            "This program is distributed in the hope that it will be useful, "
            "but WITHOUT ANY WARRANTY; without even the implied warranty of "
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
            "GNU General Public License for more details.<br><br>"

            "You should have received a copy of the GNU General Public License "
            "along with this program; if not, write to the Free Software "
            "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA."

    );
}

char *ui_getpos(void) { return (uih_savepostostr(uih)); }

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
 * ui_no_menuitems_i18n is the counter
 * that counts the number of items. The local variables
 * count the local items.
 */

#define MAX_MENUITEMS_I18N 20
/* These variables must be global: */
static menuitem menuitems_i18n[MAX_MENUITEMS_I18N];
int ui_no_menuitems_i18n = 0;

#define UI (MENUFLAG_NOPLAY | MENUFLAG_NOOPTION)

static void ui_registermenus_i18n(void)
{
    int no_menuitems_i18n =
        ui_no_menuitems_i18n; /* This variable must be local. */
    MENUINT_I("file", NULL, gettext("Quit"), "quit",
              MENUFLAG_INTERRUPT | MENUFLAG_ATSTARTUP, ui_quitwr, 1);
    MENUNOP_I("helpmenu", "h", gettext("Help"), "help", MENUFLAG_INCALC,
              ui_helpwr);
    MENUNOP_I("helpmenu", NULL, gettext("About"), "about", NULL, ui_about);
    MENUNOPCB_I("ui", NULL, gettext("Disable XaoS's builtin GUI"), "nogui",
                MENUFLAG_INCALC | MENUFLAG_ATSTARTUP | MENUFLAG_NOMENU,
                ui_noguisw, ui_noguienabled);
    MENUSEPARATOR_I("ui");
    MENUNOPCB_I("ui", "/", gettext("Status"), "status", MENUFLAG_INCALC,
                ui_status, ui_statusenabled); /*FIXME: add also ? as key */

    MENUNOPCB_I("ui", "l", gettext("Ministatus"), "ministatus", MENUFLAG_INCALC,
                ui_ministatus, ui_ministatusenabled);
    MENUSEPARATOR_I("ui");
    MENUSEPARATOR_I("uia");
    MENUNOPCB_I("uia", "/", gettext("Status"), "animstatus",
                UI | MENUFLAG_INCALC, ui_status,
                ui_statusenabled); /*FIXME: add also ? as key */

    MENUNOPCB_I("uia", "l", gettext("Ministatus"), "animministatus",
                UI | MENUFLAG_INCALC, ui_ministatus, ui_ministatusenabled);
    MENUSEPARATOR_I("uia");
    no_menuitems_i18n -= ui_no_menuitems_i18n;
    menu_add(&(menuitems_i18n[ui_no_menuitems_i18n]), no_menuitems_i18n);
    ui_no_menuitems_i18n += no_menuitems_i18n;
}

static void ui_unregistermenus(void)
{
    menu_delete(menuitems, NITEMS(menuitems));
    menu_delete(menuitems_i18n, ui_no_menuitems_i18n);
}

int number_six = 6;

#ifdef SFFE_USING
/* parser variables vars */
cmplx Z, C, pZ;
#endif

#define MAX_WELCOME 80

void ui_printspeed()
{
    int c = 0;
    int x, y, b, k;
    int linesize = uih->image->bytesperpixel * uih->image->height;
    int size = linesize * uih->image->height;
    window->showStatus("Preparing for speedtest");
    uih->passfunc = NULL;
    tl_sleep(1000000);
    for (c = 0; c < 5; c++)
        widget->repaint();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    window->showStatus("Measuring dislay speed");
    tl_sleep(1000000);
    tl_update_time();
    tl_reset_timer(maintimer);
    c = 0;
    while (tl_lookup_timer(maintimer) < 5000000) {
        widget->repaint();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        tl_update_time();
        c++;
    }
    x_message("Driver speed: %g FPS (%.4f MBPS)", c / 5.0,
              c * (double)size / 5.0 / 1024 / 1024);

    window->showStatus("Measuring memcpy speed");
    for (c = 0; c < 5; c++) {
        for (x = 0; x < uih->image->height; x++)
            memcpy(uih->image->currlines[y], uih->image->oldlines[y], linesize);
    }
    tl_update_time();
    tl_reset_timer(maintimer);
    c = 0;
    while (tl_lookup_timer(maintimer) < 5000000) {
        for (x = 0; x < uih->image->height; x++)
            memcpy(uih->image->currlines[y], uih->image->oldlines[y], linesize);
        tl_update_time(), c++;
    }
    x_message("Memcpy speed: %g FPS (%.4f MBPS)", c / 5.0,
              c * (double)size / 5.0 / 1024 / 1024);

    window->showStatus("Measuring missaligned memcpy speed");
    tl_update_time();
    tl_reset_timer(maintimer);
    c = 0;
    while (tl_lookup_timer(maintimer) < 5000000) {
        for (x = 0; x < uih->image->height; x++)
            memcpy(uih->image->currlines[y] + 1, uih->image->oldlines[y] + 2,
                   linesize - 2);
        tl_update_time(), c++;
    }
    x_message("Missaligned memcpy speed: %g FPS (%.4f MBPS)", c / 5.0,
              c * (double)size / 5.0 / 1024 / 1024);

    window->showStatus("Measuring size6 memcpy speed");
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
              c * (double)size / 5.0 / 1024 / 1024);

    widget->repaint();
    window->showStatus("Measuring calculation speed");
    speed_test(uih->fcontext, uih->image);
    window->showStatus("Measuring new image calculation loop");
    uih_prepare_image(uih);
    tl_update_time();
    tl_reset_timer(maintimer);
    for (c = 0; c < 5; c++)
        uih_newimage(uih), uih->fcontext->version++, uih_prepare_image(uih);
    widget->repaint();
    x_message("New image caluclation took %g seconds (%.2g fps)",
              tl_lookup_timer(maintimer) / 5.0 / 1000000.0,
              5000000.0 / tl_lookup_timer(maintimer));
    tl_update_time();
    for (c = 0; c < 5; c++)
        uih_animate_image(uih), uih_prepare_image(uih), c++;
    c = 0;
    tl_update_time();
    tl_reset_timer(maintimer);
    window->showStatus("Measuring zooming algorithm loop");
    while (tl_lookup_timer(maintimer) < 5000000)
        uih_animate_image(uih), uih_prepare_image(uih), tl_update_time(), c++;
    x_message("Approximation loop speed: %g FPS", c / 5.0);
    ui_doquit(0);
}

void ui_init(int argc, char **argv)
{
    int width, height;
    char welcome[MAX_WELCOME];

    xio_init(argv[0]);
    params_register(global_params);
    params_register(ui_fractal_params);
    uih_registermenudialogs_i18n(); /* Internationalized dialogs. */
    /* Dialogs must be generated before menus because menu items
       link to dialog pointers. */
    uih_registermenus_i18n(); /* Internationalized menus. */
    uih_registermenus();
    ui_registermenus_i18n(); /* Internationalized menus. */
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
    window = new MainWindow();
    widget = window->fractalWidget();
    window->show();

    QScreen *screen = window->windowHandle()->screen();
    if (!pixelwidth)
        pixelwidth = 2.54 / screen->physicalDotsPerInchX();
    if (!pixelheight)
        pixelheight = 2.54 / screen->physicalDotsPerInchY();
    signal(SIGFPE, SIG_IGN);
    xth_init(defthreads);
    {
        int i = ui_dorender_params();
        if (i) {
            ui_unregistermenus();
            uih_unregistermenus();
            xio_uninit();
            exit_xaos(i - 1);
        }
    }
#ifdef DEBUG
    printf("Getting size\n");
#endif
    width = widget->size().width();
    height = widget->size().height();
    widget->setCursor(Qt::WaitCursor);
    window->showStatus("Initializing. Please wait");
    window->showStatus("Creating framebuffer");
    struct image *image = ui_mkimages(width, height);

    window->showStatus("Initializing fractal engine");

    /* gloabuih initialization moved into uih_mkcontext function : malczak */
    uih = uih_mkcontext(PIXELSIZE, image, ui_passfunc, ui_message,
                        ui_updatemenus);

    window->buildMenu(uih, uih->menuroot);
#ifdef HOMEDIR
    if (getenv("HOME") != NULL) {
        char home[256], *env = getenv("HOME");
        int maxsize =
            255 - (int)strlen(CONFIGFILE) - 1; /*Avoid buffer owerflow */
        int i;
        for (i = 0; i < maxsize && env[i]; i++)
            home[i] = env[i];
        home[i] = 0;
        xio_addfname(configfile, home, CONFIGFILE);
    } else
#endif
        xio_addfname(configfile, XIO_EMPTYPATH, CONFIGFILE);
    srand(time(NULL));
    uih->fcontext->version++;
    maintimer = tl_create_timer();
    arrowtimer = tl_create_timer();
    loopt = tl_create_timer();
    window->showStatus("Loading message catalog");
    uih_loadcatalog(uih, QLocale::system().name().left(2).toUtf8());
    window->showStatus("Initializing timing system");
    uih_newimage(uih);
    tl_update_time();
    /*tl_process_group (syncgroup, NULL); */
    tl_reset_timer(maintimer);
    tl_reset_timer(arrowtimer);
#ifdef COMPILE_PIPE
    if (defpipe != NULL) {
        window->showStatus("Initializing pipe");
        ui_pipe_init(defpipe);
    }
#else
    if (defpipe != NULL) {
        x_fatalerror("Pipe input not supported!");
    }
#endif
    /*uih_constantframetime(uih,1000000/20); */
    window->showStatus("Reading configuration file");
    {
        xio_file f = xio_ropen(configfile); /*load the configuration file */
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
    window->showStatus("Processing command line parameters");
    {
        const menuitem *item;
        dialogparam *d;
        while ((item = menu_delqueue(&d)) != NULL) {
            uih_saveundo(uih);
            menu_activate(item, uih, d);
        }
    }
    sprintf(welcome, gettext("Welcome to XaoS version %s"), XaoS_VERSION);
    /*TYPE*/ uih_message(uih, welcome);
    if (printspeed) {
        ui_printspeed();
    }
#ifdef SFFE_USING
    /*SFFE : malczak */
    if (uih->parser->expression == NULL) {
        if (sffeform)
            err = sffe_parse(&uih->parser, (char *)sffeform);
        else
            sffe_parse(&uih->parser, "z^2+c");
    }

    if (sffeinit) {
        uih->pinit = sffe_alloc();
        sffe_regvar(&uih->pinit, &pZ, "p");
        sffe_regvar(&uih->pinit, &C, "c");
        if (sffe_parse(&uih->pinit, (char *)sffeinit) > 0)
            sffe_free(&uih->pinit);
    };

    if (err > 0)
        sffe_parse(&uih->parser, "z^2+c");
        /*SFFE*/
#endif
    window->showStatus("Entering main loop");
}

static struct image *ui_mkimages(int width, int height)
{
    struct palette *palette;
    union paletteinfo info;
    info.truec.rmask = 0xff0000;
    info.truec.gmask = 0x00ff00;
    info.truec.bmask = 0x0000ff;
    palette =
        createpalette(0, 0, UI_TRUECOLOR, 0, 0, NULL, NULL, NULL, NULL, &info);
    if (!palette) {
        delete window;
        x_error(gettext("Can not create palette"));
        ui_outofmem();
        exit_xaos(-1);
    }
    struct image *image =
        create_image_qt(width, height, palette, pixelwidth, pixelheight);
    if (!image) {
        delete window;
        x_error(gettext("Can not create image"));
        ui_outofmem();
        exit_xaos(-1);
    }
    widget->setImage(image);
    return image;
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
    uih_clearwindows(uih);
    uih_stoptimers(uih);
    uih_cycling_stop(uih);
    uih_savepalette(uih);
    w = widget->size().width();
    h = widget->size().height();
    assert(w > 0 && w < 65000 && h > 0 && h < 65000);
    if (w != uih->image->width || h != uih->image->height) {
        destroy_image(uih->image);
        destroypalette(uih->palette);
        struct image *image = ui_mkimages(w, h);
        if (!uih_updateimage(uih, image)) {
            delete window;
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
    uih->display = 1;
    ;
    uih_cycling_continue(uih);
}

void ui_mainloop(int loop)
{
    int inmovement = 1;
    int x, y, b, k;
    int time;
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    do {
        widget->setCursor(uih->play ? Qt::CrossCursor
                                    : uih->inhibittextoutput ? Qt::CrossCursor
                                                             : Qt::CrossCursor);
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
        ui_mouse(!inmovement && !uih->inanimation);
        inmovement = 0;
        if (callresize)
            ui_resize(), callresize = 0;
    } while (loop);
}
