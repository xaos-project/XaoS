#ifndef CUSTOMDIALOG_H
#define CUSTOMDIALOG_H

#include <QDialog>

#include "ui.h"

class CustomDialog : public QDialog
{
    Q_OBJECT

private:
    const menuitem *m_menuitem;
    const menudialog *m_dialog;
    dialogparam *m_parameters;

private slots:
    void chooseInputFile();
    void chooseOutputFile();
    void showHelp();

public:
    CustomDialog(struct uih_context *uih, const menuitem *item, const menudialog *dialog, QWidget *parent = 0);

    void accept();
    dialogparam *parameters();
};

#endif // CUSTOMDIALOG_H
