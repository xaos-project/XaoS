#include <config.h>
#ifndef _plan9_
#ifndef NO_MALLOC_H
#include <malloc.h>
#endif
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#else
#include <u.h>
#include <libc.h>
#include <stdio.h>
#endif
#define SLARGEITER
#include <filter.h>
#include <fractal.h>
#include <xthread.h>
/* 
 * This is an implementation of famous boundary trace algorithm.
 * See fractint documentation if you don't know what this means :)
 *
 * Here is two different implentation of this algorithm - one is faster
 * and second is threadable.
 *
 * An faster one is quite usual implementation - get first uncalculated pixel,
 * trace algorithm using labirinth "right hand rule" way (well, I currently
 * use left hand rule) then trace same boundary again and fill. It is
 * implemented in tracerectangle/tracecolor
 *
 * An threaded one follows description I sent to sci.fractals:
 Hi
 few weeks ago I asked for multithreaded algorithm for boundary trace
 I received following reply by Paul Derbyshire 
 > One method is this. One b-trace algorithm pushes pixels onto a stack.
 > Initially the screen border pixels are all pushed. Then a loop starts. A
 > pixel is popped and calculated or recalled, with 4 orthogonal neighbors
 > calculated or recalled if already calculated; if the central pixel is at a
 > color boundary the neighbors are pushed. The image is done when the stack
 > hits empty, then empty areas of image are floodfilled by their boundary
 > color. (Fractint does it differently, not sure how). By this method, the
 > stack will usually have at least 4 pixels, and so four substacks can be
 > assigned to each of 4 processors, and each processor has a "processor to
 > its left" designated as if they were in a "logical" ring. Each processor
 > pushes new pixels on the processor to its left's substack, and pops from
 > its own. This way, busy parts of the image wind up spread among all
 > processors. By adding substacks, this can be expanded to accomodate more
 > processors. Some amount is optimal, after which a point of diminishing
 > returns is reached when most processors only do a few pixels and spend
 > most of their time waiting for new stuff from the processor to its right.
 > You'll need to figure out this optimum somehow; I can't guess what it
 > would be. Probably around 64 processors. (More than that, you would do
 > well just to assign separate processors small rectangular subsets of the
 > image anyways.) Also, the end is only reached when NO processors have
 > anything in their stacks.
 This method looks very interesting but has few serious problems.
 Most significant probably is that it always caluclates pixels up
 to distance 3 from boundary. Simple modification should lower it
 to distance 2. But "right hand rule" based algorithm should actualy
 calculate points just to distance 1. So I want to implement such alrogithm,
 since number of caluclated points is still significant.

 So I think I have to extend stack for two informations:
 1) direction
 2) color of boundary I am tracking(not color I am at)
 and main algorithm should look like:
 1) detect color of current point
 2) look right. Is there same color? 
 yes:add point at the right to stack and exit
 is there boundary color?
 no:we are meeting boundary with different color - so we need
 to track this boundary too. add point at right to stack with oposite
 direction and boundary color set to current color
 3) look forward: similiar scheme as to look right
 4) look left: again similiar
 5) exit

 This hould trace boundaries to distance 1 (I hope) and do not miss anything.
 Problem is that this algorithm never ends, since boundaries will be rescaned
 again and again. So I need to add an caluclated array wich should looks like:
 for every point has mask for all directions that were scaned+calculated mask
 (set to 1 if pixel is already calculated)+inprocess mask(set to 1 if some
 other processor is currently calculating it)

 Scan masks should be set in thime when pixel is added to stack and pixel
 should not be added in case that mask is already set. I don't this that locks
 to this array is required since time spent by setting masks should be quite
 small and possible race conditions should not be drastical(except recalculating
 some point)

 I was also thinking about perCPU queues. I think that one queue is OK,
 it is simplier and should not take much more time(except it will be locked
 more often but time spend in locked queue in comparsion to time spent
 in rest should be so small so this should not be drastical for less than 10
 procesors)

 At the other hand - perCPU queues should have one advantage - each
 cpu should own part of image and points from its part will be added to 
 this cpu. This should avoid procesor cache conflict and speed up process.
 At the other hand, when cpu's queue is empty, cpu will have to browse
 others queues too and steal some points from other CPU, wich should introduce
 some slowdown and I am not sure if this way will bring any improvement.

 Other think that should vote for perCPU queue is fact, that in one CPU
 queue should be more often situations when queue is empty since one procesor
 is caluclating points and other has wait, since it had tendency to trace
 just one boundary at the time. At the other hand, most of boundaries should
 cross broders, so they should be added to queue at the initialization, so
 this should be OK.

 I am beginer to threds, SMP etc. So I looking for ideas, and suggestions
 to improve this alg. (or design different one).
 Also someone with SMP box, who should test me code is welcomed.
 BTW whats is the average/maximal number of CPU in todays SMP boxes?

 Please reply directly to my email:hubicka@paru.cas.cz

 Thanks 
 Honza
 * This way is implemented in tracerectangle2/tracepoint. It is enabled
 * just when threads are compiled in. Also when nthreads=1, old faster
 * algorithm is used.
 *
 * Implementation notes:
 * 1) I decided to use one queue instead of stack, since I expect, it will
 *    have tendency to trace all boundaries at the time, not just one.
 *    This will make queue bigger and reduce probability of situation, where
 *    queue is empty and other processors has to wait for one, that is
 *    calculating and should add something there (maybe :)
 * 2) Stack (queue :) is used just when neccesary - in situations where queue
 *    is quite full (there is more items than 10) procesor just continues in
 *    tracing path it started. This reduces number of slow stack operations,
 *    locks/unlocks, cache conflicts and other bad thinks. 
 * 3) Just each fourth pixel should be added into queue
 * 4) Foodfill algorithm should be avoided since colors at the boundaries
 *    are always correct, we should simply go trought each scanline and when
 *    pixel is uncalcualted, copy value from its left neighbor
 *
 * Current implementation has about 6% lower results than "fast" algorithm
 * using one thread. When two threads enabled (at my one processor linux
 * box) lock/unlock overhead eats next 8%, three threads eats next 1% :)
 */
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <archaccel.h>

