#include "enginebridge.h"

#include <QGuiApplication>
#include <QScreen>
#include <cassert>
#include <cstdio>
#include <cstring>

#include "filter.h"
#include "formulas.h"
#include "fractal.h"
#include "grlib.h"
#include "i18n.h"
#include "plane.h"
#include "timers.h"
#include "ui_helper.h"
#include "xerror.h"
#include "xmenu.h"
#include "xthread.h"

// External declarations from main.cpp / other translation units
extern float pixelwidth, pixelheight;
extern tl_group *syncgroup;
extern struct image *create_image_qt(int width, int height,
                                     struct palette *palette, float pixelwidth,
                                     float pixelheight);

// Forward declarations for menu system
extern void uih_registermenus(void);
extern void ui_registermenus_i18n(void);

EngineBridge::EngineBridge(QObject *parent) : QObject(parent) {
  m_messageFont = QFont("Sans", 12);
}

EngineBridge::~EngineBridge() {
  if (m_uih) {
    uih_cycling_off(m_uih);
    uih_freecatalog(m_uih);
    uih_freecontext(m_uih);
  }
  if (m_mainTimer)
    tl_free_timer(m_mainTimer);
  if (m_loopTimer)
    tl_free_timer(m_loopTimer);
  if (m_image) {
    destroypalette(m_image->palette);
    destroy_image(m_image);
  }
}

struct image *EngineBridge::createImage(int width, int height) {
  struct palette *palette;
  union paletteinfo info;
  info.truec.rmask = 0xff0000;
  info.truec.gmask = 0x00ff00;
  info.truec.bmask = 0x0000ff;
  palette = createpalette(0, 0, TRUECOLOR, 0, 0, NULL, NULL, NULL, NULL, &info);
  if (!palette) {
    qWarning("EngineBridge: Cannot create palette");
    return nullptr;
  }

  float pw = pixelwidth;
  float ph = pixelheight;
  if (pw == 0.0f || ph == 0.0f) {
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
      pw = 2.54f / screen->physicalDotsPerInchX();
      ph = 2.54f / screen->physicalDotsPerInchY();
    } else {
      pw = ph = 0.025f; // fallback ~100 DPI
    }
  }

  struct image *image = create_image_qt(width, height, palette, pw, ph);
  if (!image) {
    qWarning("EngineBridge: Cannot create image");
    destroypalette(palette);
    return nullptr;
  }
  return image;
}

void EngineBridge::destroyCurrentImage() {
  if (m_image) {
    destroypalette(m_image->palette);
    destroy_image(m_image);
    m_image = nullptr;
  }
}

// Static callbacks
int EngineBridge::passFunc(struct uih_context *uih, int /*display*/,
                           const char *text, float percent) {
  if (uih->data) {
    EngineBridge *bridge = reinterpret_cast<EngineBridge *>(uih->data);
    if (text && percent > 0) {
      char str[128];
      snprintf(str, sizeof(str), "%s %.1f%%", text, (double)percent);
      bridge->m_statusMessage = QString::fromUtf8(str);
      emit bridge->statusMessageChanged(bridge->m_statusMessage);
    }
  }
  return 0;
}

void EngineBridge::longWait(struct uih_context *uih) {
  if (uih->data) {
    EngineBridge *bridge = reinterpret_cast<EngineBridge *>(uih->data);
    bridge->m_statusMessage = QStringLiteral("Please wait...");
    emit bridge->statusMessageChanged(bridge->m_statusMessage);
  }
}

void EngineBridge::updateMenus(struct uih_context * /*uih*/,
                               const char * /*name*/) {
  // Mobile UI doesn't use the traditional menu system.
  // Menu state is driven by Q_PROPERTY bindings instead.
}

