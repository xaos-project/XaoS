#include "fractalwidget.h"

#include <QtGui>
#ifdef USE_OPENGL
#include <QtOpenGL>
#endif

#include "ui.h"
#include "filter.h"

FractalWidget::FractalWidget()
{
    m_image = NULL;
    setMouseTracking(true);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
}

QPoint FractalWidget::mousePosition() { return m_mousePosition; }

void FractalWidget::setImage(struct image *image) { m_image = image; }

QSize FractalWidget::sizeHint() const { return m_sizeHint; }

void FractalWidget::setSizeHint(const QSize &size) { m_sizeHint = size; }

#ifdef USE_OPENGL
void FractalWidget::paintGL()
{
    if (m_image) {
        QImage *qimage =
            reinterpret_cast<QImage **>(m_image->data)[m_image->currimage];
        QImage glimage = QGLWidget::convertToGLFormat(*qimage);
        glDrawPixels(glimage.width(), glimage.height(), GL_RGBA,
                     GL_UNSIGNED_BYTE, glimage.bits());
    }
}

void FractalWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}
#else
void FractalWidget::paintEvent(QPaintEvent *event)
{
    if (m_image) {
        QPainter painter(this);
        QImage *qimage =
            reinterpret_cast<QImage **>(m_image->data)[m_image->currimage];
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.drawImage(0, 0, *qimage);
    }
}
#endif

void FractalWidget::mousePressEvent(QMouseEvent *event)
{
    m_mousePosition = event->pos();
    event->ignore();
}

void FractalWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_mousePosition = event->pos();
    event->ignore();
}

void FractalWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_mousePosition = event->pos();
    event->ignore();
}

void FractalWidget::wheelEvent(QWheelEvent *event)
{
    m_mousePosition = event->pos();
    event->ignore();
}
