#include "fractalwidget.h"

#include <QtGui>
#ifdef USE_OPENGL
#include <QtOpenGL>
#endif
#include <QGestureEvent>
#include <QPinchGesture>
#include <QPanGesture>

#include "ui.h"
#include "filter.h"

FractalWidget::FractalWidget()
{
    m_image = NULL;
    setMouseTracking(true);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::TapGesture);
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

bool FractalWidget::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture) {
        QGestureEvent *gestureEvent = static_cast<QGestureEvent *>(event);
        if (QGesture *pinch = gestureEvent->gesture(Qt::PinchGesture)) {
            QPinchGesture *p = static_cast<QPinchGesture *>(pinch);
            if (p->state() == Qt::GestureStarted) {
                m_lastScale = 1.0;
            }
            if (p->state() == Qt::GestureUpdated) {
                // Qt's totalScaleFactor() is cumulative since the start of the gesture.
                // We compare it against our last recorded scale to detect changes.
                qreal totalScale = p->totalScaleFactor();
                qreal delta = totalScale / m_lastScale;

                // Threshold to avoid extreme jitter and capture intentional movement
                if (delta > 1.05 || delta < 0.95) {
                    m_mousePosition = p->centerPoint();
                    // Simulate mouse wheel for zooming
                    // 120/-120 are standard QWheelEvent ticks
                    int ticks = (delta > 1.0) ? 120 : -120;
                    
                    QWheelEvent wheel(p->centerPoint(), p->centerPoint(),
                                      QPoint(0, 0), QPoint(0, ticks),
                                      Qt::NoButton, Qt::NoModifier, Qt::ScrollUpdate, false);
                    QCoreApplication::sendEvent(parent(), &wheel);
                    m_lastScale = totalScale;
                }
            }
        } else if (QGesture *pan = gestureEvent->gesture(Qt::PanGesture)) {
            QPanGesture *p = static_cast<QPanGesture *>(pan);
            if (p->state() == Qt::GestureStarted) {
                m_lastPan = p->lastOffset();
                QMouseEvent press(QEvent::MouseButtonPress, p->lastOffset(), p->lastOffset(), p->lastOffset(),
                                  Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QCoreApplication::sendEvent(parent(), &press);
            }
            if (p->state() == Qt::GestureUpdated) {
                m_lastPan = p->offset();
                m_mousePosition = p->lastOffset();
                QMouseEvent move(QEvent::MouseMove, p->lastOffset(), p->lastOffset(), p->lastOffset(),
                                 Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QCoreApplication::sendEvent(parent(), &move);
            }
            if (p->state() == Qt::GestureFinished || p->state() == Qt::GestureCanceled) {
                QMouseEvent release(QEvent::MouseButtonRelease, p->lastOffset(), p->lastOffset(), p->lastOffset(),
                                    Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                QCoreApplication::sendEvent(parent(), &release);
            }
        }
        return true;
    }
    return QWidget::event(event);
}
