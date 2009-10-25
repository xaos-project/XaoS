#include "fractalmodel.h"

#include <QtGui>

#include <config.h>
#include <filter.h>
#include <formulas.h>
#include <plane.h>
#include <ui_helper.h>

int uih_message(uih_context * c, CONST char *message)
{
    FractalModel *m = reinterpret_cast<FractalModel*>(c->data);
    m->showMessage(QString(message));
    return 0;
}

int uih_error(uih_context * c, CONST char *error)
{
    FractalModel *m = reinterpret_cast<FractalModel*>(c->data);
    m->showError(QString(error));
    return 0;
}

void uih_clearmessages(uih_context * c)
{
    FractalModel *m = reinterpret_cast<FractalModel*>(c->data);
    m->clearMessage();
}

void uih_initmessages(uih_context * c) {}
void uih_destroymessages(uih_context * c) {}
void uih_printmessages(uih_context * c) {}

FractalModel::FractalModel()
{
    m_mouseX = m_mouseY = m_mouseButtons = 0;

    for (int i = 0; i < nformulas; i++) {
        m_formulas += ::formulas[i].name[!::formulas[i].mandelbrot];
    }

    for (int i = 0; i < OUTCOLORING - 1; i++) {
        m_exteriorColors += outcolorname[i];
    }

    for (int i = 0; i < INCOLORING - 1; i++) {
        m_interiorColors += incolorname[i];
    }

    for (int i = 0; i < TCOLOR - 1; i++) {
        m_exteriorColors += tcolorname[i];
        m_interiorColors += tcolorname[i];
    }

    for (int i = 0; planename[i] != NULL; i++) {
        m_planes += planename[i];
    }

    m_uih = uih_mkcontext(0, createImage(QSize(640, 480)), NULL, NULL, NULL);
    m_uih->data = this;
    //m_uih->fcontext->version++;
    //uih_loadcatalog(m_uih, "english");
    uih_newimage(m_uih);

    QTimer::singleShot(0, this, SLOT(updateFractal()));
}

FractalModel::~FractalModel()
{
    destroypalette(m_uih->image->palette);
    destroy_image(m_uih->image);
    uih_freecontext(m_uih);
}


const QImage & FractalModel::image() {
    return m_image[m_uih->image->currimage];
}

void FractalModel::setSize(const QSize &size)
{
    uih_interrupt(m_uih);
    uih_clearwindows(m_uih);
    uih_stoptimers(m_uih);
    uih_cycling_stop(m_uih);
    uih_savepalette(m_uih);
    if (size.width() != m_uih->image->width || size.height() != m_uih->image->height) {
        destroy_image(m_uih->image);
        destroypalette(m_uih->palette);

        if (!uih_updateimage(m_uih, createImage(size))) {
            /*
            driver->uninit();
            x_error(gettext("Can not allocate tables"));
            ui_outofmem();
            exit_xaos(-1);
            */
        }
        tl_process_group(syncgroup, NULL);
        //tl_reset_timer(maintimer);
        //tl_reset_timer(arrowtimer);
        uih_newimage(m_uih);
    }
    uih_newimage(m_uih);
    uih_restorepalette(m_uih);
    /*uih_mkdefaultpalette(m_uih); */
    m_uih->display = 1;
    uih_cycling_continue(m_uih);
}

struct image *FractalModel::createImage(const QSize &size)
{
    m_image[0] = QImage(size.width(), size.height(), QImage::Format_RGB32);
    m_image[1] = QImage(size.width(), size.height(), QImage::Format_RGB32);

    union paletteinfo info;
    info.truec.rmask = 0xff0000;
    info.truec.gmask = 0x00ff00;
    info.truec.bmask = 0x0000ff;

    struct palette *pal = createpalette (0, 0, TRUECOLOR, 0, 0, NULL,
                                         NULL, NULL, NULL, &info);

    struct image *img =
    create_image_cont(m_image[0].width(), m_image[0].height(),
                      m_image[0].bytesPerLine(), 2,
                      m_image[0].bits(),
                      m_image[1].bits(),
                      pal, NULL,
                      0,
                      0.01,
                      0.01);

    return img;
}

void FractalModel::loadFile(const QString &fileName)
{
    uih_loadfile(m_uih, fileName.toUtf8());
}

bool FractalModel::saveFile(const QString &fileName)
{
    uih_saveposfile(m_uih, fileName.toUtf8());

    if (m_uih->errstring == NULL)
        return true;
    else
        return false;
}

void FractalModel::showMessage(const QString &message)
{
    printf(message.toAscii());
    emit messageChanged(message);
}

