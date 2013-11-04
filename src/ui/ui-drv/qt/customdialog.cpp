#include <QtWidgets>


#include "customdialog.h"

#include "ui.h"

CustomDialog::CustomDialog(struct uih_context *uih, const menuitem *item, const menudialog *dialog, QWidget *parent)
        : QDialog(parent)
{
    m_menuitem = item;
    m_dialog = dialog;
    m_parameters = 0;

    setWindowTitle(item->name);

    QBoxLayout *dialogLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    QFormLayout *formLayout = new QFormLayout();

    for (int i = 0; dialog[i].question; i++) {

        QString label(dialog[i].question);
        if (dialog[i].type == DIALOG_COORD) {

            QLineEdit *real = new QLineEdit(QString::number(dialog[i].deffloat, 'g'), this);
            real->setObjectName(label + "real");
            real->setValidator(new QDoubleValidator(real));

            QLineEdit *imag = new QLineEdit(QString::number(dialog[i].deffloat2, 'g'), this);
            imag->setObjectName(label + "imag");
            imag->setValidator(new QDoubleValidator(imag));

            QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->addWidget(real);
            layout->addWidget(new QLabel("+", this));
            layout->addWidget(imag);
            layout->addWidget(new QLabel("i", this));
            
            formLayout->addRow(label, layout);

        } else if (dialog[i].type == DIALOG_IFILE || dialog[i].type == DIALOG_OFILE) {

            QLineEdit *filename = new QLineEdit(dialog[i].defstr, this);
            filename->setObjectName(label);

            QToolButton *chooser = new QToolButton(this);
            chooser->setObjectName(label);
            chooser->setText("...");

            if (dialog[i].type == DIALOG_IFILE)
                connect(chooser, SIGNAL(clicked()), this, SLOT(chooseInputFile()));
            else
                connect(chooser, SIGNAL(clicked()), this, SLOT(chooseOutputFile()));

            QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->addWidget(filename);
            layout->addWidget(chooser);

            formLayout->addRow(label, layout);

        } else if (dialog[i].type == DIALOG_CHOICE) {

            QComboBox *combo = new QComboBox(this);
            combo->setObjectName(label);

            const char **str = (const char **)dialog[i].defstr;
            for (int j = 0; str[j] != NULL; j++)
                combo->addItem(str[j]);
            combo->setCurrentIndex(dialog[i].defint);

            formLayout->addRow(label, combo);

        } else {

            QLineEdit *field = new QLineEdit(this);
            field->setObjectName(label);

            if (dialog[i].type == DIALOG_INT) {
                field->setText(QString::number(dialog[i].defint));
                field->setValidator(new QIntValidator(field));
            }
            else if (dialog[i].type == DIALOG_FLOAT) {
                field->setText(QString::number(dialog[i].deffloat, 'g'));
                field->setValidator(new QDoubleValidator(field));
            } else {
                field->setText(dialog[i].defstr);
            }

            formLayout->addRow(label, field);

        }
    }

    dialogLayout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
            (QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help),
            Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(showHelp()));
    dialogLayout->addWidget(buttonBox);

    setLayout(dialogLayout);
}

void CustomDialog::accept()
{
    int nitems;
    for (nitems = 0; m_dialog[nitems].question; nitems++);
    m_parameters = (dialogparam *)malloc(sizeof (*m_parameters) * nitems);

    for (int i = 0; i < nitems; i++) {
        QString label(m_dialog[i].question);

        if (m_dialog[i].type == DIALOG_COORD) {

            QLineEdit *real = findChild<QLineEdit *>(label + "real");
            QLineEdit *imag = findChild<QLineEdit *>(label + "imag");

            m_parameters[i].dcoord[0] = real->text().toFloat();
            m_parameters[i].dcoord[1] = imag->text().toFloat();

        } else if (m_dialog[i].type == DIALOG_CHOICE) {

            QComboBox *field = findChild<QComboBox *>(label);
            m_parameters[i].dint = field->currentIndex();

        } else {

            QLineEdit *field = findChild<QLineEdit *>(label);

            if (m_dialog[i].type == DIALOG_INT)
                m_parameters[i].dint = field->text().toInt();
            else if (m_dialog[i].type == DIALOG_FLOAT)
                m_parameters[i].number = field->text().toFloat();
            else
                m_parameters[i].dstring = strdup(field->text().toUtf8());
        }
    }

    QDialog::accept();
}

dialogparam *CustomDialog::parameters()
{
    return m_parameters;
}

void CustomDialog::chooseInputFile()
{
    QLineEdit *field = findChild<QLineEdit *>(sender()->objectName());

    QString filter = "XaoS Files (*.xpf *.xaf)";
    QString directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    QString fileName = QFileDialog::getOpenFileName(this, sender()->objectName(), directory, filter);
    if (!fileName.isNull())
    {
        field->setText(fileName);
    }
}

void CustomDialog::chooseOutputFile()
{
    QLineEdit *field = findChild<QLineEdit *>(sender()->objectName());
    QString directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    QString fileName = QFileDialog::getSaveFileName(this, sender()->objectName(), directory);
    if (!fileName.isNull())
        field->setText(fileName);
}

void CustomDialog::showHelp()
{
    ui_help(m_menuitem->shortname);
}
