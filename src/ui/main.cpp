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
#include <cassert>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <QtWidgets>
#include <QMessageBox>

#include "config.h"
#include "mainwindow.h"
#include "fractalwidget.h"
#include "filter.h"
#include "ui_helper.h"
#include "ui.h"
#include "param.h"
#include "timers.h"
#include "plane.h"
#include "xthread.h"
#include "xerror.h"
#include "xmenu.h"
#include "grlib.h"
#include "i18n.h"

#ifdef SFFE_USING
#include "sffe.h"
#endif

/* Command line variables */
static int printspeed;
static int delaytime = 0;
static int defthreads = 0;
static int maxframerate = 80;
float pixelwidth = 0.0, pixelheight = 0.0;

#ifdef SFFE_USING
static char *sffeform = NULL;
static char *sffeinit = NULL;
#endif

static char *defrender = NULL;
static const char *rbasename = "anim";
static int alias = 0;
static int slowmode = 0;
static char *imgtype;
static char *defsize;
static float framerate;
static int letterspersec = 20;
static int defvectors;
static int iframedist;

const struct params global_params[] = {
    {"-delay", P_NUMBER, &delaytime, "Delay screen updates (milliseconds)"},
    {"-speedtest", P_SWITCH, &printspeed,
     "Test speed of calculation loop. Then exit"},
#ifndef nthreads
    {"-threads", P_NUMBER, &defthreads, "Set number of threads (CPUs) to use"},
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

    {"", P_HELP, NULL, "Animation rendering:"},
    {"-render", P_STRING, &defrender,
     "Render animation into seqence of .png files"},
    {"-basename", P_STRING, &rbasename,
     "Name for .png files (XaoS will add 4 digit number and extension"},
    {"-size", P_STRING, &defsize, "widthxheight"},
    {"-renderimage", P_STRING, &imgtype, "256 or truecolor"},
    {"-renderframerate", P_FLOAT, &framerate, "framerate"},
    {"-antialiasing", P_SWITCH, &alias,
     "Perform antialiasing (slow, requires quite lot of memory)"},
    {"-alwaysrecalc", P_SWITCH, &slowmode,
     "Always recalculate whole image (slowes down rendering, increases quality)"},
    {"-rendervectors", P_SWITCH, &defvectors,
     "Render motion vectors (should be used for MPEG encoding)"},
    {"-iframedist", P_NUMBER, &iframedist,
     "Recommended distance between I frames in pat file (should be used for MPEG encoding)"},

    {NULL, 0, NULL, NULL}};

#define MAXPARAMS 40
static const struct params *params[MAXPARAMS];
int nparams;

