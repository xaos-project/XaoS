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
#include <assert.h>

#include <QtWidgets>
#include <cstring>
#include <iostream>
#include <set>

#include "mainwindow.h"
#include "fractalwidget.h"
#include "customdialog.h"

#include "config.h"
#include "fconfig.h"
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
#include "archaccel.h"
#include "uiint.h"
#include "i18n.h"

#ifdef SFFE_USING
#include "sffe.h"
#endif

#ifndef exit_xaos
#define exit_xaos(i) exit(i)
#endif

xio_pathdata configfile;
static void ui_unregistermenus (void);
static struct image *ui_mkimages (int, int);

int err;
/*UI state */
uih_context *uih;
int ui_nogui;
static int callresize = 0;
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
    {"-delay", P_NUMBER, &delaytime,
     "Delay screen updates (milliseconds)"},
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
     "Pixel size options: \n\n  Knowledge of exact pixel size makes random dot stereogram look better. \n  Also is used for choosing correct view area"},
    {"-pixelwidth", P_FLOAT, &pixelwidth,
     "exact size of one pixel in centimeters"},
    {"-pixelheight", P_FLOAT, &pixelheight,
     "exact size of one pixel in centimeters"},
#ifdef SFFE_USING
    {"-formula", P_STRING, &sffeform,
     "user formula"},
    {"-forminit", P_STRING, &sffeinit,
     "z0 for user formula"},
#endif
    {NULL, 0, NULL, NULL}
};

MainWindow *window;
FractalWidget *widget;

int
main(int argc, char *argv[])
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
    xaosTranslator.load("XaoS_" + QLocale::system().name());
    app.installTranslator(&xaosTranslator);

    ui_init(argc, argv);
    ui_mainloop(1);
}

extern "C" {

const char *
qt_gettext(const char *text)
{
    static std::map<const char *, const char *> strings;
    const char *trans = strings[text];
    if (trans == NULL) {
        trans = strdup(QCoreApplication::translate("", text).toStdString().c_str());
        strings[text] = trans;
    }
    return trans;
}

}

static void
ui_updatemenus (uih_context * c, const char *name)
{
    const struct menuitem *item;
    if (ui_nogui) {
        if (name == NULL) {
            printf ("Root \"%s\"", uih->menuroot);
        }
        item = menu_findcommand (name);
        if (item == NULL) {
            /*x_fatalerror ("Internall error:unknown command %s", name); */
            return;
        }
        if (item->flags & MENUFLAG_CHECKBOX) {
            if (menu_enabled (item, c))
                printf ("checkbox \"%s\" on\n", name);
            else
                printf ("checkbox \"%s\" off\n", name);
        }
        if (item->flags & MENUFLAG_RADIO) {
            if (menu_enabled (item, c))
                printf ("radio \"%s\"\n", name);
        }
    }
    if (name == NULL) {
        window->buildMenu(uih->menuroot);
        return;
    }
    item = menu_findcommand (name);
    if (item == NULL) {
        /*fprintf (stderr, "Internall error:unknown command %s\n", name); */
        return;
    }
    if (item->flags & (MENUFLAG_CHECKBOX | MENUFLAG_RADIO)) {
        window->toggleMenu(name);
    }
}

static void
ui_display (void)
{
    if (nthreads == 1)
        uih_drawwindows (uih);
    widget->repaint();
    uih_cycling_continue (uih);
}

extern int dynsize;
static void
ui_outofmem (void)
{
    x_error (gettext ("XaoS is out of memory."));
}


static int
ui_passfunc (struct uih_context *c, int display, const char *text, float percent)
{
    char str[80];
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    uih_update(c, widget->mousePosition().x(), widget->mousePosition().y(), window->mouseButtons());
    uih_iterchange(c, window->keyCombination(), window->mouseButtons());
    if (!c->play) {
        if (c->display) {
            ui_display ();
            display = 1;
        }
        if (!c->interruptiblemode && !c->play) {
            if (display) {
                if (percent)
                    sprintf (str, "%s %3.2f%%        ", text, (double) percent);
                else
                    sprintf (str, "%s          ", text);
                window->showStatus(str);
            }
        }
    }
    return (0);
}

