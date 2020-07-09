#include <QtWidgets>
#define __USE_MINGW_ANSI_STDIO 1 // for long double support on Windows
#include <cstdio>

#include "customdialog.h"

#include "config.h"
#include "ui.h"
#include "misc-f.h"
#include "filter.h"
#include "ui_helper.h"

#ifdef USE_FLOAT128
#include <quadmath.h>
#endif

QString format(number_t number)
{
    char buf[256];
#ifdef USE_FLOAT128
    quadmath_snprintf(buf, 256, "%.34Qg", (__float128)number);
#else
#ifdef USE_LONG_DOUBLE
    snprintf(buf, 256, "%.20Lg", (long double)number);
#else
    snprintf(buf, 256, "%.20g", (double)number);
#endif
#endif
    return QString(buf);
}

CustomDialog::CustomDialog(struct uih_context *uih, const menuitem *item,
                           const menudialog *dialog, QWidget *parent)
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

            QLineEdit *real = new QLineEdit(format(dialog[i].deffloat), this);
            QFontMetrics metric(real->font());
            real->setMinimumWidth(metric.width(real->text()) * 1.1);
            real->setObjectName(label + "real");
            // real->setValidator(new QDoubleValidator(real));

            QLineEdit *imag = new QLineEdit(format(dialog[i].deffloat2), this);
            imag->setObjectName(label + "imag");
            imag->setMinimumWidth(metric.width(imag->text()) * 1.1);
            // imag->setValidator(new QDoubleValidator(imag));

            QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->addWidget(real);
            layout->addWidget(new QLabel("+", this));
            layout->addWidget(imag);
            layout->addWidget(new QLabel("i", this));

            formLayout->addRow(label, layout);

        } else if (dialog[i].type == DIALOG_IFILE ||
                   dialog[i].type == DIALOG_OFILE) {

            QLineEdit *filename = new QLineEdit(dialog[i].defstr, this);
            QFontMetrics metric(filename->font());
            filename->setMinimumWidth(metric.width(filename->text()) * 1.1);
            filename->setObjectName(label);

            QToolButton *chooser = new QToolButton(this);
            chooser->setObjectName(label);
            chooser->setText("...");

            if (dialog[i].type == DIALOG_IFILE)
                connect(chooser, SIGNAL(clicked()), this,
                        SLOT(chooseInputFile()));
            else
                connect(chooser, SIGNAL(clicked()), this,
                        SLOT(chooseOutputFile()));

            QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->addWidget(filename);
            layout->addWidget(chooser);

            formLayout->addRow(label, layout);

        } else if (dialog[i].type == DIALOG_IFILES) {

            QTextEdit *filenames = new QTextEdit(this);
            filenames->setMaximumHeight(filenames->height() * 2);
            filenames->setObjectName(label);

            QToolButton *chooser = new QToolButton(this);
            chooser->setObjectName(label);
            chooser->setText("...");
            connect(chooser, SIGNAL(clicked()), this,
                    SLOT(chooseInputFiles()));

            QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->addWidget(filenames);
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

        } else if(dialog[i].type == DIALOG_PALSLIDER) {

            gradientpal = clonepalette(uih->image->palette);
            palcontext = uih;
            // 3 inputs decide color, Algorithm Number, Seed and shift
            // For Algorithm number
            algono = new QSpinBox(this);
            algono->setObjectName(label + "algono");
            algono->setValue(palcontext->palettetype);
            algono->setRange(1, 3);

            // Algo Slider
            algoslider = new QSlider(Qt::Horizontal, this);
            algoslider->setObjectName(label);
            algoslider->setRange(1, PALGORITHMS);
            algoslider->setValue(algono->value());
            // algoslider->setMinimumWidth(this->width()*2);

            // For Seed Number
            seedno = new QSpinBox(this);
            seedno->setObjectName(label + "seedno");
            seedno->setRange(0, gradientpal->size);
            seedno->setValue(palcontext->paletteseed);

            // Seed Slider
            seedslider = new QSlider(Qt::Horizontal, this);
            seedslider->setObjectName(label);
            seedslider->setRange(0, gradientpal->size);
            seedslider->setValue(seedno->value());

            // For Shift Number
            shiftno = new QSpinBox(this);
            shiftno->setObjectName(label + "shiftno");
            shiftno->setRange(0, gradientpal->size);
            shiftno->setValue(palcontext->paletteshift + palcontext->manualpaletteshift);

            // Shift Slider
            shiftslider = new QSlider(Qt::Horizontal, this);
            shiftslider->setObjectName(label);
            shiftslider->setRange(0, gradientpal->size);
            shiftslider->setValue(shiftno->value());

            // Add them to Layout
            formLayout->addRow("Algorithm", algono);
            formLayout->addWidget(algoslider);
            formLayout->addRow("Seed", seedno);
            formLayout->addWidget(seedslider);
            formLayout->addRow("Shift", shiftno);
            formLayout->addWidget(shiftslider);

            img = new QLabel(this);
            img->setScaledContents(true);
            formLayout->addRow(img);
            updateVisualiser();
            connect(algono,SIGNAL(valueChanged(int)), algoslider, SLOT(setValue(int)));
            connect(algoslider, SIGNAL(valueChanged(int)), algono, SLOT(setValue(int)));
            connect(algono, SIGNAL(valueChanged(int)), this, SLOT(updateVisualiser()));
            connect(seedno,SIGNAL(valueChanged(int)), seedslider, SLOT(setValue(int)));
            connect(seedslider, SIGNAL(valueChanged(int)), seedno, SLOT(setValue(int)));
            connect(seedno, SIGNAL(valueChanged(int)), this, SLOT(updateVisualiser()));
            connect(shiftno,SIGNAL(valueChanged(int)), shiftslider, SLOT(setValue(int)));
            connect(shiftslider, SIGNAL(valueChanged(int)), shiftno, SLOT(setValue(int)));
            connect(shiftno, SIGNAL(valueChanged(int)), this, SLOT(updateVisualiser()));
        } else {

            QLineEdit *field = new QLineEdit(this);
            field->setObjectName(label);

            if (dialog[i].type == DIALOG_INT) {
                field->setText(QString::number(dialog[i].defint));
                field->setValidator(new QIntValidator(field));
            } else if (dialog[i].type == DIALOG_FLOAT) {
                field->setText(format(dialog[i].deffloat));
                // field->setValidator(new QDoubleValidator(field));
            } else {
                field->setText(dialog[i].defstr);
            }
            QFontMetrics metric(field->font());
            field->setMinimumWidth(metric.width(field->text()) * 1.1);
            formLayout->addRow(label, field);
        }
    }

    dialogLayout->addLayout(formLayout);

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox((QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                             Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    dialogLayout->addWidget(buttonBox);

    setLayout(dialogLayout);
}

void CustomDialog::accept()
{
    int nitems;
    char *ps;
    for (nitems = 0; m_dialog[nitems].question; nitems++)
        ;
    m_parameters = (dialogparam *)malloc(sizeof(*m_parameters) * nitems);

    for (int i = 0; i < nitems; i++) {
        QString label(m_dialog[i].question);

        if (m_dialog[i].type == DIALOG_COORD) {

            QLineEdit *real = findChild<QLineEdit *>(label + "real");
            QLineEdit *imag = findChild<QLineEdit *>(label + "imag");

            m_parameters[i].dcoord[0] = xstrtonum(real->text().toUtf8(), &ps);
            m_parameters[i].dcoord[1] = xstrtonum(imag->text().toUtf8(), &ps);

        } else if (m_dialog[i].type == DIALOG_CHOICE) {

            QComboBox *field = findChild<QComboBox *>(label);
            m_parameters[i].dint = field->currentIndex();

        } else if (m_dialog[i].type == DIALOG_IFILES){

            QTextEdit *field = findChild<QTextEdit *>(label);
            QString raw_fnames = field->toPlainText();
            fnames = raw_fnames.split("\n");

        } else {

            QLineEdit *field = findChild<QLineEdit *>(label);

            if (m_dialog[i].type == DIALOG_INT)
                m_parameters[i].dint = field->text().toInt();
            else if (m_dialog[i].type == DIALOG_FLOAT)
                m_parameters[i].number = xstrtonum(field->text().toUtf8(), &ps);
            else if (m_dialog[i].type == DIALOG_PALSLIDER) {
                // Copy data to original context
                palcontext->manualpaletteshift = 0;
                palcontext->palettetype = algono->value();
                palcontext->paletteseed = seedno->value();
                palcontext->paletteshift = shiftno->value();
                m_parameters[i].dint = 1;
                destroypalette(gradientpal);
            }
            else
                m_parameters[i].dstring = strdup(field->text().toUtf8());
        }
    }

    QDialog::accept();
}

dialogparam *CustomDialog::parameters() { return m_parameters; }

void CustomDialog::chooseInputFile()
{
    QLineEdit *field = findChild<QLineEdit *>(sender()->objectName());

    QSettings settings;
    QString fileLocation = settings.value("MainWindow/lastFileLocation", QDir::homePath()).toString();
    QString fileName = QFileDialog::getOpenFileName(
        this, sender()->objectName(), fileLocation, "*.xpf *.xaf");
    if (!fileName.isNull()) {
        field->setText(fileName);
        settings.setValue("MainWindow/lastFileLocation", QFileInfo(fileName).absolutePath());
    }
}

void CustomDialog::chooseOutputFile()
{
    QLineEdit *field = findChild<QLineEdit *>(sender()->objectName());
    QSettings settings;
    QString fileLocation = settings.value("MainWindow/lastFileLocation", QDir::homePath()).toString();
    QString fileName = QFileDialog::getSaveFileName(
        this, sender()->objectName(), fileLocation);
    if (!fileName.isNull()) {
        field->setText(fileName);
        settings.setValue("MainWindow/lastFileLocation", QFileInfo(fileName).absolutePath());
    }
}

void CustomDialog::chooseInputFiles()
{
    QTextEdit *field = findChild<QTextEdit *>(sender()->objectName());
    QSettings settings;
    QString fileLocation = settings.value("MainWindow/lastFileLocation", QDir::homePath()).toString();
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this, sender()->objectName(), fileLocation, "*.xpf *.xaf");
    if(!fileNames.isEmpty()) {
        for(auto file: fileNames) {
            field->append(file);
        }
        settings.setValue("MainWindow/lastFileLocation", QFileInfo(fileNames[0]).absolutePath());
    }
}

void CustomDialog::updateVisualiser()
{
    // Get updated Colors
    int colors[101][3];
    getPaletteColor(gradientpal, seedno->value(),
                    algono->value()-1 < 0? 0:algono->value()-1, shiftno->value(), colors);

    // Load Curve
    QImage palImage(100, 1, QImage::Format_RGB32);

    // Fill Curve
    for(int i=0;i<100;i++) {
        QRgb value = qRgb(colors[i][0], colors[i][1], colors[i][2]);
        palImage.setPixelColor(i, 0, value);
    }

    // Save Result
    QPixmap newImage = QPixmap::fromImage(palImage.scaled(this->algono->width(), this->algono->height()));
    img->setPixmap(newImage);
}
