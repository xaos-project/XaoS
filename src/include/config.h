#ifndef CONFIG_H
#define CONFIG_H

// XaoS release
#define XaoS_VERSION "4.2"

// URLs
#define HELP_URL "https://github.com/xaos-project/XaoS/wiki"
#define WEB_URL "http://xaos.sourceforge.net/"
#define DOWNLOAD_URL "https://github.com/xaos-project/XaoS/releases"
#define FEEDBACK_URL "https://github.com/xaos-project/XaoS/issues"
#define FORUM_URL "https://groups.google.com/d/forum/xaos-users"
#define FRACTALINFO_URL "https://github.com/xaos-project/XaoS/wiki/Fractal-Types#"

// File locations
#define DATAPATH "/usr/share/XaoS"
#define TUTORIALPATH DATAPATH "/tutorial/"
#define EXAMPLESPATH DATAPATH "/examples/"
#define CATALOGSPATH DATAPATH "/catalogs/"
#define HELPPATH DATAPATH "/help/xaos.hlp"

// Config file name
#ifdef _WIN32
#define CONFIGFILE "XaoS.cfg"
#else
#define CONFIGFILE ".XaoSrc"
#endif

// Optional features
#define USE_PTHREAD

// Numeric type
#ifdef USE_FLOAT128
typedef __float128 number_t;
#else
#ifdef USE_LONG_DOUBLE
typedef long double number_t;
#else
typedef double number_t;
#endif
#endif

// Supported color depths
#define STRUECOLOR
#define STRUECOLOR16 // required for edge detection and pseudo 3d

// Fractal defaults
#define DEFAULT_MAX_ITER 1000
#define DEFAULT_BAILOUT 4
#define MAXSTEP (0.008 * 3)
#define STEP (0.0006 * 3)
#define ROTATIONSPEED 30
#define FRAMERATE 20
#define SPEEDUP 1.05

// Autopilot configuration
#define LOOKSIZE 2 // size explored by autopilot
#define RANGE1 30
#define NGUESSES (RANGE1 * RANGE1 / 2)
#define MAXTIME 10     // maximum zooming time to one direction
#define NGUESSES1 10   // maximum number of guesses using first method
#define NGUESSES2 1000 // maximum number of guesses using second method

// Default user formula
// #define USER_FORMULA "z^log(c)*p"
// #define USER_FORMULA "c^z+im(p)*{0;1}"
#define USER_FORMULA "(abs(re(z))+i*abs(im(z)))^2+c"

// Disable optional statistics collection and reporting
//#define STATISTICS
#undef STAT
#ifdef STATISTICS
#define STAT(x) x
#else
#define STAT(x)
#endif

#define NUMBER_BIG ((number_t)INT_MAX)
#endif // CONFIG_H
