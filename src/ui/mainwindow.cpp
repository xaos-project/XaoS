#include <QtWidgets>
#include <cassert>
#include <QTimer>
#include <iostream>

#include "mainwindow.h"
#include "fractalwidget.h"
#include "customdialog.h"

#include "ui.h"
#include "ui_helper.h"
#include "timers.h"
#include "i18n.h"
#include "xerror.h"
#include "filter.h"
#include "xthread.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/val.h>
#include <iostream>
#include <stdlib.h>
#include <ctype.h>
#endif

void MainWindow::printSpeed()
{
    int c = 0;
    int x, y = 0;
    int linesize = uih->image->bytesperpixel * uih->image->height;
    int size = linesize * uih->image->height;
    showStatus("Preparing for speedtest");
    uih->passfunc = NULL;
    tl_sleep(1000000);
    for (c = 0; c < 5; c++)
        widget->repaint();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    showStatus("Measuring display speed");
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

    showStatus("Measuring memcpy speed");
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

    showStatus("Measuring missaligned memcpy speed");
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

    showStatus("Measuring size6 memcpy speed");
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
    showStatus("Measuring calculation speed");
    speed_test(uih->fcontext, uih->image);
    showStatus("Measuring new image calculation loop");
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
    showStatus("Measuring zooming algorithm loop");
    while (tl_lookup_timer(maintimer) < 5000000)
        uih_animate_image(uih), uih_prepare_image(uih), tl_update_time(), c++;
    x_message("Approximation loop speed: %g FPS", c / 5.0);
    ui_quit(0);
}

void MainWindow::menuActivate(const menuitem *item, dialogparam *d)
{
    if (item == NULL)
        return;
    if (item->type == MENU_SUBMENU) {
        popupMenu(item->shortname);
        return;
    } else {
        if (menu_havedialog(item, uih) && d == NULL) {
            showDialog(item->shortname);
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
                sprintf(s, TR("Message", "Enabling: %s. "), item->name);
            else
                sprintf(s, TR("Message", "Disabling: %s. "), item->name);
            uih_message(uih, s);
        } else
            uih_message(uih, item->name);
        uih_saveundo(uih);
        menu_activate(item, uih, d);
        if (d != NULL)
            menu_destroydialog(item, d, uih);
    }
}

void MainWindow::processQueue()
{
    const menuitem *item;
    dialogparam *d;
    if (uih->incalculation)
        return;
    while ((item = menu_delqueue(&d)) != NULL) {
        menuActivate(item, d);
    }
}

int MainWindow::processKey(int key)
{
    int sym = tolower(key);
    if (sym == ' ') {
        uih->display = 1;
        if (uih->play) {
            if (uih->incalculation) {
                uih_updatestatus(uih);
                widget->repaint();
            } else {
                uih_skipframe(uih);
                showStatus(TR("Message", "Skipping, please wait..."));
            }
        }
    } else {
        char mkey[2];
        mkey[0] = key;
        mkey[1] = 0;
        const menuitem *item;
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
            menuActivate(item, p);
        }
    }
    processQueue();
    return 0;
}

#define KEYLEFT 1
#define KEYRIGHT 2
#define KEYUP 4
#define KEYDOWN 8

bool MainWindow::processArrows(int *counter, const char *text, int speed,
                               int keys, int lastkeys, int down, int up,
                               bool tenskip, int min, int max)
{
    static int pid = -1;
    bool changed = false;
    if (tl_lookup_timer(arrowtimer) > 1000000)
        tl_reset_timer(arrowtimer);
    if ((keys & up) && !(lastkeys & up)) {
        (*counter)++;
        tenskip = false;
        changed = true;
        tl_reset_timer(arrowtimer);
    }
    if ((keys & down) && !(lastkeys & down)) {
        (*counter)--;
        tenskip = false;
        changed = true;
        tl_reset_timer(arrowtimer);
    }
    while (tl_lookup_timer(arrowtimer) > speed * FRAMETIME) {
        tl_slowdown_timer(arrowtimer, speed * FRAMETIME);
        if (keys & up) {
            if (tenskip && !(*counter % 10))
                (*counter) += 10;
            else
                (*counter)++;
            changed = true;
        }
        if (keys & down) {
            if (tenskip && !(*counter % 10))
                (*counter) -= 10;
            else
                (*counter)--;
            changed = true;
        }
    }
    if (changed) {
        if (*counter > max)
            *counter = max;
        if (*counter < min)
            *counter = min;
        char str[80];
        sprintf(str, text, *counter);
        uih_rmmessage(uih, pid);
        pid = uih_message(uih, str);
    }
    return changed;
}

#define ROTATESPEEDUP 30

