#include "fractalwidget.h"

#include <QtGui>

FractalWidget::FractalWidget()
{
    m_image = 0;
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
    emit mouseChanged(event);
}

void FractalWidget::mouseReleaseEvent(QMouseEvent *event)
{
    emit mouseChanged(event);
}

void FractalWidget::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseChanged(event);
}

void FractalWidget::resizeEvent(QResizeEvent *event)
{
    emit sizeChanged();
}

void FractalWidget::wheelEvent(QWheelEvent *event)
{
    emit mouseChanged(event);
}

void FractalWidget::drawImage(QImage *image)
{
    m_image = image;
    update();
}
