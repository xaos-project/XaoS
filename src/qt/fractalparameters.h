#ifndef FRACTALPARAMETERS_H
#define FRACTALPARAMETERS_H

#include <QtGui/QWidget>

#include "fractalmodel.h"

namespace Ui {
    class FractalParameters;
}

class FractalParameters : public QWidget {
    Q_OBJECT
public:
    FractalParameters(QWidget *parent = 0);
    ~FractalParameters();

    void setModel(FractalModel *model);

protected:
    void changeEvent(QEvent *e);

private slots:
    void on_formula_currentIndexChanged(int index);
    void on_interiorColor_currentIndexChanged(int index);
    void on_exteriorColor_currentIndexChanged(int index);
    void on_plane_currentIndexChanged(int index);
    void on_perturbationX_editingFinished();
    void on_perturbationY_editingFinished();
    void on_perturbationX_sliderMoved(int value);
    void on_perturbationY_sliderMoved(int value);
    void on_centerX_editingFinished();
    void on_centerY_editingFinished();
    void updateParameters();

private:
    Ui::FractalParameters *m_ui;
    FractalModel *m_model;
};

#endif // FRACTALPARAMETERS_H
