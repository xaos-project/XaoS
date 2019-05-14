#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
class QImage;
class FractalWidget;
class MainWindow:public QMainWindow  {
    Q_OBJECT
private:
    Qt::MouseButtons m_mouseButtons = 0;
    Qt::KeyboardModifiers m_keyboardModifiers = 0;
    int m_keyCombination = 0;
    FractalWidget * m_fractalWidget;
    void readSettings();
    void writeSettings();
    static QKeySequence::StandardKey keyForItem(const QString & name);
protected:
    void closeEvent(QCloseEvent *);
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent * event);
private slots:
    void activateMenuItem();
    void updateMenuCheckmarks();
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    FractalWidget *fractalWidget();
    void buildMenu(struct uih_context *uih, const char *name);
    void buildMenu(struct uih_context *uih, const char *name, QMenu *parent);
    void buildMenu(struct uih_context *uih, const char *name, QMenu *parent, bool numbered);
    void popupMenu(struct uih_context *uih, const char *name);
    void toggleMenu(struct uih_context *uih, const char *name);
    void showDialog(struct uih_context *uih, const char *name);
    struct image *createImage();
    struct uih_context *createContext();
    void showStatus(const char *text);
    int mouseButtons();
    int keyCombination();
};

#endif // MAINWINDOW_H