void FractalModel::showError(const QString &error)
{
    emit errorEncountered(error);
}

void FractalModel::clearMessage()
{
    emit messageCleared();
}

void FractalModel::updateFractal()
{
    uih_prepare_image(m_uih);
    uih_drawwindows(m_uih);
    emit imageChanged();
    uih_cycling_continue(m_uih);
    uih_displayed(m_uih);
    uih_update(m_uih, m_mouseX, m_mouseY, m_mouseButtons);
    int time = tl_process_group(syncgroup, NULL);
    QTimer::singleShot(0, this, SLOT(updateFractal()));
}

void FractalModel::copy()
{
    /*
    QMimeData *mimeData = new QMimeData;
    mimeData->setText(QString(uih_savepostostr(m_uih)));
    mimeData->setImageData(m_image[m_uih->image->currimage]);
    QApplication::clipboard()->setMimeData(mimeData);
    */
}

void FractalModel::paste()
{
    /*
    uih_loadstr(m_uih, QApplication::clipboard()->text().toAscii());
    */
}

void FractalModel::processEvent(QMouseEvent *event)
{
    m_mouseX = event->x();
    m_mouseY = event->y();
    switch (event->type()) {
        case QEvent::MouseButtonPress:
            if (event->buttons() & Qt::LeftButton)
                m_mouseButtons |= BUTTON1;
            if (event->buttons() & Qt::MidButton)
                m_mouseButtons |= BUTTON2;
            if (event->buttons() & Qt::RightButton)
                m_mouseButtons |= BUTTON3;
            break;

        case QEvent::MouseButtonRelease:
            if (!(event->buttons() & Qt::LeftButton))
                m_mouseButtons &= ~BUTTON1;
            if (!(event->buttons() & Qt::MidButton))
                m_mouseButtons &= ~BUTTON2;
            if (!(event->buttons() & Qt::RightButton))
                m_mouseButtons &= ~BUTTON3;
            break;

        default:
            break;
    }
}

void FractalModel::processEvent(QWheelEvent *event)
{
    // TODO: handle wheel events
}

const QStringList &FractalModel::formulas()
{
    return m_formulas;
}

void FractalModel::setFormula(int index)
{
    uih_interrupt(m_uih);
    uih_setformula(m_uih, index);
}

int FractalModel::formula()
{
    return (m_uih->fcontext->currentformula - ::formulas);
}

const QStringList &FractalModel::exteriorColors()
{
    return m_exteriorColors;
}

void FractalModel::setExteriorColor(int index)
{
    uih_interrupt(m_uih);
    if (index < (OUTCOLORING - 1)) {
        uih_setoutcoloringmode(m_uih, index);
    } else {
        uih_setoutcoloringmode(m_uih, (OUTCOLORING - 1));
        uih_setouttcolor(m_uih, index - (OUTCOLORING - 1));
    }
}

int FractalModel::exteriorColor()
{
    return m_uih->fcontext->coloringmode + m_uih->fcontext->outtcolor;
}

const QStringList &FractalModel::interiorColors()
{
    return m_interiorColors;
}

void FractalModel::setInteriorColor(int index)
{
    uih_interrupt(m_uih);
    if (index < (INCOLORING - 1)) {
        uih_setincoloringmode(m_uih, index);
    } else {
        uih_setincoloringmode(m_uih, (INCOLORING - 1));
        uih_setintcolor(m_uih, index - (INCOLORING - 1));
    }
}

int FractalModel::interiorColor()
{
    return m_uih->fcontext->incoloringmode + m_uih->fcontext->intcolor;
}

const QStringList &FractalModel::planes()
{
    return m_planes;
}

void FractalModel::setPlane(int index)
{
    uih_interrupt(m_uih);
    uih_setplane(m_uih, index);
}

int FractalModel::plane()
{
    return m_uih->fcontext->plane;
}

QPointF FractalModel::perturbation()
{
    return QPointF(m_uih->fcontext->bre, m_uih->fcontext->bim);
}

void FractalModel::setPerturbation(const QPointF &perturbation)
{
    uih_interrupt(m_uih);
    uih_setperbutation(m_uih, (number_t)(perturbation.x()), (number_t)(perturbation.y()));
}

QPointF FractalModel::center()
{
    return QPointF(m_uih->fcontext->s.cr, m_uih->fcontext->s.ci);
}

void FractalModel::setCenter(const QPointF &center)
{
    uih_interrupt(m_uih);
    m_uih->fcontext->s.cr = center.x();
    m_uih->fcontext->s.ci = center.y();
}

