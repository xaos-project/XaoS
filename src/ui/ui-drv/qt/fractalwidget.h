#ifndef FRACTALWIDGET_H
#define FRACTALWIDGET_H

#include <QWidget>

class QImage;
class QPoint;

class FractalWidget : public QWidget
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
    void paintEvent (QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

public:
    FractalWidget();

    void createImages();
    void destroyImages();

    char *imageBuffer1();
    char *imageBuffer2();
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