#include <filter.h>
#include <btrace.h>
#include <plane.h>
#include "calculate.h"

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3
#define turnleft(d) (((d)+3)&3)
#define turnright(d) (((d)+1)&3)
#define turnoposite(d) (((d)+2)&3)
#define callwait() if(cfilter.wait_function!=NULL) cfilter.wait_function(&cfilter);


#ifndef nthreads
static int size;
static unsigned int inset;
static int nwaiting;
static int exitnow;
#define PAGESHIFT 14
#define PAGESIZE (1<<PAGESHIFT)
#define MAXPAGES 200		/*Well limit is about 6MB of stack..Hope it will never owerflow */
#define MAXSIZE (MAXPAGES*PAGESIZE-1)
struct stack
{
  int color;
  short x, y;
  char direction;
};
static int npages[MAXTHREADS];
static struct stack *pages[MAXPAGES];
static struct stack **starts[MAXTHREADS];
static int sizes[MAXTHREADS];
static int maxsize, maxsize2;
static CONST char dirrections[][2] = {
  {0, -1},
  {1, 0},
  {0, 1},
  {-1, 0},
};
#define addstack(sx,sy,d,c,periodicity) { \
  int page; \
  int nstack=(((sy)-ystart)*nthreads)/(yend-ystart+1); \
  struct stack *ptr; \
  calculated[sx+sy*CALCWIDTH]|=1<<d; \
  xth_lock(0); \
  if(size<maxsize2) {\
  while(sizes[nstack]>=maxsize) if(nstack>=nthreads-1) nstack=0; else nstack++; \
  page=sizes[nstack]>>PAGESHIFT; \
  if(page==npages[nstack]) starts[nstack][npages[nstack]]=(struct stack *)malloc(sizeof(struct stack)*PAGESIZE),npages[nstack]++;\
  ptr=starts[nstack][page]+(sizes[nstack]&(PAGESIZE-1)); \
  ptr->x=sx; \
  ptr->y=sy; \
  if(periodicity) \
  ptr->direction=d|8; else \
  ptr->direction=d; \
  ptr->color=c; \
  size++; \
  sizes[nstack]++; \
  if(nwaiting) xth_wakefirst(0); \
  } \
  xth_unlock(0); \
}
/*Non locking one used by init code */
#define addstack1(sx,sy,d,c) { \
  int page; \
  struct stack *ptr; \
  int nstack=(((sy)-y1)*nthreads)/(y2-y1+1); \
  calculated[sx+sy*CALCWIDTH]|=1<<d; \
  if(size<maxsize2) {\
  while(sizes[nstack]>=maxsize) if(nstack==nthreads-1) nstack=0; else nstack++; \
  page=sizes[nstack]>>PAGESHIFT; \
  if(page==npages[nstack]) starts[nstack][npages[nstack]]=(struct stack *)malloc(sizeof(struct stack)*PAGESIZE),npages[nstack]++; \
  ptr=starts[nstack][page]+(sizes[nstack]&(PAGESIZE-1)); \
  ptr->x=sx; \
  ptr->y=sy; \
  ptr->direction=d|8; \
  ptr->color=c; \
  size++; \
  sizes[nstack]++; \
  } \
}
static int xstart, ystart, xend, yend;
#endif