int ui_params_parser(int argc, char **argv)
{
    int i, p = 0, d;
    int ie = 0;
    int is;
    const struct params *par = NULL;
    int error = 0;
    int found;
    for (i = 1; i < argc && !error; i++) {
        found = 0;
#ifdef MACOSX
        if (strncmp("-psn", argv[i], 4) == 0)
            continue;
#endif
        if (!strcmp("-help", argv[i])) {
            error = 1;
            break;
        }
        for (d = 0; d < nparams; d++) {
            par = params[d];
            for (p = 0; par[p].name != NULL && !error; p++) {
                if (!strcmp(par[p].name, argv[i])) {
                    found = 1;
                    is = i;
                    switch (par[p].type) {
                        case P_SWITCH:
                            *((int *)par[p].value) = 1;
                            break;
                        case P_NUMBER: {
                            int n;
                            if (i == argc - 1) {
                                x_error("parameter %s requires numeric value.",
                                        argv[i]);
                                error = 1;
                                break;
                            }
                            if (sscanf(argv[i + 1], "%i", &n) != 1) {
                                x_error("parameter for %s is not number.",
                                        argv[i]);
                                error = 1;
                                break;
                            }
                            *((int *)par[p].value) = n;
                            i++;
                        } break;
                        case P_FLOAT: {
                            float n;
                            if (i == argc - 1) {
                                x_error(
                                    "parameter %s requires floating point numeric value.",
                                    argv[i]);
                                error = 1;
                                break;
                            }
                            if (sscanf(argv[i + 1], "%f", &n) != 1) {
                                x_error(
                                    "parameter for %s is not floating point number.",
                                    argv[i]);
                                error = 1;
                                break;
                            }
                            *((float *)par[p].value) = n;
                            i++;
                        } break;
                        case P_STRING: {
                            if (i == argc - 1) {
                                x_error("parameter %s requires string value.",
                                        argv[i]);
                                error = 1;
                                break;
                            }
                            i++;
                            *((char **)par[p].value) = *(argv + i);
                        }
                    }
                    ie = i;
                    i = is;
                }
            }
        }
        if (d == nparams && !found) {
            i = menu_processargs(i, argc, argv);
            if (i < 0) {
                error = 1;
                break;
            } else
                i++;
        } else
            i = ie;
    }
    if (error) {
        const char *name[] = {"", "number", "string", "f.point"};
        printf("                 XaoS" XaoS_VERSION " help text\n\n");
        printf("option string   param   description\n\n");
        for (d = 0; d < nparams; d++) {
            par = params[d];
            for (p = 0; par[p].name != NULL; p++) {
                if (par[p].type == P_HELP)
                    printf("\n%s\n\n", par[p].help);
                else if (!par[p].type)
                    printf(" %-14s   %s\n", par[p].name, par[p].help);
                else
                    printf(" %-14s  %s\n%14s    %s\n", par[p].name,
                           name[par[p].type], "", par[p].help);
            }
            if (p == 0)
                printf(" No options available for now\n");
        }
        menu_printhelp();
        return 0;
    }
    return (1);
}

void params_register(const struct params *par) { params[nparams++] = par; }

int ui_render(void)
{
    if (defrender != NULL) {
        int imagetype = TRUECOLOR;
        int width = 640, height = 480;
#ifndef STRUECOLOR24
        if (imagetype == TRUECOLOR24)
            imagetype = TRUECOLOR;
#endif
        if (imgtype != NULL) {
            if (!strcmp("256", imgtype))
                imagetype = C256;
            else if (!strcmp("truecolor", imgtype)) {
                x_fatalerror("Unknown image type:%s", imgtype);
            }
        }
        if (defsize != NULL && !sscanf(defsize, "%ix%i", &width, &height) &&
            (width <= 0 || height <= 0)) {
            x_fatalerror("Invalid size (use for example 320x200");
        }
        if (framerate <= 0)
            framerate = 30;
        uih_renderanimation(NULL, rbasename, defrender, width, height,
                            pixelwidth, pixelheight, (int)(1000000 / framerate),
                            imagetype, alias, slowmode, letterspersec, NULL);
        return 1;
    }
    return 0;
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

static MainWindow *window;
static FractalWidget *widget;
static uih_context *uih;

static tl_timer *maintimer;
static tl_timer *loopt;
static tl_timer *arrowtimer;

void ui_printspeed()
{
    int c = 0;
    int x, y = 0;
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
                       uih->image->oldlines[y] + x, 6);
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
    ui_quit(0);
}

static void ui_unregistermenus(void);

void ui_quit(int i)
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
    printf(gettext("Thank you for using XaoS\n"));
    exit(i);
}

static void ui_help(struct uih_context *c)
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

#define MAX_MENUITEMS_I18N 20
/* These variables must be global: */
static menuitem *menuitems;
static menuitem menuitems_i18n[MAX_MENUITEMS_I18N];
int ui_no_menuitems_i18n = 0;

#define UI (MENUFLAG_NOPLAY | MENUFLAG_NOOPTION)

static void ui_registermenus_i18n(void)
{
    int no_menuitems_i18n =
        ui_no_menuitems_i18n; /* This variable must be local. */
    MENUINT_I("file", NULL, gettext("Quit"), "quit",
              MENUFLAG_INTERRUPT | MENUFLAG_ATSTARTUP, ui_quit, 0);
    MENUNOP_I("helpmenu", "h", gettext("Help"), "help", MENUFLAG_INCALC,
              ui_help);
    MENUNOP_I("helpmenu", NULL, gettext("About"), "about", NULL, ui_about);
    no_menuitems_i18n -= ui_no_menuitems_i18n;
    menu_add(&(menuitems_i18n[ui_no_menuitems_i18n]), no_menuitems_i18n);
    ui_no_menuitems_i18n += no_menuitems_i18n;
}

