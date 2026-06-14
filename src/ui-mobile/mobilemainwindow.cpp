#include "mobilemainwindow.h"
#include "fractalwidget.h"
#include "mobilebridge.h"

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QScreen>
#include <QThread>
#include <QTimer>
#include <cassert>
#include <cstdio>
#include <ctime>

#include "ui.h"
#include "ui_helper.h"
#include "filter.h"
#include "fractal.h"
#include "grlib.h"
#include "i18n.h"
#include "timers.h"
#include "xerror.h"
#include "xmenu.h"
#include "xthread.h"

// Externs from ui.h / other translation units
extern float pixelwidth, pixelheight;
extern tl_group *syncgroup;
extern struct image *create_image_qt(int width, int height,
                                     struct palette *palette, float pixelwidth,
                                     float pixelheight);

// Static engine callbacks — these route to our MobileMainWindow instance
static int ui_passfunc(struct uih_context *uih, int display, const char *text,
                       float percent) {
  if (uih->data) {
    auto *w = reinterpret_cast<MobileMainWindow *>(uih->data);
    return w->showProgress(display, text, percent);
  }
  return 0;
}

static void ui_message_cb(struct uih_context *uih) {
  if (uih->data) {
    auto *w = reinterpret_cast<MobileMainWindow *>(uih->data);
    w->pleaseWait();
  }
}

void ui_updatemenus(struct uih_context *uih, const char *name) {
  if (uih->data) {
    auto *w = reinterpret_cast<MobileMainWindow *>(uih->data);
    w->updateMenus(name);
  }
}

// ─────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────

MobileMainWindow::MobileMainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("XaoS");
  setMouseTracking(true);

  // Create the fractal rendering widget as central widget
  m_widget = new FractalWidget();
  setCentralWidget(m_widget);

  // Enter fullscreen for mobile
  showFullScreen();

  m_messageFont = QFont(QApplication::font().family(), 12);
}

MobileMainWindow::~MobileMainWindow() {
  if (m_uih) {
    uih_cycling_off(m_uih);
    uih_freecatalog(m_uih);
    uih_freecontext(m_uih);
  }
  if (m_mainTimer)
    tl_free_timer(m_mainTimer);
  if (m_loopTimer)
    tl_free_timer(m_loopTimer);
  if (m_uih && m_uih->image) {
    destroypalette(m_uih->image->palette);
    destroy_image(m_uih->image);
  }
}

// ─────────────────────────────────────────────────────────────
// Image creation (same palette setup as desktop)
// ─────────────────────────────────────────────────────────────

struct image *MobileMainWindow::makeImage(int width, int height) {
  struct palette *palette;
  union paletteinfo info;
  info.truec.rmask = 0xff0000;
  info.truec.gmask = 0x00ff00;
  info.truec.bmask = 0x0000ff;
  palette = createpalette(0, 0, TRUECOLOR, 0, 0, NULL, NULL, NULL, NULL, &info);
  if (!palette) {
    qCritical("MobileMainWindow: Cannot create palette");
    return nullptr;
  }

  struct image *image = create_image_qt(width, height, palette,
                                        pixelwidth, pixelheight);
  if (!image) {
    qCritical("MobileMainWindow: Cannot create image");
    destroypalette(palette);
    return nullptr;
  }
  m_widget->setImage(image);
  return image;
}

// ─────────────────────────────────────────────────────────────
// Initialization
// ─────────────────────────────────────────────────────────────

void MobileMainWindow::init() {
  // Compute pixel size from screen DPI
  QScreen *screen = windowHandle() ? windowHandle()->screen()
                                   : QGuiApplication::primaryScreen();
  if (screen) {
    if (!pixelwidth)
      pixelwidth = 2.54f / screen->physicalDotsPerInchX();
    if (!pixelheight)
      pixelheight = 2.54f / screen->physicalDotsPerInchY();
  }

  int width = m_widget->size().width();
  int height = m_widget->size().height();
  struct image *image = makeImage(width, height);
  if (!image) {
    qCritical("MobileMainWindow: Failed to create initial image");
    return;
  }

  m_uih = uih_mkcontext(PIXELSIZE, image, ui_passfunc, ui_message_cb,
                         ui_updatemenus);
  if (!m_uih) {
    qCritical("MobileMainWindow: Failed to create uih context");
    return;
  }

  m_uih->data = this;
  m_uih->font = &m_messageFont;
  m_uih->fcontext->version++;
  uih_newimage(m_uih);

  // Create timers
  tl_update_time();
  m_mainTimer = tl_create_timer();
  m_loopTimer = tl_create_timer();
  tl_reset_timer(m_mainTimer);
  tl_reset_timer(m_loopTimer);

  // Create the QML overlay for mobile touch controls
  createOverlay();
}

