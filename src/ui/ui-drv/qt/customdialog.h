#ifndef CUSTOMDIALOG_H
#define CUSTOMDIALOG_H

#include <QDialog>

#include "ui.h"

class CustomDialog : public QDialog
{
public:
    CustomDialog(struct uih_context *uih, const menuitem *item, const menudialog *dialog, QWidget *parent = 0);
};

#endif // CUSTOMDIALOG_H
