#ifndef FRACTALBRIDGE_H
#define FRACTALBRIDGE_H

#include <QObject>
#include <QString>
#include <QMargins>

struct uih_context;
struct menuitem;

class MainWindow;

/**
 * @brief C++ bridge exposing XaoS engine state and commands to QML.
 *
 * This object is set as a context property on the QML overlay so that
 * QML buttons can call executeCommand("zoom"), read coordinates, etc.
 */
class FractalBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString zoomLevel READ zoomLevel NOTIFY stateChanged)
    Q_PROPERTY(QString realCoord READ realCoord NOTIFY stateChanged)
    Q_PROPERTY(QString imagCoord READ imagCoord NOTIFY stateChanged)
    Q_PROPERTY(int maxIterations READ maxIterations NOTIFY stateChanged)
    Q_PROPERTY(QString fractalName READ fractalName NOTIFY stateChanged)
    Q_PROPERTY(bool autopilotActive READ autopilotActive NOTIFY stateChanged)

    // Safe area insets (pixels) for notch, home indicator, status bar
    Q_PROPERTY(int safeTop READ safeTop NOTIFY safeAreaChanged)
    Q_PROPERTY(int safeBottom READ safeBottom NOTIFY safeAreaChanged)
    Q_PROPERTY(int safeLeft READ safeLeft NOTIFY safeAreaChanged)
    Q_PROPERTY(int safeRight READ safeRight NOTIFY safeAreaChanged)

public:
    explicit FractalBridge(MainWindow *window, QObject *parent = nullptr);

    QString zoomLevel() const;
    QString realCoord() const;
    QString imagCoord() const;
    int maxIterations() const;
    QString fractalName() const;
    bool autopilotActive() const;

    int safeTop() const;
    int safeBottom() const;
    int safeLeft() const;
    int safeRight() const;

    void updateSafeAreaInsets();

    void setUih(struct uih_context *uih);

    /// Call from the main event loop to push state changes to QML.
    void refreshState();

public slots:
    /**
     * Execute a named XaoS command (e.g. "zoom", "unzoom", "initstate",
     * "autopilot", "saveimg", "undo", "redo").
     */
    Q_INVOKABLE void executeCommand(const QString &command);

    /// Open an XaoS menu by name (e.g. "root", "palettemenu").
    Q_INVOKABLE void openMenu(const QString &menuName);

    Q_INVOKABLE void startZoomIn();
    Q_INVOKABLE void startZoomOut();
    Q_INVOKABLE void stopZoom();

    Q_INVOKABLE void gesturePinchStarted();
    Q_INVOKABLE void gesturePinch(double scale, double centerX, double centerY);
    Q_INVOKABLE void gesturePan(double dx, double dy, double centerX, double centerY);
    Q_INVOKABLE void gesturePanFinished();

signals:
    void stateChanged();
    void safeAreaChanged();

private:
    MainWindow *m_window;
    struct uih_context *m_uih = nullptr;

    // Cached values to avoid unnecessary signal emissions
    QString m_zoomLevel;
    QString m_realCoord;
    QString m_imagCoord;
    int m_maxIter = 0;
    QString m_fractalName;
    bool m_autopilot = false;

    // Safe area
    int m_safeTop = 0;
    int m_safeBottom = 0;
    int m_safeLeft = 0;
    int m_safeRight = 0;

    double m_lastPinchScale = 1.0;
};

#endif // FRACTALBRIDGE_H
