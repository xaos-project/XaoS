#include "fractalwidget.h"

#include <QtGui>

FractalWidget::FractalWidget()
{
    setFocusPolicy(Qt::WheelFocus);
    setMouseTracking(true);
    m_image = 0;
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

void FractalWidget::wheelEvent(QWheelEvent *event)
{
    emit mouseChanged(event);
}

void FractalWidget::keyPressEvent(QKeyEvent *event)
{
    emit keyPressed(event);
}

void FractalWidget::keyReleaseEvent(QKeyEvent *event)
{
    emit keyReleased(event);
}

void FractalWidget::paintEvent (QPaintEvent *event)
{
    if (m_image) {
        QPainter painter(this);
        painter.drawImage(0, 0, *m_image);
    }
}

void FractalWidget::resizeEvent(QResizeEvent *event)
{
    emit sizeChanged();
}

void FractalWidget::drawImage(QImage *image)
{
    m_image = image;
    update();
}