// ─────────────────────────────────────────────────────────────
// QML overlay — transparent layer on top of FractalWidget
// ─────────────────────────────────────────────────────────────

void MobileMainWindow::createOverlay() {
  m_bridge = new MobileBridge(this, this);
  m_bridge->setUih(m_uih);

  m_overlay = new QQuickWidget(this);
  m_overlay->setResizeMode(QQuickWidget::SizeRootObjectToView);
  m_overlay->setClearColor(Qt::transparent);
  m_overlay->setAttribute(Qt::WA_AlwaysStackOnTop);
  m_overlay->setAttribute(Qt::WA_TranslucentBackground);
  // Let touch events through transparent areas to FractalWidget
  m_overlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);

  // Expose bridge to QML
  m_overlay->rootContext()->setContextProperty("bridge", m_bridge);

  m_overlay->setSource(QUrl("qrc:/qml/main.qml"));

  if (m_overlay->status() == QQuickWidget::Error) {
    qWarning() << "QML overlay errors:";
    for (const auto &error : m_overlay->errors())
      qWarning() << "  " << error.toString();
  }

  m_overlay->show();
  m_overlay->raise();

  // Ensure overlay covers the full window
  m_overlay->setGeometry(0, 0, width(), height());
}

// ─────────────────────────────────────────────────────────────
// Main event loop — matches the desktop eventLoop() pattern
// ─────────────────────────────────────────────────────────────

void MobileMainWindow::eventLoop() {
  QTimer eventTimer;
  eventTimer.setTimerType(Qt::PreciseTimer);

  connect(&eventTimer, &QTimer::timeout, this, [this]() {
    if (!m_uih)
      return;

    int inmovement = 1;

    // Refresh QML bridge state
    if (m_bridge)
      m_bridge->refreshState();

    // Render frame if engine has new data
    if (m_uih->display) {
      uih_prepare_image(m_uih);
      uih_updatestatus(m_uih);
      m_widget->repaint();
    }

    // Process engine timer group
    int time = tl_process_group(syncgroup, nullptr);
    if (time != -1) {
      if (!inmovement && !m_uih->inanimation) {
        if (time > 1000000 / 50)
          time = 1000000 / 50;
        if (time > delaytime) {
          QThread::usleep(time - delaytime);
          tl_update_time();
        }
      }
      inmovement = 1;
    }

    // Frame rate limiting
    if (delaytime || maxframerate) {
      tl_update_time();
      time = tl_lookup_timer(m_loopTimer);
      tl_reset_timer(m_loopTimer);
      time = 1000000 / maxframerate - time;
      if (time < delaytime)
        time = delaytime;
      if (time) {
        QThread::usleep(time);
        tl_update_time();
      }
    }

    // Process pending menu commands
    processQueue();

    // Process input events and feed to engine
    processEvents(!inmovement && !m_uih->inanimation);
    inmovement = 0;

    // Handle deferred resize
    if (m_shouldResize) {
      resizeImage(m_widget->size().width(), m_widget->size().height());
      m_shouldResize = false;
    }
  });

  // Zero-interval timer — fires as fast as Qt can process events
  eventTimer.start(0);

  QCoreApplication::exec();
}

// ─────────────────────────────────────────────────────────────
// Event processing (input → engine)
// ─────────────────────────────────────────────────────────────

void MobileMainWindow::processEvents(bool wait) {
  QCoreApplication::processEvents(wait ? QEventLoop::WaitForMoreEvents
                                       : QEventLoop::AllEvents);

  int mousex = m_widget->mousePosition().x();
  int mousey = m_widget->mousePosition().y();
  int buttons = mouseButtons();

  tl_update_time();
  uih_update(m_uih, mousex, mousey, buttons);

  // Speed control via main timer
  if (tl_lookup_timer(m_mainTimer) > FRAMETIME || buttons) {
    double mul1 = tl_lookup_timer(m_mainTimer) / FRAMETIME;
    double su = 1 + (SPEEDUP - 1) * mul1;
    if (su > 2 * SPEEDUP)
      su = SPEEDUP;
    tl_reset_timer(m_mainTimer);
  }
}

