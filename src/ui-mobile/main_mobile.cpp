/*
 *     XaoS Mobile UI - Entry point
 *     Uses Qt Quick/QML instead of Qt Widgets
 */

#include <QFont>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>

#include "enginebridge.h"
#include "fractalquickitem.h"

// Engine globals needed by the fractal engine
#include "config.h"
#include "filter.h"
#include "fractal.h"
#include "param.h"
#include "timers.h"
#include "ui_helper.h"
#include "xmenu.h"
#include "xthread.h"

// Required global variables (same as desktop main.cpp)
int printspeed = 0;
int delaytime = 0;
int maxframerate = 80;
float pixelwidth = 0.0, pixelheight = 0.0;
extern tl_group *syncgroup; // defined in timers.cpp
extern int nthreads;        // defined in xthread.cpp

// Globals from desktop UI that engine code references
#include "xio.h"
#include <QStringList>

xio_pathdata configfile;
QStringList fnames = {};

// Params for engine
static const struct params params[] = {{NULL, 0, NULL, NULL}};

int nparams = 0;

void params_register(const struct params * /*par*/) {
  // Minimal implementation for mobile
}

// Required by engine but simplified for mobile
char *qt_gettext(const char * /*context*/, const char *text) {
  return const_cast<char *>(text);
}

void ui_unregistermenus(void) {
  // No-op for mobile
}

void ui_quit(int i) { QCoreApplication::exit(i); }

void ui_help(struct uih_context * /*uih*/) {}
void ui_download(struct uih_context * /*uih*/) {}
void ui_feedback(struct uih_context * /*uih*/) {}
void ui_forum(struct uih_context * /*uih*/) {}

void ui_about(struct uih_context * /*uih*/) {
  // TODO: Show About dialog via QML signal
}

void ui_font(struct uih_context * /*uih*/) {}

void uih_setlanguage(uih_context * /*c*/, int /*l*/) {}

void ui_fractalinfo(struct uih_context * /*uih*/) {}

// Menu system stubs - mobile uses QML UI instead
extern void uih_registermenus(void);
void ui_registermenus_i18n(void) {}

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  app.setApplicationName("XaoS");
  app.setApplicationVersion("4.3");
  app.setOrganizationName("GNU");

  // Initialize threading
  nthreads = 1; // Start with single thread on mobile

  // syncgroup is already statically initialized in timers.cpp

  // Register engine menus
  uih_registermenus();

  // Register QML types
  qmlRegisterType<FractalQuickItem>("XaoS", 1, 0, "FractalQuickItem");

  // Create engine bridge and initialize with safe default size
  EngineBridge engineBridge;
  engineBridge.init(640, 480);

  // Set up QML engine
  QQmlApplicationEngine engine;

  // Expose engine bridge to QML
  engine.rootContext()->setContextProperty("engineBridge", &engineBridge);

  // Load main QML
  engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

  if (engine.rootObjects().isEmpty()) {
    qCritical("Failed to load QML");
    return -1;
  }

  return app.exec();
}
