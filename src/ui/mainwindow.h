#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
class QImage;
class FractalWidget;
class MainWindow:public QMainWindow  {
    Q_OBJECT
private:
    struct uih_context *m_context;
    FractalWidget * m_fractalWidget;
    void readSettings();
    void writeSettings();
    static QKeySequence::StandardKey keyForItem(const QString & name);
protected:
    void closeEvent(QCloseEvent *);
private slots:
    void activateMenuItem();
    void updateMenuCheckmarks();
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    FractalWidget *fractalWidget();
    struct uih_context *createContext();
    void buildMenu(const char *name);
    void buildMenu(const char *name, QMenu *parent);
    void buildMenu(const char *name, QMenu *parent, bool numbered);
    void popupMenu(const char *name);
    void toggleMenu(const char *name);
    void showDialog(const char *name);
    void showStatus(const char *text);
};

#endif // MAINWINDOW_H
