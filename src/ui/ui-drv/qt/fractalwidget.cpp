#include "fractalwidget.h"

#include <QtGui>

FractalWidget::FractalWidget()
{
    m_image = 0;
    m_mouseButtons = 0;
}

void FractalWidget::paintEvent (QPaintEvent *event)
{
    if (m_image) {
        QPainter painter(this);
        painter.drawImage(0, 0, *m_image);
    }
}

void FractalWidget::mousePressEvent(QMouseEvent *event)
{
    m_mouseButtons = event->buttons();
}

void FractalWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_mouseButtons = event->buttons();
}

void FractalWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_mousePosition = event->pos();
}

void FractalWidget::wheelEvent(QWheelEvent *event)
{
}

void FractalWidget::drawImage(QImage *image)
{
    m_image = image;
    update();
}

QPoint FractalWidget::mousePosition()
{
    return m_mousePosition;
}

Qt::MouseButtons FractalWidget::mouseButtons()
{
    return m_mouseButtons;
}
