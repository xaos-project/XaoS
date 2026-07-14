#include "mobilebridge.h"
#include "mobilemainwindow.h"
#include "fractalwidget.h"
#include "ui_helper.h"
#include "fractal.h"
#include "filter.h"
#include "xmenu.h"
#include "formulas.h"
#include "sffe.h"
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QImage>
#include <QMouseEvent>
#include <QStandardPaths>
#include <QWheelEvent>
#include <cmath>
MobileBridge::MobileBridge(MobileMainWindow *window, QObject *parent)
    : QObject(parent), m_window(window) {}
void MobileBridge::setUih(struct uih_context *uih) { m_uih = uih; }
// State refresh — called from event loop to push to QML
void MobileBridge::refreshState() {
    if (!m_uih || !m_uih->fcontext)
        return;
    bool changed = false;
    const fractal_context *fc = m_uih->fcontext;
    // Formula name
    QString newName;
    if (fc->currentformula)
        newName = fc->currentformula->name[!fc->mandelbrot];
    if (newName != m_formulaName) {
        m_formulaName = newName;
        changed = true;
    }
    // Max iterations
    int newIter = (int)fc->maxiter;
    if (newIter != m_maxIter) {
        m_maxIter = newIter;
        changed = true;
    }
    // Autopilot
    bool newAP = m_uih->autopilot != 0;
    if (newAP != m_autopilot) {
        m_autopilot = newAP;
        changed = true;
    }
    // Palette state
    int newAlg = m_uih->palettetype;
    int newSeed = m_uih->paletteseed;
    int newShift = m_uih->paletteshift + m_uih->manualpaletteshift;
    if (newAlg != m_palAlg || newSeed != m_palSeed || newShift != m_palShift) {
        m_palAlg = newAlg;
        m_palSeed = newSeed;
        m_palShift = newShift;
        changed = true;
    }
    // Zoom magnification
    double newZoom = 1.0;
    if (fc->currentformula && fc->s.rr > 0) {
        newZoom = (double)fc->currentformula->v.rr / (double)fc->s.rr;
    }
    if (std::abs(newZoom - m_zoomMag) > m_zoomMag * 0.001) {
        m_zoomMag = newZoom;
        changed = true;
    }
    	
  // User formula expression
  QString newUF;
  if (fc->userformula && fc->userformula->expression)
    newUF = QString::fromUtf8(fc->userformula->expression);
  if (newUF != m_userFormula) {
    m_userFormula = newUF;
    changed = true;
  }
  // User initial-value expression
  QString newUI;
  if (fc->userinitial && fc->userinitial->expression)
    newUI = QString::fromUtf8(fc->userinitial->expression);
  if (newUI != m_userInitial) {
    m_userInitial = newUI;
    changed = true;
  }
  
  if (changed)
    emit stateChanged();
}
// ─────────────────────────────────────────────────────────────
// Property getters
// ─────────────────────────────────────────────────────────────
QString MobileBridge::formulaName() const { return m_formulaName; }
int MobileBridge::maxIterations() const { return m_maxIter; }
bool MobileBridge::autopilotActive() const { return m_autopilot; }
int MobileBridge::formulaCount() const { return nformulas; }
int MobileBridge::paletteAlgorithm() const { return m_palAlg; }
int MobileBridge::paletteSeed() const { return m_palSeed; }
int MobileBridge::paletteShift() const { return m_palShift; }

QString MobileBridge::userFormulaText() const { return m_userFormula; }
QString MobileBridge::userInitialText() const { return m_userInitial; }

