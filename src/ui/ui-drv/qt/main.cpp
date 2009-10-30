#include "ui.h"

#include <QtGui/QApplication>
#include "mainwindow.h"

MainWindow *window;

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("XaoS");
    QCoreApplication::setApplicationVersion("4.0");
    QCoreApplication::setOrganizationName("XaoS");
    QCoreApplication::setOrganizationDomain("xaos.sourceforge.net");

    QApplication a(argc, argv);
    ui_init(argc, argv);

    if (window)
        window->startMainLoop();

    return a.exec();
}

static int
qt_initDriver ()
{
    window = new MainWindow();
    window->show();
    return 1;
}

static void
qt_uninitDriver ()
{
    delete window;
}

static int
qt_allocBuffers (char **b1, char **b2)
{
    window->createImages();
    *b1 = window->imageBuffer1();
    *b2 = window->imageBuffer2();
    return window->imageBytesPerLine();
}

static void
qt_freeBuffers (char *b1, char *b2)
{
    window->destroyImages();
}

static void
qt_getImageSize (int *w, int *h)
{
    *w = window->imageSize().width();
    *h = window->imageSize().height();
}

static void
qt_flipBuffers ()
{
    window->switchActiveImage();
}

static void
qt_redrawImage()
{
    window->redrawImage();
}

static void
qt_processEvents (int wait, int *mx, int *my, int *mb, int *k)
{
    //QCoreApplication::processEvents();

    *mx = window->mousePosition().x();
    *my = window->mousePosition().y();
    *mb = window->mouseButtons();
    *k = 0;
}

static void
qt_getMouse (int *x, int *y, int *b)
{
    *x = window->mousePosition().x();
    *y = window->mousePosition().y();
    *b = window->mouseButtons();
}

static void
qt_printText(int x, int y, CONST char *text)
{
    window->showMessage(QString(text));
}

static void
qt_setCursorType (int type)
{
    window->setCursorType(type);
}

static void
qt_buildMenu (struct uih_context *uih, CONST char *name)
{
    window->buildMenu(uih, name);
}

static void
qt_showPopUpMenu (struct uih_context *c, CONST char *name)
{
}

static void
qt_showDialog (struct uih_context *c, CONST char *name)
{
}

static void
qt_showHelp (struct uih_context *c, CONST char *name)
{
}

struct gui_driver qt_gui_driver = {
/* setrootmenu */   qt_buildMenu,
/* enabledisable */ NULL,
/* menu */          NULL,
/* dialog */        NULL,
/* help */          NULL
};

static struct params qt_params[] = {
{NULL, 0, NULL, NULL}
};

struct ui_driver qt_driver = {
/* name */          "Qt Driver",
/* init */          qt_initDriver,
/* getsize */       qt_getImageSize,
/* processevents */ qt_processEvents,
/* getmouse */      qt_getMouse,
/* uninit */        qt_uninitDriver,
/* set_color */     NULL,
/* set_range */     NULL,
/* print */         qt_printText,
/* display */       qt_redrawImage,
/* alloc_buffers */ qt_allocBuffers,
/* free_buffers */  qt_freeBuffers,
/* filp_buffers */  qt_flipBuffers,
/* mousetype */     qt_setCursorType,
/* flush */         NULL,
/* textwidth */     12,
/* textheight */    12,
/* params */        qt_params,
/* flags */         PIXELSIZE,
/* width */         0.01,
/* height */        0.01,
/* maxwidth */      0,
/* maxheight */     0,
/* imagetype */     UI_TRUECOLOR,
/* palettestart */  0,
/* paletteend */    256,
/* maxentries */    255,
/* rmask */         0xff0000,
/* gmask */         0x00ff00,
/* bmask */         0x0000ff,
/* gui_driver */    &qt_gui_driver
};
