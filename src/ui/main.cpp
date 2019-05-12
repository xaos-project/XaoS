#include <QtWidgets>
#include <cstring>
#include <iostream>
#include <set>

#include "mainwindow.h"
#include "fractalwidget.h"

#include "ui.h"
#include "filter.h"
#include "grlib.h"
#include "ui_helper.h"
#include "version.h"

static struct params params[] = {
{NULL, 0, NULL, NULL}
};

static int initDriver();
static void getImageSize(int *w, int *h);
static void processEvents(int wait, int *mx, int *my, int *mb, int *k);
static void getMouse(int *x, int *y, int *b);
static void uninitDriver();
static void printText(int x, int y, const char *text);
static void redrawImage();
static int allocBuffers (char **b1, char **b2, void **data);
static void freeBuffers(char *b1, char *b2);
static void flipBuffers();
static void setCursorType(int type);

struct ui_driver qt_driver = {
    /* name */          "Qt Driver",
    /* init */          initDriver,
    /* getsize */       getImageSize,
    /* processevents */ processEvents,
    /* getmouse */      getMouse,
    /* uninit */        uninitDriver,
    /* set_color */     NULL,
    /* set_range */     NULL,
    /* print */         printText,
    /* display */       redrawImage,
    /* alloc_buffers */ allocBuffers,
    /* free_buffers */  freeBuffers,
    /* filp_buffers */  flipBuffers,
    /* mousetype */     setCursorType,
    /* flush */         NULL,
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
    return MAIN_FUNCTION(argc, argv);
}

static int
initDriver()
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

static void
uninitDriver()
{
    delete window;
}

static int
allocBuffers (char **b1, char **b2, void **data)
{
    widget->createImages();
    *b1 = widget->imageBuffer1();
    *b2 = widget->imageBuffer2();
    *data = widget->imagePointer();
    return widget->imageBytesPerLine();
}

static void
freeBuffers(char *b1, char *b2)
{
    widget->destroyImages();
}

static void
getImageSize(int *w, int *h)
{
    *w = widget->size().width();
    *h = widget->size().height();
}

static void
flipBuffers()
{
    widget->switchActiveImage();
}

static void
redrawImage()
{
    widget->repaint();
}

static void
processEvents(int wait, int *mx, int *my, int *mb, int *k)
{
    QCoreApplication::processEvents(wait ? QEventLoop::WaitForMoreEvents : QEventLoop::AllEvents);

    *mx = widget->mousePosition().x();
    *my = widget->mousePosition().y();
    *mb = widget->mouseButtons();
    *k = widget->keyCombination();
}

static void
getMouse(int *x, int *y, int *b)
{
    *x = widget->mousePosition().x();
    *y = widget->mousePosition().y();
    *b = widget->mouseButtons();
}

static void
printText(int x, int y, const char *text)
{
}

static void
setCursorType(int type)
{
    widget->setCursorType(type);
}

extern "C" {

void
ui_setrootmenu(struct uih_context *uih, const char *name)
{
    window->buildMenu(uih, name);
}

void
ui_enabledisable(struct uih_context *uih, const char *name)
{
    window->toggleMenu(uih, name);
}

void
ui_menu(struct uih_context *uih, const char *name)
{
    window->popupMenu(uih, name);
}

void
ui_builddialog(struct uih_context *c, const char *name)
{
    window->showDialog(c, name);
}

void
ui_help(struct uih_context *c, const char *name)
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
