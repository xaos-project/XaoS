#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>

#include "ui_helper.h"
#include "timers.h"
#include "xmenu.h"

class QImage;
class FractalWidget;
class MainWindow : public QMainWindow
{
    Q_OBJECT
  private:
    Qt::MouseButtons m_mouseButtons = Qt::NoButton;
    int m_mouseWheel = 0;
    timespec wheeltimer;
    Qt::KeyboardModifiers m_keyboardModifiers = Qt::NoModifier;
    int m_keyCombination = 0;
    bool shouldResize = false;
    FractalWidget *widget;
    uih_context *uih;
    tl_timer *maintimer;
    tl_timer *loopt;
    tl_timer *arrowtimer;
    QMenuBar *menuBarRef;
    QFont messageFont;
    QWidget *m_mobileOverlay = nullptr;
    QWidget *m_topHeader = nullptr;

    static QKeySequence::StandardKey keyForItem(const QString &name);
    void buildMenu(const char *name, QMenu *parent, bool numbered);
    void buildMenu(const char *name, QMenu *parent);
    void buildMenu(const char *name);
    void popupMenu(const char *name);
    void toggleMenu(const char *name);
    void showDialog(const char *name);
    void showStatus(const char *text);
    int mouseButtons();
    int keyCombination();
    void readSettings();
    void writeSettings();
    void menuActivate(const menuitem *item, dialogparam *d);
    void processQueue();
    int processKey(int key);
    bool processArrows(int *counter, const char *text, int speed, int keys,
                       int lastkeys, int down, int up, bool tenskip, int min,
                       int max);
    void processEvents(bool wait);
    struct image *makeImage(int width, int height);
    void resizeImage(int width, int height);
    void printSpeed();

  protected:
    void closeEvent(QCloseEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
#ifndef Q_OS_MACOS
#ifndef USE_OPENGL
    void mouseMoveEvent(QMouseEvent *event) override;
#endif
#endif
private slots:
    void activateMenuItem();
    void updateMenuCheckmarks();
    void updateVisualiser();
    void colorPicker();

  public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

#ifndef Q_OS_MACOS
#ifndef USE_OPENGL
    void showFullScreen();
    void showNormal();
#endif
#endif
    void pleaseWait();
    int showProgress(int display, const char *text, float percent);
    void updateMenus(const char *name);
    void init();
    void eventLoop();
    void chooseFont();
    void createMobileOverlay();
    void createTopHeader();
};

#endif // MAINWINDOW_H
