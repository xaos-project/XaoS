#ifndef CUSTOMDIALOG_H
#define CUSTOMDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>

#include "ui.h"

extern QStringList fnames;

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
    unsigned char newColors[][3];
  private slots:
    void chooseInputFile();
    void chooseInputFiles();
    void chooseOutputFile();
    void updateVisualiser();
    void colorPicker();
  public:
    CustomDialog(struct uih_context *uih, const menuitem *item,
                 const menudialog *dialog, QWidget *parent = 0);
    void accept();
    dialogparam *parameters();
};

#endif // CUSTOMDIALOG_H
