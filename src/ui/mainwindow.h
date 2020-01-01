#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>

#include "ui_helper.h"
#include "timers.h"
#include "xmenu.h"

class QImage;
class FractalWidget;
class MainWindow : public QMainWindow
{
    Q_OBJECT
  private:
    Qt::MouseButtons m_mouseButtons = 0;
    int m_mouseWheel = 0;
    timespec wheeltimer;
    Qt::KeyboardModifiers m_keyboardModifiers = 0;
    int m_keyCombination = 0;
    bool shouldResize = false;
    FractalWidget *widget;
    uih_context *uih;
    tl_timer *maintimer;
    tl_timer *loopt;
    tl_timer *arrowtimer;

    void buildMenu(const char *name);
    void buildMenu(const char *name, QMenu *parent);
    void buildMenu(const char *name, QMenu *parent, bool numbered);
    void popupMenu(const char *name);
    void toggleMenu(const char *name);
    void showDialog(const char *name);
    void showStatus(const char *text);
    int mouseButtons();
    int keyCombination();
    void readSettings();
    void writeSettings();
    void menuActivate(const menuitem *item, dialogparam *d);
    void processBuffer();
    int processKey(int key);
    int processCounter(int *counter, const char *text, int speed, int keys,
                       int lastkeys, int down, int up, int tenskip, int min,
                       int max);
    void processEvents(bool wait);
    void doResize();
    void printSpeed();
    struct image *makeImages(int width, int height);
    static QKeySequence::StandardKey keyForItem(const QString &name);

  protected:
    void closeEvent(QCloseEvent *);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *event);
  private slots:
    void activateMenuItem();
    void updateMenuCheckmarks();

  public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void pleaseWait();
    int showProgress(int display, const char *text, float percent);
    void updateMenus(const char *name);
    void init();
    void eventLoop();
};

#endif // MAINWINDOW_H
