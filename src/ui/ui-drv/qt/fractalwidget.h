#ifndef FRACTALWIDGET_H
#define FRACTALWIDGET_H

#include <QGLWidget>

class QImage;
class QPoint;

#ifdef USE_OPENGL
class FractalWidget : public QGLWidget
#else
class FractalWidget : public QWidget
#endif
{
    Q_OBJECT

private:
    QImage *m_image[2];
    int m_activeImage;
    QPoint m_mousePosition;
    Qt::MouseButtons m_mouseButtons;
    Qt::KeyboardModifiers m_keyboardModifiers;
    int m_keyCombination;
    QSize m_sizeHint;

    void updateMouse(QMouseEvent *event);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
#ifndef USE_OPENGL
    void paintEvent (QPaintEvent *event);
#endif
    void resizeEvent(QResizeEvent *event);

public:
    FractalWidget();

#ifdef USE_OPENGL
    void paintGL();
    void resizeGL(int w, int h);
#endif

    void createImages();
    void destroyImages();

    char *imageBuffer1();
    char *imageBuffer2();
    void *imagePointer();
    int imageBytesPerLine();
    QSize imageSize();

    void switchActiveImage();

    QPoint mousePosition();
    int mouseButtons();
    int keyCombination();

    void setCursorType(int type);

    QSize sizeHint() const;
    void setSizeHint(const QSize &size);
};

#endif // FRACTALWIDGET_H
