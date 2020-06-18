#ifndef CUSTOMDIALOG_H
#define CUSTOMDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>

#include "ui.h"
class CustomDialog : public QDialog
{
    Q_OBJECT
  private:
    struct palette *gradientpal;
    uih_context *palcontext;
    const menuitem *m_menuitem;
    const menudialog *m_dialog;
    dialogparam *m_parameters;
    QSpinBox *algono, *seedno, *shiftno;
    QLabel *img;
    QSlider *seedslider, *algoslider, *shiftslider;
  private slots:
    void chooseInputFile();
    void chooseOutputFile();
    void updateVisualiser();

  public:
    CustomDialog(struct uih_context *uih, const menuitem *item,
                 const menudialog *dialog, QWidget *parent = 0);
    void accept();
    dialogparam *parameters();
};

#endif // CUSTOMDIALOG_H