void
ui_menuactivate (const menuitem * item, dialogparam * d)
{
    if (item == NULL)
        return;
    if (item->type == MENU_SUBMENU) {
        window->popupMenu(item->shortname);
        return;
    } else {
        if (menu_havedialog (item, uih) && d == NULL) {
            window->showDialog(item->shortname);
            return;
        }
        if (uih->incalculation && !(item->flags & MENUFLAG_INCALC)) {
            menu_addqueue (item, d);
            if (item->flags & MENUFLAG_INTERRUPT)
                uih_interrupt (uih);
            return;
        }
        if (item->flags & MENUFLAG_CHECKBOX) {
            char s[256];
            uih_updateministatus(uih);
            widget->repaint();
            if (!menu_enabled (item, uih))
                sprintf (s, gettext ("Enabling: %s. "), item->name);
            else
                sprintf (s, gettext ("Disabling: %s. "), item->name);
            uih_message (uih, s);
        } else
            uih_message (uih, item->name);
        uih_saveundo (uih);
        menu_activate (item, uih, d);
        if (d != NULL)
            menu_destroydialog (item, d, uih);
    }
}


static void
ui_noguisw (uih_context * uih)
{
    ui_nogui ^= 1;
    ui_updatemenus (uih, "nogui");
}

static int
ui_noguienabled (uih_context * uih)
{
    return (ui_nogui);
}

static void
ui_message (struct uih_context *u)
{
    char s[100];
    if (uih->play)
        return;
    widget->setCursor(Qt::WaitCursor);
    sprintf (s, gettext ("Please wait while calculating %s"), uih->fcontext->currentformula->name[!uih->fcontext->mandelbrot]);
    window->showStatus(s);
}

void
ui_call_resize (void)
{
    callresize = 1;
    uih_interrupt (uih);
}

static void
processbuffer (void)
{
    const menuitem *item;
    dialogparam *d;
    if (uih->incalculation)
        return;
    while ((item = menu_delqueue (&d)) != NULL) {
        ui_menuactivate (item, d);
    }
}

static void
ui_doquit (int i) NORETURN;

static void
ui_doquit (int i)
{
    uih_cycling_off (uih);
    uih_freecatalog (uih);
    uih_freecontext (uih);
    tl_free_timer (maintimer);
    tl_free_timer (arrowtimer);
    tl_free_timer (loopt);
    widget->setImage(NULL);
    delete window;
    destroypalette (uih->image->palette);
    destroy_image (uih->image);
    xth_uninit ();
    xio_uninit ();
    ui_unregistermenus ();
    uih_unregistermenus ();
    exit_xaos (i);
}

void
ui_quit (void)
{
    printf (gettext ("Thank you for using XaoS\n"));
    ui_doquit (0);
}

static void
ui_quitwr (uih_context * c, int quit)
{
    if (c == NULL) {
        ui_unregistermenus ();
        uih_unregistermenus ();
        xio_uninit ();
        exit_xaos (0);
    }
    if (quit)
        ui_quit ();
}

