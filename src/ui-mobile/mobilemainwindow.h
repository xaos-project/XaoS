#ifndef MOBILEMAINWINDOW_H
#define MOBILEMAINWINDOW_H

#include <QMainWindow>
#include <QFont>

#include "ui_helper.h"
#include "timers.h"

class FractalWidget;
class QQuickWidget;
class MobileBridge;

/**
 * @brief Mobile main window using QWidget for fractal rendering
 *        and a transparent QML overlay for touch UI controls.
 *
 * This follows the same architecture as the desktop MainWindow:
 * FractalWidget paints the fractal via QPainter directly,
 * and the event loop drives the engine at maximum frame rate.
 * The QML overlay sits on top for mobile-specific UI elements.
 */
class MobileMainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MobileMainWindow(QWidget *parent = nullptr);
  ~MobileMainWindow();

  /// Initialize the engine and start rendering.
  void init();

  /// Enter the main event loop (called from main).
  void eventLoop();

  /// Access for the bridge to send synthetic events
  FractalWidget *fractalWidget() const { return m_widget; }
  void setSyntheticButtons(int buttons) { m_syntheticButtons = buttons; }

  // Callbacks used by the engine
  int showProgress(int display, const char *text, float percent);
  void pleaseWait();
  void updateMenus(const char *name);

protected:
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

private:
  struct image *makeImage(int width, int height);
  void resizeImage(int width, int height);
  int mouseButtons();
  void processQueue();
  void processEvents(bool wait);
  void createOverlay();

  // Core state
  FractalWidget *m_widget = nullptr;
  uih_context *m_uih = nullptr;

  // Timers
  tl_timer *m_mainTimer = nullptr;
  tl_timer *m_loopTimer = nullptr;

  // Input state
  Qt::MouseButtons m_mouseButtons = Qt::NoButton;
  int m_mouseWheel = 0;
  int m_syntheticButtons = 0;
  bool m_shouldResize = false;

  // UI
  QFont m_messageFont;
  QQuickWidget *m_overlay = nullptr;
  MobileBridge *m_bridge = nullptr;
};

#endif // MOBILEMAINWINDOW_H
