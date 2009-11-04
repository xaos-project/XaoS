#include <QtGui/QApplication>

#include "mainwindow.h"

#include "ui.h"
#include "filter.h"
#include "ui_helper.h"
#include "version.h"


MainWindow *window;

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("XaoS");
    QCoreApplication::setApplicationVersion(XaoS_VERSION);
    QCoreApplication::setOrganizationName("XaoS Project");
    QCoreApplication::setOrganizationDomain("xaos.sourceforge.net");

    QApplication a(argc, argv);
    MAIN_FUNCTION(argc, argv);
    //return a.exec();
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
    QCoreApplication::processEvents(wait ? QEventLoop::WaitForMoreEvents : QEventLoop::AllEvents);

    *mx = window->mousePosition().x();
    *my = window->mousePosition().y();
    *mb = window->mouseButtons();
    *k = window->keyCombination();
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
    window->showMessage(text);
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
qt_popupMenu (struct uih_context *uih, CONST char *name)
{
    window->popupMenu(uih, name);
}

static void
qt_toggleMenu (struct uih_context *uih, CONST char *name)
{
    window->toggleMenu(uih, name);
}

static void
qt_showPopUpMenu (struct uih_context *c, CONST char *name)
{
}

static void
qt_showDialog (struct uih_context *c, CONST char *name)
{
    window->showDialog(c, name);
}

static void
qt_showHelp (struct uih_context *c, CONST char *name)
{
}

int uih_message(uih_context * c, CONST char *message)
{
    printf(message);
    window->showMessage(message);
    return 0;
}

int uih_error(uih_context * c, CONST char *error)
{
    window->showError(error);
    return 0;
}

void uih_clearmessages(uih_context * c)
{
    //window->clearMessage();
}

void uih_initmessages(uih_context * c) {}
void uih_rmmessage(uih_context * c, int pid) {}
void uih_destroymessages(uih_context * c) {}
void uih_printmessages(uih_context * c) {}


struct gui_driver qt_gui_driver = {
/* setrootmenu */   qt_buildMenu,
/* enabledisable */ qt_toggleMenu,
/* menu */          qt_popupMenu,
/* dialog */        qt_showDialog,
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
