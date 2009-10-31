#ifndef FRACTALWIDGET_H
#define FRACTALWIDGET_H

#include <QWidget>

class QImage;
class QPoint;

class FractalWidget : public QWidget
{
    Q_OBJECT

private:
    QImage *m_image;
    QPoint m_mousePosition;
    Qt::MouseButtons m_mouseButtons;

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void paintEvent (QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

signals:
    void mouseChanged(QMouseEvent *event);
    void mouseChanged(QWheelEvent *event);
    void sizeChanged();

public:
    FractalWidget();
    void drawImage(QImage *image);
};

#endif // FRACTALWIDGET_H
