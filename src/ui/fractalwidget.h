#ifndef FRACTALWIDGET_H
#define FRACTALWIDGET_H

#include "config.h"
#include <QWidget>
#ifdef USE_OPENGL
#include <QOpenGLWidget>
#endif
class QImage;
class QPoint;
class QGestureEvent;

#ifdef USE_OPENGL
class FractalWidget : public QOpenGLWidget
#else
class FractalWidget : public QWidget
#endif
{
    Q_OBJECT
  private:
    struct image *m_image = NULL;
    QSize m_sizeHint;
    QPointF m_mousePosition = QPointF(0.0, 0.0);
    qreal m_lastScale = 1.0;
    QPointF m_lastPan;

  protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool event(QEvent *event) override;
#ifdef USE_OPENGL
    void paintGL();
    void resizeGL(int w, int h);
#else
    void paintEvent(QPaintEvent *event) override;
#endif
  public:
    FractalWidget();
    QSize sizeHint() const override;
    QPointF mousePosition();
    void setImage(struct image *image);
};

#endif // FRACTALWIDGET_H
