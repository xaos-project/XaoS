#include "fractalparameters.h"
#include "ui_fractalparameters.h"
#include "formulas.h"
#include "ui_helper.h"

FractalParameters::FractalParameters(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::FractalParameters)
{
    m_ui->setupUi(this);
}

FractalParameters::~FractalParameters()
{
    delete m_ui;
}

void FractalParameters::on_formula_currentIndexChanged(int index)
{
    if (m_model)
        m_model->setFormula(index);
}

void FractalParameters::on_interiorColor_currentIndexChanged(int index)
{
    if (m_model)
        m_model->setInteriorColor(index);
}

void FractalParameters::on_exteriorColor_currentIndexChanged(int index)
{
    if (m_model)
        m_model->setExteriorColor(index);
}

void FractalParameters::on_plane_currentIndexChanged(int index)
{
    if (m_model)
        m_model->setPlane(index);
}

void FractalParameters::on_perturbationX_editingFinished()
{
    /*
    if (m_model)
        m_model->setPerturbation(QPointF(
                m_ui->perturbationX->text().toFloat(),
                m_ui->perturbationY->text().toFloat()));
                */
}

void FractalParameters::on_perturbationY_editingFinished()
{
    /*
    if (m_model)
        m_model->setPerturbation(QPointF(
                m_ui->perturbationX->text().toFloat(),
                m_ui->perturbationY->text().toFloat()));
                */
}

void FractalParameters::on_perturbationX_sliderMoved(int value)
{
    if (m_model)
        m_model->setPerturbation(QPointF(
                (float)m_ui->perturbationX->value() / (m_ui->perturbationX->maximum() - m_ui->perturbationX->minimum()) * -10,
                (float)m_ui->perturbationY->value() / (m_ui->perturbationY->maximum() - m_ui->perturbationY->minimum()) * -10));
}

void FractalParameters::on_perturbationY_sliderMoved(int value)
{
    if (m_model)
        m_model->setPerturbation(QPointF(
                (float)m_ui->perturbationX->value() / (m_ui->perturbationX->maximum() - m_ui->perturbationX->minimum()) * -10,
                (float)m_ui->perturbationY->value() / (m_ui->perturbationY->maximum() - m_ui->perturbationY->minimum()) * -10));
}

void FractalParameters::on_centerX_editingFinished()
{
    if (m_model)
        m_model->setCenter(QPointF(
                m_ui->centerX->text().toFloat(),
                m_ui->centerY->text().toFloat()));
}

void FractalParameters::on_centerY_editingFinished()
{
    if (m_model)
        m_model->setCenter(QPointF(
                m_ui->centerX->text().toFloat(),
                m_ui->centerY->text().toFloat()));
}

void FractalParameters::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void FractalParameters::updateParameters()
{
    if (!m_ui->formula->hasFocus())
        m_ui->formula->setCurrentIndex(m_model->formula());

    if (!m_ui->interiorColor->hasFocus())
        m_ui->interiorColor->setCurrentIndex(m_model->interiorColor());

    if (!m_ui->exteriorColor->hasFocus())
        m_ui->exteriorColor->setCurrentIndex(m_model->exteriorColor());

    if (!m_ui->plane->hasFocus())
        m_ui->plane->setCurrentIndex(m_model->plane());

    if (!m_ui->perturbationX->hasFocus())
        m_ui->perturbationX->setValue(m_model->perturbation().x());
                //setText(QString::number(m_model->perturbation().x()));

    if (!m_ui->perturbationY->hasFocus())
        m_ui->perturbationY->setValue(m_model->perturbation().y());
                //setText(QString::number(m_model->perturbation().y()));

    if (!m_ui->centerX->hasFocus())
        m_ui->centerX->setText(QString::number(m_model->center().x()));

    if (!m_ui->centerY->hasFocus())
        m_ui->centerY->setText(QString::number(m_model->center().y()));
}

void FractalParameters::setModel(FractalModel *model)
{
    m_model = model;

    m_ui->formula->clear();
    m_ui->formula->addItems(m_model->formulas());

    m_ui->interiorColor->clear();
    m_ui->interiorColor->addItems(m_model->interiorColors());

    m_ui->exteriorColor->clear();
    m_ui->exteriorColor->addItems(m_model->exteriorColors());

    m_ui->plane->clear();
    m_ui->plane->addItems(m_model->planes());

    connect(m_model, SIGNAL(imageChanged()), this, SLOT(updateParameters()));
}