// Initialization
void EngineBridge::init(int width, int height) {
  if (m_initialized)
    return;

  if (width <= 0 || height <= 0) {
    width = 640;
    height = 480;
  }

  m_image = createImage(width, height);
  if (!m_image) {
    qCritical("EngineBridge: Failed to create initial image");
    return;
  }

  m_uih = uih_mkcontext(PIXELSIZE, m_image, passFunc, longWait, updateMenus);
  if (!m_uih) {
    qCritical("EngineBridge: Failed to create uih context");
    destroyCurrentImage();
    return;
  }

  m_uih->data = this;
  m_uih->font = &m_messageFont;

  // Skip catalog loading — xio_getcatalog depends on desktop-specific init.
  // English text works without catalogs (TR() falls back to source strings).

  m_uih->fcontext->version++;
  uih_newimage(m_uih);

  // Create timers
  tl_update_time();
  m_mainTimer = tl_create_timer();
  m_loopTimer = tl_create_timer();
  tl_reset_timer(m_mainTimer);
  tl_reset_timer(m_loopTimer);

  m_initialized = true;

  emit formulaChanged();
  emit iterationsChanged();
}

void EngineBridge::resize(int width, int height) {
  if (!m_uih || !m_initialized)
    return;

  if (m_uih->incalculation) {
    uih_interrupt(m_uih);
    return;
  }

  if (width == m_image->width && height == m_image->height)
    return;

  uih_clearwindows(m_uih);
  uih_stoptimers(m_uih);
  uih_cycling_stop(m_uih);
  uih_savepalette(m_uih);

  assert(width > 0 && width < 65000 && height > 0 && height < 65000);

  destroy_image(m_uih->image);
  destroypalette(m_uih->palette);

  m_image = createImage(width, height);
  if (!m_image) {
    qCritical("EngineBridge: Failed to create resized image");
    return;
  }

  if (!uih_updateimage(m_uih, m_image)) {
    qCritical("EngineBridge: Failed to update image in context");
    return;
  }

  tl_process_group(syncgroup, NULL);
  tl_reset_timer(m_mainTimer);

  uih_newimage(m_uih);
  uih_restorepalette(m_uih);
  m_uih->display = 1;
  uih_cycling_continue(m_uih);
}

// Core engine loop
void EngineBridge::updateEngine(int mouseX, int mouseY, int buttons) {
  if (!m_uih || !m_initialized)
    return;

  tl_update_time();
  uih_update(m_uih, mouseX, mouseY, buttons);
}

void EngineBridge::prepareImage() {
  if (!m_uih || !m_initialized)
    return;

  if (m_uih->display) {
    uih_prepare_image(m_uih);
    uih_updatestatus(m_uih);
    emit imageReady();
  }

  tl_process_group(syncgroup, nullptr);
}

bool EngineBridge::needsDisplay() const { return m_uih && m_uih->display; }

// Fractal control
void EngineBridge::setFormula(int index) {
  if (!m_uih)
    return;
  uih_saveundo(m_uih);
  uih_setformula(m_uih, index);
  emit formulaChanged();
}

QString EngineBridge::getFormulaName(int index) const {
  if (index < 0 || index >= nformulas)
    return QString();
  // formulas[index] has two names: [0] for Mandelbrot, [1] for Julia variant
  return QString::fromUtf8(formulas[index].name[0]);
}

QString EngineBridge::formulaName() const {
  if (!m_uih || !m_uih->fcontext)
    return QStringLiteral("Mandelbrot");
  return QString::fromUtf8(
      m_uih->fcontext->currentformula->name[!m_uih->fcontext->mandelbrot]);
}

int EngineBridge::formulaCount() const { return nformulas; }

void EngineBridge::setIterations(int n) {
  if (!m_uih)
    return;
  uih_saveundo(m_uih);
  uih_setmaxiter(m_uih, n);
  emit iterationsChanged();
}

int EngineBridge::iterations() const {
  if (!m_uih || !m_uih->fcontext)
    return 170;
  return m_uih->fcontext->maxiter;
}

