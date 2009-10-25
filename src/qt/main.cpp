#include <signal.h>
#include <QtGui/QApplication>
#include "mainwindow.h"

#include "config.h"
#include "xio.h"
#include "xthread.h"
#include "filter.h"
#include "ui_helper.h"

int main(int argc, char *argv[])
{
    xio_init(argv[0]);
    uih_registermenudialogs_i18n();
    uih_registermenus_i18n();
    uih_registermenus();
    signal(SIGFPE, SIG_IGN);
    //xth_init(1);
    //srand(time(NULL));

    QApplication a(argc, argv);
    MainWindow *w = new MainWindow;
    w->show();
    return a.exec();
}
