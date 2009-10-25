#ifndef FRACTALMODEL_H
#define FRACTALMODEL_H

#include <QObject>
#include <QImage>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QStringList>

class FractalModel : public QObject
{
    Q_OBJECT

public:
    FractalModel();
    ~FractalModel();

    void loadFile(const QString &fileName);
    bool saveFile(const QString &fileName);

    void showMessage(const QString &message);
    void showError(const QString &error);
    void clearMessage();

    const QStringList &formulas();
    void setFormula(int index);
    int formula();

    const QStringList &exteriorColors();
    void setExteriorColor(int index);
    int exteriorColor();

    const QStringList &interiorColors();
    void setInteriorColor(int index);
    int interiorColor();

    const QStringList &planes();
    void setPlane(int index);
    int plane();

    QPointF perturbation();
    void setPerturbation(const QPointF &perturbation);

    QPointF center();
    void setCenter(const QPointF &center);

    void setSize(const QSize &size);
    const QImage & image();

    void processEvent(QMouseEvent *event);
    void processEvent(QWheelEvent *event);

signals:
    void messageChanged(const QString &message);
    void messageCleared();
    void errorEncountered(const QString &error);
    void imageChanged();

private slots:
    void updateFractal();
    void copy();
    void paste();

private:
    struct image *createImage(const QSize &size);

    QImage m_image[2];
    struct uih_context *m_uih;
    int m_mouseX, m_mouseY, m_mouseButtons;

    QStringList m_formulas;
    QStringList m_interiorColors;
    QStringList m_exteriorColors;
    QStringList m_planes;

};

#endif // FRACTALMODEL_H
