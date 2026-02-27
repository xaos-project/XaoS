#include "fractalquickitem.h"
#include "enginebridge.h"

#include <QImage>
#include <QPainter>
#include <QQuickWindow>

// Engine headers for button constants
#include "ui_helper.h"

// Target ~30 FPS for smooth feel without wasting CPU
static const int FRAME_INTERVAL_MS = 33;
// When idle (no interaction), slow down to ~10 FPS
static const int IDLE_INTERVAL_MS = 100;

FractalQuickItem::FractalQuickItem(QQuickItem *parent)
    : QQuickPaintedItem(parent) {
  setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptTouchEvents(true);
  setAntialiasing(false);
  // Use Image target for better software renderer performance
  setRenderTarget(QQuickPaintedItem::Image);

  // Frame timer drives the engine loop
  m_frameTimer = new QTimer(this);
  m_frameTimer->setTimerType(Qt::CoarseTimer);
  m_frameTimer->setInterval(IDLE_INTERVAL_MS);
  connect(m_frameTimer, &QTimer::timeout, this, &FractalQuickItem::onFrameTick);
}

FractalQuickItem::~FractalQuickItem() {
  if (m_frameTimer)
    m_frameTimer->stop();
}

void FractalQuickItem::setEngine(EngineBridge *engine) {
  if (m_engine == engine)
    return;
  m_engine = engine;
  emit engineChanged();

  // Start the frame timer once we have an engine
  if (m_engine && !m_frameTimer->isActive()) {
    m_frameTimer->start();
  }
}

void FractalQuickItem::paint(QPainter *painter) {
  if (!m_engine)
    return;

  struct image *img = m_engine->engineImage();
  if (!img || !img->data)
    return;

  QImage *qimage = reinterpret_cast<QImage **>(img->data)[img->currimage];
  if (!qimage)
    return;

  // Direct blit — fastest path
  QRectF target(0, 0, width(), height());
  painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
  painter->setCompositionMode(QPainter::CompositionMode_Source);
  painter->drawImage(target, *qimage);
}

void FractalQuickItem::geometryChange(const QRectF &newGeometry,
                                      const QRectF &oldGeometry) {
  QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);

  if (m_engine && newGeometry.width() > 0 && newGeometry.height() > 0) {
    qreal dpr = window() ? window()->devicePixelRatio() : 1.0;
    int w = static_cast<int>(newGeometry.width() * dpr);
    int h = static_cast<int>(newGeometry.height() * dpr);
    m_engine->resize(w, h);
  }
}

void FractalQuickItem::onFrameTick() {
  if (!m_engine)
    return;

  // Feed current mouse/touch state to the engine
  m_engine->updateEngine(m_mouseX, m_mouseY, m_buttons);

  // Let the engine do fractal computation
  m_engine->prepareImage();

  // Only trigger a repaint if the engine produced a new frame
  if (m_engine->needsDisplay()) {
    update();
  }

  // Adaptive frame rate: fast when interacting, slow when idle
  bool interacting = (m_buttons != 0) || m_engine->needsDisplay();
  int targetInterval = interacting ? FRAME_INTERVAL_MS : IDLE_INTERVAL_MS;
  if (m_frameTimer->interval() != targetInterval) {
    m_frameTimer->setInterval(targetInterval);
  }
}

// --- Coordinate mapping ---
// QML gives us logical pixel coordinates (0..width(), 0..height())
// The engine image may be at a different resolution (DPI-scaled)
// We need to map: qml_coord -> engine_image_coord

void FractalQuickItem::mapToEngine(qreal qmlX, qreal qmlY, int &engineX,
                                   int &engineY) {
  if (!m_engine || !m_engine->engineImage()) {
    engineX = static_cast<int>(qmlX);
    engineY = static_cast<int>(qmlY);
    return;
  }
  struct image *img = m_engine->engineImage();
  // Scale from QML item size to engine image size
  qreal scaleX = (width() > 0) ? static_cast<qreal>(img->width) / width() : 1.0;
  qreal scaleY =
      (height() > 0) ? static_cast<qreal>(img->height) / height() : 1.0;
  engineX = static_cast<int>(qmlX * scaleX);
  engineY = static_cast<int>(qmlY * scaleY);
}

// --- Gesture methods called from QML ---

void FractalQuickItem::startZoomIn(qreal x, qreal y) {
  mapToEngine(x, y, m_mouseX, m_mouseY);
  m_buttons = BUTTON1; // Left mouse = zoom in
  m_zooming = true;
  m_frameTimer->setInterval(FRAME_INTERVAL_MS); // Speed up
  emit zoomingChanged();
}

void FractalQuickItem::startZoomOut(qreal x, qreal y) {
  mapToEngine(x, y, m_mouseX, m_mouseY);
  m_buttons = BUTTON3; // Right mouse = zoom out
  m_zooming = true;
  m_frameTimer->setInterval(FRAME_INTERVAL_MS);
  emit zoomingChanged();
}

void FractalQuickItem::stopZoom() {
  m_buttons = 0;
  m_zooming = false;
  emit zoomingChanged();
}

void FractalQuickItem::updateMousePosition(qreal x, qreal y) {
  mapToEngine(x, y, m_mouseX, m_mouseY);
}

void FractalQuickItem::startPan(qreal x, qreal y) {
  m_panStart = QPointF(x, y);
  m_lastPanPos = m_panStart;
  m_panning = true;
  m_buttons = BUTTON2; // Middle mouse = pan/drag
  mapToEngine(x, y, m_mouseX, m_mouseY);
  m_frameTimer->setInterval(FRAME_INTERVAL_MS);
  emit panningChanged();
}

void FractalQuickItem::updatePan(qreal x, qreal y) {
  if (!m_panning)
    return;
  mapToEngine(x, y, m_mouseX, m_mouseY);
  m_lastPanPos = QPointF(x, y);
}

void FractalQuickItem::stopPan() {
  m_buttons = 0;
  m_panning = false;
  emit panningChanged();
}

void FractalQuickItem::tapAt(qreal x, qreal y) {
  mapToEngine(x, y, m_mouseX, m_mouseY);
}
