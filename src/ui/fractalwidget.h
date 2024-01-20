#ifndef FRACTALWIDGET_H
#define FRACTALWIDGET_H

#include "config.h"
#include <QWidget>
#ifdef USE_OPENGL
#include <QOpenGLWidget>
#endif
class QImage;
class QPoint;

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

  protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
#ifdef USE_OPENGL
    void paintGL();
    void resizeGL(int w, int h);
#else
    void paintEvent(QPaintEvent *event);
#endif
  public:
    FractalWidget();
    QSize sizeHint() const;
    QPointF mousePosition();
    void setImage(struct image *image);
};

#endif // FRACTALWIDGET_H
