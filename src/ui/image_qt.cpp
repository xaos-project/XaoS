#include <QtWidgets>

#include "config.h"
#include "filter.h"
#include "grlib.h"
#include "xio.h"
#include "misc-f.h"

static QFont getFont(void *font) {
    if (font)
        return *reinterpret_cast<QFont *>(font);
    else
        return QFont(QApplication::font().family(), 12);
}

int xprint(struct image *image, void *font, int x, int y,
           const char *text, int fgcolor, int bgcolor, int mode)
{
    char line[BUFSIZ];
    int pos = strcspn(text, "\n");
    strncpy(line, text, pos);
    line[pos] = '\0';

    QImage *qimage = reinterpret_cast<QImage **>(image->data)[image->currimage];
    QFontMetrics metrics(getFont(font), qimage);
    QPainter painter(qimage);
    painter.setFont(getFont(font));

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

int xtextwidth(struct image */*image*/, void *font, const char *text)
{
    char line[BUFSIZ];
    int pos = strcspn(text, "\n");
    strncpy(line, text, pos);
    line[pos] = '\0';

    QFontMetrics metrics(getFont(font));
    return metrics.width(line) + 1;
}

int xtextheight(struct image */*image*/, void *font)
{
    QFontMetrics metrics(getFont(font));
    return metrics.height() + 1;
}

int xtextcharw(struct image */*image*/, void *font, const char c)
{
    QFontMetrics metrics(getFont(font));
    return metrics.width(c);
}

// Saves image as png with xpf chunk data
const char *writepng(xio_constpath filename, const struct image *image, xio_file xpf_data)
{
    QImage *qimage = reinterpret_cast<QImage **>(image->data)[image->currimage];
    if(xpf_data != NULL){
        QString xpf_chunk = xio_getstring(xpf_data);
        qimage->setText("Metadata", xpf_chunk);
    }
    qimage->save(filename);
    return NULL;
}

// Reads png image and xpf associated data
const char* readpng(xio_constpath filename)
{
    QImageReader reader(filename);
    const QImage xaos_image = reader.read();
    QString xpf_chunk = xaos_image.text("Metadata");
    const char *xpf_data = NULL;
    if(xpf_chunk != NULL or !xpf_chunk.isEmpty())
        xpf_data = mystrdup(xpf_chunk.toStdString().c_str());
    return xpf_data;
}

static void freeImage(struct image *img)
{
    free(img);
}

struct image *create_image_qt(int width, int height, struct palette *palette,
                              float pixelwidth, float pixelheight)
{
    QImage **data = new QImage *[2];
    data[0] = new QImage(width, height, QImage::Format_RGB32);
    data[1] = new QImage(width, height, QImage::Format_RGB32);
    struct image *img = create_image_cont(
        width, height, data[0]->bytesPerLine(), 2, data[0]->bits(),
        data[1]->bits(), palette, NULL, 0, pixelwidth, pixelheight);
    if (!img) {
        delete data[0];
        delete data[1];
        delete[] data;
        return NULL;
    }
    img->data = data;
    img->free = freeImage;
    return img;
}

void overlayGrid(uih_context *c, int fgcolor)
{
    struct image* image = c->image;
    QImage *qimage = reinterpret_cast<QImage **>(image->data)[image->currimage];
    QPainter painter(qimage);
    QPen pen;
    pen.setColor(fgcolor);
    pen.setWidth(2);
    painter.setPen(pen);

    //Find fractal origin (0,0)
    long long int x1 = (0 - c->fcontext->rs.nc) /
        (c->fcontext->rs.mc - c->fcontext->rs.nc) *
        c->zengine->image->width;
    long long int y1 = (0 - c->fcontext->rs.ni) /
        (c->fcontext->rs.mi - c->fcontext->rs.ni) *
        c->zengine->image->height;

    /* FIXME Support greater zoom*/
    double currzoom =
        (c->fcontext->currentformula->v.rr) / (c->fcontext->s.rr);
    if(currzoom > 100000){
        uih_error(c, "Cartesian Grid not supported on zoom > 100000x");
        uih_message(c, "Re-enable after zooming out");
        uih_cartesiangrid(c);
    }

    // Find next coordinate (1,1)
    long long int x2 = (1 - c->fcontext->rs.nc) /
        (c->fcontext->rs.mc - c->fcontext->rs.nc) *
        c->zengine->image->width;
    long long int y2 = (1 - c->fcontext->rs.ni) /
        (c->fcontext->rs.mi - c->fcontext->rs.ni) *
        c->zengine->image->height;

    // Find current zoom level
    long double rr = c->fcontext->s.rr/10.0;
    long double counter=0;
    while(rr<1){
        rr*=10;
        counter++;
    }

    // Set step size
    long double xinterval = x2-x1;
    long double yinterval = y2-y1;
    long double xstep = xinterval/pow(10.0, counter - 1);
    long double ystep = yinterval/pow(10.0, counter - 1);

    // Do Not draw smaller coordinates if step size is too low
    // Draw Boundary Boxes
    if(xstep > 1 and ystep > 1){
        for(long double i=x1; i<=image->width; i+=xstep*10){
            painter.drawLine(i, 0, i, image->height);
        }
        for(long double i=x1; i>=0; i-=xstep*10){
            painter.drawLine(i, 0, i, image->height);
        }
        for(long double i=y1; i<=image->height; i+=ystep*10){
            painter.drawLine(0, i, image->width, i);
        }
        for(long double i=y1; i>=0; i-=ystep*10){
            painter.drawLine(0, i, image->width, i);
        }
    }

    pen.setWidth(1);
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);

    // Draw grid boxes
    if(xstep > 1 and ystep > 1){
        for(long double i=x1; i<=image->width; i+=xstep){
            painter.drawLine(i, 0, i, image->height);
        }
        for(long double i=x1; i>=0; i-=xstep){
            painter.drawLine(i, 0, i, image->height);
        }
        for(long double i=y1; i<=image->height; i+=ystep){
            painter.drawLine(0, i, image->width, i);
        }
        for(long double i=y1; i>=0; i-=ystep){
            painter.drawLine(0, i, image->width, i);
        }
    }
    return;
}
