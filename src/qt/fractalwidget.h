#ifndef FRACTALWIDGET_H
#define FRACTALWIDGET_H

#include <QWidget>

class FractalModel;

class FractalWidget : public QWidget
{
    Q_OBJECT

public:
    FractalWidget();
    ~FractalWidget();

    void setModel(FractalModel *model);

    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    void init();

    FractalModel *m_model;
};

#endif // FRACTALWIDGET_H