void MobileMainWindow::processQueue() {
  if (!m_uih || m_uih->incalculation)
    return;

  const menuitem *item;
  dialogparam *d;
  while ((item = menu_delqueue(&d)) != NULL) {
    if (item->type == MENU_SUBMENU)
      continue; // No submenus on mobile
    if (m_uih->incalculation && !(item->flags & MENUFLAG_INCALC)) {
      menu_addqueue(item, d);
      if (item->flags & MENUFLAG_INTERRUPT)
        uih_interrupt(m_uih);
      return;
    }
    uih_saveundo(m_uih);
    menu_activate(item, m_uih, d);
    if (d)
      menu_destroydialog(item, d, m_uih);
  }
}

int MobileMainWindow::mouseButtons() {
  int buttons = 0;
  if (m_mouseButtons & Qt::LeftButton)
    buttons |= BUTTON1;
  if (m_mouseButtons & Qt::MiddleButton)
    buttons |= BUTTON2;
  if (m_mouseButtons & Qt::RightButton)
    buttons |= BUTTON3;

  // Merge synthetic buttons from QML gestures
  buttons |= m_syntheticButtons;

  // Handle mouse wheel
  if (m_mouseWheel > 0)
    buttons |= BUTTON1;
  if (m_mouseWheel < 0)
    buttons |= BUTTON3;
  // Auto-clear wheel after ~1 second
  // (simplified; desktop uses clock_gettime)
  if (m_mouseWheel != 0)
    m_mouseWheel = 0;

  return buttons;
}

// ─────────────────────────────────────────────────────────────
// Resize
// ─────────────────────────────────────────────────────────────

void MobileMainWindow::resizeImage(int width, int height) {
  if (!m_uih)
    return;
  if (m_uih->incalculation) {
    uih_interrupt(m_uih);
    return;
  }
  if (width == m_uih->image->width && height == m_uih->image->height)
    return;

  uih_clearwindows(m_uih);
  uih_stoptimers(m_uih);
  uih_cycling_stop(m_uih);
  uih_savepalette(m_uih);

  assert(width > 0 && width < 65000 && height > 0 && height < 65000);

  destroy_image(m_uih->image);
  destroypalette(m_uih->palette);

  struct image *image = makeImage(width, height);
  if (!image) {
    qCritical("MobileMainWindow: Failed to create resized image");
    return;
  }

  if (!uih_updateimage(m_uih, image)) {
    qCritical("MobileMainWindow: Failed to update image in context");
    return;
  }

  tl_process_group(syncgroup, NULL);
  tl_reset_timer(m_mainTimer);

  uih_newimage(m_uih);
  uih_restorepalette(m_uih);
  m_uih->display = 1;
  uih_cycling_continue(m_uih);
}

void MobileMainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);

  // Keep overlay covering the full window
  if (m_overlay)
    m_overlay->setGeometry(0, 0, width(), height());

  m_shouldResize = true;
}

// ─────────────────────────────────────────────────────────────
// Mouse/touch events — captured by the window
// ─────────────────────────────────────────────────────────────

void MobileMainWindow::mousePressEvent(QMouseEvent *event) {
  m_mouseButtons = event->buttons();
}

void MobileMainWindow::mouseReleaseEvent(QMouseEvent *event) {
  m_mouseButtons = event->buttons();
}

void MobileMainWindow::wheelEvent(QWheelEvent *event) {
  m_mouseWheel = event->angleDelta().y();
}

void MobileMainWindow::closeEvent(QCloseEvent *) { ui_quit(0); }

// ─────────────────────────────────────────────────────────────
// Engine callbacks
// ─────────────────────────────────────────────────────────────

int MobileMainWindow::showProgress(int display, const char *text,
                                    float percent) {
  processEvents(false);
  if (!m_uih->play && m_uih->display) {
    if (nthreads == 1)
      uih_drawwindows(m_uih);
    m_widget->repaint();
    uih_cycling_continue(m_uih);
  }
  return 0;
}

void MobileMainWindow::pleaseWait() {
  if (m_uih->play)
    return;
  setCursor(Qt::WaitCursor);
}

void MobileMainWindow::updateMenus(const char * /*name*/) {
  // Mobile doesn't use the traditional menu system
}
