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
  Q_PROPERTY(int paletteAlgorithm READ paletteAlgorithm NOTIFY stateChanged)
  Q_PROPERTY(int paletteSeed READ paletteSeed NOTIFY stateChanged)
  Q_PROPERTY(int paletteShift READ paletteShift NOTIFY stateChanged)
  Q_PROPERTY(QString version READ version CONSTANT)
  Q_PROPERTY(QString zoomLevel READ zoomLevel NOTIFY stateChanged)
  Q_PROPERTY(QString userFormulaText READ userFormulaText NOTIFY stateChanged)
  Q_PROPERTY(QString userInitialText READ userInitialText NOTIFY stateChanged)
  Q_PROPERTY(int juliaMode READ juliaMode NOTIFY stateChanged)
  Q_PROPERTY(bool isMandelbrot READ isMandelbrot NOTIFY stateChanged)
public:
  explicit MobileBridge(MobileMainWindow *window, QObject *parent = nullptr);

  void setUih(struct uih_context *uih);

  void refreshState();

  QString formulaName() const;
  int maxIterations() const;
  bool autopilotActive() const;
  int formulaCount() const;
  int paletteAlgorithm() const;
  int paletteSeed() const;
  int paletteShift() const;
  QString version() const;
  QString zoomLevel() const;
  QString userFormulaText() const;
  QString userInitialText() const;
  int juliaMode() const;
  bool isMandelbrot() const;

public slots:
  Q_INVOKABLE void executeCommand(const QString &command);

  Q_INVOKABLE void setFormula(int index);
  Q_INVOKABLE QString getFormulaName(int index) const;
  Q_INVOKABLE void setIterations(int n);
  Q_INVOKABLE void toggleAutopilot();
  Q_INVOKABLE void toggleJulia();
  Q_INVOKABLE void toggleMandelbrot();
  Q_INVOKABLE void resetView();
  Q_INVOKABLE void randomizePalette();
  Q_INVOKABLE void setCustomPalette(int algorithm, int seed, int shift);
  Q_INVOKABLE QStringList getPalettePreview(int algorithm, int seed, int shift);
  Q_INVOKABLE void loadRandomExample();
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();

  Q_INVOKABLE void setUserFormula(const QString &expr);
  Q_INVOKABLE void setUserInitial(const QString &expr);
  
  Q_INVOKABLE void startZoomIn();
  Q_INVOKABLE void startZoomOut();
  Q_INVOKABLE void stopZoom();

  Q_INVOKABLE void updatePointerPosition(double x, double y);
  Q_INVOKABLE void gesturePinchStarted();
  Q_INVOKABLE void gesturePinch(double scale, double centerX, double centerY);
  Q_INVOKABLE void gesturePan(double dx, double dy, double centerX,
                               double centerY);
  Q_INVOKABLE void gesturePanFinished();

  Q_INVOKABLE QString getCurrentXpf();            
  Q_INVOKABLE void loadFromXpf(const QString &xpfData); 
  Q_INVOKABLE bool saveThumbnail(const QString &path);   
  Q_INVOKABLE QString getTempPath(const QString &filename);

signals:
  void stateChanged();

private:
  MobileMainWindow *m_window;
  struct uih_context *m_uih = nullptr;

  QString m_formulaName;
  int m_maxIter = 0;
  bool m_autopilot = false;
  int m_palAlg = 0;
  int m_palSeed = 0;
  int m_palShift = 0;
  double m_zoomMag = 1.0;
  QString m_userFormula;
  QString m_userInitial;
  int m_juliaMode = 0;
  bool m_isMandelbrot = true;

  double m_lastPinchScale = 1.0;
};

#endif
