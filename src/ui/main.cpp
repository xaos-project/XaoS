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

/* Command line variables */
int printspeed;
int delaytime = 0;
int maxframerate = 80;
float pixelwidth = 0.0, pixelheight = 0.0;

static char *defrender = NULL;
static const char *rbasename = "anim";
static int alias = 0;
static int slowmode = 0;
static char *imgtype;
static char *defsize;
static float framerate;
static int letterspersec = 20;

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
    {"", P_HELP, NULL, "Animation rendering:"},
    {"-render", P_STRING, &defrender,
     "Render animation into sequence of .png files"},
    {"-basename", P_STRING, &rbasename,
     "Name for .png files (XaoS will add 4 digit number and extension"},
    {"-size", P_STRING, &defsize, "widthxheight"},
    {"-renderimage", P_STRING, &imgtype, "256 or truecolor"},
    {"-renderframerate", P_FLOAT, &framerate, "framerate"},
    {"-antialiasing", P_SWITCH, &alias,
     "Perform antialiasing (slow, requires quite lot of memory)"},
    {"-alwaysrecalc", P_SWITCH, &slowmode,
     "Always recalculate whole image (slowes down rendering, increases quality)"},

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
    int show_help = 0;
    int found;
    for (i = 1; i < argc && !error; i++) {
        found = 0;
        if (!strcmp("-help", argv[i])) {
            show_help = 1;
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
        return 0;
    }
    if (show_help) {
        const char *name[] = {"", "number", "string", "f.point"};
        printf("                 XaoS " XaoS_VERSION " help text\n\n");
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

const char *qt_gettext(const char *context, const char *text)
{
    static std::map<std::pair<const char *, const char *>, const char *>
        strings;
    const char *trans = strings[std::make_pair(context, text)];
    if (trans == NULL) {
        trans = strdup(
            QCoreApplication::translate(context, text).toStdString().c_str());
        strings[std::make_pair(context, text)] = trans;
    }
    return trans;
}

static MainWindow *window;
static void ui_unregistermenus(void);

void ui_quit(int i)
{
    delete window;
    xth_uninit();
    xio_uninit();
    ui_unregistermenus();
    uih_unregistermenus();
    puts(TR("Message", "Thank you for using XaoS\n"));
    exit(i);
}

static void ui_help(struct uih_context */*uih*/)
{
    QDesktopServices::openUrl(QUrl(HELP_URL));
}

static void ui_download(struct uih_context */*uih*/)
{
    QDesktopServices::openUrl(QUrl(DOWNLOAD_URL));
}

static void ui_fractalinfo(struct uih_context *uih)
{
    QString fractalname = uih->fcontext->currentformula->shortname;
    bool higherpower = false;
    for (auto c : fractalname) {
        if(c >= '0' and c <= '9') {
            fractalname.remove(c);
            higherpower = true;
        }
    }
    QMap<QString, QString> map;

    // Dictionary of fractaltype and corresponding wiki hypertext link
    map["mandel"] = higherpower ? "higher-power-mandelbrots" : "mandelbrot";
    fractalname = map.find(fractalname) != map.end() ? map[fractalname] : fractalname;

    QDesktopServices::openUrl(QUrl(FRACTALINFO_URL + fractalname));
}

static void ui_feedback(struct uih_context */*uih*/)
{
    QDesktopServices::openUrl(QUrl(FEEDBACK_URL));
}

static void ui_forum(struct uih_context */*uih*/)
{
    QDesktopServices::openUrl(QUrl(FORUM_URL));
}

static void ui_about(struct uih_context *uih)
{
    MainWindow *window = nullptr;
    if (uih->data)
        window = reinterpret_cast<MainWindow *>(uih->data);
    QMessageBox::about(
        window, TR("Menu", "About"),
        "<a href=\"https://xaos-project.github.io/\">" +
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
            "Copyright © 1996-2020 XaoS Contributors<br>"
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

static void ui_font(struct uih_context *uih)
{
    MainWindow *window = nullptr;
    if (uih->data)
        window = reinterpret_cast<MainWindow *>(uih->data);
    window->chooseFont();
}

void uih_setlanguage(uih_context *c, int l)
{
    const char* menu = lang1(l);
    uih_updatemenus(c, menu);
    setLanguage(lang2(l));

    QSettings settings;
    settings.setValue("MainWindow/language", lang2(l));
    QMessageBox msgBox;
    msgBox.setText(TR("Message", "XaoS must restart to change the language."));
    msgBox.setInformativeText(TR("Message", "Do you want to quit now?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    int ret = msgBox.exec();
    if (ret == QMessageBox::Yes) {
        exit(0);
    }
}

#ifndef Q_OS_MAC
static void ui_fullscreensw(struct uih_context *uih)
{
    if (uih->data) {
        MainWindow *window = reinterpret_cast<MainWindow *>(uih->data);
        if (window->isFullScreen())
            window->showNormal();
        else
            window->showFullScreen();
    }
}

static int ui_fullscreenselected(struct uih_context *uih)
{
    if (uih->data) {
        MainWindow *window = reinterpret_cast<MainWindow *>(uih->data);
        return window->isFullScreen();
    }
    return 0;
}
#endif

/* WARNING: Increase this number in case there are new menu items added. */
#define MAX_MENUITEMS_I18N 33
/* These variables must be global: */
static menuitem *menuitems;
static menuitem menuitems_i18n[MAX_MENUITEMS_I18N];
int ui_no_menuitems_i18n = 0;

#define UI (MENUFLAG_NOPLAY | MENUFLAG_NOOPTION)

static void ui_unregistermenus(void)
{
    menu_delete(menuitems, NITEMS(menuitems));
    menu_delete(menuitems_i18n, ui_no_menuitems_i18n);
}

QTranslator qtTranslator;
QTranslator qtBaseTranslator;
QTranslator xaosTranslator;
char languageSetting[6] = "";
bool languageSysDefault = true;
// please keep the languages in the same order
const char *languages1[] = {
    "__", "cs", "en", "fr", "de", "hi", "hu", "is", "it", "pt", "ro", "ru", "rs", "es", "sv", "vi"
};
const char *languages2[] = {
    "_____", "cs_CZ", "en_US", "fr_FR", "de_DE", "hi_HI", "hu_HU", "is_IS", "it_IT", "pt_PT", "ro_RO", "ru_RU", "rs_RS", "es_ES", "sv_SV", "vi_VN"
};

const char *lang1(int i) {
    return languages1[i];
}
const char *lang2(int i) {
    return languages2[i];
}
const char* getLanguage() {
    return languageSetting;
}

static int ui_languageselected(uih_context *c, int p)
{
    if (c == NULL)
        return 0;
    if (languageSysDefault) {
        return (strcmp(languages2[UIH_LANG_SYS_DEFAULT], languages2[p]) == 0);
    }
    return (strcmp(languageSetting, languages2[p]) == 0);
}

void setLanguage(const char *lang) {
    if (lang == NULL || strcmp(languages2[UIH_LANG_SYS_DEFAULT], lang) == 0) {
        languageSysDefault = true;
    } else {
        languageSysDefault = false;
    }
    qApp->removeTranslator(&qtTranslator);
    qApp->removeTranslator(&xaosTranslator);
    QString language = QString(lang);
    if (language.isNull() || languageSysDefault) {
        language = QLocale::system().name();
    }
    strcpy(languageSetting, language.toStdString().c_str());
    qtTranslator.load("qt_" + language,
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    qApp->installTranslator(&qtTranslator);
    qtBaseTranslator.load(QLocale::system(),
                          QStringLiteral("qtbase_"));
    qApp->installTranslator(&qtBaseTranslator);
    xaosTranslator.load("XaoS_" + language,
                        ":/i18n");
    qApp->installTranslator(&xaosTranslator);

    /* Without this some locales (e.g. the Hungarian) replaces "." to ","
       in numerical format and this will cause an automatic truncation
       at each parameter at certain places, e.g. drawing a new fractal. */
    QLocale::system().setNumberOptions(QLocale::DefaultNumberOptions);
    setlocale(LC_NUMERIC, "C");
    // printf("Language set to %s\n", languageSetting);
    // fflush(stdout);
}

static void ui_registermenus_i18n(void)
{
    int no_menuitems_i18n =
        ui_no_menuitems_i18n; /* This variable must be local. */
    MENUINT_I("file", NULL, TR("Menu", "Quit"), "quit",
              MENUFLAG_INTERRUPT | MENUFLAG_ATSTARTUP, ui_quit, UI);

    MENUNOP_I("ui", NULL, TR("Menu", "Message Font..."), "font", UI, ui_font);
    MENUNOP_I("uia", NULL, TR("Menu", "Message Font..."), "font", UI, ui_font);

    SUBMENU_I("ui", NULL, TR("Menu", "Set Language"), "setlang");
    // please keep the languages alphabetically ordered

    MENUINTRB_I("setlang", NULL, TR("Menu", "System default"), "__", UI, uih_setlanguage,
                UIH_LANG_SYS_DEFAULT, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "Czech", "cs", UI, uih_setlanguage,
                UIH_LANG_CS, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "English", "en", UI, uih_setlanguage,
                UIH_LANG_EN, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "French", "fr", UI, uih_setlanguage,
                UIH_LANG_FR, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "German", "de", UI, uih_setlanguage,
                UIH_LANG_DE, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "Hindi", "hi", UI, uih_setlanguage,
                UIH_LANG_HI, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "Hungarian", "hu", UI, uih_setlanguage,
                UIH_LANG_HU, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "Icelandic", "is", UI, uih_setlanguage,
                UIH_LANG_IS, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "Italian", "it", UI, uih_setlanguage,
                UIH_LANG_IT, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "Portuguese", "pt", UI, uih_setlanguage,
                UIH_LANG_PT, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "Romanian", "ro", UI, uih_setlanguage,
                UIH_LANG_RO, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "Russian", "ru", UI, uih_setlanguage,
                UIH_LANG_RU, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "Serbian", "rs", UI, uih_setlanguage,
                UIH_LANG_RS, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "Spanish", "es", UI, uih_setlanguage,
                UIH_LANG_ES, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "Swedish", "sv", UI, uih_setlanguage,
                UIH_LANG_SV, ui_languageselected);
    MENUINTRB_I("setlang", NULL, "Vietnamese", "vi", UI, uih_setlanguage,
                UIH_LANG_VI, ui_languageselected);


    MENUSEPARATOR_I("ui");
    MENUSEPARATOR_I("uia");
#ifndef Q_OS_MACOS
    MENUNOPCB_I("ui", NULL, TR("Menu", "Fullscreen"), "fullscreen", UI,
                ui_fullscreensw, ui_fullscreenselected);
    MENUNOPCB_I("uia", NULL, TR("Menu", "Fullscreen"), "fullscreena", UI,
                ui_fullscreensw, ui_fullscreenselected);
#endif

    MENUNOP_I("helpmenu", "h", TR("Menu", "Help"), "help", MENUFLAG_INCALC,
              ui_help);
    MENUNOP_I("helpmenu", NULL, TR("Menu", "Info on current fractal"), "fractalinfo",
              MENUFLAG_INCALC, ui_fractalinfo);
    MENUNOP_I("helpmenu", NULL, TR("Menu", "Send Feedback"), "feedback", MENUFLAG_INCALC,
              ui_feedback);
    MENUNOP_I("helpmenu", NULL, TR("Menu", "Get Updates"), "updates", MENUFLAG_INCALC,
              ui_download);
    MENUNOP_I("helpmenu", NULL, TR("Menu", "User Forum"), "forum", MENUFLAG_INCALC,
              ui_forum);
    MENUSEPARATOR_I("helpmenu");
    MENUNOP_I("helpmenu", NULL, TR("Menu", "About"), "about", UI, ui_about);

    no_menuitems_i18n -= ui_no_menuitems_i18n;
    menu_add(&(menuitems_i18n[ui_no_menuitems_i18n]), no_menuitems_i18n);
    ui_no_menuitems_i18n += no_menuitems_i18n;
    if (ui_no_menuitems_i18n > MAX_MENUITEMS_I18N) {
        fprintf(stderr, "MAX_MENUITEMS_I18N is set to an insufficiently low number, please increase it to %d\n", ui_no_menuitems_i18n);
        fflush(stderr);
        exit(1);
    }
#ifdef DEBUG
    printf("Filled %d ui menu items out of %d.\n", ui_no_menuitems_i18n,
           MAX_MENUITEMS_I18N);
#endif
}

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

    QSettings settings;
    if (defthreads == 0) {
        // defthreads will be 0 if -threads command line option was not given
        // so load it from saved settings instead
        defthreads = settings.value("MainWindow/threadCount", 1).toInt();
    } else {
        // -threads command line option was given, so save it to settings
        settings.setValue("MainWindow/threadCount", defthreads);
    }
    xth_init(defthreads);

    char *l = new char[6];
    strcpy(l, settings.value("MainWindow/language").toString().toStdString().c_str());
    if (strlen(l) >= 2) {
        setLanguage(l);
    } else {
        setLanguage(NULL);
    }

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

    int i = ui_render();
    if (i) {
        ui_unregistermenus();
        uih_unregistermenus();
        xio_uninit();
        exit(i - 1);
    }

    window = new MainWindow();
    window->eventLoop();

    return 0;
}