void MainWindow::processEvents(bool wait)
{
    char str[80];
    static int spid;
    QCoreApplication::processEvents(wait ? QEventLoop::WaitForMoreEvents
                                         : QEventLoop::AllEvents);
    static bool dirty = false;
    static int lastkey;
    static int maxiter;

    int mousex = widget->mousePosition().x();
    int mousey = widget->mousePosition().y();
    int buttons = mouseButtons();
    int key = keyCombination();
    tl_update_time();
    assert(!((key) & ~(KEYLEFT | KEYRIGHT | KEYUP | KEYDOWN)) &&
           !((buttons) & ~(BUTTON1 | BUTTON2 | BUTTON3)));
    uih_update(uih, mousex, mousey, buttons);
    if (uih->play) {
        processArrows(&uih->letterspersec,
                      TR("Message", "Letters per second %i  "), 2, key, lastkey,
                      KEYLEFT, KEYRIGHT, false, 1, INT_MAX);
        return;
    }
    if (!uih->cycling) {
        if (uih->rotatemode == ROTATE_CONTINUOUS) {
            static int rpid;
            if (key == KEYRIGHT)
                uih->rotationspeed +=
                    ROTATESPEEDUP * tl_lookup_timer(maintimer) / 1000000.0;
            else if (key == KEYLEFT)
                uih->rotationspeed -=
                    ROTATESPEEDUP * tl_lookup_timer(maintimer) / 1000000.0;
            if (key & (KEYLEFT | KEYRIGHT)) {
                uih_rmmessage(uih, rpid);
                sprintf(
                    str,
                    TR("Message", "Rotation speed:%2.2f degrees per second "),
                    (float)uih->rotationspeed);
                rpid = uih_message(uih, str);
            }
            tl_reset_timer(maintimer);
        } else {
            if (!dirty)
                maxiter = uih->fcontext->maxiter;
            if (processArrows(&maxiter, TR("Message", "Iterations: %i   "), 1,
                              key, lastkey, KEYLEFT, KEYRIGHT, false, 1,
                              INT_MAX) ||
                (key & (KEYLEFT | KEYRIGHT))) {
                dirty = true;
                lastkey = key;
                return;
            }
        }
    }
    if (dirty) {
        if (uih->incalculation)
            uih_interrupt(uih);
        else {
            uih_setmaxiter(uih, maxiter);
            dirty = false;
        }
    }
    if (uih->cycling) {
        if (processArrows(&uih->cyclingspeed,
                          TR("Message", "Cycling speed: %i   "), 1, key,
                          lastkey, KEYLEFT, KEYRIGHT, 0, -1000000, INT_MAX)) {
            uih_setcycling(uih, uih->cyclingspeed);
        }
    }
    if (tl_lookup_timer(maintimer) > FRAMETIME || buttons) {
        double mul1 = tl_lookup_timer(maintimer) / FRAMETIME;
        double su = 1 + (SPEEDUP - 1) * mul1;
        if (su > 2 * SPEEDUP)
            su = SPEEDUP;
        tl_reset_timer(maintimer);
        if (key & KEYUP)
            uih->speedup *= su, uih->maxstep *= su;
        else if (key & KEYDOWN)
            uih->speedup /= su, uih->maxstep /= su;
        if (key & (KEYUP | KEYDOWN)) {
            sprintf(str, TR("Message", "speed:%2.2f "),
                    (double)uih->speedup * (1.0 / STEP));
            uih_rmmessage(uih, spid);
            spid = uih_message(uih, str);
        }
    }
    lastkey = key;
    return;
}

struct image *MainWindow::makeImage(int width, int height)
{
    struct palette *palette;
    union paletteinfo info;
    info.truec.rmask = 0xff0000;
    info.truec.gmask = 0x00ff00;
    info.truec.bmask = 0x0000ff;
    palette =
        createpalette(0, 0, TRUECOLOR, 0, 0, NULL, NULL, NULL, NULL, &info);
    if (!palette) {
        x_error(TR("Error", "Can not create palette"));
        x_error(TR("Error", "XaoS is out of memory."));
        ui_quit(-1);
    }
    struct image *image =
        create_image_qt(width, height, palette, pixelwidth, pixelheight);
    if (!image) {
        x_error(TR("Error", "Can not create image"));
        x_error(TR("Error", "XaoS is out of memory."));
        ui_quit(-1);
    }
    widget->setImage(image);
    return image;
}

void MainWindow::resizeImage(int width, int height)
{
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
    assert(width > 0 && width < 65000 && height > 0 && height < 65000);
    if (width != uih->image->width || height != uih->image->height) {
        destroy_image(uih->image);
        destroypalette(uih->palette);
        struct image *image = makeImage(width, height);
        if (!uih_updateimage(uih, image)) {
            x_error(TR("Error", "Can not allocate tables"));
            x_error(TR("Error", "XaoS is out of memory."));
            ui_quit(-1);
        }
        tl_process_group(syncgroup, NULL);
        tl_reset_timer(maintimer);
        tl_reset_timer(arrowtimer);
        uih_newimage(uih);
    }
    uih_newimage(uih);
    uih_restorepalette(uih);
    uih->display = 1;
    uih_cycling_continue(uih);
}

xio_pathdata configfile;

