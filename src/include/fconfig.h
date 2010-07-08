#ifndef FCONFIG_H
#define FCONFIG_H
#include <config.h>
#define TUTORIALPATH DATAPATH"/tutorial/"
#define EXAMPLESPATH DATAPATH"/examples/"
#define CATALOGSPATH DATAPATH"/catalogs/"
#define HELPPATH DATAPATH"/help/xaos.hlp"

#define DEFAULT_MAX_ITER 170
/*default number of iterations should be set
				         lower for slow computers */
#define DEFAULT_BAILOUT  4
#define MAXSTEP		(0.008*3)
/*zooming step. For slow computer should
				          be set lower. Longer steps takes more time
				          and invoke yet longer ones etc.. */
#define STEP		(0.0006*3)
/*speedup */
#define XSIZE		640
/*default sizes of window */
#define YSIZE		480
#define ROTATIONSPEED   30



/*some constatnts used by various parts of XaoS */
#ifndef DEBUG
#define NDEBUG
#endif
#ifndef FRAMERATE		/*plan9 seems to require this */
#define FRAMERATE	20
#define LOOKSIZE 2
/*size explored by autopilot */
#define RANGE1 30
#define NGUESSES (RANGE1*RANGE1/2)
#define MAXTIME 10
/*maximum zooming time to one direction */
#define NGUESSES1 10
/*maximum number of guesses using first
				   method */
#define NGUESSES2 1000
/*maximum number of guesses using second
				   method */
#define SPEEDUP		1.05
/*speedup of speedup */
#endif
/*#define SLOWFUNCPTR*/

#ifdef _plan9_
#undef number_t
#define number_t double
#else
typedef FPOINT_TYPE number_t;
#endif

#undef STAT
#ifdef STATISTICS
#define STAT(x) x
#else
#define STAT(x)
#endif
#define NUMBER_BIG ((number_t)INT_MAX)
#endif				/*FCONFIG_H */
