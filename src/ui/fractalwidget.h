#ifndef FRACTALWIDGET_H
#define FRACTALWIDGET_H

#include <QGLWidget>

struct image;
class QImage;
class QPoint;
#define USE_OPENGL
#ifdef USE_OPENGL
class FractalWidget :public QGLWidget
#else
class FractalWidget: public QWidget
#endif
{
    Q_OBJECT
private:
    struct image *m_image = NULL;
    QImage *m_qimage[2];
    QPoint m_mousePosition = QPoint(0, 0);
    Qt::MouseButtons m_mouseButtons = 0;
    Qt::KeyboardModifiers m_keyboardModifiers = 0;
    int m_keyCombination = 0;
    QSize m_sizeHint;
    void updateMouse(QMouseEvent * event);
protected:
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent * event);
    
#ifndef USE_OPENGL
    void paintEvent(QPaintEvent * event);
    
#endif
    void resizeEvent(QResizeEvent * event);
public:
    FractalWidget();

#ifdef USE_OPENGL
    void paintGL();
    void resizeGL (int w, int h);
#endif
    struct image *createImages();
    void destroyImages();
    QPoint mousePosition();
    int mouseButtons();
    int keyCombination();
    void setCursorType(int type);
    QSize sizeHint()const;
    void setSizeHint(const QSize & size);
};


#endif // FRACTALWIDGET_H
