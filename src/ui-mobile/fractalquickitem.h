#ifndef FRACTALQUICKITEM_H
#define FRACTALQUICKITEM_H

#include <QPointF>
#include <QQuickPaintedItem>
#include <QTimer>

class EngineBridge;

class FractalQuickItem : public QQuickPaintedItem {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(
      EngineBridge *engine READ engine WRITE setEngine NOTIFY engineChanged)
  Q_PROPERTY(bool zooming READ zooming NOTIFY zoomingChanged)
  Q_PROPERTY(bool panning READ panning NOTIFY panningChanged)

public:
  explicit FractalQuickItem(QQuickItem *parent = nullptr);
  ~FractalQuickItem();

  void paint(QPainter *painter) override;

  EngineBridge *engine() const { return m_engine; }
  void setEngine(EngineBridge *engine);

  bool zooming() const { return m_zooming; }
  bool panning() const { return m_panning; }

  // Called from QML gesture handlers
  Q_INVOKABLE void startZoomIn(qreal x, qreal y);
  Q_INVOKABLE void startZoomOut(qreal x, qreal y);
  Q_INVOKABLE void stopZoom();
  Q_INVOKABLE void updateMousePosition(qreal x, qreal y);
  Q_INVOKABLE void startPan(qreal x, qreal y);
  Q_INVOKABLE void updatePan(qreal x, qreal y);
  Q_INVOKABLE void stopPan();
  Q_INVOKABLE void tapAt(qreal x, qreal y);

signals:
  void engineChanged();
  void zoomingChanged();
  void panningChanged();

protected:
  void geometryChange(const QRectF &newGeometry,
                      const QRectF &oldGeometry) override;

private slots:
  void onFrameTick();

private:
  EngineBridge *m_engine = nullptr;
  QTimer *m_frameTimer = nullptr;

  // Mouse/touch state translated to engine button flags
  int m_mouseX = 0;
  int m_mouseY = 0;
  int m_buttons = 0;

  bool m_zooming = false;
  bool m_panning = false;

  // Panning state
  QPointF m_panStart;
  QPointF m_lastPanPos;

  // Coordinate mapping from QML to engine space
  void mapToEngine(qreal qmlX, qreal qmlY, int &engineX, int &engineY);
};

#endif // FRACTALQUICKITEM_H