void MainWindow::eventLoop()
{
    QTimer eventTimer;
    eventTimer.setTimerType(Qt::PreciseTimer);

    connect(&eventTimer, &QTimer::timeout, this, [=]() {
        int inmovement = 1;

        widget->setCursor(uih->play ? Qt::ForbiddenCursor : Qt::CrossCursor);

        if (uih->display) {
            uih_prepare_image(uih);
            uih_updatestatus(uih);
            widget->repaint();
            showStatus("");
        }

        int time = tl_process_group(syncgroup, nullptr);
        if (time != -1) {
            if (!inmovement && !uih->inanimation) {
                if (time > 1000000 / 50)
                    time = 1000000 / 50;
                if (time > delaytime) {
                    QThread::usleep(time - delaytime);
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
                QThread::usleep(time);
                tl_update_time();
            }
        }

        processQueue();
        processEvents(!inmovement && !uih->inanimation);
        inmovement = 0;

        if (shouldResize) {
            resizeImage(widget->size().width(), widget->size().height());
            shouldResize = false;
        }
    });

    // Start the event timer
    eventTimer.start(0);

    // Enter the Qt event loop
    QCoreApplication::exec();
}

void MainWindow::updateMenus(const char *name)
{
    const struct menuitem *item;
    if (name == NULL) {
        buildMenu(uih->menuroot);
        return;
    }
    item = menu_findcommand(name);
    if (item == NULL)
        return;
    if (item->flags & (MENUFLAG_CHECKBOX | MENUFLAG_RADIO)) {
        toggleMenu(name);
    }
}

void ui_updatemenus(struct uih_context *uih, const char *name)
{
    if (uih->data) {
        MainWindow *window = reinterpret_cast<MainWindow *>(uih->data);
        window->updateMenus(name);
    }
}

int MainWindow::showProgress(int display, const char *text, float percent)
{
    char str[80];
    processEvents(false);
    if (!uih->play) {
        if (uih->display) {
            if (nthreads == 1)
                uih_drawwindows(uih);
            widget->repaint();
            uih_cycling_continue(uih);
            display = 1;
        }
        if (!uih->interruptiblemode && !uih->play) {
            if (display) {
                if (percent)
                    sprintf(str, "%s %3.2f%%        ", text, (double)percent);
                else
                    sprintf(str, "%s          ", text);
                showStatus(str);
            }
        }
    }
    return 0;
}

static int ui_passfunc(struct uih_context *uih, int display, const char *text,
                       float percent)
{
    if (uih->data) {
        MainWindow *window = reinterpret_cast<MainWindow *>(uih->data);
        return window->showProgress(display, text, percent);
    }
    return 0;
}

void MainWindow::pleaseWait()
{
    char s[100];
    if (uih->play)
        return;
    widget->setCursor(Qt::WaitCursor);
    sprintf(s, TR("Message", "Please wait while calculating %s"),
            uih->fcontext->currentformula->name[!uih->fcontext->mandelbrot]);
    showStatus(s);
}

static void ui_message(struct uih_context *uih)
{
    if (uih->data) {
        MainWindow *window = reinterpret_cast<MainWindow *>(uih->data);
        window->pleaseWait();
    }
}

void MainWindow::chooseFont()
{
    QFontDialog *fontDialog = new QFontDialog(this);
    QSettings settings;
    QFont qfont(settings.value("MainWindow/messageFontFamily").toString(),
        settings.value("MainWindow/messageFontSize").toInt());
    fontDialog->setCurrentFont(qfont);
    connect(fontDialog, &QFontDialog::fontSelected,
            [=](const QFont &messageFont) {
                QSettings settings;
                settings.setValue("MainWindow/messageFontFamily", messageFont.family());
                settings.setValue("MainWindow/messageFontSize",
                                  messageFont.pointSize());
                uih->font = (void *) &messageFont;
            });
    fontDialog->open();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    menuBarRef = menuBar();
    setWindowTitle(QCoreApplication::applicationName());
    setMouseTracking(true);

    widget = new FractalWidget();
    setCentralWidget(widget);

    readSettings();

    widget->setCursor(Qt::WaitCursor);
    showStatus("Initializing. Please wait.");
    show();

    QScreen *screen = windowHandle()->screen();
    if (!pixelwidth)
        pixelwidth = 2.54 / screen->physicalDotsPerInchX();
    if (!pixelheight)
        pixelheight = 2.54 / screen->physicalDotsPerInchY();

    int width = widget->size().width();
    int height = widget->size().height();
    struct image *image = makeImage(width, height);
    uih = uih_mkcontext(PIXELSIZE, image, ui_passfunc, ui_message,
                        ui_updatemenus);
    uih->data = this;
    uih->font = &messageFont;
    buildMenu(uih->menuroot);
    uih->fcontext->version++;
    uih_newimage(uih);
    QSettings settings;

    // Try to load a catalog for the current language and if it doesn't exist,
    // default to English. Fixes "No catalog loaded" messages on tutorials
    // when using a language XaoS doesn't support
    //if (!uih_loadcatalog(uih, QLocale::system().name().left(2).toUtf8()))
    if (!uih_loadcatalog(uih, QString(getLanguage()).left(2).toUtf8()))
        uih_loadcatalog(uih, "en");

    tl_update_time();
    maintimer = tl_create_timer();
    arrowtimer = tl_create_timer();
    loopt = tl_create_timer();
    tl_reset_timer(maintimer);
    tl_reset_timer(arrowtimer);

    if (getenv("HOME") != NULL) {
        char home[256], *env = getenv("HOME");
        int maxsize =
            255 - (int)strlen(CONFIGFILE) - 1; /*Avoid buffer overflow */
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
    sprintf(welcome, TR("Message", "Welcome to XaoS version %s"), XaoS_VERSION);
    uih_message(uih, welcome);
    if (printspeed)
        printSpeed();

#ifdef __EMSCRIPTEN__
    // Obtain a command via URL:
    emscripten::val location = emscripten::val::global("location");
    auto href = location["href"].as<std::string>();
    int q = href.find("?");
    if (q == std::string::npos) {
        return; // No command was started with "?", stop and return.
    }
    auto command = href.substr(q + 1);
    if (command.length() == 0) {
        return; // No command was given, stop and return.
    }
    // std::cerr << "command = " << command << "\n";

    // Manual url_decode:
    const char *src = command.c_str();
    char *dst = new char[command.length() + 1];
    char *com = dst;
    // Taken from https://stackoverflow.com/questions/2673207/c-c-url-decode-library
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a')
                a -= 'a'-'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a'-'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
    // printf("command = %s\n", com);
    uih_loadstr(uih, com);
#endif

}

MainWindow::~MainWindow()
{
    uih_cycling_off(uih);
    uih_freecatalog(uih);
    uih_freecontext(uih);
    tl_free_timer(maintimer);
    tl_free_timer(arrowtimer);
    tl_free_timer(loopt);
    // Sometimes the image pointer is set to to 0xFEEEFEEEFEEEFEEE when we get
    // here and it crashes without this guard. Not sure why. Possibly
    // related to https://sourceforge.net/p/mingw-w64/bugs/727/
    if (uih->image != (image *)0xFEEEFEEEFEEEFEEE) {
        destroypalette(uih->image->palette);
        destroy_image(uih->image);
    }
}

void MainWindow::readSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/windowState").toByteArray());
    QString fontFamily = settings
                             .value("MainWindow/messageFontFamily",
                                    QApplication::font().family())
                             .toString();
    int fontSize = settings
                       .value("MainWindow/messageFontSize",
                              12)
                       .toInt();
    messageFont = QFont(fontFamily, fontSize);

}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/windowState", saveState());
}

void MainWindow::closeEvent(QCloseEvent *)
{
    writeSettings();
    ui_quit(0);
}

