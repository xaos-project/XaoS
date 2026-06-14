#ifndef MOBILEBRIDGE_H
#define MOBILEBRIDGE_H

#include <QObject>
#include <QString>

struct uih_context;
class MobileMainWindow;

/**
 * @brief C++ bridge exposing XaoS engine state and commands to QML.
 *
 * This is a thin layer: QML buttons call methods here, which translate
 * to engine commands or synthetic input events on the MobileMainWindow.
 */
class MobileBridge : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString formulaName READ formulaName NOTIFY stateChanged)
  Q_PROPERTY(int maxIterations READ maxIterations NOTIFY stateChanged)
  Q_PROPERTY(bool autopilotActive READ autopilotActive NOTIFY stateChanged)
  Q_PROPERTY(int formulaCount READ formulaCount CONSTANT)

public:
  explicit MobileBridge(MobileMainWindow *window, QObject *parent = nullptr);

  void setUih(struct uih_context *uih);

  /// Called from the event loop to push state changes to QML.
  void refreshState();

  // Property getters
  QString formulaName() const;
  int maxIterations() const;
  bool autopilotActive() const;
  int formulaCount() const;

public slots:
  /// Execute a named XaoS command (e.g. "initstate", "autopilot")
  Q_INVOKABLE void executeCommand(const QString &command);

  /// Fractal control
  Q_INVOKABLE void setFormula(int index);
  Q_INVOKABLE QString getFormulaName(int index) const;
  Q_INVOKABLE void setIterations(int n);
  Q_INVOKABLE void toggleAutopilot();
  Q_INVOKABLE void toggleJulia();
  Q_INVOKABLE void resetView();
  Q_INVOKABLE void randomizePalette();
  Q_INVOKABLE void loadRandomExample();
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();

  /// Zoom buttons — set synthetic mouse button state
  Q_INVOKABLE void startZoomIn();
  Q_INVOKABLE void startZoomOut();
  Q_INVOKABLE void stopZoom();

  /// Touch gesture translation
  Q_INVOKABLE void gesturePinchStarted();
  Q_INVOKABLE void gesturePinch(double scale, double centerX, double centerY);
  Q_INVOKABLE void gesturePan(double dx, double dy, double centerX,
                               double centerY);
  Q_INVOKABLE void gesturePanFinished();

signals:
  void stateChanged();

private:
  MobileMainWindow *m_window;
  struct uih_context *m_uih = nullptr;

  // Cached values to avoid unnecessary signal emissions
  QString m_formulaName;
  int m_maxIter = 0;
  bool m_autopilot = false;

  double m_lastPinchScale = 1.0;
};

#endif // MOBILEBRIDGE_H