static unsigned char *calculated;
#define CALCULATED 16
#define CALCULATING 32
#define CALCWIDTH cimage.width

static number_t *xcoord, *ycoord;
#ifndef inline
REGISTERS (3)
     CONSTF static pixel32_t calculatepixel (int x, int y, int peri)
{
  return (calculate (xcoord[x], ycoord[y], peri));
}
#else
#define calculatepixel(x,y,peri) (calculate(xcoord[x],ycoord[y],peri))
#endif
#define putpixel(x,y,c) p_setp((cpixel_t *)cimage.currlines[y],x,c)
#define getpixel(x,y) p_getp((cpixel_t *)cimage.currlines[y],x)
#include <c256.h>
#define tracecolor tracecolor8
#define tracepoint tracepoint8
#define dosymetries dosymetries8
#define queue queue8
#define bfill bfill8
#include "btraced.c"
#include <hicolor.h>
#define tracecolor tracecolor16
#define tracepoint tracepoint16
#define dosymetries dosymetries16
#define queue queue16
#define bfill bfill16
#include "btraced.c"
#include <true24.h>
#define tracecolor tracecolor24
#define tracepoint tracepoint24
#define dosymetries dosymetries24
#define queue queue24
#define bfill bfill24
#include "btraced.c"
#include <truecolor.h>
#define tracecolor tracecolor32
#define tracepoint tracepoint32
#define dosymetries dosymetries32
#define queue queue32
#define bfill bfill32
#include "btraced.c"

#ifdef HAVE_GETTEXT
#include <libintl.h>
#else
#define gettext(STRING) STRING
#endif