void EngineBridge::toggleAutopilot() {
  if (!m_uih)
    return;
  if (m_uih->autopilot)
    uih_autopilot_off(m_uih);
  else
    uih_autopilot_on(m_uih);
  emit autopilotChanged();
}

bool EngineBridge::autopilotActive() const { return m_uih && m_uih->autopilot; }

void EngineBridge::toggleJulia() {
  if (!m_uih)
    return;
  if (m_uih->juliamode)
    uih_disablejulia(m_uih);
  else
    uih_enablejulia(m_uih);
  emit juliaModeChanged();
}

bool EngineBridge::juliaMode() const { return m_uih && m_uih->juliamode; }

void EngineBridge::setInColoringMode(int mode) {
  if (!m_uih)
    return;
  uih_saveundo(m_uih);
  uih_setincoloringmode(m_uih, mode);
}

void EngineBridge::setOutColoringMode(int mode) {
  if (!m_uih)
    return;
  uih_saveundo(m_uih);
  uih_setoutcoloringmode(m_uih, mode);
}

void EngineBridge::setPlane(int mode) {
  if (!m_uih)
    return;
  uih_saveundo(m_uih);
  uih_setplane(m_uih, mode);
}

void EngineBridge::resetView() {
  if (!m_uih)
    return;
  uih_saveundo(m_uih);
  uih_initstate(m_uih);
  uih_newimage(m_uih);
  emit formulaChanged();
  emit iterationsChanged();
}

void EngineBridge::recalculate() {
  if (!m_uih)
    return;
  uih_recalculate(m_uih);
}

// Palette
void EngineBridge::randomizePalette() {
  if (!m_uih)
    return;
  uih_saveundo(m_uih);
  uih_mkpalette(m_uih);
}

void EngineBridge::setDefaultPalette() {
  if (!m_uih)
    return;
  uih_saveundo(m_uih);
  uih_mkdefaultpalette(m_uih);
}

void EngineBridge::cyclePalette(bool enable) {
  if (!m_uih)
    return;
  if (enable)
    uih_cycling_on(m_uih);
  else
    uih_cycling_off(m_uih);
}

double EngineBridge::zoomSpeed() const {
  if (!m_uih)
    return 1.0;
  return (double)m_uih->speedup;
}

void EngineBridge::setZoomSpeed(double speed) {
  if (!m_uih)
    return;
  uih_setspeedup(m_uih, (number_t)speed);
  emit zoomSpeedChanged();
}

QString EngineBridge::statusMessage() const { return m_statusMessage; }

// Filters
void EngineBridge::enableFilter(int index) {
  if (!m_uih)
    return;
  uih_enablefilter(m_uih, index);
}

void EngineBridge::disableFilter(int index) {
  if (!m_uih)
    return;
  uih_disablefilter(m_uih, index);
}

// Save/Load
void EngineBridge::savePosition(const QString &path) {
  if (!m_uih)
    return;
  uih_saveposfile(m_uih, path.toUtf8().constData());
}

void EngineBridge::loadPosition(const QString &path) {
  if (!m_uih)
    return;
  uih_loadfile(m_uih, path.toUtf8().constData());
  emit formulaChanged();
  emit iterationsChanged();
}

void EngineBridge::loadRandomExample() {
  if (!m_uih)
    return;
  uih_saveundo(m_uih);
  uih_loadexample(m_uih);
  emit formulaChanged();
  emit iterationsChanged();
}

void EngineBridge::savePNG(const QString &path) {
  if (!m_uih)
    return;
  uih_savepngfile(m_uih, path.toUtf8().constData());
}

// Undo/Redo
void EngineBridge::undo() {
  if (!m_uih)
    return;
  uih_undo(m_uih);
  emit formulaChanged();
  emit iterationsChanged();
}

void EngineBridge::redo() {
  if (!m_uih)
    return;
  uih_redo(m_uih);
  emit formulaChanged();
  emit iterationsChanged();
}
