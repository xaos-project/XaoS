#include <QtWidgets>
#include <cstring>

#include "mainwindow.h"
#include "fractalwidget.h"

#include "ui.h"
#include "filter.h"
#include "grlib.h"
#include "ui_helper.h"
#include "version.h"


MainWindow *window;
FractalWidget *widget;

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("XaoS");
    QCoreApplication::setApplicationVersion(XaoS_VERSION);
    QCoreApplication::setOrganizationName("XaoS Project");
    QCoreApplication::setOrganizationDomain("xaos.sourceforge.net");

    QApplication a(argc, argv);

    //return a.exec();
    return MAIN_FUNCTION(argc, argv);
}

static int
qt_initDriver ()
{
    window = new MainWindow();
    widget = window->fractalWidget();
    window->show();
    return 1;
}

static void
qt_uninitDriver ()
{
    delete window;
}

static int
qt_allocBuffers (char **b1, char **b2, void **data)
{
    widget->createImages();
    *b1 = widget->imageBuffer1();
    *b2 = widget->imageBuffer2();
    *data = widget->imagePointer();
    return widget->imageBytesPerLine();
}

static void
qt_freeBuffers (char *b1, char *b2)
{
    widget->destroyImages();
}

static void
qt_getImageSize (int *w, int *h)
{
    *w = widget->size().width();
    *h = widget->size().height();
}

static void
qt_flipBuffers ()
{
    widget->switchActiveImage();
}

static void
qt_redrawImage()
{
    widget->repaint();
}

static void
qt_processEvents (int wait, int *mx, int *my, int *mb, int *k)
{
    QCoreApplication::processEvents(wait ? QEventLoop::WaitForMoreEvents : QEventLoop::AllEvents);

    *mx = widget->mousePosition().x();
    *my = widget->mousePosition().y();
    *mb = widget->mouseButtons();
    *k = widget->keyCombination();
}

static void
qt_getMouse (int *x, int *y, int *b)
{
    *x = widget->mousePosition().x();
    *y = widget->mousePosition().y();
    *b = widget->mouseButtons();
}

static void
qt_printText(int x, int y, const char *text)
{
    window->showMessage(text);
}

static void
qt_setCursorType (int type)
{
    widget->setCursorType(type);
}

static void
qt_buildMenu (struct uih_context *uih, const char *name)
{
    window->buildMenu(uih, name);
}

static void
qt_popupMenu (struct uih_context *uih, const char *name)
{
    window->popupMenu(uih, name);
}

static void
qt_toggleMenu (struct uih_context *uih, const char *name)
{
    window->toggleMenu(uih, name);
}

static void
qt_showDialog (struct uih_context *c, const char *name)
{
    window->showDialog(c, name);
}

static void
qt_showHelp (struct uih_context *c, const char *name)
{
}

QFont qt_getFont() {
    return QFont(QApplication::font().family(), 12);
    //return QFont("Calibri", 12);
}

int qt_imagePrint(struct image *image, int x, int y,
                   const char *text, int fgcolor, int bgcolor, int mode)
{
    char line[BUFSIZ];
    int pos = strcspn(text, "\n");
    strncpy(line, text, pos);
    line[pos] = '\0';

    QImage *qimage = reinterpret_cast<QImage **>(image->data)[image->currimage];
    QFontMetrics metrics(qt_getFont(), qimage);
    QPainter painter(qimage);
    painter.setFont(qt_getFont());

    if (mode == TEXT_PRESSED) {
        painter.setPen(fgcolor);
        painter.drawText(x + 1, y + 1 + metrics.ascent(), line);
    } else {
        painter.setPen(bgcolor);
        painter.drawText(x + 1, y + 1 + metrics.ascent(), line);
        painter.setPen(fgcolor);
        painter.drawText(x, y + metrics.ascent(), line);
    }

    return strlen(line);
}

int qt_imageTextWidth(struct image *image, const char *text)
{
    char line[BUFSIZ];
    int pos = strcspn(text, "\n");
    strncpy(line, text, pos);
    line[pos] = '\0';

    QImage *qimage = reinterpret_cast<QImage **>(image->data)[image->currimage];
    QFontMetrics metrics(qt_getFont(), qimage);

    return metrics.width(line) + 1;
}

int qt_imageTextHeight(struct image *image)
{
    QImage *qimage = reinterpret_cast<QImage **>(image->data)[image->currimage];
    QFontMetrics metrics(qt_getFont(), qimage);

    return metrics.height() + 1;
}

int qt_imageCharWidth(struct image *image, const char c)
{
    QImage *qimage = reinterpret_cast<QImage **>(image->data)[image->currimage];
    QFontMetrics metrics(qt_getFont(), qimage);

    return metrics.width(c);
}

const char *qt_saveImage(struct image *image, const char *filename)
{
    QImage *qimage = reinterpret_cast<QImage **>(image->data)[image->currimage];
    qimage->save(filename);
    return NULL;
}

struct gui_driver qt_gui_driver = {
/* setrootmenu */   qt_buildMenu,
/* enabledisable */ qt_toggleMenu,
/* menu */          qt_popupMenu,
/* dialog */        qt_showDialog,
/* help */          qt_showHelp
};

struct image_driver qt_image_driver =
{
/* print */      qt_imagePrint,
/* textwidth */  qt_imageTextWidth,
/* textheight */ qt_imageTextHeight,
/* charwidth */  qt_imageCharWidth,
/* saveimage */  qt_saveImage
};

static struct params qt_params[] = {
{NULL, 0, NULL, NULL}
};

extern "C" {

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
/* gui_driver */    &qt_gui_driver,
                    &qt_image_driver
};

}