QKeySequence::StandardKey MainWindow::keyForItem(const QString &name)
{
    if (name == "initstate")
        return QKeySequence::New;
    if (name == "loadpos")
        return QKeySequence::Open;
    if (name == "savepos")
        return QKeySequence::Save;
    if (name == "quit")
        return QKeySequence::Quit;
    if (name == "undo")
        return QKeySequence::Undo;
    if (name == "redo")
        return QKeySequence::Redo;
    if (name == "fullscreen" || name == "fullscreena")
        return QKeySequence::FullScreen;
    if (name == "interrupt")
        return QKeySequence::Cancel;
    if (name == "recalculate")
        return QKeySequence::Refresh;
    if (name == "help")
        return QKeySequence::HelpContents;
    if (name == "copyurl")
        return QKeySequence::Copy;
    if (name == "saveimg")
        return QKeySequence::SaveAs;
    if (name == "fractalinfo")
        return QKeySequence::WhatsThis;
    if (name == "autopilot")
        return QKeySequence::ZoomIn;

    return QKeySequence::UnknownKey;
}

void MainWindow::buildMenu(const char *name)
{
    menuBarRef->clear();
    foreach (QAction *action, actions())
        removeAction(action);

    const menuitem *item;
    for (int i = 0; (item = menu_item(name, i)) != NULL; i++) {
        if (item->type == MENU_SUBMENU) {
            QMenu *menu = menuBarRef->addMenu(QString(item->name));
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

            /* TODO: Do this more elegantly. */
            if (strcmp(item->shortname, "loadpos") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen));
            }
            if (strcmp(item->shortname, "savepos") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentSave));
            }
            if (strcmp(item->shortname, "initstate") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentNew));
            }
            if (strcmp(item->shortname, "undo") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditUndo));
            }
            if (strcmp(item->shortname, "redo") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditRedo));
            }
            if (strcmp(item->shortname, "quit") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ApplicationExit));
            }
            if (strcmp(item->shortname, "record") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaRecord));
            }
            if (strcmp(item->shortname, "play") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
            }
            if (strcmp(item->shortname, "copyurl") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy));
            }
            if (strcmp(item->shortname, "about") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::HelpAbout));
            }
            if (strcmp(item->shortname, "saveimg") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::InsertImage));
            }
            if (strcmp(item->shortname, "updates") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::SoftwareUpdateAvailable));
            }
            if (strcmp(item->shortname, "autopilot") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ZoomIn));
            }
            if (strcmp(item->shortname, "fullscreen") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ViewFullscreen));
            }
            if (strcmp(item->shortname, "command") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::InputKeyboard));
            }
            if (strcmp(item->shortname, "feedback") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MailSend));
            }
            if (strcmp(item->shortname, "help") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::HelpAbout)); // HelpFaq would be better, but it does not work
            }
            if (strcmp(item->shortname, "xtextcenter") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::FormatJustifyCenter));
            }
            if (strcmp(item->shortname, "xtextright") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::FormatJustifyRight));
            }
            if (strcmp(item->shortname, "xtextleft") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::FormatJustifyLeft));
            }
            if (strcmp(item->shortname, "clearscreen") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditClear));
            }
            if (strcmp(item->shortname, "status") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DialogInformation));
            }
            if (strcmp(item->shortname, "interrupt") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ProcessStop));
            }
            if (strcmp(item->shortname, "mouserotate") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::InputMouse));
            }
            if (strcmp(item->shortname, "display") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ViewRestore));
            }
            if (strcmp(item->shortname, "uiview") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentProperties));
            }
            if (strcmp(item->shortname, "font") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::FormatTextBold));
            }
            if (strcmp(item->shortname, "text") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::InsertText));
            }
            if (strcmp(item->shortname, "usrform") == 0) {
                action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::InputTablet));
            }
            // std::cout << item->shortname << std::endl;

            if (item->flags & (MENUFLAG_RADIO | MENUFLAG_CHECKBOX)) {
                action->setCheckable(true);
                action->setChecked(menu_enabled(item, uih));
                if (item->flags & MENUFLAG_RADIO) {
                    if (!group)
                        group = new QActionGroup(parent);
                    action->setActionGroup(group);
                }
            }
            connect(action, SIGNAL(triggered()), this,
                    SLOT(activateMenuItem()));
            parent->addAction(action);
            if (action->shortcut() != QKeySequence::UnknownKey)
                addAction(
                    action); // so that shortcuts work when menubar is hidden
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
    QAction *action = menuBarRef->findChild<QAction *>(name);
    if (action)
        action->setChecked(menu_enabled(item, uih));
}

void MainWindow::activateMenuItem()
{
    QAction *action = qobject_cast<QAction *>(sender());
    const menuitem *item = menu_findcommand(action->objectName().toUtf8());
    menuActivate(item, NULL);
}

void MainWindow::updateMenuCheckmarks()
{
    QMenu *menu = qobject_cast<QMenu *>(sender());
    foreach (QAction *action, menu->actions()) {
        if (action->isCheckable()) {
            const menuitem *item =
                menu_findcommand(action->objectName().toUtf8());
            action->setChecked(menu_enabled(item, uih));
        }
    }
}

struct palette *gradientpal; QSpinBox *seedno, *algono, *shiftno; QLabel *img;

void MainWindow::updateVisualiser()
{
    // Get updated Colors
    int colors[101][3];
    getPaletteColor(gradientpal, seedno->value(),
                    algono->value()-1 < 0? 0:algono->value()-1, shiftno->value(), colors);

    // Load Curve
    QImage palImage(100, 1, QImage::Format_RGB32);

    // Fill Curve
    for(int i=0;i<100;i++) {
        QRgb value = qRgb(colors[i][0], colors[i][1], colors[i][2]);
        palImage.setPixelColor(i, 0, value);
    }

    // Save Result
    QPixmap newImage = QPixmap::fromImage(palImage.scaled(algono->width(),
                                                          algono->height()));
    img->setPixmap(newImage);
}

unsigned char newColors[32][3];

