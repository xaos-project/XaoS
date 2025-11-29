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

QPointF FractalWidget::mousePosition() {
    qreal dpr = devicePixelRatio();
    return QPointF(m_mousePosition.x() * dpr, m_mousePosition.y() * dpr);
}

void FractalWidget::setImage(struct image *image) { m_image = image; }

QSize FractalWidget::sizeHint() const { return QSize(800, 600); }

#ifdef USE_OPENGL
void FractalWidget::paintGL()
{
    if (m_image) {
        QImage *qimage =
            reinterpret_cast<QImage **>(m_image->data)[m_image->currimage];
        // QImage glimage = QGLWidget::convertToOpenGLFormat(*qimage);
        // glDrawPixels(glimage.width(), glimage.height(), GL_RGBA,
        //              GL_UNSIGNED_BYTE, glimage.bits());
        // For some reason, Qt 6 requires mirroring the image and using another color space.
        // The old convertToOpenGLFormat is no longer supported in Qt 6.
        QImage mirrored = qimage->mirrored(false, true);
        glDrawPixels(qimage->width(), qimage->height(), GL_BGRA_EXT,
                  GL_UNSIGNED_BYTE, mirrored.bits());
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
void FractalWidget::paintEvent(QPaintEvent */*event*/)
{
    if (m_image) {
        QPainter painter(this);
        QImage *qimage =
            reinterpret_cast<QImage **>(m_image->data)[m_image->currimage];
        painter.setCompositionMode(QPainter::CompositionMode_Source);

        // Scale the high-DPI image to fit the logical widget size
        QRect targetRect(0, 0, width(), height());
        painter.drawImage(targetRect, *qimage);
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
    m_mousePosition = event->position();
    event->ignore();
}