static void ui_unregistermenus(void)
{
    menu_delete(menuitems, NITEMS(menuitems));
    menu_delete(menuitems_i18n, ui_no_menuitems_i18n);
}

static void ui_updatemenus(uih_context *c, const char *name)
{
    const struct menuitem *item;
    if (name == NULL) {
        window->buildMenu(c, uih->menuroot);
        return;
    }
    item = menu_findcommand(name);
    if (item == NULL)
        return;
    if (item->flags & (MENUFLAG_CHECKBOX | MENUFLAG_RADIO)) {
        window->toggleMenu(uih, name);
    }
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
            uih_updatestatus(uih);
            widget->repaint();
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

int ui_key(int key)
{
    int sym;
    char mkey[2];
    const menuitem *item;
    switch (sym = tolower(key)) {
        case ' ':
            uih->display = 1;
            if (uih->play) {
                if (uih->incalculation) {
                    uih_updatestatus(uih);
                    widget->repaint();
                } else {
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
                    int mousex, mousey;
                    mousex = widget->mousePosition().x();
                    mousey = widget->mousePosition().y();
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

static int processcounter(int *counter, const char *text, int speed, int keys,
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

#define ROTATESPEEDUP 30
#define CHECKPROCESSEVENTS(b, k)                                               \
    assert(!((k) & ~15) && !((b) & ~(BUTTON1 | BUTTON2 | BUTTON3)))

static void ui_events(bool wait)
{
    char str[80];
    static int spid;
    QCoreApplication::processEvents(wait ? QEventLoop::WaitForMoreEvents
                                         : QEventLoop::AllEvents);
    static int dirty = 0;
    static int lastiter;
    static int maxiter;

    int mousex = widget->mousePosition().x();
    int mousey = widget->mousePosition().y();
    int mousebuttons = window->mouseButtons();
    int iterchange = window->keyCombination();
    tl_update_time();
    CHECKPROCESSEVENTS(mousebuttons, iterchange);
    uih_update(uih, mousex, mousey, mousebuttons);
    if (uih->play) {
        processcounter(&uih->letterspersec, gettext("Letters per second %i  "),
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
            if (processcounter(&maxiter, gettext("Iterations: %i   "), 1,
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
        if (processcounter(&uih->cyclingspeed, gettext("Cycling speed: %i   "),
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

static int ui_passfunc(struct uih_context *c, int display, const char *text,
                       float percent)
{
    char str[80];
    ui_events(false);
    if (!uih->play) {
        if (uih->display) {
            if (nthreads == 1)
                uih_drawwindows(uih);
            widget->repaint();
            uih_cycling_continue(uih);
            display = 1;
        }
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

static void ui_message(struct uih_context *uih)
{
    char s[100];
    if (uih->play)
        return;
    widget->setCursor(Qt::WaitCursor);
    sprintf(s, gettext("Please wait while calculating %s"),
            uih->fcontext->currentformula->name[!uih->fcontext->mandelbrot]);
    window->showStatus(s);
}

static void ui_outofmem(void) { x_error(gettext("XaoS is out of memory.")); }

static struct image *ui_mkimages(int width, int height)
{
    struct palette *palette;
    union paletteinfo info;
    info.truec.rmask = 0xff0000;
    info.truec.gmask = 0x00ff00;
    info.truec.bmask = 0x0000ff;
    palette =
        createpalette(0, 0, TRUECOLOR, 0, 0, NULL, NULL, NULL, NULL, &info);
    if (!palette) {
        delete window;
        x_error(gettext("Can not create palette"));
        ui_outofmem();
        exit(-1);
    }
    struct image *image =
        create_image_qt(width, height, palette, pixelwidth, pixelheight);
    if (!image) {
        delete window;
        x_error(gettext("Can not create image"));
        ui_outofmem();
        exit(-1);
    }
    widget->setImage(image);
    return image;
}

static int callresize = 0;

void ui_call_resize(void)
{
    callresize = 1;
    uih_interrupt(uih);
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
            exit(-1);
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

#ifdef SFFE_USING
cmplx Z, C, pZ;
#endif

xio_pathdata configfile;

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

    xio_init(argv[0]);
    signal(SIGFPE, SIG_IGN);
    srand(time(NULL));

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);
    QTranslator xaosTranslator;
    xaosTranslator.load("XaoS_" + QLocale::system().name(), ":/i18n");
    app.installTranslator(&xaosTranslator);

    /* Without this some locales (e.g. the Hungarian) replaces "." to ","
       in numerical format and this will cause an automatic truncation
       at each parameter at certain places, e.g. drawing a new fractal. */
    QLocale::system().setNumberOptions(QLocale::DefaultNumberOptions);
    setlocale(LC_NUMERIC, "C");

    params_register(global_params);
    uih_registermenudialogs_i18n(); /* Internationalized dialogs. */
    /* Dialogs must be generated before menus because menu items
       link to dialog pointers. */
    uih_registermenus_i18n(); /* Internationalized menus. */
    uih_registermenus();
    ui_registermenus_i18n(); /* Internationalized menus. */
    if (!ui_params_parser(argc, argv)) {
        ui_unregistermenus();
        uih_unregistermenus();
        xio_uninit();
        exit(-1);
    }

    xth_init(defthreads);

    window = new MainWindow();
    widget = window->fractalWidget();
    widget->setCursor(Qt::WaitCursor);
    window->showStatus("Initializing. Please wait.");
    window->show();

    QScreen *screen = window->windowHandle()->screen();
    if (!pixelwidth)
        pixelwidth = 2.54 / screen->physicalDotsPerInchX();
    if (!pixelheight)
        pixelheight = 2.54 / screen->physicalDotsPerInchY();

    int i = ui_render();
    if (i) {
        ui_unregistermenus();
        uih_unregistermenus();
        xio_uninit();
        exit(i - 1);
    }

    int width = widget->size().width();
    int height = widget->size().height();
    struct image *image = ui_mkimages(width, height);
    /* gloabuih initialization moved into uih_mkcontext function : malczak */
    uih = uih_mkcontext(PIXELSIZE, image, ui_passfunc, ui_message,
                        ui_updatemenus);

    window->buildMenu(uih, uih->menuroot);

    uih->fcontext->version++;
    uih_newimage(uih);

    uih_loadcatalog(uih, QLocale::system().name().left(2).toUtf8());

    tl_update_time();
    maintimer = tl_create_timer();
    arrowtimer = tl_create_timer();
    loopt = tl_create_timer();
    tl_reset_timer(maintimer);
    tl_reset_timer(arrowtimer);

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
        xio_addfname(configfile, XIO_EMPTYPATH, CONFIGFILE);
    xio_file f = xio_ropen(configfile); /*load the configuration file */
    if (f != XIO_FAILED) {
        uih_load(uih, f, configfile);
        if (uih->errstring) {
            x_error("Configuration file %s load failed", configfile);
            uih_printmessages(uih);
            x_error("Hint: try to remove it");
            ui_quit(1);
        }
    }

    const menuitem *item;
    dialogparam *d;
    while ((item = menu_delqueue(&d)) != NULL) {
        uih_saveundo(uih);
        menu_activate(item, uih, d);
    }

    char welcome[80];
    sprintf(welcome, gettext("Welcome to XaoS version %s"), XaoS_VERSION);
    /*TYPE*/ uih_message(uih, welcome);
    if (printspeed) {
        ui_printspeed();
    }

#ifdef SFFE_USING
    int err = 0;
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
#endif

    int inmovement = 1;
    int time;
    // QCoreApplication::processEvents(QEventLoop::AllEvents);
    for (;;) {
        widget->setCursor(uih->play ? Qt::ForbiddenCursor : Qt::CrossCursor);
        if (uih->display) {
            uih_prepare_image(uih);
            uih_updatestatus(uih);
            widget->repaint();
            window->showStatus("");
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
        ui_events(!inmovement && !uih->inanimation);
        inmovement = 0;
        if (callresize)
            ui_resize(), callresize = 0;
    };
}
