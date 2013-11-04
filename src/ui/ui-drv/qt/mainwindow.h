#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>

class QImage;
class FractalWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    FractalWidget *m_fractalWidget;

    void readSettings();
    void writeSettings();

    static QKeySequence::StandardKey keyForItem(const QString &name);

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void activateMenuItem();

 public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    FractalWidget *fractalWidget();

    void showMessage(const QString &message);

    void buildMenu(struct uih_context *uih, const char *name);
    void buildMenu(struct uih_context *uih, const char *name, QMenu *parent);
    void popupMenu(struct uih_context *uih, const char *name);
    void toggleMenu(struct uih_context *uih, const char *name);

    void showDialog(struct uih_context *uih, const char *name);
};

#endif // MAINWINDOW_H