void MainWindow::colorPicker()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    int idx = button->objectName().toInt();
    QColorDialog* qColorDialog = new QColorDialog(this);
    QColor currentColor(newColors[idx][0], newColors[idx][1], newColors[idx][2]);
    qColorDialog->setCurrentColor(currentColor);
    qColorDialog->setModal(false);
    connect(qColorDialog, &QColorDialog::colorSelected, qColorDialog,
            [=](const QColor &color) {
                QPalette pal = button->palette();
                button->setAutoFillBackground(true);
                pal.setColor(QPalette::Button, color);
                button->setPalette(pal);
                button->update();
                newColors[idx][0] = color.red();
                newColors[idx][1] = color.green();
                newColors[idx][2] = color.blue();
            });
    qColorDialog->open();
}

void MainWindow::showDialog(const char *name)
{
    const menuitem *item = menu_findcommand(name);
    if (!item)
        return;

    const menudialog *dialog = menu_getdialog(uih, item);
    if (!dialog)
        return;

    int nitems;
    for (nitems = 0; dialog[nitems].question; nitems++)
        ;

    if (nitems == 1 &&
        (dialog[0].type == DIALOG_IFILE || dialog[0].type == DIALOG_OFILE)) {
        QString filter =
            QString("*.%1").arg(QFileInfo(dialog[0].defstr).completeSuffix());
        QSettings settings;
        QString fileLocation =
            settings.value("MainWindow/lastFileLocation", QDir::homePath())
                .toString();
        if (dialog[0].type == DIALOG_IFILE) {
            // fileName = QFileDialog::getOpenFileName(this, item->name, fileLocation, filter);
            QFileDialog *qFileDialog = new QFileDialog(this);
            // qFileDialog->setWindowTitle(dialog->question);
            qFileDialog->setWindowTitle(item->name);
            qFileDialog->setDirectory(fileLocation);
            qFileDialog->setNameFilter(filter);
            connect(qFileDialog, &QFileDialog::fileSelected,
                    [=](const QString &value) {
                        QString fileName = value;
                        if (!fileName.isNull()) {
                            QString ext = "." + QFileInfo(dialog[0].defstr).suffix();

                            if (!fileName.endsWith(".xpf") and !fileName.endsWith(".png")
                                and !fileName.endsWith(ext))
                                fileName += ext;

                            dialogparam *param = (dialogparam *)malloc(sizeof(dialogparam));
                            param->dstring = strdup(fileName.toUtf8());
                            menuActivate(item, param);
                            QSettings settings;
                            settings.setValue("MainWindow/lastFileLocation",
                                QFileInfo(fileName).absolutePath());
                        }
            });
            qFileDialog->open();
        }
        else if (dialog[0].type == DIALOG_OFILE) {
            char defname[256];
            strcpy(defname,
                   QDir(fileLocation).filePath(dialog[0].defstr).toUtf8());
            char *split = strchr(defname, '*');
            *split = 0;
            strcpy(defname, xio_getfilename(defname, split + 1));
            // fileName = QFileDialog::getSaveFileName(this, item->name, defname, filter);
            QFileDialog *qFileDialog = new QFileDialog(this);
            qFileDialog->setWindowTitle(item->name);
            qFileDialog->setDirectory(fileLocation);
            qFileDialog->setNameFilter(filter);
            qFileDialog->setAcceptMode(QFileDialog::AcceptSave);
            connect(qFileDialog, &QFileDialog::fileSelected,
                    [=](const QString &value) {
                        QString fileName = value;
                        if (!fileName.isNull()) {
                            QString ext = "." + QFileInfo(dialog[0].defstr).suffix();

                            if (!fileName.endsWith(".xpf") and !fileName.endsWith(".png")
                                and !fileName.endsWith(ext))
                                fileName += ext;

                        dialogparam *param = (dialogparam *)malloc(sizeof(dialogparam));
                        param->dstring = strdup(fileName.toUtf8());
                        menuActivate(item, param);
                        QSettings settings;
                        settings.setValue("MainWindow/lastFileLocation",
                              QFileInfo(fileName).absolutePath());
                        }
            });
            qFileDialog->open();
        }
    } else {

        dialogparam *param = (dialogparam *)malloc(sizeof(dialogparam));

        switch (dialog->type) {
            case DIALOG_INT:
            case DIALOG_FLOAT:
            case DIALOG_STRING:
            case DIALOG_KEYSTRING:
            {
                QInputDialog *qInputDialog = new QInputDialog(this);
                qInputDialog->setLabelText(dialog->question);
                qInputDialog->setWindowTitle(item->name);
                switch (dialog->type) {
                    case DIALOG_INT:
                    {
                        qInputDialog->setIntMaximum(1000000);
                        qInputDialog->setIntValue(dialog->defint);
                        connect(qInputDialog, &QInputDialog::intValueSelected, qInputDialog,
                            [=](const unsigned long int &value) {
                                param->dint = value;
                                menuActivate(item, param);
                            });
                        break;
                    }
                    case DIALOG_FLOAT:
                    {
                        qInputDialog->setDoubleMaximum(1000000);
                        qInputDialog->setDoubleValue(dialog->deffloat);
                        connect(qInputDialog, &QInputDialog::doubleValueSelected, qInputDialog,
                            [=](const double &value) {
                                param->number = value;
                                menuActivate(item, param);
                            });
                        break;
                    }
                    case DIALOG_STRING:
                    {
                        qInputDialog->setTextValue(dialog->defstr);
                        connect(qInputDialog, &QInputDialog::textValueSelected, qInputDialog,
                            [=](const QString &text) {
                                    param->dstring = strdup((char *) text.toStdString().c_str());
                                menuActivate(item, param);
                            });
                        break;
                    }
                    case DIALOG_KEYSTRING:
                    {
                        qInputDialog->setTextValue(dialog->defstr);
                        connect(qInputDialog, &QInputDialog::textValueSelected, qInputDialog,
                            [=](const QString &text) {
                                    param->dstring = strdup((char *) text.toStdString().c_str());
                                menuActivate(item, param);
                            });
                        break;
                    }
                }
                qInputDialog->open();
                break;
            }
            case DIALOG_COORD:
            {
                QDialog *qDialog = new QDialog(this);
                qDialog->setWindowTitle(item->name);
                QBoxLayout *dialogLayout = new QBoxLayout(QBoxLayout::TopToBottom, qDialog);
                QFormLayout *formLayout = new QFormLayout();
                QString label(dialog->question);
                QLineEdit *real = new QLineEdit(CustomDialog::format(dialog->deffloat), qDialog);
                QFontMetrics metric(real->font());
                real->setMinimumWidth(metric.horizontalAdvance(real->text()) * 1.1);
                real->setObjectName("real");
                QLineEdit *imag = new QLineEdit(CustomDialog::format(dialog->deffloat2), qDialog);
                imag->setObjectName("imag");
                imag->setMinimumWidth(metric.horizontalAdvance(imag->text()) * 1.1);
                // imag->setValidator(new QDoubleValidator(imag));
                QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->addWidget(real);
                layout->addWidget(new QLabel("+", qDialog));
                layout->addWidget(imag);
                layout->addWidget(new QLabel("i", qDialog));
                formLayout->addRow(label, layout);
                dialogLayout->addLayout(formLayout);
                QDialogButtonBox *buttonBox =
                    new QDialogButtonBox((QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal, qDialog);
                connect(buttonBox, SIGNAL(accepted()), qDialog, SLOT(accept()));
                connect(buttonBox, SIGNAL(rejected()), qDialog, SLOT(reject()));
                connect(real, SIGNAL(textEdited(QString)), qDialog, SLOT(update()));
                dialogLayout->addWidget(buttonBox);
                qDialog->setLayout(dialogLayout);
                connect(qDialog, &QDialog::accepted, qDialog,
                        [=](void){
                            QLineEdit *real = qDialog->findChild<QLineEdit *>("real");
                            param->dcoord[0] = real->text().toDouble();
                            QLineEdit *imag = qDialog->findChild<QLineEdit *>("imag");
                            param->dcoord[1] = imag->text().toDouble();
                            menuActivate(item, param);
                        });
                qDialog->adjustSize(); // this is sometimes too high in WASM, FIXME, maybe Qt6 bug?
                qDialog->open();
                break;
            }
            case DIALOG_PALSLIDER:
            {
                QDialog *qDialog = new QDialog(this);
                qDialog->setWindowTitle(item->name);
                QBoxLayout *dialogLayout = new QBoxLayout(QBoxLayout::TopToBottom, qDialog);
                gradientpal = clonepalette(uih->image->palette);
                uih_context *palcontext;
                palcontext = uih;
                // 3 inputs decide color, Algorithm Number, Seed and shift
                // For Algorithm number
                QSlider *seedslider, *algoslider, *shiftslider;
                algono = new QSpinBox(this);
                QString label(dialog->question);
                algono->setObjectName(label + "algono");
                algono->setValue(palcontext->palettetype);
                algono->setRange(1, 3);

                // Algo Slider
                algoslider = new QSlider(Qt::Horizontal, qDialog);
                algoslider->setObjectName(label + "-algono");
                algoslider->setRange(1, PALGORITHMS);
                algoslider->setValue(algono->value());
                // algoslider->setMinimumWidth(this->width()*2);

                // For Seed Number
                seedno = new QSpinBox(qDialog);
                seedno->setObjectName(label + "seedno");
                seedno->setRange(0, gradientpal->size);
                seedno->setValue(palcontext->paletteseed);

                // Seed Slider
                seedslider = new QSlider(Qt::Horizontal, qDialog);
                seedslider->setObjectName(label + "-seedno");
                seedslider->setRange(0, gradientpal->size);
                seedslider->setValue(seedno->value());

                // For Shift Number
                shiftno = new QSpinBox(this);
                shiftno->setObjectName(label + "shiftno");
                shiftno->setRange(0, gradientpal->size);
                shiftno->setValue(palcontext->paletteshift + palcontext->manualpaletteshift);

                // Shift Slider
                shiftslider = new QSlider(Qt::Horizontal, qDialog);
                shiftslider->setObjectName(label + "-shiftno");
                shiftslider->setRange(0, gradientpal->size);
                shiftslider->setValue(shiftno->value());

                // Add them to Layout
                QFormLayout *formLayout = new QFormLayout();
                formLayout->addRow("Algorithm", algono);
                formLayout->addWidget(algoslider);
                formLayout->addRow("Seed", seedno);
                formLayout->addWidget(seedslider);
                formLayout->addRow("Shift", shiftno);
                formLayout->addWidget(shiftslider);

                img = new QLabel(qDialog);
                img->setScaledContents(true);
                formLayout->addRow(img);
                updateVisualiser();

                connect(algono,SIGNAL(valueChanged(int)), algoslider, SLOT(setValue(int)));
                connect(algoslider, SIGNAL(valueChanged(int)), algono, SLOT(setValue(int)));
                connect(algono, SIGNAL(valueChanged(int)), this, SLOT(updateVisualiser()));
                connect(seedno,SIGNAL(valueChanged(int)), seedslider, SLOT(setValue(int)));
                connect(seedslider, SIGNAL(valueChanged(int)), seedno, SLOT(setValue(int)));
                connect(seedno, SIGNAL(valueChanged(int)), this, SLOT(updateVisualiser()));
                connect(shiftno,SIGNAL(valueChanged(int)), shiftslider, SLOT(setValue(int)));
                connect(shiftslider, SIGNAL(valueChanged(int)), shiftno, SLOT(setValue(int)));
                connect(shiftno, SIGNAL(valueChanged(int)), this, SLOT(updateVisualiser()));

                dialogLayout->addLayout(formLayout);
                QDialogButtonBox *buttonBox =
                    new QDialogButtonBox((QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal, qDialog);

                connect(buttonBox, SIGNAL(accepted()), qDialog, SLOT(accept()));
                connect(buttonBox, SIGNAL(rejected()), qDialog, SLOT(reject()));

                dialogLayout->addWidget(buttonBox);
                qDialog->setLayout(dialogLayout);

                connect(qDialog, &QDialog::accepted, qDialog,
                        [=](void){
                            QSlider *algo = qDialog->findChild<QSlider *>(label + "-algono");
                            palcontext->palettetype = algo->sliderPosition();
                            palcontext->manualpaletteshift = 0;
                            QSlider *seed = qDialog->findChild<QSlider *>(label + "-seedno");
                            palcontext->paletteseed = seed->sliderPosition();
                            QSlider *shift = qDialog->findChild<QSlider *>(label + "-shiftno");
                            palcontext->paletteshift = shift->sliderPosition();
                            param->dint = 1;
                            menuActivate(item, param);
                            destroypalette(gradientpal);
                        });
                qDialog->open();
                break;
            }
            case DIALOG_PALPICKER:
            {
                QDialog *qDialog = new QDialog(this);
                qDialog->setWindowTitle(item->name);
                QBoxLayout *dialogLayout = new QBoxLayout(QBoxLayout::TopToBottom, qDialog);

                uih_context *palcontext;
                palcontext = uih;
                getDEFSEGMENTColor(newColors);

                QList< QPushButton* > buttons;
                QBoxLayout *layout1 = new QBoxLayout(QBoxLayout::LeftToRight);
                QBoxLayout *layout2 = new QBoxLayout(QBoxLayout::LeftToRight);
                QBoxLayout *layout3 = new QBoxLayout(QBoxLayout::LeftToRight);
                for(auto bidx = 0; bidx < 31; ++bidx ) {
                        auto button = new QPushButton{ QString::number(bidx) };
                        button->setObjectName(QString::number(bidx));
                        QColor color(newColors[bidx][0], newColors[bidx][1], newColors[bidx][2]);
                        QPalette pal = button->palette();
                        button->setAutoFillBackground(true);
                        pal.setColor(QPalette::Button, color);
                        button->setPalette(pal);
                        button->update();
                        buttons << button;
                        if(bidx <= 10)
                    layout1->addWidget(button);
                        else if(bidx>10 and bidx <= 20)
                    layout2->addWidget(button);
                        else
                    layout3->addWidget(button);

                    connect(button, SIGNAL(clicked()), this, SLOT(colorPicker()));
                }
                QFormLayout *formLayout = new QFormLayout();
                formLayout->addRow(layout1);
                formLayout->addRow(layout2);
                formLayout->addRow(layout3);
                dialogLayout->addLayout(formLayout);
                QDialogButtonBox *buttonBox =
                    new QDialogButtonBox((QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal, qDialog);

                connect(buttonBox, SIGNAL(accepted()), qDialog, SLOT(accept()));
                connect(buttonBox, SIGNAL(rejected()), qDialog, SLOT(reject()));

                dialogLayout->addWidget(buttonBox);
                qDialog->setLayout(dialogLayout);

                connect(qDialog, &QDialog::accepted, qDialog,
                        [=](void){
                            mkcustompalette(palcontext->image->palette, newColors);
                            menuActivate(item, param);
                        });
                qDialog->open();
                break;
            }
            case DIALOG_LIST: // This is used only in Formulas/UserFormulas
            {
                QDialog *qDialog = new QDialog(this);
                qDialog->setWindowTitle(item->name);
                QBoxLayout *dialogLayout = new QBoxLayout(QBoxLayout::TopToBottom, qDialog);

                QComboBox *list = new QComboBox(this);
                QString label(dialog->question);
                list->setObjectName(label);
                list->setEditable(true);
                list->addItem(dialog->defstr);
                list->setObjectName(label + "-data");

                QSettings settings;
                QStringList formulas = settings.value("Formulas/UserFormulas").toStringList();

                for (int j = 0; j < formulas.count(); j++) {
                    bool found = false;
                    for (int i = 0; i < list->count(); i++) {
                        if (QString::compare(list->itemText(i), formulas[j], Qt::CaseSensitive) == 0) {
                            found = true;
                        }
                    }
                    if (!found)
                        list->addItem(formulas[j]);
                }

                // list->addItems(formulas);

                QFormLayout *formLayout = new QFormLayout();
                formLayout->addRow(label, list);
                dialogLayout->addLayout(formLayout);

                QDialogButtonBox *buttonBox =
                    new QDialogButtonBox((QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal, qDialog);

                connect(buttonBox, SIGNAL(accepted()), qDialog, SLOT(accept()));
                connect(buttonBox, SIGNAL(rejected()), qDialog, SLOT(reject()));
                dialogLayout->addWidget(buttonBox);
                qDialog->setLayout(dialogLayout);
                qDialog->adjustSize(); // this is sometimes too high in WASM, FIXME, maybe Qt6 bug?

                connect(qDialog, &QDialog::accepted, qDialog,
                        [=](void){
                            QComboBox *selected = qDialog->findChild<QComboBox *>(label + "-data");
                            param->dstring = strdup(selected->currentText().toUtf8());
                            menuActivate(item, param);
                        });
                qDialog->open();
                break;
            }
            case DIALOG_CHOICE:
            {
                QDialog *qDialog = new QDialog(this);
                qDialog->setWindowTitle(item->name);
                QBoxLayout *dialogLayout = new QBoxLayout(QBoxLayout::TopToBottom, qDialog);

                QComboBox *combo = new QComboBox(this);
                QString label(dialog->question);
                combo->setObjectName(label);

                const char **str = (const char **)dialog->defstr;
                for (int j = 0; str[j] != NULL; j++)
                    combo->addItem(str[j]);
                combo->setCurrentIndex(dialog->defint);
                QFormLayout *formLayout = new QFormLayout();
                formLayout->addRow(label, combo);
                dialogLayout->addLayout(formLayout);

                QDialogButtonBox *buttonBox =
                    new QDialogButtonBox((QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal, qDialog);

                connect(buttonBox, SIGNAL(accepted()), qDialog, SLOT(accept()));
                connect(buttonBox, SIGNAL(rejected()), qDialog, SLOT(reject()));
                dialogLayout->addWidget(buttonBox);
                qDialog->setLayout(dialogLayout);
                qDialog->adjustSize(); // this is sometimes too high in WASM, FIXME, maybe Qt6 bug?

                connect(qDialog, &QDialog::accepted, qDialog,
                        [=](void){
                            QComboBox *selected = qDialog->findChild<QComboBox *>(label);
                            param->dint = selected->currentIndex();
                            menuActivate(item, param);
                        });
                qDialog->open();
                break;
            }
            default:
            {
                CustomDialog customDialog(uih, item, dialog, this);
                if (customDialog.exec() == QDialog::Accepted)
                    menuActivate(item, customDialog.parameters());
            }
        }
    }
}

#ifdef __wasm
#define STATUS_VIA_STDOUT
//#define STATUS_VIA_PROGRESSBAR
#else
#define STATUS_VIA_PROGRESSBAR
// #define STATUS_VIA_WINDOWTITLE
#endif

QProgressDialog *qProgressDialog;
int progress = 0;
void MainWindow::showStatus(const char *text)
{
// This is not working properly, maybe because of a missing QTimer event/setting.
#ifdef STATUS_VIA_UIH_MESSAGE
    if (uih != NULL) {
        uih_message(uih, text);
        uih_updatestatus(uih);
    }
#endif

#ifdef STATUS_VIA_WINDOWTITLE
    if (strlen(text))
        setWindowTitle(
            QCoreApplication::applicationName().append(" - ").append(text));
    else
        setWindowTitle(QCoreApplication::applicationName());
#endif

#ifdef STATUS_VIA_STDOUT
    std::cout << "STATUS: " << text << "\n";
#endif

// This feature is experimental. It works natively but not in WASM.
#ifdef STATUS_VIA_PROGRESSBAR
    bool newProgress = (qProgressDialog == NULL);
    if (QString(text) == "") {
            if (!newProgress) {
                qProgressDialog->close();
                progress = 0;
                return;
            }
    }

    if (newProgress) {
        qProgressDialog = new QProgressDialog(this);
    } else {
        qProgressDialog->setValue(progress);
        qProgressDialog->setMinimumDuration(0);
        QString t = QString(text).trimmed();
        if (t.endsWith("%")) {
                progress = t.right(6).left(5).toDouble(); // save the percentage
                // std::cout << "t=" << t.toStdString() << " progress=" << progress << "\n";
                t = t.left(t.length()-6); // remove the percentage
        }

        qProgressDialog->setCancelButton(NULL);
        qProgressDialog->setWindowTitle(t);
        if (progress < 100) {
                progress++;
        }
        else {
                progress=0;
        }
    }

    if (newProgress) {
        qProgressDialog->show();
    }
#endif
}

int MainWindow::mouseButtons()
{
    int mouseButtons = 0;
    if (m_keyboardModifiers & Qt::ShiftModifier) {
        // Shift key makes left and right buttons emulate middle button
        if (m_mouseButtons & (Qt::LeftButton | Qt::RightButton))
            mouseButtons |= BUTTON2;
    } else {
        // Otherwise, mouse buttons map normally
        if (m_mouseButtons & Qt::LeftButton)
            mouseButtons |= BUTTON1;
        if (m_mouseButtons & Qt::MiddleButton)
            mouseButtons |= BUTTON2;
        if (m_mouseButtons & Qt::RightButton)
            mouseButtons |= BUTTON3;
    }
    // handle mouse wheel operations
    if (m_mouseWheel > 0)
        mouseButtons |= BUTTON1;
    if (m_mouseWheel < 0)
        mouseButtons |= BUTTON3;
    if (m_mouseWheel != 0) {
        timespec timenow;
        clock_gettime(CLOCK_REALTIME, &timenow);
        long elapsed = timenow.tv_sec * 1.0e9 + timenow.tv_nsec -
                       wheeltimer.tv_sec * 1.0e9 - wheeltimer.tv_nsec;
        if (elapsed > 1.0e9) // timing is hardcoded here
            m_mouseWheel = 0;
    }
    return mouseButtons;
}

int MainWindow::keyCombination() { return m_keyCombination; }

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    m_mouseButtons = event->buttons();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_mouseButtons = event->buttons();
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    m_mouseWheel = event->angleDelta().y();
    clock_gettime(CLOCK_REALTIME, &wheeltimer);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    m_keyboardModifiers = event->modifiers();

    switch (event->key()) {
        case Qt::Key_Left:
            m_keyCombination |= KEYLEFT;
            break;
        case Qt::Key_Right:
            m_keyCombination |= KEYRIGHT;
            break;
        case Qt::Key_Up:
            m_keyCombination |= KEYUP;
            break;
        case Qt::Key_Down:
            m_keyCombination |= KEYDOWN;
            break;
        default:
            if (!event->text().isEmpty())
                processKey(event->text().toUtf8()[0]);
            else
                event->ignore();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    m_keyboardModifiers = event->modifiers();

    switch (event->key()) {
        case Qt::Key_Left:
            m_keyCombination &= ~KEYLEFT;
            break;
        case Qt::Key_Right:
            m_keyCombination &= ~KEYRIGHT;
            break;
        case Qt::Key_Up:
            m_keyCombination &= ~KEYUP;
            break;
        case Qt::Key_Down:
            m_keyCombination &= ~KEYDOWN;
            break;
        default:
            event->ignore();
    }
}

#ifndef Q_OS_MACOS
#ifndef USE_OPENGL

void MainWindow::showFullScreen()
{
    menuBarRef->setParent(centralWidget());
    QMainWindow::showFullScreen();
}

void MainWindow::showNormal()
{
    setMenuBar(menuBarRef);
    menuBarRef->show();
    QMainWindow::showNormal();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (isFullScreen()) {
        if (event->pos().y() < menuBarRef->sizeHint().height())
            menuBarRef->show();
        else
            menuBarRef->hide();
    }
}
#endif
#endif

void MainWindow::resizeEvent(QResizeEvent * /*event*/)
{
#ifndef Q_OS_MACOS
#ifndef USE_OPENGL
    if (isFullScreen())
        menuBarRef->resize(size().width(), menuBarRef->sizeHint().height());
#endif
#endif
    shouldResize = true;
}