#ifndef SLOWCACHESYNC
#ifndef nthreads
static int
tracerectangle2 (int x1, int y1, int x2, int y2)
{
  int x, y;
  cfilter.max = y2 - y1;
  cfilter.pass = gettext ("Boundary trace");
  cfilter.pos = 0;
  maxsize = MAXPAGES / nthreads;
  for (y = 0; y < nthreads; y++)
    {
      npages[y] = 0;		/*stack is empty */
      sizes[y] = 0;
      starts[y] = pages + y * maxsize;
    }
  maxsize *= PAGESIZE;
  maxsize2 = maxsize * nthreads;
  size = 0;
  nwaiting = 0;
  exitnow = 0;
  inset = cpalette.pixels[0];
  for (y = y1; y <= y2; y++)
    {
      memset_long (calculated + x1 + y * CALCWIDTH, 0, x2 - x1 + 1);
    }
  for (x = x1; x <= x2; x += 4)
    {
      addstack1 (x, y1, LEFT, INT_MAX);
      addstack1 (x, y2, RIGHT, INT_MAX);
    }
  for (y = y1; y <= y2; y += 4)
    {
      addstack1 (x1, y, DOWN, INT_MAX);
      addstack1 (x2, y, UP, INT_MAX);
    }
  xstart = x1;
  ystart = y1;
  xend = x2;
  yend = y2;
  switch (cimage.bytesperpixel)
    {
    case 1:
      xth_function (queue8, NULL, 1);
      xth_sync ();
      xth_function (bfill8, NULL, yend - ystart - 1);
      break;
#ifdef SUPPORT16
    case 2:
      xth_function (queue16, NULL, 1);
      xth_sync ();
      xth_function (bfill16, NULL, yend - ystart - 1);
      break;
#endif
#ifdef STRUECOLOR24
    case 3:
      xth_function (queue24, NULL, 1);
      xth_sync ();
      xth_function (bfill24, NULL, yend - ystart - 1);
      break;
#endif
    case 4:
      xth_function (queue32, NULL, 1);
      xth_sync ();
      xth_function (bfill32, NULL, yend - ystart - 1);
      break;
    }
  xth_sync ();
  for (y = 0; y < nthreads; y++)
    for (x = 0; x < npages[y]; x++)
      free (starts[y][x]);	/*free memory allocated for stack */
  return 1;
}
#endif
#endif
static void
skip (int x1, int y1, int x2, int y2)
{
  int src = y1;
  int xstart = x1 * cimage.bytesperpixel;
  int xsize = (x2 - x1 + 1) * cimage.bytesperpixel;
  y1++;
  for (; y1 <= y2; y1++)
    {
      memcpy (cimage.currlines[y1] + xstart, cimage.currlines[src] + xstart,
	      xsize);
      ycoord[y1] = ycoord[src];
    }
}
static int
tracerectangle (int x1, int y1, int x2, int y2)
{
  int x, y;
  unsigned char *calc;
  cfilter.max = y2 - y1;
  cfilter.pass = gettext ("Boundary trace");
  cfilter.pos = 0;
  for (y = y1; y <= y2; y++)
    {
      memset_long (calculated + x1 + y * CALCWIDTH, 0,
		   (size_t) (x2 - x1 + 1));
    }
  switch (cimage.bytesperpixel)
    {
    case 1:
      for (y = y1; y <= y2; y++)
	{
	  calc = calculated + y * CALCWIDTH;
	  for (x = x1; x <= x2; x++)
	    if (!calc[x])
	      {
		tracecolor8 (x1, y1, x2, y2, x, y);
	      }
	  cfilter.pos = y - y1;
	  callwait ();
	  if (cfilter.interrupt)
	    {
	      skip (x1, y, x2, y2);
	      return 0;
	    }
	}
      break;
#ifdef SUPPORT16
    case 2:
      for (y = y1; y <= y2; y++)
	{
	  calc = calculated + y * CALCWIDTH;
	  for (x = x1; x <= x2; x++)
	    if (!calc[x])
	      {
		tracecolor16 (x1, y1, x2, y2, x, y);
	      }
	  cfilter.pos = y - y1;
	  callwait ();
	  if (cfilter.interrupt)
	    {
	      skip (x1, y, x2, y2);
	      return 0;
	    }
	}
      break;
#endif
#ifdef STRUECOLOR24
    case 3:
      for (y = y1; y <= y2; y++)
	{
	  calc = calculated + y * CALCWIDTH;
	  for (x = x1; x <= x2; x++)
	    if (!calc[x])
	      {
		tracecolor24 (x1, y1, x2, y2, x, y);
	      }
	  cfilter.pos = y - y1;
	  callwait ();
	  if (cfilter.interrupt)
	    {
	      skip (x1, y, x2, y2);
	      return 0;
	    }
	}
#endif
    case 4:
      for (y = y1; y <= y2; y++)
	{
	  calc = calculated + y * CALCWIDTH;
	  for (x = x1; x <= x2; x++)
	    if (!calc[x])
	      {
		tracecolor32 (x1, y1, x2, y2, x, y);
	      }
	  cfilter.pos = y - y1;
	  callwait ();
	  if (cfilter.interrupt)
	    {
	      skip (x1, y, x2, y2);
	      return 0;
	    }
	}
      break;
    }
  return 1;
}