int
ui_key (int key)
{
    int sym;
    char mkey[2];
    const menuitem *item;
    switch (sym = tolower (key)) {
        case ' ':
            uih->display = 1;
            if (uih->play) {
                if (uih->incalculation) {
                    uih_updateministatus(uih);
                    widget->repaint();
                }
                else {
                    uih_skipframe (uih);
                    window->showStatus(gettext("Skipping, please wait..."));
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
            item = menu_findkey (mkey, uih->menuroot);
            if (item == NULL) {
                mkey[0] = sym;
                item = menu_findkey (mkey, uih->menuroot);
            }
            if (item != NULL) {
                dialogparam *p = NULL;
                if (menu_havedialog (item, uih)) {
                    const menudialog *d = menu_getdialog (uih, item);
                    int mousex, mousey, buttons;
                    mousex = widget->mousePosition().x();
                    mousey = widget->mousePosition().y();
                    buttons = window->mouseButtons();
                    if (d[0].question != NULL && d[1].question == NULL && d[0].type == DIALOG_COORD) {
                        p = (dialogparam *) malloc (sizeof (dialogparam));
                        uih_screentofractalcoord (uih, mousex, mousey, p->dcoord, p->dcoord + 1);
                    }
                }
                ui_menuactivate (item, p);
            }
            break;
    }
    processbuffer ();
    return 0;
}

#ifdef _EFENCE_
int EF_ALIGNMENT = 1;
int EF_PROTECT_BELOW = 0;
int EF_PROTECT_FREE = 1;
#endif
static void
ui_helpwr (struct uih_context *c)
{
    QDesktopServices::openUrl(QUrl(HELP_URL));
}

static void
ui_about (struct uih_context *c)
{
    QMessageBox::about(NULL, gettext("About"),
                       "<a href=\"http://xaos.sf.net\">" +
                       QCoreApplication::applicationName() + "</a> " +
                       QCoreApplication::applicationVersion() +
                       " (" + QSysInfo::kernelType() + " " +
                       // QSysInfo::kernelVersion() + " "
                       // QSysInfo::buildAbi() + " " +
                       QSysInfo::buildCpuArchitecture() + ")"
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

#define UI (MENUFLAG_NOPLAY|MENUFLAG_NOOPTION)

static void
ui_registermenus_i18n (void)
{
    int no_menuitems_i18n = ui_no_menuitems_i18n;       /* This variable must be local. */
    MENUINT_I ("file", NULL, gettext ("Quit"), "quit", MENUFLAG_INTERRUPT | MENUFLAG_ATSTARTUP, ui_quitwr, 1);
    MENUNOP_I ("helpmenu", "h", gettext ("Help"), "help", MENUFLAG_INCALC, ui_helpwr);
    MENUNOP_I ("helpmenu", NULL, gettext ("About"), "about", NULL, ui_about);
    MENUNOPCB_I ("ui", NULL, gettext ("Disable XaoS's builtin GUI"), "nogui", MENUFLAG_INCALC | MENUFLAG_ATSTARTUP | MENUFLAG_NOMENU, ui_noguisw, ui_noguienabled);
    no_menuitems_i18n -= ui_no_menuitems_i18n;
    menu_add (&(menuitems_i18n[ui_no_menuitems_i18n]), no_menuitems_i18n);
    ui_no_menuitems_i18n += no_menuitems_i18n;
}

static void
ui_unregistermenus (void)
{
    menu_delete (menuitems, NITEMS (menuitems));
    menu_delete (menuitems_i18n, ui_no_menuitems_i18n);
}

int number_six = 6;

#ifdef SFFE_USING
        /* parser variables vars */
cmplx Z, C, pZ;
#endif

void
ui_printspeed(struct uih_context *uih)
{
    int c = 0;
    int x, y, b, k;
    int linesize = uih->image->bytesperpixel * uih->image->height;
    int size = linesize * uih->image->height;
    window->showStatus("Preparing for speedtest");
    uih->passfunc = NULL;
    tl_sleep (1000000);
    for (c = 0; c < 5; c++)
        widget->repaint();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    window->showStatus("Measuring dislay speed");
    tl_sleep (1000000);
    tl_update_time ();
    tl_reset_timer (maintimer);
    c = 0;
    while (tl_lookup_timer (maintimer) < 5000000) {
        widget->repaint();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        tl_update_time();
        c++;
    }
    x_message ("Driver speed: %g FPS (%.4f MBPS)", c / 5.0, c * (double) size / 5.0 / 1024 / 1024);

    window->showStatus("Measuring memcpy speed");
    for (c = 0; c < 5; c++) {
        for (x = 0; x < uih->image->height; x++)
            memcpy (uih->image->currlines[y], uih->image->oldlines[y], linesize);
    }
    tl_update_time ();
    tl_reset_timer (maintimer);
    c = 0;
    while (tl_lookup_timer (maintimer) < 5000000) {
        for (x = 0; x < uih->image->height; x++)
            memcpy (uih->image->currlines[y], uih->image->oldlines[y], linesize);
        tl_update_time (), c++;
    }
    x_message ("Memcpy speed: %g FPS (%.4f MBPS)", c / 5.0, c * (double) size / 5.0 / 1024 / 1024);

    window->showStatus("Measuring missaligned memcpy speed");
    tl_update_time ();
    tl_reset_timer (maintimer);
    c = 0;
    while (tl_lookup_timer (maintimer) < 5000000) {
        for (x = 0; x < uih->image->height; x++)
            memcpy (uih->image->currlines[y] + 1, uih->image->oldlines[y] + 2, linesize - 2);
        tl_update_time (), c++;
    }
    x_message ("Missaligned memcpy speed: %g FPS (%.4f MBPS)", c / 5.0, c * (double) size / 5.0 / 1024 / 1024);

    window->showStatus("Measuring size6 memcpy speed");
    tl_update_time ();
    tl_reset_timer (maintimer);
    c = 0;
    while (tl_lookup_timer (maintimer) < 5000000) {
        int x, y;
        for (y = 0; y < uih->image->height; y++)
            for (x = 0; x < linesize - 6; x += 6) {
                memcpy (uih->image->currlines[y] + x, uih->image->oldlines[y] + x, number_six);
            }
        tl_update_time (), c++;
    }
    x_message ("Size 6 memcpy speed: %g FPS (%.4f MBPS)", c / 5.0, c * (double) size / 5.0 / 1024 / 1024);

    widget->repaint();
    window->showStatus("Measuring calculation speed");
    speed_test (uih->fcontext, uih->image);
    window->showStatus("Measuring new image calculation loop");
    uih_prepare_image (uih);
    tl_update_time ();
    tl_reset_timer (maintimer);
    for (c = 0; c < 5; c++)
        uih_newimage (uih), uih->fcontext->version++, uih_prepare_image (uih);
    widget->repaint();
    x_message ("New image caluclation took %g seconds (%.2g fps)", tl_lookup_timer (maintimer) / 5.0 / 1000000.0, 5000000.0 / tl_lookup_timer (maintimer));
    tl_update_time ();
    for (c = 0; c < 5; c++)
        uih_animate_image (uih), uih_prepare_image (uih), c++;
    c = 0;
    tl_update_time ();
    tl_reset_timer (maintimer);
    window->showStatus("Measuring zooming algorithm loop");
    while (tl_lookup_timer (maintimer) < 5000000)
        uih_animate_image (uih), uih_prepare_image (uih), tl_update_time (), c++;
    x_message ("Approximation loop speed: %g FPS", c / 5.0);
    ui_doquit (0);
}

void
ui_init (int argc, char **argv)
{
    xio_init (argv[0]);
    params_register (global_params);
    params_register (ui_fractal_params);
    uih_registermenudialogs_i18n ();    /* Internationalized dialogs. */
    /* Dialogs must be generated before menus because menu items
       link to dialog pointers. */
    uih_registermenus_i18n ();  /* Internationalized menus. */
    uih_registermenus ();
    ui_registermenus_i18n ();   /* Internationalized menus. */
    if (!params_parser (argc, argv)) {
        ui_unregistermenus ();
        uih_unregistermenus ();
        xio_uninit ();
        exit_xaos (-1);
    }
#ifdef MEM_DEBUG
    D_NORMAL;
#endif
#ifdef DEBUG
    printf ("Initializing driver\n");
#endif

    window = new MainWindow();
    widget = window->fractalWidget();
    window->show();

    QScreen *screen = window->windowHandle()->screen();
    if (!pixelwidth)
        pixelwidth = 2.54 / screen->physicalDotsPerInchX();
    if (!pixelheight)
        pixelheight = 2.54 / screen->physicalDotsPerInchY();

    signal (SIGFPE, SIG_IGN);
    srand (time (NULL));
    xth_init (defthreads);

    int i = ui_dorender_params ();
    if (i) {
        ui_unregistermenus ();
        uih_unregistermenus ();
        xio_uninit ();
        exit_xaos (i - 1);
    }

#ifdef HOMEDIR
    if (getenv ("HOME") != NULL) {
        char home[256], *env = getenv ("HOME");
        int maxsize = 255 - (int) strlen (CONFIGFILE) - 1;      /*Avoid buffer owerflow */
        int i;
        for (i = 0; i < maxsize && env[i]; i++)
            home[i] = env[i];
        home[i] = 0;
        xio_addfname (configfile, home, CONFIGFILE);
    } else
#endif
        xio_addfname (configfile, XIO_EMPTYPATH, CONFIGFILE);

    maintimer = tl_create_timer ();
    arrowtimer = tl_create_timer ();
    loopt = tl_create_timer ();

    tl_update_time ();
    /*tl_process_group (syncgroup, NULL); */
    tl_reset_timer (maintimer);
    tl_reset_timer (arrowtimer);

#ifdef COMPILE_PIPE
    if (defpipe != NULL) {
        window->showStatus("Initializing pipe");
        ui_pipe_init (defpipe);
    }
#else
    if (defpipe != NULL) {
        x_fatalerror ("Pipe input not supported!");
    }
#endif

    uih = window->createContext();

    if (printspeed) {
        ui_printspeed(uih);
    }
}


void
ui_resize (void)
{
    int w, h;

    /* Prevent crash on startup for Mac OS X */
    if (!uih)
        return;

    if (uih->incalculation) {
        uih_interrupt (uih);
        return;
    }
    uih_clearwindows (uih);
    uih_stoptimers (uih);
    uih_cycling_stop (uih);
    uih_savepalette (uih);
    w = widget->size().width();
    h = widget->size().height();
    assert (w > 0 && w < 65000 && h > 0 && h < 65000);
    if (w != uih->image->width || h != uih->image->height) {
        widget->setImage(NULL);
        destroy_image (uih->image);
        destroypalette (uih->palette);
        struct image *image = window->createImage();
        if (!uih_updateimage (uih, image)) {
            delete window;
            x_error (gettext ("Can not allocate tables"));
            ui_outofmem ();
            exit_xaos (-1);
        }
        tl_process_group (syncgroup, NULL);
        tl_reset_timer (maintimer);
        tl_reset_timer (arrowtimer);
        uih_newimage (uih);
    }
    uih_newimage (uih);
    uih_restorepalette (uih);
    /*uih_mkdefaultpalette(uih); */
    uih->display = 1;;
    uih_cycling_continue (uih);
}

void
ui_mainloop (int loop)
{
    int inmovement = 1;
    int time;
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    do {
        widget->setCursor(uih->play ? Qt::WaitCursor : uih->inhibittextoutput ? Qt::CrossCursor: Qt::CrossCursor);
        if (uih->display) {
            uih_prepare_image (uih);
            uih_updateministatus(uih);
            widget->repaint();
        }
        if ((time = tl_process_group (syncgroup, NULL)) != -1) {
            if (!inmovement && !uih->inanimation) {
                if (time > 1000000 / 50)
                    time = 1000000 / 50;
                if (time > delaytime) {
                    tl_sleep (time - delaytime);
                    tl_update_time ();
                }
            }
            inmovement = 1;
        }
        if (delaytime || maxframerate) {
            tl_update_time ();
            time = tl_lookup_timer (loopt);
            tl_reset_timer (loopt);
            time = 1000000 / maxframerate - time;
            if (time < delaytime)
                time = delaytime;
            if (time) {
                tl_sleep (time);
                tl_update_time ();
            }
        }
        processbuffer ();
        QCoreApplication::processEvents(!inmovement && !uih->inanimation ? QEventLoop::WaitForMoreEvents : QEventLoop::AllEvents);
        uih_update(uih, widget->mousePosition().x(), widget->mousePosition().y(), window->mouseButtons());
        uih_iterchange(uih, window->keyCombination(), window->mouseButtons());
        inmovement = 0;
        if (callresize)
            ui_resize (), callresize = 0;
    }
    while (loop);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QCoreApplication::applicationName());

    m_fractalWidget = new FractalWidget();
    setCentralWidget(m_fractalWidget);

    readSettings();
}

MainWindow::~MainWindow()
{
}

struct uih_context *
MainWindow::createContext()
{
    m_fractalWidget->setCursor(Qt::WaitCursor);
    showStatus("Initializing. Please wait");
    showStatus("Creating framebuffer");
    struct image *image = createImage();
    showStatus("Initializing fractal engine");
    m_context = uih_mkcontext (PIXELSIZE, image, ui_passfunc, ui_message, ui_updatemenus);
    m_context->fcontext->version++;
    buildMenu(m_context->menuroot);
    showStatus("Loading message catalog");
    char language[10];
    strcpy(language, QLocale::system().name().toStdString().c_str());
    language[2] = '\0';
    uih_loadcatalog (m_context, language);
    showStatus("Initializing timing system");
    uih_newimage (m_context);
    /*uih_constantframetime(m_context,1000000/20); */
    showStatus("Reading configuration file");
    {
        xio_file f = xio_ropen (configfile);    /*load the configuration file */
        if (f != XIO_FAILED) {
            uih_load (m_context, f, configfile);
            if (m_context->errstring) {
                x_error ("Configuration file %s load failed", configfile);
                uih_printmessages (m_context);
                x_error ("Hint:try to remove it :)");
                ui_doquit (1);
            }
        }
    }
    showStatus("Processing command line parameters");
    {
        const menuitem *item;
        dialogparam *d;
        while ((item = menu_delqueue (&d)) != NULL) {
            uih_saveundo (m_context);
            menu_activate (item, m_context, d);
        }
    }

    char welcome[80];
    sprintf (welcome, gettext ("Welcome to XaoS version %s"), XaoS_VERSION);
     /*TYPE*/ uih_message (m_context, welcome);
#ifdef SFFE_USING
    /*SFFE : malczak */
    if (m_context->parser->expression == NULL) {
        if (sffeform)
            err = sffe_parse (&m_context->parser, (char *) sffeform);
        else
            sffe_parse (&m_context->parser, "z^2+c");
    }

    if (sffeinit) {
        m_context->pinit = sffe_alloc ();
        sffe_regvar (&m_context->pinit, &pZ, "p");
        sffe_regvar (&m_context->pinit, &C, "c");
        if (sffe_parse (&m_context->pinit, (char *) sffeinit) > 0)
            sffe_free (&m_context->pinit);
    };

    if (err > 0)
        sffe_parse (&m_context->parser, "z^2+c");
     /*SFFE*/
#endif

    return m_context;
}

FractalWidget *MainWindow::fractalWidget()
{
    return m_fractalWidget;
}

void MainWindow::readSettings()
{
    QSettings settings;
    QPoint pos = settings.value("windowPosition", QPoint(200, 200)).toPoint();
    QSize size = settings.value("imageSize", QSize(640, 480)).toSize();
    m_fractalWidget->setSizeHint(size);
    move(pos);
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("windowPosition", pos());
    settings.setValue("imageSize", size());
}

void MainWindow::closeEvent(QCloseEvent *)
{
    writeSettings();
    ui_quit();
}

QKeySequence::StandardKey MainWindow::keyForItem(const QString &name)
{
    if (name == "initstate") return QKeySequence::New;
    if (name == "loadpos") return QKeySequence::Open;
    if (name == "savepos") return QKeySequence::Save;
    if (name == "quit") return QKeySequence::Quit;
    if (name == "undo") return QKeySequence::Undo;
    if (name == "redo") return QKeySequence::Redo;
    if (name == "interrupt") return QKeySequence::Cancel;
    if (name == "recalculate") return QKeySequence::Refresh;
    if (name == "help") return QKeySequence::HelpContents;

    return QKeySequence::UnknownKey;
}

void MainWindow::buildMenu(const char *name)
{
    menuBar()->clear();

    const menuitem *item;
    for (int i = 0; (item = menu_item(name, i)) != NULL; i++) {
        if (item->type == MENU_SUBMENU) {
            QMenu *menu = menuBar()->addMenu(QString(item->name));
            buildMenu(item->shortname, menu, false);
        }
    }
}

void MainWindow::buildMenu(const char *name, QMenu *parent, bool numbered)
{
    QActionGroup *group = 0;

    connect(parent, SIGNAL(aboutToShow()), SLOT(updateMenuCheckmarks()));

    const menuitem *item;
    for (int i = 0, n = 0; (item = menu_item(name, i)) != NULL; i++) {

        QString itemName(item->name);
        if (numbered) {
            char c;
            if (n < 9)
                c = n + '1';
            else if (n == 9)
                c = '0';
            else
                c = 'A' + n - 10;
            itemName = QString::asprintf("&%c ", c) + itemName;

            if (item->type != MENU_SEPARATOR)
                n++;
        }

        if (item->type == MENU_DIALOG || item->type == MENU_CUSTOMDIALOG)
            itemName += "...";

        if (item->type == MENU_SEPARATOR) {
            parent->addSeparator();
        } else if (item->type == MENU_SUBMENU) {
            QMenu *menu = parent->addMenu(itemName);
            buildMenu(item->shortname, menu, numbered);
        } else {
            QAction *action = new QAction(itemName, parent);
            action->setShortcuts(keyForItem(item->shortname));
            action->setObjectName(item->shortname);
            if (item->flags & (MENUFLAG_RADIO | MENUFLAG_CHECKBOX)) {
                action->setCheckable(true);
                action->setChecked(menu_enabled(item, m_context));
                if (item->flags & MENUFLAG_RADIO) {
                    if (!group)
                        group = new QActionGroup(parent);
                    action->setActionGroup(group);
                }
            }
            connect(action, SIGNAL(triggered()), this, SLOT(activateMenuItem()));
            parent->addAction(action);
        }
    }
}

void MainWindow::popupMenu(const char *name)
{
    QMenu *menu = new QMenu(this);
    buildMenu(name, menu, true);
    menu->exec(QCursor::pos());
    delete menu;
}

void MainWindow::toggleMenu(const char *name)
{
    const menuitem *item = menu_findcommand(name);
    QAction *action = menuBar()->findChild<QAction *>(name);
    if (action)
        action->setChecked(menu_enabled(item, m_context));
}

void MainWindow::activateMenuItem()
{
    QAction *action = qobject_cast<QAction *>(sender());
    const menuitem *item = menu_findcommand(action->objectName().toUtf8());
    ui_menuactivate(item, NULL);
}

void MainWindow::updateMenuCheckmarks()
{
    QMenu *menu = qobject_cast<QMenu *>(sender());
    foreach(QAction *action, menu->actions()) {
        if (action->isCheckable()) {
            const menuitem *item = menu_findcommand(action->objectName().toUtf8());
            action->setChecked(menu_enabled(item, globaluih));
        }
    }

}

void MainWindow::showDialog(const char *name)
{
    const menuitem *item = menu_findcommand(name);
    if (!item) return;

    const menudialog *dialog = menu_getdialog(m_context, item);
    if (!dialog) return;

    int nitems;
    for (nitems = 0; dialog[nitems].question; nitems++);

    if (nitems == 1 && (dialog[0].type == DIALOG_IFILE || dialog[0].type == DIALOG_OFILE)) {
        QString filter = QString("*.%1").arg(QFileInfo(dialog[0].defstr).completeSuffix());
        QString directory;// = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);

        QString fileName;
        if (dialog[0].type == DIALOG_IFILE)
            fileName = QFileDialog::getOpenFileName(this, item->name, directory, filter);
        else if (dialog[0].type == DIALOG_OFILE)
            fileName = QFileDialog::getSaveFileName(this, item->name, directory, filter);


        if (!fileName.isNull()) {
            dialogparam *param = (dialogparam *)malloc(sizeof(dialogparam));
            param->dstring = strdup(fileName.toUtf8());
            ui_menuactivate(item, param);
        }
    } else {
        CustomDialog customDialog(m_context, item, dialog, this);
        if (customDialog.exec() == QDialog::Accepted)
            ui_menuactivate(item, customDialog.parameters());
    }
}

void MainWindow::showStatus(const char *text)
{
    printf("%s\n", text);
}

int MainWindow::mouseButtons()
{

    // Qt::MetaModifier maps to control key on Macs
#ifdef Q_WS_MAC
    Qt::KeyboardModifier controlModifier = Qt::MetaModifier;
#else
    Qt::KeyboardModifier controlModifier = Qt::ControlModifier;
#endif

    int mouseButtons = 0;

    // Modifier keys change behavior of left and right mouse buttons
    if (m_keyboardModifiers & controlModifier) {
        // Control key swaps left and right buttons
        if (m_mouseButtons & Qt::LeftButton)
            mouseButtons |= BUTTON3;
        if (m_mouseButtons & Qt::RightButton)
            mouseButtons |= BUTTON1;
    } else if (m_keyboardModifiers & Qt::ShiftModifier) {
        // Shift key makes left and right buttons emulate middle button
        if (m_mouseButtons & (Qt::LeftButton | Qt::RightButton))
            mouseButtons |= BUTTON2;
    } else {
        // Otherwise, mouse buttons map normally
        if (m_mouseButtons & Qt::LeftButton)
            mouseButtons |= BUTTON1;
        if (m_mouseButtons & Qt::RightButton)
            mouseButtons |= BUTTON3;
    }

    // Middle button is unaffected by modifier keys
    if (m_mouseButtons & Qt::MidButton)
        mouseButtons |= BUTTON2;

    return mouseButtons;
}

int MainWindow::keyCombination()
{
    return m_keyCombination;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    m_mouseButtons = event->buttons();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_mouseButtons = event->buttons();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    m_keyboardModifiers = event->modifiers();

    switch (event->key()) {
    case Qt::Key_Left:
        m_keyCombination |= 1;
        break;
    case Qt::Key_Right:
        m_keyCombination |= 2;
        break;
    case Qt::Key_Up:
        m_keyCombination |= 4;
        break;
    case Qt::Key_Down:
        m_keyCombination |= 8;
        break;
    default:
        if (!event->text().isEmpty())
            ui_key(event->text().toUtf8()[0]);
        else
            event->ignore();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    m_keyboardModifiers = event->modifiers();

    switch (event->key()) {
    case Qt::Key_Left:
        m_keyCombination &= ~1;
        break;
    case Qt::Key_Right:
        m_keyCombination &= ~2;
        break;
    case Qt::Key_Up:
        m_keyCombination &= ~4;
        break;
    case Qt::Key_Down:
        m_keyCombination &= ~8;
        break;
    default:
        event->ignore();
    }
}

struct image *
MainWindow::createImage()
{
    union paletteinfo info;
    info.truec.rmask = 0xff0000;
    info.truec.gmask = 0x00ff00;
    info.truec.bmask = 0x0000ff;
    struct palette *palette = createpalette (0, 0, UI_TRUECOLOR, 0, 0, NULL, NULL, NULL, NULL, &info);
    if (!palette) {
        x_error (gettext ("Can not create palette"));
        exit(-1);
    }
    struct image *image = create_image_qt(widget->width(), widget->height(), palette, pixelwidth, pixelheight);
    if (!image) {
        x_error (gettext ("Can not create image"));
        exit(-1);
    }
    widget->setImage(image);
    return image;
}
