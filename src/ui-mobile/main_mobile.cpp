/*
 *     XaoS Mobile UI - Entry point
 *
 *     Uses QApplication + QWidget (FractalWidget) for fractal rendering
 *     with a transparent QQuickWidget overlay for mobile touch controls.
 */

#include <QApplication>
#include <QScreen>
#include <QThread>

#include "mobilemainwindow.h"

// Engine headers
#include "config.h"
#include "filter.h"
#include "fractal.h"
#include "param.h"
#include "timers.h"
#include "ui_helper.h"
#include "xmenu.h"
#include "xthread.h"

// Required global variables
int printspeed = 0;
int delaytime = 0;
int maxframerate = 80;
float pixelwidth = 0.0, pixelheight = 0.0;
extern tl_group *syncgroup;
extern int nthreads;
extern int defthreads;

#include "xio.h"
#include <QStringList>
xio_pathdata configfile;
QStringList fnames = {};

// Minimal stubs required by the engine
int nparams = 0;
void params_register(const struct params * /*par*/) {}

char *qt_gettext(const char * /*ctx*/, const char *text) {
  return const_cast<char *>(text);
}

void ui_unregistermenus(void) {}
void ui_quit(int i) { QCoreApplication::exit(i); }
void ui_help(struct uih_context *) {}
void ui_download(struct uih_context *) {}
void ui_feedback(struct uih_context *) {}
void ui_forum(struct uih_context *) {}
void ui_about(struct uih_context *) {}
void ui_font(struct uih_context *) {}
void uih_setlanguage(uih_context *, int) {}
void ui_fractalinfo(struct uih_context *) {}
void ui_registermenus_i18n(void) {}

extern void uih_registermenus(void);

int main(int argc, char *argv[]) {
  // QApplication — required for QWidget support
  QApplication app(argc, argv);
  app.setApplicationName("XaoS");
  app.setApplicationVersion(XaoS_VERSION);
  app.setOrganizationName("GNU");

  // Multi-threading: auto-detect cores, cap at 4 for mobile
  int idealThreads = QThread::idealThreadCount();
  if (idealThreads <= 0)
    idealThreads = 1;
  defthreads = qMin(idealThreads, 4);
  xth_init(defthreads);

  // Register engine menus
  uih_registermenus();

  // Create the mobile window and initialize
  MobileMainWindow window;
  window.init();

  // Enter the event loop — this drives the fractal rendering
  // at maximum frame rate using QTimer(0)
  window.eventLoop();

  return 0;
}
