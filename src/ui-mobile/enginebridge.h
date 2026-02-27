#ifndef ENGINEBRIDGE_H
#define ENGINEBRIDGE_H

#include <QFont>
#include <QImage>
#include <QObject>
#include <QString>

// XaoS engine headers (C linkage)
#include "config.h"
#include "filter.h"
#include "fractal.h"
#include "timers.h"
#include "ui_helper.h"
#include "xmenu.h"

class EngineBridge : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString formulaName READ formulaName NOTIFY formulaChanged)
  Q_PROPERTY(int iterations READ iterations WRITE setIterations NOTIFY
                 iterationsChanged)
  Q_PROPERTY(bool autopilotActive READ autopilotActive NOTIFY autopilotChanged)
  Q_PROPERTY(bool juliaMode READ juliaMode NOTIFY juliaModeChanged)
  Q_PROPERTY(double zoomSpeed READ zoomSpeed WRITE setZoomSpeed NOTIFY
                 zoomSpeedChanged)
  Q_PROPERTY(
      QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
  Q_PROPERTY(int formulaCount READ formulaCount CONSTANT)

public:
  explicit EngineBridge(QObject *parent = nullptr);
  ~EngineBridge();

  // Initialization
  Q_INVOKABLE void init(int width, int height);
  Q_INVOKABLE void resize(int width, int height);

  // Core engine loop
  Q_INVOKABLE void updateEngine(int mouseX, int mouseY, int buttons);
  Q_INVOKABLE void prepareImage();

  // Fractal control
  Q_INVOKABLE void setFormula(int index);
  Q_INVOKABLE QString getFormulaName(int index) const;
  Q_INVOKABLE void setIterations(int n);
  Q_INVOKABLE void toggleAutopilot();
  Q_INVOKABLE void toggleJulia();
  Q_INVOKABLE void setInColoringMode(int mode);
  Q_INVOKABLE void setOutColoringMode(int mode);
  Q_INVOKABLE void setPlane(int mode);
  Q_INVOKABLE void resetView();
  Q_INVOKABLE void recalculate();

  // Palette
  Q_INVOKABLE void randomizePalette();
  Q_INVOKABLE void setDefaultPalette();
  Q_INVOKABLE void cyclePalette(bool enable);

  // Filters
  Q_INVOKABLE void enableFilter(int index);
  Q_INVOKABLE void disableFilter(int index);

  // Save/Load
  Q_INVOKABLE void savePosition(const QString &path);
  Q_INVOKABLE void loadPosition(const QString &path);
  Q_INVOKABLE void loadRandomExample();
  Q_INVOKABLE void savePNG(const QString &path);

  // Undo/Redo
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();

  // Property getters
  QString formulaName() const;
  int iterations() const;
  bool autopilotActive() const;
  bool juliaMode() const;
  double zoomSpeed() const;
  void setZoomSpeed(double speed);
  QString statusMessage() const;
  int formulaCount() const;

  // Direct access for FractalQuickItem
  struct image *engineImage() const { return m_image; }
  uih_context *context() const { return m_uih; }
  bool needsDisplay() const;

signals:
  void imageReady();
  void formulaChanged();
  void iterationsChanged();
  void autopilotChanged();
  void juliaModeChanged();
  void zoomSpeedChanged();
  void statusMessageChanged(const QString &msg);

private:
  struct image *createImage(int width, int height);
  void destroyCurrentImage();

  // Engine state
  uih_context *m_uih = nullptr;
  struct image *m_image = nullptr;

  // Timers
  tl_timer *m_mainTimer = nullptr;
  tl_timer *m_loopTimer = nullptr;

  // UI state
  QFont m_messageFont;
  QString m_statusMessage;
  bool m_initialized = false;

  // Static callbacks for uih_context
  static int passFunc(struct uih_context *uih, int display, const char *text,
                      float percent);
  static void longWait(struct uih_context *uih);
  static void updateMenus(struct uih_context *uih, const char *name);
};

#endif // ENGINEBRIDGE_H