QString MobileBridge::zoomLevel() const {
    double z = m_zoomMag;
    if (z < 1000.0)
        return QString::number(z, 'f', (z < 10.0) ? 2 : (z < 100.0) ? 1 : 0) + QStringLiteral("\u00d7");
    if (z < 1e6)
        return QString::number(z / 1e3, 'f', 1) + QStringLiteral("K\u00d7");
    if (z < 1e9)
        return QString::number(z / 1e6, 'f', 1) + QStringLiteral("M\u00d7");
    return QString::number(z / 1e9, 'f', 1) + QStringLiteral("G\u00d7");
}
QString MobileBridge::getFormulaName(int index) const {
    if (index < 0 || index >= nformulas)
        return QString();
    return QString::fromUtf8(formulas[index].name[0]);
}
// ─────────────────────────────────────────────────────────────
// Fractal control commands
// ─────────────────────────────────────────────────────────────
void MobileBridge::executeCommand(const QString &command) {
    if (!m_uih)
        return;
    const menuitem *item = menu_findcommand(command.toUtf8().constData());
    if (item) {
        uih_saveundo(m_uih);
        menu_activate(item, m_uih, nullptr);
    }
}
void MobileBridge::setFormula(int index) {
    if (!m_uih)
        return;
    uih_saveundo(m_uih);
    uih_setformula(m_uih, index);
}

void MobileBridge::setUserFormula(const QString &expr) {
  if (!m_uih || !m_uih->fcontext)
    return;
  uih_saveundo(m_uih);
  uih_sffeset(m_uih, m_uih->fcontext->userformula,
              expr.toUtf8().constData());
}

void MobileBridge::setUserInitial(const QString &expr) {
  if (!m_uih || !m_uih->fcontext)
    return;
  uih_saveundo(m_uih);
  uih_sffeset(m_uih, m_uih->fcontext->userinitial,
              expr.toUtf8().constData());
}

void MobileBridge::setIterations(int n) {
    if (!m_uih)
        return;
    uih_saveundo(m_uih);
    uih_setmaxiter(m_uih, n);
}
void MobileBridge::toggleAutopilot() {
    if (!m_uih)
        return;
    if (m_uih->autopilot)
        uih_autopilot_off(m_uih);
    else
        uih_autopilot_on(m_uih);
}
void MobileBridge::toggleJulia() {
    if (!m_uih)
        return;
    if (m_uih->juliamode)
        uih_disablejulia(m_uih);
    else
        uih_enablejulia(m_uih);
}
void MobileBridge::resetView() {
    if (!m_uih)
        return;
    uih_saveundo(m_uih);
    uih_initstate(m_uih);
    uih_newimage(m_uih);
}
void MobileBridge::randomizePalette() {
    if (!m_uih)
        return;
    uih_saveundo(m_uih);
    uih_mkpalette(m_uih);
}
void MobileBridge::setCustomPalette(int algorithm, int seed, int shift) {
    if (!m_uih || !m_uih->zengine || !m_uih->zengine->fractalc ||
        !m_uih->zengine->fractalc->palette)
        return;
    uih_saveundo(m_uih);
    uih_cycling_stop(m_uih);
    // algorithm in QML is 1-based (1..3), engine needs 0-based
    int alg0 = qBound(0, algorithm - 1, PALGORITHMS - 1);
    if (mkpalette(m_uih->zengine->fractalc->palette, seed, alg0) != 0) {
        uih_newimage(m_uih);
    }
    m_uih->manualpaletteshift = 0;
    m_uih->palettetype = algorithm;
    m_uih->palettechanged = 1;
    m_uih->paletteseed = seed;
    if (shiftpalette(m_uih->zengine->fractalc->palette, shift)) {
        uih_newimage(m_uih);
    }
    m_uih->paletteshift = shift;
    uih_cycling_continue(m_uih);
}
void MobileBridge::loadRandomExample() {
    if (!m_uih || !m_uih->fcontext || !m_uih->zengine || !m_uih->zengine->fractalc)
        return;
    uih_saveundo(m_uih);
    uih_cycling_stop(m_uih);
    // 1. Pick a random formula
    int f = QRandomGenerator::global()->bounded(nformulas);
    set_formula(m_uih->fcontext, f);
    // 2. Reset view to formula defaults
    m_uih->fcontext->s.cr = m_uih->fcontext->currentformula->v.cr;
    m_uih->fcontext->s.ci = m_uih->fcontext->currentformula->v.ci;
    m_uih->fcontext->s.rr = m_uih->fcontext->currentformula->v.rr;
    m_uih->fcontext->s.ri = m_uih->fcontext->currentformula->v.ri;
    // 3. Pick random palette
    int seed = QRandomGenerator::global()->bounded(65536);
    int alg = QRandomGenerator::global()->bounded(PALGORITHMS);
    mkpalette(m_uih->zengine->fractalc->palette, seed, alg);
    m_uih->palettetype = alg + 1;
    m_uih->paletteseed = seed;
    m_uih->palettechanged = 1;
    m_uih->manualpaletteshift = 0;
    uih_newimage(m_uih);
    uih_cycling_continue(m_uih);
}
void MobileBridge::undo() {
    if (!m_uih)
        return;
    uih_undo(m_uih);
}
void MobileBridge::redo() {
    if (!m_uih)
        return;
    uih_redo(m_uih);
}

