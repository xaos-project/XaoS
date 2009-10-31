#include <QtGui>


#include "customdialog.h"

#include "ui.h"

CustomDialog::CustomDialog(struct uih_context *uih, const menuitem *item, const menudialog *dialog, QWidget *parent)
        : QDialog(parent)
{
    QBoxLayout *dialogLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);

    QFormLayout *formLayout = new QFormLayout();
    for (int i = 0; dialog[i].question; i++)
    {
        QString label(dialog[i].question);
        switch (dialog[i].type) {
            case DIALOG_COORD:
            {
                QLineEdit *real = new QLineEdit(QString::number(dialog[i].deffloat, 'g'), this);
                QLineEdit *imag = new QLineEdit(QString::number(dialog[i].deffloat2, 'g'), this);

                QBoxLayout *coordLayout = new QBoxLayout(QBoxLayout::LeftToRight);
                coordLayout->setContentsMargins(0, 0, 0, 0);
                coordLayout->addWidget(real);
                coordLayout->addWidget(new QLabel("+", this));
                coordLayout->addWidget(imag);
                coordLayout->addWidget(new QLabel("i", this));
                formLayout->addRow(label, coordLayout);
                break;
            }
            case DIALOG_IFILE:
            case DIALOG_OFILE:
            {
                QLineEdit *fileName = new QLineEdit(dialog[i].defstr, this);
                QToolButton *chooseButton = new QToolButton(this);
                chooseButton->setText("...");

                QBoxLayout *fileLayout = new QBoxLayout(QBoxLayout::LeftToRight);
                fileLayout->setContentsMargins(0, 0, 0, 0);
                fileLayout->addWidget(fileName);
                fileLayout->addWidget(chooseButton);
                formLayout->addRow(label, fileLayout);
                break;
            }
            case DIALOG_CHOICE:
            {
                QComboBox *combo = new QComboBox(this);
                const char **str = (CONST char **) dialog[i].defstr;
                for (int j = 0; str[j] != NULL; j++)
                    combo->addItem(str[j]);
                combo->setCurrentIndex(dialog[i].defint);
                formLayout->addRow(label, combo);
                break;
            }
            default:
            {
                QLineEdit *value = new QLineEdit(this);
                switch (dialog[i].type) {
                    case DIALOG_INT:
                        value->setText(QString::number(dialog[i].defint));
                        break;
                    case DIALOG_FLOAT:
                        value->setText(QString::number(dialog[i].deffloat, 'g'));
                        break;
                    case DIALOG_STRING:
                    case DIALOG_KEYSTRING:
                        value->setText(dialog[i].defstr);
                        break;
                }
                formLayout->addRow(label, value);
                break;
            }
        }
    }
    dialogLayout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
            (QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help), Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    dialogLayout->addWidget(buttonBox);
    setLayout(dialogLayout);
}
