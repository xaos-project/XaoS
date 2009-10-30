#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>

class QImage;
class FractalWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    FractalWidget *m_fractalWidget;

    QImage *m_image[2];
    int m_activeImage;

    void readSettings();
    void writeSettings();

    static QKeySequence::StandardKey keyForItem(const QString &name);

 public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void createImages();
    void destroyImages();

    char *imageBuffer1();
    char *imageBuffer2();
    int imageBytesPerLine();
    QSize imageSize();

    void switchActiveImage();
    void redrawImage();

    QPoint mousePosition();
    int mouseButtons();

    void showMessage(const QString &message);
    void setCursorType(int type);

    void buildMenu(struct uih_context *uih, const char *name);
    void buildMenu(struct uih_context *uih, const char *name, QMenu *parent);

public slots:
    void startMainLoop();
    void activateMenuItem();
};

#endif // MAINWINDOW_H
