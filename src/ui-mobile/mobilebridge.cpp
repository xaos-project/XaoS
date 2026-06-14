#include "mobilebridge.h"
#include "mobilemainwindow.h"
#include "fractalwidget.h"

#include "ui_helper.h"
#include "fractal.h"
#include "xmenu.h"
#include "formulas.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>

MobileBridge::MobileBridge(MobileMainWindow *window, QObject *parent)
    : QObject(parent), m_window(window) {}

void MobileBridge::setUih(struct uih_context *uih) { m_uih = uih; }

// ─────────────────────────────────────────────────────────────
// State refresh — called from event loop to push to QML
// ─────────────────────────────────────────────────────────────

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

void MobileBridge::loadRandomExample() {
  if (!m_uih)
    return;
  uih_saveundo(m_uih);
  uih_loadexample(m_uih);
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

// ─────────────────────────────────────────────────────────────
// Zoom buttons — synthetic mouse buttons
// ─────────────────────────────────────────────────────────────

void MobileBridge::startZoomIn() {
  m_window->setSyntheticButtons(BUTTON1);
}

void MobileBridge::startZoomOut() {
  m_window->setSyntheticButtons(BUTTON3);
}

void MobileBridge::stopZoom() { m_window->setSyntheticButtons(0); }

// ─────────────────────────────────────────────────────────────
// Touch gesture translation → synthetic Qt events
// ─────────────────────────────────────────────────────────────

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