int
boundarytrace (int x1, int y1, int x2, int y2, number_t * xpos,
	       number_t * ypos)
{
  int i;
  int i1;
  int xsym, ysym;
  int cy1, cy2;
  int cx1, cx2;
  int ydiv;
#ifdef HAVE_ALLOCA
  calculated = (unsigned char *) alloca (cimage.width * (y2 + 1));
#else
  calculated = (unsigned char *) malloc (cimage.width * (y2 + 1));
#endif
  if (calculated == NULL)
    {
      return 0;
    }
  xcoord = xpos;
  ycoord = ypos;


  if (cursymetry.xsym < cfractalc.rs.nc || cursymetry.xsym > cfractalc.rs.mc)
    xsym = -10;
  else
    xsym =
      (int) (0.5 +
	     ((cursymetry.xsym -
	       cfractalc.rs.nc) * cimage.width / (cfractalc.rs.mc -
						  cfractalc.rs.nc)));
  if (cursymetry.ysym < cfractalc.rs.ni || cursymetry.ysym > cfractalc.rs.mi)
    ysym = -10;
  else
    ysym =
      (int) (0.5 +
	     ((cursymetry.ysym -
	       cfractalc.rs.ni) * cimage.height / (cfractalc.rs.mi -
						   cfractalc.rs.ni)));
  ydiv =
    (int) (0.5 +
	   ((-cfractalc.rs.ni) * cimage.height /
	    (cfractalc.rs.mi - cfractalc.rs.ni)));
  if (xsym > x1 && xsym < x2)
    {
      if (xsym - x1 > x2 - xsym)
	cx1 = x1, cx2 = xsym;
      else
	/*xsym--, */ cx1 = xsym, cx2 = x2;
    }
  else
    xsym = -1, cx1 = x1, cx2 = x2;
  if (ysym > y1 && ysym < y2)
    {
      if (ysym - y1 > y2 - ysym)
	cy1 = y1, cy2 = ysym;
      else
	cy1 = ysym, cy2 = y2;
    }
  else
    ysym = -1, cy1 = y1, cy2 = y2;
  for (i = cx1; i <= cx2; i++)
    {
      xcoord[i] =
	cfractalc.rs.nc + i * (cfractalc.rs.mc -
			       cfractalc.rs.nc) / cimage.width;
    }
  for (i = cy1; i <= cy2; i++)
    {
      ycoord[i] =
	cfractalc.rs.ni + i * (cfractalc.rs.mi -
			       cfractalc.rs.ni) / cimage.height;
    }
  i = 1;
#ifndef SLOWCACHESYNC
#ifndef nthreads
  if (nthreads != 1)
    {
      if (ydiv > cy1 && ydiv < cy2)
	{
	  i |= tracerectangle2 (cx1, cy1, cx2, ydiv),
	    i |= tracerectangle2 (cx1, ydiv, cx2, cy2);
	}
      else
	i |= tracerectangle2 (cx1, cy1, cx2, cy2);
    }
  else
#endif
#endif
    {
      if (ydiv > cy1 && ydiv < cy2)
	{
	  i |= tracerectangle (cx1, cy1, cx2, ydiv),
	    i |= tracerectangle (cx1, ydiv, cx2, cy2);
	}
      else
	i |= tracerectangle (cx1, cy1, cx2, cy2);
    }
  if (!i)
    {
#ifndef HAVE_ALLOCA
      free (calculated);
#endif
      return 0;
    }
#ifndef HAVE_ALLOCA
  free (calculated);
#endif
  drivercall (cimage,
	      dosymetries8 (x1, x2, y1, y2, xsym, cx1, cx2),
	      dosymetries16 (x1, x2, y1, y2, xsym, cx1, cx2),
	      dosymetries24 (x1, x2, y1, y2, xsym, cx1, cx2),
	      dosymetries32 (x1, x2, y1, y2, xsym, cx1, cx2));
  for (i = cx1; i <= cx2; i++)
    {
      if (xsym != -1)
	{
	  i1 = 2 * xsym - i;
	  if (i1 >= x1 && i1 <= x2 && i != i1)
	    xcoord[i1] = 2 * cursymetry.xsym - xcoord[i];
	}
    }
  for (i = cy1; i <= cy2; i++)
    {
      if (ysym != -1)
	{
	  i1 = 2 * ysym - i;
	  if (i1 >= y1 && i1 <= y2 && i != i1)
	    ycoord[i1] = 2 * cursymetry.ysym - ycoord[i];
	}
    }
  if (cy1 != y1)
    {
      register int yy1, yy2;
      int xstart = x1 * cimage.bytesperpixel;
      int xsize = (x2 - x1 + 1) * cimage.bytesperpixel;
      yy1 = y1;
      yy2 = 2 * ysym - y1;
      while (yy1 < yy2)
	{
	  memcpy (cimage.currlines[yy1] + xstart,
		  cimage.currlines[yy2] + xstart, (size_t) xsize);
	  yy1++;
	  yy2--;
	}
    }
  if (cy2 != y2)
    {
      register int yy1, yy2;
      int xstart = x1 * cimage.bytesperpixel;
      int xsize = (x2 - x1 + 1) * cimage.bytesperpixel;
      yy1 = y2;
      yy2 = 2 * ysym - y2;
      while (yy1 > yy2)
	{
	  memcpy (cimage.currlines[yy1] + xstart,
		  cimage.currlines[yy2] + xstart, (size_t) xsize);
	  yy1--;
	  yy2++;
	}
    }
  return 1;
}

int
boundarytraceall (number_t * xpos, number_t * ypos)
{
  return (boundarytrace
	  (0, 0, cimage.width - 1, cimage.height - 1, xpos, ypos));
}