// Zoom buttons — synthetic mouse buttons
void MobileBridge::startZoomIn() {
    m_window->setSyntheticButtons(BUTTON1);
}
void MobileBridge::startZoomOut() {
    m_window->setSyntheticButtons(BUTTON3);
}
void MobileBridge::stopZoom() { m_window->setSyntheticButtons(0); }

// Touch gesture translation → synthetic Qt events
void MobileBridge::gesturePinchStarted() { m_lastPinchScale = 1.0; }
void MobileBridge::gesturePinch(double scale, double centerX,
                                double centerY) {
    double delta = scale / m_lastPinchScale;
    // Threshold to avoid jitter
    if (delta > 1.02 || delta < 0.98) {
        int ticks = (delta > 1.0) ? 120 : -120;
        QPointF pos(centerX, centerY);
        QWheelEvent wheel(pos, pos, QPoint(0, 0), QPoint(0, ticks), Qt::NoButton,
                          Qt::NoModifier, Qt::ScrollUpdate, false);
        QCoreApplication::sendEvent(m_window->fractalWidget(), &wheel);
        m_lastPinchScale = scale;
    }
}
void MobileBridge::gesturePan(double /*dx*/, double /*dy*/, double centerX,
                              double centerY) {
    // Pan: simulate middle mouse drag
    m_window->setSyntheticButtons(BUTTON2);
    QPointF pos(centerX, centerY);
    QMouseEvent move(QEvent::MouseMove, pos, pos, pos, Qt::MiddleButton,
                     Qt::MiddleButton, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window->fractalWidget(), &move);
}
void MobileBridge::gesturePanFinished() { m_window->setSyntheticButtons(0); }

// Community sharing — save / load / thumbnail
QString MobileBridge::getCurrentXpf() {
    if (!m_uih)
        return QString();
    char *pos = uih_savepostostr(m_uih);
    if (!pos)
        return QString();
    QString result = QString::fromUtf8(pos);
    free(pos);
    return result;
}
void MobileBridge::loadFromXpf(const QString &xpfData) {
    if (!m_uih || xpfData.isEmpty())
        return;
    uih_saveundo(m_uih);
    QByteArray utf8 = xpfData.toUtf8();
    uih_loadstr(m_uih, utf8.constData());
    char *copy = strdup(utf8.constData());
    uih_loadstr(m_uih, copy);
}
bool MobileBridge::saveThumbnail(const QString &path) {
    if (!m_uih || !m_uih->image || !m_uih->image->data)
        return false;
    // Get the current fractal image from the engine
    struct image *img = m_uih->image;
    QImage *qimage =
        reinterpret_cast<QImage **>(img->data)[img->currimage];
    if (!qimage || qimage->isNull())
        return false;
    // Scale down to 512x512 thumbnail (preserving aspect ratio)
    QImage thumbnail =
        qimage->scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return thumbnail.save(path, "PNG");
}
QString MobileBridge::getTempPath(const QString &filename) {
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    return tempDir + "/" + filename;
}
