#include <QtGui>

#include "fractalwidget.h"
#include "fractalmodel.h"

FractalWidget::FractalWidget()
{
}

FractalWidget::~FractalWidget()
{
}

void FractalWidget::resizeEvent(QResizeEvent *event)
{
    if (m_model)
        m_model->setSize(event->size());
}

void FractalWidget::paintEvent(QPaintEvent *event)
{
    if (m_model) {
        QPainter painter(this);
        painter.drawImage(0, 0, m_model->image());
    }
}

void FractalWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_model)
        m_model->processEvent(event);
}

void FractalWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_model)
        m_model->processEvent(event);
}

void FractalWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_model)
        m_model->processEvent(event);
}

void FractalWidget::wheelEvent(QWheelEvent *event)
{
    if (m_model)
        m_model->processEvent(event);
}

void FractalWidget::setModel(FractalModel *model)
{
    m_model = model;
    connect(m_model, SIGNAL(imageChanged()), this, SLOT(update()));
}
