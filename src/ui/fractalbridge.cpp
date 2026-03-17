#include "fractalbridge.h"
#include "mainwindow.h"
#include "ui_helper.h"
#include "fractal.h"
#include "xmenu.h"

#include <cmath>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QEvent>

FractalBridge::FractalBridge(MainWindow *window, QObject *parent)
    : QObject(parent), m_window(window)
{
}

void FractalBridge::setUih(struct uih_context *uih)
{
    m_uih = uih;
}

QString FractalBridge::zoomLevel() const { return m_zoomLevel; }
QString FractalBridge::realCoord() const { return m_realCoord; }
QString FractalBridge::imagCoord() const { return m_imagCoord; }
int FractalBridge::maxIterations() const { return m_maxIter; }
QString FractalBridge::fractalName() const { return m_fractalName; }
bool FractalBridge::autopilotActive() const { return m_autopilot; }

void FractalBridge::refreshState()
{
    if (!m_uih || !m_uih->fcontext)
        return;

    bool changed = false;

    // Calculate zoom level as a power-of-10 exponent
    const fractal_context *fc = m_uih->fcontext;
    double viewWidth = fc->s.ri - fc->s.rr; // approximate view span
    if (viewWidth <= 0) viewWidth = 4.0;
    int zoomExp = (int)std::max(0.0, -std::log10(std::abs(viewWidth) / 4.0));
    QString newZoom = QString("DEEP ZOOM 10%1").arg(
        zoomExp > 0 ? QString::fromUtf8("\u00B9\u2070").left(0) +
        QString::number(zoomExp) : "⁰");
    // Simpler: show as "10^N"
    newZoom = QString("DEEP ZOOM 10^%1").arg(zoomExp);
    if (newZoom != m_zoomLevel) { m_zoomLevel = newZoom; changed = true; }

    // Real and imaginary coordinates of the center
    QString newRe = QString("RE: %1").arg(
        QString::number((double)fc->s.cr, 'f', 10));
    QString newIm = QString("IM: %1").arg(
        QString::number((double)fc->s.ci, 'f', 10));
    if (newRe != m_realCoord) { m_realCoord = newRe; changed = true; }
    if (newIm != m_imagCoord) { m_imagCoord = newIm; changed = true; }

    // Max iterations
    int newIter = (int)fc->maxiter;
    if (newIter != m_maxIter) { m_maxIter = newIter; changed = true; }

    // Fractal name
    QString newName;
    if (fc->currentformula)
        newName = fc->currentformula->name[!fc->mandelbrot];
    if (newName != m_fractalName) { m_fractalName = newName; changed = true; }

    // Autopilot
    bool newAP = m_uih->autopilot != 0;
    if (newAP != m_autopilot) { m_autopilot = newAP; changed = true; }

    if (changed)
        emit stateChanged();
}

void FractalBridge::executeCommand(const QString &command)
{
    if (!m_uih) return;

    const menuitem *item = menu_findcommand(command.toUtf8().constData());
    if (item) {
        qDebug() << "FractalBridge::executeCommand:" << command;
        // Use menuActivate through the MainWindow
        m_window->menuActivateFromBridge(item, nullptr);
    } else {
        qDebug() << "FractalBridge: unknown command:" << command;
    }
}

void FractalBridge::openMenu(const QString &menuName)
{
    if (!m_uih) return;
    qDebug() << "FractalBridge::openMenu:" << menuName;
    m_window->popupMenuFromBridge(menuName.toUtf8().constData());
}

void FractalBridge::startZoomIn()
{
    qDebug() << "FractalBridge::startZoomIn";
    m_window->setSyntheticButtons(BUTTON1);
}

void FractalBridge::startZoomOut()
{
    qDebug() << "FractalBridge::startZoomOut";
    m_window->setSyntheticButtons(BUTTON3);
}

void FractalBridge::stopZoom()
{
    qDebug() << "FractalBridge::stopZoom";
    m_window->setSyntheticButtons(0);
}

void FractalBridge::gesturePinch(double scale, double centerX, double centerY)
{
    // scale is cumulative since start of gesture. Compare with last scale.
    double delta = scale / m_lastPinchScale;
    
    // Use a small threshold (e.g. 2%) to avoid jitter
    if (delta > 1.02 || delta < 0.98) {
        int ticks = (delta > 1.0) ? 120 : -120;
        
        QPointF pos(centerX, centerY);
        QWheelEvent wheel(pos, pos, QPoint(0, 0), QPoint(0, ticks),
                          Qt::NoButton, Qt::NoModifier, Qt::ScrollUpdate, false);
        QCoreApplication::sendEvent(m_window->centralWidget(), &wheel);
        
        m_lastPinchScale = scale;
    }
}

void FractalBridge::gesturePinchStarted()
{
    m_lastPinchScale = 1.0;
}

void FractalBridge::gesturePan(double dx, double dy, double centerX, double centerY)
{
    // Simulate mouse move for panning
    // XaoS expects mouse BUTTON2 (middle) for panning
    QPointF pos(centerX, centerY);
    
    // Ensure BUTTON2 is set during move
    m_window->setSyntheticButtons(BUTTON2);
    
    QMouseEvent move(QEvent::MouseMove, pos, pos, pos,
                     Qt::MiddleButton, Qt::MiddleButton, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window->centralWidget(), &move);
}

void FractalBridge::gesturePanFinished()
{
    qDebug() << "FractalBridge::gesturePanFinished";
    m_window->setSyntheticButtons(0);
}

int FractalBridge::safeTop() const { return m_safeTop; }
int FractalBridge::safeBottom() const { return m_safeBottom; }
int FractalBridge::safeLeft() const { return m_safeLeft; }
int FractalBridge::safeRight() const { return m_safeRight; }

void FractalBridge::updateSafeAreaInsets()
{
    int top = 0, bottom = 0, left = 0, right = 0;

    QWindow *win = m_window ? m_window->windowHandle() : nullptr;

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // Qt 6.8+ provides safeAreaMargins() directly
    if (win) {
        QMarginsF margins = win->safeAreaMargins();
        top = qRound(margins.top());
        bottom = qRound(margins.bottom());
        left = qRound(margins.left());
        right = qRound(margins.right());
    }
#else
    // Fallback: use platform-specific defaults
    Q_UNUSED(win);
#if defined(Q_OS_IOS)
    // iPhone with notch / Dynamic Island
    top = 59;
    bottom = 34;
#elif defined(Q_OS_ANDROID)
    // Android status bar
    top = 24;
#endif
#endif

    // Apply minimum safe margins on mobile even if platform reports 0
#if defined(Q_OS_IOS)
    if (top < 20) top = 20;
    if (bottom < 20) bottom = 20;
#elif defined(Q_OS_ANDROID)
    if (top < 24) top = 24;
#endif

    bool changed = (top != m_safeTop || bottom != m_safeBottom ||
                    left != m_safeLeft || right != m_safeRight);
    m_safeTop = top;
    m_safeBottom = bottom;
    m_safeLeft = left;
    m_safeRight = right;

    if (changed) {
        qDebug() << "Safe area insets:" << top << bottom << left << right;
        emit safeAreaChanged();
    }
}
