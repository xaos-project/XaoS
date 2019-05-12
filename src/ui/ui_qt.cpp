#include <QtWidgets>
#include <cstring>
#include <iostream>
#include <set>

#include "mainwindow.h"
#include "fractalwidget.h"

#include "ui.h"
#include "uiint.h"
#include "filter.h"
#include "grlib.h"
#include "ui_helper.h"
#include "version.h"

static struct params params[] = {
{NULL, 0, NULL, NULL}
};

struct ui_driver qt_driver = {
    /* name */          "Qt Driver",
    /* textwidth */     12,
    /* textheight */    12,
    /* params */        params,
    /* flags */         PIXELSIZE,
    /* width */         0.025,
    /* height */        0.025,
    /* maxwidth */      0,
    /* maxheight */     0,
    /* imagetype */     UI_TRUECOLOR,
    /* palettestart */  0,
    /* paletteend */    256,
    /* maxentries */    255,
    /* rmask */         0xff0000,
    /* gmask */         0x00ff00,
    /* bmask */         0x0000ff
};

MainWindow *window;
FractalWidget *widget;

int
main(int argc, char *argv[])
{
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

int
qt_init()
{
    window = new MainWindow();
    widget = window->fractalWidget();
    window->show();

    QScreen *screen = window->windowHandle()->screen();
    qt_driver.width = 2.54 / screen->physicalDotsPerInchX();
    qt_driver.height = 2.54 / screen->physicalDotsPerInchY();
    printf("pixelsize: %fx%f\n", qt_driver.width, qt_driver.height);

    return 1;
}

void
qt_uninit()
{
    delete window;
}

int
qt_alloc_buffers (char **b1, char **b2, void **data)
{
    widget->createImages();
    *b1 = widget->imageBuffer1();
    *b2 = widget->imageBuffer2();
    *data = widget->imagePointer();
    return widget->imageBytesPerLine();
}

void
qt_free_buffers(char *b1, char *b2)
{
    widget->destroyImages();
}

void
qt_getsize(int *w, int *h)
{
    *w = widget->size().width();
    *h = widget->size().height();
}

void
qt_flip_buffers()
{
    widget->switchActiveImage();
}

void
qt_display()
{
    widget->repaint();
}

void
qt_processevents(int wait, int *mx, int *my, int *mb, int *k)
{
    QCoreApplication::processEvents(wait ? QEventLoop::WaitForMoreEvents : QEventLoop::AllEvents);

    *mx = widget->mousePosition().x();
    *my = widget->mousePosition().y();
    *mb = widget->mouseButtons();
    *k = widget->keyCombination();
}

void
qt_getmouse(int *x, int *y, int *b)
{
    *x = widget->mousePosition().x();
    *y = widget->mousePosition().y();
    *b = widget->mouseButtons();
}

void
qt_print(int x, int y, const char *text)
{
}

void
qt_mousetype(int type)
{
    widget->setCursorType(type);
}

void
qt_setrootmenu(struct uih_context *uih, const char *name)
{
    window->buildMenu(uih, name);
}

void
qt_enabledisable(struct uih_context *uih, const char *name)
{
    window->toggleMenu(uih, name);
}

void
qt_menu(struct uih_context *uih, const char *name)
{
    window->popupMenu(uih, name);
}

void
qt_builddialog(struct uih_context *c, const char *name)
{
    window->showDialog(c, name);
}

void
qt_help(struct uih_context *c, const char *name)
{
    QDesktopServices::openUrl(QUrl(HELP_URL));
}

const char *
qt_locale()
{
    return QLocale::system().name().toStdString().c_str();
}

const char *
qt_gettext(char *text)
{
    static std::map<char *, char *> strings;
    char *trans = strings[text];
    if (trans == NULL) {
        trans = strdup(QCoreApplication::translate("", text).toStdString().c_str());
        strings[text] = trans;
    }
    return trans;
}

}
